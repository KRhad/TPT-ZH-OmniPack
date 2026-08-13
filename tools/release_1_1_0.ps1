param(
    [ValidateSet("rc", "stable")]
    [string] $Channel = "rc",

    [string] $BuildDirectory = "build-release-1.1.0",

    [string] $OutputDirectory = "dist\\1.1.0",

    [switch] $SkipLongSoak,
    [switch] $AllowDirtyValidation
)

$ErrorActionPreference = "Stop"
$ProgressPreference = "SilentlyContinue"

# Meson stores child environments in test logs.  Release validation must not
# copy credential-like variables into ignored build artifacts or a package.
Get-ChildItem Env: | Where-Object {
    $_.Name -match '(?i)(TOKEN|SECRET|PASSWORD|PASSWD|API_KEY|PAT$|PAT_)'
} | ForEach-Object {
    Remove-Item -LiteralPath ("Env:" + $_.Name) -ErrorAction SilentlyContinue
}

$sourceRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot "..")).Path
$git = (Get-Command git.exe -CommandType Application -ErrorAction Stop | Select-Object -First 1).Source
$msysBin = "C:\msys64\ucrt64\bin"
$msysUsrBin = "C:\msys64\usr\bin"
# Keep the Git installation that resolved before adding MSYS2 to PATH at the
# front.  Git for Windows applies the checkout's autocrlf configuration while
# MSYS2 Git may omit its system config, which turns a clean CRLF checkout into
# thousands of false working-tree changes.  Meson/Ninja/Python still resolve
# from MSYS2, while every Git subprocess (including package_test_release.py)
# uses this same native executable and config.
$gitBin = (Get-Item -LiteralPath $git).Directory.FullName
$env:PATH = "$gitBin;$msysBin;$msysUsrBin;" + $env:PATH
$meson = Join-Path $msysBin "meson.exe"
$ninja = Join-Path $msysBin "ninja.exe"
$python = Join-Path $msysBin "python3.exe"
$objcopy = Join-Path $msysBin "objcopy.exe"
$strip = Join-Path $msysBin "strip.exe"
$objdump = Join-Path $msysBin "objdump.exe"
$strings = Join-Path $msysBin "strings.exe"

foreach ($tool in @($meson, $ninja, $python, $objcopy, $strip, $objdump, $strings)) {
    if (-not (Test-Path -LiteralPath $tool -PathType Leaf)) { throw "Required release tool is missing: $tool" }
}

if ($Channel -eq "stable" -and $SkipLongSoak) {
    throw "Stable release rejects -SkipLongSoak"
}
if ($Channel -eq "stable" -and $AllowDirtyValidation) {
    throw "Stable release rejects -AllowDirtyValidation"
}

$version = if ($Channel -eq "stable") { "1.1.0" } else { "1.1.0-rc1" }
$kind = if ($Channel -eq "stable") { "release" } else { "release-candidate" }
$statusLines = @(
    & $git -C $sourceRoot status --porcelain=v1 --untracked-files=all 2>$null |
        Where-Object { -not [string]::IsNullOrWhiteSpace($_) }
)
$sourceDirty = @($statusLines).Count -gt 0
Write-Verbose "release source=$sourceRoot status_count=$(@($statusLines).Count)"
if ($sourceDirty -and -not ($Channel -eq "rc" -and $AllowDirtyValidation)) {
    throw "RELEASE BLOCKED: source worktree is dirty; commit reviewed changes before a reproducible package"
}

$buildDirectory = if ([IO.Path]::IsPathRooted($BuildDirectory)) { $BuildDirectory } else { Join-Path $sourceRoot $BuildDirectory }
$outputDirectory = if ([IO.Path]::IsPathRooted($OutputDirectory)) { $OutputDirectory } else { Join-Path $sourceRoot $OutputDirectory }
$validationDirectory = Join-Path $outputDirectory "validation-$version"
$releaseDirectory = Join-Path $outputDirectory "release"
$symbolsDirectory = Join-Path $outputDirectory "symbols"
New-Item -ItemType Directory -Force $validationDirectory, $releaseDirectory, $symbolsDirectory | Out-Null

$gate = [ordered]@{
    Build = "NOT TESTED"
    UnitTests = "NOT TESTED"
    AtmosphereBench = "NOT TESTED"
    MassConservation = "NOT TESTED"
    OmniSaveReload = "NOT TESTED"
    OfficialTPTSaveCompatibility = "NOT TESTED"
    SDLGPU = "NOT TESTED"
    CPUFallback = "NOT TESTED"
    GPUNumericalValidation = "NOT TESTED"
    WindowsCleanMachine = "NOT TESTED"
    SDL3GUI = "NOT TESTED"
    Soak2Hours = "NOT TESTED"
    ExecutableStripped = "NOT TESTED"
    DebugSymbolsSeparated = "NOT TESTED"
    PackageManifest = "NOT TESTED"
    SHA256 = "NOT TESTED"
}

