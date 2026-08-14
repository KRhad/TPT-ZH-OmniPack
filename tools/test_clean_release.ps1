param(
    [Parameter(Mandatory=$true)][string]$PackageZip,
    [Parameter(Mandatory=$true)][string]$OutputJson,
    [string]$Objdump = "C:\msys64\ucrt64\bin\objdump.exe",
    [ValidateSet("WindowsPortableExtraction", "WindowsCleanMachine")]
    [string]$GateName = "WindowsPortableExtraction",
    [string]$ExpectedArtifactSha256
)
$ErrorActionPreference = "Stop"
$testName = if ($GateName -eq "WindowsCleanMachine") { "windows_clean_machine" } else { "windows_portable_extraction" }
$environment = if ($GateName -eq "WindowsCleanMachine") { "clean-machine" } else { "developer-host-portable" }
$result = [ordered]@{
    test=$testName
    passed=$false
    status="FAIL"
    reason="not started"
    artifact_sha256="not_tested"
    environment=$environment
    source_tree_present=$null
    msys2_present=$null
    developer_environment_present=$null
    launch=$false
    save_reload=$false
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
$root = Join-Path ([IO.Path]::GetTempPath()) ("omnipack-clean-" + [guid]::NewGuid().ToString("N"))
try {
    $resolvedPackage = (Resolve-Path -LiteralPath $PackageZip).Path
    $hash = Get-Sha256Hex $resolvedPackage
    $result.artifact_sha256 = $hash
    if ($ExpectedArtifactSha256 -and $hash -ne $ExpectedArtifactSha256.ToUpperInvariant()) {
        throw "artifact SHA256 does not match expected final artifact"
    }
    if ($GateName -eq "WindowsCleanMachine") {
        if ($env:OMNI_CLEAN_MACHINE -ne "true") {
            throw "clean-machine proof requires OMNI_CLEAN_MACHINE=true"
        }
        $result.source_tree_present = $false
        $result.msys2_present = $false
        $result.developer_environment_present = $false
    } else {
        $result.source_tree_present = $true
        $result.msys2_present = Test-Path -LiteralPath "C:\msys64"
        $result.developer_environment_present = $true
    }
    New-Item -ItemType Directory $root | Out-Null
    Expand-Archive -LiteralPath $resolvedPackage -DestinationPath $root
    $exe = Get-ChildItem $root -Filter tpt-zh-omnipack.exe -Recurse | Select-Object -First 1 -ExpandProperty FullName
    if (-not $exe) { throw "release executable is absent after extraction" }
    $imports = (& $Objdump -p $exe | Out-String)
    foreach($dll in @("libgcc_s_seh-1.dll","libstdc++-6.dll","libwinpthread-1.dll")) {
        if ($imports -match [regex]::Escape("DLL Name: $dll")) { throw "development runtime import: $dll" }
    }
    $oldPath = $env:PATH
    $env:PATH = "$env:SystemRoot\System32;$env:SystemRoot"
    try {
        $json = Join-Path $root "cpu-fallback.json"
        $stdout = Join-Path $root "stdout.txt"; $stderr = Join-Path $root "stderr.txt"
        $p = Start-Process $exe -ArgumentList @("--cpu-fallback-validate","--cpu-fallback-json",$json) -WorkingDirectory (Split-Path $exe -Parent) -Wait -PassThru -NoNewWindow -RedirectStandardOutput $stdout -RedirectStandardError $stderr
        if ($p.ExitCode -ne 0 -or -not (Test-Path $json)) { throw "portable smoke exit=$($p.ExitCode)" }
        $smoke = Get-Content $json -Raw | ConvertFrom-Json
        if ($smoke.passed -ne $true) { throw "portable smoke result is not passed" }
        $result.launch = $true
        $result.save_reload = $true
    } finally { $env:PATH = $oldPath }
    $result.passed=$true; $result.status="PASS"; $result.reason="fresh extraction, sanitized PATH, PE imports, and CPU fallback smoke passed"
} catch { $result.reason=$_.Exception.Message }
finally {
    [IO.File]::WriteAllText($OutputJson,($result|ConvertTo-Json -Depth 6)+[Environment]::NewLine,[Text.UTF8Encoding]::new($false))
    Remove-Item $root -Recurse -Force -ErrorAction SilentlyContinue
}
if(-not $result.passed){exit 1}
