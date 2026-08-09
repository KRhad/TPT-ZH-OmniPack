param(
    [Parameter(Mandatory = $true)]
    [string] $Executable,

    [ValidateRange(1, 180)]
    [int] $TimeoutSeconds = 90,

    [string] $TemporaryDirectory = [System.IO.Path]::GetTempPath()
)

$ErrorActionPreference = "Stop"
$resolvedExecutable = (Resolve-Path -LiteralPath $Executable).Path
$scriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$autorunSource = Join-Path $scriptRoot "runtime\omni_lifecycle_ledger.lua"
if (-not (Test-Path -LiteralPath $autorunSource -PathType Leaf)) {
    throw "Missing Lua lifecycle-ledger script: $autorunSource"
}

$tempParent = [System.IO.Path]::GetFullPath($TemporaryDirectory)
if (-not (Test-Path -LiteralPath $tempParent -PathType Container)) {
    throw "Temporary directory does not exist: $tempParent"
}
$testRoot = Join-Path $tempParent (
    "tpt-omnipack-lifecycle-ledger-" + [guid]::NewGuid().ToString("N")
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
    throw "Refusing to create lifecycle-ledger test outside the temporary directory"
}

$process = $null
$passed = $false
try {
    New-Item -ItemType Directory -Path $resolvedTestRoot | Out-Null
    Copy-Item -LiteralPath $autorunSource -Destination (
        Join-Path $resolvedTestRoot "autorun.lua"
    )
    $result = Join-Path $resolvedTestRoot "omni-lifecycle-ledger.result"
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
    if (-not $process) { throw "Failed to start the client" }

    $deadline = [DateTime]::UtcNow.AddSeconds($TimeoutSeconds)
    $resultText = ""
    do {
        Start-Sleep -Milliseconds 100
        if (Test-Path -LiteralPath $result -PathType Leaf) {
            $resultText = [string](Get-Content -LiteralPath $result -Raw)
            if ($resultText -match "(?m)^OMNI_LIFECYCLE_LEDGER_STATUS=(PASS|FAIL)\r?$") {
                break
            }
        }
        $process.Refresh()
    } while ([DateTime]::UtcNow -lt $deadline -and -not $process.HasExited)

    if (
        $resultText -match "(?m)^OMNI_LIFECYCLE_LEDGER_RECORD_UNITS_ONLY=true\r?$" -and
        $resultText -match "(?m)^OMNI_LIFECYCLE_LEDGER_TICKS=8\r?$" -and
        $resultText -match "(?m)^OMNI_LIFECYCLE_LEDGER_RECONCILIATION_FAILURES=0\r?$" -and
        $resultText -match "(?m)^OMNI_LIFECYCLE_LEDGER_LAST_FRAME_RECONCILED=true\r?$" -and
        $resultText -match "(?m)^OMNI_LIFECYCLE_LEDGER_LAST_UNATTRIBUTED_RECORD_DELTA_ABS=0\r?$" -and
        $resultText -match "(?m)^OMNI_LIFECYCLE_LEDGER_OUTSIDE_DIRECT_TRANSITIONS=1\r?$" -and
        $resultText -match "(?m)^OMNI_LIFECYCLE_LEDGER_BRMT_TUNG_PREPARATION_TRANSITIONS=1\r?$" -and
        $resultText -match "(?m)^OMNI_LIFECYCLE_LEDGER_STATUS=PASS\r?$"
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
            "Lua lifecycle-ledger regression failed; responding=$responding; " +
            "result=$resultText; artifacts=$resolvedTestRoot"
        )
    }
    if (-not $responding) {
        throw "Lua lifecycle-ledger regression passed but client is unresponsive"
    }
    Write-Output "runtime-lua-lifecycle-ledger-test: PASS"
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
                    Write-Warning "Could not remove lifecycle-ledger test directory: $resolvedTestRoot"
                }
                Start-Sleep -Milliseconds 100
            }
        }
    }
}
