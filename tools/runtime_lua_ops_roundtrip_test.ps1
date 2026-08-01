param(
    [Parameter(Mandatory = $true)]
    [string] $Executable,

    [ValidateSet(
        "mixed",
        "official",
        "metallurgy",
        "biology",
        "chemistry",
        "nuclear",
        "periodic",
        "electronics"
    )]
    [string] $Scenario = "mixed",

    [ValidateRange(1, 180)]
    [int] $TimeoutSeconds = 30,

    [string] $TemporaryDirectory = [System.IO.Path]::GetTempPath(),

    [switch] $KeepArtifacts
)

$ErrorActionPreference = "Stop"
$resolvedExecutable = (Resolve-Path -LiteralPath $Executable).Path
$scriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$autorunSource = Join-Path $scriptRoot "runtime\ops_roundtrip_regression.lua"
if (-not (Test-Path -LiteralPath $autorunSource -PathType Leaf)) {
    throw "Missing Lua OPS roundtrip script: $autorunSource"
}

$tempParent = [System.IO.Path]::GetFullPath($TemporaryDirectory)
if (-not (Test-Path -LiteralPath $tempParent -PathType Container)) {
    throw "Temporary directory does not exist: $tempParent"
}
$testRoot = Join-Path $tempParent (
    "tpt-omnipack-ops-roundtrip-" + $Scenario + "-" +
    [guid]::NewGuid().ToString("N")
)
$resolvedTestRoot = [System.IO.Path]::GetFullPath($testRoot)
$tempPrefix = $tempParent.TrimEnd(
    [System.IO.Path]::DirectorySeparatorChar,
    [System.IO.Path]::AltDirectorySeparatorChar
) + [System.IO.Path]::DirectorySeparatorChar
if (-not $resolvedTestRoot.StartsWith(
    $tempPrefix,
    [System.StringComparison]::OrdinalIgnoreCase
)) {
    throw "Refusing to create an OPS runtime test outside the temporary directory"
}

function Remove-OpsTestRoot {
    if (-not (Test-Path -LiteralPath $resolvedTestRoot)) { return }
    for ($attempt = 1; $attempt -le 20; $attempt++) {
        try {
            Remove-Item -LiteralPath $resolvedTestRoot -Recurse -Force -ErrorAction Stop
            return
        }
        catch {
            if ($attempt -eq 20) {
                Write-Warning (
                    "Could not remove OPS temporary directory after 20 attempts: " +
                    "$resolvedTestRoot ($($_.Exception.Message))"
                )
                return
            }
            Start-Sleep -Milliseconds 100
        }
    }
}

function Get-ResultValue {
    param(
        [Parameter(Mandatory = $true)]
        [string] $Text,

        [Parameter(Mandatory = $true)]
        [string] $Key
    )

    $match = [regex]::Match(
        $Text,
        "(?m)^" + [regex]::Escape($Key) + "=([^\r\n]+)\r?$"
    )
    if (-not $match.Success) {
        throw "Missing $Key in phase result"
    }
    return $match.Groups[1].Value
}

