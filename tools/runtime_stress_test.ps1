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
        "S10-CARRIERS-ROUNDTRIP",
        "S11-AUTOMATION-FACTORY",
        "S12-AUTOMATION-SIGNAL-LOOP",
        "S13-ELECTRONICS-DENSE",
        "S14-ENVIRONMENT-DENSE",
        "S15-PERIODIC-ALL",
        "S16-INORGANIC-DENSE",
        "S17-MATERIALS-DENSE",
        "S18-ISOTOPES-DENSE",
        "S19-ORGANICS-DENSE",
        "S20-FULL-CATALOG"
    )]
    [string] $SampleId,

    [ValidateRange(0, 600)]
    [int] $WarmupSeconds = 0,

    [ValidateRange(1, 7200)]
    [int] $SampleSeconds = 30,

    [ValidateRange(3, 24)]
    [int] $FixtureStride = 3,

    [string] $OutputDirectory,

    [string] $PackageZip,

    [ValidateSet("0.1.0-test", "0.2.0-dev", "0.3.0-dev", "0.6.0-dev", "0.7.0-dev", "1.0.0-rc9", "1.1.0-rc1", "1.1.0")]
    [string] $PackageVersion = "0.1.0-test",

    [string] $TemporaryDirectory = [System.IO.Path]::GetTempPath(),

    [switch] $LongRun,

    [switch] $Smoke,

    [switch] $KeepArtifacts
)

$ErrorActionPreference = "Stop"
function Get-Sha256Hex {
    param([Parameter(Mandatory = $true)][string] $Path)
    $sha = [System.Security.Cryptography.SHA256]::Create()
    try {
        $stream = [IO.File]::OpenRead($Path)
        try {
            return ([BitConverter]::ToString($sha.ComputeHash($stream))).Replace('-', '').ToUpperInvariant()
        } finally { $stream.Dispose() }
    } finally { $sha.Dispose() }
}
if ($Smoke) {
    $WarmupSeconds = 0
    $SampleSeconds = 2
    $FixtureStride = [Math]::Max($FixtureStride, 12)
}
elseif ($LongRun) {
    $WarmupSeconds = 60
    $SampleSeconds = 7200
}
if ($LongRun -and $SampleId -ne "S20-FULL-CATALOG") {
    throw "LongRun requires S20-FULL-CATALOG"
}
if (-not $OutputDirectory) {
    $OutputDirectory = "artifacts/performance/$PackageVersion"
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

function ConvertTo-FiniteDouble {
    param(
        [Parameter(Mandatory = $true)][hashtable] $Values,
        [Parameter(Mandatory = $true)][string] $Name
    )
    if (-not $Values.ContainsKey($Name)) {
        throw "Stress Lua did not provide $Name"
    }
    $parsed = 0.0
    if (-not [double]::TryParse(
        [string]$Values[$Name],
        [System.Globalization.NumberStyles]::Float,
        [System.Globalization.CultureInfo]::InvariantCulture,
        [ref]$parsed
    ) -or [double]::IsNaN($parsed) -or [double]::IsInfinity($parsed)) {
        throw "Stress Lua provided a non-finite or invalid $Name"
    }
    return [double]$parsed
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
        Sha256 = Get-Sha256Hex $path
    }
}

