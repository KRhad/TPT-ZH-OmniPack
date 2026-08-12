param(
    [Parameter(Mandatory = $true)] [string] $Executable,
    [ValidateRange(1, 120)] [int] $TimeoutSeconds = 30,
    [string] $TemporaryDirectory = [System.IO.Path]::GetTempPath(),
    [string] $RuntimeDirectory = "C:\msys64\ucrt64\bin"
)
$ErrorActionPreference = "Stop"
$exe = (Resolve-Path -LiteralPath $Executable).Path
$runtime = (Resolve-Path -LiteralPath $RuntimeDirectory).Path
$autorun = Join-Path (Split-Path -Parent $MyInvocation.MyCommand.Path) "runtime\omni_alloy_composition.lua"
$root = Join-Path ([System.IO.Path]::GetFullPath($TemporaryDirectory)) ("tpt-omni-alloy-" + [guid]::NewGuid().ToString("N"))
$process = $null
$passed = $false
try {
    New-Item -ItemType Directory -Path $root | Out-Null
    [System.IO.File]::WriteAllText((Join-Path $root "powder.pref"), "{}" + [Environment]::NewLine,
        [System.Text.UTF8Encoding]::new($false))
    Copy-Item -LiteralPath $autorun -Destination (Join-Path $root "autorun.lua")
    $resultPath = Join-Path $root "omni-alloy-composition.result"
    $start = [System.Diagnostics.ProcessStartInfo]::new()
    $start.FileName = $exe
    $start.WorkingDirectory = $root
    $start.UseShellExecute = $false
    $start.CreateNoWindow = $true
    $start.ArgumentList.Add("ddir")
    $start.ArgumentList.Add($root)
    $start.Environment["PATH"] = $runtime + [System.IO.Path]::PathSeparator + $env:PATH
    foreach ($name in @("GITHUB_PAT_TOKEN", "GITHUB_TOKEN", "GH_TOKEN")) { [void]$start.Environment.Remove($name) }
    $process = [System.Diagnostics.Process]::Start($start)
    $deadline = [DateTime]::UtcNow.AddSeconds($TimeoutSeconds)
    do {
        Start-Sleep -Milliseconds 100
        $text = if (Test-Path $resultPath) { [string](Get-Content $resultPath -Raw) } else { "" }
        if ($text -match "(?m)^OMNI_ALLOY_COMPOSITION_STATUS=PASS\r?$") { $passed = $true; break }
        if ($text -match "(?m)^OMNI_ALLOY_COMPOSITION_STATUS=FAIL\r?$") { break }
    } while ([DateTime]::UtcNow -lt $deadline -and -not $process.HasExited)
    if (-not $passed) { throw "Omni alloy Lua runtime failed; result=$text; artifacts=$root" }
    Write-Output "runtime-lua-omni-alloy-composition: PASS"
    Write-Output $text.Trim()
}
finally {
    if ($process -and -not $process.HasExited) { Stop-Process -Id $process.Id -Force; $process.WaitForExit() }
    if ($passed -and (Test-Path $root)) { Remove-Item $root -Recurse -Force }
}
