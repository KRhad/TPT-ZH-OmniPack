param(
    [string] $Executable,

    [switch] $RunRusanovUniform,

    [Parameter(Mandatory = $true)]
    [string] $BuildDirectory,

    [string] $MesonExecutable = "meson",

    [string] $GitExecutable = "git",

    [string] $OutputDirectory = "artifacts/vnext-atmospherebench"
)

$ErrorActionPreference = "Stop"
$ProgressPreference = "SilentlyContinue"

$sourceRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot "..")).Path
$resolvedBuildDirectory = (Resolve-Path -LiteralPath $BuildDirectory).Path

function Get-Sha256 {
    param([Parameter(Mandatory = $true)][string] $Path)
    return (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash
}

function Get-TextSha256 {
    param([Parameter(Mandatory = $true)][string] $Text)
    $sha256 = New-Object System.Security.Cryptography.SHA256Managed
    try {
        $bytes = [System.Text.Encoding]::UTF8.GetBytes($Text)
        return (-join ($sha256.ComputeHash($bytes) | ForEach-Object { $_.ToString("X2") }))
    } finally {
        $sha256.Dispose()
    }
}

function Get-SourceState {
    param(
        [Parameter(Mandatory = $true)][string] $Repository,
        [Parameter(Mandatory = $true)][string] $GitCommand
    )
    $topLevel = (& $GitCommand -C $Repository rev-parse --show-toplevel).Trim()
    if ($LASTEXITCODE -ne 0 -or -not $topLevel) {
        throw "Cannot resolve AtmosphereBench source repository"
    }
    $topLevel = [System.IO.Path]::GetFullPath($topLevel)
    $head = (& $GitCommand -C $Repository rev-parse HEAD).Trim()
    if ($LASTEXITCODE -ne 0 -or $head -notmatch '^[0-9a-f]{40}$') {
        throw "Cannot resolve AtmosphereBench source HEAD"
    }
    $status = @(& $GitCommand -C $Repository status --porcelain=v1 --untracked-files=all)
    if ($LASTEXITCODE -ne 0) {
        throw "Cannot inspect AtmosphereBench source worktree"
    }
    if ($status.Count -ne 0) {
        throw "AtmosphereBench requires a clean source worktree; commit changes before recording evidence"
    }
    return [pscustomobject]@{
        Repository = $topLevel
        Commit = $head
        Dirty = $false
        StateSha256 = Get-TextSha256 -Text "HEAD=$head`n"
    }
}

function Read-KeyValue {
    param(
        [Parameter(Mandatory = $true)][string] $Text,
        [Parameter(Mandatory = $true)][string] $Key
    )
    $match = [regex]::Match($Text, "(?m)^" + [regex]::Escape($Key) + "=([^`r`n]+)`r?$")
    if (-not $match.Success) {
        throw "AtmosphereBench output is missing $Key"
    }
    return $match.Groups[1].Value
}

$gitCommand = (Get-Command -Name $GitExecutable -CommandType Application -ErrorAction Stop).Source
$sourceState = Get-SourceState -Repository $sourceRoot -GitCommand $gitCommand
$buildSystemFilesPath = Join-Path $resolvedBuildDirectory "meson-info/intro-buildsystem_files.json"
if (-not (Test-Path -LiteralPath $buildSystemFilesPath -PathType Leaf)) {
    throw "AtmosphereBench build-system provenance is missing: $buildSystemFilesPath"
}
$buildSystemFiles = Get-Content -LiteralPath $buildSystemFilesPath -Raw | ConvertFrom-Json
$expectedRootMeson = [System.IO.Path]::GetFullPath((Join-Path $sourceState.Repository "meson.build"))
$configuredForSource = $false
foreach ($buildSystemFile in $buildSystemFiles) {
    $candidate = [System.IO.Path]::GetFullPath([string]$buildSystemFile)
    if ([string]::Equals($candidate, $expectedRootMeson, [System.StringComparison]::OrdinalIgnoreCase)) {
        $configuredForSource = $true
        break
    }
}
if (-not $configuredForSource) {
    throw "AtmosphereBench build directory is configured from a different source root"
}

$mesonCommand = (Get-Command -Name $MesonExecutable -CommandType Application -ErrorAction Stop).Source
$compileOutput = @(& $mesonCommand "compile" "-C" $resolvedBuildDirectory "atmospherebench" 2>&1)
$compileExitCode = $LASTEXITCODE
if ($compileExitCode -ne 0) {
    throw "AtmosphereBench rebuild failed: exit_code=$compileExitCode output=$($compileOutput -join [Environment]::NewLine)"
}
$targetsOutput = @(& $mesonCommand "introspect" "--targets" $resolvedBuildDirectory 2>&1)
if ($LASTEXITCODE -ne 0) {
    throw "Cannot inspect AtmosphereBench Meson target: $($targetsOutput -join [Environment]::NewLine)"
}
try {
    $targetsText = $targetsOutput -join [Environment]::NewLine
    $targets = $targetsText | ConvertFrom-Json
} catch {
    throw "AtmosphereBench Meson target metadata is invalid JSON: $($_.Exception.Message)"
}
$benchTargets = @($targets | Where-Object {
    $_.name -eq "atmospherebench" -and $_.id -eq "atmospherebench@exe" -and $_.type -eq "executable"
})
if ($benchTargets.Count -ne 1) {
    throw "AtmosphereBench Meson target must resolve uniquely"
}
$benchTargetFiles = @($benchTargets[0].filename)
if ($benchTargetFiles.Count -ne 1 -or -not $benchTargetFiles[0]) {
    throw "AtmosphereBench Meson target must declare exactly one executable output"
}
$resolvedExecutable = (Resolve-Path -LiteralPath ([string]$benchTargetFiles[0])).Path
if ($Executable) {
    $requestedExecutable = (Resolve-Path -LiteralPath $Executable).Path
    if (-not [string]::Equals($requestedExecutable, $resolvedExecutable, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Requested executable does not match the freshly built AtmosphereBench Meson target"
    }
}

$compileCommandsPath = Join-Path $resolvedBuildDirectory "compile_commands.json"
if (-not (Test-Path -LiteralPath $compileCommandsPath -PathType Leaf)) {
    throw "AtmosphereBench build provenance file is missing: $compileCommandsPath"
}
$compileCommands = Get-Content -LiteralPath $compileCommandsPath -Raw | ConvertFrom-Json
$benchCommands = @($compileCommands | Where-Object {
    ($_.file -replace '\\', '/') -match 'tools/atmospherebench/'
})
if ($benchCommands.Count -lt 2) {
    throw "AtmosphereBench compile commands are missing"
}
$expectedBenchSources = @(
    [System.IO.Path]::GetFullPath((Join-Path $sourceState.Repository "tools/atmospherebench/AtmosphereBench.cpp")),
    [System.IO.Path]::GetFullPath((Join-Path $sourceState.Repository "tools/atmospherebench/Rusanov1D.cpp")),
    [System.IO.Path]::GetFullPath((Join-Path $sourceState.Repository "tools/atmospherebench/main.cpp"))
)
$actualBenchSources = @($benchCommands | ForEach-Object {
    $file = [string]$_.file
    if ([System.IO.Path]::IsPathRooted($file)) {
        [System.IO.Path]::GetFullPath($file)
    } else {
        [System.IO.Path]::GetFullPath((Join-Path ([string]$_.directory) $file))
    }
})
foreach ($expectedSource in $expectedBenchSources) {
    $present = $false
    foreach ($actualSource in $actualBenchSources) {
        if ([string]::Equals($actualSource, $expectedSource, [System.StringComparison]::OrdinalIgnoreCase)) {
            $present = $true
            break
        }
    }
    if (-not $present) {
        throw "AtmosphereBench compile commands are not bound to current source: $expectedSource"
    }
}
$gnuStrict = $true
$msvcStrict = $true
foreach ($command in $benchCommands) {
    foreach ($requiredFlag in @("-fno-fast-math", "-fno-unsafe-math-optimizations", "-ffp-contract=off")) {
        if ($command.command -notmatch [regex]::Escape('"' + $requiredFlag + '"')) {
            $gnuStrict = $false
        }
    }
    if ($command.command -notmatch '"/fp:strict"') {
        $msvcStrict = $false
    }
    if ($command.command -match '"-ffast-math"|"-funsafe-math-optimizations"|"/fp:fast"') {
        throw "AtmosphereBench inherited a Legacy fast-math flag"
    }
}
if (-not $gnuStrict -and -not $msvcStrict) {
    throw "AtmosphereBench strict FP flags are missing for both GNU-like and MSVC compilers"
}
$strictReferenceMode = if ($gnuStrict) { "gnu_strict" } else { "msvc_strict" }

$timer = [System.Diagnostics.Stopwatch]::StartNew()
$runMode = if ($RunRusanovUniform) { "rusanov_uniform" } else { "contract_uniform" }
$runArgument = if ($RunRusanovUniform) { "--run-rusanov-uniform" } else { "--run-uniform" }
$output = @(& $resolvedExecutable $runArgument 2>&1)
$exitCode = $LASTEXITCODE
$timer.Stop()
if ($exitCode -ne 0) {
    throw "AtmosphereBench $runMode run failed: exit_code=$exitCode output=$($output -join [Environment]::NewLine)"
}
$text = $output -join [Environment]::NewLine
$candidateOutput = @(& $resolvedExecutable "--list-candidates" 2>&1)
if ($LASTEXITCODE -ne 0) {
    throw "AtmosphereBench candidate-list run failed: output=$($candidateOutput -join [Environment]::NewLine)"
}
$candidateText = $candidateOutput -join [Environment]::NewLine
if ($candidateText -notmatch '(?m)^selection_status=unselected$') {
    throw "AtmosphereBench candidate list must remain unselected"
}
foreach ($candidateLine in @(
    "candidate=fvm_rusanov|status=implemented_1d_periodic_uniform_probe|solver_implemented=true",
    "candidate=fvm_hlle|status=registered_only|solver_implemented=false",
    "candidate=lbm_d2q9|status=registered_only|solver_implemented=false"
)) {
    if ($candidateText -notmatch [regex]::Escape($candidateLine)) {
        throw "AtmosphereBench candidate registration drifted: $candidateLine"
    }
}
if ((Read-KeyValue -Text $text -Key "physical_scale_selection") -ne "unselected") {
    throw "AtmosphereBench selected PhysicalScale unexpectedly"
}
if ((Read-KeyValue -Text $text -Key "atmosphere_solver_selection") -ne "unselected") {
    throw "AtmosphereBench selected a solver unexpectedly"
}
$candidateImplementations = "none"
$benchmarkKind = "atmospherebench_contract_uniform"
$performanceGate = "not_evaluated_contract_only"
$timingScope = "standalone_contract_uniform_no_solver_step"
$solverResultStatus = Read-KeyValue -Text $text -Key "result_status"
if (-not $RunRusanovUniform) {
    if ($solverResultStatus -ne "contract_only") {
        throw "AtmosphereBench scaffold must report result_status=contract_only"
    }
    if ((Read-KeyValue -Text $text -Key "case_time_domain") -ne "nondimensional_contract") {
        throw "AtmosphereBench scaffold must keep the shared case nondimensional"
    }
    if ((Read-KeyValue -Text $text -Key "case_timestep") -ne "1") {
        throw "AtmosphereBench scaffold contract timestep drifted"
    }
    if ((Read-KeyValue -Text $text -Key "case_step_count") -ne "0") {
        throw "AtmosphereBench scaffold must not claim a solver step"
    }
    foreach ($gridKey in @{ "grid_cells_x" = "4"; "grid_cells_y" = "3"; "grid_cell_count" = "12" }.GetEnumerator()) {
        if ((Read-KeyValue -Text $text -Key $gridKey.Key) -ne $gridKey.Value) {
            throw "AtmosphereBench shared grid contract drifted: $($gridKey.Key)"
        }
    }
    if ((Read-KeyValue -Text $text -Key "eos_fixture") -ne "synthetic_nondimensional") {
        throw "AtmosphereBench uniform contract used an unreviewed EOS fixture"
    }
    foreach ($ledgerKey in @(
        "mass_drift", "momentum_drift", "momentum_x_drift", "momentum_y_drift", "energy_drift",
        "numerical_correction_count", "correction_mass_added", "correction_mass_removed",
        "correction_momentum_x_added", "correction_momentum_y_added", "correction_energy_added",
        "correction_energy_removed", "density_floor_hits", "pressure_floor_hits", "correction_event_count"
    )) {
        if ((Read-KeyValue -Text $text -Key $ledgerKey) -ne "0") {
            throw "Uniform contract expected $ledgerKey=0"
        }
    }
} else {
    $benchmarkKind = "atmospherebench_rusanov_uniform_probe"
    $performanceGate = "not_evaluated_candidate_probe"
    $timingScope = "standalone_rusanov_uniform_probe"
    $candidateImplementations = "fvm_rusanov"
    if ($solverResultStatus -ne "candidate_result_not_selection") {
        throw "Rusanov probe must not claim solver selection"
    }
    if ((Read-KeyValue -Text $text -Key "candidate") -ne "fvm_rusanov" -or
        (Read-KeyValue -Text $text -Key "candidate_solver_implemented") -ne "true") {
        throw "Rusanov probe candidate identity is invalid"
    }
    foreach ($probeKey in @{
        "case_time_domain" = "nondimensional_contract"; "grid_cells_x" = "64";
        "grid_cells_y" = "1"; "grid_cell_count" = "64"; "case_timestep" = "0.05";
        "case_step_count" = "16"; "positivity_preserved" = "true";
        "numerical_correction_count" = "0"; "probe_passed" = "true"
    }.GetEnumerator()) {
        if ((Read-KeyValue -Text $text -Key $probeKey.Key) -ne $probeKey.Value) {
            throw "Rusanov probe contract drifted: $($probeKey.Key)"
        }
    }
    $maximumCfl = [double](Read-KeyValue -Text $text -Key "maximum_cfl")
    if (-not [double]::IsFinite($maximumCfl) -or $maximumCfl -le 0.0 -or $maximumCfl -gt 1.0) {
        throw "Rusanov probe CFL is outside the strict positivity contract"
    }
    foreach ($driftKey in @("mass_drift", "momentum_x_drift", "momentum_y_drift", "energy_drift")) {
        if ([Math]::Abs([double](Read-KeyValue -Text $text -Key $driftKey)) -gt 1e-12) {
            throw "Rusanov probe drift exceeds the periodic conservation tolerance: $driftKey"
        }
    }
}
$finalSourceState = Get-SourceState -Repository $sourceRoot -GitCommand $gitCommand
if ($finalSourceState.Commit -ne $sourceState.Commit -or $finalSourceState.StateSha256 -ne $sourceState.StateSha256) {
    throw "AtmosphereBench source changed during build or measurement"
}

$outputBase = if ([System.IO.Path]::IsPathRooted($OutputDirectory)) {
    [System.IO.Path]::GetFullPath($OutputDirectory)
} else {
    [System.IO.Path]::GetFullPath((Join-Path $sourceRoot $OutputDirectory))
}
$runId = [DateTime]::UtcNow.ToString("yyyyMMddTHHmmssZ") + "-" + [guid]::NewGuid().ToString("N").Substring(0, 8)
$resultDirectory = Join-Path $outputBase $runId
if (Test-Path -LiteralPath $resultDirectory) {
    throw "Refusing to reuse AtmosphereBench output directory: $resultDirectory"
}
New-Item -ItemType Directory -Path $resultDirectory | Out-Null

$stateDensity = $null
$statePressure = $null
$stateAndFluxScratchBytesPerCell = $null
if ($RunRusanovUniform) {
    $stateDensity = [double](Read-KeyValue -Text $text -Key "minimum_density")
    $statePressure = [double](Read-KeyValue -Text $text -Key "minimum_pressure")
    $stateAndFluxScratchBytesPerCell = [int](Read-KeyValue -Text $text -Key "state_and_flux_scratch_bytes_per_cell")
} else {
    $stateDensity = [double](Read-KeyValue -Text $text -Key "state_density")
    $statePressure = [double](Read-KeyValue -Text $text -Key "state_pressure")
}
$limitations = @(
    "No production Air, Simulation, Particle, Save, or Lua code is linked.",
    "No HLLE, LBM, source-term, boundary, or multi-species candidate is implemented in this scaffold."
)
if ($RunRusanovUniform) {
    $limitations += "Rusanov is limited to a first-order strict-double 1D periodic uniform probe; this is not solver selection or production evidence."
} else {
    $limitations += "This result is a contract artifact, not solver-performance or physical-time evidence."
}

$result = [ordered]@{
    schema_version = 1
    benchmark_kind = $benchmarkKind
    benchmark_execution_status = "PASS"
    performance_gate = $performanceGate
    source = [ordered]@{
        commit = $sourceState.Commit
        dirty = $sourceState.Dirty
        state_sha256 = $sourceState.StateSha256
        git_executable = $gitCommand
        git_sha256 = Get-Sha256 -Path $gitCommand
        runner_sha256 = Get-Sha256 -Path $PSCommandPath
    }
    executable = [ordered]@{
        path = $resolvedExecutable
        sha256 = Get-Sha256 -Path $resolvedExecutable
        length_bytes = (Get-Item -LiteralPath $resolvedExecutable).Length
    }
    build = [ordered]@{
        directory = $resolvedBuildDirectory
        meson_executable = $mesonCommand
        meson_sha256 = Get-Sha256 -Path $mesonCommand
        meson_target_id = $benchTargets[0].id
        meson_target_output = $resolvedExecutable
        compile_commands_sha256 = Get-Sha256 -Path $compileCommandsPath
        rebuilt_before_measurement = $true
        source_root_verified = $true
        source_stable_through_measurement = $true
        strict_reference_flags_verified = $true
        strict_reference_mode = $strictReferenceMode
        legacy_fast_math_inherited = $false
    }
    physical_scale = [ordered]@{
        selection_status = "unselected"
        candidate_config = "resources/omnicore/v1/physical-scale-candidates.json"
    }
    solver = [ordered]@{
        selection_status = "unselected"
        result_status = $solverResultStatus
        candidate_implementations = $candidateImplementations
    }
    case = [ordered]@{
        id = Read-KeyValue -Text $text -Key "case"
        time_domain = Read-KeyValue -Text $text -Key "case_time_domain"
        timestep = [double](Read-KeyValue -Text $text -Key "case_timestep")
        step_count = [int](Read-KeyValue -Text $text -Key "case_step_count")
        grid_cells_x = [int](Read-KeyValue -Text $text -Key "grid_cells_x")
        grid_cells_y = [int](Read-KeyValue -Text $text -Key "grid_cells_y")
        grid_cell_count = [int](Read-KeyValue -Text $text -Key "grid_cell_count")
        state_bytes_per_cell = [int](Read-KeyValue -Text $text -Key "state_bytes_per_cell")
        state_and_flux_scratch_bytes_per_cell = $stateAndFluxScratchBytesPerCell
        density = $stateDensity
        pressure = $statePressure
        mass_drift = [double](Read-KeyValue -Text $text -Key "mass_drift")
        momentum_drift = [double](Read-KeyValue -Text $text -Key "momentum_drift")
        momentum_x_drift = [double](Read-KeyValue -Text $text -Key "momentum_x_drift")
        momentum_y_drift = [double](Read-KeyValue -Text $text -Key "momentum_y_drift")
        energy_drift = [double](Read-KeyValue -Text $text -Key "energy_drift")
        numerical_correction_count = [int](Read-KeyValue -Text $text -Key "numerical_correction_count")
    }
    numerical_correction_ledger = [ordered]@{
        mass_added = [double](Read-KeyValue -Text $text -Key "correction_mass_added")
        mass_removed = [double](Read-KeyValue -Text $text -Key "correction_mass_removed")
        momentum_x_added = [double](Read-KeyValue -Text $text -Key "correction_momentum_x_added")
        momentum_y_added = [double](Read-KeyValue -Text $text -Key "correction_momentum_y_added")
        energy_added = [double](Read-KeyValue -Text $text -Key "correction_energy_added")
        energy_removed = [double](Read-KeyValue -Text $text -Key "correction_energy_removed")
        density_floor_hits = [int](Read-KeyValue -Text $text -Key "density_floor_hits")
        pressure_floor_hits = [int](Read-KeyValue -Text $text -Key "pressure_floor_hits")
        event_count = [int](Read-KeyValue -Text $text -Key "correction_event_count")
    }
    measurement = [ordered]@{
        elapsed_milliseconds = [Math]::Round($timer.Elapsed.TotalMilliseconds, 6)
        timing_scope = $timingScope
    }
    limitations = $limitations
}
$utf8 = [System.Text.UTF8Encoding]::new($false)
[System.IO.File]::WriteAllText((Join-Path $resultDirectory "stdout.txt"), $text + [Environment]::NewLine, $utf8)
[System.IO.File]::WriteAllText((Join-Path $resultDirectory "result.json"), ($result | ConvertTo-Json -Depth 8) + [Environment]::NewLine, $utf8)

Write-Output "run-atmospherebench: PASS"
Write-Output "source_commit=$($sourceState.Commit)"
Write-Output "source_dirty=false"
Write-Output "physical_scale_selection=unselected"
Write-Output "atmosphere_solver_selection=unselected"
Write-Output "performance_gate=$performanceGate"
Write-Output "result_json=$(Join-Path $resultDirectory 'result.json')"
