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

    [ValidateRange(1, 1000000)]
    [int] $TotalSteps = 1000,

    [ValidateRange(1, 1000000)]
    [int] $SampleInterval = 10,

    [uint32] $SeedA = 101,
    [uint32] $SeedB = 202,
    [uint32] $SeedC = 303,
    [uint32] $SeedD = 404,

    [string] $RuntimeDirectory = "C:\msys64\ucrt64\bin",

    [string] $PythonExecutable = "C:\msys64\ucrt64\bin\python.exe",

    [string] $NinjaExecutable = "C:\msys64\ucrt64\bin\ninja.exe",

    [string] $OutputDirectory = "artifacts/vnext-legacy-ledger",

    [string] $TemporaryDirectory = [System.IO.Path]::GetTempPath(),

    [ValidateRange(30, 7200)]
    [int] $TimeoutSeconds = 1800,

    [switch] $Smoke,

    [switch] $KeepTemporary
)

$ErrorActionPreference = "Stop"
$ProgressPreference = "SilentlyContinue"

if ($Smoke) {
    $TotalSteps = 3
    $SampleInterval = 1
}
if ($SampleInterval -gt $TotalSteps) {
    throw "SampleInterval cannot exceed TotalSteps"
}
foreach ($label in @($LeftLabel, $RightLabel)) {
    if ($label -notmatch '^[A-Za-z0-9][A-Za-z0-9._-]{0,63}$') {
        throw "Unsafe Legacy ledger label: $label"
    }
}
if ($LeftLabel -eq $RightLabel) {
    throw "Legacy ledger labels must be distinct"
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
$resolvedNinja = (Resolve-Path -LiteralPath $NinjaExecutable).Path
$luaSource = Join-Path $PSScriptRoot "runtime\legacy_numerical_ledger.lua"
$comparatorSource = Join-Path $PSScriptRoot "legacy_ledger_compare.py"
foreach ($required in @($luaSource, $comparatorSource)) {
    if (-not (Test-Path -LiteralPath $required -PathType Leaf)) {
        throw "Missing Legacy ledger source: $required"
    }
}

function Get-TextSha256 {
    param([Parameter(Mandatory = $true)][string] $Text)
    return Get-BytesSha256 -Bytes ([System.Text.Encoding]::UTF8.GetBytes($Text))
}

function Get-BytesSha256 {
    param([Parameter(Mandatory = $true)][byte[]] $Bytes)
    $hasher = [System.Security.Cryptography.SHA256]::Create()
    try {
        return ([BitConverter]::ToString($hasher.ComputeHash($Bytes))).Replace("-", "")
    }
    finally {
        $hasher.Dispose()
    }
}

function Get-ExpectedSampleSteps {
    $steps = [System.Collections.Generic.SortedSet[int]]::new()
    [void]$steps.Add(0)
    [void]$steps.Add(1)
    [void]$steps.Add($TotalSteps)
    for ($step = $SampleInterval; $step -le $TotalSteps; $step += $SampleInterval) {
        [void]$steps.Add($step)
    }
    return [int[]]$steps
}

function Read-KeyValueFile {
    param([Parameter(Mandatory = $true)][string] $Path)
    $values = @{}
    foreach ($line in Get-Content -LiteralPath $Path) {
        if ($line -notmatch '^([A-Za-z0-9_]+)=(.*)$') {
            throw "Invalid Legacy ledger result line: $line"
        }
        if ($values.ContainsKey($matches[1])) {
            throw "Duplicate Legacy ledger result key: $($matches[1])"
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
        throw "Legacy ledger result field is not a nonnegative integer: $Field"
    }
    return [int64]$Text
}

function Convert-UInt32Text {
    param(
        [Parameter(Mandatory = $true)][string] $Text,
        [Parameter(Mandatory = $true)][string] $Field
    )
    $value = Convert-NonnegativeInteger -Text $Text -Field $Field
    if ($value -gt [uint32]::MaxValue) {
        throw "Legacy ledger result field exceeds uint32: $Field"
    }
    return [uint32]$value
}

function Stop-ProcessTree {
    param([Parameter(Mandatory = $true)][System.Diagnostics.Process] $Process)
    if ($Process.HasExited) {
        return
    }
    try {
        $Process.Kill($true)
    }
    catch {
        $taskkill = Join-Path ([Environment]::SystemDirectory) "taskkill.exe"
        if (Test-Path -LiteralPath $taskkill -PathType Leaf) {
            & $taskkill /PID ([string]$Process.Id) /T /F 2>$null | Out-Null
            [void]$Process.WaitForExit(5000)
        }
        if (-not $Process.HasExited) {
            $Process.Kill()
        }
    }
    $Process.WaitForExit()
}

function Set-MinimalChildEnvironment {
    param([Parameter(Mandatory = $true)][System.Diagnostics.ProcessStartInfo] $StartInfo)
    $StartInfo.Environment.Clear()
    $StartInfo.Environment["PATH"] = $resolvedRuntime +
        [System.IO.Path]::PathSeparator + [Environment]::SystemDirectory
    foreach ($name in @("SystemRoot", "WINDIR", "TEMP", "TMP")) {
        $value = [Environment]::GetEnvironmentVariable($name)
        if ($value) {
            $StartInfo.Environment[$name] = $value
        }
    }
    foreach ($secretName in @("GITHUB_PAT_TOKEN", "GITHUB_TOKEN", "GH_TOKEN")) {
        [void]$StartInfo.Environment.Remove($secretName)
    }
}

function ConvertTo-WindowsProcessArgument {
    param(
        [Parameter(Mandatory = $true)]
        [AllowEmptyString()]
        [string] $Argument
    )
    $quoted = [System.Text.StringBuilder]::new()
    [void]$quoted.Append('"')
    $backslashes = 0
    foreach ($character in $Argument.ToCharArray()) {
        if ($character -eq '\') {
            $backslashes += 1
            continue
        }
        if ($character -eq '"') {
            if ($backslashes -gt 0) {
                [void]$quoted.Append(('\' * ($backslashes * 2)))
            }
            [void]$quoted.Append('\"')
            $backslashes = 0
            continue
        }
        if ($backslashes -gt 0) {
            [void]$quoted.Append(('\' * $backslashes))
            $backslashes = 0
        }
        [void]$quoted.Append($character)
    }
    if ($backslashes -gt 0) {
        [void]$quoted.Append(('\' * ($backslashes * 2)))
    }
    [void]$quoted.Append('"')
    return $quoted.ToString()
}

function Set-ProcessArguments {
    param(
        [Parameter(Mandatory = $true)]
        [System.Diagnostics.ProcessStartInfo] $StartInfo,
        [Parameter(Mandatory = $true)]
        [AllowEmptyCollection()]
        [string[]] $Arguments
    )
    $StartInfo.Arguments = (@(
        $Arguments | ForEach-Object { ConvertTo-WindowsProcessArgument -Argument $_ }
    ) -join " ")
}

function Invoke-CapturedCommand {
    param(
        [Parameter(Mandatory = $true)][string] $FileName,
        [Parameter(Mandatory = $true)][string[]] $Arguments,
        [Parameter(Mandatory = $true)][string] $WorkingDirectory,
        [Parameter(Mandatory = $true)][string] $Label,
        [ValidateRange(1, 600)][int] $CommandTimeoutSeconds = 120
    )
    $startInfo = [System.Diagnostics.ProcessStartInfo]::new()
    $startInfo.FileName = $FileName
    $startInfo.WorkingDirectory = $WorkingDirectory
    $startInfo.UseShellExecute = $false
    $startInfo.CreateNoWindow = $true
    $startInfo.RedirectStandardOutput = $true
    $startInfo.RedirectStandardError = $true
    Set-ProcessArguments -StartInfo $startInfo -Arguments $Arguments
    Set-MinimalChildEnvironment -StartInfo $startInfo

    $process = [System.Diagnostics.Process]::Start($startInfo)
    if (-not $process) {
        throw "Failed to start captured command: $Label"
    }
    $stdoutTask = $process.StandardOutput.ReadToEndAsync()
    $stderrTask = $process.StandardError.ReadToEndAsync()
    try {
        if (-not $process.WaitForExit($CommandTimeoutSeconds * 1000)) {
            Stop-ProcessTree -Process $process
            throw "Captured command timed out: $Label"
        }
        $process.WaitForExit()
        return [pscustomobject]@{
            ExitCode = $process.ExitCode
            Stdout = $stdoutTask.GetAwaiter().GetResult()
            Stderr = $stderrTask.GetAwaiter().GetResult()
        }
    }
    finally {
        if (-not $process.HasExited) {
            Stop-ProcessTree -Process $process
        }
        $process.Dispose()
    }
}

function Get-FileIdentity {
    param([Parameter(Mandatory = $true)][string] $Path)
    $item = Get-Item -LiteralPath $Path
    $version = [System.Diagnostics.FileVersionInfo]::GetVersionInfo($item.FullName)
    return [ordered]@{
        path = $item.FullName
        length_bytes = [int64]$item.Length
        sha256 = (Get-FileHash -LiteralPath $item.FullName -Algorithm SHA256).Hash
        file_version = if ($version.FileVersion) { [string]$version.FileVersion } else { "not_available" }
        product_version = if ($version.ProductVersion) { [string]$version.ProductVersion } else { "not_available" }
    }
}

function Get-RuntimeDllInventory {
    return @(
        Get-ChildItem -LiteralPath $resolvedRuntime -File -Filter "*.dll" |
            Sort-Object Name |
            ForEach-Object {
                [ordered]@{
                    name = $_.Name
                    length_bytes = [int64]$_.Length
                    sha256 = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash
                }
            }
    )
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
        throw "Refusing to remove Legacy ledger data outside the selected temporary root"
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
                Write-Warning "Unable to remove isolated Legacy ledger directory: $fullPath"
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
        throw "Cannot resolve Legacy ledger source HEAD"
    }
    $status = @(& git -C $Repository status --porcelain=v1 --untracked-files=all)
    if ($LASTEXITCODE -ne 0) {
        throw "Cannot inspect Legacy ledger source worktree"
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
        throw "Cannot hash tracked Legacy ledger changes"
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
        throw "Cannot enumerate untracked Legacy ledger files"
    }
    return [pscustomobject]@{
        Commit = $head
        Dirty = $status.Count -gt 0
        StateSha256 = Get-TextSha256 -Text $material.ToString()
        StatusLines = [string[]]$status
    }
}

function Get-BuildProvenance {
    param(
        [Parameter(Mandatory = $true)][string] $Directory,
        [Parameter(Mandatory = $true)][string] $Executable
    )
    $optionsPath = Join-Path $Directory "meson-info\intro-buildoptions.json"
    $compilersPath = Join-Path $Directory "meson-info\intro-compilers.json"
    $commandsPath = Join-Path $Directory "compile_commands.json"
    $targetsPath = Join-Path $Directory "meson-info\intro-targets.json"
    $mesonInfoPath = Join-Path $Directory "meson-info\meson-info.json"
    foreach ($path in @(
        $optionsPath, $compilersPath, $commandsPath, $targetsPath, $mesonInfoPath
    )) {
        if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
            throw "Missing Legacy ledger build provenance file: $path"
        }
    }
    $options = [ordered]@{}
    foreach ($row in (Get-Content -LiteralPath $optionsPath -Raw | ConvertFrom-Json)) {
        $options[$row.name] = $row.value
    }
    if (-not $options.Contains("fp_mode")) {
        throw "Legacy ledger build does not expose fp_mode"
    }
    if (-not $options.Contains("app_exe")) {
        throw "Legacy ledger build does not expose app_exe"
    }
    $mesonInfo = Get-Content -LiteralPath $mesonInfoPath -Raw | ConvertFrom-Json
    $mesonSource = [System.IO.Path]::GetFullPath([string]$mesonInfo.directories.source)
    $mesonBuild = [System.IO.Path]::GetFullPath([string]$mesonInfo.directories.build)
    if (-not $mesonSource.Equals(
        [System.IO.Path]::GetFullPath($sourceRoot),
        [System.StringComparison]::OrdinalIgnoreCase
    )) {
        throw "Legacy ledger build source root mismatch: $mesonSource"
    }
    if (-not $mesonBuild.Equals(
        [System.IO.Path]::GetFullPath($Directory),
        [System.StringComparison]::OrdinalIgnoreCase
    )) {
        throw "Legacy ledger build directory identity mismatch: $mesonBuild"
    }
    $targets = Get-Content -LiteralPath $targetsPath -Raw | ConvertFrom-Json
    $powderTargets = @($targets | Where-Object {
        $_.type -eq "executable" -and $_.name -eq [string]$options["app_exe"]
    })
    if ($powderTargets.Count -ne 1 -or @($powderTargets[0].filename).Count -ne 1) {
        throw "Cannot identify one Meson powder executable target"
    }
    $targetExecutable = [System.IO.Path]::GetFullPath(
        [string]@($powderTargets[0].filename)[0]
    )
    if (-not $targetExecutable.Equals(
        [System.IO.Path]::GetFullPath($Executable),
        [System.StringComparison]::OrdinalIgnoreCase
    )) {
        throw "Executable does not match the Meson powder target: expected=$targetExecutable"
    }
    $commands = Get-Content -LiteralPath $commandsPath -Raw | ConvertFrom-Json
    $compileCommands = [ordered]@{}
    foreach ($row in $commands) {
        $file = ([string]$row.file) -replace '\\', '/'
        $output = ([string]$row.output) -replace '\\', '/'
        if (-not $file -or -not $output -or -not $row.command) {
            throw "Legacy ledger compile command is missing file, output or command"
        }
        if ($compileCommands.Contains($output)) {
            throw "Duplicate Legacy ledger compile output: $output"
        }
        $compileCommands[$output] = [pscustomobject]@{
            SourceFile = $file
            Command = [string]$row.command
        }
    }
    if ($compileCommands.Count -eq 0) {
        throw "Legacy ledger build has no compile commands"
    }
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
        CompileCommands = $compileCommands
        CompileCommandCount = [int]$compileCommands.Count
        SourceRoot = $mesonSource
        BuildDirectory = $mesonBuild
        TargetExecutable = $targetExecutable
    }
}

function Assert-BuildPairCompatible {
    param(
        [Parameter(Mandatory = $true)] $Left,
        [Parameter(Mandatory = $true)] $Right
    )
    $leftCompiler = $Left.Compiler | ConvertTo-Json -Depth 20 -Compress
    $rightCompiler = $Right.Compiler | ConvertTo-Json -Depth 20 -Compress
    if ($leftCompiler -ne $rightCompiler) {
        throw "Legacy ledger compiler identities differ"
    }
    $leftKeys = @($Left.Options.Keys | Sort-Object)
    $rightKeys = @($Right.Options.Keys | Sort-Object)
    if (($leftKeys -join "`n") -ne ($rightKeys -join "`n")) {
        throw "Legacy ledger Meson option keys differ"
    }
    foreach ($key in $leftKeys) {
        if ($key -eq "fp_mode") {
            continue
        }
        $leftValue = $Left.Options[$key] | ConvertTo-Json -Depth 20 -Compress
        $rightValue = $Right.Options[$key] | ConvertTo-Json -Depth 20 -Compress
        if ($leftValue -ne $rightValue) {
            throw "Legacy ledger Meson options differ outside fp_mode: $key"
        }
    }
}

function Assert-FpCompileContract {
    param(
        [Parameter(Mandatory = $true)] $Build,
        [Parameter(Mandatory = $true)][ValidateSet("legacy_fast", "strict")][string] $Mode
    )
    $compilerId = [string]$Build.Compiler.id
    $command = [string]$Build.SimulationCompileCommand
    if ($compilerId -eq "gcc" -or $compilerId -eq "clang") {
        if ($Mode -eq "legacy_fast") {
            if ($command -notmatch '(?:^|\s)"?-ffast-math"?(?:\s|$)' -or
                $command -match '(?:^|\s)"?-fno-fast-math"?(?:\s|$)') {
                throw "Legacy-fast compile command does not enforce the expected FP contract"
            }
        }
        elseif ($command -notmatch '(?:^|\s)"?-fno-fast-math"?(?:\s|$)' -or
            $command -notmatch '(?:^|\s)"?-fno-unsafe-math-optimizations"?(?:\s|$)' -or
            $command -notmatch '(?:^|\s)"?-ffp-contract=off"?(?:\s|$)') {
            throw "Strict compile command does not enforce the expected FP contract"
        }
    }
    elseif ($compilerId -eq "msvc") {
        $expected = if ($Mode -eq "legacy_fast") { "/fp:fast" } else { "/fp:strict" }
        if ($command -notmatch [regex]::Escape($expected)) {
            throw "MSVC compile command does not enforce $expected"
        }
    }
    else {
        throw "Unsupported compiler for Legacy ledger FP validation: $compilerId"
    }
}

function Assert-CommandFpContract {
    param(
        [Parameter(Mandatory = $true)][string] $Command,
        [Parameter(Mandatory = $true)][string] $CompilerId,
        [Parameter(Mandatory = $true)][ValidateSet("legacy_fast", "strict")][string] $Mode,
        [Parameter(Mandatory = $true)][string] $SourceFile
    )
    if ($CompilerId -eq "gcc" -or $CompilerId -eq "clang") {
        if ($Mode -eq "legacy_fast") {
            if ($Command -notmatch '(?:^|\s)"?-ffast-math"?(?:\s|$)' -or
                $Command -notmatch '(?:^|\s)"?-funsafe-math-optimizations"?(?:\s|$)' -or
                $Command -match '(?:^|\s)"?-fno-fast-math"?(?:\s|$)') {
                throw "Legacy-fast FP flags are invalid for compile unit: $SourceFile"
            }
        }
        elseif ($Command -notmatch '(?:^|\s)"?-fno-fast-math"?(?:\s|$)' -or
            $Command -notmatch '(?:^|\s)"?-fno-unsafe-math-optimizations"?(?:\s|$)' -or
            $Command -notmatch '(?:^|\s)"?-ffp-contract=off"?(?:\s|$)' -or
            $Command -match '(?:^|\s)"?-ffast-math"?(?:\s|$)' -or
            $Command -match '(?:^|\s)"?-funsafe-math-optimizations"?(?:\s|$)') {
            throw "Strict FP flags are invalid for compile unit: $SourceFile"
        }
    }
    elseif ($CompilerId -eq "msvc") {
        $expected = if ($Mode -eq "legacy_fast") { "/fp:fast" } else { "/fp:strict" }
        if ($Command -notmatch [regex]::Escape($expected)) {
            throw "MSVC compile unit does not enforce $expected`: $SourceFile"
        }
    }
    else {
        throw "Unsupported compiler for compile-unit FP validation: $CompilerId"
    }
}

function Get-NormalizedFpCompileCommand {
    param(
        [Parameter(Mandatory = $true)][string] $Command,
        [Parameter(Mandatory = $true)][string] $CompilerId
    )
    $flags = if ($CompilerId -eq "msvc") {
        @("/fp:fast", "/fp:strict")
    }
    else {
        @(
            "-ffast-math", "-funsafe-math-optimizations", "-fno-fast-math",
            "-fno-unsafe-math-optimizations", "-ffp-contract=off"
        )
    }
    $normalized = $Command
    foreach ($flag in $flags) {
        $pattern = '(?<!\S)"?' + [regex]::Escape($flag) + '"?(?!\S)'
        $normalized = [regex]::Replace($normalized, $pattern, "")
    }
    return ([regex]::Replace($normalized, '\s+', " ")).Trim()
}

function Assert-AllCompileCommandsCompatible {
    param(
        [Parameter(Mandatory = $true)] $Left,
        [Parameter(Mandatory = $true)] $Right
    )
    $leftOutputs = @($Left.CompileCommands.Keys | Sort-Object)
    $rightOutputs = @($Right.CompileCommands.Keys | Sort-Object)
    if (($leftOutputs -join "`n") -ne ($rightOutputs -join "`n")) {
        throw "Legacy ledger compile-unit sets differ"
    }
    $compilerId = [string]$Left.Compiler.id
    foreach ($output in $leftOutputs) {
        $leftRecord = $Left.CompileCommands[$output]
        $rightRecord = $Right.CompileCommands[$output]
        if ($leftRecord.SourceFile -ne $rightRecord.SourceFile) {
            throw "Compile output source files differ: $output"
        }
        $file = [string]$leftRecord.SourceFile
        $leftCommand = [string]$leftRecord.Command
        $rightCommand = [string]$rightRecord.Command
        Assert-CommandFpContract -Command $leftCommand -CompilerId $compilerId `
            -Mode "legacy_fast" -SourceFile $file
        Assert-CommandFpContract -Command $rightCommand -CompilerId $compilerId `
            -Mode "strict" -SourceFile $file
        $leftNormalized = Get-NormalizedFpCompileCommand `
            -Command $leftCommand -CompilerId $compilerId
        $rightNormalized = Get-NormalizedFpCompileCommand `
            -Command $rightCommand -CompilerId $compilerId
        if ($leftNormalized -ne $rightNormalized) {
            throw "Compile commands differ outside the FP contract: $output ($file)"
        }
    }
}

function Invoke-BuildTarget {
    param([Parameter(Mandatory = $true)] $Build)
    $target = Split-Path -Leaf $Build.TargetExecutable
    $record = Invoke-CapturedCommand -FileName $resolvedNinja `
        -Arguments @("-C", $Build.BuildDirectory, $target) `
        -WorkingDirectory $sourceRoot -Label ("ninja build " + $target) `
        -CommandTimeoutSeconds ([Math]::Min($TimeoutSeconds, 600))
    $combined = ($record.Stdout + [Environment]::NewLine + $record.Stderr).Trim()
    if ($record.ExitCode -ne 0 -or
        -not (Test-Path -LiteralPath $Build.TargetExecutable -PathType Leaf)) {
        throw "Legacy ledger target build failed for $target`: $combined"
    }
    return [ordered]@{
        method = "ninja_target_build"
        target = $target
        result = "built_successfully_before_execution"
        source_commit_embedded_in_executable = "not_verified"
    }
}

function Get-BuildProvenanceSha256 {
    param([Parameter(Mandatory = $true)] $Build)
    $material = [ordered]@{
        fp_mode = $Build.FpMode
        options = $Build.Options
        compiler = $Build.Compiler
        compile_commands = $Build.CompileCommands
        source_root = $Build.SourceRoot
        build_directory = $Build.BuildDirectory
        target_executable = $Build.TargetExecutable
    }
    return Get-TextSha256 -Text ($material | ConvertTo-Json -Depth 30 -Compress)
}

function Write-ProbeFiles {
    param([Parameter(Mandatory = $true)][string] $Root)
    New-Item -ItemType Directory -Path $Root | Out-Null
    $utf8NoBom = [System.Text.UTF8Encoding]::new($false)
    [System.IO.File]::WriteAllText(
        (Join-Path $Root "powder.pref"),
        ('{"LuaHookTimeout":' + ($TimeoutSeconds * 1000) + '}') + [Environment]::NewLine,
        $utf8NoBom
    )
    [System.IO.File]::WriteAllBytes((Join-Path $Root "autorun.lua"), $frozenLuaBytes)
    $configPath = Join-Path $Root "legacy-ledger.config"
    [System.IO.File]::WriteAllLines(
        $configPath,
        @(
            "schema_version=1",
            "scenario=$Scenario",
            "total_steps=$TotalSteps",
            "sample_interval=$SampleInterval",
            "seed_a=$SeedA",
            "seed_b=$SeedB",
            "seed_c=$SeedC",
            "seed_d=$SeedD"
        ),
        [System.Text.Encoding]::ASCII
    )
    return $configPath
}

function Resolve-ProbeOutput {
    param(
        [Parameter(Mandatory = $true)][string] $Root,
        [Parameter(Mandatory = $true)][string] $ReportedName,
        [Parameter(Mandatory = $true)][string] $ExpectedName
    )
    if ($ReportedName -ne $ExpectedName) {
        throw "Unexpected Legacy ledger output name: $ReportedName"
    }
    $rootPrefix = [System.IO.Path]::GetFullPath($Root).TrimEnd(
        [System.IO.Path]::DirectorySeparatorChar,
        [System.IO.Path]::AltDirectorySeparatorChar
    ) + [System.IO.Path]::DirectorySeparatorChar
    $path = [System.IO.Path]::GetFullPath((Join-Path $Root $ReportedName))
    if (-not $path.StartsWith($rootPrefix, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Legacy ledger output escapes the isolated probe root: $ReportedName"
    }
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
        throw "Legacy ledger output missing: $path"
    }
    return $path
}

function Invoke-Probe {
    param(
        [Parameter(Mandatory = $true)][string] $Executable,
        [Parameter(Mandatory = $true)][string] $Root,
        [Parameter(Mandatory = $true)][string] $Label
    )
    $configPath = Write-ProbeFiles -Root $Root
    $resultPath = Join-Path $Root "legacy-ledger.result"
    $startInfo = [System.Diagnostics.ProcessStartInfo]::new()
    $startInfo.FileName = $Executable
    $startInfo.WorkingDirectory = $Root
    $startInfo.UseShellExecute = $false
    $startInfo.CreateNoWindow = $true
    $startInfo.RedirectStandardOutput = $true
    $startInfo.RedirectStandardError = $true
    Set-ProcessArguments -StartInfo $startInfo -Arguments @("ddir", $Root)
    Set-MinimalChildEnvironment -StartInfo $startInfo

    $started = [DateTime]::UtcNow
    $process = [System.Diagnostics.Process]::Start($startInfo)
    if (-not $process) {
        throw "Failed to start Legacy ledger probe: label=$Label"
    }
    $stdoutTask = $process.StandardOutput.ReadToEndAsync()
    $stderrTask = $process.StandardError.ReadToEndAsync()
    $maxSampledWorkingSet = [int64]0
    $maxSampledPrivateBytes = [int64]0
    $timedOut = $false
    try {
        $deadline = $started.AddSeconds($TimeoutSeconds)
        do {
            $process.Refresh()
            if (-not $process.HasExited) {
                $maxSampledWorkingSet = [Math]::Max(
                    $maxSampledWorkingSet, [int64]$process.WorkingSet64
                )
                $maxSampledPrivateBytes = [Math]::Max(
                    $maxSampledPrivateBytes, [int64]$process.PrivateMemorySize64
                )
                Start-Sleep -Milliseconds 50
            }
        } while (-not $process.HasExited -and [DateTime]::UtcNow -lt $deadline)
        if (-not $process.HasExited) {
            $timedOut = $true
            Stop-ProcessTree -Process $process
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
        if ($timedOut) {
            throw "Legacy ledger probe timed out: label=$Label; root=$Root; stdout=$stdoutPath; stderr=$stderrPath"
        }
        if (-not (Test-Path -LiteralPath $resultPath -PathType Leaf)) {
            throw "Legacy ledger probe produced no result: label=$Label; exit=$($process.ExitCode); root=$Root"
        }
        $values = Read-KeyValueFile -Path $resultPath
        if ($process.ExitCode -ne 0 -or $values.OMNI_LEGACY_LEDGER_STATUS -ne "PASS") {
            throw "Legacy ledger probe failed: label=$Label; exit=$($process.ExitCode); error=$($values.error); root=$Root"
        }
        foreach ($identity in @(
            [pscustomobject]@{ Field = "schema_version"; Expected = "1" },
            [pscustomobject]@{ Field = "scenario"; Expected = $Scenario },
            [pscustomobject]@{ Field = "seed"; Expected = "$SeedA,$SeedB,$SeedC,$SeedD" },
            [pscustomobject]@{ Field = "total_steps"; Expected = [string]$TotalSteps },
            [pscustomobject]@{ Field = "sample_interval"; Expected = [string]$SampleInterval },
            [pscustomobject]@{ Field = "proxy_contract"; Expected = "legacy_state_proxies_not_physical_units" },
            [pscustomobject]@{ Field = "physical_mass_conservation_evaluated"; Expected = "false" },
            [pscustomobject]@{ Field = "physical_energy_conservation_evaluated"; Expected = "false" },
            [pscustomobject]@{ Field = "physical_momentum_conservation_evaluated"; Expected = "false" },
            [pscustomobject]@{ Field = "source_sink_attribution_evaluated"; Expected = "false" },
            [pscustomobject]@{ Field = "correction_events_evaluated"; Expected = "false" }
        )) {
            if ($values[$identity.Field] -ne $identity.Expected) {
                throw "Legacy ledger identity mismatch: label=$Label; field=$($identity.Field)"
            }
        }
        $ledgerPath = Resolve-ProbeOutput -Root $Root `
            -ReportedName $values.ledger_file -ExpectedName "ledger.csv"
        $typeCountsPath = Resolve-ProbeOutput -Root $Root `
            -ReportedName $values.type_counts_file -ExpectedName "type-counts.csv"

        $expectedSteps = @(Get-ExpectedSampleSteps)
        $ledgerRows = @(Import-Csv -LiteralPath $ledgerPath)
        $samples = Convert-NonnegativeInteger -Text $values.samples -Field "samples"
        if ($samples -ne $expectedSteps.Count -or $ledgerRows.Count -ne $expectedSteps.Count) {
            throw "Legacy ledger sample count mismatch: label=$Label"
        }
        $actualSteps = @($ledgerRows | ForEach-Object {
            Convert-NonnegativeInteger -Text $_.step -Field "ledger.step"
        })
        if (($actualSteps -join ",") -ne ($expectedSteps -join ",")) {
            throw "Legacy ledger sample schedule mismatch: label=$Label"
        }

        $generatedParticles = Convert-NonnegativeInteger `
            -Text $values.generated_particles -Field "generated_particles"
        $initialParticles = Convert-NonnegativeInteger `
            -Text $values.initial_particles -Field "initial_particles"
        $finalParticles = Convert-NonnegativeInteger `
            -Text $values.final_particles -Field "final_particles"
        $initialHash = Convert-UInt32Text `
            -Text $values.initial_state_hash -Field "initial_state_hash"
        $finalHash = Convert-UInt32Text `
            -Text $values.final_state_hash -Field "final_state_hash"
        $atmosphereCells = Convert-NonnegativeInteger `
            -Text $values.atmosphere_cells -Field "atmosphere_cells"
        if ($atmosphereCells -le 0 -or $generatedParticles -ne $initialParticles) {
            throw "Legacy ledger result has invalid initial counts: label=$Label"
        }
        $firstRow = $ledgerRows[0]
        $lastRow = $ledgerRows[-1]
        if ((Convert-UInt32Text -Text $firstRow.state_hash_fnv1a32 -Field "first.hash") -ne $initialHash -or
            (Convert-UInt32Text -Text $lastRow.state_hash_fnv1a32 -Field "last.hash") -ne $finalHash -or
            (Convert-NonnegativeInteger -Text $firstRow.particles -Field "first.particles") -ne $initialParticles -or
            (Convert-NonnegativeInteger -Text $lastRow.particles -Field "last.particles") -ne $finalParticles) {
            throw "Legacy ledger result does not match CSV endpoints: label=$Label"
        }
        foreach ($row in $ledgerRows) {
            if ((Convert-NonnegativeInteger `
                -Text $row.atmosphere_cells -Field "ledger.atmosphere_cells") -ne $atmosphereCells) {
                throw "Legacy ledger atmosphere cell count drifted: label=$Label"
            }
        }

        $observationGroups = [ordered]@{
            nonfinite_observations = @(
                "particle_nonfinite_x", "particle_nonfinite_y", "particle_nonfinite_vx",
                "particle_nonfinite_vy", "particle_nonfinite_temp", "air_nonfinite_pressure",
                "air_nonfinite_velocity_x", "air_nonfinite_velocity_y",
                "air_nonfinite_ambient_heat"
            )
            range_violation_observations = @(
                "particle_position_out_of_bounds", "particle_temp_below_min",
                "particle_temp_above_max", "air_pressure_below_min",
                "air_pressure_above_max", "air_velocity_x_below_min",
                "air_velocity_x_above_max", "air_velocity_y_below_min",
                "air_velocity_y_above_max", "air_ambient_heat_below_min",
                "air_ambient_heat_above_max"
            )
            bound_occupancy_observations = @(
                "particle_temp_at_min", "particle_temp_at_max", "air_pressure_at_min",
                "air_pressure_at_max", "air_velocity_x_at_min", "air_velocity_x_at_max",
                "air_velocity_y_at_min", "air_velocity_y_at_max",
                "air_ambient_heat_at_min", "air_ambient_heat_at_max"
            )
        }
        foreach ($group in $observationGroups.GetEnumerator()) {
            $csvTotal = [int64]0
            foreach ($row in $ledgerRows) {
                foreach ($field in $group.Value) {
                    $csvTotal += Convert-NonnegativeInteger `
                        -Text $row.$field -Field ("ledger." + $field)
                }
            }
            $reportedTotal = Convert-NonnegativeInteger `
                -Text $values[$group.Key] -Field $group.Key
            if ($csvTotal -ne $reportedTotal) {
                throw "Legacy ledger observation total mismatch: label=$Label; field=$($group.Key)"
            }
        }
        return [pscustomobject]@{
            Label = $Label
            Root = $Root
            Values = $values
            ResultPath = $resultPath
            ConfigPath = $configPath
            LedgerPath = $ledgerPath
            TypeCountsPath = $typeCountsPath
            StdoutPath = $stdoutPath
            StderrPath = $stderrPath
            WallSeconds = [Math]::Round(([DateTime]::UtcNow - $started).TotalSeconds, 6)
            CpuSeconds = [Math]::Round($process.TotalProcessorTime.TotalSeconds, 6)
            MaxSampledWorkingSetBytes = $maxSampledWorkingSet
            MaxSampledPrivateBytes = $maxSampledPrivateBytes
        }
    }
    finally {
        if (-not $process.HasExited) {
            Stop-ProcessTree -Process $process
        }
        $process.Dispose()
    }
}

function Invoke-Comparator {
    param(
        [Parameter(Mandatory = $true)] $LeftRecord,
        [Parameter(Mandatory = $true)] $RightRecord,
        [Parameter(Mandatory = $true)][string] $OutputPath,
        [Parameter(Mandatory = $true)][string] $WorkingDirectory
    )
    $stdoutPath = Join-Path $WorkingDirectory "comparator-stdout.log"
    $stderrPath = Join-Path $WorkingDirectory "comparator-stderr.log"
    $startInfo = [System.Diagnostics.ProcessStartInfo]::new()
    $startInfo.FileName = $resolvedPython
    $startInfo.WorkingDirectory = $WorkingDirectory
    $startInfo.UseShellExecute = $false
    $startInfo.CreateNoWindow = $true
    $startInfo.RedirectStandardOutput = $true
    $startInfo.RedirectStandardError = $true
    Set-ProcessArguments -StartInfo $startInfo -Arguments @(
        $frozenComparatorPath,
        "--left-ledger", $LeftRecord.LedgerPath,
        "--right-ledger", $RightRecord.LedgerPath,
        "--left-types", $LeftRecord.TypeCountsPath,
        "--right-types", $RightRecord.TypeCountsPath,
        "--total-steps", [string]$TotalSteps,
        "--sample-interval", [string]$SampleInterval,
        "--output", $OutputPath
    )
    Set-MinimalChildEnvironment -StartInfo $startInfo
    $startInfo.Environment["PYTHONDONTWRITEBYTECODE"] = "1"
    $startInfo.Environment["PYTHONUTF8"] = "1"

    $process = [System.Diagnostics.Process]::Start($startInfo)
    if (-not $process) {
        throw "Failed to start Legacy ledger comparator"
    }
    $stdoutTask = $process.StandardOutput.ReadToEndAsync()
    $stderrTask = $process.StandardError.ReadToEndAsync()
    $timedOut = $false
    try {
        if (-not $process.WaitForExit($TimeoutSeconds * 1000)) {
            $timedOut = $true
            Stop-ProcessTree -Process $process
        }
        $process.WaitForExit()
        $stdout = $stdoutTask.GetAwaiter().GetResult()
        $stderr = $stderrTask.GetAwaiter().GetResult()
        [System.IO.File]::WriteAllText(
            $stdoutPath, $stdout, [System.Text.UTF8Encoding]::new($false)
        )
        [System.IO.File]::WriteAllText(
            $stderrPath, $stderr, [System.Text.UTF8Encoding]::new($false)
        )
        if ($timedOut) {
            throw "Legacy ledger comparator timed out; stdout=$stdoutPath; stderr=$stderrPath"
        }
        if ($process.ExitCode -ne 0 -or
            -not (Test-Path -LiteralPath $OutputPath -PathType Leaf)) {
            throw "Legacy ledger comparator failed: exit=$($process.ExitCode); stderr=$stderrPath"
        }
        return [pscustomobject]@{
            StdoutPath = $stdoutPath
            StderrPath = $stderrPath
        }
    }
    finally {
        if (-not $process.HasExited) {
            Stop-ProcessTree -Process $process
        }
        $process.Dispose()
    }
}

function Copy-ProbeRecord {
    param(
        [Parameter(Mandatory = $true)] $Record,
        [Parameter(Mandatory = $true)][string] $Destination
    )
    $prefix = $Record.Label
    Copy-Item -LiteralPath $Record.ResultPath -Destination (
        Join-Path $Destination ($prefix + ".result")
    )
    Copy-Item -LiteralPath $Record.ConfigPath -Destination (
        Join-Path $Destination ($prefix + ".config")
    )
    Copy-Item -LiteralPath $Record.LedgerPath -Destination (
        Join-Path $Destination ($prefix + "-ledger.csv")
    )
    Copy-Item -LiteralPath $Record.TypeCountsPath -Destination (
        Join-Path $Destination ($prefix + "-type-counts.csv")
    )
    Copy-Item -LiteralPath $Record.StdoutPath -Destination (
        Join-Path $Destination ($prefix + "-stdout.log")
    )
    Copy-Item -LiteralPath $Record.StderrPath -Destination (
        Join-Path $Destination ($prefix + "-stderr.log")
    )
}

$frozenLuaBytes = [System.IO.File]::ReadAllBytes($luaSource)
$frozenComparatorBytes = [System.IO.File]::ReadAllBytes($comparatorSource)
$frozenWrapperBytes = [System.IO.File]::ReadAllBytes($PSCommandPath)
$frozenLuaSha256 = Get-BytesSha256 -Bytes $frozenLuaBytes
$frozenComparatorSha256 = Get-BytesSha256 -Bytes $frozenComparatorBytes
$frozenWrapperSha256 = Get-BytesSha256 -Bytes $frozenWrapperBytes
$pythonIdentity = Get-FileIdentity -Path $resolvedPython
$ninjaIdentity = Get-FileIdentity -Path $resolvedNinja
$runtimeDllInventory = @(Get-RuntimeDllInventory)
$runtimeDllInventorySha256 = Get-TextSha256 -Text (
    $runtimeDllInventory | ConvertTo-Json -Depth 5 -Compress
)
$pythonVersionRecord = Invoke-CapturedCommand -FileName $resolvedPython `
    -Arguments @("--version") -WorkingDirectory $sourceRoot -Label "Python version"
$pythonVersion = ($pythonVersionRecord.Stdout + $pythonVersionRecord.Stderr).Trim()
if ($pythonVersionRecord.ExitCode -ne 0 -or
    $pythonVersion -notmatch '^Python \d+\.\d+\.\d+') {
    throw "Cannot verify Legacy ledger Python version: $pythonVersion"
}
$gitState = Get-GitState -Repository $sourceRoot
$leftBuild = Get-BuildProvenance `
    -Directory $resolvedLeftBuild -Executable $resolvedLeftExecutable
$rightBuild = Get-BuildProvenance `
    -Directory $resolvedRightBuild -Executable $resolvedRightExecutable
if ($ExpectedLeftFpMode -and $leftBuild.FpMode -ne $ExpectedLeftFpMode) {
    throw "Left fp_mode mismatch: expected=$ExpectedLeftFpMode; actual=$($leftBuild.FpMode)"
}
if ($ExpectedRightFpMode -and $rightBuild.FpMode -ne $ExpectedRightFpMode) {
    throw "Right fp_mode mismatch: expected=$ExpectedRightFpMode; actual=$($rightBuild.FpMode)"
}
Assert-BuildPairCompatible -Left $leftBuild -Right $rightBuild
$leftBuildExecution = Invoke-BuildTarget -Build $leftBuild
$rightBuildExecution = Invoke-BuildTarget -Build $rightBuild
$leftBuild = Get-BuildProvenance `
    -Directory $resolvedLeftBuild -Executable $resolvedLeftExecutable
$rightBuild = Get-BuildProvenance `
    -Directory $resolvedRightBuild -Executable $resolvedRightExecutable
if (($ExpectedLeftFpMode -and $leftBuild.FpMode -ne $ExpectedLeftFpMode) -or
    ($ExpectedRightFpMode -and $rightBuild.FpMode -ne $ExpectedRightFpMode)) {
    throw "Legacy ledger FP mode changed during target build"
}
Assert-BuildPairCompatible -Left $leftBuild -Right $rightBuild
Assert-FpCompileContract -Build $leftBuild -Mode "legacy_fast"
Assert-FpCompileContract -Build $rightBuild -Mode "strict"
Assert-AllCompileCommandsCompatible -Left $leftBuild -Right $rightBuild
$leftBuildProvenanceSha256 = Get-BuildProvenanceSha256 -Build $leftBuild
$rightBuildProvenanceSha256 = Get-BuildProvenanceSha256 -Build $rightBuild
$leftExeInfo = Get-Item -LiteralPath $resolvedLeftExecutable
$rightExeInfo = Get-Item -LiteralPath $resolvedRightExecutable
$leftExeHash = (Get-FileHash -LiteralPath $resolvedLeftExecutable -Algorithm SHA256).Hash
$rightExeHash = (Get-FileHash -LiteralPath $resolvedRightExecutable -Algorithm SHA256).Hash
if ($leftExeHash -eq $rightExeHash) {
    throw "Legacy ledger executables have identical SHA-256"
}

$tempParent = [System.IO.Path]::GetFullPath($TemporaryDirectory)
if (-not (Test-Path -LiteralPath $tempParent -PathType Container)) {
    throw "Temporary directory does not exist: $tempParent"
}
$runId = [DateTime]::UtcNow.ToString("yyyyMMddTHHmmssZ") + "-" +
    [guid]::NewGuid().ToString("N").Substring(0, 8)
$testRoot = [System.IO.Path]::GetFullPath(
    (Join-Path $tempParent ("tpt-omnipack-legacy-ledger-" + $runId))
)
$tempPrefix = $tempParent.TrimEnd(
    [System.IO.Path]::DirectorySeparatorChar,
    [System.IO.Path]::AltDirectorySeparatorChar
) + [System.IO.Path]::DirectorySeparatorChar
if (-not $testRoot.StartsWith($tempPrefix, [System.StringComparison]::OrdinalIgnoreCase)) {
    throw "Refusing to create Legacy ledger data outside the selected temporary root"
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
    $frozenLuaPath = Join-Path $testRoot "ledger-tool.lua"
    $frozenComparatorPath = Join-Path $testRoot "ledger-comparator.py"
    $frozenWrapperPath = Join-Path $testRoot "ledger-wrapper.ps1"
    [System.IO.File]::WriteAllBytes($frozenLuaPath, $frozenLuaBytes)
    [System.IO.File]::WriteAllBytes($frozenComparatorPath, $frozenComparatorBytes)
    [System.IO.File]::WriteAllBytes($frozenWrapperPath, $frozenWrapperBytes)
    $leftRecord = Invoke-Probe -Executable $resolvedLeftExecutable `
        -Root (Join-Path $testRoot $LeftLabel) -Label $LeftLabel
    $rightRecord = Invoke-Probe -Executable $resolvedRightExecutable `
        -Root (Join-Path $testRoot $RightLabel) -Label $RightLabel

    $comparisonPath = Join-Path $testRoot "ledger-comparison.json"
    $comparatorRecord = Invoke-Comparator `
        -LeftRecord $leftRecord -RightRecord $rightRecord `
        -OutputPath $comparisonPath -WorkingDirectory $testRoot
    $comparison = Get-Content -LiteralPath $comparisonPath -Raw | ConvertFrom-Json
    if ($comparison.status -ne "PASS" -or
        $comparison.comparison_kind -ne "same_source_cpu_fp_mode_legacy_proxy_ledger") {
        throw "Legacy ledger comparison contract failed"
    }
    if (-not $comparison.left.finite_exported_state -or
        -not $comparison.right.finite_exported_state -or
        -not $comparison.left.range_contract_pass -or
        -not $comparison.right.range_contract_pass) {
        throw "Legacy ledger found exported non-finite or out-of-range state"
    }
    foreach ($claim in @(
        "physical_mass_conservation_evaluated",
        "physical_energy_conservation_evaluated",
        "physical_momentum_conservation_evaluated",
        "source_sink_attribution_evaluated",
        "correction_events_evaluated"
    )) {
        if ($comparison.claims.$claim) {
            throw "Legacy ledger made a forbidden physical claim: $claim"
        }
    }
    if (-not $comparison.claims.legacy_field_proxies_only -or
        -not $comparison.claims.exported_particle_float_subset_only -or
        ($SampleInterval -gt 1 -and -not $comparison.claims.sampled_states_only) -or
        ($SampleInterval -eq 1 -and -not $comparison.claims.all_tick_post_update_exported_fields)) {
        throw "Legacy ledger comparison omitted its sampling/proxy scope"
    }

    $finalGitState = Get-GitState -Repository $sourceRoot
    if ($finalGitState.Commit -ne $gitState.Commit -or
        $finalGitState.StateSha256 -ne $gitState.StateSha256) {
        throw "Legacy ledger source worktree changed during execution"
    }
    foreach ($tool in @(
        [pscustomobject]@{ Path = $luaSource; Hash = $frozenLuaSha256 },
        [pscustomobject]@{ Path = $comparatorSource; Hash = $frozenComparatorSha256 },
        [pscustomobject]@{ Path = $PSCommandPath; Hash = $frozenWrapperSha256 }
    )) {
        if ((Get-FileHash -LiteralPath $tool.Path -Algorithm SHA256).Hash -ne $tool.Hash) {
            throw "Legacy ledger tool source changed during execution: $($tool.Path)"
        }
    }
    $leftExeAfter = Get-FileIdentity -Path $resolvedLeftExecutable
    $rightExeAfter = Get-FileIdentity -Path $resolvedRightExecutable
    $pythonAfter = Get-FileIdentity -Path $resolvedPython
    $ninjaAfter = Get-FileIdentity -Path $resolvedNinja
    if ($leftExeAfter.sha256 -ne $leftExeHash -or
        $leftExeAfter.length_bytes -ne $leftExeInfo.Length -or
        $rightExeAfter.sha256 -ne $rightExeHash -or
        $rightExeAfter.length_bytes -ne $rightExeInfo.Length) {
        throw "Legacy ledger executable changed during execution; retained_test_root=$testRoot"
    }
    if ($pythonAfter.sha256 -ne $pythonIdentity.sha256 -or
        $ninjaAfter.sha256 -ne $ninjaIdentity.sha256) {
        throw "Legacy ledger execution tool changed during execution; retained_test_root=$testRoot"
    }
    $runtimeDllInventoryAfter = @(Get-RuntimeDllInventory)
    $runtimeDllInventoryAfterSha256 = Get-TextSha256 -Text (
        $runtimeDllInventoryAfter | ConvertTo-Json -Depth 5 -Compress
    )
    if ($runtimeDllInventoryAfterSha256 -ne $runtimeDllInventorySha256) {
        throw "Legacy ledger runtime DLL inventory changed; retained_test_root=$testRoot"
    }
    $leftBuildAfter = Get-BuildProvenance `
        -Directory $resolvedLeftBuild -Executable $resolvedLeftExecutable
    $rightBuildAfter = Get-BuildProvenance `
        -Directory $resolvedRightBuild -Executable $resolvedRightExecutable
    if ((Get-BuildProvenanceSha256 -Build $leftBuildAfter) -ne
            $leftBuildProvenanceSha256 -or
        (Get-BuildProvenanceSha256 -Build $rightBuildAfter) -ne
            $rightBuildProvenanceSha256) {
        throw "Legacy ledger build provenance changed; retained_test_root=$testRoot"
    }

    New-Item -ItemType Directory -Path $artifactRoot | Out-Null
    Copy-ProbeRecord -Record $leftRecord -Destination $artifactRoot
    Copy-ProbeRecord -Record $rightRecord -Destination $artifactRoot
    Copy-Item -LiteralPath $comparisonPath -Destination (
        Join-Path $artifactRoot "ledger-comparison.json"
    )
    Copy-Item -LiteralPath $comparatorRecord.StdoutPath -Destination (
        Join-Path $artifactRoot "comparator-stdout.log"
    )
    Copy-Item -LiteralPath $comparatorRecord.StderrPath -Destination (
        Join-Path $artifactRoot "comparator-stderr.log"
    )
    Copy-Item -LiteralPath $frozenLuaPath -Destination (
        Join-Path $artifactRoot "ledger-tool.lua"
    )
    Copy-Item -LiteralPath $frozenComparatorPath -Destination (
        Join-Path $artifactRoot "ledger-comparator.py"
    )
    Copy-Item -LiteralPath $frozenWrapperPath -Destination (
        Join-Path $artifactRoot "ledger-wrapper.ps1"
    )

    $artifactFiles = @(
        Get-ChildItem -LiteralPath $artifactRoot -File | Sort-Object Name | ForEach-Object {
            [ordered]@{
                name = $_.Name
                length_bytes = [int64]$_.Length
                sha256 = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash
            }
        }
    )
    $records = @($leftRecord, $rightRecord)
    $metrics = @($records | ForEach-Object {
        [ordered]@{
            label = $_.Label
            wall_seconds = $_.WallSeconds
            cpu_seconds = $_.CpuSeconds
            max_sampled_working_set_bytes = $_.MaxSampledWorkingSetBytes
            max_sampled_private_bytes = $_.MaxSampledPrivateBytes
            memory_sample_interval_ms = 50
        }
    })
    $manifest = [ordered]@{
        schema_version = 1
        status = "PASS"
        run_id = $runId
        recorded_at_utc = [DateTime]::UtcNow.ToString("o")
        scenario = $Scenario
        comparison_kind = "same_source_cpu_fp_mode_legacy_proxy_ledger"
        performance_gate = "not_evaluated"
        seed = [uint64[]]@($SeedA, $SeedB, $SeedC, $SeedD)
        total_steps = $TotalSteps
        sample_interval = $SampleInterval
        samples = [int]$comparison.samples
        sampled_finite_exported_state = [ordered]@{
            left = [bool]$comparison.left.finite_exported_state
            right = [bool]$comparison.right.finite_exported_state
        }
        sampled_range_contract_pass = [ordered]@{
            left = [bool]$comparison.left.range_contract_pass
            right = [bool]$comparison.right.range_contract_pass
        }
        comparison = $comparison
        physical_mass_conservation_evaluated = $false
        physical_energy_conservation_evaluated = $false
        physical_momentum_conservation_evaluated = $false
        source_sink_attribution_evaluated = $false
        correction_events_evaluated = $false
        source = [ordered]@{
            commit = $gitState.Commit
            dirty = [bool]$gitState.Dirty
            worktree_state_sha256 = $gitState.StateSha256
            status_lines = [string[]]$gitState.StatusLines
            tool_inputs_frozen_before_execution = $true
            lua_sha256 = $frozenLuaSha256
            comparator_sha256 = $frozenComparatorSha256
            wrapper_sha256 = $frozenWrapperSha256
        }
        left = [ordered]@{
            label = $LeftLabel
            executable_sha256 = $leftExeHash
            executable_length_bytes = [int64]$leftExeInfo.Length
            build_directory = $resolvedLeftBuild
            fp_mode = $leftBuild.FpMode
            build_provenance_sha256 = $leftBuildProvenanceSha256
            build_execution = $leftBuildExecution
            compiler = $leftBuild.Compiler
            meson_options = $leftBuild.Options
            simulation_compile_command = $leftBuild.SimulationCompileCommand
            compile_command_count = $leftBuild.CompileCommandCount
            meson_target_executable = $leftBuild.TargetExecutable
            backend = "CPU"
        }
        right = [ordered]@{
            label = $RightLabel
            executable_sha256 = $rightExeHash
            executable_length_bytes = [int64]$rightExeInfo.Length
            build_directory = $resolvedRightBuild
            fp_mode = $rightBuild.FpMode
            build_provenance_sha256 = $rightBuildProvenanceSha256
            build_execution = $rightBuildExecution
            compiler = $rightBuild.Compiler
            meson_options = $rightBuild.Options
            simulation_compile_command = $rightBuild.SimulationCompileCommand
            compile_command_count = $rightBuild.CompileCommandCount
            meson_target_executable = $rightBuild.TargetExecutable
            backend = "CPU"
        }
        build_pair_validation = [ordered]@{
            same_source_root = $true
            same_compiler = $true
            options_equal_except_fp_mode = $true
            executable_matches_meson_target = $true
            simulation_compile_fp_contract = $true
            all_compile_commands_equal_except_fp_mode = $true
            all_compile_units_fp_contract = $true
            ninja_target_built_before_execution = $true
            source_commit_embedded_in_executable = "not_verified"
        }
        sampling_scope = [ordered]@{
            sampled_states_only = [bool]$comparison.claims.sampled_states_only
            all_tick_post_update_exported_fields = [bool]$comparison.claims.all_tick_post_update_exported_fields
            unsampled_ticks_finite_state = if ($SampleInterval -eq 1) {
                "all_observed_post_update_exported_fields"
            }
            else {
                "not_tested"
            }
            exported_particle_float_subset_only = $true
        }
        ledger_semantics = [ordered]@{
            particle_count_unit = "records"
            particle_property_classes_are_mass = $false
            element_weight_used_as_mass = $false
            particle_temperature_sum_is_energy = $false
            velocity_sums_are_momentum = $false
            air_pressure_sum_is_gas_mass = $false
            bound_occupancy_is_clamp_event_count = $false
        }
        exported_domains = @(
            "active Particle type/x/y/vx/vy/temp",
            "Particle property-class record counts and type histogram",
            "pv/vx/vy/hv for every Air cell",
            "compensated state-proxy sums and extrema"
        )
        unobserved_domains = @(
            "physical condensed or gas mass",
            "physical energy or momentum",
            "create/kill/type-change source attribution",
            "internal clamp or numerical correction events",
            "Particle integer payload fields",
            "walls/fans/gravity/portal/wireless/stickmen auxiliary state"
        )
        process_metrics = $metrics
        execution_toolchain = [ordered]@{
            python_executable = $pythonIdentity
            python_reported_version = $pythonVersion
            python_stdlib_and_extension_modules = "not_fully_captured"
            ninja_executable = $ninjaIdentity
            executable_rehashed_after_execution = $true
            build_provenance_rehashed_after_execution = $true
            runtime_dll_inventory_rehashed_after_execution = $true
        }
        runtime_directory = $resolvedRuntime
        runtime_directory_dll_inventory_sha256 = $runtimeDllInventorySha256
        runtime_directory_dll_inventory = $runtimeDllInventory
        direct_runtime_dependency_attribution = "not_evaluated"
        child_environment = [ordered]@{
            policy = "minimal_allowlist"
            inherited_names = @("SystemRoot", "WINDIR", "TEMP", "TMP")
            explicit_names = @("PATH")
            comparator_additional_names = @("PYTHONDONTWRITEBYTECODE", "PYTHONUTF8")
        }
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
        ($manifest | ConvertTo-Json -Depth 30) + [Environment]::NewLine,
        [System.Text.UTF8Encoding]::new($false)
    )

    $completed = $true
    Write-Output "runtime-legacy-numerical-ledger: PASS"
    Write-Output "scenario=$Scenario"
    Write-Output "total_steps=$TotalSteps"
    Write-Output "sample_interval=$SampleInterval"
    Write-Output "samples=$($comparison.samples)"
    Write-Output "first_sampled_state_hash_divergence_step=$($comparison.first_sampled_state_hash_divergence.step)"
    Write-Output "first_sampled_metadata_divergence_step=$($comparison.first_sampled_metadata_divergence.step)"
    Write-Output "first_sampled_metric_divergence_step=$($comparison.first_sampled_metric_divergence.step)"
    Write-Output "left_finite_exported_state=$($comparison.left.finite_exported_state.ToString().ToLowerInvariant())"
    Write-Output "right_finite_exported_state=$($comparison.right.finite_exported_state.ToString().ToLowerInvariant())"
    Write-Output "all_tick_post_update_exported_fields=$($comparison.claims.all_tick_post_update_exported_fields.ToString().ToLowerInvariant())"
    Write-Output "sampled_states_only=$($comparison.claims.sampled_states_only.ToString().ToLowerInvariant())"
    Write-Output "physical_mass_conservation_evaluated=false"
    Write-Output "physical_energy_conservation_evaluated=false"
    Write-Output "source_commit=$($gitState.Commit)"
    Write-Output "source_dirty=$($gitState.Dirty.ToString().ToLowerInvariant())"
    Write-Output "manifest_sha256=$((Get-FileHash -LiteralPath $manifestPath -Algorithm SHA256).Hash)"
    Write-Output "manifest_json=$manifestPath"
}
catch {
    Write-Error (
        "Legacy ledger run failed; retained_test_root=$testRoot; " +
        $_.Exception.Message
    ) -ErrorAction Continue
    throw
}
finally {
    if ($completed -and -not $KeepTemporary -and (Test-Path -LiteralPath $testRoot)) {
        Remove-IsolatedRoot -Path $testRoot -Parent $tempParent
    }
}
