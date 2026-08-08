param(
    [Parameter(Mandatory = $true)]
    [string] $LeftExecutable,

    [Parameter(Mandatory = $true)]
    [string] $RightExecutable,

    [string] $LeftBuildDirectory,

    [string] $RightBuildDirectory,

    [string] $LeftLabel = "legacy-fast",

    [string] $RightLabel = "strict",

    [string] $ExpectedLeftFpMode = "legacy_fast",

    [string] $ExpectedRightFpMode = "strict",

    [ValidateSet("empty", "mixed-medium")]
    [string] $Scenario = "mixed-medium",

    [ValidateRange(1, 100000)]
    [int] $TraceSteps = 120,

    [uint32] $SeedA = 101,
    [uint32] $SeedB = 202,
    [uint32] $SeedC = 303,
    [uint32] $SeedD = 404,

    [string] $RuntimeDirectory = "C:\msys64\ucrt64\bin",

    [string] $PythonExecutable = "C:\msys64\ucrt64\bin\python.exe",

    [string] $OutputDirectory = "artifacts/vnext-differential",

    [string] $TemporaryDirectory = [System.IO.Path]::GetTempPath(),

    [ValidateRange(30, 7200)]
    [int] $TimeoutSeconds = 900,

    [switch] $KeepTemporary
)

$ErrorActionPreference = "Stop"
$ProgressPreference = "SilentlyContinue"

foreach ($label in @($LeftLabel, $RightLabel)) {
    if ($label -notmatch '^[A-Za-z0-9][A-Za-z0-9._-]{0,63}$') {
        throw "Unsafe differential label: $label"
    }
}
if ($LeftLabel -eq $RightLabel) {
    throw "Differential labels must be distinct"
}

$sourceRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot "..")).Path
$resolvedLeftExecutable = (Resolve-Path -LiteralPath $LeftExecutable).Path
$resolvedRightExecutable = (Resolve-Path -LiteralPath $RightExecutable).Path
$resolvedLeftBuild = if ($LeftBuildDirectory) {
    (Resolve-Path -LiteralPath $LeftBuildDirectory).Path
}
else {
    (Get-Item -LiteralPath $resolvedLeftExecutable).Directory.FullName
}
$resolvedRightBuild = if ($RightBuildDirectory) {
    (Resolve-Path -LiteralPath $RightBuildDirectory).Path
}
else {
    (Get-Item -LiteralPath $resolvedRightExecutable).Directory.FullName
}
$resolvedRuntime = (Resolve-Path -LiteralPath $RuntimeDirectory).Path
$resolvedPython = (Resolve-Path -LiteralPath $PythonExecutable).Path
$luaSource = Join-Path $PSScriptRoot "runtime\differential_probe.lua"
$comparatorSource = Join-Path $PSScriptRoot "differential_compare.py"
foreach ($required in @($luaSource, $comparatorSource)) {
    if (-not (Test-Path -LiteralPath $required -PathType Leaf)) {
        throw "Missing differential source: $required"
    }
}

function Get-TextSha256 {
    param([Parameter(Mandatory = $true)][string] $Text)
    return [Convert]::ToHexString(
        [System.Security.Cryptography.SHA256]::HashData(
            [System.Text.Encoding]::UTF8.GetBytes($Text)
        )
    )
}

function Read-KeyValueFile {
    param([Parameter(Mandatory = $true)][string] $Path)
    $values = @{}
    foreach ($line in Get-Content -LiteralPath $Path) {
        if ($line -notmatch '^([A-Za-z0-9_]+)=(.*)$') {
            throw "Invalid differential result line: $line"
        }
        if ($values.ContainsKey($matches[1])) {
            throw "Duplicate differential result key: $($matches[1])"
        }
        $values[$matches[1]] = $matches[2]
    }
    return $values
}

function Convert-NonnegativeInteger {
    param(
        [Parameter(Mandatory = $true)][string] $Text,
        [Parameter(Mandatory = $true)][string] $Field
    )
    if ($Text -notmatch '^\d+$') {
        throw "Differential result field is not a nonnegative integer: $Field"
    }
    return [int64]$Text
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
        throw "Refusing to remove differential data outside the selected temporary root"
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
                Write-Warning "Unable to remove isolated differential directory: $fullPath"
                return
            }
            Start-Sleep -Milliseconds 100
        }
    }
}

