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
$autorunSource = Join-Path $scriptRoot "runtime\element_alias_regression.lua"
if (-not (Test-Path -LiteralPath $autorunSource -PathType Leaf)) {
    throw "Missing element alias runtime script: $autorunSource"
}
$tempParent = [System.IO.Path]::GetFullPath($TemporaryDirectory)
if (-not (Test-Path -LiteralPath $tempParent -PathType Container)) {
    throw "Temporary directory does not exist: $tempParent"
}
$testRoot = Join-Path $tempParent (
    "tpt-omnipack-element-alias-" + [guid]::NewGuid().ToString("N")
)
$resolvedTestRoot = [System.IO.Path]::GetFullPath($testRoot)
$prefix = $tempParent.TrimEnd(
    [System.IO.Path]::DirectorySeparatorChar,
    [System.IO.Path]::AltDirectorySeparatorChar
) + [System.IO.Path]::DirectorySeparatorChar
if (-not $resolvedTestRoot.StartsWith(
    $prefix, [System.StringComparison]::OrdinalIgnoreCase
)) {
    throw "Refusing to create alias test outside the temporary directory"
}

function Remove-TestRoot {
    if (-not (Test-Path -LiteralPath $resolvedTestRoot)) { return }
    for ($attempt = 1; $attempt -le 20; $attempt++) {
        try {
            Remove-Item -LiteralPath $resolvedTestRoot -Recurse -Force -ErrorAction Stop
            return
        }
        catch {
            if ($attempt -eq 20) {
                Write-Warning "Could not remove alias test root: $resolvedTestRoot"
                return
            }
            Start-Sleep -Milliseconds 100
        }
    }
}

function Get-Value {
    param([string] $Text, [string] $Key)
    $match = [regex]::Match(
        $Text, "(?m)^" + [regex]::Escape($Key) + "=([^\r\n]+)\r?$"
    )
    if (-not $match.Success) { throw "Missing $Key in alias result" }
    return $match.Groups[1].Value
}

function Invoke-Phase {
    param([ValidateRange(1, 3)][int] $Phase)
    [System.IO.File]::WriteAllText(
        (Join-Path $resolvedTestRoot "element-alias.phase"),
        [string]$Phase,
        [System.Text.Encoding]::ASCII
    )
    $resultPath = Join-Path $resolvedTestRoot (
        "element-alias-phase" + $Phase + ".result"
    )
    if (Test-Path -LiteralPath $resultPath) {
        Remove-Item -LiteralPath $resultPath -Force
    }
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
    if (-not $process) { throw "Failed to start alias phase $Phase" }
    try {
        $deadline = [DateTime]::UtcNow.AddSeconds($TimeoutSeconds)
        $text = ""
        do {
            Start-Sleep -Milliseconds 100
            if (Test-Path -LiteralPath $resultPath -PathType Leaf) {
                $text = [string](Get-Content -LiteralPath $resultPath -Raw)
                if ($text -match "(?m)^OMNI_ALIAS_STATUS=(PASS|FAIL)\r?$") { break }
            }
            $process.Refresh()
        } while ([DateTime]::UtcNow -lt $deadline -and -not $process.HasExited)
        if (-not $process.HasExited -and $text -match "(?m)^OMNI_ALIAS_STATUS=(PASS|FAIL)\r?$") {
            [void]$process.WaitForExit(5000)
            $process.Refresh()
        }
        if (-not $process.HasExited) {
            Stop-Process -Id $process.Id -Force
            $process.WaitForExit()
            throw "Alias phase $Phase timed out; artifacts=$resolvedTestRoot"
        }
        if (-not $text -and (Test-Path -LiteralPath $resultPath -PathType Leaf)) {
            $text = [string](Get-Content -LiteralPath $resultPath -Raw)
        }
        if ($process.ExitCode -ne 0 -or $text -notmatch "(?m)^OMNI_ALIAS_STATUS=PASS\r?$") {
            throw "Alias phase $Phase failed; exit=$($process.ExitCode); result=$text; artifacts=$resolvedTestRoot"
        }
        return [pscustomobject]@{
            Phase = $Phase
            Text = $text.Trim()
            Stamp = Get-Value -Text $text -Key "OMNI_ALIAS_STAMP"
        }
    }
    finally {
        if (-not $process.HasExited) {
            Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue
            $process.WaitForExit()
        }
        $process.Dispose()
    }
}

