param(
    [Parameter(Mandatory = $true)]
    [string] $Executable,

    [ValidateRange(1, 180)]
    [int] $TimeoutSeconds = 30,

    [string] $TemporaryDirectory = [System.IO.Path]::GetTempPath(),

    [switch] $KeepArtifacts
)

$ErrorActionPreference = "Stop"
$resolvedExecutable = (Resolve-Path -LiteralPath $Executable).Path
$scriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$autorunSource = Join-Path $scriptRoot "runtime\disabled_module_regression.lua"
if (-not (Test-Path -LiteralPath $autorunSource -PathType Leaf)) {
    throw "Missing disabled-module regression script: $autorunSource"
}

$tempParent = [System.IO.Path]::GetFullPath($TemporaryDirectory)
if (-not (Test-Path -LiteralPath $tempParent -PathType Container)) {
    throw "Temporary directory does not exist: $tempParent"
}
$testRoot = Join-Path $tempParent (
    "tpt-omnipack-disabled-module-" + [guid]::NewGuid().ToString("N")
)
$resolvedTestRoot = [System.IO.Path]::GetFullPath($testRoot)
$tempPrefix = $tempParent.TrimEnd(
    [System.IO.Path]::DirectorySeparatorChar,
    [System.IO.Path]::AltDirectorySeparatorChar
) + [System.IO.Path]::DirectorySeparatorChar
if (-not $resolvedTestRoot.StartsWith(
    $tempPrefix,
    [System.StringComparison]::OrdinalIgnoreCase
)) {
    throw "Refusing to create a disabled-module test outside the temporary directory"
}

function Remove-DisabledModuleRoot {
    if (-not (Test-Path -LiteralPath $resolvedTestRoot)) { return }
    for ($attempt = 1; $attempt -le 20; $attempt++) {
        try {
            Remove-Item -LiteralPath $resolvedTestRoot -Recurse -Force -ErrorAction Stop
            return
        }
        catch {
            if ($attempt -eq 20) {
                Write-Warning (
                    "Could not remove disabled-module temporary directory: " +
                    "$resolvedTestRoot ($($_.Exception.Message))"
                )
                return
            }
            Start-Sleep -Milliseconds 100
        }
    }
}

function Invoke-DisabledModulePhase {
    param(
        [Parameter(Mandatory = $true)]
        [ValidateRange(1, 2)]
        [int] $Phase,

        [Parameter(Mandatory = $true)]
        [bool] $ModulesEnabled
    )

    [System.IO.File]::WriteAllText(
        (Join-Path $resolvedTestRoot "disabled-module.phase"),
        [string]$Phase,
        [System.Text.Encoding]::ASCII
    )
    $preference = if ($ModulesEnabled) {
        '{"Omni":{"Modules":{"Metallurgy":true,"Biology":true,"Chemistry":true,"AdvancedNuclear":true}}}'
    } else {
        '{"Omni":{"Modules":{"Metallurgy":false,"Biology":false,"Chemistry":false,"AdvancedNuclear":false}}}'
    }
    [System.IO.File]::WriteAllText(
        (Join-Path $resolvedTestRoot "powder.pref"),
        $preference,
        [System.Text.UTF8Encoding]::new($false)
    )
    $resultPath = Join-Path $resolvedTestRoot (
        "disabled-module-phase" + $Phase + ".result"
    )

    $startInfo = [System.Diagnostics.ProcessStartInfo]::new()
    $startInfo.FileName = $resolvedExecutable
    $startInfo.WorkingDirectory = $resolvedTestRoot
    $startInfo.UseShellExecute = $false
    $startInfo.CreateNoWindow = $true
    $startInfo.ArgumentList.Add("ddir")
    $startInfo.ArgumentList.Add($resolvedTestRoot)
    foreach ($secretName in @("GITHUB_PAT_TOKEN", "GITHUB_TOKEN", "GH_TOKEN")) {
        [void]$startInfo.Environment.Remove($secretName)
    }

    $process = [System.Diagnostics.Process]::Start($startInfo)
    if (-not $process) {
        throw "Failed to start the client for disabled-module phase $Phase"
    }
    try {
        $deadline = [DateTime]::UtcNow.AddSeconds($TimeoutSeconds)
        $resultText = ""
        do {
            Start-Sleep -Milliseconds 100
            if (Test-Path -LiteralPath $resultPath -PathType Leaf) {
                $resultText = [string](Get-Content -LiteralPath $resultPath -Raw)
                if ($resultText -match "(?m)^OMNI_DISABLED_MODULE_STATUS=(PASS|FAIL)\r?$") {
                    break
                }
            }
            $process.Refresh()
        } while ([DateTime]::UtcNow -lt $deadline -and -not $process.HasExited)

        if (-not $process.HasExited) {
            [void]$process.WaitForExit(2000)
        }
        if (-not $process.HasExited) {
            Stop-Process -Id $process.Id -Force
            $process.WaitForExit()
            throw "Disabled-module phase $Phase timed out; artifacts=$resolvedTestRoot"
        }
        if (-not $resultText -and (Test-Path -LiteralPath $resultPath -PathType Leaf)) {
            $resultText = [string](Get-Content -LiteralPath $resultPath -Raw)
        }
        if ($resultText -notmatch "(?m)^OMNI_DISABLED_MODULE_STATUS=PASS\r?$" -or
            $process.ExitCode -ne 0) {
            throw (
                "Disabled-module phase $Phase failed; exit_code=$($process.ExitCode); " +
                "result=$resultText; artifacts=$resolvedTestRoot"
            )
        }
        return $resultText.Trim()
    }
    finally {
        if (-not $process.HasExited) {
            Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue
            $process.WaitForExit()
        }
        $process.Dispose()
    }
}

