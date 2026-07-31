param(
    [Parameter(Mandatory = $true)]
    [string] $Executable,

    [ValidateRange(1, 180)]
    [int] $TimeoutSeconds = 90,

    [string] $TemporaryDirectory = [System.IO.Path]::GetTempPath(),

    [switch] $KeepArtifacts
)

$ErrorActionPreference = "Stop"
$resolvedExecutable = (Resolve-Path -LiteralPath $Executable).Path
$scriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$autorunSource = Join-Path $scriptRoot "runtime\alchemy_progression_regression.lua"
if (-not (Test-Path -LiteralPath $autorunSource -PathType Leaf)) {
    throw "Missing Lua alchemy progression script: $autorunSource"
}
$tempParent = [System.IO.Path]::GetFullPath($TemporaryDirectory)
$tempPrefix = $tempParent.TrimEnd(
    [System.IO.Path]::DirectorySeparatorChar,
    [System.IO.Path]::AltDirectorySeparatorChar) + [System.IO.Path]::DirectorySeparatorChar

function Invoke-ProgressionMode {
    param(
        [Parameter(Mandatory = $true)]
        [ValidateSet("modules-on", "modules-off")]
        [string] $Mode
    )

    $testRoot = [System.IO.Path]::GetFullPath((Join-Path $tempParent (
        "tpt-omnipack-alchemy-progression-$Mode-" + [guid]::NewGuid().ToString("N"))))
    if (-not $testRoot.StartsWith($tempPrefix, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to create alchemy progression directory outside temporary root"
    }
    $process = $null
    $passed = $false
    try {
        New-Item -ItemType Directory -Path $testRoot | Out-Null
        Copy-Item -LiteralPath $autorunSource -Destination (Join-Path $testRoot "autorun.lua")
        [System.IO.File]::WriteAllText(
            (Join-Path $testRoot "alchemy-progression-mode.txt"),
            $Mode + "`n",
            [System.Text.Encoding]::ASCII)
        $preference = if ($Mode -eq "modules-off") {
            '{"Omni":{"Progress":{"AlchemyMode":true},"Modules":{"Metallurgy":false,"Chemistry":false}}}'
        } else {
            '{"Omni":{"Progress":{"AlchemyMode":true},"Modules":{"Metallurgy":true,"Chemistry":true}}}'
        }
        [System.IO.File]::WriteAllText(
            (Join-Path $testRoot "powder.pref"), $preference, [System.Text.Encoding]::ASCII)

        $startInfo = [System.Diagnostics.ProcessStartInfo]::new()
        $startInfo.FileName = $resolvedExecutable
        $startInfo.WorkingDirectory = $testRoot
        $startInfo.UseShellExecute = $false
        $startInfo.CreateNoWindow = $true
        $startInfo.ArgumentList.Add("ddir")
        $startInfo.ArgumentList.Add($testRoot)
        foreach ($secretName in @("GITHUB_PAT_TOKEN", "GITHUB_TOKEN", "GH_TOKEN")) {
            [void]$startInfo.Environment.Remove($secretName)
        }
        $process = [System.Diagnostics.Process]::Start($startInfo)
        if (-not $process) { throw "Failed to start alchemy progression client" }

        $resultPath = Join-Path $testRoot "lua-alchemy-progression.result"
        $deadline = [DateTime]::UtcNow.AddSeconds($TimeoutSeconds)
        $resultText = ""
        do {
            Start-Sleep -Milliseconds 100
            if (Test-Path -LiteralPath $resultPath -PathType Leaf) {
                $resultText = [string](Get-Content -LiteralPath $resultPath -Raw)
                if ($resultText -match "(?m)^OMNI_ALCHEMY_PROGRESSION_STATUS=(PASS|FAIL)\r?$") { break }
            }
            $process.Refresh()
        } while ([DateTime]::UtcNow -lt $deadline -and -not $process.HasExited)

        if ($resultText -notmatch "(?m)^OMNI_ALCHEMY_PROGRESSION_STATUS=PASS\r?$" -or
            $resultText -notmatch "(?m)^OMNI_ALCHEMY_STAGES=10\r?$" -or
            $resultText -notmatch "(?m)^OMNI_ALCHEMY_MODULE_MODE=$Mode\r?$" -or
            $resultText -notmatch "(?m)^OMNI_ALCHEMY_MASTERY=true\r?$") {
            throw "Alchemy progression failed; mode=$Mode; result=$resultText; artifacts=$testRoot"
        }
        if ($process.HasExited -and $process.ExitCode -ne 0) {
            throw "Alchemy progression client exited with code $($process.ExitCode); mode=$Mode; artifacts=$testRoot"
        }
        $passed = $true
        return $resultText.Trim()
    }
    finally {
        if ($process -and -not $process.HasExited) {
            Stop-Process -Id $process.Id -Force
            $process.WaitForExit()
        }
        if ($process) { $process.Dispose() }
        if ($passed -and -not $KeepArtifacts -and (Test-Path -LiteralPath $testRoot)) {
            Remove-Item -LiteralPath $testRoot -Recurse -Force
        }
    }
}

$modulesOn = Invoke-ProgressionMode -Mode modules-on
$modulesOff = Invoke-ProgressionMode -Mode modules-off
Write-Output "runtime-lua-alchemy-progression-test: PASS (modes=2 stages=20 total_frames=4620)"
Write-Output $modulesOn
Write-Output $modulesOff
