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
$autorunSource = Join-Path $scriptRoot "runtime\alchemy_gate_regression.lua"
if (-not (Test-Path -LiteralPath $autorunSource -PathType Leaf)) {
    throw "Missing Lua alchemy regression script: $autorunSource"
}
$tempParent = [System.IO.Path]::GetFullPath($TemporaryDirectory)
if (-not (Test-Path -LiteralPath $tempParent -PathType Container)) {
    throw "Temporary directory does not exist: $tempParent"
}
$tempPrefix = $tempParent.TrimEnd(
    [System.IO.Path]::DirectorySeparatorChar,
    [System.IO.Path]::AltDirectorySeparatorChar) + [System.IO.Path]::DirectorySeparatorChar

function Invoke-AlchemyMode {
    param(
        [Parameter(Mandatory = $true)]
        [ValidateSet("locked", "free", "stamp-source")]
        [string] $Mode,

        [string] $StampPath,

        [switch] $KeepRoot
    )

    $testRoot = [System.IO.Path]::GetFullPath((Join-Path $tempParent (
        "tpt-omnipack-alchemy-$Mode-" + [guid]::NewGuid().ToString("N"))))
    if (-not $testRoot.StartsWith($tempPrefix, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to create alchemy test directory outside the temporary root"
    }
    $process = $null
    $passed = $false
    try {
        New-Item -ItemType Directory -Path $testRoot | Out-Null
        Copy-Item -LiteralPath $autorunSource -Destination (Join-Path $testRoot "autorun.lua")
        [System.IO.File]::WriteAllText(
            (Join-Path $testRoot "alchemy-mode.txt"), $Mode + "`n", [System.Text.Encoding]::ASCII)
        $enabled = if ($Mode -eq "locked") { "true" } else { "false" }
        $preference = '{"Omni":{"Progress":{"AlchemyMode":' + $enabled + '}}}'
        [System.IO.File]::WriteAllText(
            (Join-Path $testRoot "powder.pref"), $preference, [System.Text.Encoding]::ASCII)
        if ($StampPath) {
            $stampTarget = Join-Path $testRoot "alchemy-locked.stm"
            Copy-Item -LiteralPath $StampPath -Destination $stampTarget
            [System.IO.File]::WriteAllText(
                (Join-Path $testRoot "locked-stamp.txt"), "alchemy-locked.stm`n", [System.Text.Encoding]::ASCII)
        }

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
        if (-not $process) { throw "Failed to start alchemy regression client" }

        $resultPath = Join-Path $testRoot "lua-alchemy-gate.result"
        $deadline = [DateTime]::UtcNow.AddSeconds($TimeoutSeconds)
        $resultText = ""
        do {
            Start-Sleep -Milliseconds 100
            if (Test-Path -LiteralPath $resultPath -PathType Leaf) {
                $resultText = [string](Get-Content -LiteralPath $resultPath -Raw)
                if ($resultText -match "(?m)^OMNI_ALCHEMY_GATE_STATUS=(PASS|FAIL)\r?$") { break }
            }
            $process.Refresh()
        } while ([DateTime]::UtcNow -lt $deadline -and -not $process.HasExited)

        if ($resultText -notmatch "(?m)^OMNI_ALCHEMY_GATE_STATUS=PASS\r?$" -or
            $resultText -notmatch "(?m)^OMNI_ALCHEMY_GATE_MODE=$Mode\r?$") {
            throw "Alchemy Lua regression failed; mode=$Mode; result=$resultText; artifacts=$testRoot"
        }
        if ($process.HasExited -and $process.ExitCode -ne 0) {
            throw "Alchemy client exited with code $($process.ExitCode); mode=$Mode; artifacts=$testRoot"
        }
        $passed = $true
        return [pscustomobject]@{ Root = $testRoot; Text = $resultText.Trim() }
    }
    finally {
        if ($process -and -not $process.HasExited) {
            Stop-Process -Id $process.Id -Force
            $process.WaitForExit()
        }
        if ($process) { $process.Dispose() }
        if ($passed -and -not $KeepRoot -and (Test-Path -LiteralPath $testRoot)) {
            Remove-Item -LiteralPath $testRoot -Recurse -Force
        }
    }
}

$source = $null
try {
    $source = Invoke-AlchemyMode -Mode stamp-source -KeepRoot
    if ($source.Text -notmatch "(?m)^OMNI_ALCHEMY_STAMP_ID=([^\r\n]+)\r?$") {
        throw "Stamp-source run did not report a stamp ID: $($source.Text)"
    }
    $stampPath = Join-Path $source.Root ("stamps\" + $matches[1] + ".stm")
    if (-not (Test-Path -LiteralPath $stampPath -PathType Leaf)) {
        throw "Stamp-source run did not create expected fixture: $stampPath"
    }
    $locked = Invoke-AlchemyMode -Mode locked -StampPath $stampPath
    $free = Invoke-AlchemyMode -Mode free
    Write-Output "runtime-lua-alchemy-test: PASS (gate_modes=2 fixture_runs=1 assertions=22)"
    Write-Output $source.Text
    Write-Output $locked.Text
    Write-Output $free.Text
}
finally {
    if ($source -and $source.Root.StartsWith($tempPrefix, [System.StringComparison]::OrdinalIgnoreCase) -and
        (Test-Path -LiteralPath $source.Root)) {
        Remove-Item -LiteralPath $source.Root -Recurse -Force
    }
}
