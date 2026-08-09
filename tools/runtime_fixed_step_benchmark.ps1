param(
    [Parameter(Mandatory = $true)]
    [string] $Executable,

    [ValidateSet("empty", "mixed-medium")]
    [string] $Scenario = "mixed-medium",

    [ValidateRange(0, 100000)]
    [int] $WarmupSteps = 30,

    [ValidateRange(1, 1000000)]
    [int] $StepsPerPass = 120,

    [ValidateRange(1, 50)]
    [int] $Passes = 5,

    [uint32] $SeedA = 101,
    [uint32] $SeedB = 202,
    [uint32] $SeedC = 303,
    [uint32] $SeedD = 404,

    [string] $BuildDirectory,

    [string] $RuntimeDirectory = "C:\msys64\ucrt64\bin",

    [string] $OutputDirectory = "artifacts/vnext-benchmark",

    [string] $TemporaryDirectory = [System.IO.Path]::GetTempPath(),

    [ValidateRange(10, 7200)]
    [int] $TimeoutSeconds = 900,

    [switch] $Smoke,

    [switch] $EnableOmniProfiler,

    [switch] $KeepTemporary
)

$ErrorActionPreference = "Stop"
$ProgressPreference = "SilentlyContinue"

if ($Smoke) {
    $WarmupSteps = 2
    $StepsPerPass = 10
    $Passes = 2
}

$sourceRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot "..")).Path
$resolvedExecutable = (Resolve-Path -LiteralPath $Executable).Path
$resolvedBuildDirectory = if ($BuildDirectory) {
    (Resolve-Path -LiteralPath $BuildDirectory).Path
}
else {
    (Get-Item -LiteralPath $resolvedExecutable).Directory.FullName
}
$luaSource = Join-Path $PSScriptRoot "runtime\fixed_step_benchmark.lua"
if (-not (Test-Path -LiteralPath $luaSource -PathType Leaf)) {
    throw "Missing fixed-step Lua source: $luaSource"
}

$resolvedRuntimeDirectory = $null
if ($RuntimeDirectory) {
    $resolvedRuntimeDirectory = (Resolve-Path -LiteralPath $RuntimeDirectory).Path
    if (-not (Test-Path -LiteralPath $resolvedRuntimeDirectory -PathType Container)) {
        throw "Runtime directory is not a directory: $resolvedRuntimeDirectory"
    }
}

function Get-TextSha256 {
    param([Parameter(Mandatory = $true)][string] $Text)
    $bytes = [System.Text.Encoding]::UTF8.GetBytes($Text)
    return [Convert]::ToHexString(
        [System.Security.Cryptography.SHA256]::HashData($bytes)
    )
}

function Read-KeyValueFile {
    param([Parameter(Mandatory = $true)][string] $Path)
    $values = @{}
    foreach ($line in Get-Content -LiteralPath $Path) {
        if ($line -notmatch '^([A-Za-z0-9_]+)=(.*)$') {
            throw "Invalid fixed-step Lua result line: $line"
        }
        if ($values.ContainsKey($matches[1])) {
            throw "Duplicate fixed-step Lua result key: $($matches[1])"
        }
        $values[$matches[1]] = $matches[2]
    }
    return $values
}

function Convert-InvariantDouble {
    param(
        [Parameter(Mandatory = $true)][string] $Text,
        [Parameter(Mandatory = $true)][string] $Field
    )
    $value = 0.0
    if (-not [double]::TryParse(
        $Text,
        [System.Globalization.NumberStyles]::Float,
        [System.Globalization.CultureInfo]::InvariantCulture,
        [ref]$value
    )) {
        throw "Fixed-step Lua result field is not a finite-format number: $Field"
    }
    if ([double]::IsNaN($value) -or [double]::IsInfinity($value)) {
        throw "Fixed-step Lua result field is not finite: $Field"
    }
    return $value
}

function Convert-NonnegativeInteger {
    param(
        [Parameter(Mandatory = $true)][string] $Text,
        [Parameter(Mandatory = $true)][string] $Field
    )
    if ($Text -notmatch '^\d+$') {
        throw "Fixed-step Lua result field is not a nonnegative integer: $Field"
    }
    return [int64]$Text
}