function Invoke-OpsPhase {
    param(
        [Parameter(Mandatory = $true)]
        [ValidateRange(1, 3)]
        [int] $Phase
    )

    $phasePath = Join-Path $resolvedTestRoot "ops-roundtrip.phase"
    [System.IO.File]::WriteAllText(
        $phasePath,
        [string]$Phase,
        [System.Text.Encoding]::ASCII
    )
    $resultPath = Join-Path $resolvedTestRoot (
        "ops-roundtrip-phase" + $Phase + ".result"
    )
    if (Test-Path -LiteralPath $resultPath) {
        Remove-Item -LiteralPath $resultPath -Force
    }

    $startInfo = [System.Diagnostics.ProcessStartInfo]::new()
    $startInfo.FileName = $resolvedExecutable
    $startInfo.WorkingDirectory = $resolvedTestRoot
    $startInfo.UseShellExecute = $false
    $startInfo.CreateNoWindow = $true
    $startInfo.ArgumentList.Add("ddir")
    $startInfo.ArgumentList.Add($resolvedTestRoot)
    foreach ($secretName in @("GITHUB_PAT_TOKEN", "GITHUB_TOKEN", "GH_TOKEN")) {
        [void]$startInfo.Environment.Remove($secretName)
    }

    $process = [System.Diagnostics.Process]::Start($startInfo)
    if (-not $process) {
        throw "Failed to start the client for OPS phase $Phase"
    }

    try {
    $deadline = [DateTime]::UtcNow.AddSeconds($TimeoutSeconds)
    $resultText = ""
    do {
        Start-Sleep -Milliseconds 100
        if (Test-Path -LiteralPath $resultPath -PathType Leaf) {
            $resultText = [string](Get-Content -LiteralPath $resultPath -Raw)
            if ($resultText -match "(?m)^OMNI_OPS_STATUS=(PASS|FAIL)\r?$") {
                break
            }
        }
        $process.Refresh()
    } while ([DateTime]::UtcNow -lt $deadline -and -not $process.HasExited)

    if (-not $process.HasExited) {
        $remainingMilliseconds = [Math]::Max(
            0,
            [int]([DateTime]::UtcNow.Subtract($deadline).Negate().TotalMilliseconds)
        )
        if ($remainingMilliseconds -gt 0) {
            [void]$process.WaitForExit($remainingMilliseconds)
        }
    }

    if (-not $process.HasExited) {
        Stop-Process -Id $process.Id -Force
        $process.WaitForExit()
        throw (
            "OPS phase $Phase wrote a result but did not exit before timeout; " +
            "artifacts=$resolvedTestRoot"
        )
    }

    if (-not $resultText -and (Test-Path -LiteralPath $resultPath -PathType Leaf)) {
        $resultText = [string](Get-Content -LiteralPath $resultPath -Raw)
    }
    if ($resultText -notmatch "(?m)^OMNI_OPS_STATUS=PASS\r?$") {
        throw (
            "Lua OPS phase $Phase failed; exit_code=$($process.ExitCode); " +
            "result=$resultText; artifacts=$resolvedTestRoot"
        )
    }
    if ($process.ExitCode -ne 0) {
        throw (
            "Lua OPS phase $Phase reported PASS but exited with " +
            "$($process.ExitCode); artifacts=$resolvedTestRoot"
        )
    }

    $resultScenario = Get-ResultValue -Text $resultText -Key (
        "OMNI_OPS_SCENARIO"
    )
    if ($resultScenario -ne $Scenario) {
        throw (
            "OPS phase $Phase reported scenario '$resultScenario', expected " +
            "'$Scenario'; artifacts=$resolvedTestRoot"
        )
    }

    return [pscustomobject]@{
        Phase = $Phase
        ProcessId = $process.Id
        Text = $resultText.Trim()
        Stamp = Get-ResultValue -Text $resultText -Key "OMNI_OPS_STAMP"
    }
    }
    finally {
        if ($process) {
            if (-not $process.HasExited) {
                Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue
                $process.WaitForExit()
            }
            $process.Dispose()
        }
    }
}

