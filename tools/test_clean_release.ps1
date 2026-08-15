param(
    [Parameter(Mandatory = $true)][string] $PackageZip,
    [Parameter(Mandatory = $true)][string] $OutputJson,
    [Parameter(Mandatory = $true)][string] $RunId,
    [ValidateSet("WindowsPortableExtraction", "WindowsCleanMachine", "SDL3GUI")]
    [string] $GateName = "WindowsPortableExtraction",
    [string] $ExpectedArtifactSha256,
    [string] $ExpectedValidatorSha256,
    [string] $Objdump = "C:\msys64\ucrt64\bin\objdump.exe"
)

$ErrorActionPreference = "Stop"
$startedAt = [DateTime]::UtcNow
$testName = if ($GateName -eq "WindowsCleanMachine") { "windows_clean_machine" } elseif ($GateName -eq "SDL3GUI") { "sdl3_gui" } else { "windows_portable_extraction" }
$result = [ordered]@{
    schema = "omnipack-release-evidence"
    schema_version = 1
    test = $testName
    run_id = $RunId
    status = "FAIL"
    passed = $false
    reason = "not started"
    gate_started_at = $startedAt.ToString("o")
    gate_finished_at = $null
    candidate_filename = $null
    candidate_sha256 = $null
    source_checkout_used = $null
    source_tree_indicators = @()
    msys2_present = Test-Path -LiteralPath "C:\msys64"
    development_tools_detected = [ordered]@{}
    source_markers_checked = $false
    development_tool_probe_completed = $false
    runtime_validator = "test_clean_release.ps1"
    runtime_validator_sha256 = $null
    validator_binding_checked = $false
    runtime_job_kind = if ($GateName -eq "WindowsCleanMachine") { "runtime-only" } else { "developer-portable" }
    project_build_tool_dependency_used = $false
    candidate_extracted_to_fresh_directory = $false
    candidate_source_markers_checked = $false
    candidate_source_tree_indicators = @()
    runtime_target_executable_count = 0
    sanitized_path_used = $false
    runtime_payload_schema_version = 0
    launch_passed = $false
    fixture_created = $false
    initial_simulate_passed = $false
    initial_save_passed = $false
    initial_parse_passed = $false
    initial_reload_passed = $false
    initial_state_validate_passed = $false
    post_step_simulate_passed = $false
    post_step_finite_passed = $false
    post_step_inventory_passed = $false
    post_step_atmosphere_passed = $false
    post_step_save_passed = $false
    post_step_parse_passed = $false
    post_step_reload_passed = $false
    post_step_roundtrip_passed = $false
    save_reload_passed = $false
    state_validate_passed = $false
    enhanced_mode = $false
    omni_state_present = $false
    water_sidecar_roundtrip = $false
    runtime_completion_reached = $false
    initial_particle_count = -1
    post_step_particle_count = -1
    final_particle_count = -1
    atmosphere_non_finite_cells = -1
    runtime_stdout = $null
    runtime_stdout_sha256 = $null
    runtime_stderr = $null
    runtime_stderr_sha256 = $null
    runtime_inner_evidence = $null
    runtime_inner_evidence_sha256 = $null
    clean_shutdown = $false
    artifact = $null
    window_created = $false
    frame_rendered = $false
    resize = $false
    fullscreen_toggle = $false
    keyboard = $false
    mouse = $false
    text_input = $false
    clipboard = $false
    screenshot_created = $false
    screenshot_width = 0
    screenshot_height = 0
    screenshot_bytes = 0
    screenshot_pixel_count = 0
    screenshot_nonzero_pixels = 0
    screenshot_distinct_colors = 0
    restart = $false
}

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

function Get-SourceIndicators {
    param([string[]] $StartPaths)
    $found = @{}
    foreach ($start in $StartPaths) {
        if (-not $start) { continue }
        $item = Get-Item -LiteralPath $start -ErrorAction SilentlyContinue
        if (-not $item) { continue }
        $directory = if ($item.PSIsContainer) { $item.FullName } else { $item.Directory.FullName }
        while ($directory) {
            $gitMarker = Join-Path $directory ".git"
            $mesonMarker = Join-Path $directory "meson.build"
            $sourceMarker = Join-Path $directory "src\PowderToy.cpp"
            $releaseMarker = Join-Path $directory "tools\release_1_1_0.ps1"
            if (Test-Path -LiteralPath $gitMarker) { $found["$directory\.git"] = $true }
            if ((Test-Path -LiteralPath $mesonMarker) -and (Test-Path -LiteralPath $sourceMarker)) {
                $found["$directory\meson.build+src"] = $true
            }
            if (Test-Path -LiteralPath $releaseMarker) { $found[$releaseMarker] = $true }
            $parent = [IO.Directory]::GetParent($directory)
            if (-not $parent -or $parent.FullName -eq $directory) { break }
            $directory = $parent.FullName
        }
    }
    return @($found.Keys | Sort-Object)
}