function Get-GitState {
    param([Parameter(Mandatory = $true)][string] $Repository)
    $head = (& git -C $Repository rev-parse HEAD).Trim()
    if ($LASTEXITCODE -ne 0 -or $head -notmatch '^[0-9a-f]{40}$') {
        throw "Cannot resolve differential source HEAD"
    }
    $status = @(& git -C $Repository status --porcelain=v1 --untracked-files=all)
    if ($LASTEXITCODE -ne 0) {
        throw "Cannot inspect differential source worktree"
    }
    $material = [System.Text.StringBuilder]::new()
    [void]$material.AppendLine("HEAD=$head")
    foreach ($line in $status) {
        [void]$material.AppendLine("STATUS=$line")
    }
    foreach ($line in @(& git -C $Repository diff --binary HEAD -- .)) {
        [void]$material.AppendLine("DIFF=$line")
    }
    if ($LASTEXITCODE -ne 0) {
        throw "Cannot hash tracked differential changes"
    }
    foreach ($relativePath in @(
        & git -C $Repository -c core.quotepath=false ls-files --others --exclude-standard
    )) {
        $fullPath = Join-Path $Repository $relativePath
        if (Test-Path -LiteralPath $fullPath -PathType Leaf) {
            [void]$material.AppendLine(
                "UNTRACKED=$relativePath|$((Get-FileHash -LiteralPath $fullPath -Algorithm SHA256).Hash)"
            )
        }
    }
    if ($LASTEXITCODE -ne 0) {
        throw "Cannot enumerate untracked differential files"
    }
    return [pscustomobject]@{
        Commit = $head
        Dirty = $status.Count -gt 0
        StateSha256 = Get-TextSha256 -Text $material.ToString()
        StatusLines = [string[]]$status
    }
}

function Get-BuildProvenance {
    param([Parameter(Mandatory = $true)][string] $Directory)
    $optionsPath = Join-Path $Directory "meson-info\intro-buildoptions.json"
    $compilersPath = Join-Path $Directory "meson-info\intro-compilers.json"
    $commandsPath = Join-Path $Directory "compile_commands.json"
    foreach ($path in @($optionsPath, $compilersPath, $commandsPath)) {
        if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
            throw "Missing build provenance file: $path"
        }
    }
    $options = [ordered]@{}
    foreach ($row in (Get-Content -LiteralPath $optionsPath -Raw | ConvertFrom-Json)) {
        $options[$row.name] = $row.value
    }
    if (-not $options.Contains("fp_mode")) {
        throw "Differential build does not expose fp_mode"
    }
    $commands = Get-Content -LiteralPath $commandsPath -Raw | ConvertFrom-Json
    $simulation = $commands | Where-Object {
        ($_.file -replace '\\', '/') -match 'src/simulation/Simulation\.cpp$'
    } | Select-Object -First 1
    if (-not $simulation -or -not $simulation.command) {
        throw "Cannot find Simulation.cpp compile command"
    }
    $compilers = Get-Content -LiteralPath $compilersPath -Raw | ConvertFrom-Json
    return [pscustomobject]@{
        FpMode = [string]$options["fp_mode"]
        Options = $options
        Compiler = $compilers.host.cpp
        SimulationCompileCommand = [string]$simulation.command
    }
}

function Write-ProbeFiles {
    param(
        [Parameter(Mandatory = $true)][string] $Root,
        [Parameter(Mandatory = $true)][ValidateSet("trace", "capture")][string] $Phase,
        [Parameter(Mandatory = $true)][int] $CaptureStep
    )
    New-Item -ItemType Directory -Path $Root | Out-Null
    $utf8NoBom = [System.Text.UTF8Encoding]::new($false)
    [System.IO.File]::WriteAllText(
        (Join-Path $Root "powder.pref"),
        '{"LuaHookTimeout":900000}' + [Environment]::NewLine,
        $utf8NoBom
    )
    Copy-Item -LiteralPath $luaSource -Destination (Join-Path $Root "autorun.lua")
    $configPath = Join-Path $Root "differential.config"
    [System.IO.File]::WriteAllLines(
        $configPath,
        @(
            "schema_version=1",
            "phase=$Phase",
            "scenario=$Scenario",
            "trace_steps=$TraceSteps",
            "capture_step=$CaptureStep",
            "seed_a=$SeedA",
            "seed_b=$SeedB",
            "seed_c=$SeedC",
            "seed_d=$SeedD"
        ),
        [System.Text.Encoding]::ASCII
    )
    return $configPath
}