function Get-PackageProvenance {
    param(
        [Parameter(Mandatory = $true)][string] $Package,
        [Parameter(Mandatory = $true)][string] $ExecutablePath,
        [Parameter(Mandatory = $true)][string] $ExpectedVersion
    )

    $resolvedPackage = (Resolve-Path -LiteralPath $Package).Path
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $archive = [System.IO.Compression.ZipFile]::OpenRead($resolvedPackage)
    try {
        $manifestEntries = @(
            $archive.Entries | Where-Object { $_.FullName -match '/(?:TEST-)?MANIFEST\.txt$' }
        )
        if ($manifestEntries.Count -ne 1) {
            throw "Expected exactly one TEST-MANIFEST.txt or MANIFEST.txt in package, found $($manifestEntries.Count)"
        }
        $reader = [System.IO.StreamReader]::new(
            $manifestEntries[0].Open(),
            [System.Text.Encoding]::UTF8,
            $true
        )
        try {
            $manifestText = $reader.ReadToEnd()
        }
        finally {
            $reader.Dispose()
        }
        $kindMatches = [regex]::Matches(
            $manifestText,
            '(?m)^kind=(public-test|local-dev|release-candidate|release)\r?$'
        )
        if ($kindMatches.Count -ne 1) {
            throw "Package manifest must contain one supported package kind"
        }
        $versionMatches = [regex]::Matches(
            $manifestText,
            '(?m)^version=([^\r\n]+)\r?$'
        )
        if ($versionMatches.Count -ne 1 -or $versionMatches[0].Groups[1].Value -ne $ExpectedVersion) {
            throw "Package manifest version does not match $ExpectedVersion"
        }
        $revisionMatches = [regex]::Matches(
            $manifestText,
            '(?m)^revision=([0-9a-f]{40})\r?$'
        )
        if ($revisionMatches.Count -ne 1) {
            throw "Package manifest must contain exactly one lowercase 40-character revision"
        }
        $sourceStateMatches = [regex]::Matches(
            $manifestText,
            '(?m)^source_state=(clean|dirty)\r?$'
        )
        $sourceWorktreeMatches = [regex]::Matches(
            $manifestText,
            '(?m)^source_worktree_sha256=([0-9A-F]{64})\r?$'
        )
        if ($sourceStateMatches.Count -gt 1 -or $sourceWorktreeMatches.Count -gt 1) {
            throw "Package manifest contains duplicate source provenance fields"
        }
        if ($sourceStateMatches.Count -ne $sourceWorktreeMatches.Count) {
            throw "Package manifest source provenance is incomplete"
        }
        $sourceState = if ($sourceStateMatches.Count -eq 1) {
            $sourceStateMatches[0].Groups[1].Value
        } else {
            "legacy_commit_only"
        }
        $sourceWorktreeSha256 = if ($sourceWorktreeMatches.Count -eq 1) {
            $sourceWorktreeMatches[0].Groups[1].Value
        } else {
            "not_recorded"
        }
        $executableMatches = [regex]::Matches(
            $manifestText,
            '(?m)^member=tpt-zh-omnipack\.exe\|([0-9]+)\|([0-9A-F]{64})\r?$'
        )
        if ($executableMatches.Count -ne 1) {
            throw "Package manifest must contain exactly one tpt-zh-omnipack.exe member"
        }
        $actualExecutable = Get-Item -LiteralPath $ExecutablePath
        $expectedLength = [int64]$executableMatches[0].Groups[1].Value
        $expectedHash = $executableMatches[0].Groups[2].Value
        $actualHash = Get-Sha256Hex $actualExecutable.FullName
        if ($actualExecutable.Length -ne $expectedLength) {
            throw "Package executable size does not match the selected executable"
        }
        if ($actualHash -ne $expectedHash) {
            throw "Package executable hash does not match the selected executable"
        }
        return [pscustomobject]@{
            Revision = $revisionMatches[0].Groups[1].Value
            Sha256 = Get-Sha256Hex $resolvedPackage
            Kind = $kindMatches[0].Groups[1].Value
            SourceState = $sourceState
            SourceWorktreeSha256 = $sourceWorktreeSha256
        }
    }
    finally {
        $archive.Dispose()
    }
}

