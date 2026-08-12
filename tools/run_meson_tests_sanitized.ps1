param(
    [Parameter(Mandatory = $true)]
    [string] $BuildDir,

    [string] $Suite = ""
)

$ErrorActionPreference = "Stop"

# Meson serializes the complete child environment in testlog.json. Remove
# credential-like variables before launching it so local ignored logs cannot
# retain secrets from the interactive shell.
Get-ChildItem Env: | Where-Object {
    $_.Name -match '(?i)(TOKEN|SECRET|PASSWORD|PASSWD|API_KEY|PAT$|PAT_)'
} | ForEach-Object {
    Remove-Item -LiteralPath ("Env:" + $_.Name) -ErrorAction SilentlyContinue
}

$env:PATH = "C:\msys64\ucrt64\bin;C:\msys64\usr\bin;" + $env:PATH
$mesonArgs = @(
    "test",
    "-C", $BuildDir,
    "--no-rebuild",
    "--print-errorlogs"
)
if ($Suite) {
    $mesonArgs += @("--suite", $Suite)
}

& C:\msys64\ucrt64\bin\meson.exe @mesonArgs

exit $LASTEXITCODE
