param(
    [string] $Executable,

    [switch] $RunRusanovUniform,

    [switch] $RunRusanovPressurePulse,

    [switch] $RunRusanovDensityAdvection,

    [switch] $RunRusanovContactDiscontinuity,

    [switch] $RunRusanovNearVacuumExpansion,

    [switch] $RunRusanovSodShockTube,

    [switch] $RunRusanovDensityAdvectionRefinement,

    [switch] $RunRusanovLowMachAdvection,

    [switch] $RunAllSpeedRusanovLowMachAdvection,

    [switch] $RunHllcRusanovFallbackLowMachAdvection,

    [switch] $RunHllcRusanovFallbackNearVacuumExpansion,

    [switch] $RunHllcRusanovFallbackSodShockTube,

    [switch] $RunHllcRusanovFallbackOpenBoundaryLeak,

    [switch] $RunHllcRusanovFallbackPerformance,

    [switch] $RunHllc2DUniform,

    [switch] $RunHllc2DPressurePulse,

    [switch] $RunHllc2DSealedHeating,

    [switch] $RunHllc2DNaturalConvection,

    [switch] $RunHllc2DSpeciesMixing,

    [switch] $RunHllc2DPerformance,

    [switch] $RunLbmD2Q9Uniform,

    [switch] $RunLbmD2Q9ShearWave,

    [switch] $RunLegacyLikeUniform,

    [switch] $RunLegacyLikePressurePulse,

    [switch] $RunHybridAllSpeedPolicy,

    [switch] $RunRusanovOpenBoundaryLeak,

    [switch] $RunRusanovPerformance,

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
    [System.IO.Path]::GetFullPath((Join-Path $sourceState.Repository "tools/atmospherebench/Hllc2D.cpp")),
    [System.IO.Path]::GetFullPath((Join-Path $sourceState.Repository "tools/atmospherebench/HybridPolicy1D.cpp")),
    [System.IO.Path]::GetFullPath((Join-Path $sourceState.Repository "tools/atmospherebench/LbmD2Q9.cpp")),
    [System.IO.Path]::GetFullPath((Join-Path $sourceState.Repository "tools/atmospherebench/LegacyLike.cpp")),
    [System.IO.Path]::GetFullPath((Join-Path $sourceState.Repository "tools/atmospherebench/Rusanov1D.cpp")),
    [System.IO.Path]::GetFullPath((Join-Path $sourceState.Repository "tools/atmospherebench/Species2D.cpp")),
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

if ((@($RunRusanovUniform, $RunRusanovPressurePulse, $RunRusanovDensityAdvection,
		$RunRusanovContactDiscontinuity, $RunRusanovNearVacuumExpansion,
		$RunRusanovSodShockTube, $RunRusanovDensityAdvectionRefinement,
		$RunRusanovLowMachAdvection, $RunAllSpeedRusanovLowMachAdvection,
		$RunHllcRusanovFallbackLowMachAdvection,
		$RunHllcRusanovFallbackNearVacuumExpansion,
		$RunHllcRusanovFallbackSodShockTube,
		$RunHllcRusanovFallbackOpenBoundaryLeak,
		$RunHllcRusanovFallbackPerformance,
		$RunHllc2DUniform, $RunHllc2DPressurePulse, $RunHllc2DSealedHeating,
		$RunHllc2DNaturalConvection, $RunHllc2DSpeciesMixing, $RunHllc2DPerformance,
		$RunLbmD2Q9Uniform, $RunLbmD2Q9ShearWave,
		$RunLegacyLikeUniform, $RunLegacyLikePressurePulse,
		$RunHybridAllSpeedPolicy,
		$RunRusanovOpenBoundaryLeak,
		$RunRusanovPerformance) |
        Where-Object { $_ }).Count -gt 1) {
    throw "Select only one AtmosphereBench run mode"
}
$isRusanovProbe = $RunRusanovUniform -or $RunRusanovPressurePulse `
	-or $RunRusanovDensityAdvection -or $RunRusanovContactDiscontinuity `
	-or $RunRusanovNearVacuumExpansion -or $RunRusanovSodShockTube `
	-or $RunRusanovDensityAdvectionRefinement -or $RunRusanovLowMachAdvection `
	-or $RunAllSpeedRusanovLowMachAdvection `
	-or $RunHllcRusanovFallbackLowMachAdvection `
	-or $RunHllcRusanovFallbackNearVacuumExpansion `
	-or $RunHllcRusanovFallbackSodShockTube `
	-or $RunHllcRusanovFallbackOpenBoundaryLeak `
	-or $RunHllcRusanovFallbackPerformance `
	-or $RunHllc2DUniform -or $RunHllc2DPressurePulse -or $RunHllc2DSealedHeating `
	-or $RunHllc2DNaturalConvection -or $RunHllc2DSpeciesMixing -or $RunHllc2DPerformance `
	-or $RunLbmD2Q9Uniform -or $RunLbmD2Q9ShearWave `
	-or $RunLegacyLikeUniform -or $RunLegacyLikePressurePulse `
	-or $RunHybridAllSpeedPolicy `
	-or $RunRusanovOpenBoundaryLeak -or $RunRusanovPerformance
$isLbmProbe = $RunLbmD2Q9Uniform -or $RunLbmD2Q9ShearWave
$isLegacyLikeProbe = $RunLegacyLikeUniform -or $RunLegacyLikePressurePulse
$isHybridPolicyProbe = $RunHybridAllSpeedPolicy
$timer = [System.Diagnostics.Stopwatch]::StartNew()
$runMode = if ($RunRusanovPerformance) {
	"rusanov_performance"
} elseif ($RunRusanovOpenBoundaryLeak) {
	"rusanov_open_boundary_leak"
} elseif ($RunRusanovLowMachAdvection) {
	"rusanov_low_mach_advection"
} elseif ($RunAllSpeedRusanovLowMachAdvection) {
	"all_speed_rusanov_low_mach_advection"
} elseif ($RunHllcRusanovFallbackLowMachAdvection) {
	"hllc_rusanov_fallback_low_mach_advection"
} elseif ($RunHllcRusanovFallbackNearVacuumExpansion) {
	"hllc_rusanov_fallback_near_vacuum_expansion"
} elseif ($RunHllcRusanovFallbackSodShockTube) {
	"hllc_rusanov_fallback_sod_shock_tube"
} elseif ($RunHllcRusanovFallbackOpenBoundaryLeak) {
	"hllc_rusanov_fallback_open_boundary_leak"
} elseif ($RunHllcRusanovFallbackPerformance) {
	"hllc_rusanov_fallback_performance"
} elseif ($RunHllc2DPressurePulse) {
	"hllc_2d_pressure_pulse"
} elseif ($RunHllc2DSealedHeating) {
	"hllc_2d_sealed_heating"
} elseif ($RunHllc2DNaturalConvection) {
	"hllc_2d_natural_convection"
} elseif ($RunHllc2DSpeciesMixing) {
	"hllc_2d_species_mixing"
} elseif ($RunHllc2DPerformance) {
	"hllc_2d_performance"
} elseif ($RunLbmD2Q9Uniform) {
	"lbm_d2q9_uniform"
} elseif ($RunLbmD2Q9ShearWave) {
	"lbm_d2q9_shear_wave"
} elseif ($RunLegacyLikeUniform) {
	"legacy_like_uniform"
} elseif ($RunLegacyLikePressurePulse) {
	"legacy_like_pressure_pulse"
} elseif ($RunHybridAllSpeedPolicy) {
	"hybrid_all_speed_policy"
} elseif ($RunHllc2DUniform) {
	"hllc_2d_uniform"
} elseif ($RunRusanovDensityAdvectionRefinement) {
	"rusanov_density_advection_refinement"
} elseif ($RunRusanovSodShockTube) {
	"rusanov_sod_shock_tube"
} elseif ($RunRusanovNearVacuumExpansion) {
	"rusanov_near_vacuum_expansion"
} elseif ($RunRusanovContactDiscontinuity) {
    "rusanov_contact_discontinuity"
} elseif ($RunRusanovDensityAdvection) {
    "rusanov_density_advection"
} elseif ($RunRusanovPressurePulse) {
    "rusanov_pressure_pulse"
} elseif ($RunRusanovUniform) {
    "rusanov_uniform"
} else {
	"contract_uniform"
}
$runArgument = if ($RunRusanovPerformance) {
	"--run-rusanov-performance"
} elseif ($RunRusanovOpenBoundaryLeak) {
	"--run-rusanov-open-boundary-leak"
} elseif ($RunRusanovLowMachAdvection) {
	"--run-rusanov-low-mach-advection"
} elseif ($RunAllSpeedRusanovLowMachAdvection) {
	"--run-all-speed-rusanov-low-mach-advection"
} elseif ($RunHllcRusanovFallbackLowMachAdvection) {
	"--run-hllc-rusanov-fallback-low-mach-advection"
} elseif ($RunHllcRusanovFallbackNearVacuumExpansion) {
	"--run-hllc-rusanov-fallback-near-vacuum-expansion"
} elseif ($RunHllcRusanovFallbackSodShockTube) {
	"--run-hllc-rusanov-fallback-sod-shock-tube"
} elseif ($RunHllcRusanovFallbackOpenBoundaryLeak) {
	"--run-hllc-rusanov-fallback-open-boundary-leak"
} elseif ($RunHllcRusanovFallbackPerformance) {
	"--run-hllc-rusanov-fallback-performance"
} elseif ($RunHllc2DPressurePulse) {
	"--run-hllc-2d-pressure-pulse"
} elseif ($RunHllc2DSealedHeating) {
	"--run-hllc-2d-sealed-heating"
} elseif ($RunHllc2DNaturalConvection) {
	"--run-hllc-2d-natural-convection"
} elseif ($RunHllc2DSpeciesMixing) {
	"--run-hllc-2d-species-mixing"
} elseif ($RunHllc2DPerformance) {
	"--run-hllc-2d-performance"
} elseif ($RunLbmD2Q9Uniform) {
	"--run-lbm-d2q9-uniform"
} elseif ($RunLbmD2Q9ShearWave) {
	"--run-lbm-d2q9-shear-wave"
} elseif ($RunLegacyLikeUniform) {
	"--run-legacy-like-uniform"
} elseif ($RunLegacyLikePressurePulse) {
	"--run-legacy-like-pressure-pulse"
} elseif ($RunHybridAllSpeedPolicy) {
	"--run-hybrid-all-speed-policy"
} elseif ($RunHllc2DUniform) {
	"--run-hllc-2d-uniform"
} elseif ($RunRusanovDensityAdvectionRefinement) {
	"--run-rusanov-density-advection-refinement"
} elseif ($RunRusanovSodShockTube) {
	"--run-rusanov-sod-shock-tube"
} elseif ($RunRusanovNearVacuumExpansion) {
	"--run-rusanov-near-vacuum-expansion"
} elseif ($RunRusanovContactDiscontinuity) {
    "--run-rusanov-contact-discontinuity"
} elseif ($RunRusanovDensityAdvection) {
    "--run-rusanov-density-advection"
} elseif ($RunRusanovPressurePulse) {
    "--run-rusanov-pressure-pulse"
} elseif ($RunRusanovUniform) {
    "--run-rusanov-uniform"
} else {
    "--run-uniform"
}
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
if ((Read-KeyValue -Text $candidateText -Key "selection_status") -ne "unselected") {
    throw "AtmosphereBench candidate list must remain unselected"
}
foreach ($candidateLine in @(
    "candidate=legacy_like|status=implemented_dimensionless_uniform_pressure_pulse_control_only|solver_implemented=true",
    "candidate=fvm_rusanov|status=implemented_1d_uniform_pressure_pulse_density_advection_contact_near_vacuum_sod_refinement_low_mach_open_leak_performance_probes|solver_implemented=true",
    "candidate=fvm_all_speed_rusanov|status=implemented_1d_low_mach_probe_rejected|solver_implemented=true",
    "candidate=fvm_hllc_rusanov_fallback|status=implemented_1d_low_mach_near_vacuum_sod_open_leak_performance_and_2d_uniform_pressure_pulse_sealed_heating_natural_convection_species_mixing_performance_probes|solver_implemented=true",
    "candidate=hybrid_all_speed_event_local|status=implemented_uncoupled_low_mach_transport_and_whole_case_hllc_sod_policy_probe_not_solver|solver_implemented=false",
    "candidate=fvm_hlle|status=registered_only|solver_implemented=false",
    "candidate=lbm_d2q9|status=implemented_isothermal_uniform_shear_wave_only|solver_implemented=true"
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
if (-not $isRusanovProbe) {
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
} elseif ($isHybridPolicyProbe) {
	$benchmarkKind = "atmospherebench_hybrid_all_speed_policy_probe"
	$performanceGate = "not_evaluated_uncoupled_policy_probe"
	$timingScope = "standalone_uncoupled_hybrid_policy_probe"
	$candidateImplementations = "hybrid_all_speed_event_local"
	if ($solverResultStatus -ne "policy_probe_not_solver_selection") {
		throw "Hybrid policy probe must not claim solver selection"
	}
	foreach ($probeKey in @{
		"candidate" = "hybrid_all_speed_event_local";
		"candidate_solver_implemented" = "false";
		"policy_probe_implemented" = "true";
		"physical_time_policy" = "unselected";
		"low_mach_bulk_route" = "conservative_constant_pressure_transport";
		"compressible_event_route" = "hllc_rusanov_fallback_whole_case";
		"cross_route_boundary_coupling" = "not_implemented";
		"event_local_subcycling" = "not_implemented";
		"production_boundary_coupling" = "not_implemented";
		"low_mach_route_count" = "3"; "compressible_route_count" = "1";
		"low_mach_suitability_passed" = "true";
		"compressible_sod_passed" = "true";
		"compressible_sod_shock_reference_passed" = "true";
		"compressible_sod_flux_fallback_count" = "0";
		"compressible_sod_correction_count" = "0";
		"policy_selection_ready" = "false";
		"candidate_disposition" = "continue_coupling_evaluation";
		"benchmark_execution_status" = "PASS";
		"probe_passed" = "true";
		"numerical_correction_count" = "0";
		"state_bytes_per_cell" = "32";
		"state_and_flux_scratch_bytes_per_cell" = "96";
		"state_and_flux_scratch_bytes_total" = "12288"
	}.GetEnumerator()) {
		if ((Read-KeyValue -Text $text -Key $probeKey.Key) -ne $probeKey.Value) {
			throw "Hybrid policy contract drifted: $($probeKey.Key)"
		}
	}
	foreach ($metricKey in @(
		"moderate_density_l1_error", "low_density_l1_error", "very_low_density_l1_error",
		"moderate_total_variation_ratio", "low_total_variation_ratio", "very_low_total_variation_ratio")) {
		$metric = [double](Read-KeyValue -Text $text -Key $metricKey)
		if ([double]::IsNaN($metric) -or [double]::IsInfinity($metric) -or $metric -le 0.0) {
			throw "Hybrid policy metric is invalid: $metricKey"
		}
	}
	if ([double](Read-KeyValue -Text $text -Key "very_low_density_l1_error") -gt 0.05 -or
		[double](Read-KeyValue -Text $text -Key "very_low_total_variation_ratio") -lt 0.8) {
		throw "Hybrid low-Mach transport exceeded the unchanged suitability threshold"
	}
	foreach ($driftKey in @("mass_drift", "momentum_x_drift", "momentum_y_drift", "energy_drift")) {
		if ([Math]::Abs([double](Read-KeyValue -Text $text -Key $driftKey)) -gt 1e-9) {
			throw "Hybrid low-Mach transport drift exceeds tolerance: $driftKey"
		}
	}
} elseif ($isLegacyLikeProbe) {
	$isPulse = $RunLegacyLikePressurePulse
	$benchmarkKind = if ($isPulse) {
		"atmospherebench_legacy_like_pressure_pulse_control"
	} else {
		"atmospherebench_legacy_like_uniform_control"
	}
	$performanceGate = "not_evaluated_control_probe"
	$timingScope = if ($isPulse) {
		"standalone_legacy_like_pressure_pulse_control"
	} else {
		"standalone_legacy_like_uniform_control"
	}
	$candidateImplementations = "legacy_like"
	if ($solverResultStatus -ne "control_result_not_solver_selection") {
		throw "Legacy-like control must not claim solver selection"
	}
	if ((Read-KeyValue -Text $text -Key "candidate") -ne "legacy_like" -or
		(Read-KeyValue -Text $text -Key "candidate_solver_implemented") -ne "true") {
		throw "Legacy-like control identity is invalid"
	}
	foreach ($probeKey in @{
		"case_time_domain" = "nondimensional_contract"; "dimension" = "2";
		"boundary_mode" = "periodic";
		"control_model" = "dimensionless_pressure_velocity_stencil";
		"production_air_equivalence" = "not_claimed";
		"physical_mass_state" = "not_implemented";
		"physical_density_state" = "not_implemented";
		"physical_momentum_state" = "not_implemented";
		"physical_energy_state" = "not_implemented";
		"species_state" = "not_implemented";
		"mass_conservation" = "not_applicable_no_mass_state";
		"momentum_conservation" = "not_applicable_no_momentum_density_state";
		"energy_conservation" = "not_applicable_no_energy_state";
		"grid_cells_x" = "64"; "grid_cells_y" = "48"; "grid_cell_count" = "3072";
		"cell_length" = "1"; "case_timestep" = "1";
		"maximum_cfl" = "not_applicable_legacy_dimensionless_stencil";
		"minimum_density" = "not_applicable_no_density_state";
		"mass_drift" = "not_applicable_no_mass_state";
		"momentum_x_drift" = "not_applicable_no_momentum_density_state";
		"momentum_y_drift" = "not_applicable_no_momentum_density_state";
		"momentum_drift" = "not_applicable_no_momentum_density_state";
		"energy_drift" = "not_applicable_no_energy_state";
		"pressure_sum_preserved" = "true"; "finite_state" = "true";
		"numerical_correction_count" = "0"; "state_bytes_per_cell" = "32";
		"state_and_flux_scratch_bytes_per_cell" = "64";
		"state_and_flux_scratch_bytes_total" = "196608"; "probe_passed" = "true"
	}.GetEnumerator()) {
		if ((Read-KeyValue -Text $text -Key $probeKey.Key) -ne $probeKey.Value) {
			throw "Legacy-like control contract drifted: $($probeKey.Key)"
		}
	}
	$expectedCase = if ($isPulse) { "legacy_like_pressure_pulse_2d" } else { "legacy_like_uniform_2d" }
	$expectedSteps = if ($isPulse) { "96" } else { "32" }
	$expectedEvolved = if ($isPulse) { "true" } else { "false" }
	$expectedUniform = if ($isPulse) { "false" } else { "true" }
	$expectedPeakReduced = if ($isPulse) { "true" } else { "false" }
	foreach ($probeKey in @{
		"case" = $expectedCase; "case_step_count" = $expectedSteps;
		"state_evolved" = $expectedEvolved; "uniform_preserved" = $expectedUniform;
		"pressure_peak_reduced" = $expectedPeakReduced
	}.GetEnumerator()) {
		if ((Read-KeyValue -Text $text -Key $probeKey.Key) -ne $probeKey.Value) {
			throw "Legacy-like case contract drifted: $($probeKey.Key)"
		}
	}
	if ([Math]::Abs([double](Read-KeyValue -Text $text -Key "pressure_sum_drift")) -gt 1e-10) {
		throw "Legacy-like pressure-field sum drift exceeds control tolerance"
	}
	if ($isPulse) {
		$initialPeak = [double](Read-KeyValue -Text $text -Key "initial_maximum_pressure")
		$finalPeak = [double](Read-KeyValue -Text $text -Key "final_maximum_pressure")
		$maximumVelocity = [double](Read-KeyValue -Text $text -Key "maximum_absolute_velocity")
		if ($finalPeak -ge $initialPeak - 1e-6 -or $maximumVelocity -le 1e-6) {
			throw "Legacy-like pressure pulse did not evolve through the control stencil"
		}
	} elseif ([Math]::Abs([double](Read-KeyValue -Text $text -Key "state_change_l1")) -gt 1e-12) {
		throw "Legacy-like uniform state changed above tolerance"
	}
} elseif ($isLbmProbe) {
	$isShearWave = $RunLbmD2Q9ShearWave
	$benchmarkKind = if ($isShearWave) {
		"atmospherebench_lbm_d2q9_shear_wave_probe"
	} else {
		"atmospherebench_lbm_d2q9_uniform_probe"
	}
	$performanceGate = "not_evaluated_candidate_probe"
	$timingScope = if ($isShearWave) {
		"standalone_lbm_d2q9_shear_wave_probe"
	} else {
		"standalone_lbm_d2q9_uniform_probe"
	}
	$candidateImplementations = "lbm_d2q9"
	if ($solverResultStatus -ne "candidate_result_not_selection") {
		throw "LBM D2Q9 probe must not claim solver selection"
	}
	if ((Read-KeyValue -Text $text -Key "candidate") -ne "lbm_d2q9" -or
		(Read-KeyValue -Text $text -Key "candidate_solver_implemented") -ne "true") {
		throw "LBM D2Q9 candidate identity is invalid"
	}
	foreach ($probeKey in @{
		"case_time_domain" = "nondimensional_contract"; "dimension" = "2";
		"boundary_mode" = "periodic"; "lbm_model" = "d2q9_bgk_isothermal";
		"energy_state" = "not_implemented";
		"energy_conservation" = "not_applicable_no_energy_state";
		"near_vacuum_support" = "unsupported_low_mach_positive_population_contract";
		"shock_support" = "unsupported_isothermal_low_mach_model";
		"species_support" = "not_implemented"; "cell_length" = "1";
		"case_timestep" = "1"; "relaxation_time" = "0.8";
		"kinematic_viscosity" = "0.1";
		"maximum_cfl" = "not_applicable_lattice_streaming";
		"energy_drift" = "not_applicable_no_energy_state";
		"mass_conserved" = "true"; "momentum_conserved" = "true";
		"positivity_preserved" = "true"; "numerical_correction_count" = "0";
		"state_bytes_per_cell" = "72"; "state_and_flux_scratch_bytes_per_cell" = "144";
		"probe_passed" = "true"
	}.GetEnumerator()) {
		if ((Read-KeyValue -Text $text -Key $probeKey.Key) -ne $probeKey.Value) {
			throw "LBM D2Q9 contract drifted: $($probeKey.Key)"
		}
	}
	$expectedCase = if ($isShearWave) { "lbm_d2q9_shear_wave_2d" } else { "lbm_d2q9_uniform_2d" }
	$expectedCellsX = "64"
	$expectedCellsY = if ($isShearWave) { "64" } else { "48" }
	$expectedCellCount = if ($isShearWave) { "4096" } else { "3072" }
	$expectedSteps = if ($isShearWave) { "128" } else { "64" }
	$expectedScratchTotal = if ($isShearWave) { "589824" } else { "442368" }
	$expectedEvolved = if ($isShearWave) { "true" } else { "false" }
	$expectedUniform = if ($isShearWave) { "false" } else { "true" }
	$expectedShear = if ($isShearWave) { "true" } else { "false" }
	foreach ($probeKey in @{
		"case" = $expectedCase; "grid_cells_x" = $expectedCellsX;
		"grid_cells_y" = $expectedCellsY; "grid_cell_count" = $expectedCellCount;
		"case_step_count" = $expectedSteps; "state_evolved" = $expectedEvolved;
		"uniform_preserved" = $expectedUniform; "shear_reference_passed" = $expectedShear;
		"state_and_flux_scratch_bytes_total" = $expectedScratchTotal
	}.GetEnumerator()) {
		if ((Read-KeyValue -Text $text -Key $probeKey.Key) -ne $probeKey.Value) {
			throw "LBM D2Q9 case contract drifted: $($probeKey.Key)"
		}
	}
	foreach ($metricKey in @("minimum_density", "minimum_pressure", "minimum_population", "maximum_mach")) {
		$value = [double](Read-KeyValue -Text $text -Key $metricKey)
		if ([double]::IsNaN($value) -or [double]::IsInfinity($value) -or $value -le 0.0) {
			throw "LBM D2Q9 positive metric is invalid: $metricKey"
		}
	}
	if ([double](Read-KeyValue -Text $text -Key "maximum_mach") -gt 0.1) {
		throw "LBM D2Q9 probe exceeded its published low-Mach domain"
	}
	foreach ($driftKey in @("mass_drift", "momentum_x_drift", "momentum_y_drift")) {
		if ([Math]::Abs([double](Read-KeyValue -Text $text -Key $driftKey)) -gt 1e-8) {
			throw "LBM D2Q9 conservation drift exceeds tolerance: $driftKey"
		}
	}
	if ($isShearWave) {
		$relativeError = [double](Read-KeyValue -Text $text -Key "shear_amplitude_relative_error")
		if ([double]::IsNaN($relativeError) -or [double]::IsInfinity($relativeError) -or
			$relativeError -lt 0.0 -or $relativeError -gt 0.02) {
			throw "LBM D2Q9 shear-wave decay error exceeds tolerance"
		}
	} elseif ([Math]::Abs([double](Read-KeyValue -Text $text -Key "state_change_l1")) -gt 1e-10) {
		throw "LBM D2Q9 uniform state changed above tolerance"
	}
} elseif ($RunHllc2DSpeciesMixing) {
	$benchmarkKind = "atmospherebench_hllc_2d_species_mixing_probe"
	$performanceGate = "not_evaluated_candidate_probe"
	$timingScope = "standalone_hllc_2d_species_mixing_probe"
	$candidateImplementations = "fvm_hllc_rusanov_fallback"
	if ($solverResultStatus -ne "candidate_result_not_selection") {
		throw "HLLC 2D species-mixing probe must not claim solver selection"
	}
	if ((Read-KeyValue -Text $text -Key "candidate") -ne "fvm_hllc_rusanov_fallback" -or
		(Read-KeyValue -Text $text -Key "candidate_solver_implemented") -ne "true") {
		throw "HLLC 2D species-mixing candidate identity is invalid"
	}
	foreach ($probeKey in @{
		"case_time_domain" = "nondimensional_contract"; "dimension" = "2";
		"boundary_mode" = "periodic"; "species_model" = "passive_conserved_binary_fixture";
		"species_eos_coupling" = "not_implemented"; "physical_diffusion" = "not_implemented";
		"grid_cells_x" = "32"; "grid_cells_y" = "24"; "grid_cell_count" = "768";
		"cell_length" = "1"; "case_timestep" = "0.1"; "case_step_count" = "320";
		"initial_species_a_mass" = "384"; "final_species_a_mass" = "384";
		"initial_species_b_mass" = "384"; "final_species_b_mass" = "384";
		"initial_mixed_cell_count" = "0"; "final_mixed_cell_count" = "768";
		"gas_ledger_closes" = "true"; "species_ledger_closes" = "true";
		"species_bounds_preserved" = "true"; "composition_evolved" = "true";
		"mixed_region_formed" = "true"; "flux_fallback_count" = "0";
		"numerical_correction_count" = "0"; "state_bytes_per_cell" = "40";
		"state_and_flux_scratch_bytes_per_cell" = "200";
		"state_and_flux_scratch_bytes_total" = "153600"; "probe_passed" = "true"
	}.GetEnumerator()) {
		if ((Read-KeyValue -Text $text -Key $probeKey.Key) -ne $probeKey.Value) {
			throw "HLLC 2D species-mixing contract drifted: $($probeKey.Key)"
		}
	}
	$maximumCfl = [double](Read-KeyValue -Text $text -Key "maximum_cfl")
	if ([double]::IsNaN($maximumCfl) -or [double]::IsInfinity($maximumCfl) -or
		$maximumCfl -le 0.0 -or $maximumCfl -gt 1.0) {
		throw "HLLC 2D species-mixing CFL is outside the strict positivity contract"
	}
	$minimumSpeciesFraction = [double](Read-KeyValue -Text $text -Key "minimum_species_a_fraction")
	$maximumSpeciesFraction = [double](Read-KeyValue -Text $text -Key "maximum_species_a_fraction")
	if ($minimumSpeciesFraction -lt -1e-12 -or $maximumSpeciesFraction -gt 1.0 + 1e-12) {
		throw "HLLC 2D species-mixing fraction bounds were violated"
	}
	$initialCompositionVariation = [double](Read-KeyValue -Text $text -Key "initial_composition_total_variation")
	$finalCompositionVariation = [double](Read-KeyValue -Text $text -Key "final_composition_total_variation")
	$compositionStateChange = [double](Read-KeyValue -Text $text -Key "composition_state_change_l1")
	if ($finalCompositionVariation -gt $initialCompositionVariation - 1e-6 -or
		$compositionStateChange -le 1e-6) {
		throw "HLLC 2D species-mixing composition did not evolve with a measurable TV decrease"
	}
	foreach ($driftKey in @(
		"mass_drift", "momentum_x_drift", "momentum_y_drift", "energy_drift",
		"species_a_mass_drift", "species_b_mass_drift")) {
		if ([Math]::Abs([double](Read-KeyValue -Text $text -Key $driftKey)) -gt 1e-10) {
			throw "HLLC 2D species-mixing conservation drift exceeds tolerance: $driftKey"
		}
	}
} elseif ($RunHllc2DPerformance) {
	$benchmarkKind = "atmospherebench_hllc_2d_performance_probe"
	$performanceGate = "recorded_candidate_measurement_no_budget"
	$timingScope = "standalone_hllc_2d_periodic_end_to_end_probe_wrapper"
	$candidateImplementations = "fvm_hllc_rusanov_fallback"
	if ($solverResultStatus -ne "candidate_result_not_selection") {
		throw "HLLC 2D performance probe must not claim solver selection"
	}
	if ((Read-KeyValue -Text $text -Key "candidate") -ne "fvm_hllc_rusanov_fallback" -or
		(Read-KeyValue -Text $text -Key "candidate_solver_implemented") -ne "true") {
		throw "HLLC 2D performance candidate identity is invalid"
	}
	foreach ($probeKey in @{
		"case_time_domain" = "nondimensional_contract"; "dimension" = "2";
		"boundary_mode" = "periodic"; "physical_time_policy" = "unselected";
		"performance_budget_status" = "unselected"; "grid_cells_x" = "612";
		"grid_cells_y" = "384"; "grid_cell_count" = "235008";
		"cell_length" = "1"; "case_step_count" = "32";
		"performance_warmup_count" = "1"; "performance_repeat_count" = "3";
		"performance_steps" = "32"; "legacy_grid_cells_x" = "153";
		"legacy_grid_cells_y" = "96"; "legacy_grid_cell_count" = "14688";
		"doubled_grid_cells_x" = "306"; "doubled_grid_cells_y" = "192";
		"doubled_grid_cell_count" = "58752"; "particle_grid_cells_x" = "612";
		"particle_grid_cells_y" = "384"; "particle_grid_cell_count" = "235008";
		"state_evolved" = "true"; "positivity_preserved" = "true";
		"legacy_grid_flux_fallback_count" = "0";
		"doubled_grid_flux_fallback_count" = "0";
		"particle_grid_flux_fallback_count" = "0";
		"numerical_correction_count" = "0"; "state_bytes_per_cell" = "32";
		"state_and_flux_scratch_bytes_per_cell" = "160";
		"state_and_flux_scratch_bytes_total" = "37601280";
		"performance_measurement_passed" = "true"; "probe_passed" = "true"
	}.GetEnumerator()) {
		if ((Read-KeyValue -Text $text -Key $probeKey.Key) -ne $probeKey.Value) {
			throw "HLLC 2D performance contract drifted: $($probeKey.Key)"
		}
	}
	foreach ($metricKey in @(
		"case_timestep", "legacy_grid_elapsed_milliseconds",
		"doubled_grid_elapsed_milliseconds", "particle_grid_elapsed_milliseconds",
		"legacy_grid_milliseconds_per_step", "doubled_grid_milliseconds_per_step",
		"particle_grid_milliseconds_per_step", "legacy_grid_cell_updates_per_second",
		"doubled_grid_cell_updates_per_second", "particle_grid_cell_updates_per_second",
		"reference_frame_budget_milliseconds", "legacy_grid_fraction_of_reference_frame",
		"doubled_grid_fraction_of_reference_frame", "particle_grid_fraction_of_reference_frame")) {
		$value = [double](Read-KeyValue -Text $text -Key $metricKey)
		if ([double]::IsNaN($value) -or [double]::IsInfinity($value) -or $value -le 0.0) {
			throw "HLLC 2D performance metric is invalid: $metricKey"
		}
	}
	$maximumCfl = [double](Read-KeyValue -Text $text -Key "maximum_cfl")
	if ([double]::IsNaN($maximumCfl) -or [double]::IsInfinity($maximumCfl) -or
		$maximumCfl -le 0.0 -or $maximumCfl -gt 1.0) {
		throw "HLLC 2D performance CFL is outside the strict positivity contract"
	}
	foreach ($driftKey in @("mass_drift", "momentum_x_drift", "momentum_y_drift", "energy_drift")) {
		if ([Math]::Abs([double](Read-KeyValue -Text $text -Key $driftKey)) -gt 1e-7) {
			throw "HLLC 2D performance conservation drift exceeds tolerance: $driftKey"
		}
	}
} elseif ($RunHllc2DNaturalConvection) {
	$benchmarkKind = "atmospherebench_hllc_2d_natural_convection_probe"
	$performanceGate = "not_evaluated_candidate_probe"
	$timingScope = "standalone_hllc_2d_natural_convection_probe"
	$candidateImplementations = "fvm_hllc_rusanov_fallback"
	if ($solverResultStatus -ne "candidate_result_not_selection") {
		throw "HLLC 2D natural-convection probe must not claim solver selection"
	}
	if ((Read-KeyValue -Text $text -Key "candidate") -ne "fvm_hllc_rusanov_fallback" -or
		(Read-KeyValue -Text $text -Key "candidate_solver_implemented") -ne "true") {
		throw "HLLC 2D natural-convection candidate identity is invalid"
	}
	foreach ($probeKey in @{
		"case_time_domain" = "nondimensional_contract"; "dimension" = "2";
		"boundary_mode" = "sealed"; "grid_cells_x" = "32"; "grid_cells_y" = "24";
		"grid_cell_count" = "768"; "cell_length" = "1"; "case_timestep" = "0.01";
		"case_step_count" = "400"; "gravity_y" = "-0.04";
		"hot_temperature_amplitude" = "0.5"; "circulation_observed" = "true";
		"control_positivity_preserved" = "true"; "heated_positivity_preserved" = "true";
		"control_source_ledger_closes" = "false"; "heated_source_ledger_closes" = "false";
		"control_source_and_boundary_ledger_closes" = "true";
		"heated_source_and_boundary_ledger_closes" = "true";
		"control_source_event_count" = "307200"; "heated_source_event_count" = "307200";
		"control_boundary_event_count" = "400"; "heated_boundary_event_count" = "400";
		"control_flux_fallback_count" = "0"; "heated_flux_fallback_count" = "0";
		"control_numerical_correction_count" = "0";
		"heated_numerical_correction_count" = "0";
		"control_probe_passed" = "true"; "heated_probe_passed" = "true";
		"state_and_flux_scratch_bytes_total" = "124672"; "probe_passed" = "true"
	}.GetEnumerator()) {
		if ((Read-KeyValue -Text $text -Key $probeKey.Key) -ne $probeKey.Value) {
			throw "HLLC 2D natural-convection contract drifted: $($probeKey.Key)"
		}
	}
	foreach ($prefix in @("control", "heated")) {
		foreach ($positiveKey in @("minimum_density", "minimum_pressure", "maximum_cfl")) {
			$value = [double](Read-KeyValue -Text $text -Key "${prefix}_${positiveKey}")
			if ([double]::IsNaN($value) -or [double]::IsInfinity($value) -or
				$value -le 0.0 -or ($positiveKey -eq "maximum_cfl" -and $value -gt 1.0)) {
				throw "HLLC 2D natural-convection metric is invalid: ${prefix}_${positiveKey}"
			}
		}
		foreach ($balanceKey in @(
			"combined_mass_balance_error", "combined_momentum_x_balance_error",
			"combined_momentum_y_balance_error", "combined_energy_balance_error")) {
			if ([Math]::Abs([double](Read-KeyValue -Text $text -Key "${prefix}_${balanceKey}")) -gt 1e-8) {
				throw "HLLC 2D natural-convection ledger exceeds tolerance: ${prefix}_${balanceKey}"
			}
		}
	}
	$thermalCenterRise = [double](Read-KeyValue -Text $text -Key "thermal_center_rise")
	$thermalWeightedVelocityY = [double](Read-KeyValue -Text $text -Key "thermal_weighted_velocity_y")
	$maximumUpwardVelocityDifference = [double](Read-KeyValue -Text $text -Key "maximum_upward_velocity_difference")
	$minimumDownwardVelocityDifference = [double](Read-KeyValue -Text $text -Key "minimum_downward_velocity_difference")
	$maximumAbsoluteVelocityDifference = [double](Read-KeyValue -Text $text -Key "maximum_absolute_velocity_difference")
	if ($thermalCenterRise -lt 0.05 -or $thermalWeightedVelocityY -le 0.0 -or
		$maximumUpwardVelocityDifference -le 1e-4 -or
		$minimumDownwardVelocityDifference -ge -1e-4 -or
		$maximumAbsoluteVelocityDifference -le 1e-4) {
		throw "HLLC 2D natural-convection circulation signal is below its published threshold"
	}
} elseif ($RunHllc2DUniform -or $RunHllc2DPressurePulse -or $RunHllc2DSealedHeating) {
    $isPulse = $RunHllc2DPressurePulse
	$isHeating = $RunHllc2DSealedHeating
    $benchmarkKind = if ($isHeating) { "atmospherebench_hllc_2d_sealed_heating_probe" } elseif ($isPulse) { "atmospherebench_hllc_2d_pressure_pulse_probe" } else { "atmospherebench_hllc_2d_uniform_probe" }
    $performanceGate = "not_evaluated_candidate_probe"
	$timingScope = if ($isHeating) { "standalone_hllc_2d_sealed_heating_probe" } elseif ($isPulse) { "standalone_hllc_2d_pressure_pulse_probe" } else { "standalone_hllc_2d_uniform_probe" }
    $candidateImplementations = "fvm_hllc_rusanov_fallback"
    if ($solverResultStatus -ne "candidate_result_not_selection") {
        throw "HLLC 2D probe must not claim solver selection"
    }
    if ((Read-KeyValue -Text $text -Key "candidate") -ne "fvm_hllc_rusanov_fallback" -or
        (Read-KeyValue -Text $text -Key "candidate_solver_implemented") -ne "true") {
        throw "HLLC 2D probe candidate identity is invalid"
    }
    foreach ($probeKey in @{
        "case_time_domain" = "nondimensional_contract"; "dimension" = "2";
        "grid_cells_x" = "32";
        "grid_cells_y" = "24"; "grid_cell_count" = "768";
        "cell_length" = "1"; "positivity_preserved" = "true";
        "flux_fallback_count" = "0"; "numerical_correction_count" = "0";
        "probe_passed" = "true"
    }.GetEnumerator()) {
        if ((Read-KeyValue -Text $text -Key $probeKey.Key) -ne $probeKey.Value) {
            throw "HLLC 2D contract drifted: $($probeKey.Key)"
        }
    }
	$expectedBoundaryMode = if ($isHeating) { "sealed" } else { "periodic" }
	if ((Read-KeyValue -Text $text -Key "boundary_mode") -ne $expectedBoundaryMode) {
		throw "HLLC 2D boundary contract drifted"
	}
    if ($isHeating) {
		foreach ($probeKey in @{
			"case_timestep" = "0.01"; "case_step_count" = "40";
			"state_evolved" = "true"; "pressure_peak_reduced" = "false";
			"pressure_increased" = "true"; "temperature_increased" = "true";
			"source_ledger_closes" = "true"; "source_event_count" = "30720";
			"source_mass_net" = "0"; "source_momentum_x_net" = "0";
			"source_momentum_y_net" = "0"; "state_and_flux_scratch_bytes_total" = "124672"
		}.GetEnumerator()) {
			if ((Read-KeyValue -Text $text -Key $probeKey.Key) -ne $probeKey.Value) {
				throw "HLLC 2D sealed-heating contract drifted: $($probeKey.Key)"
			}
		}
		$sourceEnergy = [double](Read-KeyValue -Text $text -Key "source_energy_net")
		$energyDrift = [double](Read-KeyValue -Text $text -Key "energy_drift")
		if ([double]::IsNaN($sourceEnergy) -or [double]::IsInfinity($sourceEnergy) -or
			$sourceEnergy -le 0.0 -or [Math]::Abs($energyDrift - $sourceEnergy) -gt 1e-9) {
			throw "HLLC 2D sealed-heating energy source did not reconcile"
		}
		foreach ($balanceKey in @(
			"source_mass_balance_error", "source_momentum_x_balance_error",
			"source_momentum_y_balance_error", "source_energy_balance_error")) {
			if ([Math]::Abs([double](Read-KeyValue -Text $text -Key $balanceKey)) -gt 1e-9) {
				throw "HLLC 2D sealed-heating source balance exceeds tolerance: $balanceKey"
			}
		}
		foreach ($thermoKey in @("final_mean_pressure", "final_mean_temperature")) {
			$expectedKey = $thermoKey -replace '^final_', 'expected_final_'
			$actualThermo = [double](Read-KeyValue -Text $text -Key $thermoKey)
			$expectedThermo = [double](Read-KeyValue -Text $text -Key $expectedKey)
			if ([Math]::Abs($actualThermo - $expectedThermo) -gt 1e-12) {
				throw "HLLC 2D sealed-heating thermodynamic response drifted: $thermoKey"
			}
		}
    } elseif ($isPulse) {
        foreach ($probeKey in @{
            "case_timestep" = "0.01"; "case_step_count" = "40";
            "state_evolved" = "true"; "pressure_peak_reduced" = "true"
        }.GetEnumerator()) {
            if ((Read-KeyValue -Text $text -Key $probeKey.Key) -ne $probeKey.Value) {
                throw "HLLC 2D pressure-pulse contract drifted: $($probeKey.Key)"
            }
        }
    } else {
        foreach ($probeKey in @{
            "case_timestep" = "0.02"; "case_step_count" = "8";
            "state_evolved" = "false"; "pressure_peak_reduced" = "false";
            "state_change_l1" = "0"
        }.GetEnumerator()) {
            if ((Read-KeyValue -Text $text -Key $probeKey.Key) -ne $probeKey.Value) {
                throw "HLLC 2D uniform contract drifted: $($probeKey.Key)"
            }
        }
    }
    $maximumCfl = [double](Read-KeyValue -Text $text -Key "maximum_cfl")
    if ([double]::IsNaN($maximumCfl) -or [double]::IsInfinity($maximumCfl) -or
        $maximumCfl -le 0.0 -or $maximumCfl -gt 1.0) {
        throw "HLLC 2D CFL is outside the strict positivity contract"
    }
	$driftKeys = if ($isHeating) {
		@("mass_drift", "momentum_x_drift", "momentum_y_drift")
	} else {
		@("mass_drift", "momentum_x_drift", "momentum_y_drift", "energy_drift")
	}
    foreach ($driftKey in $driftKeys) {
        if ([Math]::Abs([double](Read-KeyValue -Text $text -Key $driftKey)) -gt 1e-9) {
            throw "HLLC 2D conservation drift exceeds tolerance: $driftKey"
        }
    }
} elseif ($RunRusanovUniform) {
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
    if ([double]::IsNaN($maximumCfl) -or [double]::IsInfinity($maximumCfl) -or
        $maximumCfl -le 0.0 -or $maximumCfl -gt 1.0) {
        throw "Rusanov probe CFL is outside the strict positivity contract"
    }
    foreach ($driftKey in @("mass_drift", "momentum_x_drift", "momentum_y_drift", "energy_drift")) {
        if ([Math]::Abs([double](Read-KeyValue -Text $text -Key $driftKey)) -gt 1e-12) {
            throw "Rusanov probe drift exceeds the periodic conservation tolerance: $driftKey"
        }
    }
} elseif ($RunRusanovPressurePulse) {
    $benchmarkKind = "atmospherebench_rusanov_pressure_pulse_probe"
    $performanceGate = "not_evaluated_candidate_probe"
    $timingScope = "standalone_rusanov_pressure_pulse_probe"
    $candidateImplementations = "fvm_rusanov"
    if ($solverResultStatus -ne "candidate_result_not_selection") {
        throw "Rusanov pressure-pulse probe must not claim solver selection"
    }
    if ((Read-KeyValue -Text $text -Key "candidate") -ne "fvm_rusanov" -or
        (Read-KeyValue -Text $text -Key "candidate_solver_implemented") -ne "true") {
        throw "Rusanov pressure-pulse candidate identity is invalid"
    }
    foreach ($probeKey in @{
        "case_time_domain" = "nondimensional_contract"; "grid_cells_x" = "128";
        "grid_cells_y" = "1"; "grid_cell_count" = "128"; "case_timestep" = "0.02";
        "case_step_count" = "64"; "positivity_preserved" = "true";
        "pressure_peak_reduced" = "true"; "state_evolved" = "true";
        "numerical_correction_count" = "0"; "probe_passed" = "true"
    }.GetEnumerator()) {
        if ((Read-KeyValue -Text $text -Key $probeKey.Key) -ne $probeKey.Value) {
            throw "Rusanov pressure-pulse contract drifted: $($probeKey.Key)"
        }
    }
    $maximumCfl = [double](Read-KeyValue -Text $text -Key "maximum_cfl")
    if ([double]::IsNaN($maximumCfl) -or [double]::IsInfinity($maximumCfl) -or
        $maximumCfl -le 0.0 -or $maximumCfl -gt 1.0) {
        throw "Rusanov pressure-pulse CFL is outside the strict positivity contract"
    }
    $stateChangeL1 = [double](Read-KeyValue -Text $text -Key "state_change_l1")
    if ([double]::IsNaN($stateChangeL1) -or [double]::IsInfinity($stateChangeL1) -or $stateChangeL1 -le 1e-12) {
        throw "Rusanov pressure-pulse did not produce a measurable state evolution"
    }
    foreach ($driftKey in @("mass_drift", "momentum_x_drift", "momentum_y_drift", "energy_drift")) {
        if ([Math]::Abs([double](Read-KeyValue -Text $text -Key $driftKey)) -gt 1e-10) {
            throw "Rusanov pressure-pulse drift exceeds the periodic conservation tolerance: $driftKey"
        }
    }
} elseif ($RunRusanovDensityAdvection) {
    $benchmarkKind = "atmospherebench_rusanov_density_advection_probe"
    $performanceGate = "not_evaluated_candidate_probe"
    $timingScope = "standalone_rusanov_density_advection_probe"
    $candidateImplementations = "fvm_rusanov"
    if ($solverResultStatus -ne "candidate_result_not_selection") {
        throw "Rusanov density-advection probe must not claim solver selection"
    }
    if ((Read-KeyValue -Text $text -Key "candidate") -ne "fvm_rusanov" -or
        (Read-KeyValue -Text $text -Key "candidate_solver_implemented") -ne "true") {
        throw "Rusanov density-advection candidate identity is invalid"
    }
    foreach ($probeKey in @{
        "case_time_domain" = "nondimensional_contract"; "grid_cells_x" = "128";
        "grid_cells_y" = "1"; "grid_cell_count" = "128"; "case_timestep" = "0.1";
        "case_step_count" = "20"; "positivity_preserved" = "true";
        "state_evolved" = "true"; "advection_reference_passed" = "true";
        "reference_shift_cells" = "1"; "numerical_correction_count" = "0";
        "probe_passed" = "true"
    }.GetEnumerator()) {
        if ((Read-KeyValue -Text $text -Key $probeKey.Key) -ne $probeKey.Value) {
            throw "Rusanov density-advection contract drifted: $($probeKey.Key)"
        }
    }
    $maximumCfl = [double](Read-KeyValue -Text $text -Key "maximum_cfl")
    if ([double]::IsNaN($maximumCfl) -or [double]::IsInfinity($maximumCfl) -or
        $maximumCfl -le 0.0 -or $maximumCfl -gt 1.0) {
        throw "Rusanov density-advection CFL is outside the strict positivity contract"
    }
    foreach ($errorKey in @("density_l1_error", "density_linf_error", "pressure_linf_error")) {
        $errorValue = [double](Read-KeyValue -Text $text -Key $errorKey)
        if ([double]::IsNaN($errorValue) -or [double]::IsInfinity($errorValue)) {
            throw "Rusanov density-advection error is non-finite: $errorKey"
        }
    }
    if ([double](Read-KeyValue -Text $text -Key "density_l1_error") -gt 0.01 -or
        [double](Read-KeyValue -Text $text -Key "density_linf_error") -gt 0.02 -or
        [double](Read-KeyValue -Text $text -Key "pressure_linf_error") -gt 1e-10) {
        throw "Rusanov density-advection reference error exceeds its published tolerance"
    }
    foreach ($driftKey in @("mass_drift", "momentum_x_drift", "momentum_y_drift", "energy_drift")) {
        if ([Math]::Abs([double](Read-KeyValue -Text $text -Key $driftKey)) -gt 1e-10) {
            throw "Rusanov density-advection drift exceeds the periodic conservation tolerance: $driftKey"
        }
    }
} elseif ($RunRusanovContactDiscontinuity) {
    $benchmarkKind = "atmospherebench_rusanov_contact_discontinuity_probe"
    $performanceGate = "not_evaluated_candidate_probe"
    $timingScope = "standalone_rusanov_contact_discontinuity_probe"
    $candidateImplementations = "fvm_rusanov"
    if ($solverResultStatus -ne "candidate_result_not_selection") {
        throw "Rusanov contact probe must not claim solver selection"
    }
    foreach ($probeKey in @{
        "case_time_domain" = "nondimensional_contract"; "grid_cells_x" = "128";
        "grid_cells_y" = "1"; "grid_cell_count" = "128"; "case_timestep" = "0.1";
        "case_step_count" = "20"; "positivity_preserved" = "true";
        "state_evolved" = "true"; "advection_reference_passed" = "true";
        "density_bounds_preserved" = "true"; "reference_shift_cells" = "1";
        "numerical_correction_count" = "0"; "probe_passed" = "true"
    }.GetEnumerator()) {
        if ((Read-KeyValue -Text $text -Key $probeKey.Key) -ne $probeKey.Value) {
            throw "Rusanov contact contract drifted: $($probeKey.Key)"
        }
    }
    $maximumCfl = [double](Read-KeyValue -Text $text -Key "maximum_cfl")
    if ([double]::IsNaN($maximumCfl) -or [double]::IsInfinity($maximumCfl) -or
        $maximumCfl -le 0.0 -or $maximumCfl -gt 1.0) {
        throw "Rusanov contact CFL is outside the strict positivity contract"
    }
    if ([double](Read-KeyValue -Text $text -Key "density_l1_error") -gt 0.02 -or
        [double](Read-KeyValue -Text $text -Key "density_linf_error") -gt 0.2 -or
        [double](Read-KeyValue -Text $text -Key "pressure_linf_error") -gt 1e-10) {
        throw "Rusanov contact reference error exceeds its published first-order tolerance"
    }
    foreach ($driftKey in @("mass_drift", "momentum_x_drift", "momentum_y_drift", "energy_drift")) {
        if ([Math]::Abs([double](Read-KeyValue -Text $text -Key $driftKey)) -gt 1e-10) {
            throw "Rusanov contact drift exceeds the periodic conservation tolerance: $driftKey"
        }
    }
} elseif ($RunRusanovNearVacuumExpansion -or $RunHllcRusanovFallbackNearVacuumExpansion) {
    $isHllcProbe = $RunHllcRusanovFallbackNearVacuumExpansion
    $expectedCandidate = if ($isHllcProbe) { "fvm_hllc_rusanov_fallback" } else { "fvm_rusanov" }
    $benchmarkKind = if ($isHllcProbe) { "atmospherebench_hllc_rusanov_fallback_near_vacuum_expansion_probe" } else { "atmospherebench_rusanov_near_vacuum_expansion_probe" }
    $performanceGate = "not_evaluated_candidate_probe"
    $timingScope = if ($isHllcProbe) { "standalone_hllc_rusanov_fallback_near_vacuum_expansion_probe" } else { "standalone_rusanov_near_vacuum_expansion_probe" }
    $candidateImplementations = $expectedCandidate
    if ($solverResultStatus -ne "candidate_result_not_selection") {
        throw "Rusanov near-vacuum probe must not claim solver selection"
    }
    if ((Read-KeyValue -Text $text -Key "candidate") -ne $expectedCandidate -or
        (Read-KeyValue -Text $text -Key "candidate_solver_implemented") -ne "true") {
        throw "Rusanov near-vacuum candidate identity is invalid"
    }
    foreach ($probeKey in @{
        "case_time_domain" = "nondimensional_contract"; "grid_cells_x" = "128";
        "grid_cells_y" = "1"; "grid_cell_count" = "128"; "case_timestep" = "0.02";
        "case_step_count" = "32"; "positivity_preserved" = "true";
		"state_evolved" = "true"; "low_density_region_mass_increased" = "true";
		"numerical_correction_count" = "0";
        "density_floor_hits" = "0"; "pressure_floor_hits" = "0";
        "probe_passed" = "true"
    }.GetEnumerator()) {
        if ((Read-KeyValue -Text $text -Key $probeKey.Key) -ne $probeKey.Value) {
            throw "Rusanov near-vacuum contract drifted: $($probeKey.Key)"
        }
    }
    $maximumCfl = [double](Read-KeyValue -Text $text -Key "maximum_cfl")
    if ([double]::IsNaN($maximumCfl) -or [double]::IsInfinity($maximumCfl) -or
        $maximumCfl -le 0.0 -or $maximumCfl -gt 1.0) {
        throw "Rusanov near-vacuum CFL is outside the strict positivity contract"
    }
    $minimumDensity = [double](Read-KeyValue -Text $text -Key "minimum_density")
    $minimumPressure = [double](Read-KeyValue -Text $text -Key "minimum_pressure")
    if ([double]::IsNaN($minimumDensity) -or [double]::IsInfinity($minimumDensity) -or
        $minimumDensity -le 0.0 -or $minimumDensity -gt 1e-4) {
        throw "Rusanov near-vacuum density did not remain positive and near-vacuum"
    }
    if ([double]::IsNaN($minimumPressure) -or [double]::IsInfinity($minimumPressure) -or
        $minimumPressure -le 0.0 -or $minimumPressure -gt 1e-6) {
        throw "Rusanov near-vacuum pressure did not remain positive and near-vacuum"
    }
    $stateChangeL1 = [double](Read-KeyValue -Text $text -Key "state_change_l1")
    if ([double]::IsNaN($stateChangeL1) -or [double]::IsInfinity($stateChangeL1) -or
        $stateChangeL1 -le 1e-12) {
        throw "Rusanov near-vacuum probe did not produce a measurable expansion"
    }
	$initialLowDensityRegionMass = [double](Read-KeyValue -Text $text -Key "initial_low_density_region_mass")
	$finalLowDensityRegionMass = [double](Read-KeyValue -Text $text -Key "final_low_density_region_mass")
	if ([double]::IsNaN($initialLowDensityRegionMass) -or
		[double]::IsInfinity($initialLowDensityRegionMass) -or
		[double]::IsNaN($finalLowDensityRegionMass) -or
		[double]::IsInfinity($finalLowDensityRegionMass) -or
		$finalLowDensityRegionMass -le $initialLowDensityRegionMass) {
		throw "Rusanov near-vacuum expansion did not transfer mass into the low-density region"
	}
    foreach ($driftKey in @("mass_drift", "momentum_x_drift", "momentum_y_drift", "energy_drift")) {
        if ([Math]::Abs([double](Read-KeyValue -Text $text -Key $driftKey)) -gt 1e-10) {
            throw "Rusanov near-vacuum drift exceeds the periodic conservation tolerance: $driftKey"
        }
    }
    if ($isHllcProbe -and (Read-KeyValue -Text $text -Key "flux_fallback_count") -ne "0") {
        throw "HLLC near-vacuum probe unexpectedly used the Rusanov fallback"
    }
} elseif ($RunRusanovSodShockTube -or $RunHllcRusanovFallbackSodShockTube) {
    $isHllcProbe = $RunHllcRusanovFallbackSodShockTube
    $expectedCandidate = if ($isHllcProbe) { "fvm_hllc_rusanov_fallback" } else { "fvm_rusanov" }
    $benchmarkKind = if ($isHllcProbe) { "atmospherebench_hllc_rusanov_fallback_sod_shock_tube_probe" } else { "atmospherebench_rusanov_sod_shock_tube_probe" }
    $performanceGate = "not_evaluated_candidate_probe"
    $timingScope = if ($isHllcProbe) { "standalone_hllc_rusanov_fallback_sod_shock_tube_probe" } else { "standalone_rusanov_sod_shock_tube_probe" }
    $candidateImplementations = $expectedCandidate
    if ($solverResultStatus -ne "candidate_result_not_selection") {
        throw "Rusanov Sod probe must not claim solver selection"
    }
    if ((Read-KeyValue -Text $text -Key "candidate") -ne $expectedCandidate -or
        (Read-KeyValue -Text $text -Key "candidate_solver_implemented") -ne "true") {
        throw "Rusanov Sod candidate identity is invalid"
    }
    foreach ($probeKey in @{
        "case_time_domain" = "nondimensional_contract"; "boundary_mode" = "sealed";
		"eos_gamma" = "1.4"; "eos_specific_gas_constant" = "1";
		"initial_left_density" = "1"; "initial_left_pressure" = "1";
		"initial_right_density" = "0.125"; "initial_right_pressure" = "0.1";
        "grid_cells_x" = "256"; "grid_cells_y" = "1"; "grid_cell_count" = "256";
        "cell_length" = "0.00390625"; "case_timestep" = "0.0005";
        "case_step_count" = "400"; "simulated_time" = "0.2";
        "positivity_preserved" = "true"; "state_evolved" = "true";
        "density_bounds_preserved" = "true"; "shock_reference_passed" = "true";
        "boundary_ledger_closes" = "true"; "numerical_correction_count" = "0";
        "density_floor_hits" = "0"; "pressure_floor_hits" = "0";
        "probe_passed" = "true"
    }.GetEnumerator()) {
        if ((Read-KeyValue -Text $text -Key $probeKey.Key) -ne $probeKey.Value) {
            throw "Rusanov Sod contract drifted: $($probeKey.Key)"
        }
    }
    $maximumCfl = [double](Read-KeyValue -Text $text -Key "maximum_cfl")
    if ([double]::IsNaN($maximumCfl) -or [double]::IsInfinity($maximumCfl) -or
        $maximumCfl -le 0.0 -or $maximumCfl -gt 1.0) {
        throw "Rusanov Sod CFL is outside the strict positivity contract"
    }
    $shockPosition = [double](Read-KeyValue -Text $text -Key "shock_position")
    $maximumVelocityX = [double](Read-KeyValue -Text $text -Key "maximum_velocity_x")
    if ([double]::IsNaN($shockPosition) -or [double]::IsInfinity($shockPosition) -or
        $shockPosition -lt 0.8 -or $shockPosition -gt 0.9) {
        throw "Rusanov Sod shock front is outside the published t=0.2 window"
    }
    if ([double]::IsNaN($maximumVelocityX) -or [double]::IsInfinity($maximumVelocityX) -or
        $maximumVelocityX -lt 0.5 -or $maximumVelocityX -gt 1.2) {
        throw "Rusanov Sod velocity is outside the published first-order window"
    }
    foreach ($balanceKey in @(
        "mass_balance_error", "momentum_x_balance_error", "momentum_y_balance_error",
        "energy_balance_error"
    )) {
        if ([Math]::Abs([double](Read-KeyValue -Text $text -Key $balanceKey)) -gt 1e-9) {
            throw "Rusanov Sod boundary-adjusted ledger exceeds tolerance: $balanceKey"
        }
    }
    foreach ($zeroExchangeKey in @("boundary_mass_exchange", "boundary_momentum_y_exchange", "boundary_energy_exchange")) {
        if ([Math]::Abs([double](Read-KeyValue -Text $text -Key $zeroExchangeKey)) -gt 1e-12) {
            throw "Rusanov Sod sealed boundary exchanged an invalid quantity: $zeroExchangeKey"
        }
    }
    $boundaryMomentumXExchange = [double](Read-KeyValue -Text $text -Key "boundary_momentum_x_exchange")
    if ([double]::IsNaN($boundaryMomentumXExchange) -or
        [double]::IsInfinity($boundaryMomentumXExchange) -or
        $boundaryMomentumXExchange -le 0.0) {
        throw "Rusanov Sod wall-pressure impulse was not recorded"
    }
    if ($isHllcProbe -and (Read-KeyValue -Text $text -Key "flux_fallback_count") -ne "0") {
        throw "HLLC Sod probe unexpectedly used the Rusanov fallback"
    }
} elseif ($RunRusanovDensityAdvectionRefinement) {
    $benchmarkKind = "atmospherebench_rusanov_density_advection_refinement_probe"
    $performanceGate = "not_evaluated_candidate_probe"
    $timingScope = "standalone_rusanov_density_advection_refinement_probe"
    $candidateImplementations = "fvm_rusanov"
    if ($solverResultStatus -ne "candidate_result_not_selection") {
        throw "Rusanov refinement probe must not claim solver selection"
    }
    foreach ($probeKey in @{
        "case_time_domain" = "nondimensional_contract"; "boundary_mode" = "periodic";
        "refinement_levels" = "3"; "total_simulated_time" = "0.25";
        "reference_velocity" = "0.5"; "coarse_cells" = "64";
        "medium_cells" = "128"; "fine_cells" = "256";
        "coarse_steps" = "64"; "medium_steps" = "128"; "fine_steps" = "256";
        "coarse_reference_shift_cells" = "8"; "medium_reference_shift_cells" = "16";
        "fine_reference_shift_cells" = "32"; "positivity_preserved" = "true";
        "state_evolved" = "true"; "refinement_passed" = "true";
        "numerical_correction_count" = "0"; "probe_passed" = "true"
    }.GetEnumerator()) {
        if ((Read-KeyValue -Text $text -Key $probeKey.Key) -ne $probeKey.Value) {
            throw "Rusanov refinement contract drifted: $($probeKey.Key)"
        }
    }
    $coarseL1 = [double](Read-KeyValue -Text $text -Key "coarse_density_l1_error")
    $mediumL1 = [double](Read-KeyValue -Text $text -Key "medium_density_l1_error")
    $fineL1 = [double](Read-KeyValue -Text $text -Key "fine_density_l1_error")
    if ($coarseL1 -le $mediumL1 -or $mediumL1 -le $fineL1 -or $fineL1 -le 0.0) {
        throw "Rusanov refinement L1 error did not decrease monotonically"
    }
    foreach ($orderKey in @("coarse_to_medium_l1_order", "medium_to_fine_l1_order")) {
        $order = [double](Read-KeyValue -Text $text -Key $orderKey)
        if ([double]::IsNaN($order) -or [double]::IsInfinity($order) -or
            $order -lt 0.8 -or $order -gt 1.2) {
            throw "Rusanov refinement observed order is outside the published first-order window: $orderKey"
        }
    }
    foreach ($pressureKey in @("coarse_pressure_linf_error", "medium_pressure_linf_error", "fine_pressure_linf_error")) {
        if ([Math]::Abs([double](Read-KeyValue -Text $text -Key $pressureKey)) -gt 1e-10) {
            throw "Rusanov refinement pressure preservation exceeded tolerance: $pressureKey"
        }
    }
    foreach ($cflKey in @("coarse_maximum_cfl", "medium_maximum_cfl", "fine_maximum_cfl")) {
        $cfl = [double](Read-KeyValue -Text $text -Key $cflKey)
        if ([double]::IsNaN($cfl) -or [double]::IsInfinity($cfl) -or $cfl -le 0.0 -or $cfl -gt 1.0) {
            throw "Rusanov refinement CFL is outside the strict positivity contract: $cflKey"
        }
    }
    foreach ($driftKey in @(
        "coarse_mass_drift", "medium_mass_drift", "fine_mass_drift",
        "coarse_energy_drift", "medium_energy_drift", "fine_energy_drift"
    )) {
        if ([Math]::Abs([double](Read-KeyValue -Text $text -Key $driftKey)) -gt 1e-9) {
            throw "Rusanov refinement drift exceeds tolerance: $driftKey"
        }
    }
} elseif ($RunRusanovLowMachAdvection -or $RunHllcRusanovFallbackLowMachAdvection) {
    $isHllcProbe = $RunHllcRusanovFallbackLowMachAdvection
    $expectedCandidate = if ($isHllcProbe) { "fvm_hllc_rusanov_fallback" } else { "fvm_rusanov" }
    $benchmarkKind = if ($isHllcProbe) { "atmospherebench_hllc_rusanov_fallback_low_mach_advection_probe" } else { "atmospherebench_rusanov_low_mach_advection_probe" }
    $performanceGate = "not_evaluated_candidate_probe"
    $timingScope = if ($isHllcProbe) { "standalone_hllc_rusanov_fallback_low_mach_advection_probe" } else { "standalone_rusanov_low_mach_advection_probe" }
    $candidateImplementations = $expectedCandidate
    if ($solverResultStatus -ne "candidate_result_not_selection") {
        throw "Rusanov low-Mach probe must not claim solver selection"
    }
    if ((Read-KeyValue -Text $text -Key "candidate") -ne $expectedCandidate -or
        (Read-KeyValue -Text $text -Key "candidate_solver_implemented") -ne "true") {
        throw "Low-Mach probe candidate identity is invalid"
    }
    foreach ($probeKey in @{
        "case_time_domain" = "nondimensional_contract"; "boundary_mode" = "periodic";
        "grid_cells_x" = "128"; "grid_cells_y" = "1"; "grid_cell_count" = "128";
        "reference_shift_distance" = "0.125"; "moderate_velocity" = "0.5";
        "low_velocity" = "0.05"; "very_low_velocity" = "0.005";
        "moderate_steps" = "139"; "low_steps" = "1062"; "very_low_steps" = "10300";
        "positivity_preserved" = "true"; "state_evolved" = "true";
        "numerical_correction_count" = "0"; "probe_passed" = "true"
    }.GetEnumerator()) {
        if ((Read-KeyValue -Text $text -Key $probeKey.Key) -ne $probeKey.Value) {
            throw "Rusanov low-Mach contract drifted: $($probeKey.Key)"
        }
    }
    foreach ($metricKey in @(
        "moderate_nominal_mach", "low_nominal_mach", "very_low_nominal_mach",
        "moderate_density_l1_error", "low_density_l1_error", "very_low_density_l1_error",
        "moderate_total_variation_ratio", "low_total_variation_ratio", "very_low_total_variation_ratio",
        "low_to_moderate_l1_ratio", "very_low_to_moderate_l1_ratio"
    )) {
        $metric = [double](Read-KeyValue -Text $text -Key $metricKey)
        if ([double]::IsNaN($metric) -or [double]::IsInfinity($metric)) {
            throw "Rusanov low-Mach metric is non-finite: $metricKey"
        }
    }
    if ([double](Read-KeyValue -Text $text -Key "very_low_density_l1_error") -le
        [double](Read-KeyValue -Text $text -Key "moderate_density_l1_error")) {
        throw "Rusanov low-Mach characterization did not expose increasing diffusion"
    }
    foreach ($cflKey in @("moderate_maximum_cfl", "low_maximum_cfl", "very_low_maximum_cfl")) {
        $cfl = [double](Read-KeyValue -Text $text -Key $cflKey)
        if ([double]::IsNaN($cfl) -or [double]::IsInfinity($cfl) -or $cfl -le 0.0 -or $cfl -gt 1.0) {
            throw "Rusanov low-Mach CFL is outside the strict positivity contract: $cflKey"
        }
    }
    if ($isHllcProbe) {
        if ((Read-KeyValue -Text $text -Key "low_mach_suitability_passed") -ne "true") {
            throw "HLLC low-Mach suitability gate did not pass"
        }
        foreach ($fallbackKey in @("moderate_flux_fallback_count", "low_flux_fallback_count", "very_low_flux_fallback_count")) {
            if ((Read-KeyValue -Text $text -Key $fallbackKey) -ne "0") {
                throw "HLLC low-Mach probe unexpectedly used the Rusanov fallback: $fallbackKey"
            }
        }
    }
} elseif ($RunAllSpeedRusanovLowMachAdvection) {
    $benchmarkKind = "atmospherebench_all_speed_rusanov_low_mach_advection_probe"
    $performanceGate = "not_evaluated_candidate_probe"
    $timingScope = "standalone_all_speed_rusanov_low_mach_advection_probe"
    $candidateImplementations = "fvm_all_speed_rusanov"
    if ($solverResultStatus -ne "candidate_result_not_selection") {
        throw "All-speed Rusanov low-Mach probe must not claim solver selection"
    }
    if ((Read-KeyValue -Text $text -Key "candidate") -ne "fvm_all_speed_rusanov" -or
        (Read-KeyValue -Text $text -Key "candidate_solver_implemented") -ne "true") {
        throw "All-speed Rusanov probe candidate identity is invalid"
    }
    foreach ($probeKey in @{
        "case_time_domain" = "nondimensional_contract"; "boundary_mode" = "periodic";
        "grid_cells_x" = "128"; "grid_cells_y" = "1"; "grid_cell_count" = "128";
        "reference_shift_distance" = "0.125"; "moderate_velocity" = "0.5";
        "low_velocity" = "0.05"; "very_low_velocity" = "0.005";
        "positivity_preserved" = "true"; "state_evolved" = "true";
        "numerical_correction_count" = "0"; "probe_passed" = "true";
        "benchmark_execution_status" = "PASS"; "candidate_disposition" = "reject_low_mach_suitability";
        "low_mach_suitability_passed" = "false"
    }.GetEnumerator()) {
        if ((Read-KeyValue -Text $text -Key $probeKey.Key) -ne $probeKey.Value) {
            throw "All-speed Rusanov low-Mach contract drifted: $($probeKey.Key)"
        }
    }
    foreach ($metricKey in @(
        "moderate_nominal_mach", "low_nominal_mach", "very_low_nominal_mach",
        "moderate_density_l1_error", "low_density_l1_error", "very_low_density_l1_error",
        "moderate_total_variation_ratio", "low_total_variation_ratio", "very_low_total_variation_ratio",
        "low_to_moderate_l1_ratio", "very_low_to_moderate_l1_ratio"
    )) {
        $metric = [double](Read-KeyValue -Text $text -Key $metricKey)
        if ([double]::IsNaN($metric) -or [double]::IsInfinity($metric)) {
            throw "All-speed Rusanov low-Mach metric is non-finite: $metricKey"
        }
    }
    if ([double](Read-KeyValue -Text $text -Key "very_low_density_l1_error") -le 0.05) {
        throw "All-speed Rusanov low-Mach negative result unexpectedly meets the L1 threshold"
    }
    foreach ($cflKey in @("moderate_maximum_cfl", "low_maximum_cfl", "very_low_maximum_cfl")) {
        $cfl = [double](Read-KeyValue -Text $text -Key $cflKey)
        if ([double]::IsNaN($cfl) -or [double]::IsInfinity($cfl) -or $cfl -le 0.0 -or $cfl -gt 1.0) {
            throw "All-speed Rusanov low-Mach CFL is outside the strict positivity contract: $cflKey"
        }
    }
} elseif ($RunRusanovOpenBoundaryLeak -or $RunHllcRusanovFallbackOpenBoundaryLeak) {
    $isHllcProbe = $RunHllcRusanovFallbackOpenBoundaryLeak
    $expectedCandidate = if ($isHllcProbe) { "fvm_hllc_rusanov_fallback" } else { "fvm_rusanov" }
    $benchmarkKind = if ($isHllcProbe) { "atmospherebench_hllc_rusanov_fallback_open_boundary_leak_probe" } else { "atmospherebench_rusanov_open_boundary_leak_probe" }
    $performanceGate = "not_evaluated_candidate_probe"
    $timingScope = if ($isHllcProbe) { "standalone_hllc_rusanov_fallback_open_boundary_leak_probe" } else { "standalone_rusanov_open_boundary_leak_probe" }
    $candidateImplementations = $expectedCandidate
    if ($solverResultStatus -ne "candidate_result_not_selection") {
        throw "Rusanov open-boundary leak probe must not claim solver selection"
    }
    if ((Read-KeyValue -Text $text -Key "candidate") -ne $expectedCandidate -or
        (Read-KeyValue -Text $text -Key "candidate_solver_implemented") -ne "true") {
        throw "Open-boundary leak candidate identity is invalid"
    }
    foreach ($probeKey in @{
        "case_time_domain" = "nondimensional_contract";
        "boundary_mode" = "sealed_left_open_right_reservoir";
        "grid_cells_x" = "128"; "grid_cells_y" = "1"; "grid_cell_count" = "128";
        "cell_length" = "0.0078125"; "case_timestep" = "0.001";
        "case_step_count" = "120"; "positivity_preserved" = "true";
        "state_evolved" = "true"; "boundary_ledger_closes" = "true";
        "mass_decreased" = "true"; "numerical_correction_count" = "0";
        "probe_passed" = "true"
    }.GetEnumerator()) {
        if ((Read-KeyValue -Text $text -Key $probeKey.Key) -ne $probeKey.Value) {
            throw "Rusanov open-boundary leak contract drifted: $($probeKey.Key)"
        }
    }
    $rightBoundaryMassOut = [double](Read-KeyValue -Text $text -Key "right_boundary_mass_out")
    if ([double]::IsNaN($rightBoundaryMassOut) -or [double]::IsInfinity($rightBoundaryMassOut) -or
        $rightBoundaryMassOut -le 0.0) {
        throw "Rusanov open-boundary leak did not record positive mass outflow"
    }
    if ([Math]::Abs([double](Read-KeyValue -Text $text -Key "left_boundary_mass_exchange")) -gt 1e-12) {
        throw "Rusanov open-boundary leak crossed the sealed left wall"
    }
    foreach ($balanceKey in @("mass_balance_error", "momentum_x_balance_error", "momentum_y_balance_error", "energy_balance_error")) {
        if ([Math]::Abs([double](Read-KeyValue -Text $text -Key $balanceKey)) -gt 1e-9) {
            throw "Rusanov open-boundary leak ledger exceeds tolerance: $balanceKey"
        }
    }
    $maximumCfl = [double](Read-KeyValue -Text $text -Key "maximum_cfl")
    if ([double]::IsNaN($maximumCfl) -or [double]::IsInfinity($maximumCfl) -or
        $maximumCfl -le 0.0 -or $maximumCfl -gt 1.0) {
        throw "Rusanov open-boundary leak CFL is outside the strict positivity contract"
    }
    if ($isHllcProbe -and (Read-KeyValue -Text $text -Key "flux_fallback_count") -ne "0") {
        throw "HLLC open-boundary leak unexpectedly used the Rusanov fallback"
    }
} else {
    $isHllcProbe = $RunHllcRusanovFallbackPerformance
    $expectedCandidate = if ($isHllcProbe) { "fvm_hllc_rusanov_fallback" } else { "fvm_rusanov" }
    $benchmarkKind = if ($isHllcProbe) { "atmospherebench_hllc_rusanov_fallback_performance_probe" } else { "atmospherebench_rusanov_performance_probe" }
    $performanceGate = "recorded_candidate_measurement_no_budget"
    $timingScope = if ($isHllcProbe) { "standalone_hllc_rusanov_fallback_performance_probe_wrapper" } else { "standalone_rusanov_performance_probe_wrapper" }
    $candidateImplementations = $expectedCandidate
    if ($solverResultStatus -ne "candidate_result_not_selection") {
        throw "Rusanov performance probe must not claim solver selection"
    }
    if ((Read-KeyValue -Text $text -Key "candidate") -ne $expectedCandidate -or
        (Read-KeyValue -Text $text -Key "candidate_solver_implemented") -ne "true") {
        throw "Performance probe candidate identity is invalid"
    }
    foreach ($probeKey in @{
        "case_time_domain" = "nondimensional_contract"; "boundary_mode" = "periodic";
        "grid_cells_x" = "58752"; "grid_cells_y" = "1"; "grid_cell_count" = "58752";
        "case_step_count" = "64"; "performance_warmup_count" = "1";
        "performance_repeat_count" = "3"; "small_cells" = "14688";
        "medium_cells" = "29376"; "large_cells" = "58752";
        "performance_steps" = "64"; "positivity_preserved" = "true";
        "state_evolved" = "true"; "numerical_correction_count" = "0";
        "performance_measurement_passed" = "true"; "probe_passed" = "true"
    }.GetEnumerator()) {
        if ((Read-KeyValue -Text $text -Key $probeKey.Key) -ne $probeKey.Value) {
            throw "Rusanov performance contract drifted: $($probeKey.Key)"
        }
    }
    foreach ($metricKey in @(
        "small_elapsed_milliseconds", "medium_elapsed_milliseconds", "large_elapsed_milliseconds",
        "small_cell_updates_per_second", "medium_cell_updates_per_second", "large_cell_updates_per_second"
    )) {
        $metric = [double](Read-KeyValue -Text $text -Key $metricKey)
        if ([double]::IsNaN($metric) -or [double]::IsInfinity($metric) -or $metric -le 0.0) {
            throw "Rusanov performance metric is invalid: $metricKey"
        }
    }
    foreach ($driftKey in @("mass_drift", "momentum_x_drift", "momentum_y_drift", "energy_drift")) {
        if ([Math]::Abs([double](Read-KeyValue -Text $text -Key $driftKey)) -gt 1e-7) {
            throw "Rusanov performance run drift exceeds tolerance: $driftKey"
        }
    }
    if ($isHllcProbe) {
        foreach ($fallbackKey in @("small_flux_fallback_count", "medium_flux_fallback_count", "large_flux_fallback_count")) {
            if ((Read-KeyValue -Text $text -Key $fallbackKey) -ne "0") {
                throw "HLLC performance probe unexpectedly used the Rusanov fallback: $fallbackKey"
            }
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
$maximumDensity = $null
$statePressure = $null
$stateAndFluxScratchBytesPerCell = $null
$stateAndFluxScratchBytesTotal = $null
$initialMaximumPressure = $null
$finalMaximumPressure = $null
$stateChangeL1 = $null
$stateEvolved = $null
$pressurePeakReduced = $null
$pressureIncreased = $null
$temperatureIncreased = $null
$initialMeanPressure = $null
$finalMeanPressure = $null
$initialMeanTemperature = $null
$finalMeanTemperature = $null
$expectedFinalMeanPressure = $null
$expectedFinalMeanTemperature = $null
$sourceLedgerCloses = $null
$sourceAndBoundaryLedgerCloses = $null
$sourceMassNet = $null
$sourceMomentumXNet = $null
$sourceMomentumYNet = $null
$sourceEnergyNet = $null
$sourceEventCount = $null
$sourceMassBalanceError = $null
$sourceMomentumXBalanceError = $null
$sourceMomentumYBalanceError = $null
$sourceEnergyBalanceError = $null
$boundaryMassNet = $null
$boundaryMomentumXNet = $null
$boundaryMomentumYNet = $null
$boundaryEnergyNet = $null
$boundaryEventCount = $null
$combinedMassBalanceError = $null
$combinedMomentumXBalanceError = $null
$combinedMomentumYBalanceError = $null
$combinedEnergyBalanceError = $null
$naturalConvection = $null
$speciesMixing = $null
$speciesModel = $null
$speciesEosCoupling = $null
$physicalDiffusion = $null
$initialSpeciesAMass = $null
$finalSpeciesAMass = $null
$initialSpeciesBMass = $null
$finalSpeciesBMass = $null
$speciesAMassDrift = $null
$speciesBMassDrift = $null
$minimumSpeciesAFraction = $null
$maximumSpeciesAFraction = $null
$initialCompositionTotalVariation = $null
$finalCompositionTotalVariation = $null
$compositionStateChangeL1 = $null
$initialMixedCellCount = $null
$finalMixedCellCount = $null
$gasLedgerCloses = $null
$speciesLedgerCloses = $null
$speciesBoundsPreserved = $null
$compositionEvolved = $null
$mixedRegionFormed = $null
$densityL1Error = $null
$densityLinfError = $null
$pressureLinfError = $null
$totalVariationRatio = $null
$referenceVelocity = $null
$referenceShiftCells = $null
$advectionReferencePassed = $null
$densityBoundsPreserved = $null
$initialLowDensityRegionMass = $null
$finalLowDensityRegionMass = $null
$lowDensityRegionMassIncreased = $null
$boundaryMode = $null
$cellLength = $null
$simulatedTime = $null
$shockPosition = $null
$minimumVelocityX = $null
$maximumVelocityX = $null
$shockReferencePassed = $null
$boundaryLedgerCloses = $null
$boundaryMassExchange = $null
$boundaryMomentumXExchange = $null
$boundaryMomentumYExchange = $null
$boundaryEnergyExchange = $null
$leftBoundaryMassExchange = $null
$leftBoundaryMomentumXExchange = $null
$leftBoundaryMomentumYExchange = $null
$leftBoundaryEnergyExchange = $null
$rightBoundaryMassExchange = $null
$rightBoundaryMomentumXExchange = $null
$rightBoundaryMomentumYExchange = $null
$rightBoundaryEnergyExchange = $null
$massDecreased = $null
$rightBoundaryMassOut = $null
$massBalanceError = $null
$momentumXBalanceError = $null
$momentumYBalanceError = $null
$energyBalanceError = $null
$eosGamma = $null
$eosSpecificGasConstant = $null
$initialLeftDensity = $null
$initialLeftPressure = $null
$initialRightDensity = $null
$initialRightPressure = $null
$refinementLevels = $null
$totalSimulatedTime = $null
$coarseCells = $null
$mediumCells = $null
$fineCells = $null
$coarseDensityL1Error = $null
$mediumDensityL1Error = $null
$fineDensityL1Error = $null
$coarseToMediumL1Order = $null
$mediumToFineL1Order = $null
$coarseMaximumCfl = $null
$mediumMaximumCfl = $null
$fineMaximumCfl = $null
$moderateNominalMach = $null
$lowNominalMach = $null
$veryLowNominalMach = $null
$moderateDensityL1Error = $null
$lowDensityL1Error = $null
$veryLowDensityL1Error = $null
$moderateTotalVariationRatio = $null
$lowTotalVariationRatio = $null
$veryLowTotalVariationRatio = $null
$lowToModerateL1Ratio = $null
$veryLowToModerateL1Ratio = $null
$lowMachSuitabilityPassed = $null
$performanceWarmupCount = $null
$performanceRepeatCount = $null
$performanceSmallCells = $null
$performanceMediumCells = $null
$performanceLargeCells = $null
$performanceSteps = $null
$smallElapsedMilliseconds = $null
$mediumElapsedMilliseconds = $null
$largeElapsedMilliseconds = $null
$smallCellUpdatesPerSecond = $null
$mediumCellUpdatesPerSecond = $null
$largeCellUpdatesPerSecond = $null
$hllc2DPerformance = $null
$physicalTimePolicy = $null
$performanceBudgetStatus = $null
$referenceFrameBudgetMilliseconds = $null
$legacyGridCellsX = $null
$legacyGridCellsY = $null
$legacyGridCellCount = $null
$doubledGridCellsX = $null
$doubledGridCellsY = $null
$doubledGridCellCount = $null
$particleGridCellsX = $null
$particleGridCellsY = $null
$particleGridCellCount = $null
$legacyGridElapsedMilliseconds = $null
$doubledGridElapsedMilliseconds = $null
$particleGridElapsedMilliseconds = $null
$legacyGridMillisecondsPerStep = $null
$doubledGridMillisecondsPerStep = $null
$particleGridMillisecondsPerStep = $null
$legacyGridCellUpdatesPerSecond = $null
$doubledGridCellUpdatesPerSecond = $null
$particleGridCellUpdatesPerSecond = $null
$legacyGridFractionOfReferenceFrame = $null
$doubledGridFractionOfReferenceFrame = $null
$particleGridFractionOfReferenceFrame = $null
$lbmD2Q9 = $null
$lbmEnergyState = $null
$lbmEnergyConservation = $null
$lbmNearVacuumSupport = $null
$lbmShockSupport = $null
$lbmSpeciesSupport = $null
$legacyLike = $null
$hybridPolicy = $null
if ($isRusanovProbe) {
	$stateDensity = if ($isLegacyLikeProbe) {
		$null
	} else {
		[double](Read-KeyValue -Text $text -Key "minimum_density")
	}
    $statePressure = [double](Read-KeyValue -Text $text -Key "minimum_pressure")
    $stateAndFluxScratchBytesPerCell = [double](Read-KeyValue -Text $text -Key "state_and_flux_scratch_bytes_per_cell")
	if ($isHybridPolicyProbe) {
		$boundaryMode = Read-KeyValue -Text $text -Key "boundary_mode"
		$cellLength = [double](Read-KeyValue -Text $text -Key "cell_length")
		$stateAndFluxScratchBytesTotal = [int64](Read-KeyValue -Text $text -Key "state_and_flux_scratch_bytes_total")
		$moderateDensityL1 = [double](Read-KeyValue -Text $text -Key "moderate_density_l1_error")
		$lowDensityL1 = [double](Read-KeyValue -Text $text -Key "low_density_l1_error")
		$veryLowDensityL1 = [double](Read-KeyValue -Text $text -Key "very_low_density_l1_error")
		$moderateTv = [double](Read-KeyValue -Text $text -Key "moderate_total_variation_ratio")
		$lowTv = [double](Read-KeyValue -Text $text -Key "low_total_variation_ratio")
		$veryLowTv = [double](Read-KeyValue -Text $text -Key "very_low_total_variation_ratio")
		$hybridPolicy = [ordered]@{
			low_mach_bulk_route = Read-KeyValue -Text $text -Key "low_mach_bulk_route"
			compressible_event_route = Read-KeyValue -Text $text -Key "compressible_event_route"
			cross_route_boundary_coupling = Read-KeyValue -Text $text -Key "cross_route_boundary_coupling"
			event_local_subcycling = Read-KeyValue -Text $text -Key "event_local_subcycling"
			production_boundary_coupling = Read-KeyValue -Text $text -Key "production_boundary_coupling"
			low_mach_route_count = [int](Read-KeyValue -Text $text -Key "low_mach_route_count")
			compressible_route_count = [int](Read-KeyValue -Text $text -Key "compressible_route_count")
			low_mach_suitability_passed = (Read-KeyValue -Text $text -Key "low_mach_suitability_passed") -eq "true"
			compressible_sod_passed = (Read-KeyValue -Text $text -Key "compressible_sod_passed") -eq "true"
			compressible_sod_shock_reference_passed = (Read-KeyValue -Text $text -Key "compressible_sod_shock_reference_passed") -eq "true"
			compressible_sod_flux_fallback_count = [int](Read-KeyValue -Text $text -Key "compressible_sod_flux_fallback_count")
			compressible_sod_correction_count = [int](Read-KeyValue -Text $text -Key "compressible_sod_correction_count")
			moderate_density_l1_error = $moderateDensityL1
			low_density_l1_error = $lowDensityL1
			very_low_density_l1_error = $veryLowDensityL1
			moderate_total_variation_ratio = $moderateTv
			low_total_variation_ratio = $lowTv
			very_low_total_variation_ratio = $veryLowTv
			policy_selection_ready = (Read-KeyValue -Text $text -Key "policy_selection_ready") -eq "true"
			candidate_disposition = Read-KeyValue -Text $text -Key "candidate_disposition"
		}
	} elseif ($isLegacyLikeProbe) {
		$boundaryMode = Read-KeyValue -Text $text -Key "boundary_mode"
		$cellLength = [double](Read-KeyValue -Text $text -Key "cell_length")
		$stateAndFluxScratchBytesTotal = [int64](Read-KeyValue -Text $text -Key "state_and_flux_scratch_bytes_total")
		$initialMaximumPressure = [double](Read-KeyValue -Text $text -Key "initial_maximum_pressure")
		$finalMaximumPressure = [double](Read-KeyValue -Text $text -Key "final_maximum_pressure")
		$stateChangeL1 = [double](Read-KeyValue -Text $text -Key "state_change_l1")
		$stateEvolved = (Read-KeyValue -Text $text -Key "state_evolved") -eq "true"
		$pressurePeakReduced = (Read-KeyValue -Text $text -Key "pressure_peak_reduced") -eq "true"
		$legacyLike = [ordered]@{
			model = Read-KeyValue -Text $text -Key "control_model"
			probe_kind = Read-KeyValue -Text $text -Key "probe_kind"
			production_air_equivalence = Read-KeyValue -Text $text -Key "production_air_equivalence"
			physical_mass_state = Read-KeyValue -Text $text -Key "physical_mass_state"
			physical_density_state = Read-KeyValue -Text $text -Key "physical_density_state"
			physical_momentum_state = Read-KeyValue -Text $text -Key "physical_momentum_state"
			physical_energy_state = Read-KeyValue -Text $text -Key "physical_energy_state"
			species_state = Read-KeyValue -Text $text -Key "species_state"
			mass_conservation = Read-KeyValue -Text $text -Key "mass_conservation"
			momentum_conservation = Read-KeyValue -Text $text -Key "momentum_conservation"
			energy_conservation = Read-KeyValue -Text $text -Key "energy_conservation"
			maximum_cfl = Read-KeyValue -Text $text -Key "maximum_cfl"
			initial_pressure_sum = [double](Read-KeyValue -Text $text -Key "initial_pressure_sum")
			final_pressure_sum = [double](Read-KeyValue -Text $text -Key "final_pressure_sum")
			pressure_sum_drift = [double](Read-KeyValue -Text $text -Key "pressure_sum_drift")
			maximum_absolute_velocity = [double](Read-KeyValue -Text $text -Key "maximum_absolute_velocity")
			uniform_preserved = (Read-KeyValue -Text $text -Key "uniform_preserved") -eq "true"
			pressure_peak_reduced = $pressurePeakReduced
			pressure_sum_preserved = (Read-KeyValue -Text $text -Key "pressure_sum_preserved") -eq "true"
			finite_state = (Read-KeyValue -Text $text -Key "finite_state") -eq "true"
		}
	} elseif ($isLbmProbe) {
		$boundaryMode = Read-KeyValue -Text $text -Key "boundary_mode"
		$cellLength = [double](Read-KeyValue -Text $text -Key "cell_length")
		$maximumDensity = [double](Read-KeyValue -Text $text -Key "maximum_density")
		$stateAndFluxScratchBytesTotal = [int64](Read-KeyValue -Text $text -Key "state_and_flux_scratch_bytes_total")
		$stateChangeL1 = [double](Read-KeyValue -Text $text -Key "state_change_l1")
		$stateEvolved = (Read-KeyValue -Text $text -Key "state_evolved") -eq "true"
		$lbmEnergyState = Read-KeyValue -Text $text -Key "energy_state"
		$lbmEnergyConservation = Read-KeyValue -Text $text -Key "energy_conservation"
		$lbmNearVacuumSupport = Read-KeyValue -Text $text -Key "near_vacuum_support"
		$lbmShockSupport = Read-KeyValue -Text $text -Key "shock_support"
		$lbmSpeciesSupport = Read-KeyValue -Text $text -Key "species_support"
		$lbmD2Q9 = [ordered]@{
			model = Read-KeyValue -Text $text -Key "lbm_model"
			probe_kind = Read-KeyValue -Text $text -Key "probe_kind"
			relaxation_time = [double](Read-KeyValue -Text $text -Key "relaxation_time")
			kinematic_viscosity = [double](Read-KeyValue -Text $text -Key "kinematic_viscosity")
			maximum_cfl = Read-KeyValue -Text $text -Key "maximum_cfl"
			maximum_mach = [double](Read-KeyValue -Text $text -Key "maximum_mach")
			minimum_population = [double](Read-KeyValue -Text $text -Key "minimum_population")
			energy_state = $lbmEnergyState
			energy_conservation = $lbmEnergyConservation
			near_vacuum_support = $lbmNearVacuumSupport
			shock_support = $lbmShockSupport
			species_support = $lbmSpeciesSupport
			initial_mass = [double](Read-KeyValue -Text $text -Key "initial_mass")
			final_mass = [double](Read-KeyValue -Text $text -Key "final_mass")
			mass_conserved = (Read-KeyValue -Text $text -Key "mass_conserved") -eq "true"
			momentum_conserved = (Read-KeyValue -Text $text -Key "momentum_conserved") -eq "true"
			positivity_preserved = (Read-KeyValue -Text $text -Key "positivity_preserved") -eq "true"
			uniform_preserved = (Read-KeyValue -Text $text -Key "uniform_preserved") -eq "true"
			initial_shear_amplitude = [double](Read-KeyValue -Text $text -Key "initial_shear_amplitude")
			final_shear_amplitude = [double](Read-KeyValue -Text $text -Key "final_shear_amplitude")
			expected_shear_amplitude = [double](Read-KeyValue -Text $text -Key "expected_shear_amplitude")
			shear_amplitude_relative_error = [double](Read-KeyValue -Text $text -Key "shear_amplitude_relative_error")
			shear_reference_passed = (Read-KeyValue -Text $text -Key "shear_reference_passed") -eq "true"
		}
	} elseif ($RunHllc2DSpeciesMixing) {
		$boundaryMode = Read-KeyValue -Text $text -Key "boundary_mode"
		$cellLength = [double](Read-KeyValue -Text $text -Key "cell_length")
		$stateAndFluxScratchBytesTotal = [int64](Read-KeyValue -Text $text -Key "state_and_flux_scratch_bytes_total")
		$speciesModel = Read-KeyValue -Text $text -Key "species_model"
		$speciesEosCoupling = Read-KeyValue -Text $text -Key "species_eos_coupling"
		$physicalDiffusion = Read-KeyValue -Text $text -Key "physical_diffusion"
		$initialSpeciesAMass = [double](Read-KeyValue -Text $text -Key "initial_species_a_mass")
		$finalSpeciesAMass = [double](Read-KeyValue -Text $text -Key "final_species_a_mass")
		$initialSpeciesBMass = [double](Read-KeyValue -Text $text -Key "initial_species_b_mass")
		$finalSpeciesBMass = [double](Read-KeyValue -Text $text -Key "final_species_b_mass")
		$speciesAMassDrift = [double](Read-KeyValue -Text $text -Key "species_a_mass_drift")
		$speciesBMassDrift = [double](Read-KeyValue -Text $text -Key "species_b_mass_drift")
		$minimumSpeciesAFraction = [double](Read-KeyValue -Text $text -Key "minimum_species_a_fraction")
		$maximumSpeciesAFraction = [double](Read-KeyValue -Text $text -Key "maximum_species_a_fraction")
		$initialCompositionTotalVariation = [double](Read-KeyValue -Text $text -Key "initial_composition_total_variation")
		$finalCompositionTotalVariation = [double](Read-KeyValue -Text $text -Key "final_composition_total_variation")
		$compositionStateChangeL1 = [double](Read-KeyValue -Text $text -Key "composition_state_change_l1")
		$initialMixedCellCount = [int](Read-KeyValue -Text $text -Key "initial_mixed_cell_count")
		$finalMixedCellCount = [int](Read-KeyValue -Text $text -Key "final_mixed_cell_count")
		$gasLedgerCloses = (Read-KeyValue -Text $text -Key "gas_ledger_closes") -eq "true"
		$speciesLedgerCloses = (Read-KeyValue -Text $text -Key "species_ledger_closes") -eq "true"
		$speciesBoundsPreserved = (Read-KeyValue -Text $text -Key "species_bounds_preserved") -eq "true"
		$compositionEvolved = (Read-KeyValue -Text $text -Key "composition_evolved") -eq "true"
		$mixedRegionFormed = (Read-KeyValue -Text $text -Key "mixed_region_formed") -eq "true"
		$speciesMixing = [ordered]@{
			model = $speciesModel
			eos_coupling = $speciesEosCoupling
			physical_diffusion = $physicalDiffusion
			initial_species_a_mass = $initialSpeciesAMass
			final_species_a_mass = $finalSpeciesAMass
			species_a_mass_drift = $speciesAMassDrift
			initial_species_b_mass = $initialSpeciesBMass
			final_species_b_mass = $finalSpeciesBMass
			species_b_mass_drift = $speciesBMassDrift
			minimum_species_a_fraction = $minimumSpeciesAFraction
			maximum_species_a_fraction = $maximumSpeciesAFraction
			initial_composition_total_variation = $initialCompositionTotalVariation
			final_composition_total_variation = $finalCompositionTotalVariation
			composition_state_change_l1 = $compositionStateChangeL1
			initial_mixed_cell_count = $initialMixedCellCount
			final_mixed_cell_count = $finalMixedCellCount
			gas_ledger_closes = $gasLedgerCloses
			species_ledger_closes = $speciesLedgerCloses
			species_bounds_preserved = $speciesBoundsPreserved
			composition_evolved = $compositionEvolved
			mixed_region_formed = $mixedRegionFormed
		}
	} elseif ($RunHllc2DPerformance) {
		$boundaryMode = Read-KeyValue -Text $text -Key "boundary_mode"
		$cellLength = [double](Read-KeyValue -Text $text -Key "cell_length")
		$stateAndFluxScratchBytesTotal = [int64](Read-KeyValue -Text $text -Key "state_and_flux_scratch_bytes_total")
		$stateChangeL1 = [double](Read-KeyValue -Text $text -Key "state_change_l1")
		$stateEvolved = (Read-KeyValue -Text $text -Key "state_evolved") -eq "true"
		$physicalTimePolicy = Read-KeyValue -Text $text -Key "physical_time_policy"
		$performanceBudgetStatus = Read-KeyValue -Text $text -Key "performance_budget_status"
		$performanceWarmupCount = [int](Read-KeyValue -Text $text -Key "performance_warmup_count")
		$performanceRepeatCount = [int](Read-KeyValue -Text $text -Key "performance_repeat_count")
		$performanceSteps = [int](Read-KeyValue -Text $text -Key "performance_steps")
		$referenceFrameBudgetMilliseconds = [double](Read-KeyValue -Text $text -Key "reference_frame_budget_milliseconds")
		$legacyGridCellsX = [int](Read-KeyValue -Text $text -Key "legacy_grid_cells_x")
		$legacyGridCellsY = [int](Read-KeyValue -Text $text -Key "legacy_grid_cells_y")
		$legacyGridCellCount = [int](Read-KeyValue -Text $text -Key "legacy_grid_cell_count")
		$doubledGridCellsX = [int](Read-KeyValue -Text $text -Key "doubled_grid_cells_x")
		$doubledGridCellsY = [int](Read-KeyValue -Text $text -Key "doubled_grid_cells_y")
		$doubledGridCellCount = [int](Read-KeyValue -Text $text -Key "doubled_grid_cell_count")
		$particleGridCellsX = [int](Read-KeyValue -Text $text -Key "particle_grid_cells_x")
		$particleGridCellsY = [int](Read-KeyValue -Text $text -Key "particle_grid_cells_y")
		$particleGridCellCount = [int](Read-KeyValue -Text $text -Key "particle_grid_cell_count")
		$legacyGridElapsedMilliseconds = [double](Read-KeyValue -Text $text -Key "legacy_grid_elapsed_milliseconds")
		$doubledGridElapsedMilliseconds = [double](Read-KeyValue -Text $text -Key "doubled_grid_elapsed_milliseconds")
		$particleGridElapsedMilliseconds = [double](Read-KeyValue -Text $text -Key "particle_grid_elapsed_milliseconds")
		$legacyGridMillisecondsPerStep = [double](Read-KeyValue -Text $text -Key "legacy_grid_milliseconds_per_step")
		$doubledGridMillisecondsPerStep = [double](Read-KeyValue -Text $text -Key "doubled_grid_milliseconds_per_step")
		$particleGridMillisecondsPerStep = [double](Read-KeyValue -Text $text -Key "particle_grid_milliseconds_per_step")
		$legacyGridCellUpdatesPerSecond = [double](Read-KeyValue -Text $text -Key "legacy_grid_cell_updates_per_second")
		$doubledGridCellUpdatesPerSecond = [double](Read-KeyValue -Text $text -Key "doubled_grid_cell_updates_per_second")
		$particleGridCellUpdatesPerSecond = [double](Read-KeyValue -Text $text -Key "particle_grid_cell_updates_per_second")
		$legacyGridFractionOfReferenceFrame = [double](Read-KeyValue -Text $text -Key "legacy_grid_fraction_of_reference_frame")
		$doubledGridFractionOfReferenceFrame = [double](Read-KeyValue -Text $text -Key "doubled_grid_fraction_of_reference_frame")
		$particleGridFractionOfReferenceFrame = [double](Read-KeyValue -Text $text -Key "particle_grid_fraction_of_reference_frame")
		$hllc2DPerformance = [ordered]@{
			physical_time_policy = $physicalTimePolicy
			budget_status = $performanceBudgetStatus
			reference_frame_budget_milliseconds = $referenceFrameBudgetMilliseconds
			legacy_grid = [ordered]@{
				cells_x = $legacyGridCellsX
				cells_y = $legacyGridCellsY
				cell_count = $legacyGridCellCount
				elapsed_milliseconds = $legacyGridElapsedMilliseconds
				milliseconds_per_step = $legacyGridMillisecondsPerStep
				cell_updates_per_second = $legacyGridCellUpdatesPerSecond
				fraction_of_reference_frame = $legacyGridFractionOfReferenceFrame
			}
			doubled_grid = [ordered]@{
				cells_x = $doubledGridCellsX
				cells_y = $doubledGridCellsY
				cell_count = $doubledGridCellCount
				elapsed_milliseconds = $doubledGridElapsedMilliseconds
				milliseconds_per_step = $doubledGridMillisecondsPerStep
				cell_updates_per_second = $doubledGridCellUpdatesPerSecond
				fraction_of_reference_frame = $doubledGridFractionOfReferenceFrame
			}
			particle_grid = [ordered]@{
				cells_x = $particleGridCellsX
				cells_y = $particleGridCellsY
				cell_count = $particleGridCellCount
				elapsed_milliseconds = $particleGridElapsedMilliseconds
				milliseconds_per_step = $particleGridMillisecondsPerStep
				cell_updates_per_second = $particleGridCellUpdatesPerSecond
				fraction_of_reference_frame = $particleGridFractionOfReferenceFrame
			}
		}
	} elseif ($RunHllc2DNaturalConvection) {
		$boundaryMode = Read-KeyValue -Text $text -Key "boundary_mode"
		$cellLength = [double](Read-KeyValue -Text $text -Key "cell_length")
		$stateAndFluxScratchBytesTotal = [int64](Read-KeyValue -Text $text -Key "state_and_flux_scratch_bytes_total")
		$sourceLedgerCloses = (Read-KeyValue -Text $text -Key "heated_source_ledger_closes") -eq "true"
		$sourceAndBoundaryLedgerCloses = (Read-KeyValue -Text $text -Key "heated_source_and_boundary_ledger_closes") -eq "true"
		$sourceMassNet = [double](Read-KeyValue -Text $text -Key "heated_source_mass_net")
		$sourceMomentumXNet = [double](Read-KeyValue -Text $text -Key "heated_source_momentum_x_net")
		$sourceMomentumYNet = [double](Read-KeyValue -Text $text -Key "heated_source_momentum_y_net")
		$sourceEnergyNet = [double](Read-KeyValue -Text $text -Key "heated_source_energy_net")
		$sourceEventCount = [int](Read-KeyValue -Text $text -Key "heated_source_event_count")
		$boundaryMassNet = [double](Read-KeyValue -Text $text -Key "heated_boundary_mass_net")
		$boundaryMomentumXNet = [double](Read-KeyValue -Text $text -Key "heated_boundary_momentum_x_net")
		$boundaryMomentumYNet = [double](Read-KeyValue -Text $text -Key "heated_boundary_momentum_y_net")
		$boundaryEnergyNet = [double](Read-KeyValue -Text $text -Key "heated_boundary_energy_net")
		$boundaryEventCount = [int](Read-KeyValue -Text $text -Key "heated_boundary_event_count")
		$combinedMassBalanceError = [double](Read-KeyValue -Text $text -Key "heated_combined_mass_balance_error")
		$combinedMomentumXBalanceError = [double](Read-KeyValue -Text $text -Key "heated_combined_momentum_x_balance_error")
		$combinedMomentumYBalanceError = [double](Read-KeyValue -Text $text -Key "heated_combined_momentum_y_balance_error")
		$combinedEnergyBalanceError = [double](Read-KeyValue -Text $text -Key "heated_combined_energy_balance_error")
		$naturalConvection = [ordered]@{
			gravity_y = [double](Read-KeyValue -Text $text -Key "gravity_y")
			hot_temperature_amplitude = [double](Read-KeyValue -Text $text -Key "hot_temperature_amplitude")
			initial_thermal_center_y = [double](Read-KeyValue -Text $text -Key "initial_thermal_center_y")
			final_thermal_center_y = [double](Read-KeyValue -Text $text -Key "final_thermal_center_y")
			thermal_center_rise = [double](Read-KeyValue -Text $text -Key "thermal_center_rise")
			thermal_weighted_velocity_y = [double](Read-KeyValue -Text $text -Key "thermal_weighted_velocity_y")
			control_maximum_absolute_velocity = [double](Read-KeyValue -Text $text -Key "control_maximum_absolute_velocity")
			heated_maximum_upward_velocity = [double](Read-KeyValue -Text $text -Key "heated_maximum_upward_velocity")
			heated_minimum_downward_velocity = [double](Read-KeyValue -Text $text -Key "heated_minimum_downward_velocity")
			heated_maximum_absolute_velocity = [double](Read-KeyValue -Text $text -Key "heated_maximum_absolute_velocity")
			maximum_upward_velocity_difference = [double](Read-KeyValue -Text $text -Key "maximum_upward_velocity_difference")
			minimum_downward_velocity_difference = [double](Read-KeyValue -Text $text -Key "minimum_downward_velocity_difference")
			maximum_absolute_velocity_difference = [double](Read-KeyValue -Text $text -Key "maximum_absolute_velocity_difference")
			circulation_observed = (Read-KeyValue -Text $text -Key "circulation_observed") -eq "true"
			control = [ordered]@{
				minimum_density = [double](Read-KeyValue -Text $text -Key "control_minimum_density")
				minimum_pressure = [double](Read-KeyValue -Text $text -Key "control_minimum_pressure")
				maximum_cfl = [double](Read-KeyValue -Text $text -Key "control_maximum_cfl")
				source_and_boundary_ledger_closes = (Read-KeyValue -Text $text -Key "control_source_and_boundary_ledger_closes") -eq "true"
				source_momentum_y_net = [double](Read-KeyValue -Text $text -Key "control_source_momentum_y_net")
				source_energy_net = [double](Read-KeyValue -Text $text -Key "control_source_energy_net")
				boundary_momentum_y_net = [double](Read-KeyValue -Text $text -Key "control_boundary_momentum_y_net")
				combined_momentum_y_balance_error = [double](Read-KeyValue -Text $text -Key "control_combined_momentum_y_balance_error")
			}
			heated = [ordered]@{
				minimum_density = [double](Read-KeyValue -Text $text -Key "heated_minimum_density")
				minimum_pressure = [double](Read-KeyValue -Text $text -Key "heated_minimum_pressure")
				maximum_cfl = [double](Read-KeyValue -Text $text -Key "heated_maximum_cfl")
				source_and_boundary_ledger_closes = $sourceAndBoundaryLedgerCloses
				source_momentum_y_net = $sourceMomentumYNet
				source_energy_net = $sourceEnergyNet
				boundary_momentum_y_net = $boundaryMomentumYNet
				combined_momentum_y_balance_error = $combinedMomentumYBalanceError
			}
		}
    } elseif ($RunHllc2DUniform -or $RunHllc2DPressurePulse -or $RunHllc2DSealedHeating) {
        $boundaryMode = Read-KeyValue -Text $text -Key "boundary_mode"
        $cellLength = [double](Read-KeyValue -Text $text -Key "cell_length")
		$stateAndFluxScratchBytesTotal = [int64](Read-KeyValue -Text $text -Key "state_and_flux_scratch_bytes_total")
        $initialMaximumPressure = [double](Read-KeyValue -Text $text -Key "initial_maximum_pressure")
        $finalMaximumPressure = [double](Read-KeyValue -Text $text -Key "final_maximum_pressure")
		$initialMeanPressure = [double](Read-KeyValue -Text $text -Key "initial_mean_pressure")
		$finalMeanPressure = [double](Read-KeyValue -Text $text -Key "final_mean_pressure")
		$initialMeanTemperature = [double](Read-KeyValue -Text $text -Key "initial_mean_temperature")
		$finalMeanTemperature = [double](Read-KeyValue -Text $text -Key "final_mean_temperature")
        $stateChangeL1 = [double](Read-KeyValue -Text $text -Key "state_change_l1")
        $stateEvolved = (Read-KeyValue -Text $text -Key "state_evolved") -eq "true"
        $pressurePeakReduced = (Read-KeyValue -Text $text -Key "pressure_peak_reduced") -eq "true"
		$pressureIncreased = (Read-KeyValue -Text $text -Key "pressure_increased") -eq "true"
		$temperatureIncreased = (Read-KeyValue -Text $text -Key "temperature_increased") -eq "true"
		$sourceLedgerCloses = (Read-KeyValue -Text $text -Key "source_ledger_closes") -eq "true"
		$sourceAndBoundaryLedgerCloses = (Read-KeyValue -Text $text -Key "source_and_boundary_ledger_closes") -eq "true"
		$sourceMassNet = [double](Read-KeyValue -Text $text -Key "source_mass_net")
		$sourceMomentumXNet = [double](Read-KeyValue -Text $text -Key "source_momentum_x_net")
		$sourceMomentumYNet = [double](Read-KeyValue -Text $text -Key "source_momentum_y_net")
		$sourceEnergyNet = [double](Read-KeyValue -Text $text -Key "source_energy_net")
		$sourceEventCount = [int](Read-KeyValue -Text $text -Key "source_event_count")
		$sourceMassBalanceError = [double](Read-KeyValue -Text $text -Key "source_mass_balance_error")
		$sourceMomentumXBalanceError = [double](Read-KeyValue -Text $text -Key "source_momentum_x_balance_error")
		$sourceMomentumYBalanceError = [double](Read-KeyValue -Text $text -Key "source_momentum_y_balance_error")
		$sourceEnergyBalanceError = [double](Read-KeyValue -Text $text -Key "source_energy_balance_error")
		$boundaryMassNet = [double](Read-KeyValue -Text $text -Key "boundary_mass_net")
		$boundaryMomentumXNet = [double](Read-KeyValue -Text $text -Key "boundary_momentum_x_net")
		$boundaryMomentumYNet = [double](Read-KeyValue -Text $text -Key "boundary_momentum_y_net")
		$boundaryEnergyNet = [double](Read-KeyValue -Text $text -Key "boundary_energy_net")
		$boundaryEventCount = [int](Read-KeyValue -Text $text -Key "boundary_event_count")
		$combinedMassBalanceError = [double](Read-KeyValue -Text $text -Key "combined_mass_balance_error")
		$combinedMomentumXBalanceError = [double](Read-KeyValue -Text $text -Key "combined_momentum_x_balance_error")
		$combinedMomentumYBalanceError = [double](Read-KeyValue -Text $text -Key "combined_momentum_y_balance_error")
		$combinedEnergyBalanceError = [double](Read-KeyValue -Text $text -Key "combined_energy_balance_error")
		if ($RunHllc2DSealedHeating) {
			$expectedFinalMeanPressure = [double](Read-KeyValue -Text $text -Key "expected_final_mean_pressure")
			$expectedFinalMeanTemperature = [double](Read-KeyValue -Text $text -Key "expected_final_mean_temperature")
		}
    } elseif ($RunRusanovPressurePulse) {
        $initialMaximumPressure = [double](Read-KeyValue -Text $text -Key "initial_maximum_pressure")
        $finalMaximumPressure = [double](Read-KeyValue -Text $text -Key "final_maximum_pressure")
        $stateChangeL1 = [double](Read-KeyValue -Text $text -Key "state_change_l1")
        $stateEvolved = (Read-KeyValue -Text $text -Key "state_evolved") -eq "true"
        $pressurePeakReduced = (Read-KeyValue -Text $text -Key "pressure_peak_reduced") -eq "true"
    } elseif ($RunRusanovDensityAdvection) {
        $stateChangeL1 = [double](Read-KeyValue -Text $text -Key "state_change_l1")
        $stateEvolved = (Read-KeyValue -Text $text -Key "state_evolved") -eq "true"
        $densityL1Error = [double](Read-KeyValue -Text $text -Key "density_l1_error")
        $densityLinfError = [double](Read-KeyValue -Text $text -Key "density_linf_error")
        $pressureLinfError = [double](Read-KeyValue -Text $text -Key "pressure_linf_error")
        $totalVariationRatio = [double](Read-KeyValue -Text $text -Key "total_variation_ratio")
        $referenceVelocity = [double](Read-KeyValue -Text $text -Key "reference_velocity")
        $referenceShiftCells = [int](Read-KeyValue -Text $text -Key "reference_shift_cells")
        $advectionReferencePassed = (Read-KeyValue -Text $text -Key "advection_reference_passed") -eq "true"
    } elseif ($RunRusanovContactDiscontinuity) {
        $maximumDensity = [double](Read-KeyValue -Text $text -Key "maximum_density")
        $stateChangeL1 = [double](Read-KeyValue -Text $text -Key "state_change_l1")
        $stateEvolved = (Read-KeyValue -Text $text -Key "state_evolved") -eq "true"
        $densityL1Error = [double](Read-KeyValue -Text $text -Key "density_l1_error")
        $densityLinfError = [double](Read-KeyValue -Text $text -Key "density_linf_error")
        $pressureLinfError = [double](Read-KeyValue -Text $text -Key "pressure_linf_error")
        $totalVariationRatio = [double](Read-KeyValue -Text $text -Key "total_variation_ratio")
        $referenceVelocity = [double](Read-KeyValue -Text $text -Key "reference_velocity")
        $referenceShiftCells = [int](Read-KeyValue -Text $text -Key "reference_shift_cells")
        $advectionReferencePassed = (Read-KeyValue -Text $text -Key "advection_reference_passed") -eq "true"
        $densityBoundsPreserved = (Read-KeyValue -Text $text -Key "density_bounds_preserved") -eq "true"
	} elseif ($RunRusanovNearVacuumExpansion -or $RunHllcRusanovFallbackNearVacuumExpansion) {
		$maximumDensity = [double](Read-KeyValue -Text $text -Key "maximum_density")
		$initialMaximumPressure = [double](Read-KeyValue -Text $text -Key "initial_maximum_pressure")
		$finalMaximumPressure = [double](Read-KeyValue -Text $text -Key "final_maximum_pressure")
		$stateChangeL1 = [double](Read-KeyValue -Text $text -Key "state_change_l1")
		$stateEvolved = (Read-KeyValue -Text $text -Key "state_evolved") -eq "true"
		$initialLowDensityRegionMass = [double](Read-KeyValue -Text $text -Key "initial_low_density_region_mass")
		$finalLowDensityRegionMass = [double](Read-KeyValue -Text $text -Key "final_low_density_region_mass")
		$lowDensityRegionMassIncreased = (Read-KeyValue -Text $text -Key "low_density_region_mass_increased") -eq "true"
	} elseif ($RunRusanovSodShockTube -or $RunHllcRusanovFallbackSodShockTube) {
		$maximumDensity = [double](Read-KeyValue -Text $text -Key "maximum_density")
		$boundaryMode = Read-KeyValue -Text $text -Key "boundary_mode"
		$eosGamma = [double](Read-KeyValue -Text $text -Key "eos_gamma")
		$eosSpecificGasConstant = [double](Read-KeyValue -Text $text -Key "eos_specific_gas_constant")
		$initialLeftDensity = [double](Read-KeyValue -Text $text -Key "initial_left_density")
		$initialLeftPressure = [double](Read-KeyValue -Text $text -Key "initial_left_pressure")
		$initialRightDensity = [double](Read-KeyValue -Text $text -Key "initial_right_density")
		$initialRightPressure = [double](Read-KeyValue -Text $text -Key "initial_right_pressure")
		$initialMaximumPressure = $initialLeftPressure
		$finalMaximumPressure = [double](Read-KeyValue -Text $text -Key "maximum_pressure")
		$cellLength = [double](Read-KeyValue -Text $text -Key "cell_length")
		$simulatedTime = [double](Read-KeyValue -Text $text -Key "simulated_time")
		$shockPosition = [double](Read-KeyValue -Text $text -Key "shock_position")
		$minimumVelocityX = [double](Read-KeyValue -Text $text -Key "minimum_velocity_x")
		$maximumVelocityX = [double](Read-KeyValue -Text $text -Key "maximum_velocity_x")
		$stateChangeL1 = [double](Read-KeyValue -Text $text -Key "state_change_l1")
		$stateEvolved = (Read-KeyValue -Text $text -Key "state_evolved") -eq "true"
		$densityBoundsPreserved = (Read-KeyValue -Text $text -Key "density_bounds_preserved") -eq "true"
		$shockReferencePassed = (Read-KeyValue -Text $text -Key "shock_reference_passed") -eq "true"
		$boundaryLedgerCloses = (Read-KeyValue -Text $text -Key "boundary_ledger_closes") -eq "true"
		$boundaryMassExchange = [double](Read-KeyValue -Text $text -Key "boundary_mass_exchange")
		$boundaryMomentumXExchange = [double](Read-KeyValue -Text $text -Key "boundary_momentum_x_exchange")
		$boundaryMomentumYExchange = [double](Read-KeyValue -Text $text -Key "boundary_momentum_y_exchange")
		$boundaryEnergyExchange = [double](Read-KeyValue -Text $text -Key "boundary_energy_exchange")
		$massBalanceError = [double](Read-KeyValue -Text $text -Key "mass_balance_error")
		$momentumXBalanceError = [double](Read-KeyValue -Text $text -Key "momentum_x_balance_error")
		$momentumYBalanceError = [double](Read-KeyValue -Text $text -Key "momentum_y_balance_error")
		$energyBalanceError = [double](Read-KeyValue -Text $text -Key "energy_balance_error")
		$stateAndFluxScratchBytesTotal = [int64](Read-KeyValue -Text $text -Key "state_and_flux_scratch_bytes_total")
	} elseif ($RunRusanovDensityAdvectionRefinement) {
		$maximumDensity = [double](Read-KeyValue -Text $text -Key "maximum_density")
		$finalMaximumPressure = [double](Read-KeyValue -Text $text -Key "maximum_pressure")
		$stateChangeL1 = [double](Read-KeyValue -Text $text -Key "state_change_l1")
		$stateEvolved = (Read-KeyValue -Text $text -Key "state_evolved") -eq "true"
		$boundaryMode = Read-KeyValue -Text $text -Key "boundary_mode"
		$refinementLevels = [int](Read-KeyValue -Text $text -Key "refinement_levels")
		$totalSimulatedTime = [double](Read-KeyValue -Text $text -Key "total_simulated_time")
		$referenceVelocity = [double](Read-KeyValue -Text $text -Key "reference_velocity")
		$coarseCells = [int](Read-KeyValue -Text $text -Key "coarse_cells")
		$mediumCells = [int](Read-KeyValue -Text $text -Key "medium_cells")
		$fineCells = [int](Read-KeyValue -Text $text -Key "fine_cells")
		$coarseDensityL1Error = [double](Read-KeyValue -Text $text -Key "coarse_density_l1_error")
		$mediumDensityL1Error = [double](Read-KeyValue -Text $text -Key "medium_density_l1_error")
		$fineDensityL1Error = [double](Read-KeyValue -Text $text -Key "fine_density_l1_error")
		$coarseToMediumL1Order = [double](Read-KeyValue -Text $text -Key "coarse_to_medium_l1_order")
		$mediumToFineL1Order = [double](Read-KeyValue -Text $text -Key "medium_to_fine_l1_order")
		$coarseMaximumCfl = [double](Read-KeyValue -Text $text -Key "coarse_maximum_cfl")
		$mediumMaximumCfl = [double](Read-KeyValue -Text $text -Key "medium_maximum_cfl")
		$fineMaximumCfl = [double](Read-KeyValue -Text $text -Key "fine_maximum_cfl")
	} elseif ($RunRusanovLowMachAdvection -or $RunAllSpeedRusanovLowMachAdvection `
		-or $RunHllcRusanovFallbackLowMachAdvection) {
		$boundaryMode = Read-KeyValue -Text $text -Key "boundary_mode"
		$maximumDensity = [double](Read-KeyValue -Text $text -Key "maximum_density")
		$finalMaximumPressure = [double](Read-KeyValue -Text $text -Key "maximum_pressure")
		$stateChangeL1 = [double](Read-KeyValue -Text $text -Key "state_change_l1")
		$stateEvolved = (Read-KeyValue -Text $text -Key "state_evolved") -eq "true"
		$moderateNominalMach = [double](Read-KeyValue -Text $text -Key "moderate_nominal_mach")
		$lowNominalMach = [double](Read-KeyValue -Text $text -Key "low_nominal_mach")
		$veryLowNominalMach = [double](Read-KeyValue -Text $text -Key "very_low_nominal_mach")
		$moderateDensityL1Error = [double](Read-KeyValue -Text $text -Key "moderate_density_l1_error")
		$lowDensityL1Error = [double](Read-KeyValue -Text $text -Key "low_density_l1_error")
		$veryLowDensityL1Error = [double](Read-KeyValue -Text $text -Key "very_low_density_l1_error")
		$moderateTotalVariationRatio = [double](Read-KeyValue -Text $text -Key "moderate_total_variation_ratio")
		$lowTotalVariationRatio = [double](Read-KeyValue -Text $text -Key "low_total_variation_ratio")
		$veryLowTotalVariationRatio = [double](Read-KeyValue -Text $text -Key "very_low_total_variation_ratio")
		$lowToModerateL1Ratio = [double](Read-KeyValue -Text $text -Key "low_to_moderate_l1_ratio")
		$veryLowToModerateL1Ratio = [double](Read-KeyValue -Text $text -Key "very_low_to_moderate_l1_ratio")
		$lowMachSuitabilityText = Read-KeyValue -Text $text -Key "low_mach_suitability_passed"
		if ($lowMachSuitabilityText -ne "true" -and $lowMachSuitabilityText -ne "false") {
			throw "Rusanov low-Mach suitability result is invalid"
		}
		$lowMachSuitabilityPassed = $lowMachSuitabilityText -eq "true"
	} elseif ($RunRusanovOpenBoundaryLeak -or $RunHllcRusanovFallbackOpenBoundaryLeak) {
		$boundaryMode = Read-KeyValue -Text $text -Key "boundary_mode"
		$eosGamma = [double](Read-KeyValue -Text $text -Key "eos_gamma")
		$eosSpecificGasConstant = [double](Read-KeyValue -Text $text -Key "eos_specific_gas_constant")
		$initialLeftDensity = [double](Read-KeyValue -Text $text -Key "initial_left_density")
		$initialLeftPressure = [double](Read-KeyValue -Text $text -Key "initial_left_pressure")
		$initialRightDensity = [double](Read-KeyValue -Text $text -Key "initial_right_density")
		$initialRightPressure = [double](Read-KeyValue -Text $text -Key "initial_right_pressure")
		$maximumDensity = [double](Read-KeyValue -Text $text -Key "maximum_density")
		$finalMaximumPressure = [double](Read-KeyValue -Text $text -Key "maximum_pressure")
		$cellLength = [double](Read-KeyValue -Text $text -Key "cell_length")
		$simulatedTime = [double](Read-KeyValue -Text $text -Key "simulated_time")
		$stateChangeL1 = [double](Read-KeyValue -Text $text -Key "state_change_l1")
		$stateEvolved = (Read-KeyValue -Text $text -Key "state_evolved") -eq "true"
		$boundaryLedgerCloses = (Read-KeyValue -Text $text -Key "boundary_ledger_closes") -eq "true"
		$boundaryMassExchange = [double](Read-KeyValue -Text $text -Key "boundary_mass_exchange")
		$boundaryMomentumXExchange = [double](Read-KeyValue -Text $text -Key "boundary_momentum_x_exchange")
		$boundaryMomentumYExchange = [double](Read-KeyValue -Text $text -Key "boundary_momentum_y_exchange")
		$boundaryEnergyExchange = [double](Read-KeyValue -Text $text -Key "boundary_energy_exchange")
		$leftBoundaryMassExchange = [double](Read-KeyValue -Text $text -Key "left_boundary_mass_exchange")
		$leftBoundaryMomentumXExchange = [double](Read-KeyValue -Text $text -Key "left_boundary_momentum_x_exchange")
		$leftBoundaryMomentumYExchange = [double](Read-KeyValue -Text $text -Key "left_boundary_momentum_y_exchange")
		$leftBoundaryEnergyExchange = [double](Read-KeyValue -Text $text -Key "left_boundary_energy_exchange")
		$rightBoundaryMassExchange = [double](Read-KeyValue -Text $text -Key "right_boundary_mass_exchange")
		$rightBoundaryMomentumXExchange = [double](Read-KeyValue -Text $text -Key "right_boundary_momentum_x_exchange")
		$rightBoundaryMomentumYExchange = [double](Read-KeyValue -Text $text -Key "right_boundary_momentum_y_exchange")
		$rightBoundaryEnergyExchange = [double](Read-KeyValue -Text $text -Key "right_boundary_energy_exchange")
		$massDecreased = (Read-KeyValue -Text $text -Key "mass_decreased") -eq "true"
		$rightBoundaryMassOut = [double](Read-KeyValue -Text $text -Key "right_boundary_mass_out")
		$massBalanceError = [double](Read-KeyValue -Text $text -Key "mass_balance_error")
		$momentumXBalanceError = [double](Read-KeyValue -Text $text -Key "momentum_x_balance_error")
		$momentumYBalanceError = [double](Read-KeyValue -Text $text -Key "momentum_y_balance_error")
		$energyBalanceError = [double](Read-KeyValue -Text $text -Key "energy_balance_error")
		$stateAndFluxScratchBytesTotal = [int64](Read-KeyValue -Text $text -Key "state_and_flux_scratch_bytes_total")
	} elseif ($RunRusanovPerformance -or $RunHllcRusanovFallbackPerformance) {
		$boundaryMode = Read-KeyValue -Text $text -Key "boundary_mode"
		$maximumDensity = [double](Read-KeyValue -Text $text -Key "maximum_density")
		$finalMaximumPressure = [double](Read-KeyValue -Text $text -Key "maximum_pressure")
		$stateChangeL1 = [double](Read-KeyValue -Text $text -Key "state_change_l1")
		$stateEvolved = (Read-KeyValue -Text $text -Key "state_evolved") -eq "true"
		$performanceWarmupCount = [int](Read-KeyValue -Text $text -Key "performance_warmup_count")
		$performanceRepeatCount = [int](Read-KeyValue -Text $text -Key "performance_repeat_count")
		$performanceSmallCells = [int](Read-KeyValue -Text $text -Key "small_cells")
		$performanceMediumCells = [int](Read-KeyValue -Text $text -Key "medium_cells")
		$performanceLargeCells = [int](Read-KeyValue -Text $text -Key "large_cells")
		$performanceSteps = [int](Read-KeyValue -Text $text -Key "performance_steps")
		$smallElapsedMilliseconds = [double](Read-KeyValue -Text $text -Key "small_elapsed_milliseconds")
		$mediumElapsedMilliseconds = [double](Read-KeyValue -Text $text -Key "medium_elapsed_milliseconds")
		$largeElapsedMilliseconds = [double](Read-KeyValue -Text $text -Key "large_elapsed_milliseconds")
		$smallCellUpdatesPerSecond = [double](Read-KeyValue -Text $text -Key "small_cell_updates_per_second")
		$mediumCellUpdatesPerSecond = [double](Read-KeyValue -Text $text -Key "medium_cell_updates_per_second")
		$largeCellUpdatesPerSecond = [double](Read-KeyValue -Text $text -Key "large_cell_updates_per_second")
	}
} else {
    $stateDensity = [double](Read-KeyValue -Text $text -Key "state_density")
    $statePressure = [double](Read-KeyValue -Text $text -Key "state_pressure")
}
$limitations = @(
    "No production Air, Simulation, Particle, Save, or Lua code is linked.",
	"No HLLE, thermal/compressible LBM, production TPT wall-coupling, species-EOS coupling, or physical species diffusion is implemented in this scaffold."
)
if ($RunRusanovPerformance) {
	$limitations += "Rusanov performance is a single-threaded strict-double 1D end-to-end candidate measurement with allocation and validation included; no production budget or solver selection is implied."
} elseif ($RunRusanovOpenBoundaryLeak) {
	$limitations += "Rusanov open-boundary leak uses a fixed nondimensional low-pressure reservoir and sealed left wall; it is boundary-ledger evidence, not a production TPT boundary model or performance claim."
} elseif ($isHybridPolicyProbe) {
	$limitations += "Hybrid policy evidence combines a conservative constant-pressure low-Mach transport fixture with a separate whole-case HLLC Sod fixture; cross-route boundary coupling, event-local subcycling and production boundaries are not implemented, so this is not solver or physical-time selection."
} elseif ($isLegacyLikeProbe) {
	$limitations += "Legacy-like is a standalone dimensionless pressure/velocity control stencil, not production Legacy Air equivalence; it has no physical mass, density, momentum-density, energy, species, EOS, vacuum or conservation state."
} elseif ($isLbmProbe) {
	$limitations += "D2Q9 BGK LBM is implemented only as an isothermal periodic low-Mach comparison; it has no total-energy state and does not support the required near-vacuum, shock, species, reacting-gas or production-boundary contracts."
} elseif ($RunHllc2DPerformance) {
	$limitations += "HLLC 2D performance is a single-threaded strict-double periodic end-to-end candidate measurement; physical time and the atmosphere frame budget remain unselected, and no production, species, sealed/open-boundary or GPU performance is implied."
} elseif ($RunHllc2DNaturalConvection) {
	$limitations += "HLLC has one nondimensional strict-double 2D gravity/control natural-convection probe with explicit source and wall-exchange ledgers; it is not a well-balanced proof, physical-time, material-property, production TPT wall, performance or solver-selection result."
} elseif ($RunHllc2DSpeciesMixing) {
	$limitations += "HLLC has one periodic strict-double 2D passive binary conserved-species fixture; species do not affect the EOS, no physical diffusion is implemented, and this is not a production multi-species or solver-selection result."
} elseif ($RunHllc2DSealedHeating) {
	$limitations += "HLLC has one nondimensional strict-double 2D sealed uniform-heating result with an explicit applied-source ledger; it is not physical-time, material-property, TPT wall, performance or solver-selection evidence."
} elseif ($RunHllc2DPressurePulse) {
	$limitations += "HLLC has one periodic strict-double 2D pressure-pulse result; it is not a TPT wall, source-term, physical-time or solver-selection result."
} elseif ($RunHllc2DUniform) {
	$limitations += "HLLC has one periodic strict-double 2D uniform-preservation result; it is not a production or solver-selection result."
} elseif ($RunHllcRusanovFallbackPerformance) {
	$limitations += "HLLC with conservative Rusanov fallback performance is a single-threaded strict-double 1D candidate measurement; no production budget or solver selection is implied."
} elseif ($RunHllcRusanovFallbackOpenBoundaryLeak) {
	$limitations += "HLLC with conservative Rusanov fallback has one fixed-reservoir open-boundary ledger result; it is not a production TPT boundary model or solver selection."
} elseif ($RunRusanovLowMachAdvection) {
	$limitations += "Rusanov low-Mach characterization records the measured diffusion and suitability result; it is not an all-speed solver or production performance claim."
} elseif ($RunAllSpeedRusanovLowMachAdvection) {
	$limitations += "All-speed Rusanov is a standalone strict-double 1D low-Mach candidate; this measured version is rejected by the defined low-Mach suitability threshold and is not a production solver or performance claim."
} elseif ($RunHllcRusanovFallbackLowMachAdvection) {
	$limitations += "HLLC with conservative Rusanov fallback is an isolated strict-double 1D low-Mach candidate result; it is not solver selection or production performance evidence."
} elseif ($RunHllcRusanovFallbackSodShockTube) {
	$limitations += "HLLC with conservative Rusanov fallback has one sealed 1D Sod result; it is not multidimensional, production-boundary or solver-selection evidence."
} elseif ($RunHllcRusanovFallbackNearVacuumExpansion) {
	$limitations += "HLLC with conservative Rusanov fallback has one periodic 1D near-vacuum result; it does not authorize physical floors, production vacuum or solver selection."
} elseif ($RunRusanovDensityAdvectionRefinement) {
	$limitations += "Rusanov has one three-level smooth-advection refinement study; it does not establish low-Mach, multidimensional, leak, source-term or production performance behavior."
} elseif ($RunRusanovSodShockTube) {
	$limitations += "Rusanov is limited to first-order strict-double 1D uniform, periodic wave/advection/contact/near-vacuum probes and one sealed Sod shock tube; this is not solver selection or production boundary evidence."
} elseif ($RunRusanovNearVacuumExpansion) {
	$limitations += "Rusanov is limited to first-order strict-double 1D periodic uniform, pressure-pulse, density-advection, contact and near-vacuum probes; this is not solver selection or production vacuum evidence."
} elseif ($RunRusanovContactDiscontinuity) {
    $limitations += "Rusanov is limited to first-order strict-double 1D periodic uniform, pressure-pulse, density-advection and contact probes; this is not solver selection or production evidence."
} elseif ($RunRusanovDensityAdvection) {
    $limitations += "Rusanov is limited to first-order strict-double 1D periodic uniform, pressure-pulse and density-advection probes; this is not solver selection or production evidence."
} elseif ($RunRusanovPressurePulse) {
    $limitations += "Rusanov is limited to first-order strict-double 1D periodic uniform and pressure-pulse probes; this is not solver selection or production evidence."
} elseif ($RunRusanovUniform) {
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
		boundary_mode = $boundaryMode
		eos_gamma = $eosGamma
		eos_specific_gas_constant = $eosSpecificGasConstant
		initial_left_density = $initialLeftDensity
		initial_left_pressure = $initialLeftPressure
		initial_right_density = $initialRightDensity
		initial_right_pressure = $initialRightPressure
		cell_length = $cellLength
        state_bytes_per_cell = [int](Read-KeyValue -Text $text -Key "state_bytes_per_cell")
        state_and_flux_scratch_bytes_per_cell = $stateAndFluxScratchBytesPerCell
		state_and_flux_scratch_bytes_total = $stateAndFluxScratchBytesTotal
        density = $stateDensity
        maximum_density = $maximumDensity
        pressure = $statePressure
        initial_maximum_pressure = $initialMaximumPressure
        final_maximum_pressure = $finalMaximumPressure
		initial_mean_pressure = $initialMeanPressure
		final_mean_pressure = $finalMeanPressure
		initial_mean_temperature = $initialMeanTemperature
		final_mean_temperature = $finalMeanTemperature
		expected_final_mean_pressure = $expectedFinalMeanPressure
		expected_final_mean_temperature = $expectedFinalMeanTemperature
        state_change_l1 = $stateChangeL1
        state_evolved = $stateEvolved
        pressure_peak_reduced = $pressurePeakReduced
		pressure_increased = $pressureIncreased
		temperature_increased = $temperatureIncreased
        density_l1_error = $densityL1Error
        density_linf_error = $densityLinfError
        pressure_linf_error = $pressureLinfError
        total_variation_ratio = $totalVariationRatio
        reference_velocity = $referenceVelocity
        reference_shift_cells = $referenceShiftCells
        advection_reference_passed = $advectionReferencePassed
        density_bounds_preserved = $densityBoundsPreserved
		initial_low_density_region_mass = $initialLowDensityRegionMass
		final_low_density_region_mass = $finalLowDensityRegionMass
		low_density_region_mass_increased = $lowDensityRegionMassIncreased
		simulated_time = $simulatedTime
		shock_position = $shockPosition
		minimum_velocity_x = $minimumVelocityX
		maximum_velocity_x = $maximumVelocityX
		shock_reference_passed = $shockReferencePassed
		boundary_ledger_closes = $boundaryLedgerCloses
		boundary_mass_exchange = $boundaryMassExchange
		boundary_momentum_x_exchange = $boundaryMomentumXExchange
		boundary_momentum_y_exchange = $boundaryMomentumYExchange
		boundary_energy_exchange = $boundaryEnergyExchange
		left_boundary_mass_exchange = $leftBoundaryMassExchange
		left_boundary_momentum_x_exchange = $leftBoundaryMomentumXExchange
		left_boundary_momentum_y_exchange = $leftBoundaryMomentumYExchange
		left_boundary_energy_exchange = $leftBoundaryEnergyExchange
		right_boundary_mass_exchange = $rightBoundaryMassExchange
		right_boundary_momentum_x_exchange = $rightBoundaryMomentumXExchange
		right_boundary_momentum_y_exchange = $rightBoundaryMomentumYExchange
		right_boundary_energy_exchange = $rightBoundaryEnergyExchange
		mass_decreased = $massDecreased
		right_boundary_mass_out = $rightBoundaryMassOut
		mass_balance_error = $massBalanceError
		momentum_x_balance_error = $momentumXBalanceError
		momentum_y_balance_error = $momentumYBalanceError
		energy_balance_error = $energyBalanceError
		refinement_levels = $refinementLevels
		total_simulated_time = $totalSimulatedTime
		coarse_cells = $coarseCells
		medium_cells = $mediumCells
		fine_cells = $fineCells
		coarse_density_l1_error = $coarseDensityL1Error
		medium_density_l1_error = $mediumDensityL1Error
		fine_density_l1_error = $fineDensityL1Error
		coarse_to_medium_l1_order = $coarseToMediumL1Order
		medium_to_fine_l1_order = $mediumToFineL1Order
		coarse_maximum_cfl = $coarseMaximumCfl
		medium_maximum_cfl = $mediumMaximumCfl
		fine_maximum_cfl = $fineMaximumCfl
		moderate_nominal_mach = $moderateNominalMach
		low_nominal_mach = $lowNominalMach
		very_low_nominal_mach = $veryLowNominalMach
		moderate_density_l1_error = $moderateDensityL1Error
		low_density_l1_error = $lowDensityL1Error
		very_low_density_l1_error = $veryLowDensityL1Error
		moderate_total_variation_ratio = $moderateTotalVariationRatio
		low_total_variation_ratio = $lowTotalVariationRatio
		very_low_total_variation_ratio = $veryLowTotalVariationRatio
		low_to_moderate_l1_ratio = $lowToModerateL1Ratio
		very_low_to_moderate_l1_ratio = $veryLowToModerateL1Ratio
		low_mach_suitability_passed = $lowMachSuitabilityPassed
		mass_drift = if ($isLegacyLikeProbe) { $null } else { [double](Read-KeyValue -Text $text -Key "mass_drift") }
		momentum_drift = if ($isLegacyLikeProbe) { $null } else { [double](Read-KeyValue -Text $text -Key "momentum_drift") }
		momentum_x_drift = if ($isLegacyLikeProbe) { $null } else { [double](Read-KeyValue -Text $text -Key "momentum_x_drift") }
		momentum_y_drift = if ($isLegacyLikeProbe) { $null } else { [double](Read-KeyValue -Text $text -Key "momentum_y_drift") }
		energy_drift = if ($isLegacyLikeProbe -or $isLbmProbe) { $null } else { [double](Read-KeyValue -Text $text -Key "energy_drift") }
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
	conservative_source_ledger = [ordered]@{
		closes = $sourceLedgerCloses
		closes_with_boundary = $sourceAndBoundaryLedgerCloses
		mass_net = $sourceMassNet
		momentum_x_net = $sourceMomentumXNet
		momentum_y_net = $sourceMomentumYNet
		energy_net = $sourceEnergyNet
		event_count = $sourceEventCount
		mass_balance_error = $sourceMassBalanceError
		momentum_x_balance_error = $sourceMomentumXBalanceError
		momentum_y_balance_error = $sourceMomentumYBalanceError
		energy_balance_error = $sourceEnergyBalanceError
	}
	boundary_exchange_ledger = [ordered]@{
		mass_net = $boundaryMassNet
		momentum_x_net = $boundaryMomentumXNet
		momentum_y_net = $boundaryMomentumYNet
		energy_net = $boundaryEnergyNet
		event_count = $boundaryEventCount
		combined_mass_balance_error = $combinedMassBalanceError
		combined_momentum_x_balance_error = $combinedMomentumXBalanceError
		combined_momentum_y_balance_error = $combinedMomentumYBalanceError
		combined_energy_balance_error = $combinedEnergyBalanceError
	}
	natural_convection = $naturalConvection
	species_mixing = $speciesMixing
	hllc_2d_performance = $hllc2DPerformance
	lbm_d2q9 = $lbmD2Q9
	legacy_like = $legacyLike
	hybrid_policy = $hybridPolicy
    measurement = [ordered]@{
        elapsed_milliseconds = [Math]::Round($timer.Elapsed.TotalMilliseconds, 6)
        timing_scope = $timingScope
		warmup_count = $performanceWarmupCount
		repeat_count = $performanceRepeatCount
		small_cells = $performanceSmallCells
		medium_cells = $performanceMediumCells
		large_cells = $performanceLargeCells
		steps_per_repeat = $performanceSteps
		small_elapsed_milliseconds = $smallElapsedMilliseconds
		medium_elapsed_milliseconds = $mediumElapsedMilliseconds
		large_elapsed_milliseconds = $largeElapsedMilliseconds
		small_cell_updates_per_second = $smallCellUpdatesPerSecond
		medium_cell_updates_per_second = $mediumCellUpdatesPerSecond
		large_cell_updates_per_second = $largeCellUpdatesPerSecond
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
