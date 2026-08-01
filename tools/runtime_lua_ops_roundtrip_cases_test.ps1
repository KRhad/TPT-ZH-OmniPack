param(
    [Parameter(Mandatory = $true)]
    [string] $Executable,

    [ValidateRange(1, 180)]
    [int] $TimeoutSeconds = 30,

    [string] $TemporaryDirectory = [System.IO.Path]::GetTempPath(),

    [switch] $KeepArtifacts
)

$ErrorActionPreference = "Stop"
$scriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$runner = Join-Path $scriptRoot "runtime_lua_ops_roundtrip_test.ps1"
if (-not (Test-Path -LiteralPath $runner -PathType Leaf)) {
    throw "Missing OPS roundtrip runner: $runner"
}

function Get-ResultValue {
    param(
        [Parameter(Mandatory = $true)]
        [string] $Text,

        [Parameter(Mandatory = $true)]
        [string] $Key
    )

    $match = [regex]::Match(
        $Text,
        "(?m)^" + [regex]::Escape($Key) + "=([^\r\n]+)\r?$"
    )
    if (-not $match.Success) {
        throw "Missing $Key in OPS scenario result"
    }
    return $match.Groups[1].Value
}

$scenarios = @(
    "official",
    "metallurgy",
    "biology",
    "chemistry",
    "nuclear",
    "periodic",
    "electronics"
)
$results = @()
foreach ($scenario in $scenarios) {
    $arguments = @{
        Executable = $Executable
        Scenario = $scenario
        TimeoutSeconds = $TimeoutSeconds
        TemporaryDirectory = $TemporaryDirectory
    }
    if ($KeepArtifacts) {
        $arguments.KeepArtifacts = $true
    }

    $scenarioOutput = @(& $runner @arguments)
    $scenarioText = $scenarioOutput -join [Environment]::NewLine
    if ($scenarioText -notmatch "(?m)^OMNI_OPS_ROUNDTRIP_STATUS=PASS\r?$") {
        throw "OPS scenario '$scenario' did not report PASS: $scenarioText"
    }
    $reportedScenario = Get-ResultValue `
        -Text $scenarioText `
        -Key "OMNI_OPS_SCENARIO"
    if ($reportedScenario -ne $scenario) {
        throw (
            "OPS scenario result mismatch: expected '$scenario', got " +
            "'$reportedScenario'"
        )
    }

    $results += [pscustomobject]@{
        Scenario = $scenario
        Output = $scenarioOutput
        Text = $scenarioText
        Particles = [int](Get-ResultValue `
            -Text $scenarioText `
            -Key "OMNI_OPS_PARTICLES")
        Assertions = [int](Get-ResultValue `
            -Text $scenarioText `
            -Key "OMNI_OPS_FIELD_ASSERTIONS_PER_LOAD")
        StableIdentifiers = [int](Get-ResultValue `
            -Text $scenarioText `
            -Key "OMNI_OPS_STABLE_IDENTIFIER_COUNT")
        PaletteIdentifiers = [int](Get-ResultValue `
            -Text $scenarioText `
            -Key "OMNI_OPS_PALETTE_IDENTIFIERS")
    }
}

foreach ($result in $results) {
    Write-Output ($result.Output -join [Environment]::NewLine)
}

$totalParticles = ($results | Measure-Object -Property Particles -Sum).Sum
$totalAssertions = ($results | Measure-Object -Property Assertions -Sum).Sum
$totalStableIdentifiers = (
    $results | Measure-Object -Property StableIdentifiers -Sum
).Sum
$totalPaletteIdentifiers = (
    $results | Measure-Object -Property PaletteIdentifiers -Sum
).Sum

Write-Output "runtime-lua-ops-roundtrip-cases-test: PASS"
Write-Output "OMNI_OPS_CASES_STATUS=PASS"
Write-Output "OMNI_OPS_CASE_COUNT=$($scenarios.Count)"
Write-Output "OMNI_OPS_CASES=$($scenarios -join ',')"
Write-Output "OMNI_OPS_TOTAL_PROCESS_COUNT=$($scenarios.Count * 3)"
Write-Output "OMNI_OPS_TOTAL_RESTART_COUNT=$($scenarios.Count * 2)"
Write-Output "OMNI_OPS_TOTAL_LOAD_VERIFICATIONS=$($scenarios.Count * 2)"
Write-Output "OMNI_OPS_TOTAL_PARTICLES=$totalParticles"
Write-Output "OMNI_OPS_TOTAL_FIELD_ASSERTIONS_PER_LOAD=$totalAssertions"
Write-Output "OMNI_OPS_TOTAL_STABLE_IDENTIFIERS=$totalStableIdentifiers"
Write-Output "OMNI_OPS_TOTAL_PALETTE_IDENTIFIERS=$totalPaletteIdentifiers"
