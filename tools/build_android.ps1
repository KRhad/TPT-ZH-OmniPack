[CmdletBinding()]
param(
    [string]$SdkRoot = $env:ANDROID_SDK_ROOT,
    [string]$NdkVersion = '29.0.14206865',
    [string]$BuildToolsVersion = '35.0.0',
    [string]$AndroidPlatform = 'android-31',
    [string]$Jdk8Root = $env:JAVA8_HOME,
    [string]$JavaRuntimeRoot = $env:JAVA_HOME,
    [string]$BuildDirectory = 'build-android-arm64',
    [string]$OutputDirectory = 'artifacts/android/1.0.0',
    [string]$Version = '1.0.0',
    [string]$ReleaseLabel = '1.0.0',
    [string]$UpdateServer = 'https://raw.githubusercontent.com/KRhad/TPT-ZH-OmniPack/public-source',
    [string]$ProjectUrl = 'https://github.com/KRhad/TPT-ZH-OmniPack',
    [int]$AndroidVersionCode = 0,
    [int]$UpdateBuild = -1,
    [string]$Keystore = '',
    [string]$KeyAlias = 'androidkey',
    [switch]$RequireCleanSource
)

$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$sourceRevision = (& git -C $repoRoot rev-parse HEAD).Trim()
if ($LASTEXITCODE -ne 0 -or $sourceRevision -notmatch '^[0-9a-f]{40}$') {
    throw 'Unable to resolve the source Git revision'
}
$sourceStatus = @(& git -C $repoRoot status --porcelain=v1 --untracked-files=all)
if ($LASTEXITCODE -ne 0) {
    throw 'Unable to inspect the source worktree state'
}
$sourceTreeState = if ($sourceStatus.Count) { 'dirty' } else { 'clean' }
if ($RequireCleanSource -and $sourceTreeState -ne 'clean') {
    throw 'The Android release build requires a clean source worktree'
}

function Resolve-RequiredPath([string]$Path, [string]$Description) {
    if (-not $Path -or -not (Test-Path -LiteralPath $Path)) {
        throw "$Description not found: $Path"
    }
    return (Resolve-Path -LiteralPath $Path).Path
}