$passed = $false
try {
    New-Item -ItemType Directory -Path $resolvedTestRoot | Out-Null
    Copy-Item -LiteralPath $autorunSource -Destination (
        Join-Path $resolvedTestRoot "autorun.lua"
    )
    $phase1 = Invoke-DisabledModulePhase -Phase 1 -ModulesEnabled $true
    $phase2 = Invoke-DisabledModulePhase -Phase 2 -ModulesEnabled $false
    foreach ($required in @(
        "OMNI_DISABLED_MODULES=metallurgy,biology,chemistry,advanced_nuclear",
        "OMNI_DISABLED_MODULE_METALLURGY=OMNI_PT_ALUM",
        "OMNI_DISABLED_MODULE_ENGINEERING=OMNI_PT_NITI",
        "OMNI_DISABLED_MODULE_MATERIAL=OMNI_PT_RFBK",
        "OMNI_DISABLED_MODULE_SCRAP=DEFAULT_PT_BRMT",
        "OMNI_DISABLED_MODULE_BIOLOGY=OMNI_PT_STER",
        "OMNI_DISABLED_MODULE_CORE=OMNI_PT_CHLR",
        "OMNI_DISABLED_MODULE_EXPANSION=OMNI_PT_HCLA",
        "OMNI_DISABLED_MODULE_BATCH2=OMNI_PT_CARA",
        "OMNI_DISABLED_MODULE_BATCH3=OMNI_PT_AMCL",
        "OMNI_DISABLED_MODULE_NUCLEAR=OMNI_PT_NCLT",
        "OMNI_DISABLED_MODULE_ISOTOPE=OMNI_PT_CF52",
        "OMNI_DISABLED_MODULE_ORGANIC=OMNI_PT_EACT",
        "OMNI_DISABLED_MODULE_LOADED_PARTICLES=20",
        "OMNI_DISABLED_MODULE_UPDATE_EVENTS=0",
        "OMNI_DISABLED_MODULE_PERIODIC_ACTIVE=OMNI_PT_HE",
        "OMNI_DISABLED_MODULE_OPS_FORMAT=OPS1"
    )) {
        if ($phase2 -notmatch "(?m)^$([regex]::Escape($required))\r?$") {
            throw "Disabled-module phase 2 is missing '$required'; artifacts=$resolvedTestRoot"
        }
    }
    $passed = $true
    Write-Output "runtime-lua-disabled-module-test: PASS"
    Write-Output $phase2
}
finally {
    if ($passed -and -not $KeepArtifacts) {
        Remove-DisabledModuleRoot
    }
}
