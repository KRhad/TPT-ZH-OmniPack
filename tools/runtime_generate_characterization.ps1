param(
    [Parameter(Mandatory = $true)]
    [string] $Executable,

    [string] $BuildDirectory,

    [string[]] $CaseId = @(),

    [string] $RuntimeDirectory = "C:\msys64\ucrt64\bin",

    [string] $PythonExecutable = "C:\msys64\ucrt64\bin\python.exe",

    [string] $OutputDirectory = "artifacts/vnext-characterization",

    [string] $TemporaryDirectory = [System.IO.Path]::GetTempPath(),

    [ValidateRange(30, 7200)]
    [int] $TimeoutSeconds = 900,

    [switch] $KeepTemporary
)

$ErrorActionPreference = "Stop"
$ProgressPreference = "SilentlyContinue"

$sourceRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot "..")).Path
$resolvedExecutable = (Resolve-Path -LiteralPath $Executable).Path
$resolvedBuildDirectory = if ($BuildDirectory) {
    (Resolve-Path -LiteralPath $BuildDirectory).Path
}
else {
    (Get-Item -LiteralPath $resolvedExecutable).Directory.FullName
}
$resolvedRuntimeDirectory = (Resolve-Path -LiteralPath $RuntimeDirectory).Path
$luaSource = Join-Path $PSScriptRoot "runtime\characterization_scenarios.lua"
$casesSource = Join-Path $PSScriptRoot "runtime\characterization_cases.json"
$canonicalizerSource = Join-Path $PSScriptRoot "characterization_ops_canonical.py"
$loadBoundaryComparatorSource = Join-Path $PSScriptRoot "load_boundary_compare.py"
foreach ($requiredPath in @(
    $luaSource,
    $casesSource,
    $canonicalizerSource,
    $loadBoundaryComparatorSource
)) {
    if (-not (Test-Path -LiteralPath $requiredPath -PathType Leaf)) {
        throw "Missing characterization source: $requiredPath"
    }
}
$resolvedPythonExecutable = (Resolve-Path -LiteralPath $PythonExecutable).Path

function Get-TextSha256 {
    param([Parameter(Mandatory = $true)][string] $Text)
    $hasher = [System.Security.Cryptography.SHA256]::Create()
    try {
        $bytes = [System.Text.Encoding]::UTF8.GetBytes($Text)
        # SHA256.HashData and Convert.ToHexString are unavailable in Windows
        # PowerShell 5's .NET Framework. BitConverter keeps the evidence wrapper
        # byte-identical while supporting both PS5 and PS7.
        return ([System.BitConverter]::ToString($hasher.ComputeHash($bytes))).Replace("-", "")
    }
    finally {
        $hasher.Dispose()
    }
}

function Get-FileSha256 {
    param([Parameter(Mandatory = $true)][string] $Path)
    $resolvedPath = (Resolve-Path -LiteralPath $Path).Path
    $stream = [System.IO.File]::OpenRead($resolvedPath)
    $hasher = [System.Security.Cryptography.SHA256]::Create()
    try {
        return ([System.BitConverter]::ToString($hasher.ComputeHash($stream))).Replace("-", "")
    }
    finally {
        $hasher.Dispose()
        $stream.Dispose()
    }
}

