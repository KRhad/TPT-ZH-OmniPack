param(
    [Parameter(Mandatory = $true)]
    [string] $Executable,

    [ValidateSet("Generate", "Verify", "All")]
    [string] $Mode = "All",

    [string] $ExamplesDirectory = (Join-Path (Split-Path -Parent $MyInvocation.MyCommand.Path) "..\..\examples\0.2.0"),

    [string] $TemporaryDirectory = [System.IO.Path]::GetTempPath(),

    [ValidateRange(1, 180)]
    [int] $TimeoutSeconds = 45,

    [switch] $KeepArtifacts
)

$ErrorActionPreference = "Stop"
$resolvedExecutable = (Resolve-Path -LiteralPath $Executable).Path
$scriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$sourceRoot = (Resolve-Path -LiteralPath (Join-Path $scriptRoot "..")).Path
$autorunSource = Join-Path $scriptRoot "runtime\examples_regression.lua"
$specPath = Join-Path $sourceRoot "examples\0.2.0\example-spec.json"
$tutorialPath = Join-Path $sourceRoot "docs\TUTORIALS_0.2.json"
foreach ($required in @($autorunSource, $specPath, $tutorialPath)) {
    if (-not (Test-Path -LiteralPath $required -PathType Leaf)) {
        throw "Missing examples input: $required"
    }
}

$resolvedExamples = [System.IO.Path]::GetFullPath($ExamplesDirectory)
$resolvedTempParent = [System.IO.Path]::GetFullPath($TemporaryDirectory)
if (-not (Test-Path -LiteralPath $resolvedTempParent -PathType Container)) {
    throw "Temporary directory does not exist: $resolvedTempParent"
}
New-Item -ItemType Directory -Path $resolvedExamples -Force | Out-Null
$spec = Get-Content -LiteralPath $specPath -Raw | ConvertFrom-Json
$tutorials = Get-Content -LiteralPath $tutorialPath -Raw | ConvertFrom-Json
$examples = @($spec.examples)
$challengeRows = @($tutorials.tutorials)
if ($examples.Count -ne 7) {
    throw "Expected exactly 7 examples, found $($examples.Count)"
}
if ($challengeRows.Count -ne 8) {
    throw "Expected exactly 8 tutorials, found $($challengeRows.Count)"
}

function Read-KeyValueFile {
    param([Parameter(Mandatory = $true)][string] $Path)
    $values = @{}
    foreach ($line in Get-Content -LiteralPath $Path) {
        if ($line -match '^([A-Za-z0-9_]+)=(.*)$') {
            $values[$matches[1]] = $matches[2]
        }
    }
    return $values
}

