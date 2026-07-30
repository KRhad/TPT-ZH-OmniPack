param(
    [Parameter(Mandatory = $true)]
    [string] $Executable,

    [Parameter(Mandatory = $true)]
    [ValidateSet(
        "S01-METALLURGY-LARGE",
        "S02-FURNACES-PARALLEL",
        "S03-ECOLOGY-AREA",
        "S04-PATHOGEN-CONTROL",
        "S05-CHEMISTRY-DENSE",
        "S06-NEUTRON-GENERATORS",
        "S07-REACTOR-STABLE",
        "S08-REACTOR-LOCA",
        "S09-ALL-MODULES",
        "S10-CARRIERS-ROUNDTRIP"
    )]
    [string] $SampleId,

    [ValidateRange(0, 600)]
    [int] $WarmupSeconds = 60,

    [ValidateRange(1, 7200)]
    [int] $SampleSeconds = 600,

    [ValidateRange(3, 24)]
    [int] $FixtureStride = 3,

    [string] $OutputDirectory = "artifacts/performance/0.1.0-test",

    [string] $PackageZip,

    [string] $TemporaryDirectory = [System.IO.Path]::GetTempPath(),

    [switch] $Smoke,

    [switch] $KeepArtifacts
)

$ErrorActionPreference = "Stop"
if ($Smoke) {
    $WarmupSeconds = 0
    $SampleSeconds = 2
    $FixtureStride = [Math]::Max($FixtureStride, 12)
}

$sourceRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot "..")).Path
$resolvedExecutable = (Resolve-Path -LiteralPath $Executable).Path
$luaSource = Join-Path $PSScriptRoot "runtime\stress_scenarios.lua"
if (-not (Test-Path -LiteralPath $luaSource -PathType Leaf)) {
    throw "Missing stress Lua script: $luaSource"
}