function Get-ExtractedSourceIndicators {
    param([Parameter(Mandatory = $true)][string] $Root)
    $found = @{}
    $directories = @((Get-Item -LiteralPath $Root)) + @(Get-ChildItem -LiteralPath $Root -Directory -Recurse -Force)
    foreach ($directoryItem in $directories) {
        $directory = $directoryItem.FullName
        $gitMarker = Join-Path $directory ".git"
        $mesonMarker = Join-Path $directory "meson.build"
        $sourceMarker = Join-Path $directory "src\PowderToy.cpp"
        $releaseMarker = Join-Path $directory "tools\release_1_1_0.ps1"
        if (Test-Path -LiteralPath $gitMarker) { $found[$gitMarker] = $true }
        if ((Test-Path -LiteralPath $mesonMarker) -and (Test-Path -LiteralPath $sourceMarker)) {
            $found["$mesonMarker+src"] = $true
        }
        if (Test-Path -LiteralPath $releaseMarker) { $found[$releaseMarker] = $true }
    }
    return @($found.Keys | Sort-Object)
}

function Get-BmpMetadata {
    param([string]$Path)
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) { throw "GUI screenshot is absent" }
    $bytes = [IO.File]::ReadAllBytes($Path)
    if ($bytes.Length -lt 54 -or $bytes[0] -ne 0x42 -or $bytes[1] -ne 0x4D) { throw "GUI screenshot has an invalid BMP header" }
    $pixelOffset = [BitConverter]::ToInt32($bytes, 10)
    $width = [BitConverter]::ToInt32($bytes,18)
    $rawHeight = [BitConverter]::ToInt32($bytes,22)
    $height = [Math]::Abs($rawHeight)
    $bits = [BitConverter]::ToInt16($bytes,28)
    $compression = [BitConverter]::ToInt32($bytes,30)
    $bitfields = $bits -eq 32 -and $compression -eq 3
    if ($width -le 0 -or $height -le 0 -or $rawHeight -eq [Int32]::MinValue -or
        ($bits -ne 24 -and $bits -ne 32) -or ($compression -ne 0 -and -not $bitfields) -or $pixelOffset -lt 54) {
        throw "GUI screenshot dimensions or pixel format are invalid"
    }
    [uint32]$rgbMask = 0x00FFFFFF
    if ($bitfields) {
        if ($bytes.Length -lt 66 -or $pixelOffset -lt 66) { throw "GUI screenshot bitfield masks are truncated" }
        [uint32]$redMask = [BitConverter]::ToUInt32($bytes,54)
        [uint32]$greenMask = [BitConverter]::ToUInt32($bytes,58)
        [uint32]$blueMask = [BitConverter]::ToUInt32($bytes,62)
        if ($redMask -eq 0 -or $greenMask -eq 0 -or $blueMask -eq 0 -or
            ($redMask -band $greenMask) -ne 0 -or ($redMask -band $blueMask) -ne 0 -or
            ($greenMask -band $blueMask) -ne 0) {
            throw "GUI screenshot bitfield masks are invalid"
        }
        $rgbMask = $redMask -bor $greenMask -bor $blueMask
    }
    $bytesPerPixel = [int]($bits / 8)
    $rowStride = [int](([math]::Ceiling(($width * $bits) / 32.0)) * 4)
    $required = [int64]$pixelOffset + ([int64]$rowStride * $height)
    if ($required -gt $bytes.Length) { throw "GUI screenshot pixel data is truncated" }
    $nonzeroPixels = [int64]0
    $firstColor = $null
    $hasDifferentColor = $false
    for ($y = 0; $y -lt $height; $y++) {
        $row = $pixelOffset + ($y * $rowStride)
        for ($x = 0; $x -lt $width; $x++) {
            $offset = $row + ($x * $bytesPerPixel)
            [uint32]$packed = [uint32]$bytes[$offset] -bor ([uint32]$bytes[$offset + 1] -shl 8) -bor ([uint32]$bytes[$offset + 2] -shl 16)
            if ($bytesPerPixel -eq 4) { $packed = $packed -bor ([uint32]$bytes[$offset + 3] -shl 24) }
            [uint32]$color = if ($bitfields) { $packed -band $rgbMask } else { $packed -band 0x00FFFFFF }
            if ($color -ne 0) { $nonzeroPixels++ }
            if ($null -eq $firstColor) { $firstColor = $color }
            elseif ($color -ne $firstColor) { $hasDifferentColor = $true }
        }
    }
    $pixelCount = [int64]$width * $height
    $distinctColors = if ($null -eq $firstColor) { 0 } elseif ($hasDifferentColor) { 2 } else { 1 }
    if ($nonzeroPixels -lt [Math]::Max([int64]1, [Math]::Floor($pixelCount / 100)) -or $distinctColors -lt 2) {
        throw "GUI screenshot pixel content is blank or lacks the expected rendered contrast"
    }
    return [ordered]@{
        width=$width; height=$height; bytes=$bytes.Length; pixel_count=$pixelCount;
        nonzero_pixels=$nonzeroPixels; distinct_colors=$distinctColors
    }
}