function Invoke-Probe {
    param(
        [Parameter(Mandatory = $true)][string] $Executable,
        [Parameter(Mandatory = $true)][string] $Root,
        [Parameter(Mandatory = $true)][string] $Label,
        [Parameter(Mandatory = $true)][ValidateSet("trace", "capture")][string] $Phase,
        [Parameter(Mandatory = $true)][int] $CaptureStep
    )
    $configPath = Write-ProbeFiles -Root $Root -Phase $Phase -CaptureStep $CaptureStep
    $resultPath = Join-Path $Root ("differential-" + $Phase + ".result")
    $startInfo = [System.Diagnostics.ProcessStartInfo]::new()
    $startInfo.FileName = $Executable
    $startInfo.WorkingDirectory = $Root
    $startInfo.UseShellExecute = $false
    $startInfo.CreateNoWindow = $true
    $startInfo.RedirectStandardOutput = $true
    $startInfo.RedirectStandardError = $true
    $startInfo.ArgumentList.Add("ddir")
    $startInfo.ArgumentList.Add($Root)
    $startInfo.Environment["PATH"] = $resolvedRuntime +
        [System.IO.Path]::PathSeparator + $env:PATH
    foreach ($secretName in @("GITHUB_PAT_TOKEN", "GITHUB_TOKEN", "GH_TOKEN")) {
        [void]$startInfo.Environment.Remove($secretName)
    }

    $started = [DateTime]::UtcNow
    $process = [System.Diagnostics.Process]::Start($startInfo)
    if (-not $process) {
        throw "Failed to start differential probe: label=$Label; phase=$Phase"
    }
    $stdoutTask = $process.StandardOutput.ReadToEndAsync()
    $stderrTask = $process.StandardError.ReadToEndAsync()
    $peakWorkingSet = [int64]0
    $peakPrivateBytes = [int64]0
    try {
        $deadline = $started.AddSeconds($TimeoutSeconds)
        do {
            $process.Refresh()
            if (-not $process.HasExited) {
                $peakWorkingSet = [Math]::Max($peakWorkingSet, [int64]$process.WorkingSet64)
                $peakPrivateBytes = [Math]::Max(
                    $peakPrivateBytes, [int64]$process.PrivateMemorySize64
                )
                Start-Sleep -Milliseconds 50
            }
        } while (-not $process.HasExited -and [DateTime]::UtcNow -lt $deadline)
        if (-not $process.HasExited) {
            Stop-Process -Id $process.Id -Force
            $process.WaitForExit()
            throw "Differential probe timed out: label=$Label; phase=$Phase; root=$Root"
        }
        $process.WaitForExit()
        $stdout = $stdoutTask.GetAwaiter().GetResult()
        $stderr = $stderrTask.GetAwaiter().GetResult()
        $stdoutPath = Join-Path $Root "client-stdout.log"
        $stderrPath = Join-Path $Root "client-stderr.log"
        [System.IO.File]::WriteAllText(
            $stdoutPath, $stdout, [System.Text.UTF8Encoding]::new($false)
        )
        [System.IO.File]::WriteAllText(
            $stderrPath, $stderr, [System.Text.UTF8Encoding]::new($false)
        )
        if (-not (Test-Path -LiteralPath $resultPath -PathType Leaf)) {
            throw "Differential probe produced no result: label=$Label; phase=$Phase; exit=$($process.ExitCode); root=$Root"
        }
        $values = Read-KeyValueFile -Path $resultPath
        if ($process.ExitCode -ne 0 -or
            $values.OMNI_DIFFERENTIAL_PROBE_STATUS -ne "PASS") {
            throw "Differential probe failed: label=$Label; phase=$Phase; exit=$($process.ExitCode); error=$($values.error); root=$Root"
        }
        if ($values.phase -ne $Phase -or $values.scenario -ne $Scenario) {
            throw "Differential probe identity mismatch: label=$Label; phase=$Phase"
        }
        return [pscustomobject]@{
            Label = $Label
            Phase = $Phase
            Root = $Root
            Values = $values
            ResultPath = $resultPath
            ConfigPath = $configPath
            StdoutPath = $stdoutPath
            StderrPath = $stderrPath
            WallSeconds = [Math]::Round(([DateTime]::UtcNow - $started).TotalSeconds, 6)
            CpuSeconds = [Math]::Round($process.TotalProcessorTime.TotalSeconds, 6)
            PeakWorkingSetBytes = $peakWorkingSet
            PeakPrivateBytes = $peakPrivateBytes
        }
    }
    finally {
        if (-not $process.HasExited) {
            Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue
            $process.WaitForExit()
        }
        $process.Dispose()
    }
}