function New-IsolatedRoot {
    param([Parameter(Mandatory = $true)][string] $Label)
    $root = [System.IO.Path]::GetFullPath((Join-Path $resolvedTempParent (
        "tpt-omnipack-examples-$Label-" + [guid]::NewGuid().ToString("N"))))
    $prefix = $resolvedTempParent.TrimEnd(
        [System.IO.Path]::DirectorySeparatorChar,
        [System.IO.Path]::AltDirectorySeparatorChar) + [System.IO.Path]::DirectorySeparatorChar
    if (-not $root.StartsWith($prefix, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to create examples directory outside temporary root"
    }
    New-Item -ItemType Directory -Path $root | Out-Null
    return $root
}

function Remove-IsolatedRoot {
    param([Parameter(Mandatory = $true)][string] $Root)
    if (-not (Test-Path -LiteralPath $Root)) { return }
    for ($attempt = 1; $attempt -le 20; $attempt++) {
        try {
            Remove-Item -LiteralPath $Root -Recurse -Force -ErrorAction Stop
            return
        }
        catch {
            if ($attempt -eq 20) {
                Write-Warning "Could not remove examples temporary directory after 20 attempts: $Root ($($_.Exception.Message))"
                return
            }
            Start-Sleep -Milliseconds 100
        }
    }
}

function Invoke-ExampleClient {
    param(
        [Parameter(Mandatory = $true)][string] $CaseId,
        [Parameter(Mandatory = $true)][ValidateSet("generate", "verify")][string] $ClientMode,
        [string] $StampId,
        [string] $ChallengeId,
        [string] $StampPath,
        [switch] $PreserveRoot
    )
    $label = ($CaseId -replace '[^A-Za-z0-9_-]', '_')
    $root = New-IsolatedRoot -Label $label
    $process = $null
    try {
        Copy-Item -LiteralPath $autorunSource -Destination (Join-Path $root "autorun.lua")
        [System.IO.File]::WriteAllText((Join-Path $root "examples.mode"), $ClientMode + "`n", [System.Text.Encoding]::ASCII)
        [System.IO.File]::WriteAllText((Join-Path $root "examples.case"), $CaseId + "`n", [System.Text.Encoding]::ASCII)
        if ($StampId) {
            [System.IO.File]::WriteAllText((Join-Path $root "examples.stamp"), $StampId + "`n", [System.Text.Encoding]::ASCII)
        }
        if ($StampPath) {
            if (-not $StampId) { throw "StampPath requires StampId" }
            $stampDirectory = Join-Path $root "stamps"
            New-Item -ItemType Directory -Path $stampDirectory -Force | Out-Null
            Copy-Item -LiteralPath $StampPath -Destination (Join-Path $stampDirectory ($StampId + ".stm")) -Force
        }
        if ($ChallengeId) {
            [System.IO.File]::WriteAllText((Join-Path $root "examples.challenge"), $ChallengeId + "`n", [System.Text.Encoding]::ASCII)
        }

        $startInfo = [System.Diagnostics.ProcessStartInfo]::new()
        $startInfo.FileName = $resolvedExecutable
        $startInfo.WorkingDirectory = $root
        $startInfo.UseShellExecute = $false
        $startInfo.CreateNoWindow = $true
        $startInfo.ArgumentList.Add("ddir")
        $startInfo.ArgumentList.Add($root)
        foreach ($secretName in @("GITHUB_PAT_TOKEN", "GITHUB_TOKEN", "GH_TOKEN")) {
            [void]$startInfo.Environment.Remove($secretName)
        }
        $process = [System.Diagnostics.Process]::Start($startInfo)
        if (-not $process) { throw "Failed to start examples client" }

        $resultPath = Join-Path $root "examples.result"
        $deadline = [DateTime]::UtcNow.AddSeconds($TimeoutSeconds)
        $terminatedAfterPass = $false
        while (-not $process.HasExited -and [DateTime]::UtcNow -lt $deadline) {
            Start-Sleep -Milliseconds 100
            if (Test-Path -LiteralPath $resultPath -PathType Leaf) {
                $text = [string](Get-Content -LiteralPath $resultPath -Raw)
                if ($text -match '(?m)^OMNI_EXAMPLE_STATUS=(PASS|FAIL)\r?$') { break }
            }
            $process.Refresh()
        }
        if (-not (Test-Path -LiteralPath $resultPath -PathType Leaf)) {
            if (-not $process.HasExited) {
                Stop-Process -Id $process.Id -Force
                $process.WaitForExit()
            }
            throw "Examples client timed out without result; artifacts=$root"
        }
        $resultText = [string](Get-Content -LiteralPath $resultPath -Raw)
        if (-not $process.HasExited) {
            $terminatedAfterPass = $resultText -match '(?m)^OMNI_EXAMPLE_STATUS=PASS\r?$'
            Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue
            $process.WaitForExit()
        }
        $values = Read-KeyValueFile -Path $resultPath
        if (($process.ExitCode -ne 0 -and -not $terminatedAfterPass) -or $values.OMNI_EXAMPLE_STATUS -ne "PASS") {
            throw "Examples client failed; case=$CaseId; challenge=$ChallengeId; result=$resultText; artifacts=$root"
        }
        return [pscustomobject]@{ Root = $root; Values = $values; Text = $resultText.Trim() }
    }
    catch {
        if ($KeepArtifacts) { Write-Output "examples_failure_artifacts=$root" }
        throw
    }
    finally {
        if ($process) {
            if (-not $process.HasExited) {
                Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue
                $process.WaitForExit()
            }
            $process.Dispose()
        }
        if (-not $KeepArtifacts -and -not $PreserveRoot -and (Test-Path -LiteralPath $root)) {
            Remove-IsolatedRoot -Root $root
        }
    }
}

function Get-GitHead {
    $head = (& git -C $sourceRoot rev-parse HEAD).Trim()
    if ($LASTEXITCODE -ne 0 -or $head -notmatch '^[0-9a-f]{40}$') {
        throw "Cannot resolve source commit"
    }
    return $head
}

function Generate-Examples {
    $generated = @()
    $ordinal = 1
    foreach ($example in $examples) {
        $result = Invoke-ExampleClient -CaseId $example.id -ClientMode "generate" -PreserveRoot
        $stamp = $result.Values.OMNI_EXAMPLE_STAMP
        if ($stamp -notmatch '^[0-9A-Fa-f]{10}$') {
            throw "Invalid generated stamp for $($example.id): $stamp"
        }
        $sourceStamp = Join-Path $result.Root ("stamps\" + $stamp + ".stm")
        if (-not (Test-Path -LiteralPath $sourceStamp -PathType Leaf)) {
            throw "Generated stamp file is missing for $($example.id): $sourceStamp"
        }
        $destination = Join-Path $resolvedExamples $example.filename
        Copy-Item -LiteralPath $sourceStamp -Destination $destination -Force
        if (-not $KeepArtifacts -and (Test-Path -LiteralPath $result.Root)) {
            Remove-IsolatedRoot -Root $result.Root
        }
        $hash = (Get-FileHash -LiteralPath $destination -Algorithm SHA256).Hash
        $stableStamp = ("02{0:X8}" -f $ordinal).ToLowerInvariant()
        $generated += [ordered]@{
            id = $example.id
            filename = $example.filename
            stamp_id = $stableStamp
            source_stamp_id = $stamp
            sha256 = $hash
            bytes = (Get-Item -LiteralPath $destination).Length
            initial_particles = [int]$result.Values.OMNI_EXAMPLE_INITIAL_PARTICLES
            generation_order = $ordinal
            purpose = $example.purpose
        }
        $ordinal++
        Write-Output "example-generated=$($example.id) stamp=$stamp sha256=$hash"
    }
    $manifest = [ordered]@{
        schema_version = 1
        content_version = $spec.content_version
        source_commit = Get-GitHead
        generator_exe_sha256 = (Get-FileHash -LiteralPath $resolvedExecutable -Algorithm SHA256).Hash
        generated_utc = [DateTime]::UtcNow.ToString("o")
        examples = $generated
        tutorial_ids = @($challengeRows | ForEach-Object { $_.id })
    }
    $manifestPath = Join-Path $resolvedExamples "manifest.json"
    [System.IO.File]::WriteAllText(
        $manifestPath,
        ($manifest | ConvertTo-Json -Depth 8) + [Environment]::NewLine,
        [System.Text.UTF8Encoding]::new($false))
    Write-Output "examples-manifest=$manifestPath"
    return $manifest
}

function Verify-Examples {
    $manifestPath = Join-Path $resolvedExamples "manifest.json"
    if (-not (Test-Path -LiteralPath $manifestPath -PathType Leaf)) {
        throw "Missing examples manifest: $manifestPath"
    }
    $manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
    $manifestExamples = @($manifest.examples)
    if ($manifestExamples.Count -ne 7) { throw "Manifest must contain 7 examples" }
    if (@($manifest.tutorial_ids).Count -ne 8) { throw "Manifest must contain 8 tutorial IDs" }
    $results = @()
    foreach ($challenge in $challengeRows) {
        $entry = $manifestExamples | Where-Object { $_.id -eq $challenge.example_id }
        if (-not $entry) { throw "Challenge $($challenge.id) references missing example $($challenge.example_id)" }
        $examplePath = Join-Path $resolvedExamples $entry.filename
        if (-not (Test-Path -LiteralPath $examplePath -PathType Leaf)) { throw "Missing example file: $examplePath" }
        $actualHash = (Get-FileHash -LiteralPath $examplePath -Algorithm SHA256).Hash
        if ($actualHash -ne $entry.sha256) { throw "Example hash mismatch: $($entry.id)" }
        $rootResult = Invoke-ExampleClient -CaseId $entry.id -ClientMode "verify" -StampId $entry.stamp_id -ChallengeId $challenge.id -StampPath $examplePath
        $results += [ordered]@{
            id = $challenge.id
            example_id = $entry.id
            status = $rootResult.Values.OMNI_EXAMPLE_STATUS
            assertions = [int]$rootResult.Values.OMNI_EXAMPLE_ASSERTIONS
            initial_particles = [int]$rootResult.Values.OMNI_EXAMPLE_INITIAL_PARTICLES
            final_particles = [int]$rootResult.Values.OMNI_EXAMPLE_FINAL_PARTICLES
            stamp_id = $entry.stamp_id
        }
        Write-Output "tutorial-pass=$($challenge.id) assertions=$($rootResult.Values.OMNI_EXAMPLE_ASSERTIONS)"
    }
    $report = [ordered]@{
        schema_version = 1
        source_commit = Get-GitHead
        executable_sha256 = (Get-FileHash -LiteralPath $resolvedExecutable -Algorithm SHA256).Hash
        example_count = $manifestExamples.Count
        tutorial_count = $results.Count
        pass_count = @($results | Where-Object { $_.status -eq "PASS" }).Count
        results = $results
    }
    $reportPath = Join-Path $resolvedExamples "tutorials-runtime-report.json"
    [System.IO.File]::WriteAllText(
        $reportPath,
        ($report | ConvertTo-Json -Depth 8) + [Environment]::NewLine,
        [System.Text.UTF8Encoding]::new($false))
    if ($report.pass_count -ne 8) { throw "Only $($report.pass_count)/8 tutorials passed" }
    Write-Output "tutorials-report=$reportPath"
    return $report
}

if ($Mode -in @("Generate", "All")) {
    [void](Generate-Examples)
}
if ($Mode -in @("Verify", "All")) {
    $report = Verify-Examples
    Write-Output "runtime-lua-examples-test: PASS"
    Write-Output "OMNI_EXAMPLES_STATUS=PASS"
    Write-Output "OMNI_EXAMPLES_COUNT=7"
    Write-Output "OMNI_TUTORIALS_COUNT=8"
    Write-Output "OMNI_TUTORIALS_PASS=$($report.pass_count)"
}
