param(
    [Parameter(Mandatory = $true)]
    [string] $Executable,

    [ValidateSet("Generate", "Verify", "All")]
    [string] $Mode = "Verify",

    [string] $ExamplesDirectory = (Join-Path (Split-Path -Parent $MyInvocation.MyCommand.Path) "..\examples\0.3.0"),

    [string] $TemporaryDirectory = [System.IO.Path]::GetTempPath(),

    [ValidateRange(1, 180)]
    [int] $TimeoutSeconds = 60,

    [switch] $DevelopmentProbe,

    [switch] $UpdateSourceArtifacts,

    [switch] $KeepArtifacts
)

$ErrorActionPreference = "Stop"
$resolvedExecutable = (Resolve-Path -LiteralPath $Executable).Path
$scriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$sourceRoot = (Resolve-Path -LiteralPath (Join-Path $scriptRoot "..")).Path
$autorunSource = Join-Path $scriptRoot "runtime\automation_regression.lua"
$specPath = Join-Path $sourceRoot "automation\0.3.0\scenario-spec.json"
$git = "E:\Git\cmd\git.exe"
foreach ($required in @($autorunSource, $specPath, $git)) {
    if (-not (Test-Path -LiteralPath $required -PathType Leaf)) {
        throw "Missing automation runtime input: $required"
    }
}

$resolvedExamples = [System.IO.Path]::GetFullPath($ExamplesDirectory)
$sourceExamples = [System.IO.Path]::GetFullPath((Join-Path $sourceRoot "examples\0.3.0"))
$resolvedTempParent = [System.IO.Path]::GetFullPath($TemporaryDirectory)
if (-not (Test-Path -LiteralPath $resolvedTempParent -PathType Container)) {
    throw "Temporary directory does not exist: $resolvedTempParent"
}
if (
    $Mode -in @("Generate", "All") -and
    $resolvedExamples.Equals($sourceExamples, [System.StringComparison]::OrdinalIgnoreCase) -and
    -not $UpdateSourceArtifacts
) {
    throw (
        "Refusing to replace repository automation evidence. " +
        "Use -ExamplesDirectory with an isolated directory, or pass " +
        "-UpdateSourceArtifacts for an intentional evidence refresh."
    )
}
New-Item -ItemType Directory -Path $resolvedExamples -Force | Out-Null
$spec = Get-Content -LiteralPath $specPath -Raw | ConvertFrom-Json
$scenarios = @($spec.scenarios)
$challenges = @($spec.challenges)
if ($scenarios.Count -ne 9) { throw "Expected exactly 9 automation scenarios, found $($scenarios.Count)" }
if ($challenges.Count -ne 6) { throw "Expected exactly 6 automation challenges, found $($challenges.Count)" }

function Read-KeyValueFile {
    param([Parameter(Mandatory = $true)][string] $Path)
    $values = @{}
    foreach ($line in Get-Content -LiteralPath $Path) {
        if ($line -match '^([A-Za-z0-9_]+)=(.*)$') {
            $values[$matches[1]] = $matches[2]
        }
    }
    return $values
}