function Test-OpsFile {
    param(
        [Parameter(Mandatory = $true)]
        [string] $Stamp
    )

    if ($Stamp -notmatch "^[0-9A-Fa-f]{10}$") {
        throw "Unsafe or invalid stamp ID returned by client: $Stamp"
    }
    $stampPath = Join-Path $resolvedTestRoot ("stamps\" + $Stamp + ".stm")
    $resolvedStampPath = (Resolve-Path -LiteralPath $stampPath).Path
    $stampPrefix = (Join-Path $resolvedTestRoot "stamps").TrimEnd(
        [System.IO.Path]::DirectorySeparatorChar,
        [System.IO.Path]::AltDirectorySeparatorChar
    ) + [System.IO.Path]::DirectorySeparatorChar
    if (-not $resolvedStampPath.StartsWith(
        $stampPrefix,
        [System.StringComparison]::OrdinalIgnoreCase
    )) {
        throw "Resolved OPS file escaped the isolated stamps directory"
    }

    [byte[]]$bytes = [System.IO.File]::ReadAllBytes($resolvedStampPath)
    if ($bytes.Length -le 15) {
        throw "OPS file is too short: $resolvedStampPath"
    }
    $magic = [System.Text.Encoding]::ASCII.GetString($bytes, 0, 4)
    if ($magic -ne "OPS1") {
        throw "Expected OPS1 magic, got '$magic': $resolvedStampPath"
    }
    $compressionMagic = [System.Text.Encoding]::ASCII.GetString($bytes, 12, 3)
    if ($compressionMagic -ne "BZh") {
        throw (
            "Expected a bzip2-compressed OPS payload, got " +
            "'$compressionMagic': $resolvedStampPath"
        )
    }
    $uncompressedLength = [System.BitConverter]::ToUInt32($bytes, 8)
    if ($uncompressedLength -eq 0 -or $uncompressedLength -gt 209715200) {
        throw "OPS file declared an invalid payload length: $uncompressedLength"
    }
    $hash = (Get-FileHash -LiteralPath $resolvedStampPath -Algorithm SHA256).Hash
    return [pscustomobject]@{
        Path = $resolvedStampPath
        Length = $bytes.Length
        PayloadLength = $uncompressedLength
        Sha256 = $hash
    }
}

$passed = $false
try {
    New-Item -ItemType Directory -Path $resolvedTestRoot | Out-Null
    Copy-Item -LiteralPath $autorunSource -Destination (
        Join-Path $resolvedTestRoot "autorun.lua"
    )
    [System.IO.File]::WriteAllText(
        (Join-Path $resolvedTestRoot "ops-roundtrip.scenario"),
        $Scenario,
        [System.Text.Encoding]::ASCII
    )

    $phase1 = Invoke-OpsPhase -Phase 1
    $ops1 = Test-OpsFile -Stamp $phase1.Stamp

    $phase2 = Invoke-OpsPhase -Phase 2
    if ($phase2.Stamp -eq $phase1.Stamp) {
        throw "Client reused the first stamp name for the second save"
    }
    $ops2 = Test-OpsFile -Stamp $phase2.Stamp

    $phase3 = Invoke-OpsPhase -Phase 3
    if ($phase3.Stamp -ne $phase2.Stamp) {
        throw "Final restart did not verify the second saved OPS file"
    }

    $consistencyKeys = @(
        "OMNI_OPS_PARTICLES",
        "OMNI_OPS_FIELD_ASSERTIONS",
        "OMNI_OPS_DIRECT_GT255",
        "OMNI_OPS_DIRECT_ELEMENTS",
        "OMNI_OPS_CTYPE_CARRIERS",
        "OMNI_OPS_TMP_CARRIERS",
        "OMNI_OPS_TMP2_CARRIERS",
        "OMNI_OPS_STABLE_IDENTIFIER_COUNT",
        "OMNI_OPS_STABLE_IDENTIFIERS"
    )
    foreach ($key in $consistencyKeys) {
        $firstValue = Get-ResultValue -Text $phase1.Text -Key $key
        $secondValue = Get-ResultValue -Text $phase2.Text -Key $key
        $thirdValue = Get-ResultValue -Text $phase3.Text -Key $key
        if ($firstValue -ne $secondValue -or $secondValue -ne $thirdValue) {
            throw (
                "$key changed across the three OPS processes: " +
                "phase1=$firstValue; phase2=$secondValue; phase3=$thirdValue"
            )
        }
    }

    $firstPaletteCount = Get-ResultValue `
        -Text $phase1.Text `
        -Key "OMNI_OPS_PALETTE_IDENTIFIERS"
    $secondPaletteCount = Get-ResultValue `
        -Text $phase2.Text `
        -Key "OMNI_OPS_PALETTE_IDENTIFIERS"
    if ($firstPaletteCount -ne $secondPaletteCount) {
        throw (
            "OPS palette identifier count changed after resave: " +
            "first=$firstPaletteCount; second=$secondPaletteCount"
        )
    }

    $stampIndexPath = Join-Path $resolvedTestRoot "stamps\stamps.json"
    if (-not (Test-Path -LiteralPath $stampIndexPath -PathType Leaf)) {
        throw "Client did not persist its isolated stamp index"
    }
    $stampIndex = Get-Content -LiteralPath $stampIndexPath -Raw | ConvertFrom-Json
    $indexedStamps = @($stampIndex.MostRecentlyUsedFirst)
    if ($phase1.Stamp -notin $indexedStamps -or $phase2.Stamp -notin $indexedStamps) {
        throw "Client stamp index does not contain both OPS files"
    }

    $aggregatePath = Join-Path $resolvedTestRoot "ops-roundtrip.result"
    $particleCount = Get-ResultValue `
        -Text $phase3.Text `
        -Key "OMNI_OPS_PARTICLES"
    $fieldAssertions = Get-ResultValue `
        -Text $phase3.Text `
        -Key "OMNI_OPS_FIELD_ASSERTIONS"
    $directGt255 = Get-ResultValue `
        -Text $phase3.Text `
        -Key "OMNI_OPS_DIRECT_GT255"
    $directElements = Get-ResultValue `
        -Text $phase3.Text `
        -Key "OMNI_OPS_DIRECT_ELEMENTS"
    $ctypeCarriers = Get-ResultValue `
        -Text $phase3.Text `
        -Key "OMNI_OPS_CTYPE_CARRIERS"
    $tmpCarriers = Get-ResultValue `
        -Text $phase3.Text `
        -Key "OMNI_OPS_TMP_CARRIERS"
    $tmp2Carriers = Get-ResultValue `
        -Text $phase3.Text `
        -Key "OMNI_OPS_TMP2_CARRIERS"
    $stableIdentifierCount = Get-ResultValue `
        -Text $phase3.Text `
        -Key "OMNI_OPS_STABLE_IDENTIFIER_COUNT"
    $stableIdentifiers = Get-ResultValue `
        -Text $phase3.Text `
        -Key "OMNI_OPS_STABLE_IDENTIFIERS"
    $aggregateLines = @(
        "OMNI_OPS_ROUNDTRIP_STATUS=PASS",
        "OMNI_OPS_SCENARIO=$Scenario",
        "OMNI_OPS_CONTAINER=stamp",
        "OMNI_OPS_FORMAT=OPS1",
        "OMNI_OPS_PROCESS_COUNT=3",
        "OMNI_OPS_RESTART_COUNT=2",
        "OMNI_OPS_LOAD_VERIFICATIONS=2",
        "OMNI_OPS_PARTICLES=$particleCount",
        "OMNI_OPS_FIELD_ASSERTIONS_PER_LOAD=$fieldAssertions",
        "OMNI_OPS_DIRECT_GT255=$directGt255",
        "OMNI_OPS_DIRECT_ELEMENTS=$directElements",
        "OMNI_OPS_CTYPE_CARRIERS=$ctypeCarriers",
        "OMNI_OPS_TMP_CARRIERS=$tmpCarriers",
        "OMNI_OPS_TMP2_CARRIERS=$tmp2Carriers",
        "OMNI_OPS_STABLE_IDENTIFIER_COUNT=$stableIdentifierCount",
        "OMNI_OPS_STABLE_IDENTIFIERS=$stableIdentifiers",
        "OMNI_OPS_PALETTE_IDENTIFIERS=$secondPaletteCount",
        "OMNI_OPS_FIRST_STAMP=$($phase1.Stamp)",
        "OMNI_OPS_FIRST_SIZE=$($ops1.Length)",
        "OMNI_OPS_FIRST_PAYLOAD_SIZE=$($ops1.PayloadLength)",
        "OMNI_OPS_FIRST_SHA256=$($ops1.Sha256)",
        "OMNI_OPS_SECOND_STAMP=$($phase2.Stamp)",
        "OMNI_OPS_SECOND_SIZE=$($ops2.Length)",
        "OMNI_OPS_SECOND_PAYLOAD_SIZE=$($ops2.PayloadLength)",
        "OMNI_OPS_SECOND_SHA256=$($ops2.Sha256)"
    )
    [System.IO.File]::WriteAllLines(
        $aggregatePath,
        $aggregateLines,
        [System.Text.Encoding]::ASCII
    )

    $passed = $true
    Write-Output "runtime-lua-ops-roundtrip-test: PASS"
    Write-Output ($aggregateLines -join [Environment]::NewLine)
    if ($KeepArtifacts) {
        Write-Output "OMNI_OPS_ARTIFACTS=$resolvedTestRoot"
    }
}
finally {
    if (
        $passed -and
        -not $KeepArtifacts -and
        (Test-Path -LiteralPath $resolvedTestRoot)
    ) {
        Remove-OpsTestRoot
    }
}
