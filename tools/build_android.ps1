[CmdletBinding()]
param(
    [string]$SdkRoot = $env:ANDROID_SDK_ROOT,
    [string]$NdkVersion = '29.0.14206865',
    [string]$BuildToolsVersion = '35.0.0',
    [string]$AndroidPlatform = 'android-31',
    [string]$Jdk8Root = $env:JAVA8_HOME,
    [string]$JavaRuntimeRoot = $env:JAVA_HOME,
    [string]$BuildDirectory = 'build-android-arm64',
    [string]$OutputDirectory = 'artifacts/android/1.0.0-mobile-test1',
    [string]$Version = '1.0.0',
    [string]$ReleaseLabel = '1.0.0-mobile-test1',
    [int]$AndroidVersionCode = 0,
    [string]$Keystore = '',
    [string]$KeyAlias = 'androidkey'
)

$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path

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
    '-Dresolve_vcs_tag=yes',
    '-Dbuild_render=false',
    '-Dbuild_font=false',
    "-Dandroid_version_code=$AndroidVersionCode",
    "-Dandroid_keyalias=$KeyAlias"
)
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
[IO.Directory]::CreateDirectory($outputPath) | Out-Null
$outputApk = Join-Path $outputPath "TPT-ZH-OmniPack-$ReleaseLabel-Android-arm64-v8a.apk"
[IO.File]::Copy($builtApk, $outputApk, $true)

& $zipalign -c -P 16 -v 4 $outputApk | Out-Null
if ($LASTEXITCODE -ne 0) {
    throw 'APK 16 KB zip alignment verification failed'
}
if ($Keystore) {
    & $apksigner verify --verbose --print-certs $outputApk
    if ($LASTEXITCODE -ne 0) {
        throw 'APK signature verification failed'
    }
}

$badging = & $aapt dump badging $outputApk
if ($LASTEXITCODE -ne 0) {
    throw 'APK manifest inspection failed'
}
$packageLine = $badging | Select-Object -First 1
$hash = (Get-FileHash -Algorithm SHA256 -LiteralPath $outputApk).Hash
Write-Output "android_apk=$outputApk"
Write-Output "android_apk_sha256=$hash"
Write-Output "android_apk_package=$packageLine"
Write-Output "android_apk_signed=$([bool]$Keystore)"
Write-Output 'android_apk_zipalign_16k=true'