function MesonPath([string]$Path) {
    return $Path.Replace('\', '/').Replace("'", "\'")
}

if (-not $SdkRoot) {
    throw 'Set ANDROID_SDK_ROOT or pass -SdkRoot'
}
if ($UpdateBuild -lt -1 -or $UpdateBuild -gt 999) {
    throw 'UpdateBuild must be -1 or an integer from 0 through 999'
}

$SdkRoot = Resolve-RequiredPath $SdkRoot 'Android SDK'
$Jdk8Root = Resolve-RequiredPath $Jdk8Root 'JDK 8'
$JavaRuntimeRoot = Resolve-RequiredPath $JavaRuntimeRoot 'JDK 17 or newer'
$toolchainRoot = Split-Path $SdkRoot -Parent
$javaVersionText = (& (Join-Path $JavaRuntimeRoot 'bin\java.exe') -version 2>&1) -join ' '
if ($javaVersionText -notmatch 'version "(?<major>\d+)') {
    throw "Cannot determine Java runtime version: $javaVersionText"
}
$javaMajor = [int]$Matches.major
if ($javaMajor -eq 1 -and $javaVersionText -match 'version "1\.(?<legacy>\d+)') {
    $javaMajor = [int]$Matches.legacy
}
if ($javaMajor -lt 17) {
    throw "JDK 17 or newer is required for Android Build Tools; found $javaVersionText"
}
$ndkRoot = Resolve-RequiredPath (Join-Path $SdkRoot "ndk\$NdkVersion") 'Android NDK'
$buildToolsRoot = Resolve-RequiredPath (Join-Path $SdkRoot "build-tools\$BuildToolsVersion") 'Android Build Tools'
$platformJar = Resolve-RequiredPath (Join-Path $SdkRoot "platforms\$AndroidPlatform\android.jar") 'Android platform jar'
$javaRuntimeJar = Resolve-RequiredPath (Join-Path $Jdk8Root 'jre\lib\rt.jar') 'JDK 8 runtime jar'

$ndkBin = Join-Path $ndkRoot 'toolchains\llvm\prebuilt\windows-x86_64\bin'
$compiler = Resolve-RequiredPath (Join-Path $ndkBin 'aarch64-linux-android21-clang++.cmd') 'ARM64 Android compiler'
$strip = Resolve-RequiredPath (Join-Path $ndkBin 'llvm-strip.exe') 'LLVM strip'
$readelf = Resolve-RequiredPath (Join-Path $ndkBin 'llvm-readelf.exe') 'LLVM readelf'
$archiver = Resolve-RequiredPath (Join-Path $ndkBin 'llvm-ar.exe') 'LLVM archiver'
$javac = Resolve-RequiredPath (Join-Path $Jdk8Root 'bin\javac.exe') 'JDK 8 javac'
$jar = Resolve-RequiredPath (Join-Path $Jdk8Root 'bin\jar.exe') 'JDK 8 jar'
$d8 = Resolve-RequiredPath (Join-Path $buildToolsRoot 'd8.bat') 'd8'
$aapt = Resolve-RequiredPath (Join-Path $buildToolsRoot 'aapt.exe') 'aapt'
$aapt2 = Resolve-RequiredPath (Join-Path $buildToolsRoot 'aapt2.exe') 'aapt2'
$zipalign = Resolve-RequiredPath (Join-Path $buildToolsRoot 'zipalign.exe') 'zipalign'
$apksigner = Resolve-RequiredPath (Join-Path $buildToolsRoot 'apksigner.bat') 'apksigner'
$adb = Resolve-RequiredPath (Join-Path $SdkRoot 'platform-tools\adb.exe') 'adb'

$mesonCommand = Get-Command meson.exe -ErrorAction SilentlyContinue
$meson = if ($mesonCommand) { $mesonCommand.Source } else { 'C:\msys64\ucrt64\bin\meson.exe' }
$ninjaCommand = Get-Command ninja.exe -ErrorAction SilentlyContinue
$ninja = if ($ninjaCommand) { $ninjaCommand.Source } else { 'C:\msys64\ucrt64\bin\ninja.exe' }
$pythonCommand = Get-Command python3.exe -ErrorAction SilentlyContinue
$python = if ($pythonCommand) { $pythonCommand.Source } else { 'C:\msys64\ucrt64\bin\python3.exe' }
Resolve-RequiredPath $meson 'Meson' | Out-Null
Resolve-RequiredPath $ninja 'Ninja' | Out-Null
Resolve-RequiredPath $python 'Python 3' | Out-Null
$ucrtBin = Split-Path $meson -Parent
$msysRoot = Split-Path (Split-Path $ucrtBin -Parent) -Parent
$usrBin = Join-Path $msysRoot 'usr\bin'

$crossDirectory = Join-Path $toolchainRoot 'cross'
[IO.Directory]::CreateDirectory($crossDirectory) | Out-Null
$crossFile = Join-Path $crossDirectory 'windows-aarch64-generated.ini'
$crossContent = @"
[properties]
android_ndk_toolchain_prefix = android_ndk_toolchain_prefix
android_platform = '$AndroidPlatform'
android_platform_jar = '$(MesonPath $platformJar)'
java_runtime_jar = '$(MesonPath $javaRuntimeJar)'
android_strip = '$(MesonPath $strip)'

[binaries]
cpp = '$(MesonPath $compiler)'
ar = '$(MesonPath $archiver)'
strip = '$(MesonPath $strip)'
javac = '$(MesonPath $javac)'
jar = '$(MesonPath $jar)'
d8 = '$(MesonPath $d8)'
aapt = '$(MesonPath $aapt)'
aapt2 = '$(MesonPath $aapt2)'
zipalign = '$(MesonPath $zipalign)'
apksigner = '$(MesonPath $apksigner)'
adb = '$(MesonPath $adb)'
"@
[IO.File]::WriteAllText($crossFile, $crossContent, [Text.UTF8Encoding]::new($false))

$buildPath = if ([IO.Path]::IsPathRooted($BuildDirectory)) {
    [IO.Path]::GetFullPath($BuildDirectory)
} else {
    [IO.Path]::GetFullPath((Join-Path $repoRoot $BuildDirectory))
}
$outputPath = if ([IO.Path]::IsPathRooted($OutputDirectory)) {
    [IO.Path]::GetFullPath($OutputDirectory)
} else {
    [IO.Path]::GetFullPath((Join-Path $repoRoot $OutputDirectory))
}

$env:JAVA_HOME = $JavaRuntimeRoot
$env:PATH = "$ucrtBin;$usrBin;$JavaRuntimeRoot\bin;$env:PATH"
$setupArgs = @(
    'setup', $buildPath,
    '--cross-file', (Join-Path $repoRoot 'android\cross\aarch64.ini'),
    '--cross-file', $crossFile,
    '--buildtype', 'release',
    '-Dstatic=prebuilt',
    "-Doverride_display_version=$Version",
    "-Drelease_label=$ReleaseLabel",
    '-Dignore_updates=false',
    "-Dupdate_server=$UpdateServer",
    "-Dproject_url=$ProjectUrl",
    '-Dresolve_vcs_tag=yes',
    '-Dbuild_tests=false',
    '-Dbuild_render=false',
    '-Dbuild_font=false',
    "-Dandroid_version_code=$AndroidVersionCode",
    "-Dandroid_keyalias=$KeyAlias"
)
if ($UpdateBuild -ge 0) {
    $setupArgs += "-Dupdate_build=$UpdateBuild"
}
if ($Keystore) {
    $Keystore = Resolve-RequiredPath $Keystore 'Android keystore'
    $setupArgs += "-Dandroid_keystore=$Keystore"
}
if (Test-Path -LiteralPath (Join-Path $buildPath 'meson-private\coredata.dat')) {
    $setupArgs = @('setup', '--reconfigure') + $setupArgs[1..($setupArgs.Count - 1)]
}

Push-Location $repoRoot
try {
    & $meson @setupArgs
    if ($LASTEXITCODE -ne 0) {
        throw "Meson setup failed with exit code $LASTEXITCODE"
    }

    $target = 'align-apk'
    $apkName = 'tpt-zh-omnipack.unsigned.apk'
    if ($Keystore) {
        if (-not $env:ANDROID_KEYSTORE_PASS) {
            throw 'ANDROID_KEYSTORE_PASS must be set when signing the APK'
        }
        $target = 'sign-apk'
        $apkName = 'tpt-zh-omnipack.apk'
    }
    & $meson compile -C $buildPath $target
    if ($LASTEXITCODE -ne 0) {
        throw "Android build failed with exit code $LASTEXITCODE"
    }
} finally {
    Pop-Location
}

$builtApk = Resolve-RequiredPath (Join-Path $buildPath "android\$apkName") 'Built APK'
$strippedLibrary = Resolve-RequiredPath (Join-Path $buildPath 'android\tpt-zh-omnipack.stripped.so') 'Stripped ARM64 library'
[IO.Directory]::CreateDirectory($outputPath) | Out-Null
$outputApk = Join-Path $outputPath "TPT-ZH-OmniPack-$ReleaseLabel-Android-arm64-v8a.apk"
[IO.File]::Copy($builtApk, $outputApk, $true)

& $zipalign -c -P 16 -v 4 $outputApk | Out-Null
if ($LASTEXITCODE -ne 0) {
    throw 'APK 16 KB zip alignment verification failed'
}
if ($Keystore) {
    $signatureDetails = @(& $apksigner verify --verbose --print-certs $outputApk 2>&1)
    if ($LASTEXITCODE -ne 0) {
        throw 'APK signature verification failed'
    }
    $signatureDetails | Write-Output
    $signatureText = $signatureDetails -join "`n"
    foreach ($scheme in ('v1', 'v2', 'v3')) {
        if ($signatureText -notmatch "Verified using $scheme scheme .*: true") {
            throw "APK signature scheme $scheme verification failed"
        }
    }
}

$programHeaders = @(& $readelf -lW $strippedLibrary)
if ($LASTEXITCODE -ne 0) {
    throw 'Unable to inspect ARM64 ELF program headers'
}
$loadSegments = @($programHeaders | Where-Object { $_ -match '^\s*LOAD\s' })
if ($loadSegments.Count -lt 2 -or @($loadSegments | Where-Object { $_ -notmatch '\s0x4000\s*$' }).Count) {
    throw 'ARM64 ELF LOAD segments are not all aligned to 16 KB'
}
$sectionHeaders = @(& $readelf -SW $strippedLibrary)
if ($LASTEXITCODE -ne 0) {
    throw 'Unable to inspect ARM64 ELF section headers'
}
if (($sectionHeaders -join "`n") -match '\.(?:debug_[A-Za-z0-9_]*|symtab|strtab)\b') {
    throw 'Packaged ARM64 library still contains debug or symbol-table sections'
}

$badging = @(& $aapt dump badging $outputApk)
if ($LASTEXITCODE -ne 0) {
    throw 'APK manifest inspection failed'
}
$packageLine = $badging | Select-Object -First 1
$hash = (Get-FileHash -Algorithm SHA256 -LiteralPath $outputApk).Hash
$size = (Get-Item -LiteralPath $outputApk).Length
$packageName = if ($packageLine -match "name='([^']+)'" ) { $Matches[1] } else { '' }
$versionCode = if ($packageLine -match "versionCode='([^']+)'" ) { $Matches[1] } else { '' }
$versionName = if ($packageLine -match "versionName='([^']+)'" ) { $Matches[1] } else { '' }
$minimumSdk = if (($badging -join "`n") -match "sdkVersion:'([^']+)'" ) { $Matches[1] } else { '' }
$targetSdk = if (($badging -join "`n") -match "targetSdkVersion:'([^']+)'" ) { $Matches[1] } else { '' }
$nativeCode = if (($badging -join "`n") -match "native-code:\s+(.+)$" ) { $Matches[1].Replace("'", '').Trim() } else { '' }
$certificateSha256 = ''
if ($Keystore) {
    $certificateLine = $signatureDetails | Where-Object { $_ -match 'certificate SHA-256 digest:' } | Select-Object -First 1
    if ($certificateLine -and $certificateLine -match 'digest:\s*([0-9A-Fa-f]+)') {
        $certificateSha256 = $Matches[1].ToUpperInvariant()
    }
    if ($certificateSha256 -notmatch '^[0-9A-F]{64}$') {
        throw 'Unable to read the APK signing certificate SHA-256'
    }
}
if ($packageName -ne 'org.tptzh.omnipack' -or $versionName -ne $Version -or
    $minimumSdk -ne '21' -or $targetSdk -ne '33' -or $nativeCode -ne 'arm64-v8a') {
    throw 'APK manifest metadata does not match the Android release contract'
}

$shaPath = "$outputApk.sha256"
[IO.File]::WriteAllText(
    $shaPath,
    "$hash  $([IO.Path]::GetFileName($outputApk))`n",
    [Text.Encoding]::ASCII)
$guideSource = Resolve-RequiredPath (Join-Path $repoRoot 'docs\ANDROID_USER_GUIDE.md') 'Android user guide'
$guidePath = Join-Path $outputPath 'README-Android.md'
[IO.File]::Copy($guideSource, $guidePath, $true)
$manifestPath = Join-Path $outputPath 'ANDROID-MANIFEST.json'
$manifest = [ordered]@{
    schema_version = 1
    product = 'TPT-ZH-OmniPack'
    version = $Version
    release_label = $ReleaseLabel
    source_revision = $sourceRevision
    source_tree_state = $sourceTreeState
    package_name = $packageName
    version_code = [int64]$versionCode
    version_name = $versionName
    update_build = if ($UpdateBuild -ge 0) { $UpdateBuild } else { [int]($Version.Split('.')[2]) }
    minimum_sdk = [int]$minimumSdk
    target_sdk = [int]$targetSdk
    abi = $nativeCode
    apk_file = [IO.Path]::GetFileName($outputApk)
    apk_bytes = $size
    apk_sha256 = $hash
    signed = [bool]$Keystore
    signing_certificate_sha256 = $certificateSha256
    zip_alignment_16k = $true
    elf_load_alignment_16k = $true
    stripped_native_library = $true
    signature_schemes = if ($Keystore) { @('v1', 'v2', 'v3') } else { @() }
    build_tests = $false
    automatic_update_checks = $true
    update_server = $UpdateServer
    project_url = $ProjectUrl
    public_tests_included = $false
    release_ready = $false
}
[IO.File]::WriteAllText(
    $manifestPath,
    ($manifest | ConvertTo-Json -Depth 4) + "`n",
    [Text.UTF8Encoding]::new($false))
Write-Output "android_apk=$outputApk"
Write-Output "android_apk_sha256=$hash"
Write-Output "android_apk_package=$packageLine"
Write-Output "android_update_build=$(if ($UpdateBuild -ge 0) { $UpdateBuild } else { [int]($Version.Split('.')[2]) })"
Write-Output "android_apk_signed=$([bool]$Keystore)"
Write-Output 'android_apk_zipalign_16k=true'
Write-Output 'android_elf_load_alignment_16k=true'
Write-Output 'android_native_library_stripped=true'
Write-Output "android_apk_sha256_file=$shaPath"
Write-Output "android_manifest=$manifestPath"
Write-Output "android_user_guide=$guidePath"
Write-Output "android_source_revision=$sourceRevision"
Write-Output "android_source_tree_state=$sourceTreeState"
