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
$autorunSource = Join-Path $scriptRoot "runtime\omni_profiler.lua"
if (-not (Test-Path -LiteralPath $autorunSource -PathType Leaf)) {
    throw "Missing Lua profiler script: $autorunSource"
}

$tempParent = [System.IO.Path]::GetFullPath($TemporaryDirectory)
if (-not (Test-Path -LiteralPath $tempParent -PathType Container)) {
    throw "Temporary directory does not exist: $tempParent"
}
$testRoot = Join-Path $tempParent ("tpt-omnipack-profiler-" + [guid]::NewGuid().ToString("N"))
$resolvedTestRoot = [System.IO.Path]::GetFullPath($testRoot)
$tempPrefix = $tempParent.TrimEnd(
    [System.IO.Path]::DirectorySeparatorChar,
    [System.IO.Path]::AltDirectorySeparatorChar
) + [System.IO.Path]::DirectorySeparatorChar
if (-not $resolvedTestRoot.StartsWith($tempPrefix, [System.StringComparison]::OrdinalIgnoreCase)) {
    throw "Refusing to create profiler test outside the temporary directory"
}

$process = $null
$passed = $false
try {
    New-Item -ItemType Directory -Path $resolvedTestRoot | Out-Null
    Copy-Item -LiteralPath $autorunSource -Destination (Join-Path $resolvedTestRoot "autorun.lua")
    $result = Join-Path $resolvedTestRoot "omni-profiler.result"
    $startInfo = [System.Diagnostics.ProcessStartInfo]::new()
    $startInfo.FileName = $resolvedExecutable
    $startInfo.WorkingDirectory = $resolvedTestRoot
    $startInfo.UseShellExecute = $false
    $startInfo.CreateNoWindow = $true
    $startInfo.ArgumentList.Add("ddir")
    $startInfo.ArgumentList.Add($resolvedTestRoot)
    if ($resolvedRuntimeDirectory) {
        $startInfo.Environment["PATH"] = $resolvedRuntimeDirectory + [System.IO.Path]::PathSeparator + $env:PATH
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
            if ($resultText -match "(?m)^OMNI_PROFILER_STATUS=(PASS|FAIL)\r?$") {
                break
            }
        }
        $process.Refresh()
    } while ([DateTime]::UtcNow -lt $deadline -and -not $process.HasExited)

    if (
        $resultText -match "(?m)^OMNI_PROFILER_BACKEND=legacy_cpu_serial\r?$" -and
        $resultText -match "(?m)^OMNI_PROFILER_CLOCK=steady_clock_monotonic\r?$" -and
        $resultText -match "(?m)^OMNI_PROFILER_LIVE_PARTICLES=([1-9][0-9]*)\r?$" -and
        $resultText -match "(?m)^OMNI_PROFILER_ATMOSPHERE_CELLS=([1-9][0-9]*)\r?$" -and
        $resultText -match "(?m)^OMNI_PROFILER_FRAME_CALLS=6\r?$" -and
        $resultText -match "(?m)^OMNI_PROFILER_SIMULATION_CALLS=6\r?$" -and
        $resultText -match "(?m)^OMNI_PROFILER_PARTICLE_CALLS=6\r?$" -and
        $resultText -match "(?m)^OMNI_PROFILER_AIR_CALLS=6\r?$" -and
        $resultText -match "(?m)^OMNI_PROFILER_AMBIENT_HEAT_CALLS=6\r?$" -and
        $resultText -match "(?m)^OMNI_PROFILER_GRAVITY_CALLS=6\r?$" -and
        $resultText -match "(?m)^OMNI_PROFILER_LUA_CALLS=12\r?$" -and
        $resultText -match "(?m)^OMNI_PROFILER_FRAME_TOTAL_NS=([1-9][0-9]*)\r?$" -and
        $resultText -match "(?m)^OMNI_PROFILER_SIMULATION_TOTAL_NS=([1-9][0-9]*)\r?$" -and
        $resultText -match "(?m)^OMNI_PROFILER_PARTICLE_TOTAL_NS=([1-9][0-9]*)\r?$" -and
        $resultText -match "(?m)^OMNI_PROFILER_AIR_TOTAL_NS=([1-9][0-9]*)\r?$" -and
        $resultText -match "(?m)^OMNI_PROFILER_AMBIENT_HEAT_TOTAL_NS=([1-9][0-9]*)\r?$" -and
        $resultText -match "(?m)^OMNI_PROFILER_GRAVITY_TOTAL_NS=([0-9]+)\r?$" -and
        $resultText -match "(?m)^OMNI_PROFILER_LUA_TOTAL_NS=([1-9][0-9]*)\r?$" -and
        $resultText -match "(?m)^OMNI_PROFILER_PROCESS_VRAM_AVAILABLE=false\r?$" -and
        $resultText -match "(?m)^OMNI_PROFILER_PROCESS_VRAM_STATUS=not_tested_no_gpu_backend\r?$" -and
        $resultText -match "(?m)^OMNI_PROFILER_STATUS=PASS\r?$"
    ) {
        $passed = $true
    }
    $responding = $false
    if (-not $process.HasExited) {
        $process.Refresh()
        $responding = $process.Responding
    }
    if (-not $passed) {
        throw "Lua profiler regression failed; responding=$responding; result=$resultText; artifacts=$resolvedTestRoot"
    }
    if (-not $responding) {
        throw "Lua profiler regression passed but client is unresponsive"
    }
    Write-Output "runtime-lua-profiler-test: PASS"
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
                    Write-Warning "Could not remove profiler test directory: $resolvedTestRoot"
                }
                Start-Sleep -Milliseconds 100
            }
        }
    }
}