$harnessCommit = (& git -C $sourceRoot rev-parse HEAD).Trim()
if ($LASTEXITCODE -ne 0 -or $harnessCommit -notmatch '^[0-9a-f]{40}$') {
    throw "Cannot resolve the stress harness commit"
}
$sourceCommit = $null
$sourceState = "not_tested"
$sourceWorktreeSha256 = "not_tested"
$publicZipSha256 = "not_tested"
$packageKind = "not_tested"
if ($PackageZip) {
    $packageProvenance = Get-PackageProvenance `
        -Package $PackageZip `
        -ExecutablePath $resolvedExecutable `
        -ExpectedVersion $PackageVersion
    $sourceCommit = $packageProvenance.Revision
    $sourceState = $packageProvenance.SourceState
    $sourceWorktreeSha256 = $packageProvenance.SourceWorktreeSha256
    $publicZipSha256 = $packageProvenance.Sha256
    $packageKind = $packageProvenance.Kind
}
elseif (-not $Smoke) {
    throw "PackageZip is required for stability gate runs"
}
else {
    $sourceCommit = $harnessCommit
    $sourceState = "unpackaged_smoke"
    $sourceWorktreeSha256 = "not_recorded"
}

$completed = $false
try {
    New-Item -ItemType Directory -Path $testRoot | Out-Null
    # An existing empty preference object makes this an isolated returning
    # profile. It suppresses first-run modal UI (for example automatic scale
    # confirmation) without importing any real user preference or account.
    [System.IO.File]::WriteAllText(
        (Join-Path $testRoot "powder.pref"),
        "{}" + [Environment]::NewLine,
        [System.Text.UTF8Encoding]::new($false)
    )
    Copy-Item -LiteralPath $luaSource -Destination (Join-Path $testRoot "autorun.lua")
    [System.IO.File]::WriteAllLines(
        (Join-Path $testRoot "stress-scenario.config"),
        @(
            "sample_id=$SampleId",
            "warmup_seconds=$WarmupSeconds",
            "sample_seconds=$SampleSeconds",
            "fixture_stride=$FixtureStride",
            "smoke_run=$(([bool]$Smoke).ToString().ToLowerInvariant())",
            "long_run=$(([bool]$LongRun).ToString().ToLowerInvariant())"
        ),
        [System.Text.Encoding]::ASCII
    )

    $startInfo = [System.Diagnostics.ProcessStartInfo]::new()
    $startInfo.FileName = $resolvedExecutable
    $startInfo.WorkingDirectory = $testRoot
    $startInfo.UseShellExecute = $false
    $startInfo.CreateNoWindow = $true
    # Windows PowerShell 5.1 runs on .NET Framework, where ArgumentList is not
    # available. The stress path is locally generated and cannot contain a
    # literal quote, so a quoted Arguments string is deterministic here.
    if ($testRoot.Contains('"')) { throw "Stress directory contains an unsafe quote" }
    $startInfo.Arguments = '"ddir" "' + $testRoot + '"'
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
    $heartbeatPath = Join-Path $testRoot "soak-heartbeat.csv"
    $lastHeartbeatUtc = $startedAt
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
        if ($LongRun) {
            $nowUtc = [DateTime]::UtcNow
            if (Test-Path -LiteralPath $heartbeatPath -PathType Leaf) {
                $lastHeartbeatUtc = (Get-Item -LiteralPath $heartbeatPath).LastWriteTimeUtc
            }
            if (($nowUtc - $lastHeartbeatUtc).TotalSeconds -gt 120.0) {
                Stop-Process -Id $process.Id -Force
                $process.WaitForExit()
                throw "Stress client heartbeat stalled for more than 120 seconds; artifacts=$testRoot"
            }
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
    foreach ($field in @(
        "event_count_total", "event_count_peak_per_frame",
        "signal_count_total", "signal_count_peak_per_frame",
        "fixture_type_count", "fixture_created_type_count",
        "fixture_visible_type_count",
        "scenario_recovery_assertions",
        "long_run_save_load_cycles", "long_run_language_switches",
        "long_run_module_toggle_cycles", "simulation_steps", "heartbeat_count",
        "stalls", "nan_count", "inf_count")) {
        if (-not $lua.ContainsKey($field) -or $lua[$field] -notmatch '^\d+$') {
            throw "Stress Lua did not provide a nonnegative integer $field"
        }
    }
    if (-not $lua.ContainsKey("stop_event_delta") -or $lua.stop_event_delta -notmatch '^-?\d+$') {
        throw "Stress Lua did not provide an integer stop_event_delta"
    }
    foreach ($field in @(
        "signal_stop_pass", "scenario_stop_pass", "scenario_recovery_pass",
        "long_run", "long_run_settings_recovery_pass", "omni_atmosphere_active")) {
        if (-not $lua.ContainsKey($field) -or $lua[$field] -notin @("true", "false")) {
            throw "Stress Lua did not provide a boolean $field"
        }
    }
    if ([System.Convert]::ToBoolean($lua.long_run) -ne [bool]$LongRun) {
        throw "Stress Lua long_run state does not match the requested mode"
    }
    if ($LongRun -and (
        -not [System.Convert]::ToBoolean($lua.omni_atmosphere_active) -or
        [int64]$lua.stalls -ne 0 -or [int64]$lua.nan_count -ne 0 -or
        [int64]$lua.inf_count -ne 0)) {
        throw "Formal long run did not close its OmniAtmosphere finite/stall diagnostics"
    }

    $firstOps = Get-StampInfo -Stamp $lua.first_stamp
    $secondOps = Get-StampInfo -Stamp $lua.second_stamp
    $cpu = Get-CimInstance Win32_Processor | Select-Object -First 1
    $computer = Get-CimInstance Win32_ComputerSystem
    $os = Get-CimInstance Win32_OperatingSystem
    $gpu = (Get-CimInstance Win32_VideoController | Select-Object -ExpandProperty Name) -join "; "
    $machineMaterial = "$($env:COMPUTERNAME)|$($cpu.Name)|$($computer.TotalPhysicalMemory)"
    $machineBytes = [System.Text.Encoding]::UTF8.GetBytes($machineMaterial)
    $machineHasher = [System.Security.Cryptography.SHA256]::Create()
    try {
        $machineHash = $machineHasher.ComputeHash($machineBytes)
    }
    finally {
        $machineHasher.Dispose()
    }
    $machineHashHex = ([BitConverter]::ToString($machineHash)).Replace('-', '')
    $machineId = $env:COMPUTERNAME + "-" + $machineHashHex.Substring(0, 12)
    $logicalCpuCount = [int][Environment]::ProcessorCount
    $elapsedSeconds = [Math]::Max(0.000001, ([DateTime]::UtcNow - $startedAt).TotalSeconds)
    $endedAt = [DateTime]::UtcNow
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
    $artifactBase = if ([System.IO.Path]::IsPathRooted($OutputDirectory)) {
        [System.IO.Path]::GetFullPath($OutputDirectory)
    } else {
        [System.IO.Path]::GetFullPath((Join-Path $sourceRoot $OutputDirectory))
    }
    $artifactDirectory = Join-Path $artifactBase (Join-Path $machineId (Join-Path $SampleId $runId))
    New-Item -ItemType Directory -Path $artifactDirectory -Force | Out-Null
    Copy-Item -LiteralPath $resultPath -Destination (Join-Path $artifactDirectory "stress-lua.result")
    Copy-Item -LiteralPath (Join-Path $testRoot "frame-series.csv") -Destination (Join-Path $artifactDirectory "frame-series.csv")
    if ($LongRun) {
        if (-not (Test-Path -LiteralPath $heartbeatPath -PathType Leaf)) {
            throw "Formal long run did not produce soak-heartbeat.csv"
        }
        Copy-Item -LiteralPath $heartbeatPath -Destination (Join-Path $artifactDirectory "soak-heartbeat.csv")
    }
    Copy-Item -LiteralPath $firstOps.Path -Destination (Join-Path $artifactDirectory "input-first.stm")
    Copy-Item -LiteralPath $secondOps.Path -Destination (Join-Path $artifactDirectory "output-second.stm")
    $cpuSeries | Export-Csv -LiteralPath (Join-Path $artifactDirectory "process-series.csv") -NoTypeInformation -Encoding utf8

    $record = [ordered]@{
        schema_version = 1
        sample_id = $SampleId
        run_id = $runId
        source_commit = $sourceCommit
        source_state = $sourceState
        source_worktree_sha256 = $sourceWorktreeSha256
        harness_commit = $harnessCommit
        release_tag = "not_tested"
        version = $PackageVersion
        package_kind = $packageKind
        public_zip_sha256 = $publicZipSha256
        exe_sha256 = Get-Sha256Hex $resolvedExecutable
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
        enabled_modules = "metallurgy,biology,chemistry,nuclear,electronics"
        performance_protection = "event-budgets-source-confirmed"
        random_seed = "11,12,13,14"
        start_time_utc = $startedAt.ToString("o")
        end_time_utc = $endedAt.ToString("o")
        wall_clock_seconds = [Math]::Round(($endedAt - $startedAt).TotalSeconds, 6)
        warmup_seconds = ConvertTo-FiniteDouble $lua "actual_warmup_seconds"
        sample_seconds = ConvertTo-FiniteDouble $lua "actual_sample_seconds"
        initial_particles = [int]$lua.initial_particles
        peak_particles = [int]$lua.peak_particles
        final_particles = [int]$lua.final_particles
        average_fps = ConvertTo-FiniteDouble $lua "average_fps"
        one_percent_low_fps = ConvertTo-FiniteDouble $lua "one_percent_low_fps"
        minimum_fps = ConvertTo-FiniteDouble $lua "minimum_fps"
        average_cpu_percent = $averageCpu
        peak_working_set_bytes = $peakWorkingSet
        peak_private_bytes = $peakPrivateBytes
        initial_working_set_bytes = if ($cpuSeries.Count -gt 0) { [int64]$cpuSeries[0].working_set_bytes } else { 0 }
        final_working_set_bytes = if ($cpuSeries.Count -gt 0) { [int64]$cpuSeries[$cpuSeries.Count - 1].working_set_bytes } else { 0 }
        initial_private_bytes = if ($cpuSeries.Count -gt 0) { [int64]$cpuSeries[0].private_bytes } else { 0 }
        final_private_bytes = if ($cpuSeries.Count -gt 0) { [int64]$cpuSeries[$cpuSeries.Count - 1].private_bytes } else { 0 }
        event_count_total = [int64]$lua.event_count_total
        event_count_peak_per_frame = [int64]$lua.event_count_peak_per_frame
        signal_count_total = [int64]$lua.signal_count_total
        signal_count_peak_per_frame = [int64]$lua.signal_count_peak_per_frame
        fixture_type_count = [int64]$lua.fixture_type_count
        fixture_created_type_count = [int64]$lua.fixture_created_type_count
        fixture_visible_type_count = [int64]$lua.fixture_visible_type_count
        signal_stop_pass = [System.Convert]::ToBoolean($lua.signal_stop_pass)
        save_time_first_ms = ConvertTo-FiniteDouble $lua "save_time_first_ms"
        load_time_first_ms = ConvertTo-FiniteDouble $lua "load_time_first_ms"
        save_time_second_ms = ConvertTo-FiniteDouble $lua "save_time_second_ms"
        load_time_second_ms = ConvertTo-FiniteDouble $lua "load_time_second_ms"
        crashed = $false
        hung = $false
        unbounded_growth = "not_tested"
        roundtrip_pass = [System.Convert]::ToBoolean($lua.roundtrip_pass)
        scenario_stop_pass = [System.Convert]::ToBoolean($lua.scenario_stop_pass)
        scenario_recovery_pass = [System.Convert]::ToBoolean($lua.scenario_recovery_pass)
        stop_event_delta = [int64]$lua.stop_event_delta
        scenario_recovery_assertions = [int64]$lua.scenario_recovery_assertions
        long_run = [System.Convert]::ToBoolean($lua.long_run)
        long_run_save_load_cycles = [int64]$lua.long_run_save_load_cycles
        long_run_language_switches = [int64]$lua.long_run_language_switches
        long_run_module_toggle_cycles = [int64]$lua.long_run_module_toggle_cycles
        long_run_settings_recovery_pass = [System.Convert]::ToBoolean($lua.long_run_settings_recovery_pass)
        long_run_checkpoint_save_ms_total = ConvertTo-FiniteDouble $lua "long_run_checkpoint_save_ms_total"
        long_run_checkpoint_load_ms_total = ConvertTo-FiniteDouble $lua "long_run_checkpoint_load_ms_total"
        simulation_steps = [int64]$lua.simulation_steps
        heartbeat_count = [int64]$lua.heartbeat_count
        heartbeat_interval_seconds = ConvertTo-FiniteDouble $lua "heartbeat_interval_seconds"
        maximum_heartbeat_gap_seconds = ConvertTo-FiniteDouble $lua "maximum_heartbeat_gap_seconds"
        stalls = [int64]$lua.stalls
        nan_count = [int64]$lua.nan_count
        inf_count = [int64]$lua.inf_count
        omni_atmosphere_active = [System.Convert]::ToBoolean($lua.omni_atmosphere_active)
        atmosphere_mass_initial_kg = ConvertTo-FiniteDouble $lua "atmosphere_mass_initial_kg"
        atmosphere_mass_final_kg = ConvertTo-FiniteDouble $lua "atmosphere_mass_final_kg"
        atmosphere_mass_min_kg = ConvertTo-FiniteDouble $lua "atmosphere_mass_min_kg"
        atmosphere_mass_max_kg = ConvertTo-FiniteDouble $lua "atmosphere_mass_max_kg"
        atmosphere_mass_residual_abs_max_kg = ConvertTo-FiniteDouble $lua "atmosphere_mass_residual_abs_max_kg"
        species_mass_residual_abs_max_kg = ConvertTo-FiniteDouble $lua "species_mass_residual_abs_max_kg"
        minimum_density_kg_m3 = ConvertTo-FiniteDouble $lua "minimum_density_kg_m3"
        maximum_density_kg_m3 = ConvertTo-FiniteDouble $lua "maximum_density_kg_m3"
        minimum_pressure_pa = ConvertTo-FiniteDouble $lua "minimum_pressure_pa"
        maximum_pressure_pa = ConvertTo-FiniteDouble $lua "maximum_pressure_pa"
        minimum_temperature_k = ConvertTo-FiniteDouble $lua "minimum_temperature_k"
        maximum_temperature_k = ConvertTo-FiniteDouble $lua "maximum_temperature_k"
        smoke_run = [bool]$Smoke
        gate_result = if ($Smoke) { "not_tested" } else { "pending_independent_assessment" }
        notes = "Simulation FPS is measured as completed stress-loop sim.updateUpTo calls per wall-clock second. Long-run checkpoints perform ten additional same-process OPS cycles, bilingual state switches, and five-module disable/enable cycles. GUI frame presentation, display/DPI, and mathematical long-term boundedness are not inferred."
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
    Write-Output "long_run=$([bool]$LongRun)"
    Write-Output "smoke_run=$([bool]$Smoke)"
    Write-Output "stress_gate=pending_independent_assessment"
    Write-Output "result_json=$jsonPath"
}
finally {
    if ($completed -and -not $KeepArtifacts -and (Test-Path -LiteralPath $testRoot)) {
        Remove-Item -LiteralPath $testRoot -Recurse -Force
    }
}
