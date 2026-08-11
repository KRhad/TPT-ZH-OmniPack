param(
    [Parameter(Mandatory = $true)]
    [string] $BuildDirectory,

    [string] $MesonExecutable = "meson",

    [string] $GitExecutable = "git",

    [string] $OutputDirectory = "artifacts/vnext-phase5-selection"
)

$ErrorActionPreference = "Stop"
$ProgressPreference = "SilentlyContinue"

$sourceRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot "..")).Path
$resolvedBuildDirectory = (Resolve-Path -LiteralPath $BuildDirectory).Path
$resolvedOutputDirectory = if ([System.IO.Path]::IsPathRooted($OutputDirectory)) {
    [System.IO.Path]::GetFullPath($OutputDirectory)
} else {
    [System.IO.Path]::GetFullPath((Join-Path $sourceRoot $OutputDirectory))
}

function Read-KeyValue {
    param(
        [Parameter(Mandatory = $true)][string] $Text,
        [Parameter(Mandatory = $true)][string] $Key
    )
    $match = [regex]::Match($Text, "(?m)^" + [regex]::Escape($Key) + "=(.*)$")
    if (-not $match.Success) {
        throw "Missing Phase 5 selection field: $Key"
    }
    return $match.Groups[1].Value.Trim()
}

function Get-Sha256 {
    param([Parameter(Mandatory = $true)][string] $Path)
    return (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash
}

Push-Location $sourceRoot
try {
    $dirty = @(& $GitExecutable status --porcelain=v1)
    if ($LASTEXITCODE -ne 0) {
        throw "Unable to read source state"
    }
    if ($dirty.Count -ne 0) {
        throw "Phase 5 selection evidence requires a clean source worktree"
    }
    $sourceCommit = (& $GitExecutable rev-parse HEAD).Trim()
    if ($LASTEXITCODE -ne 0 -or -not $sourceCommit) {
        throw "Unable to resolve source commit"
    }

    & $MesonExecutable compile -C $resolvedBuildDirectory atmospherebench | Out-Host
    if ($LASTEXITCODE -ne 0) {
        throw "AtmosphereBench build failed"
    }

    $executable = Join-Path $resolvedBuildDirectory "atmospherebench.exe"
    if (-not (Test-Path -LiteralPath $executable)) {
        $executable = Join-Path $resolvedBuildDirectory "atmospherebench"
    }
    if (-not (Test-Path -LiteralPath $executable)) {
        throw "AtmosphereBench executable was not found"
    }

    $output = @(& $executable --run-phase5-selection 2>&1)
    if ($LASTEXITCODE -ne 0) {
        throw "Phase 5 selection contract failed: $($output -join [Environment]::NewLine)"
    }
    $text = $output -join [Environment]::NewLine

    foreach ($entry in @{
        "result_status" = "phase5_selection_contract";
        "target_version" = "1.0.5";
        "physical_scale_selection" = "selected_tpt_mm_scale_v1";
        "physical_scale_valid" = "true";
        "physical_time_policy" = "selected_all_speed_split_v1";
        "simulation_tick_fixed" = "true";
        "presentation_independent" = "true";
        "physical_acoustic_domain_cells" = "1434";
        "uniform_acoustic_scaling_used" = "false";
        "bulk_low_mach_time_integration" = "advective_step_plus_geometric_multigrid_projection";
        "compressible_event_time_integration" = "hllc_rusanov_fallback_cfl_subcycling";
        "overload_policy" = "correctness_first_allow_tick_slowdown";
        "accepted_precision_policy" = "strict_double_cpu_reference";
        "fast_math_policy" = "disabled_for_selected_reference";
        "atmosphere_solver_selection" = "selected_hybrid_fvm_projection_hllc_rusanov_v1";
        "selected_low_mach_component" = "geometric_multigrid_v_cycle";
        "selected_compressible_flux" = "hllc_with_rusanov_fallback";
        "candidate_solver_implemented" = "false";
        "production_solver_implemented" = "false";
        "production_runtime_integration" = "deferred_to_1.0.6";
        "phase5_selection_validated" = "true";
        "v1_0_5_gate_ready_for_final_validation" = "true"
    }.GetEnumerator()) {
        if ((Read-KeyValue -Text $text -Key $entry.Key) -ne $entry.Value) {
            throw "Phase 5 selection contract drifted: $($entry.Key)"
        }
    }

    $timestamp = [DateTime]::UtcNow.ToString("yyyyMMddTHHmmssZ")
    $resultDirectory = Join-Path $resolvedOutputDirectory $timestamp
    [System.IO.Directory]::CreateDirectory($resultDirectory) | Out-Null
    $stdoutPath = Join-Path $resultDirectory "stdout.txt"
    $utf8 = [System.Text.UTF8Encoding]::new($false)
    [System.IO.File]::WriteAllText($stdoutPath, $text + [Environment]::NewLine, $utf8)

    $result = [ordered]@{
        schema_version = 1
        gate = "V1_0_5_SELECTION_CONTRACT_GREEN"
        target_version = "1.0.5"
        source = [ordered]@{
            commit = $sourceCommit
            dirty = $false
            runner_sha256 = Get-Sha256 -Path $PSCommandPath
            executable_sha256 = Get-Sha256 -Path $executable
        }
        selection = [ordered]@{
            physical_scale = Read-KeyValue -Text $text -Key "physical_scale_selection"
            pixel_length_m = [double](Read-KeyValue -Text $text -Key "pixel_length_m")
            atmosphere_cell_length_m = [double](Read-KeyValue -Text $text -Key "atmosphere_cell_length_m")
            effective_depth_m = [double](Read-KeyValue -Text $text -Key "effective_depth_m")
            physical_time_policy = Read-KeyValue -Text $text -Key "physical_time_policy"
            physical_seconds_per_tick = [double](Read-KeyValue -Text $text -Key "physical_seconds_per_tick")
            physical_acoustic_domain_cells = [int](Read-KeyValue -Text $text -Key "physical_acoustic_domain_cells")
            atmosphere_solver = Read-KeyValue -Text $text -Key "atmosphere_solver_selection"
            low_mach_component = Read-KeyValue -Text $text -Key "selected_low_mach_component"
            compressible_flux = Read-KeyValue -Text $text -Key "selected_compressible_flux"
            precision = Read-KeyValue -Text $text -Key "accepted_precision_policy"
            production_solver_implemented = $false
        }
        known_yellow = @(
            "full_domain_compressible_budget_measured_over_reference_budget",
            "production_runtime_integration_deferred_to_1.0.6"
        )
    }
    $resultPath = Join-Path $resultDirectory "result.json"
    [System.IO.File]::WriteAllText(
        $resultPath,
        ($result | ConvertTo-Json -Depth 8) + [Environment]::NewLine,
        $utf8
    )
    Write-Output "run-phase5-selection: PASS"
    Write-Output "source_commit=$sourceCommit"
    Write-Output "source_dirty=false"
    Write-Output "gate=V1_0_5_SELECTION_CONTRACT_GREEN"
    Write-Output "result_json=$resultPath"
} finally {
    Pop-Location
}
