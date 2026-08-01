param(
    [Parameter(Mandatory = $true)]
    [string] $Executable,

    [ValidateRange(1, 180)]
    [int] $TimeoutSeconds = 45,

    [string] $TemporaryDirectory = [System.IO.Path]::GetTempPath()
)

$ErrorActionPreference = "Stop"
$resolvedExecutable = (Resolve-Path -LiteralPath $Executable).Path
$scriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$autorunSource = Join-Path $scriptRoot "runtime\isotope_regression.lua"
if (-not (Test-Path -LiteralPath $autorunSource -PathType Leaf)) {
    throw "Missing Lua isotope regression script: $autorunSource"
}

$tempParent = [System.IO.Path]::GetFullPath($TemporaryDirectory)
if (-not (Test-Path -LiteralPath $tempParent -PathType Container)) {
    throw "Temporary directory does not exist: $tempParent"
}
$testRoot = Join-Path $tempParent (
    "tpt-omnipack-isotope-" + [guid]::NewGuid().ToString("N")
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
    throw "Refusing to create a runtime test outside the temporary directory"
}

$process = $null
$passed = $false
try {
    New-Item -ItemType Directory -Path $resolvedTestRoot | Out-Null
    Copy-Item -LiteralPath $autorunSource -Destination (
        Join-Path $resolvedTestRoot "autorun.lua"
    )
    $result = Join-Path $resolvedTestRoot "lua-isotope-regression.result"
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
            if ($resultText -match "(?m)^OMNI_ISOTOPE_STATUS=(PASS|FAIL)\r?$") {
                break
            }
        }
        $process.Refresh()
    } while ([DateTime]::UtcNow -lt $deadline -and -not $process.HasExited)

    if (
        $resultText -match "(?m)^OMNI_ISOTOPE_STATUS=PASS\r?$" -and
        $resultText -match "(?m)^OMNI_ISOTOPE_ELEMENTS=13\r?$" -and
        $resultText -match "(?m)^OMNI_ISOTOPE_IDS=576-588\r?$" -and
        $resultText -match "(?m)^OMNI_ISOTOPE_DECAY_PATHS=12\r?$" -and
        $resultText -match "(?m)^OMNI_ISOTOPE_NEUTRON_PATHS=8\r?$" -and
        $resultText -match "(?m)^OMNI_ISOTOPE_IGNITION_PATHS=2\r?$" -and
        $resultText -match "(?m)^OMNI_ISOTOPE_PHASE_PATHS=1\r?$" -and
        $resultText -match "(?m)^OMNI_ISOTOPE_TIMER_CONTINUITY=true\r?$" -and
        $resultText -match "(?m)^OMNI_ISOTOPE_EVENT_PEAK=512\r?$" -and
        $resultText -match "(?m)^OMNI_ISOTOPE_DEUT_SEPARATE=95\r?$"
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
            "Lua isotope regression failed; responding=$responding; " +
            "result=$resultText; artifacts=$resolvedTestRoot"
        )
    }
    if (-not $responding) {
        throw "Lua isotope regression passed but client is unresponsive"
    }
    Write-Output "runtime-lua-isotope-test: PASS"
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
                    Write-Warning "Could not remove runtime test directory: $resolvedTestRoot"
                }
                Start-Sleep -Milliseconds 100
            }
        }
    }
}
