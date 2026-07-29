param(
    [Parameter(Mandatory = $true)]
    [string] $Executable,

    [ValidateRange(1, 120)]
    [int] $TimeoutSeconds = 15
)

$ErrorActionPreference = "Stop"
$resolvedExecutable = (Resolve-Path -LiteralPath $Executable).Path
$scriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$autorunSource = Join-Path $scriptRoot "runtime\module_filter_regression.lua"
if (-not (Test-Path -LiteralPath $autorunSource -PathType Leaf)) {
    throw "Missing Lua regression script: $autorunSource"
}

$tempParent = [System.IO.Path]::GetFullPath([System.IO.Path]::GetTempPath())
$testRoot = Join-Path $tempParent ("tpt-omnipack-lua-" + [guid]::NewGuid().ToString("N"))
$resolvedTestRoot = [System.IO.Path]::GetFullPath($testRoot)
$tempPrefix = $tempParent.TrimEnd(
    [System.IO.Path]::DirectorySeparatorChar,
    [System.IO.Path]::AltDirectorySeparatorChar
) + [System.IO.Path]::DirectorySeparatorChar
if (-not $resolvedTestRoot.StartsWith(
    $tempPrefix,
    [System.StringComparison]::OrdinalIgnoreCase
)) {
    throw "Refusing to create a runtime test outside the temporary directory"
}

$process = $null
$passed = $false
try {
    New-Item -ItemType Directory -Path $resolvedTestRoot | Out-Null
    Copy-Item -LiteralPath $autorunSource -Destination (Join-Path $resolvedTestRoot "autorun.lua")

    $stdout = Join-Path $resolvedTestRoot "stdout.log"
    $stderr = Join-Path $resolvedTestRoot "stderr.log"
    $result = Join-Path $resolvedTestRoot "lua-module-regression.result"

    Remove-Item Env:GITHUB_PAT_TOKEN -ErrorAction SilentlyContinue
    $process = Start-Process `
        -FilePath $resolvedExecutable `
        -ArgumentList @("ddir", $resolvedTestRoot) `
        -WorkingDirectory $resolvedTestRoot `
        -WindowStyle Hidden `
        -RedirectStandardOutput $stdout `
        -RedirectStandardError $stderr `
        -PassThru

    $deadline = [DateTime]::UtcNow.AddSeconds($TimeoutSeconds)
    do {
        Start-Sleep -Milliseconds 100
        $resultText = if (Test-Path -LiteralPath $result) {
            [string](Get-Content -LiteralPath $result -Raw)
        } else {
            ""
        }
        if (
            $resultText -match "(?m)^OMNI_LUA_ALLOC_ID=255\r?$" -and
            $resultText -match "(?m)^OMNI_LUA_ACTIVE=OMNITEST_PT_LUA1\r?$"
        ) {
            $passed = $true
            break
        }
    } while ([DateTime]::UtcNow -lt $deadline -and -not $process.HasExited)

    $responding = $false
    if (-not $process.HasExited) {
        $process.Refresh()
        $responding = $process.Responding
    }

    if (-not $passed) {
        $stderrText = if (Test-Path -LiteralPath $stderr) {
            [string](Get-Content -LiteralPath $stderr -Raw)
        } else {
            ""
        }
        throw "Lua module regression failed; responding=$responding; stderr=$stderrText; artifacts=$resolvedTestRoot"
    }
    if (-not $responding) {
        throw "Lua module regression produced a result but the client stopped responding; artifacts=$resolvedTestRoot"
    }

    Write-Output "runtime-lua-module-test: PASS"
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