function Invoke-GateCommand {
    param([string] $Name, [scriptblock] $Command)
    try {
        & $Command
        if ($LASTEXITCODE -ne 0) { throw "$Name exit=$LASTEXITCODE" }
        $gate[$Name] = "PASS"
    } catch {
        $gate[$Name] = "FAIL"
        throw
    }
}

try {
    if (-not (Test-Path -LiteralPath (Join-Path $buildDirectory "build.ninja"))) {
        & $meson setup $buildDirectory $sourceRoot "-Dbuildtype=release" "-Ddebug=true" "-Dstrip=false" "-Dstatic=prebuilt" "-Dsdl_backend=sdl3" "-Dbuild_tests=true" "-Drelease_label=$version"
    } else {
        & $meson setup --reconfigure $buildDirectory $sourceRoot "-Dbuildtype=release" "-Ddebug=true" "-Dstrip=false" "-Dstatic=prebuilt" "-Dsdl_backend=sdl3" "-Dbuild_tests=true" "-Drelease_label=$version"
    }
    Invoke-GateCommand Build { & $ninja -C $buildDirectory }
    Invoke-GateCommand UnitTests { & $meson test -C $buildDirectory --suite static --print-errorlogs -t 4 }
    Invoke-GateCommand AtmosphereBench { & $meson test -C $buildDirectory --suite static --print-errorlogs --no-rebuild "atmospherebench-contract" }
    Invoke-GateCommand MassConservation { & $meson test -C $buildDirectory --suite static --print-errorlogs --no-rebuild "omni-atmosphere-cpu-mvp" "omni-atmosphere-multispecies" }
    Invoke-GateCommand OmniSaveReload { & $meson test -C $buildDirectory --suite static --print-errorlogs --no-rebuild "omni-save-roundtrip-probe" }

    $exe = Join-Path $buildDirectory "tpt-zh-omnipack.exe"
    Invoke-GateCommand SDLGPU { & $exe "--gpu-probe" | Tee-Object -FilePath (Join-Path $validationDirectory "gpu-probe.txt") }
    Invoke-GateCommand GPUNumericalValidation { & $exe "--gpu-validate" | Tee-Object -FilePath (Join-Path $validationDirectory "gpu-validation.txt") }
    # CPU fallback is an implementation invariant tested on every non-GPU build;
    # this host still records GPU validation separately rather than faking a loss.
    Invoke-GateCommand CPUFallback { & $python (Join-Path $sourceRoot "tools\\sdlgpu_probe_contract_check.py") }

    if ($SkipLongSoak) { $gate.Soak2Hours = "NOT TESTED" }

    $rawExe = Join-Path $buildDirectory "tpt-zh-omnipack.exe"
    $releaseExe = Join-Path $releaseDirectory "tpt-zh-omnipack.exe"
    $symbolFile = Join-Path $symbolsDirectory "tpt-zh-omnipack.debug"
    Invoke-GateCommand DebugSymbolsSeparated {
        & $python (Join-Path $sourceRoot "tools\\prepare_windows_release.py") --raw-executable $rawExe --executable $releaseExe --symbols $symbolFile --objcopy $objcopy --strip $strip
    }
    Invoke-GateCommand ExecutableStripped {
        & $python (Join-Path $sourceRoot "tools\\release_binary_audit.py") --executable $releaseExe --symbols $symbolFile --objdump $objdump --strings $strings
    }

    if ($Channel -eq "stable") {
        foreach ($name in @("OfficialTPTSaveCompatibility", "WindowsCleanMachine", "SDL3GUI", "Soak2Hours")) {
            if ($gate[$name] -ne "PASS") { throw "RELEASE BLOCKED: stable mandatory gate is $name=$($gate[$name])" }
        }
    }

    $buildInfoPath = Join-Path $validationDirectory "BUILD-INFO.txt"
    $validationPath = Join-Path $validationDirectory "RELEASE-VALIDATION.txt"
    [IO.File]::WriteAllLines($buildInfoPath, @(
        "TPT-ZH OmniPack $version",
        "Git commit: $((& $git -C $sourceRoot rev-parse HEAD).Trim())",
        "Build type: Release with detached DWARF symbols",
        "SDL: 3.4.14",
        "Platform: Windows x64",
        "OmniCore: enabled",
        "Atmosphere: enabled",
        "Chemistry: enabled",
        "Compute backend: SDL_GPU Vulkan optional; CPU fallback; D3D12 no DXIL shader; CUDA not implemented"
    ), [Text.UTF8Encoding]::new($false))
    $writeStagedValidation = {
        [IO.File]::WriteAllLines($validationPath, @(
            "TPT-ZH OmniPack $version Release Validation",
            "",
            "Commit: $((& $git -C $sourceRoot rev-parse HEAD).Trim())",
            "Channel: $Channel",
            "",
            "Build: $($gate.Build)",
            "Unit tests: $($gate.UnitTests)",
            "AtmosphereBench: $($gate.AtmosphereBench)",
            "Mass conservation: $($gate.MassConservation)",
            "Official TPT save compatibility: $($gate.OfficialTPTSaveCompatibility)",
            "OmniPack save reload: $($gate.OmniSaveReload)",
            "SDL_GPU: $($gate.SDLGPU)",
            "CPU fallback: $($gate.CPUFallback)",
            "GPU numerical validation: $($gate.GPUNumericalValidation)",
            "Windows clean machine: $($gate.WindowsCleanMachine)",
            "SDL3 GUI: $($gate.SDL3GUI)",
            "2-hour soak test: $($gate.Soak2Hours)",
            "Release executable stripped: $($gate.ExecutableStripped)",
            "Debug symbols separated: $($gate.DebugSymbolsSeparated)",
            "Package manifest: $($gate.PackageManifest)",
            "SHA256: $($gate.SHA256)",
            "",
            "FINAL STATUS: RC PACKAGE GENERATED - STABLE GATES REMAIN REQUIRED"
        ), [Text.UTF8Encoding]::new($false))
    }
    & $writeStagedValidation
    $packageArgs = @(
        (Join-Path $sourceRoot "tools\\package_test_release.py"),
		"--source-root", $sourceRoot,
		"--executable", $releaseExe,
		"--symbols", $symbolFile,
		"--output-directory", $outputDirectory,
		"--version", $version,
		"--kind", $kind,
		"--objdump", $objdump,
        "--strings", $strings,
        "--extra-member", $buildInfoPath, "BUILD-INFO.txt",
        "--extra-member", $validationPath, "RELEASE-VALIDATION.txt"
	)
	if ($Channel -eq "rc" -and $AllowDirtyValidation) { $packageArgs += "--allow-dirty-validation" }
	& $python @packageArgs
    if ($LASTEXITCODE -ne 0) { throw "package creation failed" }
    $gate.PackageManifest = "PASS"
    $gate.SHA256 = "PASS"
    & $writeStagedValidation
    & $python @packageArgs
    if ($LASTEXITCODE -ne 0) { throw "final package creation failed" }
} catch {
    $failure = $_.Exception.Message
} finally {
    $commit = (& git -C $sourceRoot rev-parse HEAD).Trim()
    $lines = @(
        "TPT-ZH OmniPack $version Release Validation",
        "",
        "Commit: $commit",
        "Channel: $Channel",
        "",
        "Build: $($gate.Build)",
        "Unit tests: $($gate.UnitTests)",
        "AtmosphereBench: $($gate.AtmosphereBench)",
        "Mass conservation: $($gate.MassConservation)",
        "Official TPT save compatibility: $($gate.OfficialTPTSaveCompatibility)",
        "OmniPack save reload: $($gate.OmniSaveReload)",
        "SDL_GPU: $($gate.SDLGPU)",
        "CPU fallback: $($gate.CPUFallback)",
        "GPU numerical validation: $($gate.GPUNumericalValidation)",
        "Windows clean machine: $($gate.WindowsCleanMachine)",
        "SDL3 GUI: $($gate.SDL3GUI)",
        "2-hour soak test: $($gate.Soak2Hours)",
        "Release executable stripped: $($gate.ExecutableStripped)",
        "Debug symbols separated: $($gate.DebugSymbolsSeparated)",
        "Package manifest: $($gate.PackageManifest)",
        "SHA256: $($gate.SHA256)",
        ""
    )
    if ($failure) {
        $lines += "FINAL STATUS: RELEASE BLOCKED", "Blocking item: $failure"
    } elseif ($Channel -eq "stable") {
        $lines += "FINAL STATUS: READY FOR STABLE RELEASE"
    } else {
        $lines += "FINAL STATUS: RC PACKAGE GENERATED - STABLE GATES REMAIN REQUIRED"
    }
    $report = Join-Path $outputDirectory "RELEASE-VALIDATION.txt"
    [IO.File]::WriteAllLines($report, $lines, [Text.UTF8Encoding]::new($false))
}

if ($failure) { Write-Error $failure; exit 1 }
Write-Output "release-1.1.0: PASS $report"
