param(
    [Parameter(Mandatory = $true)]
    [string] $Executable,

    [ValidateRange(1, 180)]
    [int] $TimeoutSeconds = 90,

    [string] $TemporaryDirectory = [System.IO.Path]::GetTempPath(),

    [string] $RuntimeDirectory = "C:\msys64\ucrt64\bin"
)

$ErrorActionPreference = "Stop"
$resolvedExecutable = (Resolve-Path -LiteralPath $Executable).Path
$resolvedRuntimeDirectory = if ($RuntimeDirectory) {
    (Resolve-Path -LiteralPath $RuntimeDirectory).Path
}
else {
    $null
}
$scriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$autorunSource = Join-Path $scriptRoot "runtime\omni_correction_ledger.lua"
if (-not (Test-Path -LiteralPath $autorunSource -PathType Leaf)) {
    throw "Missing Lua correction-ledger script: $autorunSource"
}

$tempParent = [System.IO.Path]::GetFullPath($TemporaryDirectory)
if (-not (Test-Path -LiteralPath $tempParent -PathType Container)) {
    throw "Temporary directory does not exist: $tempParent"
}
$testRoot = Join-Path $tempParent (
    "tpt-omnipack-correction-ledger-" + [guid]::NewGuid().ToString("N")
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
    throw "Refusing to create correction-ledger test outside the temporary directory"
}

$process = $null
$passed = $false
try {
    New-Item -ItemType Directory -Path $resolvedTestRoot | Out-Null
    Copy-Item -LiteralPath $autorunSource -Destination (
        Join-Path $resolvedTestRoot "autorun.lua"
    )
    $result = Join-Path $resolvedTestRoot "omni-correction-ledger.result"
    $startInfo = [System.Diagnostics.ProcessStartInfo]::new()
    $startInfo.FileName = $resolvedExecutable
    $startInfo.WorkingDirectory = $resolvedTestRoot
    $startInfo.UseShellExecute = $false
    $startInfo.CreateNoWindow = $true
    $startInfo.ArgumentList.Add("ddir")
    $startInfo.ArgumentList.Add($resolvedTestRoot)
    if ($resolvedRuntimeDirectory) {
        $startInfo.Environment["PATH"] = $resolvedRuntimeDirectory +
            [System.IO.Path]::PathSeparator + $env:PATH
    }
    foreach ($secretName in @("GITHUB_PAT_TOKEN", "GITHUB_TOKEN", "GH_TOKEN")) {
        [void]$startInfo.Environment.Remove($secretName)
    }
    $process = [System.Diagnostics.Process]::Start($startInfo)
    if (-not $process) { throw "Failed to start the client" }

    $deadline = [DateTime]::UtcNow.AddSeconds($TimeoutSeconds)
    $resultText = ""
    do {
        Start-Sleep -Milliseconds 100
        if (Test-Path -LiteralPath $result -PathType Leaf) {
            $resultText = [string](Get-Content -LiteralPath $result -Raw)
            if ($resultText -match "(?m)^OMNI_CORRECTION_LEDGER_STATUS=(PASS|FAIL)\r?$") {
                break
            }
        }
        $process.Refresh()
    } while ([DateTime]::UtcNow -lt $deadline -and -not $process.HasExited)

    if (
        $resultText -match "(?m)^OMNI_CORRECTION_LEDGER_LEGACY_FIELD_UNITS_ONLY=true\r?$" -and
        $resultText -match "(?m)^OMNI_CORRECTION_LEDGER_AUDITED_AIR_CAPS_ONLY=true\r?$" -and
        $resultText -match "(?m)^OMNI_CORRECTION_LEDGER_TOTAL_EVENTS=([1-9][0-9]*)\r?$" -and
        $resultText -match "(?m)^OMNI_CORRECTION_LEDGER_RETAINED_EVENTS=([1-9][0-9]*)\r?$" -and
        $resultText -match "(?m)^OMNI_CORRECTION_LEDGER_DROPPED_EVENTS=0\r?$" -and
        $resultText -match "(?m)^OMNI_CORRECTION_LEDGER_OBSERVED_KINDS=([4-9]|1[0-2])\r?$" -and
        $resultText -match "(?m)^OMNI_CORRECTION_LEDGER_X_HIGH=([1-9][0-9]*)\r?$" -and
        $resultText -match "(?m)^OMNI_CORRECTION_LEDGER_X_LOW=([1-9][0-9]*)\r?$" -and
        $resultText -match "(?m)^OMNI_CORRECTION_LEDGER_Y_HIGH=([1-9][0-9]*)\r?$" -and
        $resultText -match "(?m)^OMNI_CORRECTION_LEDGER_Y_LOW=([1-9][0-9]*)\r?$" -and
        $resultText -match "(?m)^OMNI_CORRECTION_LEDGER_OVERFLOW_TOTAL_EVENTS=([2-9][0-9]{2,})\r?$" -and
        $resultText -match "(?m)^OMNI_CORRECTION_LEDGER_OVERFLOW_RETAINED_EVENTS=256\r?$" -and
        $resultText -match "(?m)^OMNI_CORRECTION_LEDGER_OVERFLOW_DROPPED_EVENTS=([1-9][0-9]*)\r?$" -and
        $resultText -match "(?m)^OMNI_CORRECTION_LEDGER_OVERFLOW_FIRST_SEQUENCE=([1-9][0-9]*)\r?$" -and
        $resultText -match "(?m)^OMNI_CORRECTION_LEDGER_OVERFLOW_LAST_SEQUENCE=([2-9][0-9]{2,})\r?$" -and
        $resultText -match "(?m)^OMNI_CORRECTION_LEDGER_STATUS=PASS\r?$"
    ) {
        $passed = $true
    }
    $responding = $false
    if (-not $process.HasExited) {
        $process.Refresh()
        $responding = $process.Responding
    }
    if (-not $passed) {
        throw (
            "Lua correction-ledger regression failed; responding=$responding; " +
            "result=$resultText; artifacts=$resolvedTestRoot"
        )
    }
    if (-not $responding) {
        throw "Lua correction-ledger regression passed but client is unresponsive"
    }
    Write-Output "runtime-lua-correction-ledger-test: PASS"
    Write-Output "runtime_directory=$resolvedRuntimeDirectory"
    Write-Output $resultText.Trim()
}
finally {
    if ($process -and -not $process.HasExited) {
        Stop-Process -Id $process.Id -Force
        $process.WaitForExit()
    }
    if ($process) { $process.Dispose() }
    if ($passed -and (Test-Path -LiteralPath $resolvedTestRoot)) {
        for ($attempt = 1; $attempt -le 20; $attempt++) {
            try {
                Remove-Item -LiteralPath $resolvedTestRoot -Recurse -Force -ErrorAction Stop
                break
            }
            catch {
                if ($attempt -eq 20) {
                    Write-Warning "Could not remove correction-ledger test directory: $resolvedTestRoot"
                }
                Start-Sleep -Milliseconds 100
            }
        }
    }
}