$tempParent = [System.IO.Path]::GetFullPath($TemporaryDirectory)
if (-not (Test-Path -LiteralPath $tempParent -PathType Container)) {
    throw "Temporary directory does not exist: $tempParent"
}
$runId = [DateTime]::UtcNow.ToString("yyyyMMddTHHmmssZ") + "-" + [guid]::NewGuid().ToString("N").Substring(0, 8)
$testRoot = [System.IO.Path]::GetFullPath((Join-Path $tempParent ("tpt-omnipack-stress-" + $runId)))
$tempPrefix = $tempParent.TrimEnd(
    [System.IO.Path]::DirectorySeparatorChar,
    [System.IO.Path]::AltDirectorySeparatorChar
) + [System.IO.Path]::DirectorySeparatorChar
if (-not $testRoot.StartsWith($tempPrefix, [System.StringComparison]::OrdinalIgnoreCase)) {
    throw "Refusing to create a stress directory outside the selected temporary root"
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

function Get-StampInfo {
    param([Parameter(Mandatory = $true)][string] $Stamp)
    if ($Stamp -notmatch '^[0-9A-Fa-f]{10}$') {
        throw "Unsafe stamp identifier returned by Lua: $Stamp"
    }
    $path = (Resolve-Path -LiteralPath (Join-Path $testRoot ("stamps\" + $Stamp + ".stm"))).Path
    $stampPrefix = (Join-Path $testRoot "stamps").TrimEnd(
        [System.IO.Path]::DirectorySeparatorChar,
        [System.IO.Path]::AltDirectorySeparatorChar
    ) + [System.IO.Path]::DirectorySeparatorChar
    if (-not $path.StartsWith($stampPrefix, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Resolved stress OPS escaped the isolated stamps directory"
    }
    [byte[]]$bytes = [System.IO.File]::ReadAllBytes($path)
    if ($bytes.Length -le 15 -or [System.Text.Encoding]::ASCII.GetString($bytes, 0, 4) -ne "OPS1") {
        throw "Stress save is not a valid OPS1 container: $path"
    }
    return [pscustomobject]@{
        Path = $path
        Length = $bytes.Length
        Sha256 = (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash
    }
}

$completed = $false
try {
    New-Item -ItemType Directory -Path $testRoot | Out-Null
    Copy-Item -LiteralPath $luaSource -Destination (Join-Path $testRoot "autorun.lua")
    [System.IO.File]::WriteAllLines(
        (Join-Path $testRoot "stress-scenario.config"),
        @(
            "sample_id=$SampleId",
            "warmup_seconds=$WarmupSeconds",
            "sample_seconds=$SampleSeconds",
            "fixture_stride=$FixtureStride"
        ),
        [System.Text.Encoding]::ASCII
    )

    $startInfo = [System.Diagnostics.ProcessStartInfo]::new()
    $startInfo.FileName = $resolvedExecutable
    $startInfo.WorkingDirectory = $testRoot
    $startInfo.UseShellExecute = $false
    $startInfo.CreateNoWindow = $true
    $startInfo.ArgumentList.Add("ddir")
    $startInfo.ArgumentList.Add($testRoot)
    foreach ($secretName in @("GITHUB_PAT_TOKEN", "GITHUB_TOKEN", "GH_TOKEN")) {
        [void]$startInfo.Environment.Remove($secretName)
    }

    $startedAt = [DateTime]::UtcNow
    $process = [System.Diagnostics.Process]::Start($startInfo)
    if (-not $process) {
        throw "Failed to start the stress client"
    }
    $cpuSeries = [System.Collections.Generic.List[object]]::new()
    $peakWorkingSet = [int64]0
    $peakPrivateBytes = [int64]0
    $resultPath = Join-Path $testRoot "stress-lua.result"
    $deadline = $startedAt.AddSeconds($WarmupSeconds + $SampleSeconds + 180)
    do {
        Start-Sleep -Milliseconds 500
        $process.Refresh()
        if (-not $process.HasExited) {
            $peakWorkingSet = [Math]::Max($peakWorkingSet, [int64]$process.WorkingSet64)
            $peakPrivateBytes = [Math]::Max($peakPrivateBytes, [int64]$process.PrivateMemorySize64)
            $cpuSeries.Add([pscustomobject]@{
                elapsed_seconds = [Math]::Round(([DateTime]::UtcNow - $startedAt).TotalSeconds, 6)
                total_cpu_seconds = [Math]::Round($process.TotalProcessorTime.TotalSeconds, 6)
                working_set_bytes = [int64]$process.WorkingSet64
                private_bytes = [int64]$process.PrivateMemorySize64
            })
        }
    } while (-not $process.HasExited -and [DateTime]::UtcNow -lt $deadline)

    if (-not $process.HasExited) {
        Stop-Process -Id $process.Id -Force
        $process.WaitForExit()
        throw "Stress client timed out; artifacts=$testRoot"
    }
    if (-not (Test-Path -LiteralPath $resultPath -PathType Leaf)) {
        throw "Stress client exited without a Lua result; exit_code=$($process.ExitCode); artifacts=$testRoot"
    }
    $lua = Read-KeyValueFile -Path $resultPath
    if ($process.ExitCode -ne 0 -or $lua.OMNI_STRESS_LUA_STATUS -ne "PASS") {
        throw "Stress Lua failed; exit_code=$($process.ExitCode); error=$($lua.error); artifacts=$testRoot"
    }

    $firstOps = Get-StampInfo -Stamp $lua.first_stamp
    $secondOps = Get-StampInfo -Stamp $lua.second_stamp
    $sourceCommit = (& git -C $sourceRoot rev-parse HEAD).Trim()
    if ($LASTEXITCODE -ne 0 -or $sourceCommit -notmatch '^[0-9a-f]{40}$') {
        throw "Cannot resolve the source commit"
    }

    $cpu = Get-CimInstance Win32_Processor | Select-Object -First 1
    $computer = Get-CimInstance Win32_ComputerSystem
    $os = Get-CimInstance Win32_OperatingSystem
    $gpu = (Get-CimInstance Win32_VideoController | Select-Object -ExpandProperty Name) -join "; "
    $machineMaterial = "$($env:COMPUTERNAME)|$($cpu.Name)|$($computer.TotalPhysicalMemory)"
    $machineBytes = [System.Text.Encoding]::UTF8.GetBytes($machineMaterial)
    $machineHash = [System.Security.Cryptography.SHA256]::HashData($machineBytes)
    $machineId = $env:COMPUTERNAME + "-" + ([Convert]::ToHexString($machineHash).Substring(0, 12))
    $logicalCpuCount = [int][Environment]::ProcessorCount
    $elapsedSeconds = [Math]::Max(0.000001, ([DateTime]::UtcNow - $startedAt).TotalSeconds)
    $averageCpu = [Math]::Round(
        100.0 * $process.TotalProcessorTime.TotalSeconds / $elapsedSeconds / $logicalCpuCount,
        6
    )
    $powerMode = "not_tested"
    try {
        $activePower = (& powercfg /getactivescheme 2>$null | Out-String).Trim()
        if ($LASTEXITCODE -eq 0 -and $activePower) {
            $powerMode = $activePower
        }
    }
    catch {
        $powerMode = "not_tested"
    }
    $publicZipSha256 = "not_tested"
    if ($PackageZip) {
        $publicZipSha256 = (Get-FileHash -LiteralPath (Resolve-Path -LiteralPath $PackageZip).Path -Algorithm SHA256).Hash
    }

    $artifactBase = if ([System.IO.Path]::IsPathRooted($OutputDirectory)) {
        [System.IO.Path]::GetFullPath($OutputDirectory)
    } else {
        [System.IO.Path]::GetFullPath((Join-Path $sourceRoot $OutputDirectory))
    }
    $artifactDirectory = Join-Path $artifactBase (Join-Path $machineId (Join-Path $SampleId $runId))
    New-Item -ItemType Directory -Path $artifactDirectory -Force | Out-Null
    Copy-Item -LiteralPath $resultPath -Destination (Join-Path $artifactDirectory "stress-lua.result")
    Copy-Item -LiteralPath (Join-Path $testRoot "frame-series.csv") -Destination (Join-Path $artifactDirectory "frame-series.csv")
    Copy-Item -LiteralPath $firstOps.Path -Destination (Join-Path $artifactDirectory "input-first.stm")
    Copy-Item -LiteralPath $secondOps.Path -Destination (Join-Path $artifactDirectory "output-second.stm")
    $cpuSeries | Export-Csv -LiteralPath (Join-Path $artifactDirectory "process-series.csv") -NoTypeInformation -Encoding utf8

    $record = [ordered]@{
        schema_version = 1
        sample_id = $SampleId
        run_id = $runId
        source_commit = $sourceCommit
        release_tag = "not_tested"
        version = "0.1.0-test"
        public_zip_sha256 = $publicZipSha256
        exe_sha256 = (Get-FileHash -LiteralPath $resolvedExecutable -Algorithm SHA256).Hash
        input_ops_sha256 = $firstOps.Sha256
        output_ops_first_sha256 = $firstOps.Sha256
        output_ops_second_sha256 = $secondOps.Sha256
        machine_id = $machineId
        os_build = "$($os.Caption) $($os.Version) build $($os.BuildNumber)"
        cpu_model = $cpu.Name.Trim()
        logical_cpu_count = $logicalCpuCount
        physical_memory_bytes = [int64]$computer.TotalPhysicalMemory
        gpu_model = $gpu
        power_mode = $powerMode
        display_resolution = "not_tested"
        dpi_percent = "not_tested"
        language = "not_tested"
        enabled_modules = "metallurgy,biology,chemistry,nuclear"
        performance_protection = "event-budgets-source-confirmed"
        random_seed = "11,12,13,14"
        warmup_seconds = [double]$lua.actual_warmup_seconds
        sample_seconds = [double]$lua.actual_sample_seconds
        initial_particles = [int]$lua.initial_particles
        peak_particles = [int]$lua.peak_particles
        final_particles = [int]$lua.final_particles
        average_fps = [double]$lua.average_fps
        one_percent_low_fps = [double]$lua.one_percent_low_fps
        minimum_fps = [double]$lua.minimum_fps
        average_cpu_percent = $averageCpu
        peak_working_set_bytes = $peakWorkingSet
        peak_private_bytes = $peakPrivateBytes
        event_count_total = "not_tested"
        event_count_peak_per_frame = "not_tested"
        save_time_first_ms = [double]$lua.save_time_first_ms
        load_time_first_ms = [double]$lua.load_time_first_ms
        save_time_second_ms = [double]$lua.save_time_second_ms
        load_time_second_ms = [double]$lua.load_time_second_ms
        crashed = $false
        hung = $false
        unbounded_growth = "not_tested"
        roundtrip_pass = [System.Convert]::ToBoolean($lua.roundtrip_pass)
        smoke_run = [bool]$Smoke
        gate_result = if ($Smoke) { "not_tested" } else { "incomplete_event_and_growth_evidence" }
        notes = "Simulation FPS is measured as completed sim.updateUpTo calls per wall-clock second. GUI frame presentation, display/DPI, event counters, and unbounded-growth classification are not inferred."
    }
    $jsonPath = Join-Path $artifactDirectory "result.json"
    [System.IO.File]::WriteAllText(
        $jsonPath,
        ($record | ConvertTo-Json -Depth 5) + [Environment]::NewLine,
        [System.Text.UTF8Encoding]::new($false)
    )
    $completed = $true
    Write-Output "runtime-stress-test: PASS (harness execution)"
    Write-Output "sample_id=$SampleId"
    Write-Output "smoke_run=$([bool]$Smoke)"
    Write-Output "stress_gate=not_tested"
    Write-Output "result_json=$jsonPath"
}
finally {
    if ($completed -and -not $KeepArtifacts -and (Test-Path -LiteralPath $testRoot)) {
        Remove-Item -LiteralPath $testRoot -Recurse -Force
    }
}