function Read-KeyValueFile {
    param([Parameter(Mandatory = $true)][string] $Path)
    $values = @{}
    foreach ($line in Get-Content -LiteralPath $Path) {
        if ($line -notmatch '^([A-Za-z0-9_]+)=(.*)$') {
            throw "Invalid characterization result line: $line"
        }
        if ($values.ContainsKey($matches[1])) {
            throw "Duplicate characterization result key: $($matches[1])"
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
        throw "Characterization result field is not a nonnegative integer: $Field"
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
        throw "Refusing to remove characterization data outside the selected temporary root"
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
                Write-Warning "Unable to remove isolated characterization directory: $fullPath"
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
        throw "Cannot resolve characterization source HEAD"
    }
    $status = @(& git -C $Repository status --porcelain=v1 --untracked-files=all)
    if ($LASTEXITCODE -ne 0) {
        throw "Cannot inspect characterization source worktree"
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
        throw "Cannot hash tracked characterization source changes"
    }
    foreach ($relativePath in @(
        & git -C $Repository -c core.quotepath=false ls-files --others --exclude-standard
    )) {
        $fullPath = Join-Path $Repository $relativePath
        if (Test-Path -LiteralPath $fullPath -PathType Leaf) {
            $hash = Get-FileSha256 -Path $fullPath
            [void]$material.AppendLine("UNTRACKED=$relativePath|$hash")
        }
    }
    if ($LASTEXITCODE -ne 0) {
        throw "Cannot enumerate untracked characterization source files"
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
    $optionRows = Get-Content -LiteralPath $optionsPath -Raw | ConvertFrom-Json
    $options = [ordered]@{}
    foreach ($row in $optionRows) {
        $options[$row.name] = $row.value
    }
    if ($options["fp_mode"] -ne "legacy_fast") {
        throw "Characterization baselines require fp_mode=legacy_fast"
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

function Test-OpsFile {
    param(
        [Parameter(Mandatory = $true)][string] $GenerationRoot,
        [Parameter(Mandatory = $true)][string] $Stamp
    )
    if ($Stamp -notmatch '^[0-9A-Fa-f]{10}$') {
        throw "Unsafe or invalid characterization stamp ID: $Stamp"
    }
    $stampRoot = [System.IO.Path]::GetFullPath((Join-Path $GenerationRoot "stamps"))
    $path = (Resolve-Path -LiteralPath (
        Join-Path $stampRoot ($Stamp + ".stm")
    )).Path
    $prefix = $stampRoot.TrimEnd(
        [System.IO.Path]::DirectorySeparatorChar,
        [System.IO.Path]::AltDirectorySeparatorChar
    ) + [System.IO.Path]::DirectorySeparatorChar
    if (-not $path.StartsWith($prefix, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Resolved characterization OPS escaped the generation stamp directory"
    }
    [byte[]]$bytes = [System.IO.File]::ReadAllBytes($path)
    if ($bytes.Length -le 15 -or [System.Text.Encoding]::ASCII.GetString($bytes, 0, 4) -ne "OPS1") {
        throw "Characterization save is not an OPS1 container: $path"
    }
    if ([System.Text.Encoding]::ASCII.GetString($bytes, 12, 3) -ne "BZh") {
        throw "Characterization OPS does not contain a bzip2 payload: $path"
    }
    $payloadLength = [System.BitConverter]::ToUInt32($bytes, 8)
    if ($payloadLength -eq 0 -or $payloadLength -gt 209715200) {
        throw "Characterization OPS declared an invalid payload length: $payloadLength"
    }
    return [pscustomobject]@{
        Path = $path
        Length = [int64]$bytes.Length
        PayloadLength = [uint64]$payloadLength
        Sha256 = Get-FileSha256 -Path $path
    }
}

function Get-CanonicalOpsInfo {
    param([Parameter(Mandatory = $true)][string] $Path)
    $json = & $resolvedPythonExecutable $canonicalizerSource $Path --json
    if ($LASTEXITCODE -ne 0 -or -not $json) {
        throw "OPS canonicalization failed: $Path"
    }
    return $json | ConvertFrom-Json
}

function Resolve-PhaseOutputFile {
    param(
        [Parameter(Mandatory = $true)][string] $Root,
        [Parameter(Mandatory = $true)][string] $RelativePath,
        [Parameter(Mandatory = $true)][string] $Field
    )
    if ([System.IO.Path]::IsPathRooted($RelativePath)) {
        throw "Characterization phase output must be relative: field=$Field; path=$RelativePath"
    }
    $candidate = [System.IO.Path]::GetFullPath((Join-Path $Root $RelativePath))
    $prefix = [System.IO.Path]::GetFullPath($Root).TrimEnd(
        [System.IO.Path]::DirectorySeparatorChar,
        [System.IO.Path]::AltDirectorySeparatorChar
    ) + [System.IO.Path]::DirectorySeparatorChar
    if (-not $candidate.StartsWith($prefix, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Characterization phase output escaped its isolated root: field=$Field; path=$RelativePath"
    }
    if (-not (Test-Path -LiteralPath $candidate -PathType Leaf)) {
        throw "Characterization phase output is missing: field=$Field; path=$candidate"
    }
    return $candidate
}

function Get-LoadBoundaryCapture {
    param(
        [Parameter(Mandatory = $true)][string] $Root,
        [Parameter(Mandatory = $true)] $Values,
        [Parameter(Mandatory = $true)][string] $Phase
    )
    $fields = @(
        [pscustomobject]@{ Name = "particle"; FileField = "load_boundary_particle_file"; CountField = "load_boundary_particle_records" },
        [pscustomobject]@{ Name = "cell"; FileField = "load_boundary_cell_file"; CountField = "load_boundary_cell_records" },
        [pscustomobject]@{ Name = "settings"; FileField = "load_boundary_settings_file"; CountField = "load_boundary_settings_records" }
    )
    $capture = [ordered]@{}
    foreach ($field in $fields) {
        if (-not $Values.ContainsKey($field.FileField) -or -not $Values.ContainsKey($field.CountField)) {
            throw "Characterization $Phase omitted load-boundary capture field: $($field.Name)"
        }
        $path = Resolve-PhaseOutputFile -Root $Root -RelativePath $Values[$field.FileField] -Field $field.FileField
        $capture[$field.Name] = [pscustomobject]@{
            Path = $path
            Records = Convert-NonnegativeInteger $Values[$field.CountField] $field.CountField
            Sha256 = Get-FileSha256 -Path $path
        }
    }
    return [pscustomobject]$capture
}

function Invoke-LoadBoundaryComparison {
    param(
        [Parameter(Mandatory = $true)] $Before,
        [Parameter(Mandatory = $true)] $Loaded,
        [Parameter(Mandatory = $true)][string] $OutputPath
    )
    $oldDontWriteBytecode = $env:PYTHONDONTWRITEBYTECODE
    try {
        $env:PYTHONDONTWRITEBYTECODE = "1"
        & $resolvedPythonExecutable -B $loadBoundaryComparatorSource `
            --before-particles $Before.particle.Path `
            --loaded-particles $Loaded.particle.Path `
            --before-cells $Before.cell.Path `
            --loaded-cells $Loaded.cell.Path `
            --before-settings $Before.settings.Path `
            --loaded-settings $Loaded.settings.Path `
            --output $OutputPath
        if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $OutputPath -PathType Leaf)) {
            throw "Load-boundary comparison failed: output=$OutputPath"
        }
    }
    finally {
        if ($null -eq $oldDontWriteBytecode) {
            Remove-Item Env:PYTHONDONTWRITEBYTECODE -ErrorAction SilentlyContinue
        }
        else {
            $env:PYTHONDONTWRITEBYTECODE = $oldDontWriteBytecode
        }
    }
    return Get-Content -LiteralPath $OutputPath -Raw | ConvertFrom-Json
}

function Write-PhaseFiles {
    param(
        [Parameter(Mandatory = $true)][string] $Root,
        [Parameter(Mandatory = $true)][string] $Phase,
        [Parameter(Mandatory = $true)] $Case
    )
    New-Item -ItemType Directory -Path $Root | Out-Null
    $utf8NoBom = [System.Text.UTF8Encoding]::new($false)
    [System.IO.File]::WriteAllText(
        (Join-Path $Root "powder.pref"),
        '{"LuaHookTimeout":900000}' + [Environment]::NewLine,
        $utf8NoBom
    )
    Copy-Item -LiteralPath $luaSource -Destination (Join-Path $Root "autorun.lua")
    $configPath = Join-Path $Root "characterization.config"
    [System.IO.File]::WriteAllLines(
        $configPath,
        @(
            "schema_version=1",
            "phase=$Phase",
            "case_id=$($Case.id)",
            "slug=$($Case.slug)",
            "settings_profile=$($Case.settings_profile)",
            "warmup_steps=$($Case.warmup_steps)",
            "trace_steps=$($Case.trace_steps)",
            "expected_min_particles=$($Case.expected_min_particles)",
            "expected_max_particles=$($Case.expected_max_particles)",
            "seed_a=$($Case.seed[0])",
            "seed_b=$($Case.seed[1])",
            "seed_c=$($Case.seed[2])",
            "seed_d=$($Case.seed[3])",
            "required_identifiers=$($Case.required_identifiers -join ',')"
        ),
        [System.Text.Encoding]::ASCII
    )
    return Get-FileSha256 -Path $configPath
}

function ConvertTo-ProcessArgument {
    param([Parameter(Mandatory = $true)][string] $Value)
    if ($Value.Contains('"')) {
        throw "Unsupported quote in characterization process argument"
    }
    return '"' + $Value + '"'
}

function Invoke-CharacterizationPhase {
    param(
        [Parameter(Mandatory = $true)][string] $Root,
        [Parameter(Mandatory = $true)][string] $Phase,
        [Parameter(Mandatory = $true)] $Case,
        [string] $OpenSave
    )
    $configSha256 = Write-PhaseFiles -Root $Root -Phase $Phase -Case $Case
    $resultPath = Join-Path $Root ("characterization-" + $Phase + ".result")
    $startInfo = [System.Diagnostics.ProcessStartInfo]::new()
    $startInfo.FileName = $resolvedExecutable
    $startInfo.WorkingDirectory = $Root
    $startInfo.UseShellExecute = $false
    $startInfo.CreateNoWindow = $true
    $startInfo.RedirectStandardOutput = $true
    $startInfo.RedirectStandardError = $true
    if ($null -ne $startInfo.PSObject.Properties["ArgumentList"]) {
        # Keep the modern path explicit so its command line remains easy to
        # inspect. Windows PowerShell 5 lacks ArgumentList and uses the
        # quoted Arguments fallback below.
        $startInfo.ArgumentList.Add("ddir")
        $startInfo.ArgumentList.Add($Root)
        if ($OpenSave) {
            $startInfo.ArgumentList.Add("open")
            $startInfo.ArgumentList.Add($OpenSave)
        }
    }
    else {
        $arguments = @("ddir", $Root)
        if ($OpenSave) {
            $arguments += @("open", $OpenSave)
        }
        $startInfo.Arguments = (@($arguments | ForEach-Object {
            ConvertTo-ProcessArgument -Value $_
        }) -join " ")
    }
    # EnvironmentVariables is available in both Windows PowerShell 5 and
    # PowerShell 7. PS5 exposes an incompatible adapter through Environment,
    # so select the legacy dictionary API deliberately instead of feature-
    # detecting by property name.
    $childEnvironment = $startInfo.EnvironmentVariables
    $childEnvironment["PATH"] = $resolvedRuntimeDirectory +
        [System.IO.Path]::PathSeparator + $env:PATH
    foreach ($secretName in @("GITHUB_PAT_TOKEN", "GITHUB_TOKEN", "GH_TOKEN")) {
        [void]$childEnvironment.Remove($secretName)
    }

    $started = [DateTime]::UtcNow
    $process = [System.Diagnostics.Process]::Start($startInfo)
    if (-not $process) {
        throw "Failed to start characterization phase $Phase for $($Case.id)"
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
                $peakPrivateBytes = [Math]::Max($peakPrivateBytes, [int64]$process.PrivateMemorySize64)
                Start-Sleep -Milliseconds 100
            }
        } while (-not $process.HasExited -and [DateTime]::UtcNow -lt $deadline)
        if (-not $process.HasExited) {
            Stop-Process -Id $process.Id -Force
            $process.WaitForExit()
            throw "Characterization phase timed out: case=$($Case.id); phase=$Phase; root=$Root"
        }
        $process.WaitForExit()
        $process.Refresh()
        $stdout = $stdoutTask.GetAwaiter().GetResult()
        $stderr = $stderrTask.GetAwaiter().GetResult()
        [System.IO.File]::WriteAllText(
            (Join-Path $Root "client-stdout.log"), $stdout,
            [System.Text.UTF8Encoding]::new($false)
        )
        [System.IO.File]::WriteAllText(
            (Join-Path $Root "client-stderr.log"), $stderr,
            [System.Text.UTF8Encoding]::new($false)
        )
        if (-not (Test-Path -LiteralPath $resultPath -PathType Leaf)) {
            throw "Characterization phase produced no result: case=$($Case.id); phase=$Phase; exit=$($process.ExitCode); stdout=$($stdout.Trim()); stderr=$($stderr.Trim()); root=$Root"
        }
        $result = Read-KeyValueFile -Path $resultPath
        if ($process.ExitCode -ne 0 -or $result.OMNI_CHARACTERIZATION_STATUS -ne "PASS") {
            throw "Characterization phase failed: case=$($Case.id); phase=$Phase; exit=$($process.ExitCode); error=$($result.error); root=$Root"
        }
        if ($result.case_id -ne $Case.id -or $result.slug -ne $Case.slug -or $result.phase -ne $Phase) {
            throw "Characterization phase identity mismatch: case=$($Case.id); phase=$Phase"
        }
        return [pscustomobject]@{
            Values = $result
            ResultPath = $resultPath
            ConfigPath = Join-Path $Root "characterization.config"
            ConfigSha256 = $configSha256
            StdoutPath = Join-Path $Root "client-stdout.log"
            StderrPath = Join-Path $Root "client-stderr.log"
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

$caseDocument = Get-Content -LiteralPath $casesSource -Raw | ConvertFrom-Json
if ($caseDocument.schema_version -ne 1 -or $caseDocument.suite_id -ne "legacy-c01-c14-v1") {
    throw "Unsupported characterization case manifest"
}
$allCases = @($caseDocument.cases)
if ($allCases.Count -ne 14) {
    throw "Characterization manifest must contain exactly 14 cases"
}
$expectedIds = 1..14 | ForEach-Object { "C{0:D2}" -f $_ }
$actualIdText = @($allCases.id | Sort-Object) -join ','
$expectedIdText = $expectedIds -join ','
if ($actualIdText -ne $expectedIdText) {
    throw "Characterization manifest IDs are not exactly C01-C14"
}
$selectedIds = if ($CaseId.Count) { @($CaseId | ForEach-Object { $_.ToUpperInvariant() }) } else { $expectedIds }
foreach ($id in $selectedIds) {
    if ($id -notin $expectedIds) {
        throw "Unknown characterization case ID: $id"
    }
}
$selectedCases = @($allCases | Where-Object { $_.id -in $selectedIds } | Sort-Object id)

$gitState = Get-GitState -Repository $sourceRoot
$build = Get-BuildProvenance -Directory $resolvedBuildDirectory
$executableInfo = Get-Item -LiteralPath $resolvedExecutable
$executableSha256 = Get-FileSha256 -Path $resolvedExecutable
$luaSha256 = Get-FileSha256 -Path $luaSource
$casesSha256 = Get-FileSha256 -Path $casesSource
$canonicalizerSha256 = (
    Get-FileSha256 -Path $canonicalizerSource
)
$loadBoundaryComparatorSha256 = (
    Get-FileSha256 -Path $loadBoundaryComparatorSource
)
$tempParent = [System.IO.Path]::GetFullPath($TemporaryDirectory)
if (-not (Test-Path -LiteralPath $tempParent -PathType Container)) {
    throw "Temporary directory does not exist: $tempParent"
}

$cpu = Get-CimInstance Win32_Processor -ErrorAction SilentlyContinue | Select-Object -First 1
$computer = Get-CimInstance Win32_ComputerSystem -ErrorAction Stop
$os = Get-CimInstance Win32_OperatingSystem -ErrorAction Stop
$gpu = @(Get-CimInstance Win32_VideoController -ErrorAction SilentlyContinue | ForEach-Object {
    [pscustomobject][ordered]@{
        name = [string]$_.Name
        driver_version = [string]$_.DriverVersion
    }
})
$cpuName = if ($cpu) { [string]$cpu.Name.Trim() } else { "not_tested" }
$machineId = "windows-" + (Get-TextSha256 -Text (
    "$($env:COMPUTERNAME)|$cpuName|$($computer.TotalPhysicalMemory)"
)).Substring(0, 12)
$powerMode = "not_tested"
try {
    $powerText = (& powercfg /getactivescheme 2>$null | Out-String).Trim()
    if ($LASTEXITCODE -eq 0 -and $powerText) { $powerMode = $powerText }
}
catch { $powerMode = "not_tested" }

$suiteRunId = [DateTime]::UtcNow.ToString("yyyyMMddTHHmmssZ") + "-" +
    [guid]::NewGuid().ToString("N").Substring(0, 8)
$artifactBase = if ([System.IO.Path]::IsPathRooted($OutputDirectory)) {
    [System.IO.Path]::GetFullPath($OutputDirectory)
}
else {
    [System.IO.Path]::GetFullPath((Join-Path $sourceRoot $OutputDirectory))
}
$suiteArtifactRoot = Join-Path $artifactBase (
    Join-Path $machineId (Join-Path $gitState.Commit.Substring(0, 10) $suiteRunId)
)
New-Item -ItemType Directory -Path $suiteArtifactRoot -Force | Out-Null
$frozenInputsRoot = Join-Path $suiteArtifactRoot "frozen-inputs"
New-Item -ItemType Directory -Path $frozenInputsRoot | Out-Null
foreach ($input in @(
    [pscustomobject]@{ Name = "characterization_scenarios.lua"; Path = $luaSource },
    [pscustomobject]@{ Name = "characterization_cases.json"; Path = $casesSource },
    [pscustomobject]@{ Name = "characterization_ops_canonical.py"; Path = $canonicalizerSource },
    [pscustomobject]@{ Name = "load_boundary_compare.py"; Path = $loadBoundaryComparatorSource },
    [pscustomobject]@{ Name = "runtime_generate_characterization.ps1"; Path = $PSCommandPath }
)) {
    Copy-Item -LiteralPath $input.Path -Destination (Join-Path $frozenInputsRoot $input.Name)
}

$caseResults = [System.Collections.Generic.List[object]]::new()
foreach ($case in $selectedCases) {
    $caseRunId = $case.id + "-" + $case.slug + "-" +
        [guid]::NewGuid().ToString("N").Substring(0, 8)
    $testRoot = [System.IO.Path]::GetFullPath(
        (Join-Path $tempParent ("tpt-omnipack-characterization-" + $caseRunId))
    )
    $tempPrefix = $tempParent.TrimEnd(
        [System.IO.Path]::DirectorySeparatorChar,
        [System.IO.Path]::AltDirectorySeparatorChar
    ) + [System.IO.Path]::DirectorySeparatorChar
    if (-not $testRoot.StartsWith($tempPrefix, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to create characterization data outside the selected temporary root"
    }

    $caseCompleted = $false
    try {
        New-Item -ItemType Directory -Path $testRoot | Out-Null
        $generationRoot = Join-Path $testRoot "generate"
        $verifyARoot = Join-Path $testRoot "verify-a"
        $verifyBRoot = Join-Path $testRoot "verify-b"
        $generation = Invoke-CharacterizationPhase `
            -Root $generationRoot -Phase "generate" -Case $case
        $firstOps = Test-OpsFile -GenerationRoot $generationRoot -Stamp $generation.Values.first_stamp
        $secondOps = Test-OpsFile -GenerationRoot $generationRoot -Stamp $generation.Values.second_stamp
        $firstCanonical = Get-CanonicalOpsInfo -Path $firstOps.Path
        $secondCanonical = Get-CanonicalOpsInfo -Path $secondOps.Path
        if ($firstCanonical.canonical_payload_sha256 -ne
            $secondCanonical.canonical_payload_sha256 -or
            $firstOps.PayloadLength -ne $secondOps.PayloadLength) {
            throw "Two same-state OPS logical payloads differ: case=$($case.id)"
        }

        $verifyA = Invoke-CharacterizationPhase `
            -Root $verifyARoot -Phase "verify-a" -Case $case -OpenSave $firstOps.Path
        $verifyB = Invoke-CharacterizationPhase `
            -Root $verifyBRoot -Phase "verify-b" -Case $case -OpenSave $firstOps.Path
        $traceAPath = (Resolve-Path -LiteralPath (
            Join-Path $verifyARoot $verifyA.Values.trace_file
        )).Path
        $traceBPath = (Resolve-Path -LiteralPath (
            Join-Path $verifyBRoot $verifyB.Values.trace_file
        )).Path
        $traceASha256 = Get-FileSha256 -Path $traceAPath
        $traceBSha256 = Get-FileSha256 -Path $traceBPath
        if ($traceASha256 -ne $traceBSha256) {
            throw "Cross-restart characterization traces differ: case=$($case.id)"
        }
        $traceLineCount = @(Get-Content -LiteralPath $traceAPath).Count
        if ($traceLineCount -ne [int]$case.trace_steps + 2) {
            throw "Characterization trace length mismatch: case=$($case.id); lines=$traceLineCount"
        }

        foreach ($field in @(
            "loaded_particles", "loaded_hash", "loaded_rng", "loaded_required_counts",
            "final_particles", "final_hash", "final_rng", "final_required_counts"
        )) {
            if ($verifyA.Values[$field] -ne $verifyB.Values[$field]) {
                throw "Cross-restart characterization field differs: case=$($case.id); field=$field"
            }
        }
        if ($verifyA.Values.loaded_rng -ne $generation.Values.rng_before_save) {
            throw "OPS load did not restore RNG state: case=$($case.id)"
        }
        if ((Convert-NonnegativeInteger $verifyA.Values.loaded_particles "loaded_particles") -ne
            (Convert-NonnegativeInteger $generation.Values.particles_before_save "particles_before_save")) {
            throw "OPS load changed particle count: case=$($case.id)"
        }
        if ($verifyA.Values.loaded_required_counts -ne
            $generation.Values.required_counts_before_save) {
            throw "OPS load changed required element counts: case=$($case.id)"
        }

        $beforeBoundary = Get-LoadBoundaryCapture `
            -Root $generationRoot -Values $generation.Values -Phase "generate"
        $loadedBoundaryA = Get-LoadBoundaryCapture `
            -Root $verifyARoot -Values $verifyA.Values -Phase "verify-a"
        $loadedBoundaryB = Get-LoadBoundaryCapture `
            -Root $verifyBRoot -Values $verifyB.Values -Phase "verify-b"
        if ($beforeBoundary.particle.Records -ne
            (Convert-NonnegativeInteger $generation.Values.particles_before_save "particles_before_save")) {
            throw "Pre-save load-boundary particle count differs from generation result: case=$($case.id)"
        }
        if ($beforeBoundary.cell.Records -le 0 -or $beforeBoundary.settings.Records -le 0) {
            throw "Pre-save load-boundary capture is incomplete: case=$($case.id)"
        }
        foreach ($captureName in @("particle", "cell", "settings")) {
            if ($loadedBoundaryA.$captureName.Records -ne $loadedBoundaryB.$captureName.Records) {
                throw "Cross-restart load-boundary record count differs: case=$($case.id); capture=$captureName"
            }
            if ($loadedBoundaryA.$captureName.Sha256 -ne $loadedBoundaryB.$captureName.Sha256) {
                throw "Cross-restart load-boundary capture differs: case=$($case.id); capture=$captureName"
            }
        }
        if ($loadedBoundaryA.particle.Records -ne
            (Convert-NonnegativeInteger $verifyA.Values.loaded_particles "loaded_particles")) {
            throw "Loaded load-boundary particle count differs from verify result: case=$($case.id)"
        }
        $loadBoundaryComparisonTempPath = Join-Path $testRoot "load-boundary-comparison.json"
        $loadBoundaryComparison = Invoke-LoadBoundaryComparison `
            -Before $beforeBoundary -Loaded $loadedBoundaryA `
            -OutputPath $loadBoundaryComparisonTempPath
        if ($loadBoundaryComparison.status -ne "PASS" -or
            -not $loadBoundaryComparison.claims.load_boundary_capture_complete) {
            throw "Load-boundary comparison did not report a complete capture: case=$($case.id)"
        }
        if ($loadBoundaryComparison.claims.physical_mass_conservation_evaluated -or
            $loadBoundaryComparison.claims.physical_momentum_conservation_evaluated -or
            $loadBoundaryComparison.claims.physical_energy_conservation_evaluated -or
            $loadBoundaryComparison.claims.source_sink_attribution_evaluated) {
            throw "Load-boundary comparator claimed physical accounting: case=$($case.id)"
        }
        $loadBoundaryComparisonSha256 = Get-FileSha256 -Path $loadBoundaryComparisonTempPath

        $caseArtifact = Join-Path $suiteArtifactRoot ($case.id + "-" + $case.slug)
        New-Item -ItemType Directory -Path $caseArtifact -Force | Out-Null
        Copy-Item -LiteralPath $firstOps.Path -Destination (
            Join-Path $caseArtifact ($case.id + "-" + $case.slug + ".stm")
        )
        foreach ($phaseRecord in @(
            [pscustomobject]@{ Name = "generate"; Data = $generation; Root = $generationRoot },
            [pscustomobject]@{ Name = "verify-a"; Data = $verifyA; Root = $verifyARoot },
            [pscustomobject]@{ Name = "verify-b"; Data = $verifyB; Root = $verifyBRoot }
        )) {
            Copy-Item -LiteralPath $phaseRecord.Data.ResultPath -Destination (
                Join-Path $caseArtifact ($phaseRecord.Name + ".result")
            )
            Copy-Item -LiteralPath $phaseRecord.Data.ConfigPath -Destination (
                Join-Path $caseArtifact ($phaseRecord.Name + ".config")
            )
            Copy-Item -LiteralPath $phaseRecord.Data.StdoutPath -Destination (
                Join-Path $caseArtifact ($phaseRecord.Name + "-stdout.log")
            )
            Copy-Item -LiteralPath $phaseRecord.Data.StderrPath -Destination (
                Join-Path $caseArtifact ($phaseRecord.Name + "-stderr.log")
            )
        }
        Copy-Item -LiteralPath $traceAPath -Destination (Join-Path $caseArtifact "trace-a.csv")
        Copy-Item -LiteralPath $traceBPath -Destination (Join-Path $caseArtifact "trace-b.csv")
        foreach ($captureRecord in @(
            [pscustomobject]@{ Prefix = "before-save"; Data = $beforeBoundary },
            [pscustomobject]@{ Prefix = "loaded-a"; Data = $loadedBoundaryA },
            [pscustomobject]@{ Prefix = "loaded-b"; Data = $loadedBoundaryB }
        )) {
            foreach ($captureName in @("particle", "cell", "settings")) {
                $artifactFileName = switch ($captureName) {
                    "particle" { "particles.csv" }
                    "cell" { "cells.csv" }
                    "settings" { "settings.csv" }
                    default { throw "Unknown load-boundary capture name: $captureName" }
                }
                Copy-Item -LiteralPath $captureRecord.Data.$captureName.Path -Destination (
                    Join-Path $caseArtifact ($captureRecord.Prefix + "-" + $artifactFileName)
                )
            }
        }
        Copy-Item -LiteralPath $loadBoundaryComparisonTempPath -Destination (
            Join-Path $caseArtifact "load-boundary-comparison.json"
        )

        $caseDefinitionText = $case | ConvertTo-Json -Depth 6 -Compress
        $caseRecord = [ordered]@{
            schema_version = 1
            case_id = [string]$case.id
            slug = [string]$case.slug
            title = [string]$case.title
            purpose = [string]$case.purpose
            definition_sha256 = Get-TextSha256 -Text $caseDefinitionText
            settings_profile = [string]$case.settings_profile
            seed = [uint64[]]$case.seed
            warmup_steps = [int]$case.warmup_steps
            trace_steps = [int]$case.trace_steps
            expected_particle_range = @(
                [int64]$case.expected_min_particles,
                [int64]$case.expected_max_particles
            )
            required_identifiers = [string[]]$case.required_identifiers
            generated_particles = Convert-NonnegativeInteger $generation.Values.generated_particles "generated_particles"
            generated_hash_fnv1a32 = [string]$generation.Values.generated_hash
            particles_before_save = Convert-NonnegativeInteger $generation.Values.particles_before_save "particles_before_save"
            hash_before_save_fnv1a32 = [string]$generation.Values.hash_before_save
            rng_before_save = [string]$generation.Values.rng_before_save
            required_counts_before_save = [string]$generation.Values.required_counts_before_save
            ops = [ordered]@{
                format = "OPS1+bzip2"
                length_bytes = $firstOps.Length
                payload_length_bytes = $firstOps.PayloadLength
                sha256 = $firstOps.Sha256
                duplicate_same_state_sha256 = $secondOps.Sha256
                raw_serialization_equal = $firstOps.Sha256 -eq $secondOps.Sha256
                canonical_payload_sha256 = [string]$firstCanonical.canonical_payload_sha256
                duplicate_canonical_payload_sha256 = [string]$secondCanonical.canonical_payload_sha256
                same_state_logical_payload_equal = $true
                canonicalization = [string]$firstCanonical.canonicalization
                volatile_stamp_name_count = [int]$firstCanonical.volatile_stamp_name_count
                volatile_author_date_count = [int]$firstCanonical.volatile_author_date_count
            }
            verification = [ordered]@{
                process_count = 2
                cross_restart_equal = $true
                loaded_particles = Convert-NonnegativeInteger $verifyA.Values.loaded_particles "loaded_particles"
                loaded_hash_fnv1a32 = [string]$verifyA.Values.loaded_hash
                loaded_rng = [string]$verifyA.Values.loaded_rng
                loaded_required_counts = [string]$verifyA.Values.loaded_required_counts
                final_particles = Convert-NonnegativeInteger $verifyA.Values.final_particles "final_particles"
                final_hash_fnv1a32 = [string]$verifyA.Values.final_hash
                final_rng = [string]$verifyA.Values.final_rng
                final_required_counts = [string]$verifyA.Values.final_required_counts
                trace_sha256 = $traceASha256
                trace_line_count = $traceLineCount
            }
            load_boundary = [ordered]@{
                load_boundary_capture_complete = $true
                comparison_kind = [string]$loadBoundaryComparison.comparison_kind
                comparison_sha256 = $loadBoundaryComparisonSha256
                cross_restart_capture_equal = $true
                particle_records_before_save = [int64]$beforeBoundary.particle.Records
                particle_records_loaded = [int64]$loadedBoundaryA.particle.Records
                particle_save_order_aligned_records = [int64]$loadBoundaryComparison.particles.save_order_pixel_alignment.aligned_records
                particle_save_order_misaligned_records = [int64]$loadBoundaryComparison.particles.save_order_pixel_alignment.misaligned_records
                particle_runtime_id_equal_records = [int64]$loadBoundaryComparison.particles.runtime_id.equal_records
                particle_runtime_id_changed_records = [int64]$loadBoundaryComparison.particles.runtime_id.changed_records
                particle_differing_records = [int64]$loadBoundaryComparison.particles.differing_records
                cell_records_before_save = [int64]$beforeBoundary.cell.Records
                cell_records_loaded = [int64]$loadedBoundaryA.cell.Records
                differing_cells = [int64]$loadBoundaryComparison.cells.differing_cells
                setting_records_before_save = [int64]$beforeBoundary.settings.Records
                setting_records_loaded = [int64]$loadedBoundaryA.settings.Records
                differing_settings = [int64]$loadBoundaryComparison.settings.differing_settings
                particle_payload_field_summary = $loadBoundaryComparison.particles.payload_field_summary
                cell_payload_field_summary = $loadBoundaryComparison.cells.payload_field_summary
                settings_field_summary = $loadBoundaryComparison.settings.field_summary
                particle_count_equal = $true
                required_counts_equal = $true
                rng_equal = $true
                snapshot_hash_equal = $verifyA.Values.loaded_hash -eq
                    $generation.Values.hash_before_save
                interpretation = "Field-attribution capture only: Legacy OPS may normalize or quantize state, reorder runtime IDs or reconstruct derived maps; unequal fields are reported, not silently accepted as bit-exact equality"
            }
            phase_process_metrics = @(
                [ordered]@{ phase = "generate"; wall_seconds = $generation.WallSeconds; cpu_seconds = $generation.CpuSeconds; peak_working_set_bytes = $generation.PeakWorkingSetBytes; peak_private_bytes = $generation.PeakPrivateBytes },
                [ordered]@{ phase = "verify-a"; wall_seconds = $verifyA.WallSeconds; cpu_seconds = $verifyA.CpuSeconds; peak_working_set_bytes = $verifyA.PeakWorkingSetBytes; peak_private_bytes = $verifyA.PeakPrivateBytes },
                [ordered]@{ phase = "verify-b"; wall_seconds = $verifyB.WallSeconds; cpu_seconds = $verifyB.CpuSeconds; peak_working_set_bytes = $verifyB.PeakWorkingSetBytes; peak_private_bytes = $verifyB.PeakPrivateBytes }
            )
            config_sha256 = [ordered]@{
                generate = $generation.ConfigSha256
                verify_a = $verifyA.ConfigSha256
                verify_b = $verifyB.ConfigSha256
            }
            artifact_policy = [string]$caseDocument.artifact_policy
        }
        $caseJsonPath = Join-Path $caseArtifact "result.json"
        [System.IO.File]::WriteAllText(
            $caseJsonPath,
            ($caseRecord | ConvertTo-Json -Depth 10) + [Environment]::NewLine,
            [System.Text.UTF8Encoding]::new($false)
        )
        $caseRecord["result_json_sha256"] = Get-FileSha256 -Path $caseJsonPath
        $caseResults.Add([pscustomobject]$caseRecord)
        $caseCompleted = $true
        Write-Output "characterization-case: PASS"
        Write-Output "case_id=$($case.id)"
        Write-Output "slug=$($case.slug)"
        Write-Output "ops_sha256=$($firstOps.Sha256)"
        Write-Output "ops_canonical_payload_sha256=$($firstCanonical.canonical_payload_sha256)"
        Write-Output "trace_sha256=$traceASha256"
    }
    finally {
        if ($caseCompleted -and -not $KeepTemporary -and (Test-Path -LiteralPath $testRoot)) {
            Remove-IsolatedRoot -Path $testRoot -Parent $tempParent
        }
    }
}

$manifest = [ordered]@{
    schema_version = 1
    suite_id = [string]$caseDocument.suite_id
    suite_run_id = $suiteRunId
    status = "PASS"
    case_count = $caseResults.Count
    expected_full_case_count = 14
    full_suite = $caseResults.Count -eq 14
    recorded_at_utc = [DateTime]::UtcNow.ToString("o")
    source = [ordered]@{
        commit = $gitState.Commit
        dirty = [bool]$gitState.Dirty
        worktree_state_sha256 = $gitState.StateSha256
        status_lines = [string[]]$gitState.StatusLines
        lua_sha256 = $luaSha256
        case_definitions_sha256 = $casesSha256
        ops_canonicalizer_sha256 = $canonicalizerSha256
        load_boundary_comparator_sha256 = $loadBoundaryComparatorSha256
        wrapper_sha256 = Get-FileSha256 -Path $PSCommandPath
    }
    executable = [ordered]@{
        sha256 = $executableSha256
        length_bytes = [int64]$executableInfo.Length
        build_directory = $resolvedBuildDirectory
        fp_mode = $build.FpMode
        compiler = $build.Compiler
        meson_options = $build.Options
        simulation_compile_command = $build.SimulationCompileCommand
        runtime_directory = $resolvedRuntimeDirectory
        portable_runtime_tested = $false
    }
    machine = [ordered]@{
        machine_id = $machineId
        os = "$($os.Caption) $($os.Version) build $($os.BuildNumber)"
        cpu_model = $cpuName
        logical_cpu_count = [int][Environment]::ProcessorCount
        physical_memory_bytes = [int64]$computer.TotalPhysicalMemory
        gpu = $gpu
        power_mode = $powerMode
        process_vram = "not_tested"
    }
    simulation_dt_value = [int]$caseDocument.simulation_dt_value
    simulation_dt_unit = [string]$caseDocument.simulation_dt_unit
    provenance = [string]$caseDocument.provenance
    artifact_policy = [string]$caseDocument.artifact_policy
    cases = [object[]]$caseResults
}
$manifestPath = Join-Path $suiteArtifactRoot "manifest.json"
[System.IO.File]::WriteAllText(
    $manifestPath,
    ($manifest | ConvertTo-Json -Depth 12) + [Environment]::NewLine,
    [System.Text.UTF8Encoding]::new($false)
)

Write-Output "runtime-generate-characterization: PASS"
Write-Output "suite_id=$($caseDocument.suite_id)"
Write-Output "case_count=$($caseResults.Count)"
Write-Output "full_suite=$(([bool]($caseResults.Count -eq 14)).ToString().ToLowerInvariant())"
Write-Output "source_commit=$($gitState.Commit)"
Write-Output "source_dirty=$($gitState.Dirty.ToString().ToLowerInvariant())"
Write-Output "manifest_sha256=$(Get-FileSha256 -Path $manifestPath)"
Write-Output "manifest_json=$manifestPath"