function Copy-ProbeRecord {
    param(
        [Parameter(Mandatory = $true)] $Record,
        [Parameter(Mandatory = $true)][string] $Destination
    )
    $prefix = $Record.Label + "-" + $Record.Phase
    Copy-Item -LiteralPath $Record.ResultPath -Destination (
        Join-Path $Destination ($prefix + ".result")
    )
    Copy-Item -LiteralPath $Record.ConfigPath -Destination (
        Join-Path $Destination ($prefix + ".config")
    )
    Copy-Item -LiteralPath $Record.StdoutPath -Destination (
        Join-Path $Destination ($prefix + "-stdout.log")
    )
    Copy-Item -LiteralPath $Record.StderrPath -Destination (
        Join-Path $Destination ($prefix + "-stderr.log")
    )
    if ($Record.Phase -eq "trace") {
        Copy-Item -LiteralPath (Join-Path $Record.Root $Record.Values.trace_file) `
            -Destination (Join-Path $Destination ($prefix + ".csv"))
    }
    else {
        Copy-Item -LiteralPath (Join-Path $Record.Root $Record.Values.particle_file) `
            -Destination (Join-Path $Destination ($prefix + "-particles.csv"))
        Copy-Item -LiteralPath (Join-Path $Record.Root $Record.Values.cell_file) `
            -Destination (Join-Path $Destination ($prefix + "-cells.csv"))
    }
}

$gitState = Get-GitState -Repository $sourceRoot
$leftBuild = Get-BuildProvenance -Directory $resolvedLeftBuild
$rightBuild = Get-BuildProvenance -Directory $resolvedRightBuild
if ($ExpectedLeftFpMode -and $leftBuild.FpMode -ne $ExpectedLeftFpMode) {
    throw "Left fp_mode mismatch: expected=$ExpectedLeftFpMode; actual=$($leftBuild.FpMode)"
}
if ($ExpectedRightFpMode -and $rightBuild.FpMode -ne $ExpectedRightFpMode) {
    throw "Right fp_mode mismatch: expected=$ExpectedRightFpMode; actual=$($rightBuild.FpMode)"
}
$leftExeInfo = Get-Item -LiteralPath $resolvedLeftExecutable
$rightExeInfo = Get-Item -LiteralPath $resolvedRightExecutable
$leftExeHash = (Get-FileHash -LiteralPath $resolvedLeftExecutable -Algorithm SHA256).Hash
$rightExeHash = (Get-FileHash -LiteralPath $resolvedRightExecutable -Algorithm SHA256).Hash
if ($leftExeHash -eq $rightExeHash) {
    throw "Differential executables have identical SHA-256"
}

$tempParent = [System.IO.Path]::GetFullPath($TemporaryDirectory)
if (-not (Test-Path -LiteralPath $tempParent -PathType Container)) {
    throw "Temporary directory does not exist: $tempParent"
}
$runId = [DateTime]::UtcNow.ToString("yyyyMMddTHHmmssZ") + "-" +
    [guid]::NewGuid().ToString("N").Substring(0, 8)
$testRoot = [System.IO.Path]::GetFullPath(
    (Join-Path $tempParent ("tpt-omnipack-differential-" + $runId))
)
$tempPrefix = $tempParent.TrimEnd(
    [System.IO.Path]::DirectorySeparatorChar,
    [System.IO.Path]::AltDirectorySeparatorChar
) + [System.IO.Path]::DirectorySeparatorChar
if (-not $testRoot.StartsWith($tempPrefix, [System.StringComparison]::OrdinalIgnoreCase)) {
    throw "Refusing to create differential data outside the selected temporary root"
}

$artifactBase = if ([System.IO.Path]::IsPathRooted($OutputDirectory)) {
    [System.IO.Path]::GetFullPath($OutputDirectory)
}
else {
    [System.IO.Path]::GetFullPath((Join-Path $sourceRoot $OutputDirectory))
}
$cpu = Get-CimInstance Win32_Processor -ErrorAction SilentlyContinue | Select-Object -First 1
$computer = Get-CimInstance Win32_ComputerSystem -ErrorAction Stop
$os = Get-CimInstance Win32_OperatingSystem -ErrorAction Stop
$cpuName = if ($cpu) { [string]$cpu.Name.Trim() } else { "not_tested" }
$machineId = "windows-" + (Get-TextSha256 -Text (
    "$($env:COMPUTERNAME)|$cpuName|$($computer.TotalPhysicalMemory)"
)).Substring(0, 12)
$artifactRoot = Join-Path $artifactBase (
    Join-Path $machineId (Join-Path $gitState.Commit.Substring(0, 10) $runId)
)

$completed = $false
try {
    New-Item -ItemType Directory -Path $testRoot | Out-Null
    $leftTrace = Invoke-Probe -Executable $resolvedLeftExecutable `
        -Root (Join-Path $testRoot ($LeftLabel + "-trace")) `
        -Label $LeftLabel -Phase "trace" -CaptureStep 0
    $rightTrace = Invoke-Probe -Executable $resolvedRightExecutable `
        -Root (Join-Path $testRoot ($RightLabel + "-trace")) `
        -Label $RightLabel -Phase "trace" -CaptureStep 0

    $leftTracePath = (Resolve-Path -LiteralPath (
        Join-Path $leftTrace.Root $leftTrace.Values.trace_file
    )).Path
    $rightTracePath = (Resolve-Path -LiteralPath (
        Join-Path $rightTrace.Root $rightTrace.Values.trace_file
    )).Path
    $traceComparisonPath = Join-Path $testRoot "trace-comparison.json"
    & $resolvedPython $comparatorSource trace --left $leftTracePath `
        --right $rightTracePath --output $traceComparisonPath
    if ($LASTEXITCODE -ne 0 -or
        -not (Test-Path -LiteralPath $traceComparisonPath -PathType Leaf)) {
        throw "Differential trace comparison failed"
    }
    $traceComparison = Get-Content -LiteralPath $traceComparisonPath -Raw | ConvertFrom-Json
    if ($traceComparison.status -ne "PASS" -or
        $traceComparison.compared_through_step -ne $TraceSteps) {
        throw "Differential trace comparison contract failed"
    }

    $records = [System.Collections.Generic.List[object]]::new()
    $records.Add($leftTrace)
    $records.Add($rightTrace)
    $fieldComparison = $null
    $fieldComparisonPath = $null
    $firstDivergenceStep = $null
    if ($traceComparison.divergence_found) {
        $firstDivergenceStep = [int]$traceComparison.first_divergence.step
        $leftCapture = Invoke-Probe -Executable $resolvedLeftExecutable `
            -Root (Join-Path $testRoot ($LeftLabel + "-capture")) `
            -Label $LeftLabel -Phase "capture" -CaptureStep $firstDivergenceStep
        $rightCapture = Invoke-Probe -Executable $resolvedRightExecutable `
            -Root (Join-Path $testRoot ($RightLabel + "-capture")) `
            -Label $RightLabel -Phase "capture" -CaptureStep $firstDivergenceStep
        $records.Add($leftCapture)
        $records.Add($rightCapture)

        $leftTraceRow = Import-Csv -LiteralPath $leftTracePath | Where-Object {
            [int]$_.step -eq $firstDivergenceStep
        } | Select-Object -First 1
        $rightTraceRow = Import-Csv -LiteralPath $rightTracePath | Where-Object {
            [int]$_.step -eq $firstDivergenceStep
        } | Select-Object -First 1
        if (-not $leftTraceRow -or -not $rightTraceRow) {
            throw "Cannot locate first-divergence trace rows"
        }
        foreach ($pair in @(
            [pscustomobject]@{ Record = $leftCapture; Row = $leftTraceRow; Label = $LeftLabel },
            [pscustomobject]@{ Record = $rightCapture; Row = $rightTraceRow; Label = $RightLabel }
        )) {
            $capture = $pair.Record.Values
            $traceRng = @(
                $pair.Row.rng_a, $pair.Row.rng_b, $pair.Row.rng_c, $pair.Row.rng_d
            ) -join ','
            if ($capture.state_hash -ne $pair.Row.state_hash_fnv1a32 -or
                $capture.particles -ne $pair.Row.particles -or
                $capture.rng -ne $traceRng) {
                throw "Differential capture did not reproduce trace: label=$($pair.Label)"
            }
        }

        $fieldComparisonPath = Join-Path $testRoot "field-comparison.json"
        & $resolvedPython $comparatorSource capture `
            --left-particles (Join-Path $leftCapture.Root $leftCapture.Values.particle_file) `
            --right-particles (Join-Path $rightCapture.Root $rightCapture.Values.particle_file) `
            --left-cells (Join-Path $leftCapture.Root $leftCapture.Values.cell_file) `
            --right-cells (Join-Path $rightCapture.Root $rightCapture.Values.cell_file) `
            --step $firstDivergenceStep `
            --left-hash ([uint64]$leftTraceRow.state_hash_fnv1a32) `
            --right-hash ([uint64]$rightTraceRow.state_hash_fnv1a32) `
            --cell-size 4 --neighbor-radius 2 --sample-limit 64 `
            --output $fieldComparisonPath
        if ($LASTEXITCODE -ne 0 -or
            -not (Test-Path -LiteralPath $fieldComparisonPath -PathType Leaf)) {
            throw "Differential field comparison failed"
        }
        $fieldComparison = Get-Content -LiteralPath $fieldComparisonPath -Raw | ConvertFrom-Json
        if ($fieldComparison.status -ne "PASS" -or
            -not $fieldComparison.hash_differs -or
            $fieldComparison.unexplained_hash_divergence) {
            throw "Differential field comparison did not explain the hash divergence"
        }
    }

    New-Item -ItemType Directory -Path $artifactRoot -Force | Out-Null
    foreach ($record in $records) {
        Copy-ProbeRecord -Record $record -Destination $artifactRoot
    }
    Copy-Item -LiteralPath $traceComparisonPath -Destination (
        Join-Path $artifactRoot "trace-comparison.json"
    )
    if ($fieldComparisonPath) {
        Copy-Item -LiteralPath $fieldComparisonPath -Destination (
            Join-Path $artifactRoot "field-comparison.json"
        )
    }

    $artifactFiles = @(
        Get-ChildItem -LiteralPath $artifactRoot -File | Sort-Object Name | ForEach-Object {
            [ordered]@{
                name = $_.Name
                length_bytes = [int64]$_.Length
                sha256 = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash
            }
        }
    )
    $metrics = @($records | ForEach-Object {
        [ordered]@{
            label = $_.Label
            phase = $_.Phase
            wall_seconds = $_.WallSeconds
            cpu_seconds = $_.CpuSeconds
            peak_working_set_bytes = $_.PeakWorkingSetBytes
            peak_private_bytes = $_.PeakPrivateBytes
        }
    })
    $manifest = [ordered]@{
        schema_version = 1
        status = "PASS"
        run_id = $runId
        recorded_at_utc = [DateTime]::UtcNow.ToString("o")
        scenario = $Scenario
        comparison_kind = "same_source_cpu_fp_mode"
        performance_gate = "not_evaluated"
        seed = [uint64[]]@($SeedA, $SeedB, $SeedC, $SeedD)
        trace_steps = $TraceSteps
        trace_rows = [int]$traceComparison.trace_rows
        divergence_found = [bool]$traceComparison.divergence_found
        first_divergence_step = $firstDivergenceStep
        trace_comparison = $traceComparison
        field_comparison = if ($fieldComparison) {
            [ordered]@{
                status = [string]$fieldComparison.status
                unexplained_hash_divergence = [bool]$fieldComparison.unexplained_hash_divergence
                difference_field = $fieldComparison.difference_field
                differing_particle_ids = [int64]$fieldComparison.differing_particle_ids
                differing_atmosphere_cells = [int64]$fieldComparison.differing_atmosphere_cells
                first_particle_difference = $fieldComparison.first_particle_difference
                first_atmosphere_difference = $fieldComparison.first_atmosphere_difference
                neighbor_state = $fieldComparison.neighbor_state
                particle_atmosphere_cells = $fieldComparison.particle_atmosphere_cells
            }
        }
        else { $null }
        source = [ordered]@{
            commit = $gitState.Commit
            dirty = [bool]$gitState.Dirty
            worktree_state_sha256 = $gitState.StateSha256
            status_lines = [string[]]$gitState.StatusLines
            lua_sha256 = (Get-FileHash -LiteralPath $luaSource -Algorithm SHA256).Hash
            comparator_sha256 = (Get-FileHash -LiteralPath $comparatorSource -Algorithm SHA256).Hash
            wrapper_sha256 = (Get-FileHash -LiteralPath $PSCommandPath -Algorithm SHA256).Hash
        }
        left = [ordered]@{
            label = $LeftLabel
            executable_sha256 = $leftExeHash
            executable_length_bytes = [int64]$leftExeInfo.Length
            build_directory = $resolvedLeftBuild
            fp_mode = $leftBuild.FpMode
            compiler = $leftBuild.Compiler
            meson_options = $leftBuild.Options
            simulation_compile_command = $leftBuild.SimulationCompileCommand
            backend = "CPU"
        }
        right = [ordered]@{
            label = $RightLabel
            executable_sha256 = $rightExeHash
            executable_length_bytes = [int64]$rightExeInfo.Length
            build_directory = $resolvedRightBuild
            fp_mode = $rightBuild.FpMode
            compiler = $rightBuild.Compiler
            meson_options = $rightBuild.Options
            simulation_compile_command = $rightBuild.SimulationCompileCommand
            backend = "CPU"
        }
        exported_domains = @(
            "all active Particle fields and IDs",
            "pv/vx/vy/hv",
            "wall/emap/fan",
            "gravity input/output"
        )
        unexported_snapshot_domains = @(
            "blockAir/blockAirH",
            "portal particles",
            "wireless channels",
            "stickmen/fighters"
        )
        process_metrics = $metrics
        runtime_directory = $resolvedRuntime
        portable_runtime_tested = $false
        machine = [ordered]@{
            machine_id = $machineId
            os = "$($os.Caption) $($os.Version) build $($os.BuildNumber)"
            cpu_model = $cpuName
            logical_cpu_count = [int][Environment]::ProcessorCount
            physical_memory_bytes = [int64]$computer.TotalPhysicalMemory
            process_vram = "not_tested"
        }
        artifact_policy = "private_local_not_for_public_release"
        artifact_files = $artifactFiles
    }
    $manifestPath = Join-Path $artifactRoot "manifest.json"
    [System.IO.File]::WriteAllText(
        $manifestPath,
        ($manifest | ConvertTo-Json -Depth 20) + [Environment]::NewLine,
        [System.Text.UTF8Encoding]::new($false)
    )

    $completed = $true
    Write-Output "runtime-first-divergence: PASS"
    Write-Output "scenario=$Scenario"
    Write-Output "divergence_found=$($traceComparison.divergence_found.ToString().ToLowerInvariant())"
    Write-Output "first_divergence_step=$firstDivergenceStep"
    if ($fieldComparison) {
        Write-Output "primary_domain=$($fieldComparison.difference_field.primary_domain)"
        Write-Output "first_particle_id=$($fieldComparison.difference_field.particle_id)"
        Write-Output "differing_particle_ids=$($fieldComparison.differing_particle_ids)"
        Write-Output "differing_atmosphere_cells=$($fieldComparison.differing_atmosphere_cells)"
    }
    Write-Output "source_commit=$($gitState.Commit)"
    Write-Output "source_dirty=$($gitState.Dirty.ToString().ToLowerInvariant())"
    Write-Output "manifest_sha256=$((Get-FileHash -LiteralPath $manifestPath -Algorithm SHA256).Hash)"
    Write-Output "manifest_json=$manifestPath"
}
finally {
    if ($completed -and -not $KeepTemporary -and (Test-Path -LiteralPath $testRoot)) {
        Remove-IsolatedRoot -Path $testRoot -Parent $tempParent
    }
}