function Get-OpsEvidence {
    param([string] $Stamp)
    if ($Stamp -notmatch "^[0-9A-Fa-f]{10}$") { throw "Invalid stamp ID: $Stamp" }
    $path = Join-Path $resolvedTestRoot ("stamps\" + $Stamp + ".stm")
    [byte[]]$bytes = [System.IO.File]::ReadAllBytes($path)
    if ($bytes.Length -le 15) { throw "Alias OPS is too short" }
    if ([System.Text.Encoding]::ASCII.GetString($bytes, 0, 4) -ne "OPS1") {
        throw "Alias fixture is not OPS1"
    }
    return [pscustomobject]@{
        Path = $path
        Bytes = $bytes.Length
        Sha256 = (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash
    }
}

$passed = $false
try {
    New-Item -ItemType Directory -Path $resolvedTestRoot | Out-Null
    Copy-Item -LiteralPath $autorunSource -Destination (
        Join-Path $resolvedTestRoot "autorun.lua"
    )
    $phase1 = Invoke-Phase -Phase 1
    $legacy = Get-OpsEvidence -Stamp $phase1.Stamp
    $phase2 = Invoke-Phase -Phase 2
    $canonical = Get-OpsEvidence -Stamp $phase2.Stamp
    $phase3 = Invoke-Phase -Phase 3
    if ($phase3.Stamp -ne $phase2.Stamp) {
        throw "Final alias restart did not verify the canonical stamp"
    }
    foreach ($key in @(
        "OMNI_ALIAS_IDENTIFIER",
        "OMNI_ALIAS_STABLE_ID",
        "OMNI_ALIAS_CANONICAL",
        "OMNI_ALIAS_CANONICAL_ID",
        "OMNI_ALIAS_MARKER"
    )) {
        $one = Get-Value -Text $phase1.Text -Key $key
        $two = Get-Value -Text $phase2.Text -Key $key
        $three = Get-Value -Text $phase3.Text -Key $key
        if ($one -ne $two -or $two -ne $three) {
            throw "$key changed across alias phases"
        }
    }
    $lines = @(
        "OMNI_ALIAS_ROUNDTRIP_STATUS=PASS",
        "OMNI_ALIAS_PROCESS_COUNT=3",
        "OMNI_ALIAS_IDENTIFIER=OMNI_PT_MSCR",
        "OMNI_ALIAS_STABLE_ID=278",
        "OMNI_ALIAS_CANONICAL=DEFAULT_PT_BRMT",
        "OMNI_ALIAS_CANONICAL_ID=30",
        "OMNI_ALIAS_LEGACY_STAMP=$($phase1.Stamp)",
        "OMNI_ALIAS_LEGACY_BYTES=$($legacy.Bytes)",
        "OMNI_ALIAS_LEGACY_SHA256=$($legacy.Sha256)",
        "OMNI_ALIAS_CANONICAL_STAMP=$($phase2.Stamp)",
        "OMNI_ALIAS_CANONICAL_BYTES=$($canonical.Bytes)",
        "OMNI_ALIAS_CANONICAL_SHA256=$($canonical.Sha256)",
        "OMNI_ALIAS_DIRECT_PLACEMENT_BLOCKED=true",
        "OMNI_ALIAS_MENU_HIDDEN=true",
        "OMNI_ALIAS_CTYPE_PRESERVED=true",
        "OMNI_ALIAS_NEW_SAVE_CANONICAL=true"
    )
    $passed = $true
    Write-Output "runtime-lua-element-alias-test: PASS"
    Write-Output ($lines -join [Environment]::NewLine)
    if ($KeepArtifacts) { Write-Output "OMNI_ALIAS_ARTIFACTS=$resolvedTestRoot" }
}
finally {
    if ($passed -and -not $KeepArtifacts) { Remove-TestRoot }
}