$root = Join-Path ([IO.Path]::GetTempPath()) ("omnipack-runtime-" + [guid]::NewGuid().ToString("N"))
$outputParent = Split-Path -Parent ([IO.Path]::GetFullPath($OutputJson))
if ($outputParent) { New-Item -ItemType Directory -Force -Path $outputParent | Out-Null }
try {
    if ($RunId -notmatch '^[0-9]{8}T[0-9]{6}Z-[0-9a-f]{8}$') {
        throw "run_id format is invalid"
    }
    $resolvedValidator = (Resolve-Path -LiteralPath $PSCommandPath).Path
    $validatorHash = Get-Sha256Hex $resolvedValidator
    $result.runtime_validator_sha256 = $validatorHash
    if ($GateName -eq "WindowsCleanMachine" -and
        (-not $ExpectedValidatorSha256 -or $ExpectedValidatorSha256 -notmatch '^[0-9A-Fa-f]{64}$')) {
        throw "expected runtime validator SHA256 is required for WindowsCleanMachine"
    }
    if ($ExpectedValidatorSha256) {
        if ($ExpectedValidatorSha256 -notmatch '^[0-9A-Fa-f]{64}$') {
            throw "expected runtime validator SHA256 is invalid"
        }
        if ($validatorHash -ne $ExpectedValidatorSha256.ToUpperInvariant()) {
            throw "runtime validator SHA256 does not match expected validator"
        }
        $result.validator_binding_checked = $true
    }
    $resolvedPackage = (Resolve-Path -LiteralPath $PackageZip).Path
    $hash = Get-Sha256Hex $resolvedPackage
    $result.candidate_filename = [IO.Path]::GetFileName($resolvedPackage)
    $result.candidate_sha256 = $hash
    if (-not $ExpectedArtifactSha256 -or $ExpectedArtifactSha256 -notmatch '^[0-9A-Fa-f]{64}$') {
        throw "expected candidate SHA256 is required"
    }
    if ($hash -ne $ExpectedArtifactSha256.ToUpperInvariant()) {
        throw "candidate SHA256 does not match expected artifact"
    }

    $indicators = @(Get-SourceIndicators @((Get-Location).Path, $PSScriptRoot, $resolvedPackage))
    $result.source_tree_indicators = $indicators
    $result.source_checkout_used = $indicators.Count -gt 0
    foreach ($tool in @("gcc.exe", "g++.exe", "meson.exe", "ninja.exe", "glslc.exe", "bash.exe")) {
        $command = Get-Command $tool -CommandType Application -ErrorAction SilentlyContinue | Select-Object -First 1
        $result.development_tools_detected[$tool] = if ($command) { $command.Source } else { $null }
    }
    $result.source_markers_checked = $true
    $result.development_tool_probe_completed = $true
    if ($GateName -eq "WindowsCleanMachine" -and $result.source_checkout_used) {
        throw "runtime-only validation detected a source checkout or project source markers"
    }

    New-Item -ItemType Directory -Path $root | Out-Null
    Expand-Archive -LiteralPath $resolvedPackage -DestinationPath $root
    $result.candidate_extracted_to_fresh_directory = $true
    $candidateIndicators = @(Get-ExtractedSourceIndicators -Root $root)
    $result.candidate_source_tree_indicators = $candidateIndicators
    $result.candidate_source_markers_checked = $true
    if ($candidateIndicators.Count -gt 0) { throw "runtime candidate contains source-tree markers" }
    $executables = @(Get-ChildItem -LiteralPath $root -Filter "tpt-zh-omnipack.exe" -Recurse -File)
    $result.runtime_target_executable_count = $executables.Count
    if ($executables.Count -ne 1) { throw "runtime candidate must contain exactly one release executable" }
    $exe = $executables[0].FullName

    if ($GateName -eq "WindowsPortableExtraction") {
        if (-not (Test-Path -LiteralPath $Objdump -PathType Leaf)) { throw "objdump is required for developer-host portable validation" }
        $result.project_build_tool_dependency_used = $true
        $imports = (& $Objdump -p $exe | Out-String)
        foreach ($dll in @("libgcc_s_seh-1.dll", "libstdc++-6.dll", "libwinpthread-1.dll")) {
            if ($imports -match [regex]::Escape("DLL Name: $dll")) { throw "development runtime import: $dll" }
        }
    }

    $oldPath = $env:PATH
    $oldDdir = $env:OMNI_RUNTIME_DDIR
    $env:PATH = "$env:SystemRoot\System32;$env:SystemRoot"
    $env:OMNI_RUNTIME_DDIR = $root
    $result.sanitized_path_used = $true
    try {
        if ($GateName -eq "SDL3GUI") {
            $json = Join-Path $root "sdl3-gui-inner.json"
            $stdout = Join-Path $root "gui-stdout.txt"
            $stderr = Join-Path $root "gui-stderr.txt"
            $p = Start-Process $exe -ArgumentList @("--gui-smoke-test", "--gui-smoke-json", $json) `
                -WorkingDirectory (Split-Path $exe -Parent) -Wait -PassThru -NoNewWindow `
                -RedirectStandardOutput $stdout -RedirectStandardError $stderr
            $result.clean_shutdown = $p.HasExited
            if ($p.ExitCode -ne 0 -or -not (Test-Path -LiteralPath $json -PathType Leaf)) { throw "packaged GUI validation exit=$($p.ExitCode)" }
            $smoke = Get-Content -LiteralPath $json -Raw | ConvertFrom-Json
            $required = @("window_created","frame_rendered","resize","fullscreen_toggle","keyboard","mouse","text_input","clipboard","screenshot_created","clean_shutdown","restart")
            $missing = @($required | Where-Object { $smoke.$_ -ne $true })
            if ($smoke.schema -ne "omnipack-release-evidence" -or $smoke.schema_version -ne 1 -or $smoke.test -ne "sdl3_gui" -or $smoke.status -ne "PASS" -or $smoke.passed -ne $true -or $missing.Count -gt 0) {
                throw "packaged GUI evidence is not a semantic PASS"
            }
            $innerBmp = Join-Path $root "gui-smoke.bmp"
            $bmp = Get-BmpMetadata $innerBmp
            if ($smoke.screenshot_width -ne $bmp.width -or $smoke.screenshot_height -ne $bmp.height -or
                $smoke.screenshot_bytes -ne $bmp.bytes -or $smoke.screenshot_nonzero_pixels -ne $bmp.nonzero_pixels -or
                $smoke.screenshot_distinct_colors -ne $bmp.distinct_colors) { throw "GUI screenshot metadata does not match BMP bytes" }
            $finalBmp = Join-Path $outputParent "gui-smoke.bmp"
            Copy-Item -LiteralPath $innerBmp -Destination $finalBmp -Force
            $result.artifact = "gui-smoke.bmp"
            foreach ($field in $required) { $result[$field] = $true }
            $result.screenshot_width = $bmp.width
            $result.screenshot_height = $bmp.height
            $result.screenshot_bytes = $bmp.bytes
            $result.screenshot_pixel_count = $bmp.pixel_count
            $result.screenshot_nonzero_pixels = $bmp.nonzero_pixels
            $result.screenshot_distinct_colors = $bmp.distinct_colors
            $result.launch_passed = $true
            foreach ($entry in @(
                @($stdout,"sdl3-gui.stdout.txt","runtime_stdout","runtime_stdout_sha256"),
                @($stderr,"sdl3-gui.stderr.txt","runtime_stderr","runtime_stderr_sha256"),
                @($json,"sdl3-gui.inner.json","runtime_inner_evidence","runtime_inner_evidence_sha256")
            )) {
                $target = Join-Path $outputParent $entry[1]
                Copy-Item -LiteralPath $entry[0] -Destination $target -Force
                $result[$entry[2]] = $entry[1]
                $result[$entry[3]] = Get-Sha256Hex $target
            }
        } else {
            $json = Join-Path $root "portable-runtime.json"
            $stdout = Join-Path $root "stdout.txt"
            $stderr = Join-Path $root "stderr.txt"
            $p = Start-Process $exe -ArgumentList @(
                "--portable-runtime-validate", "--portable-runtime-json", $json,
                "--portable-runtime-run-id", $RunId,
                "--portable-runtime-candidate-sha256", $hash
            ) `
                -WorkingDirectory (Split-Path $exe -Parent) -Wait -PassThru -NoNewWindow `
                -RedirectStandardOutput $stdout -RedirectStandardError $stderr
            $result.clean_shutdown = $p.HasExited -and $p.ExitCode -eq 0
            if ($p.ExitCode -ne 0 -or -not (Test-Path -LiteralPath $json -PathType Leaf)) { throw "portable runtime validation exit=$($p.ExitCode)" }
            $smoke = Get-Content -LiteralPath $json -Raw | ConvertFrom-Json
            $requiredRuntimePhases = @(
                "launch_passed", "fixture_created", "initial_simulate_passed",
                "initial_save_passed", "initial_parse_passed", "initial_reload_passed",
                "initial_state_validate_passed", "post_step_simulate_passed",
                "post_step_finite_passed", "post_step_inventory_passed",
                "post_step_atmosphere_passed", "post_step_save_passed",
                "post_step_parse_passed", "post_step_reload_passed",
                "post_step_roundtrip_passed", "save_reload_passed",
                "state_validate_passed", "enhanced_mode", "omni_state_present",
                "water_sidecar_roundtrip", "runtime_completion_reached"
            )
            $failedRuntimePhases = @($requiredRuntimePhases | Where-Object { $smoke.$_ -ne $true })
            if ($smoke.schema -ne "omnipack-release-evidence" -or $smoke.schema_version -ne 1 -or
                $smoke.payload_schema_version -ne 2 -or $smoke.run_id -ne $RunId -or
                $smoke.candidate_sha256 -ne $hash -or
                $smoke.test -ne "portable_runtime" -or $smoke.status -ne "PASS" -or $smoke.passed -ne $true -or
                $failedRuntimePhases.Count -gt 0 -or $smoke.initial_particle_count -lt 1 -or
                $smoke.post_step_particle_count -ne $smoke.initial_particle_count -or
                $smoke.final_particle_count -ne $smoke.post_step_particle_count -or
                $smoke.atmosphere_non_finite_cells -ne 0) {
                throw "portable runtime evidence is not a semantic PASS"
            }
            $result.runtime_payload_schema_version = $smoke.payload_schema_version
            foreach ($field in $requiredRuntimePhases) { $result[$field] = $smoke.$field }
            $result.initial_particle_count = $smoke.initial_particle_count
            $result.post_step_particle_count = $smoke.post_step_particle_count
            $result.final_particle_count = $smoke.final_particle_count
            $result.atmosphere_non_finite_cells = $smoke.atmosphere_non_finite_cells
            foreach ($entry in @(
                @($stdout,"$testName.stdout.txt","runtime_stdout","runtime_stdout_sha256"),
                @($stderr,"$testName.stderr.txt","runtime_stderr","runtime_stderr_sha256"),
                @($json,"$testName.inner.json","runtime_inner_evidence","runtime_inner_evidence_sha256")
            )) {
                $target = Join-Path $outputParent $entry[1]
                Copy-Item -LiteralPath $entry[0] -Destination $target -Force
                $result[$entry[2]] = $entry[1]
                $result[$entry[3]] = Get-Sha256Hex $target
            }
        }
    } finally {
        $env:PATH = $oldPath
        $env:OMNI_RUNTIME_DDIR = $oldDdir
    }
    $result.passed = $true
    $result.status = "PASS"
    $result.reason = if ($GateName -eq "SDL3GUI") { "fresh extraction and packaged GUI validation passed" } else { "fresh extraction and packaged runtime save/reload validation passed" }
} catch {
    $result.reason = $_.Exception.Message
} finally {
    $result.gate_finished_at = [DateTime]::UtcNow.ToString("o")
    [IO.File]::WriteAllText($OutputJson, ($result | ConvertTo-Json -Depth 8) + [Environment]::NewLine, [Text.UTF8Encoding]::new($false))
    Remove-Item -LiteralPath $root -Recurse -Force -ErrorAction SilentlyContinue
}
if (-not $result.passed) { exit 1 }
