param(
    [Parameter(Mandatory = $true)] [string] $Executable,
    [ValidateRange(1, 120)] [int] $TimeoutSeconds = 30,
    [string] $TemporaryDirectory = [System.IO.Path]::GetTempPath(),
    [string] $RuntimeDirectory = "C:\msys64\ucrt64\bin"
)
$ErrorActionPreference = "Stop"
$resolvedExecutable = (Resolve-Path -LiteralPath $Executable).Path
$resolvedRuntimeDirectory = (Resolve-Path -LiteralPath $RuntimeDirectory).Path
$autorunSource = Join-Path (Split-Path -Parent $MyInvocation.MyCommand.Path) "runtime\omni_solution_runtime.lua"
$tempParent = [System.IO.Path]::GetFullPath($TemporaryDirectory)
$testRoot = Join-Path $tempParent ("tpt-omni-solution-" + [guid]::NewGuid().ToString("N"))
$resolvedTestRoot = [System.IO.Path]::GetFullPath($testRoot)
$tempPrefix = $tempParent.TrimEnd([System.IO.Path]::DirectorySeparatorChar,
    [System.IO.Path]::AltDirectorySeparatorChar) + [System.IO.Path]::DirectorySeparatorChar
if (-not $resolvedTestRoot.StartsWith($tempPrefix, [System.StringComparison]::OrdinalIgnoreCase)) {
    throw "Refusing to create runtime test outside temporary directory"
}
$process = $null
$passed = $false
try {
    New-Item -ItemType Directory -Path $resolvedTestRoot | Out-Null
    [System.IO.File]::WriteAllText((Join-Path $resolvedTestRoot "powder.pref"),
        "{}" + [Environment]::NewLine, [System.Text.UTF8Encoding]::new($false))
    Copy-Item -LiteralPath $autorunSource -Destination (Join-Path $resolvedTestRoot "autorun.lua")
    $resultPath = Join-Path $resolvedTestRoot "omni-solution-runtime.result"
    $startInfo = [System.Diagnostics.ProcessStartInfo]::new()
    $startInfo.FileName = $resolvedExecutable
    $startInfo.WorkingDirectory = $resolvedTestRoot
    $startInfo.UseShellExecute = $false
    $startInfo.CreateNoWindow = $true
    $startInfo.ArgumentList.Add("ddir")
    $startInfo.ArgumentList.Add($resolvedTestRoot)
    $startInfo.Environment["PATH"] = $resolvedRuntimeDirectory + [System.IO.Path]::PathSeparator + $env:PATH
    foreach ($secretName in @("GITHUB_PAT_TOKEN", "GITHUB_TOKEN", "GH_TOKEN")) {
        [void]$startInfo.Environment.Remove($secretName)
    }
    $process = [System.Diagnostics.Process]::Start($startInfo)
    if (-not $process) { throw "Failed to start the client" }
    $deadline = [DateTime]::UtcNow.AddSeconds($TimeoutSeconds)
    do {
        Start-Sleep -Milliseconds 100
        $resultText = if (Test-Path -LiteralPath $resultPath) {
            [string](Get-Content -LiteralPath $resultPath -Raw)
        } else { "" }
        if ($resultText -match "(?m)^OMNI_SOLUTION_STATUS=PASS\r?$") { $passed = $true; break }
        if ($resultText -match "(?m)^OMNI_SOLUTION_STATUS=FAIL\r?$") { break }
    } while ([DateTime]::UtcNow -lt $deadline -and -not $process.HasExited)
    if (-not $passed) { throw "OmniSolution Lua runtime failed; result=$resultText; artifacts=$resolvedTestRoot" }
    Write-Output "runtime-lua-omni-solution: PASS"
    Write-Output $resultText.Trim()
}
finally {
    if ($process -and -not $process.HasExited) { Stop-Process -Id $process.Id -Force; $process.WaitForExit() }
    if ($passed -and (Test-Path -LiteralPath $resolvedTestRoot)) {
        Remove-Item -LiteralPath $resolvedTestRoot -Recurse -Force
    }
}
