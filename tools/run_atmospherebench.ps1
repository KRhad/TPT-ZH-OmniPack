param(
    [string] $Executable,

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
        StateSha256 = [Convert]::ToHexString([System.Security.Cryptography.SHA256]::HashData(
            [System.Text.Encoding]::UTF8.GetBytes("HEAD=$head`n")
        ))
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
$buildSystemFiles = @(Get-Content -LiteralPath $buildSystemFilesPath -Raw | ConvertFrom-Json)
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
    $targets = @($targetsText | ConvertFrom-Json)
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
$output = @(& $resolvedExecutable "--run-uniform" 2>&1)
$exitCode = $LASTEXITCODE
$timer.Stop()
if ($exitCode -ne 0) {
    throw "AtmosphereBench uniform contract run failed: exit_code=$exitCode output=$($output -join [Environment]::NewLine)"
}
$text = $output -join [Environment]::NewLine
if ((Read-KeyValue -Text $text -Key "result_status") -ne "contract_only") {
    throw "AtmosphereBench scaffold must report result_status=contract_only"
}
if ((Read-KeyValue -Text $text -Key "physical_scale_selection") -ne "unselected") {
    throw "AtmosphereBench selected PhysicalScale unexpectedly"
}
if ((Read-KeyValue -Text $text -Key "atmosphere_solver_selection") -ne "unselected") {
    throw "AtmosphereBench selected a solver unexpectedly"
}
if ((Read-KeyValue -Text $text -Key "eos_fixture") -ne "synthetic_nondimensional") {
    throw "AtmosphereBench uniform contract used an unreviewed EOS fixture"
}
foreach ($ledgerKey in @("mass_drift", "momentum_drift", "energy_drift", "numerical_correction_count")) {
    if ((Read-KeyValue -Text $text -Key $ledgerKey) -ne "0") {
        throw "Uniform contract expected $ledgerKey=0"
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

$result = [ordered]@{
    schema_version = 1
    benchmark_kind = "atmospherebench_contract_uniform"
    benchmark_execution_status = "PASS"
    performance_gate = "not_evaluated_contract_only"
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
        result_status = Read-KeyValue -Text $text -Key "result_status"
        candidate_implementations = "none"
    }
    case = [ordered]@{
        id = Read-KeyValue -Text $text -Key "case"
        density = [double](Read-KeyValue -Text $text -Key "state_density")
        pressure = [double](Read-KeyValue -Text $text -Key "state_pressure")
        mass_drift = [double](Read-KeyValue -Text $text -Key "mass_drift")
        momentum_drift = [double](Read-KeyValue -Text $text -Key "momentum_drift")
        energy_drift = [double](Read-KeyValue -Text $text -Key "energy_drift")
        numerical_correction_count = [int](Read-KeyValue -Text $text -Key "numerical_correction_count")
    }
    measurement = [ordered]@{
        elapsed_milliseconds = [Math]::Round($timer.Elapsed.TotalMilliseconds, 6)
        timing_scope = "standalone_contract_uniform_no_solver_step"
    }
    limitations = @(
        "No production Air, Simulation, Particle, Save, or Lua code is linked.",
        "No FVM, LBM, source-term, boundary, or multi-species candidate is implemented in this scaffold.",
        "This result is a contract artifact, not solver-performance or physical-time evidence."
    )
}
$utf8 = [System.Text.UTF8Encoding]::new($false)
[System.IO.File]::WriteAllText((Join-Path $resultDirectory "stdout.txt"), $text + [Environment]::NewLine, $utf8)
[System.IO.File]::WriteAllText((Join-Path $resultDirectory "result.json"), ($result | ConvertTo-Json -Depth 8) + [Environment]::NewLine, $utf8)

Write-Output "run-atmospherebench: PASS"
Write-Output "source_commit=$($sourceState.Commit)"
Write-Output "source_dirty=false"
Write-Output "physical_scale_selection=unselected"
Write-Output "atmosphere_solver_selection=unselected"
Write-Output "performance_gate=not_evaluated_contract_only"
Write-Output "result_json=$(Join-Path $resultDirectory 'result.json')"
