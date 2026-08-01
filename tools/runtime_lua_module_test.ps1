param(
    [Parameter(Mandatory = $true)]
    [string] $Executable,

    [ValidateRange(1, 120)]
    [int] $TimeoutSeconds = 15,

    [string] $TemporaryDirectory = [System.IO.Path]::GetTempPath()
)

$ErrorActionPreference = "Stop"
$resolvedExecutable = (Resolve-Path -LiteralPath $Executable).Path
$scriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$autorunSource = Join-Path $scriptRoot "runtime\module_filter_regression.lua"
if (-not (Test-Path -LiteralPath $autorunSource -PathType Leaf)) {
    throw "Missing Lua regression script: $autorunSource"
}

$tempParent = [System.IO.Path]::GetFullPath($TemporaryDirectory)
if (-not (Test-Path -LiteralPath $tempParent -PathType Container)) {
    throw "Temporary directory does not exist: $tempParent"
}
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

    $result = Join-Path $resolvedTestRoot "lua-module-regression.result"

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
        $resultText = if (Test-Path -LiteralPath $result) {
            [string](Get-Content -LiteralPath $result -Raw)
        } else {
            ""
        }
        if (
            $resultText -match "(?m)^OMNI_LUA_ALLOC_ID=255\r?$" -and
            $resultText -match "(?m)^OMNI_LUA_ACTIVE=OMNITEST_PT_LUA1\r?$" -and
            $resultText -match "(?m)^OMNI_PERIODIC_ALKALI_ACTIVE=OMNI_PT_NA\r?$" -and
            $resultText -match "(?m)^OMNI_PERIODIC_ALKALINE_EARTH_ACTIVE=OMNI_PT_CA\r?$" -and
            $resultText -match "(?m)^OMNI_PERIODIC_BORON_GROUP_ACTIVE=OMNI_PT_B\r?$" -and
            $resultText -match "(?m)^OMNI_PERIODIC_CARBON_GROUP_ACTIVE=OMNI_PT_GE\r?$" -and
            $resultText -match "(?m)^OMNI_PERIODIC_CARBON_SUPERHEAVY_ACTIVE=OMNI_PT_FL\r?$" -and
            $resultText -match "(?m)^OMNI_PERIODIC_NITROGEN_GROUP_ACTIVE=OMNI_PT_N\r?$" -and
            $resultText -match "(?m)^OMNI_PERIODIC_NITROGEN_SUPERHEAVY_ACTIVE=OMNI_PT_MC\r?$" -and
            $resultText -match "(?m)^OMNI_PERIODIC_OXYGEN_GROUP_ACTIVE=OMNI_PT_S\r?$" -and
            $resultText -match "(?m)^OMNI_PERIODIC_OXYGEN_SUPERHEAVY_ACTIVE=OMNI_PT_LV\r?$" -and
            $resultText -match "(?m)^OMNI_PERIODIC_HALOGEN_ACTIVE=OMNI_PT_F\r?$" -and
            $resultText -match "(?m)^OMNI_PERIODIC_HALOGEN_SUPERHEAVY_ACTIVE=OMNI_PT_TS\r?$" -and
            $resultText -match "(?m)^OMNI_PERIODIC_FIRST_TRANSITION_ACTIVE=OMNI_PT_SC\r?$" -and
            $resultText -match "(?m)^OMNI_PERIODIC_SECOND_TRANSITION_ACTIVE=OMNI_PT_Y\r?$" -and
            $resultText -match "(?m)^OMNI_PERIODIC_THIRD_TRANSITION_ACTIVE=OMNI_PT_HF\r?$" -and
            $resultText -match "(?m)^OMNI_PERIODIC_LANTHANIDE_ACTIVE=OMNI_PT_LA\r?$" -and
            $resultText -match "(?m)^OMNI_PERIODIC_ACTINIDE_ACTIVE=OMNI_PT_AC\r?$" -and
            $resultText -match "(?m)^OMNI_PERIODIC_SUPERHEAVY_ACTIVE=OMNI_PT_RF\r?$" -and
            $resultText -match "(?m)^OMNI_INORGANIC_ACTIVE=OMNI_PT_HCLA\r?$" -and
            $resultText -match "(?m)^OMNI_INORGANIC_BATCH2_ACTIVE=OMNI_PT_CARA\r?$" -and
            $resultText -match "(?m)^OMNI_INORGANIC_BATCH3_ACTIVE=OMNI_PT_AMCL\r?$" -and
            $resultText -match "(?m)^OMNI_ENGINEERING_HIGH_ID_ACTIVE=OMNI_PT_NITI\r?$" -and
            $resultText -match "(?m)^OMNI_MATERIALS_HIGH_ID_ACTIVE=OMNI_PT_RFBK\r?$" -and
            $resultText -match "(?m)^OMNI_ISOTOPE_HIGH_ID_ACTIVE=OMNI_PT_CF52\r?$" -and
            $resultText -match "(?m)^OMNI_ORGANIC_HIGH_ID_ACTIVE=OMNI_PT_EACT\r?$" -and
            $resultText -match "(?m)^OMNI_ELECTRONICS_HIGH_ID_ACTIVE=OMNI_PT_DIEL\r?$" -and
            $resultText -match "(?m)^OMNI_ENVIRONMENT_HIGH_ID_ACTIVE=OMNI_PT_DETG\r?$"
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
        throw "Lua module regression failed; responding=$responding; artifacts=$resolvedTestRoot"
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