function Get-Percentile {
    param(
        [Parameter(Mandatory = $true)][double[]] $Values,
        [Parameter(Mandatory = $true)][ValidateRange(0.0, 1.0)][double] $Percentile
    )
    if ($Values.Count -eq 0) {
        throw "Cannot calculate a percentile from an empty series"
    }
    [double[]]$sorted = $Values | Sort-Object
    if ($sorted.Count -eq 1) {
        return $sorted[0]
    }
    $position = ($sorted.Count - 1) * $Percentile
    $lower = [Math]::Floor($position)
    $upper = [Math]::Ceiling($position)
    if ($lower -eq $upper) {
        return $sorted[$lower]
    }
    $fraction = $position - $lower
    return $sorted[$lower] + ($sorted[$upper] - $sorted[$lower]) * $fraction
}

function Remove-IsolatedRoot {
    param(
        [Parameter(Mandatory = $true)][string] $Path,
        [Parameter(Mandatory = $true)][string] $Parent
    )
    $fullPath = [System.IO.Path]::GetFullPath($Path)
    $fullParent = [System.IO.Path]::GetFullPath($Parent).TrimEnd(
        [System.IO.Path]::DirectorySeparatorChar,
        [System.IO.Path]::AltDirectorySeparatorChar
    ) + [System.IO.Path]::DirectorySeparatorChar
    if (-not $fullPath.StartsWith($fullParent, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to remove a benchmark directory outside the selected temporary root"
    }
    for ($attempt = 1; $attempt -le 20; $attempt++) {
        try {
            if (Test-Path -LiteralPath $fullPath) {
                Remove-Item -LiteralPath $fullPath -Recurse -Force
            }
            return
        }
        catch {
            if ($attempt -eq 20) {
                Write-Warning "Unable to remove isolated benchmark directory: $fullPath"
                return
            }
            Start-Sleep -Milliseconds 100
        }
    }
}

function Get-GitSourceState {
    param([Parameter(Mandatory = $true)][string] $Repository)
    $head = (& git -C $Repository rev-parse HEAD).Trim()
    if ($LASTEXITCODE -ne 0 -or $head -notmatch '^[0-9a-f]{40}$') {
        throw "Cannot resolve benchmark source HEAD"
    }
    $statusLines = @(& git -C $Repository status --porcelain=v1 --untracked-files=all)
    if ($LASTEXITCODE -ne 0) {
        throw "Cannot inspect benchmark source worktree"
    }
    $dirty = $statusLines.Count -gt 0
    $material = [System.Text.StringBuilder]::new()
    [void]$material.AppendLine("HEAD=$head")
    foreach ($line in $statusLines) {
        [void]$material.AppendLine("STATUS=$line")
    }
    if ($dirty) {
        foreach ($line in @(& git -C $Repository diff --binary HEAD -- .)) {
            [void]$material.AppendLine("DIFF=$line")
        }
        if ($LASTEXITCODE -ne 0) {
            throw "Cannot hash tracked benchmark source changes"
        }
        $untracked = @(& git -C $Repository -c core.quotepath=false ls-files --others --exclude-standard)
        if ($LASTEXITCODE -ne 0) {
            throw "Cannot enumerate untracked benchmark source files"
        }
        foreach ($relativePath in $untracked) {
            $fullPath = Join-Path $Repository $relativePath
            if (Test-Path -LiteralPath $fullPath -PathType Leaf) {
                $hash = (Get-FileHash -LiteralPath $fullPath -Algorithm SHA256).Hash
                [void]$material.AppendLine("UNTRACKED=$relativePath|$hash")
            }
        }
    }
    return [pscustomobject]@{
        Commit = $head
        Dirty = $dirty
        StateSha256 = Get-TextSha256 -Text $material.ToString()
        StatusLines = [string[]]$statusLines
    }
}

function Get-BuildProvenance {
    param([Parameter(Mandatory = $true)][string] $Directory)
    $mesonInfo = Join-Path $Directory "meson-info"
    $optionsPath = Join-Path $mesonInfo "intro-buildoptions.json"
    $compilersPath = Join-Path $mesonInfo "intro-compilers.json"
    $compileCommandsPath = Join-Path $Directory "compile_commands.json"
    foreach ($required in @($optionsPath, $compilersPath, $compileCommandsPath)) {
        if (-not (Test-Path -LiteralPath $required -PathType Leaf)) {
            throw "Build provenance file is missing: $required"
        }
    }

    $optionRows = Get-Content -LiteralPath $optionsPath -Raw | ConvertFrom-Json
    $options = [ordered]@{}
    foreach ($row in $optionRows) {
        $options[$row.name] = $row.value
    }
    if (-not $options.Contains("fp_mode")) {
        throw "Meson build does not expose fp_mode"
    }

    $compilers = Get-Content -LiteralPath $compilersPath -Raw | ConvertFrom-Json
    $compileCommands = Get-Content -LiteralPath $compileCommandsPath -Raw | ConvertFrom-Json
    $simulationCommand = $compileCommands | Where-Object {
        ($_.file -replace '\\', '/') -match 'src/simulation/Simulation\.cpp$'
    } | Select-Object -First 1
    if (-not $simulationCommand -or -not $simulationCommand.command) {
        throw "Cannot find the Simulation.cpp compile command"
    }

    $vcsTag = "not_recorded"
    $vcsTagPath = Join-Path $Directory "src\VcsTag.h"
    if (Test-Path -LiteralPath $vcsTagPath -PathType Leaf) {
        $vcsText = Get-Content -LiteralPath $vcsTagPath -Raw
        if ($vcsText -match 'VCS_TAG\[\]\s*=\s*"([^"]*)"') {
            $vcsTag = if ($matches[1]) { $matches[1] } else { "empty" }
        }
    }

    return [pscustomobject]@{
        FpMode = [string]$options["fp_mode"]
        Options = $options
        Compiler = $compilers.host.cpp
        SimulationCompileCommand = [string]$simulationCommand.command
        VcsTag = $vcsTag
    }
}

$gitState = Get-GitSourceState -Repository $sourceRoot
$build = Get-BuildProvenance -Directory $resolvedBuildDirectory
$executableInfo = Get-Item -LiteralPath $resolvedExecutable
$luaSha256 = (Get-FileHash -LiteralPath $luaSource -Algorithm SHA256).Hash

$tempParent = [System.IO.Path]::GetFullPath($TemporaryDirectory)
if (-not (Test-Path -LiteralPath $tempParent -PathType Container)) {
    throw "Temporary directory does not exist: $tempParent"
}
$runId = [DateTime]::UtcNow.ToString("yyyyMMddTHHmmssZ") + "-" +
    [guid]::NewGuid().ToString("N").Substring(0, 8)
$testRoot = [System.IO.Path]::GetFullPath(
    (Join-Path $tempParent ("tpt-omnipack-fixed-step-" + $runId))
)
$tempPrefix = $tempParent.TrimEnd(
    [System.IO.Path]::DirectorySeparatorChar,
    [System.IO.Path]::AltDirectorySeparatorChar
) + [System.IO.Path]::DirectorySeparatorChar
if (-not $testRoot.StartsWith($tempPrefix, [System.StringComparison]::OrdinalIgnoreCase)) {
    throw "Refusing to create a benchmark directory outside the selected temporary root"
}

$process = $null
$stdoutTask = $null
$stderrTask = $null
$completed = $false
try {
    New-Item -ItemType Directory -Path $testRoot | Out-Null
    $utf8NoBom = [System.Text.UTF8Encoding]::new($false)
    $luaHookTimeoutMs = 900000
    [System.IO.File]::WriteAllText(
        (Join-Path $testRoot "powder.pref"),
        ('{"LuaHookTimeout":' + $luaHookTimeoutMs + '}') + [Environment]::NewLine,
        $utf8NoBom
    )
    Copy-Item -LiteralPath $luaSource -Destination (Join-Path $testRoot "autorun.lua")
    $configPath = Join-Path $testRoot "fixed-step-benchmark.config"
    [System.IO.File]::WriteAllLines(
        $configPath,
        @(
            "schema_version=1",
            "scenario=$Scenario",
            "warmup_steps=$WarmupSteps",
            "steps_per_pass=$StepsPerPass",
            "passes=$Passes",
            "omni_profiler_enabled=$([int][bool]$EnableOmniProfiler)",
            "seed_a=$SeedA",
            "seed_b=$SeedB",
            "seed_c=$SeedC",
            "seed_d=$SeedD"
        ),
        [System.Text.Encoding]::ASCII
    )
    $configSha256 = (Get-FileHash -LiteralPath $configPath -Algorithm SHA256).Hash

    $startInfo = [System.Diagnostics.ProcessStartInfo]::new()
    $startInfo.FileName = $resolvedExecutable
    $startInfo.WorkingDirectory = $testRoot
    $startInfo.UseShellExecute = $false
    $startInfo.CreateNoWindow = $true
    $startInfo.RedirectStandardOutput = $true
    $startInfo.RedirectStandardError = $true
    $startInfo.ArgumentList.Add("ddir")
    $startInfo.ArgumentList.Add($testRoot)
    if ($resolvedRuntimeDirectory) {
        $startInfo.Environment["PATH"] = $resolvedRuntimeDirectory +
            [System.IO.Path]::PathSeparator + $env:PATH
    }
    foreach ($secretName in @("GITHUB_PAT_TOKEN", "GITHUB_TOKEN", "GH_TOKEN")) {
        [void]$startInfo.Environment.Remove($secretName)
    }

    $startedAt = [DateTime]::UtcNow
    $process = [System.Diagnostics.Process]::Start($startInfo)
    if (-not $process) {
        throw "Failed to start the fixed-step benchmark client"
    }
    $stdoutTask = $process.StandardOutput.ReadToEndAsync()
    $stderrTask = $process.StandardError.ReadToEndAsync()
    $deadline = $startedAt.AddSeconds($TimeoutSeconds)
    $sampleIntervalMs = 50
    $processSeries = [System.Collections.Generic.List[object]]::new()
    $peakWorkingSetSampled = [int64]0
    $peakPrivateBytesSampled = [int64]0
    do {
        $process.Refresh()
        if (-not $process.HasExited) {
            $peakWorkingSetSampled = [Math]::Max(
                $peakWorkingSetSampled,
                [int64]$process.WorkingSet64
            )
            $peakPrivateBytesSampled = [Math]::Max(
                $peakPrivateBytesSampled,
                [int64]$process.PrivateMemorySize64
            )
            $processSeries.Add([pscustomobject]@{
                elapsed_seconds = [Math]::Round(
                    ([DateTime]::UtcNow - $startedAt).TotalSeconds,
                    6
                )
                total_cpu_seconds = [Math]::Round(
                    $process.TotalProcessorTime.TotalSeconds,
                    6
                )
                working_set_bytes = [int64]$process.WorkingSet64
                private_bytes = [int64]$process.PrivateMemorySize64
            })
            Start-Sleep -Milliseconds $sampleIntervalMs
        }
    } while (-not $process.HasExited -and [DateTime]::UtcNow -lt $deadline)

    if (-not $process.HasExited) {
        Stop-Process -Id $process.Id -Force
        $process.WaitForExit()
        $stdout = $stdoutTask.GetAwaiter().GetResult()
        $stderr = $stderrTask.GetAwaiter().GetResult()
        [System.IO.File]::WriteAllText(
            (Join-Path $testRoot "client-stdout.log"), $stdout, $utf8NoBom
        )
        [System.IO.File]::WriteAllText(
            (Join-Path $testRoot "client-stderr.log"), $stderr, $utf8NoBom
        )
        throw "Fixed-step benchmark timed out; stdout=$($stdout.Trim()); stderr=$($stderr.Trim()); artifacts=$testRoot"
    }
    $process.WaitForExit()
    $stdout = $stdoutTask.GetAwaiter().GetResult()
    $stderr = $stderrTask.GetAwaiter().GetResult()
    [System.IO.File]::WriteAllText(
        (Join-Path $testRoot "client-stdout.log"), $stdout, $utf8NoBom
    )
    [System.IO.File]::WriteAllText(
        (Join-Path $testRoot "client-stderr.log"), $stderr, $utf8NoBom
    )
    $process.Refresh()
    $exitCode = $process.ExitCode
    $processWallSeconds = [Math]::Max(
        0.000001,
        ([DateTime]::UtcNow - $startedAt).TotalSeconds
    )
    $processCpuSeconds = $process.TotalProcessorTime.TotalSeconds
    $peakWorkingSetBytes = [Math]::Max(
        $peakWorkingSetSampled,
        [int64]$process.PeakWorkingSet64
    )
    $peakPrivateBytesSampled = [Math]::Max(
        $peakPrivateBytesSampled,
        [int64]$process.PrivateMemorySize64
    )

    $luaResultPath = Join-Path $testRoot "fixed-step-lua.result"
    if (-not (Test-Path -LiteralPath $luaResultPath -PathType Leaf)) {
        throw "Fixed-step client exited without a Lua result; exit_code=$exitCode; stdout=$($stdout.Trim()); stderr=$($stderr.Trim()); artifacts=$testRoot"
    }
    $lua = Read-KeyValueFile -Path $luaResultPath
    if ($exitCode -ne 0 -or $lua.OMNI_FIXED_STEP_STATUS -ne "PASS") {
        throw "Fixed-step Lua failed; exit_code=$exitCode; error=$($lua.error); artifacts=$testRoot"
    }

    foreach ($field in @(
        "schema_version", "scenario", "warmup_steps", "steps_per_pass", "passes",
        "seed", "atmosphere_cells", "simulation_dt_value", "simulation_dt_unit",
        "reset_sanitization_steps", "timing_scope", "omni_profiler_enabled", "generated_particles", "initial_particles", "final_particles",
        "initial_state_hash", "final_state_hash", "deterministic_replay"
    )) {
        if (-not $lua.ContainsKey($field)) {
            throw "Fixed-step Lua result is missing field: $field"
        }
    }
    if ($lua.scenario -ne $Scenario) {
        throw "Fixed-step Lua scenario does not match the requested scenario"
    }
    if ((Convert-NonnegativeInteger $lua.warmup_steps "warmup_steps") -ne $WarmupSteps -or
        (Convert-NonnegativeInteger $lua.steps_per_pass "steps_per_pass") -ne $StepsPerPass -or
        (Convert-NonnegativeInteger $lua.passes "passes") -ne $Passes) {
        throw "Fixed-step Lua step counts do not match the requested run"
    }
    if ($lua.omni_profiler_enabled -ne $EnableOmniProfiler.ToString().ToLowerInvariant()) {
        throw "Fixed-step Lua profiler state does not match the requested run"
    }
    if ($lua.deterministic_replay -ne "true") {
        throw "Fixed-step Lua did not prove same-process deterministic replay"
    }

    $passRecords = [System.Collections.Generic.List[object]]::new()
    $elapsedValues = [System.Collections.Generic.List[double]]::new()
    for ($pass = 1; $pass -le $Passes; $pass++) {
        $prefix = "pass_$($pass)_"
        foreach ($suffix in @(
            "elapsed_seconds", "generated_hash", "initial_hash", "final_hash",
            "initial_particles", "final_particles", "profiler_frame_calls", "profiler_simulation_calls"
        )) {
            if (-not $lua.ContainsKey($prefix + $suffix)) {
                throw "Fixed-step Lua result is missing field: $prefix$suffix"
            }
        }
        $elapsed = Convert-InvariantDouble $lua[$prefix + "elapsed_seconds"] ($prefix + "elapsed_seconds")
        if ($elapsed -le 0.0) {
            throw "Fixed-step Lua pass elapsed time is not positive: $pass"
        }
        $expectedProfilerCalls = if ($EnableOmniProfiler) { [int64]$StepsPerPass } else { [int64]0 }
        foreach ($profilerField in @("profiler_frame_calls", "profiler_simulation_calls")) {
            if ((Convert-NonnegativeInteger $lua[$prefix + $profilerField] ($prefix + $profilerField)) -ne $expectedProfilerCalls) {
                throw "Fixed-step Lua profiler calls do not match the requested mode: $prefix$profilerField"
            }
        }
        $elapsedValues.Add($elapsed)
        $passRecords.Add([pscustomobject][ordered]@{
            pass = $pass
            elapsed_seconds = $elapsed
            steps = $StepsPerPass
            steps_per_second = [Math]::Round($StepsPerPass / $elapsed, 6)
            milliseconds_per_step = [Math]::Round(1000.0 * $elapsed / $StepsPerPass, 6)
            generated_state_hash_fnv1a32 = [string]$lua[$prefix + "generated_hash"]
            initial_state_hash_fnv1a32 = [string]$lua[$prefix + "initial_hash"]
            final_state_hash_fnv1a32 = [string]$lua[$prefix + "final_hash"]
            initial_particles = Convert-NonnegativeInteger $lua[$prefix + "initial_particles"] ($prefix + "initial_particles")
            final_particles = Convert-NonnegativeInteger $lua[$prefix + "final_particles"] ($prefix + "final_particles")
            omni_profiler_frame_calls = Convert-NonnegativeInteger $lua[$prefix + "profiler_frame_calls"] ($prefix + "profiler_frame_calls")
            omni_profiler_simulation_calls = Convert-NonnegativeInteger $lua[$prefix + "profiler_simulation_calls"] ($prefix + "profiler_simulation_calls")
        })
    }

    [double]$totalMeasuredSeconds = ($elapsedValues | Measure-Object -Sum).Sum
    [int64]$totalMeasuredSteps = [int64]$StepsPerPass * $Passes
    $logicalCpuCount = [int][Environment]::ProcessorCount
    $cpuRows = @(Get-CimInstance Win32_Processor -ErrorAction SilentlyContinue)
    $computer = Get-CimInstance Win32_ComputerSystem -ErrorAction Stop
    $os = Get-CimInstance Win32_OperatingSystem -ErrorAction Stop
    $gpuRows = @(Get-CimInstance Win32_VideoController -ErrorAction SilentlyContinue)
    $cpuModel = if ($cpuRows.Count) { [string]$cpuRows[0].Name.Trim() } else { "not_tested" }
    $physicalCores = if ($cpuRows.Count) {
        [int](($cpuRows | Measure-Object -Property NumberOfCores -Sum).Sum)
    }
    else { "not_tested" }
    $machineMaterial = "$($env:COMPUTERNAME)|$cpuModel|$($computer.TotalPhysicalMemory)"
    $machineId = "windows-" + (Get-TextSha256 -Text $machineMaterial).Substring(0, 12)
    $gpuInventory = @($gpuRows | ForEach-Object {
        [pscustomobject][ordered]@{
            name = [string]$_.Name
            driver_version = [string]$_.DriverVersion
            pnp_device_id = [string]$_.PNPDeviceID
            adapter_ram_bytes_wmi = if ($null -ne $_.AdapterRAM) { [uint64]$_.AdapterRAM } else { "not_tested" }
        }
    })

    $nvidiaInventory = @()
    $nvidiaSmi = Get-Command nvidia-smi -ErrorAction SilentlyContinue
    if ($nvidiaSmi) {
        $nvidiaLines = @(& $nvidiaSmi.Source `
            --query-gpu=index,name,uuid,driver_version,memory.total `
            --format=csv,noheader,nounits 2>$null)
        if ($LASTEXITCODE -eq 0) {
            $nvidiaInventory = @($nvidiaLines | ForEach-Object {
                $columns = $_ -split ',\s*', 5
                if ($columns.Count -eq 5) {
                    [pscustomobject][ordered]@{
                        index = [int]$columns[0]
                        name = $columns[1]
                        uuid = $columns[2]
                        driver_version = $columns[3]
                        total_vram_bytes = [int64]$columns[4] * 1MB
                    }
                }
            })
        }
    }

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
    }
    else {
        [System.IO.Path]::GetFullPath((Join-Path $sourceRoot $OutputDirectory))
    }
    $safeFpMode = $build.FpMode -replace '[^A-Za-z0-9_.-]', '_'
    $artifactDirectory = Join-Path $artifactBase (
        Join-Path $machineId (Join-Path $Scenario (Join-Path $safeFpMode $runId))
    )
    New-Item -ItemType Directory -Path $artifactDirectory -Force | Out-Null
    Copy-Item -LiteralPath $luaResultPath -Destination (Join-Path $artifactDirectory "fixed-step-lua.result")
    Copy-Item -LiteralPath $configPath -Destination (Join-Path $artifactDirectory "fixed-step-benchmark.config")
    Copy-Item -LiteralPath $luaSource -Destination (Join-Path $artifactDirectory "fixed_step_benchmark.lua")
    Copy-Item -LiteralPath (Join-Path $testRoot "client-stdout.log") -Destination (Join-Path $artifactDirectory "client-stdout.log")
    Copy-Item -LiteralPath (Join-Path $testRoot "client-stderr.log") -Destination (Join-Path $artifactDirectory "client-stderr.log")
    $processSeries | Export-Csv `
        -LiteralPath (Join-Path $artifactDirectory "process-series.csv") `
        -NoTypeInformation `
        -Encoding utf8

    $record = [ordered]@{
        schema_version = 1
        benchmark_kind = "client_fixed_step_simulation"
        benchmark_execution_status = "PASS"
        performance_gate = "not_evaluated"
        run_id = $runId
        recorded_at_utc = [DateTime]::UtcNow.ToString("o")
        source = [ordered]@{
            commit = $gitState.Commit
            dirty = [bool]$gitState.Dirty
            worktree_state_sha256 = $gitState.StateSha256
            status_lines = [string[]]$gitState.StatusLines
            harness_powershell_sha256 = (Get-FileHash -LiteralPath $PSCommandPath -Algorithm SHA256).Hash
            lua_generator_sha256 = $luaSha256
            run_config_sha256 = $configSha256
        }
        executable = [ordered]@{
            path = $resolvedExecutable
            sha256 = (Get-FileHash -LiteralPath $resolvedExecutable -Algorithm SHA256).Hash
            length_bytes = [int64]$executableInfo.Length
            last_write_utc = $executableInfo.LastWriteTimeUtc.ToString("o")
            vcs_tag = $build.VcsTag
        }
        build = [ordered]@{
            directory = $resolvedBuildDirectory
            fp_mode = $build.FpMode
            meson_options = $build.Options
            compiler = $build.Compiler
            simulation_compile_command = $build.SimulationCompileCommand
        }
        runtime = [ordered]@{
            backend = "legacy_cpu_client"
            gpu_compute_backend = "none"
            isolated_data_directory = $true
            user_preferences_imported = $false
            lua_hook_timeout_ms = $luaHookTimeoutMs
            runtime_path_injected = [bool]$resolvedRuntimeDirectory
            runtime_directory = if ($resolvedRuntimeDirectory) { $resolvedRuntimeDirectory } else { "not_injected" }
            portable_runtime_tested = $false
            omni_profiler_enabled = [bool]$EnableOmniProfiler
            secret_environment_removed = @("GITHUB_PAT_TOKEN", "GITHUB_TOKEN", "GH_TOKEN")
        }
        machine = [ordered]@{
            machine_id = $machineId
            os = "$($os.Caption) $($os.Version) build $($os.BuildNumber)"
            cpu_model = $cpuModel
            physical_core_count = $physicalCores
            logical_cpu_count = $logicalCpuCount
            cpu_max_clock_mhz = if ($cpuRows.Count) { [int]$cpuRows[0].MaxClockSpeed } else { "not_tested" }
            physical_memory_bytes = [int64]$computer.TotalPhysicalMemory
            free_physical_memory_at_manifest_collection_bytes = [int64]$os.FreePhysicalMemory * 1KB
            gpu_wmi = $gpuInventory
            gpu_nvidia_smi = $nvidiaInventory
            power_mode = $powerMode
        }
        scenario = [ordered]@{
            id = $Scenario
            definition = "builtin:$Scenario:v1"
            seed = @([uint64]$SeedA, [uint64]$SeedB, [uint64]$SeedC, [uint64]$SeedD)
            generated_particles = Convert-NonnegativeInteger $lua.generated_particles "generated_particles"
            initial_particles_after_warmup = Convert-NonnegativeInteger $lua.initial_particles "initial_particles"
            final_particles = Convert-NonnegativeInteger $lua.final_particles "final_particles"
            atmosphere_cell_count = Convert-NonnegativeInteger $lua.atmosphere_cells "atmosphere_cells"
            active_chunk_count = "not_available_legacy"
            reaction_candidate_count = "not_available_legacy"
        }
        measurement = [ordered]@{
            warmup_steps_per_pass = $WarmupSteps
            steps_per_pass = $StepsPerPass
            passes = $Passes
            total_measured_steps = $totalMeasuredSteps
            total_measured_seconds = [Math]::Round($totalMeasuredSeconds, 9)
            aggregate_steps_per_second = [Math]::Round($totalMeasuredSteps / $totalMeasuredSeconds, 6)
            aggregate_milliseconds_per_step = [Math]::Round(1000.0 * $totalMeasuredSeconds / $totalMeasuredSteps, 6)
            pass_seconds_p50 = [Math]::Round((Get-Percentile ([double[]]$elapsedValues) 0.50), 9)
            pass_seconds_p95 = [Math]::Round((Get-Percentile ([double[]]$elapsedValues) 0.95), 9)
            pass_seconds_min = [Math]::Round(($elapsedValues | Measure-Object -Minimum).Minimum, 9)
            pass_seconds_max = [Math]::Round(($elapsedValues | Measure-Object -Maximum).Maximum, 9)
            simulation_dt_value = 1
            simulation_dt_unit = "legacy_tick"
            reset_sanitization_steps_per_pass = Convert-NonnegativeInteger $lua.reset_sanitization_steps "reset_sanitization_steps"
            deterministic_replay = $true
            state_signature_algorithm = "Snapshot::Hash FNV-1a 32-bit"
            initial_state_hash_fnv1a32 = [string]$lua.initial_state_hash
            final_state_hash_fnv1a32 = [string]$lua.final_state_hash
            timing_scope = [string]$lua.timing_scope
            omni_profiler_enabled = [bool]$EnableOmniProfiler
            scene_generation_in_timed_region = $false
            reset_sanitization_in_timed_region = $false
            warmup_in_timed_region = $false
            state_signature_in_timed_region = $false
            rendering_in_timed_region = $false
            fps_wait_in_timed_region = $false
            lua_dispatch_in_timed_region = $true
            subsystem_timings_available = $false
            passes_detail = [object[]]$passRecords
        }
        process = [ordered]@{
            wall_seconds_including_startup = [Math]::Round($processWallSeconds, 6)
            total_cpu_seconds = [Math]::Round($processCpuSeconds, 6)
            average_cpu_percent_normalized = [Math]::Round(
                100.0 * $processCpuSeconds / $processWallSeconds / $logicalCpuCount,
                6
            )
            peak_working_set_bytes = $peakWorkingSetBytes
            peak_private_bytes_sampled = $peakPrivateBytesSampled
            process_sampling_interval_ms = $sampleIntervalMs
            peak_process_vram_bytes = "not_tested"
            peak_process_vram_reason = "per-process GPU polling would perturb this short CPU benchmark"
        }
        limitations = @(
            "This is the real client Simulation path driven synchronously through Lua, not a pure C++ microbenchmark.",
            "The timed region has no frame limiter or renderer call, but it includes Lua-to-C dispatch for each fixed step.",
            "Per-process VRAM is not available in this foundation runner; detailed subsystem exports remain in the separate profiler runtime fixture.",
            "Performance thresholds and regression budgets are not evaluated by this execution."
        )
    }
    $jsonPath = Join-Path $artifactDirectory "result.json"
    [System.IO.File]::WriteAllText(
        $jsonPath,
        ($record | ConvertTo-Json -Depth 12) + [Environment]::NewLine,
        $utf8NoBom
    )

    $completed = $true
    Write-Output "runtime-fixed-step-benchmark: PASS"
    Write-Output "scenario=$Scenario"
    Write-Output "fp_mode=$($build.FpMode)"
    Write-Output "source_commit=$($gitState.Commit)"
    Write-Output "source_dirty=$($gitState.Dirty.ToString().ToLowerInvariant())"
    Write-Output "aggregate_steps_per_second=$([Math]::Round($totalMeasuredSteps / $totalMeasuredSeconds, 6))"
    Write-Output "final_state_hash_fnv1a32=$($lua.final_state_hash)"
    Write-Output "performance_gate=not_evaluated"
    Write-Output "result_json=$jsonPath"
}
finally {
    if ($process) {
        try {
            if (-not $process.HasExited) {
                Stop-Process -Id $process.Id -Force
                $process.WaitForExit()
            }
        }
        finally {
            $process.Dispose()
        }
    }
    if ($completed -and -not $KeepTemporary -and (Test-Path -LiteralPath $testRoot)) {
        Remove-IsolatedRoot -Path $testRoot -Parent $tempParent
    }
}
