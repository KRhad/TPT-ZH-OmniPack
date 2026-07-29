param(
    [Parameter(Mandatory = $true)]
    [string] $Executable,

    [ValidateRange(1, 120)]
    [int] $TimeoutSeconds = 30,

    [string] $TemporaryDirectory = [System.IO.Path]::GetTempPath()
)

$ErrorActionPreference = "Stop"
$resolvedExecutable = (Resolve-Path -LiteralPath $Executable).Path
$scriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$autorunSource = Join-Path $scriptRoot "runtime\nuclear_regression.lua"
if (-not (Test-Path -LiteralPath $autorunSource -PathType Leaf)) {
    throw "Missing Lua regression script: $autorunSource"
}

$tempParent = [System.IO.Path]::GetFullPath($TemporaryDirectory)
if (-not (Test-Path -LiteralPath $tempParent -PathType Container)) {
    throw "Temporary directory does not exist: $tempParent"
}
$testRoot = Join-Path $tempParent ("tpt-omnipack-nuclear-" + [guid]::NewGuid().ToString("N"))
$resolvedTestRoot = [System.IO.Path]::GetFullPath($testRoot)
$tempPrefix = $tempParent.TrimEnd([System.IO.Path]::DirectorySeparatorChar, [System.IO.Path]::AltDirectorySeparatorChar) + [System.IO.Path]::DirectorySeparatorChar
if (-not $resolvedTestRoot.StartsWith($tempPrefix, [System.StringComparison]::OrdinalIgnoreCase)) {
    throw "Refusing to create a runtime test outside the temporary directory"
}

$process = $null
$passed = $false
try {
    New-Item -ItemType Directory -Path $resolvedTestRoot | Out-Null
    Copy-Item -LiteralPath $autorunSource -Destination (Join-Path $resolvedTestRoot "autorun.lua")
    $result = Join-Path $resolvedTestRoot "lua-nuclear-regression.result"
    Remove-Item Env:GITHUB_PAT_TOKEN -ErrorAction SilentlyContinue
    $startInfo = [System.Diagnostics.ProcessStartInfo]::new()
    $startInfo.FileName = $resolvedExecutable
    $startInfo.WorkingDirectory = $resolvedTestRoot
    $startInfo.UseShellExecute = $false
    $startInfo.CreateNoWindow = $true
    $startInfo.ArgumentList.Add("ddir")
    $startInfo.ArgumentList.Add($resolvedTestRoot)
    $process = [System.Diagnostics.Process]::Start($startInfo)
    if (-not $process) {
        throw "Failed to start the client"
    }

    $deadline = [DateTime]::UtcNow.AddSeconds($TimeoutSeconds)
    do {
        Start-Sleep -Milliseconds 100
        $resultText = if (Test-Path -LiteralPath $result) { [string](Get-Content -LiteralPath $result -Raw) } else { "" }
        if ($resultText -match "(?m)^OMNI_NUCLEAR_STATUS=PASS\r?$") {
            $passed = $true
            break
        }
        if ($resultText -match "(?m)^OMNI_NUCLEAR_STATUS=FAIL\r?$") {
            break
        }
    } while ([DateTime]::UtcNow -lt $deadline -and -not $process.HasExited)

    $responding = $false
    if (-not $process.HasExited) {
        $process.Refresh()
        $responding = $process.Responding
    }
    if (-not $passed) {
        throw "Lua nuclear regression failed; responding=$responding; result=$resultText; artifacts=$resolvedTestRoot"
    }
    if (-not $responding) {
        throw "Lua nuclear regression produced PASS but the client stopped responding; artifacts=$resolvedTestRoot"
    }
    Write-Output "runtime-lua-nuclear-test: PASS"
    Write-Output $resultText.Trim()
}
finally {
    if ($process -and -not $process.HasExited) {
        Stop-Process -Id $process.Id -Force
        $process.WaitForExit()
    }
    if ($passed -and (Test-Path -LiteralPath $resolvedTestRoot)) {
        Remove-Item -LiteralPath $resolvedTestRoot -Recurse -Force
    }
}