function New-IsolatedRoot {
    param([Parameter(Mandatory = $true)][string] $Label)
    $root = [System.IO.Path]::GetFullPath((Join-Path $resolvedTempParent (
        "tpt-omnipack-automation-$Label-" + [guid]::NewGuid().ToString("N"))))
    $prefix = $resolvedTempParent.TrimEnd(
        [System.IO.Path]::DirectorySeparatorChar,
        [System.IO.Path]::AltDirectorySeparatorChar) + [System.IO.Path]::DirectorySeparatorChar
    if (-not $root.StartsWith($prefix, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to create automation directory outside temporary root"
    }
    New-Item -ItemType Directory -Path $root | Out-Null
    return $root
}

function Remove-IsolatedRoot {
    param([Parameter(Mandatory = $true)][string] $Root)
    if (-not (Test-Path -LiteralPath $Root)) { return }
    for ($attempt = 1; $attempt -le 20; $attempt++) {
        try {
            Remove-Item -LiteralPath $Root -Recurse -Force -ErrorAction Stop
            return
        }
        catch {
            if ($attempt -eq 20) {
                Write-Warning "Could not remove automation temporary directory after 20 attempts: $Root ($($_.Exception.Message))"
                return
            }
            Start-Sleep -Milliseconds 100
        }
    }
}

function Get-GitHead {
    $head = (& $git -C $sourceRoot rev-parse HEAD).Trim()
    if ($LASTEXITCODE -ne 0 -or $head -notmatch '^[0-9a-f]{40}$') {
        throw "Cannot resolve source commit"
    }
    return $head
}

function Get-SourceTreeState {
    $status = @(& $git -C $sourceRoot status --porcelain)
    if ($LASTEXITCODE -ne 0) { throw "Cannot inspect source tree state" }
    if ($status.Count -eq 0) { return "clean" }
    if (-not $DevelopmentProbe) {
        throw "Automation evidence generation requires a clean source tree; use -DevelopmentProbe only for non-gate debugging"
    }
    return "dirty_probe"
}

function Invoke-AutomationClient {
    param(
        [Parameter(Mandatory = $true)][string] $CaseId,
        [Parameter(Mandatory = $true)][ValidateSet("generate", "verify")][string] $ClientMode,
        [string] $StampId,
        [string] $ChallengeId,
        [string] $StampPath,
        [switch] $PreserveRoot
    )
    $label = ($CaseId -replace '[^A-Za-z0-9_-]', '_')
    $root = New-IsolatedRoot -Label $label
    $process = $null
    try {
        Copy-Item -LiteralPath $autorunSource -Destination (Join-Path $root "autorun.lua")
        [System.IO.File]::WriteAllText((Join-Path $root "automation.mode"), $ClientMode + "`n", [System.Text.Encoding]::ASCII)
        [System.IO.File]::WriteAllText((Join-Path $root "automation.case"), $CaseId + "`n", [System.Text.Encoding]::ASCII)
        if ($ChallengeId) {
            [System.IO.File]::WriteAllText((Join-Path $root "automation.challenge"), $ChallengeId + "`n", [System.Text.Encoding]::ASCII)
        }
        if ($StampId) {
            [System.IO.File]::WriteAllText((Join-Path $root "automation.stamp"), $StampId + "`n", [System.Text.Encoding]::ASCII)
        }
        if ($StampPath) {
            if (-not $StampId) { throw "StampPath requires StampId" }
            $stampDirectory = Join-Path $root "stamps"
            New-Item -ItemType Directory -Path $stampDirectory -Force | Out-Null
            Copy-Item -LiteralPath $StampPath -Destination (Join-Path $stampDirectory ($StampId + ".stm")) -Force
        }

        $startInfo = [System.Diagnostics.ProcessStartInfo]::new()
        $startInfo.FileName = $resolvedExecutable
        $startInfo.WorkingDirectory = $root
        $startInfo.UseShellExecute = $false
        $startInfo.CreateNoWindow = $true
        $startInfo.ArgumentList.Add("ddir")
        $startInfo.ArgumentList.Add($root)
        foreach ($secretName in @("GITHUB_PAT_TOKEN", "GITHUB_TOKEN", "GH_TOKEN")) {
            [void]$startInfo.Environment.Remove($secretName)
        }
        $process = [System.Diagnostics.Process]::Start($startInfo)
        if (-not $process) { throw "Failed to start automation client" }

        $resultPath = Join-Path $root "automation.result"
        $deadline = [DateTime]::UtcNow.AddSeconds($TimeoutSeconds)
        while (-not $process.HasExited -and [DateTime]::UtcNow -lt $deadline) {
            Start-Sleep -Milliseconds 100
            if (Test-Path -LiteralPath $resultPath -PathType Leaf) {
                $text = [string](Get-Content -LiteralPath $resultPath -Raw)
                if ($text -match '(?m)^OMNI_AUTOMATION_STATUS=(PASS|FAIL)\r?$') { break }
            }
            $process.Refresh()
        }
        if (-not (Test-Path -LiteralPath $resultPath -PathType Leaf)) {
            if (-not $process.HasExited) {
                Stop-Process -Id $process.Id -Force
                $process.WaitForExit()
            }
            throw "Automation client timed out without result; case=$CaseId; artifacts=$root"
        }
        $resultText = [string](Get-Content -LiteralPath $resultPath -Raw)
        $terminatedAfterPass = $false
        if (-not $process.HasExited) {
            $terminatedAfterPass = $resultText -match '(?m)^OMNI_AUTOMATION_STATUS=PASS\r?$'
            Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue
            $process.WaitForExit()
        }
        $values = Read-KeyValueFile -Path $resultPath
        if (($process.ExitCode -ne 0 -and -not $terminatedAfterPass) -or $values.OMNI_AUTOMATION_STATUS -ne "PASS") {
            throw "Automation client failed; case=$CaseId; challenge=$ChallengeId; result=$resultText; artifacts=$root"
        }
        return [pscustomobject]@{ Root = $root; Values = $values; Text = $resultText.Trim() }
    }
    catch {
        Write-Output "automation_failure_artifacts=$root"
        throw
    }
    finally {
        if ($process) {
            if (-not $process.HasExited) {
                Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue
                $process.WaitForExit()
            }
            $process.Dispose()
        }
        if (-not $KeepArtifacts -and -not $PreserveRoot -and (Test-Path -LiteralPath $root)) {
            Remove-IsolatedRoot -Root $root
        }
    }
}

function Generate-AutomationExamples {
    $sourceTreeState = Get-SourceTreeState
    $generated = @()
    $ordinal = 1
    foreach ($scenario in $scenarios) {
        $result = Invoke-AutomationClient -CaseId $scenario.id -ClientMode "generate" -PreserveRoot
        $sourceStamp = $result.Values.OMNI_AUTOMATION_STAMP
        if ($sourceStamp -notmatch '^[0-9A-Fa-f]{10}$') {
            throw "Invalid generated automation stamp for $($scenario.id): $sourceStamp"
        }
        $sourcePath = Join-Path $result.Root ("stamps\" + $sourceStamp + ".stm")
        if (-not (Test-Path -LiteralPath $sourcePath -PathType Leaf)) {
            throw "Generated automation stamp file is missing: $sourcePath"
        }
        $filename = ("{0:D2}-{1}.stm" -f $ordinal, $scenario.id)
        $destination = Join-Path $resolvedExamples $filename
        Copy-Item -LiteralPath $sourcePath -Destination $destination -Force
        if (-not $KeepArtifacts) { Remove-IsolatedRoot -Root $result.Root }
        $stableStamp = ("03{0:X8}" -f $ordinal).ToLowerInvariant()
        $generated += [ordered]@{
            id = $scenario.id
            filename = $filename
            stamp_id = $stableStamp
            source_stamp_id = $sourceStamp
            sha256 = (Get-FileHash -LiteralPath $destination -Algorithm SHA256).Hash
            bytes = (Get-Item -LiteralPath $destination).Length
            initial_particles = [int]$result.Values.OMNI_AUTOMATION_INITIAL_PARTICLES
            generation_order = $ordinal
            title_zh = $scenario.title_zh
        }
        Write-Output "automation-generated=$($scenario.id) stamp=$sourceStamp sha256=$($generated[-1].sha256)"
        $ordinal++
    }
    $manifest = [ordered]@{
        schema_version = 1
        content_version = $spec.content_version
        source_commit = Get-GitHead
        source_tree_state = $sourceTreeState
        generator_exe_sha256 = (Get-FileHash -LiteralPath $resolvedExecutable -Algorithm SHA256).Hash
        scenario_spec_sha256 = (Get-FileHash -LiteralPath $specPath -Algorithm SHA256).Hash
        runtime_script_sha256 = (Get-FileHash -LiteralPath $autorunSource -Algorithm SHA256).Hash
        generated_utc = [DateTime]::UtcNow.ToString("o")
        scenarios = $generated
        challenge_ids = @($challenges | ForEach-Object { $_.id })
    }
    $manifestPath = Join-Path $resolvedExamples "manifest.json"
    [System.IO.File]::WriteAllText(
        $manifestPath,
        ($manifest | ConvertTo-Json -Depth 8) + [Environment]::NewLine,
        [System.Text.UTF8Encoding]::new($false))
    Write-Output "automation-manifest=$manifestPath"
    return $manifest
}

function Verify-AutomationExamples {
    $manifestPath = Join-Path $resolvedExamples "manifest.json"
    if (-not (Test-Path -LiteralPath $manifestPath -PathType Leaf)) {
        throw "Missing automation manifest: $manifestPath"
    }
    $manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
    $entries = @($manifest.scenarios)
    if ($entries.Count -ne 9) { throw "Automation manifest must contain 9 scenarios" }
    if (@($manifest.challenge_ids).Count -ne 6) { throw "Automation manifest must contain 6 challenge IDs" }
    if ($manifest.source_tree_state -ne "clean" -and -not $DevelopmentProbe) {
        throw "Dirty-probe automation artifacts cannot be used as gate evidence"
    }
    if ([string]$manifest.source_commit -notmatch '^[0-9a-f]{40}$') {
        throw "Automation manifest generation source commit is invalid"
    }
    foreach ($field in @("generator_exe_sha256", "scenario_spec_sha256", "runtime_script_sha256")) {
        if ([string]$manifest.$field -notmatch '^[0-9A-F]{64}$') {
            throw "Automation manifest $field is invalid"
        }
    }
    # The checked-in stamps are compatibility fixtures.  Their historical
    # generator identity must remain intact while the current executable,
    # current scenario contract, and current runtime script verify them.
    $verificationSourceTreeState = Get-SourceTreeState

    $results = @()
    foreach ($scenario in $scenarios) {
        $entry = $entries | Where-Object { $_.id -eq $scenario.id }
        if (-not $entry) { throw "Manifest is missing automation scenario $($scenario.id)" }
        $stampPath = Join-Path $resolvedExamples $entry.filename
        if (-not (Test-Path -LiteralPath $stampPath -PathType Leaf)) { throw "Missing automation stamp: $stampPath" }
        if ((Get-FileHash -LiteralPath $stampPath -Algorithm SHA256).Hash -ne $entry.sha256) {
            throw "Automation stamp hash mismatch: $($scenario.id)"
        }
        $challenge = @($challenges | Where-Object { $_.scenario -eq $scenario.id })
        if ($challenge.Count -gt 1) { throw "Scenario $($scenario.id) has more than one challenge" }
        $challengeId = if ($challenge.Count -eq 1) { [string]$challenge[0].id } else { "" }
        $client = Invoke-AutomationClient -CaseId $scenario.id -ClientMode "verify" -StampId $entry.stamp_id -StampPath $stampPath -ChallengeId $challengeId
        $values = $client.Values
        foreach ($field in @(
            "OMNI_AUTOMATION_ASSERTIONS", "OMNI_AUTOMATION_FRAMES", "OMNI_AUTOMATION_INPUT_EVENTS",
            "OMNI_AUTOMATION_OUTPUT_EVENTS", "OMNI_AUTOMATION_PEAK_EVENTS_PER_FRAME",
            "OMNI_AUTOMATION_BLOCKED_EVENTS", "OMNI_AUTOMATION_STOP_EVENT_DELTA",
            "OMNI_AUTOMATION_RECOVERY_ASSERTIONS", "OMNI_AUTOMATION_OMNI_EVENTS")) {
            if ($values[$field] -notmatch '^\d+$') { throw "$($scenario.id) has invalid metric $field=$($values[$field])" }
        }
        if ([int64]$values.OMNI_AUTOMATION_STOP_EVENT_DELTA -ne 0) {
            throw "$($scenario.id) produced post-stop events"
        }
        if ([int64]$values.OMNI_AUTOMATION_PEAK_EVENTS_PER_FRAME -gt [int64]$spec.bounds.max_signal_events_per_frame) {
            throw "$($scenario.id) exceeded the signal event budget"
        }
        if ([int64]$values.OMNI_AUTOMATION_FRAMES -gt [int64]$spec.bounds.max_frames_per_challenge) {
            throw "$($scenario.id) exceeded the frame contract"
        }
        $results += [pscustomobject][ordered]@{
            id = $scenario.id
            challenge_id = $challengeId
            status = $values.OMNI_AUTOMATION_STATUS
            assertions = [int64]$values.OMNI_AUTOMATION_ASSERTIONS
            frames = [int64]$values.OMNI_AUTOMATION_FRAMES
            input_events = [int64]$values.OMNI_AUTOMATION_INPUT_EVENTS
            output_events = [int64]$values.OMNI_AUTOMATION_OUTPUT_EVENTS
            peak_events_per_frame = [int64]$values.OMNI_AUTOMATION_PEAK_EVENTS_PER_FRAME
            blocked_events = [int64]$values.OMNI_AUTOMATION_BLOCKED_EVENTS
            stop_event_delta = [int64]$values.OMNI_AUTOMATION_STOP_EVENT_DELTA
            recovery_assertions = [int64]$values.OMNI_AUTOMATION_RECOVERY_ASSERTIONS
            omni_events = [int64]$values.OMNI_AUTOMATION_OMNI_EVENTS
            stamp_id = $entry.stamp_id
            stamp_sha256 = $entry.sha256
        }
        Write-Output "automation-pass=$($scenario.id) challenge=$challengeId assertions=$($values.OMNI_AUTOMATION_ASSERTIONS) frames=$($values.OMNI_AUTOMATION_FRAMES)"
    }
    $report = [ordered]@{
        schema_version = 1
        content_version = $spec.content_version
        source_commit = Get-GitHead
        source_tree_state = $manifest.source_tree_state
        verification_source_tree_state = $verificationSourceTreeState
        executable_sha256 = (Get-FileHash -LiteralPath $resolvedExecutable -Algorithm SHA256).Hash
        manifest_sha256 = (Get-FileHash -LiteralPath $manifestPath -Algorithm SHA256).Hash
        generation_source_commit = [string]$manifest.source_commit
        generator_exe_sha256 = [string]$manifest.generator_exe_sha256
        scenario_spec_sha256 = (Get-FileHash -LiteralPath $specPath -Algorithm SHA256).Hash
        runtime_script_sha256 = (Get-FileHash -LiteralPath $autorunSource -Algorithm SHA256).Hash
        scenario_count = $results.Count
        scenario_pass_count = @($results | Where-Object { $_.status -eq "PASS" }).Count
        challenge_count = @($results | Where-Object { $_.challenge_id }).Count
        challenge_pass_count = @($results | Where-Object { $_.challenge_id -and $_.status -eq "PASS" }).Count
        assertion_total = [int64](($results | Measure-Object assertions -Sum).Sum)
        frame_total = [int64](($results | Measure-Object frames -Sum).Sum)
        input_event_total = [int64](($results | Measure-Object input_events -Sum).Sum)
        output_event_total = [int64](($results | Measure-Object output_events -Sum).Sum)
        blocked_event_total = [int64](($results | Measure-Object blocked_events -Sum).Sum)
        peak_events_per_frame = [int64](($results | Measure-Object peak_events_per_frame -Maximum).Maximum)
        stop_event_delta_total = [int64](($results | Measure-Object stop_event_delta -Sum).Sum)
        recovery_assertion_total = [int64](($results | Measure-Object recovery_assertions -Sum).Sum)
        omni_event_total = [int64](($results | Measure-Object omni_events -Sum).Sum)
        results = $results
    }
    if ($report.scenario_pass_count -ne 9 -or $report.challenge_pass_count -ne 6) {
        throw "Automation runtime passed only $($report.scenario_pass_count)/9 scenarios and $($report.challenge_pass_count)/6 challenges"
    }
    $reportPath = Join-Path $resolvedExamples "runtime-report.json"
    [System.IO.File]::WriteAllText(
        $reportPath,
        ($report | ConvertTo-Json -Depth 8) + [Environment]::NewLine,
        [System.Text.UTF8Encoding]::new($false))
    Write-Output "automation-runtime-report=$reportPath"
    return $report
}

if ($Mode -in @("Generate", "All")) {
    [void](Generate-AutomationExamples)
}
if ($Mode -in @("Verify", "All")) {
    $report = Verify-AutomationExamples
    Write-Output "runtime-lua-automation-test: PASS"
    Write-Output "OMNI_AUTOMATION_STATUS=PASS"
    Write-Output "OMNI_AUTOMATION_SCENARIOS=9"
    Write-Output "OMNI_AUTOMATION_SCENARIO_PASS=$($report.scenario_pass_count)"
    Write-Output "OMNI_AUTOMATION_CHALLENGES=6"
    Write-Output "OMNI_AUTOMATION_CHALLENGE_PASS=$($report.challenge_pass_count)"
    Write-Output "OMNI_AUTOMATION_ASSERTIONS=$($report.assertion_total)"
    Write-Output "OMNI_AUTOMATION_STOP_EVENT_DELTA=$($report.stop_event_delta_total)"
}
