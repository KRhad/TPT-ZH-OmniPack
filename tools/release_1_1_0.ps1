param(
    [ValidateSet("rc", "stable")]
    [string] $Channel = "rc",
    [string] $BuildDirectory = "build-release-1.1.0",
    [string] $OutputDirectory = "dist\1.1.0",
    [string] $OfficialSaveCorpus = "",
    [switch] $SkipLongSoak,
    [switch] $AllowDirtyValidation,
    [switch] $CandidateOnly
)

$ErrorActionPreference = "Stop"
$ProgressPreference = "SilentlyContinue"
$sourceRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot "..")).Path
$git = (Get-Command git.exe -CommandType Application -ErrorAction Stop | Select-Object -First 1).Source
$msysBin = "C:\msys64\ucrt64\bin"
$msysUsrBin = "C:\msys64\usr\bin"
$gitBin = (Get-Item -LiteralPath $git).Directory.FullName
$env:PATH = "$gitBin;$msysBin;$msysUsrBin;" + $env:PATH
$meson = Join-Path $msysBin "meson.exe"
$ninja = Join-Path $msysBin "ninja.exe"
$python = Join-Path $msysBin "python3.exe"
$objcopy = Join-Path $msysBin "objcopy.exe"
$strip = Join-Path $msysBin "strip.exe"
$objdump = Join-Path $msysBin "objdump.exe"
$strings = Join-Path $msysBin "strings.exe"
foreach ($tool in @($meson,$ninja,$python,$objcopy,$strip,$objdump,$strings)) {
    if (-not (Test-Path -LiteralPath $tool -PathType Leaf)) { throw "Required release tool is missing: $tool" }
}
if ($Channel -eq "stable" -and $SkipLongSoak) { throw "Stable release rejects -SkipLongSoak" }
if ($Channel -eq "stable" -and $AllowDirtyValidation) { throw "Stable release rejects -AllowDirtyValidation" }
if ($Channel -ne "stable" -and $CandidateOnly) { throw "-CandidateOnly is valid only for the stable staging flow" }
$version = if ($Channel -eq "stable") { "1.1.0" } else { "1.1.0-rc1" }
$kind = if ($Channel -eq "stable") { "release" } else { "release-candidate" }
$runId = [DateTime]::UtcNow.ToString("yyyyMMddTHHmmssZ") + "-" + [guid]::NewGuid().ToString("N").Substring(0, 8)
$buildDirectory = if ([IO.Path]::IsPathRooted($BuildDirectory)) { $BuildDirectory } else { Join-Path $sourceRoot $BuildDirectory }
$outputDirectory = if ([IO.Path]::IsPathRooted($OutputDirectory)) { $OutputDirectory } else { Join-Path $sourceRoot $OutputDirectory }
$releaseBuildInputWrap = "subprojects/tpt-libs-prebuilt-x86_64-windows-mingw-static-release-v20251019131007.wrap"
$validationRoot = Join-Path $outputDirectory "release-validation"
$validationDirectory = Join-Path $validationRoot $runId
$releaseDirectory = Join-Path $outputDirectory "release"
$symbolsDirectory = Join-Path $outputDirectory "symbols"
$artifactStem = if ($Channel -eq "stable") { "TPT-ZH-OmniPack-1.1.0-staging-$runId-Windows-x64-SDL3" } else { "TPT-ZH-OmniPack-$version-Windows-x64-SDL3" }
$symbolArtifactStem = if ($Channel -eq "stable") { "TPT-ZH-OmniPack-1.1.0-staging-$runId-Windows-x64-Symbols" } else { "TPT-ZH-OmniPack-$version-Windows-x64-Symbols" }
$releaseZip = Join-Path $outputDirectory "$artifactStem.zip"
$symbolsZip = Join-Path $outputDirectory "$symbolArtifactStem.zip"
$promotedReleaseZip = Join-Path $outputDirectory "TPT-ZH-OmniPack-1.1.0-Windows-x64-SDL3.zip"
$promotedSymbolsZip = Join-Path $outputDirectory "TPT-ZH-OmniPack-1.1.0-Windows-x64-Symbols.zip"
$packageOk = $false
$currentCandidateSha256 = $null
$currentSymbolsSha256 = $null
$currentSymbolsMemberSha256 = $null
if ($Channel -eq "stable" -and (Test-Path -LiteralPath $buildDirectory)) {
    throw "stable release requires an absent, run-clean build directory: $buildDirectory"
}
New-Item -ItemType Directory -Force $outputDirectory,$validationRoot,$releaseDirectory,$symbolsDirectory | Out-Null
if (Test-Path -LiteralPath $validationDirectory) { throw "release run directory already exists: $validationDirectory" }
if ($Channel -eq "stable") {
    foreach ($stableArtifact in @($promotedReleaseZip,"$promotedReleaseZip.sha256",$promotedSymbolsZip,"$promotedSymbolsZip.sha256")) {
        if (Test-Path -LiteralPath $stableArtifact) { throw "stable output already exists before final promotion: $stableArtifact" }
    }
}
New-Item -ItemType Directory -Path $validationDirectory | Out-Null

$results = [ordered]@{}
$gateTestNames = @{
    SourceTreeClean="source_tree_clean"; SourceSnapshotImmutability="source_snapshot_immutability"
    Configure="configure"; Build="build"; UnitTests="unit_tests"; AtmosphereBench="atmosphere_bench"
    MassConservation="mass_conservation"; OmniSaveRoundtrip="omni_save_roundtrip"
    OfficialTPTCorpusProvenance="official_tpt_provenance"; OfficialTPTSaveCompatibility="official_tpt_save_compatibility"
    GPUNumericalValidation="gpu_validation"; SDL3Runtime="sdl_gpu_runtime"; CPUFallbackValidation="cpu_fallback"; SDL3GUI="sdl3_gui"
    DebugSymbolsSeparated="debug_symbols_separated"; ReleaseBinaryStripped="release_binary_stripped"
    PackageManifest="package_manifest"; PackageVerification="package_verification"; SymbolPackageVerification="symbol_package_verification"
    WindowsPortableExtraction="windows_portable_extraction"; WindowsCleanMachine="windows_clean_machine"; Soak2Hours="soak_2h"
    CandidateSHA256="candidate_sha256"; ArtifactImmutability="artifact_immutability"
    EvidenceSemanticIntegrity="release_validation_audit"; EvidenceHashIntegrity="release_validation_audit"
    DocumentationConsistency="release_validation_audit"; NegativeGateSuite="negative_gate_suite"; CandidatePromotion="candidate_promotion"
}
function Get-RelativeEvidencePath {
    param([string]$Path)
    if (-not $Path) { return $Path }
    try {
        $full = [IO.Path]::GetFullPath($Path)
        $root = [IO.Path]::GetFullPath($validationDirectory).TrimEnd('\') + '\'
        if ($full.StartsWith($root, [StringComparison]::OrdinalIgnoreCase)) {
            return $full.Substring($root.Length).Replace('\','/')
        }
    } catch { }
    return $Path
}
function Get-Sha256Hex {
    param([Parameter(Mandatory = $true)][string] $Path)
    $sha = [System.Security.Cryptography.SHA256]::Create()
    try {
        $stream = [IO.File]::OpenRead($Path)
        try {
            return ([BitConverter]::ToString($sha.ComputeHash($stream))).Replace('-', '').ToUpperInvariant()
        } finally { $stream.Dispose() }
    } finally { $sha.Dispose() }
}
function Get-SourceSnapshot {
    param([Parameter(Mandatory=$true)][string]$Phase)
    $snapshotPath = Join-Path $validationDirectory ("source-snapshot-" + $Phase + ".json")
    $stdoutPath = "$snapshotPath.stdout.txt"
    $stderrPath = "$snapshotPath.stderr.txt"
    foreach ($target in @($snapshotPath,$stdoutPath,$stderrPath)) {
        if (Test-Path -LiteralPath $target -PathType Leaf) { Remove-Item -LiteralPath $target -Force }
    }
    $exit = Invoke-ProcessCapture $python @(
        (Join-Path $sourceRoot "tools\source_snapshot.py"),
        "--source-root",$sourceRoot,
        "--required-wrap",$releaseBuildInputWrap,
        "--output",$snapshotPath
    ) $stdoutPath $stderrPath 120
    if ($exit -ne 0 -or -not (Test-Path -LiteralPath $snapshotPath -PathType Leaf)) {
        $detail = if (Test-Path -LiteralPath $stderrPath) { (Get-Content -LiteralPath $stderrPath -Raw).Trim() } else { "exit=$exit" }
        throw "source snapshot phase '$Phase' failed: $detail"
    }
    $value = Get-Content -LiteralPath $snapshotPath -Raw | ConvertFrom-Json
    if ($value.schema -ne "omnipack-source-snapshot" -or $value.schema_version -ne 1 -or
        $value.snapshot_format -ne 4 -or
        $value.source_worktree_sha256 -notmatch '^[0-9A-F]{64}$' -or
        $value.build_inputs_sha256 -notmatch '^[0-9A-F]{64}$' -or
        $value.build_inputs_total -ne 1 -or
        $value.tracked_files -le 0) {
        throw "source snapshot phase '$Phase' returned invalid evidence"
    }
    return $value
}
function Test-BmpScreenshot {
    param([string]$Path,[int]$ExpectedWidth=0,[int]$ExpectedHeight=0,[long]$ExpectedBytes=0)
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) { return $false }
    $bytes = [IO.File]::ReadAllBytes($Path)
    if ($bytes.Length -lt 54 -or $bytes[0] -ne 0x42 -or $bytes[1] -ne 0x4D) { return $false }
    $width = [BitConverter]::ToInt32($bytes,18)
    $height = [Math]::Abs([BitConverter]::ToInt32($bytes,22))
    if ($width -le 0 -or $height -le 0) { return $false }
    if ($ExpectedWidth -gt 0 -and $width -ne $ExpectedWidth) { return $false }
    if ($ExpectedHeight -gt 0 -and $height -ne $ExpectedHeight) { return $false }
    if ($ExpectedBytes -gt 0 -and $bytes.Length -ne $ExpectedBytes) { return $false }
    return $true
}
function Test-PortableRuntimeEvidence {
    param([string]$Path,[string]$CandidateSha256,[string]$ExpectedRunId)
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) { return $false }
    try { $outer = Get-Content -LiteralPath $Path -Raw | ConvertFrom-Json } catch { return $false }
    $required = @(
        "launch_passed","fixture_created","initial_simulate_passed",
        "initial_save_passed","initial_parse_passed","initial_reload_passed",
        "initial_state_validate_passed","post_step_simulate_passed",
        "post_step_finite_passed","post_step_inventory_passed",
        "post_step_atmosphere_passed","post_step_save_passed",
        "post_step_parse_passed","post_step_reload_passed",
        "post_step_roundtrip_passed","save_reload_passed",
        "state_validate_passed","enhanced_mode","omni_state_present",
        "water_sidecar_roundtrip","runtime_completion_reached"
    )
    if ($outer.schema -ne "omnipack-release-evidence" -or $outer.schema_version -ne 1 -or
        $outer.test -ne "windows_portable_extraction" -or $outer.run_id -ne $ExpectedRunId -or
        $outer.candidate_sha256 -ne $CandidateSha256 -or $outer.status -ne "PASS" -or
        $outer.passed -ne $true -or $outer.runtime_payload_schema_version -ne 2 -or
        $outer.candidate_source_markers_checked -ne $true -or
        @($outer.candidate_source_tree_indicators).Count -ne 0 -or
        $outer.runtime_target_executable_count -ne 1 -or $outer.clean_shutdown -ne $true -or
        @($required | Where-Object { $outer.$_ -ne $true }).Count -gt 0 -or
        $outer.initial_particle_count -lt 1 -or
        $outer.post_step_particle_count -ne $outer.initial_particle_count -or
        $outer.final_particle_count -ne $outer.post_step_particle_count -or
        $outer.atmosphere_non_finite_cells -ne 0) { return $false }
    foreach ($field in @("runtime_stdout","runtime_stderr","runtime_inner_evidence")) {
        $artifact = Join-Path $validationDirectory ([string]$outer.$field)
        $hashField = $field + "_sha256"
        if (-not (Test-Path -LiteralPath $artifact -PathType Leaf) -or
            (Get-Sha256Hex $artifact) -ne $outer.$hashField) { return $false }
    }
    $innerPath = Join-Path $validationDirectory ([string]$outer.runtime_inner_evidence)
    try { $inner = Get-Content -LiteralPath $innerPath -Raw | ConvertFrom-Json } catch { return $false }
    if ($inner.schema -ne "omnipack-release-evidence" -or $inner.schema_version -ne 1 -or
        $inner.payload_schema_version -ne 2 -or $inner.test -ne "portable_runtime" -or
        $inner.run_id -ne $ExpectedRunId -or $inner.candidate_sha256 -ne $CandidateSha256 -or
        $inner.status -ne "PASS" -or $inner.passed -ne $true -or
        @($required | Where-Object { $inner.$_ -ne $true -or $outer.$_ -ne $inner.$_ }).Count -gt 0 -or
        $inner.initial_particle_count -ne $outer.initial_particle_count -or
        $inner.post_step_particle_count -ne $outer.post_step_particle_count -or
        $inner.final_particle_count -ne $outer.final_particle_count -or
        $inner.atmosphere_non_finite_cells -ne 0) { return $false }
    return $true
}
function Read-KeyValueEvidence {
    param([string]$Path)
    $values = @{}
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) { return $values }
    foreach ($line in Get-Content -LiteralPath $Path) {
        if ($line -match '^([A-Za-z0-9_]+)=(.*)$') { $values[$matches[1]] = $matches[2] }
    }
    return $values
}
function Find-EvidenceFile {
    param([string]$Path)
    foreach ($candidate in @($Path, "$Path.json", "$Path.stdout.txt")) {
        if (Test-Path -LiteralPath $candidate -PathType Leaf) { return (Resolve-Path -LiteralPath $candidate).Path }
    }
    return $null
}
function Set-Gate {
    param([string]$Name,[string]$Status,[string]$Command,[int]$ExitCode,[string]$Evidence,[string]$Message,[string]$StartedAt,[string]$FinishedAt)
    if (-not $gateTestNames.ContainsKey($Name)) { throw "No evidence schema is registered for gate: $Name" }
    $sourceEvidenceFile = Find-EvidenceFile $Evidence
    if (-not $sourceEvidenceFile) {
        # Missing evidence can never be upgraded to PASS.  Preserve a
        # machine-readable failure record so the aggregate remains auditable.
        if ($Status -eq "PASS") {
            $Status = "FAIL"
            $ExitCode = if ($ExitCode -eq 0) { 1 } else { $ExitCode }
            $Message = "evidence is missing; requested PASS was rejected: $Message"
        }
        $sourceEvidenceFile = Join-Path $validationDirectory ("raw-" + $Name.ToLowerInvariant() + ".json")
        $missingStarted = if ($StartedAt) { $StartedAt } else { [DateTime]::UtcNow.ToString("o") }
        $missingFinished = if ($FinishedAt) { $FinishedAt } else { [DateTime]::UtcNow.ToString("o") }
        Write-ResultJson $sourceEvidenceFile ([ordered]@{
            schema="omnipack-release-evidence";schema_version=1;test=$gateTestNames[$Name];gate_name=$Name
            run_id=$runId;commit=$commitAtStart;status=$Status;passed=($Status -eq "PASS")
            candidate_sha256=$currentCandidateSha256;symbols_sha256=$currentSymbolsSha256;symbols_member_sha256=$currentSymbolsMemberSha256;gate_started_at=$missingStarted;gate_finished_at=$missingFinished
            reason=$Message
        })
    } elseif ([IO.Path]::GetExtension($sourceEvidenceFile) -eq ".json") {
        try {
            $rawDocument = Get-Content -LiteralPath $sourceEvidenceFile -Raw | ConvertFrom-Json
            if ($null -ne $rawDocument) {
                $rawIdentityOk = $rawDocument.schema -eq "omnipack-release-evidence" -and
                    $rawDocument.schema_version -eq 1 -and
                    $rawDocument.test -eq $gateTestNames[$Name] -and
                    $rawDocument.gate_name -eq $Name -and
                    $rawDocument.run_id -eq $runId -and
                    $rawDocument.commit -eq $commitAtStart -and
                    $rawDocument.status -eq $Status -and
                    $rawDocument.passed -eq ($Status -eq "PASS") -and
                    $null -ne $rawDocument.gate_started_at -and
                    $null -ne $rawDocument.gate_finished_at
                if ($Status -eq "PASS" -and -not $rawIdentityOk) {
                    $Status = "FAIL"
                    $ExitCode = if ($ExitCode -eq 0) { 1 } else { $ExitCode }
                    $Message = "raw evidence identity or result fields are invalid; requested PASS was rejected: $Message"
                }
            }
        } catch {
            if ($Status -eq "PASS") {
                $Status = "FAIL"
                $ExitCode = if ($ExitCode -eq 0) { 1 } else { $ExitCode }
                $Message = "source evidence is not valid JSON; requested PASS was rejected: $Message"
            }
        }
    } elseif ($Status -eq "PASS") {
        $Status = "FAIL"
        $ExitCode = if ($ExitCode -eq 0) { 1 } else { $ExitCode }
        $Message = "source evidence is not a JSON evidence document; requested PASS was rejected: $Message"
    }
    if (-not $StartedAt) { $StartedAt = (Get-Item -LiteralPath $sourceEvidenceFile).LastWriteTimeUtc.ToString("o") }
    if (-not $FinishedAt) { $FinishedAt = [DateTime]::UtcNow.ToString("o") }
    $sourceEvidenceRelative = Get-RelativeEvidencePath $sourceEvidenceFile
    $sourceEvidenceHash = Get-Sha256Hex $sourceEvidenceFile
    $gateEvidenceFile = Join-Path $validationDirectory ("gate-" + $Name.ToLowerInvariant() + ".json")
    Write-ResultJson $gateEvidenceFile ([ordered]@{
        schema="omnipack-release-evidence";schema_version=1;test=$gateTestNames[$Name];gate_name=$Name
        run_id=$runId;commit=$commitAtStart;status=$Status;passed=($Status -eq "PASS");exit_code=$ExitCode
        candidate_sha256=$currentCandidateSha256;symbols_sha256=$currentSymbolsSha256;symbols_member_sha256=$currentSymbolsMemberSha256;gate_started_at=$StartedAt;gate_finished_at=$FinishedAt
        source_evidence=$sourceEvidenceRelative;source_evidence_sha256=$sourceEvidenceHash;message=$Message
    })
    $evidenceRelative = Get-RelativeEvidencePath $gateEvidenceFile
    $evidenceHash = Get-Sha256Hex $gateEvidenceFile
    $results[$Name] = [ordered]@{
        Name=$Name; Status=$Status; Command=$Command; ExitCode=$ExitCode
        Evidence=$evidenceRelative; EvidenceSha256=$evidenceHash; Message=$Message
    }
}
function Set-GateCandidateBinding {
    param([string]$Name,[string]$CandidateSha256)
    if (-not $results.Contains($Name)) { throw "Cannot bind missing gate to candidate: $Name" }
    $gateEvidenceFile = Join-Path $validationDirectory ("gate-" + $Name.ToLowerInvariant() + ".json")
    $document = Get-Content -LiteralPath $gateEvidenceFile -Raw | ConvertFrom-Json
    if (($document.PSObject.Properties.Name -contains "candidate_sha256") -and
        $null -ne $document.candidate_sha256 -and $document.candidate_sha256 -ne $CandidateSha256) {
        throw "Cannot rewrite stale gate candidate identity: $Name"
    }
    $document.candidate_sha256 = $CandidateSha256
    $sourceEvidenceFile = Join-Path $validationDirectory ([string]$document.source_evidence)
    if (-not (Test-Path -LiteralPath $sourceEvidenceFile -PathType Leaf)) { throw "Cannot bind missing raw evidence to candidate: $Name" }
    $sourceDocument = Get-Content -LiteralPath $sourceEvidenceFile -Raw | ConvertFrom-Json
    foreach ($binding in @(
        @("candidate_sha256",$CandidateSha256),
        @("symbols_sha256",$currentSymbolsSha256),
        @("symbols_member_sha256",$currentSymbolsMemberSha256)
    )) {
        $property = [string]$binding[0]
        $expected = $binding[1]
        if (($sourceDocument.PSObject.Properties.Name -contains $property) -and
            $null -ne $sourceDocument.$property -and $sourceDocument.$property -ne $expected) {
            throw "Cannot rewrite stale raw artifact identity for ${Name}: $property"
        }
    }
    $sourceDocument | Add-Member -NotePropertyName candidate_sha256 -NotePropertyValue $CandidateSha256 -Force
    $sourceDocument | Add-Member -NotePropertyName symbols_sha256 -NotePropertyValue $currentSymbolsSha256 -Force
    $sourceDocument | Add-Member -NotePropertyName symbols_member_sha256 -NotePropertyValue $currentSymbolsMemberSha256 -Force
    [IO.File]::WriteAllText($sourceEvidenceFile, ($sourceDocument | ConvertTo-Json -Depth 20) + [Environment]::NewLine, [Text.UTF8Encoding]::new($false))
    $document.source_evidence_sha256 = Get-Sha256Hex $sourceEvidenceFile
    [IO.File]::WriteAllText($gateEvidenceFile, ($document | ConvertTo-Json -Depth 12) + [Environment]::NewLine, [Text.UTF8Encoding]::new($false))
    $results[$Name].EvidenceSha256 = Get-Sha256Hex $gateEvidenceFile
}
function Write-ResultJson {
    param([string]$Path,[System.Collections.IDictionary]$Value)
    [IO.File]::WriteAllText($Path, ($Value | ConvertTo-Json -Depth 12) + [Environment]::NewLine, [Text.UTF8Encoding]::new($false))
}
function Invoke-ProcessCapture {
    param([string]$FilePath,[string[]]$Arguments,[string]$StdoutPath,[string]$StderrPath,[int]$TimeoutSeconds=600)
    # ProcessStartInfo is the deterministic equivalent of Start-Process here:
    # it preserves the real exit code for GUI-subsystem Windows executables and
    # avoids PowerShell's pipeline conversion of a Process object.
    $psi = [System.Diagnostics.ProcessStartInfo]::new()
    $psi.FileName = $FilePath
    $psi.WorkingDirectory = $sourceRoot
    $psi.UseShellExecute = $false
    $psi.CreateNoWindow = $true
    $psi.RedirectStandardOutput = $true
    $psi.RedirectStandardError = $true
    # Meson records the child environment in its private test log and may
    # replay it through --print-errorlogs. Release evidence must never inherit
    # local or CI credentials, even when a mandatory test fails. Match only
    # credential-shaped suffixes so ordinary variables such as PATH remain.
    foreach ($environmentName in @($psi.Environment.Keys)) {
        if ($environmentName -match '(?i)(TOKEN|SECRET|PASSWORD|PASSWD|API[_-]?KEY|PAT)$') {
            [void]$psi.Environment.Remove($environmentName)
        }
    }
    # Windows PowerShell 5.1 does not expose ProcessStartInfo.ArgumentList;
    # use the documented command-line string property for both PS editions.
    $psi.Arguments = [string]::Join(' ', @($Arguments | ForEach-Object {
        '"' + ([string]$_).Replace('"', '\"') + '"'
    }))
    $p = [System.Diagnostics.Process]::new()
    $p.StartInfo = $psi
    if (-not $p.Start()) { return 1 }
    $stdoutTask = $p.StandardOutput.ReadToEndAsync()
    $stderrTask = $p.StandardError.ReadToEndAsync()
    if (-not $p.WaitForExit($TimeoutSeconds * 1000)) {
        $p.Kill()
        $p.WaitForExit()
        [IO.File]::WriteAllText($StdoutPath,$stdoutTask.Result,[Text.UTF8Encoding]::new($false))
        [IO.File]::WriteAllText($StderrPath,$stderrTask.Result,[Text.UTF8Encoding]::new($false))
        return 124
    }
    [IO.File]::WriteAllText($StdoutPath,$stdoutTask.Result,[Text.UTF8Encoding]::new($false))
    [IO.File]::WriteAllText($StderrPath,$stderrTask.Result,[Text.UTF8Encoding]::new($false))
    return [int]$p.ExitCode
}
function Invoke-GateProcess {
    param([string]$Name,[string]$FilePath,[string[]]$Arguments,[string]$Evidence,[int]$TimeoutSeconds=600,[scriptblock]$Validate,[switch]$ExitTwoIsNotTested)
    $stdout = "$Evidence.stdout.txt"; $stderr = "$Evidence.stderr.txt"
    $command = ($FilePath + " " + ($Arguments -join " "))
    foreach ($target in @($Evidence,"$Evidence.json","$Evidence.bound.json",$stdout,$stderr)) {
        if (Test-Path -LiteralPath $target -PathType Leaf) { Remove-Item -LiteralPath $target -Force }
    }
    $startedAt = [DateTime]::UtcNow.ToString("o")
    try {
        $exit = Invoke-ProcessCapture $FilePath $Arguments $stdout $stderr $TimeoutSeconds
        $ok = ($exit -eq 0)
        $notTested = $false
        if ($ExitTwoIsNotTested -and $exit -eq 2 -and (Test-Path -LiteralPath $Evidence -PathType Leaf)) {
            $notTestedDocument = Get-Content -LiteralPath $Evidence -Raw | ConvertFrom-Json
            $notTested = $notTestedDocument.status -eq "NOT_TESTED" -and $notTestedDocument.passed -eq $false
        }
        $stderrText = if (Test-Path -LiteralPath $stderr) { Get-Content $stderr -Raw -ErrorAction SilentlyContinue } else { "" }
        $message = if ($ok) { "process exited 0" } else { if ($stderrText) { $stderrText.Trim() } else { "process exited $exit" } }
        if ($Validate) {
            $validated = (& $Validate)
            $ok = $ok -and $validated
            if ($exit -eq 0 -and -not $validated) { $message = "process exited 0 but evidence failed semantic validation" }
        }
        $status = if ($ok) { "PASS" } elseif ($notTested) { "NOT_TESTED" } else { "FAIL" }
        $sourceEvidence = Find-EvidenceFile $Evidence
        if (-not $sourceEvidence -or [IO.Path]::GetExtension($sourceEvidence) -ne ".json") {
            $commandEvidence = "$Evidence.json"
            Write-ResultJson $commandEvidence ([ordered]@{
                schema="omnipack-release-evidence";schema_version=1;test=$gateTestNames[$Name];gate_name=$Name
                run_id=$runId;commit=$commitAtStart;status=$status;passed=($status -eq "PASS");exit_code=$exit
                candidate_sha256=$currentCandidateSha256;symbols_sha256=$currentSymbolsSha256;symbols_member_sha256=$currentSymbolsMemberSha256;gate_started_at=$startedAt;gate_finished_at=([DateTime]::UtcNow.ToString("o"))
                stdout=(Get-RelativeEvidencePath $stdout);stdout_sha256=if(Test-Path -LiteralPath $stdout){Get-Sha256Hex $stdout}else{$null}
                stderr=(Get-RelativeEvidencePath $stderr);stderr_sha256=if(Test-Path -LiteralPath $stderr){Get-Sha256Hex $stderr}else{$null}
                reason=$message
            })
            $sourceEvidence = $commandEvidence
        } else {
            $document = Get-Content -LiteralPath $sourceEvidence -Raw | ConvertFrom-Json
            $rawResultOk = $document.schema -eq "omnipack-release-evidence" -and
                $document.schema_version -eq 1 -and
                $document.test -eq $gateTestNames[$Name] -and
                $document.status -eq $status -and
                $document.passed -eq ($status -eq "PASS")
            $identityContradiction = $false
            foreach ($binding in @(
                @("gate_name",$Name),
                @("run_id",$runId),
                @("commit",$commitAtStart),
                @("candidate_sha256",$currentCandidateSha256)
            )) {
                $property = [string]$binding[0]
                $expected = $binding[1]
                if (($document.PSObject.Properties.Name -contains $property) -and
                    $null -ne $document.$property -and $document.$property -ne $expected) {
                    $identityContradiction = $true
                }
            }
            $producerFresh = (Get-Item -LiteralPath $sourceEvidence).LastWriteTimeUtc -ge ([DateTime]::Parse($startedAt).ToUniversalTime().AddSeconds(-1))
            if ((-not $rawResultOk -or $identityContradiction -or -not $producerFresh) -and $status -eq "PASS") {
                $ok = $false
                $status = "FAIL"
                $exit = if ($exit -eq 0) { 1 } else { $exit }
                $message = "process result JSON is stale, missing, or contradicts required schema/result/identity fields"
            }
            # Preserve producer bytes unchanged.  The driver owns a fresh,
            # hash-bound adapter document because it deleted the target before
            # execution and observed this process exit.  Arbitrary Set-Gate
            # callers cannot use this adapter path to bless stale JSON.
            $producerEvidence = $sourceEvidence
            $boundEvidence = "$Evidence.bound.json"
            $bound = $document | ConvertTo-Json -Depth 20 | ConvertFrom-Json
            $bound | Add-Member -NotePropertyName gate_name -NotePropertyValue $Name -Force
            $bound | Add-Member -NotePropertyName run_id -NotePropertyValue $runId -Force
            $bound | Add-Member -NotePropertyName commit -NotePropertyValue $commitAtStart -Force
            $bound | Add-Member -NotePropertyName candidate_sha256 -NotePropertyValue $currentCandidateSha256 -Force
            $bound | Add-Member -NotePropertyName symbols_sha256 -NotePropertyValue $currentSymbolsSha256 -Force
            $bound | Add-Member -NotePropertyName symbols_member_sha256 -NotePropertyValue $currentSymbolsMemberSha256 -Force
            $bound | Add-Member -NotePropertyName status -NotePropertyValue $status -Force
            $bound | Add-Member -NotePropertyName passed -NotePropertyValue ($status -eq "PASS") -Force
            $bound | Add-Member -NotePropertyName gate_started_at -NotePropertyValue $startedAt -Force
            $bound | Add-Member -NotePropertyName gate_finished_at -NotePropertyValue ([DateTime]::UtcNow.ToString("o")) -Force
            $bound | Add-Member -NotePropertyName identity_binding -NotePropertyValue "trusted_release_driver_fresh_process_adapter" -Force
            $bound | Add-Member -NotePropertyName producer_evidence -NotePropertyValue (Get-RelativeEvidencePath $producerEvidence) -Force
            $bound | Add-Member -NotePropertyName producer_evidence_sha256 -NotePropertyValue (Get-Sha256Hex $producerEvidence) -Force
            [IO.File]::WriteAllText($boundEvidence, ($bound | ConvertTo-Json -Depth 20) + [Environment]::NewLine, [Text.UTF8Encoding]::new($false))
            $sourceEvidence = $boundEvidence
        }
        $finishedAt = [DateTime]::UtcNow.ToString("o")
        Set-Gate $Name $status $command $exit $sourceEvidence $message $startedAt $finishedAt
        return $ok
    } catch {
        Set-Gate $Name "FAIL" $command -1 $Evidence $_.Exception.Message $startedAt ([DateTime]::UtcNow.ToString("o"))
        return $false
    }
}
function Set-NotTested {
    param([string]$Name,[string]$Reason)
    $evidence = Join-Path $validationDirectory ($Name.ToLowerInvariant() + ".json")
    $started = [DateTime]::UtcNow.ToString("o")
    # File timestamp APIs can trail UtcNow by a fraction of a millisecond on
    # Windows.  Keep the producer write unambiguously after gate start so the
    # stale-evidence audit remains strict without false negatives.
    Start-Sleep -Milliseconds 2
    Write-ResultJson $evidence ([ordered]@{schema="omnipack-release-evidence";schema_version=1;test=$gateTestNames[$Name];gate_name=$Name;run_id=$runId;commit=$commitAtStart;candidate_sha256=$currentCandidateSha256;symbols_sha256=$currentSymbolsSha256;symbols_member_sha256=$currentSymbolsMemberSha256;gate_started_at=$started;gate_finished_at=([DateTime]::UtcNow.ToString("o"));passed=$false;status="NOT_TESTED";reason=$Reason})
    Set-Gate $Name "NOT_TESTED" "not executed" -1 $evidence $Reason $started ([DateTime]::UtcNow.ToString("o"))
}
function Set-SourceSnapshotImmutabilityGate {
    $commitAtEnd = (& $git -C $sourceRoot rev-parse HEAD).Trim()
    $branchAtEnd = (& $git -C $sourceRoot branch --show-current).Trim()
    $statusAtEnd = @(& $git -C $sourceRoot status --porcelain=v1 --untracked-files=all 2>$null | Where-Object { $_.Trim() })
    $sameStatus = [string]::Join("`n", $dirty) -ceq [string]::Join("`n", $statusAtEnd)
    $sourceSnapshotEnd = $null
    try { $sourceSnapshotEnd = Get-SourceSnapshot "end" } catch { $sourceSnapshotEnd = $null }
    $snapshots = @($sourceSnapshotAtStart,$sourceSnapshotAfterConfigure,$sourceSnapshotAfterBuild,$sourceSnapshotBeforePackage,$sourceSnapshotAfterPackage,$sourceSnapshotEnd)
    $snapshotHashes = @($snapshots | Where-Object { $_ } | ForEach-Object { [string]$_.source_worktree_sha256 })
    $snapshotCounts = @($snapshots | Where-Object { $_ } | ForEach-Object { [int]$_.tracked_files })
    $buildSnapshots = @($sourceSnapshotAfterConfigure,$sourceSnapshotAfterBuild,$sourceSnapshotBeforePackage,$sourceSnapshotAfterPackage,$sourceSnapshotEnd)
    $buildInputHashes = @($buildSnapshots | Where-Object { $_ } | ForEach-Object { [string]$_.build_inputs_sha256 })
    $buildInputsReady = @($buildSnapshots | Where-Object { $_ } | ForEach-Object { [bool]$_.build_inputs_ready })
    $allSnapshotsPresent = $snapshots.Count -eq 6 -and $snapshotHashes.Count -eq 6 -and $snapshotCounts.Count -eq 6
    $sameContent = $allSnapshotsPresent -and (@($snapshotHashes | Select-Object -Unique).Count -eq 1)
    $sameCount = $allSnapshotsPresent -and (@($snapshotCounts | Select-Object -Unique).Count -eq 1)
    $buildInputsPresent = $buildSnapshots.Count -eq 5 -and $buildInputHashes.Count -eq 5 -and $buildInputsReady.Count -eq 5
    $sameBuildInputs = $buildInputsPresent -and (@($buildInputHashes | Select-Object -Unique).Count -eq 1)
    $allBuildInputsReady = $buildInputsPresent -and @($buildInputsReady | Where-Object { -not $_ }).Count -eq 0
    $passed = $commitAtStart -eq $commitAtEnd -and $branchAtStart -eq $branchAtEnd -and $sameStatus -and $sameContent -and $sameCount -and $sameBuildInputs -and $allBuildInputsReady
    if ($Channel -eq "stable") { $passed = $passed -and $dirty.Count -eq 0 -and $statusAtEnd.Count -eq 0 }
    $evidence = Join-Path $validationDirectory "source-snapshot-immutability.json"
    Write-ResultJson $evidence ([ordered]@{
        schema="omnipack-release-evidence";schema_version=1;test="source_snapshot_immutability";gate_name="SourceSnapshotImmutability";run_id=$runId;commit=$commitAtStart
        gate_started_at=[DateTime]::UtcNow.ToString("o");gate_finished_at=[DateTime]::UtcNow.ToString("o")
        status=if($passed){"PASS"}else{"FAIL"};passed=$passed
        commit_start=$commitAtStart;commit_end=$commitAtEnd;branch_start=$branchAtStart;branch_end=$branchAtEnd
        git_status_start=($dirty -join "`n");git_status_end=($statusAtEnd -join "`n")
        content_hash_start=if($sourceSnapshotAtStart){$sourceSnapshotAtStart.source_worktree_sha256}else{$null}
        content_hash_after_configure=if($sourceSnapshotAfterConfigure){$sourceSnapshotAfterConfigure.source_worktree_sha256}else{$null}
        content_hash_after_build=if($sourceSnapshotAfterBuild){$sourceSnapshotAfterBuild.source_worktree_sha256}else{$null}
        content_hash_before_package=if($sourceSnapshotBeforePackage){$sourceSnapshotBeforePackage.source_worktree_sha256}else{$null}
        content_hash_after_package=if($sourceSnapshotAfterPackage){$sourceSnapshotAfterPackage.source_worktree_sha256}else{$null}
        content_hash_end=if($sourceSnapshotEnd){$sourceSnapshotEnd.source_worktree_sha256}else{$null}
        tracked_files_start=if($sourceSnapshotAtStart){[int]$sourceSnapshotAtStart.tracked_files}else{0}
        tracked_files_after_configure=if($sourceSnapshotAfterConfigure){[int]$sourceSnapshotAfterConfigure.tracked_files}else{0}
        tracked_files_after_build=if($sourceSnapshotAfterBuild){[int]$sourceSnapshotAfterBuild.tracked_files}else{0}
        tracked_files_before_package=if($sourceSnapshotBeforePackage){[int]$sourceSnapshotBeforePackage.tracked_files}else{0}
        tracked_files_after_package=if($sourceSnapshotAfterPackage){[int]$sourceSnapshotAfterPackage.tracked_files}else{0}
        tracked_files_end=if($sourceSnapshotEnd){[int]$sourceSnapshotEnd.tracked_files}else{0}
        build_inputs_hash_after_configure=if($sourceSnapshotAfterConfigure){$sourceSnapshotAfterConfigure.build_inputs_sha256}else{$null}
        build_inputs_hash_after_build=if($sourceSnapshotAfterBuild){$sourceSnapshotAfterBuild.build_inputs_sha256}else{$null}
        build_inputs_hash_before_package=if($sourceSnapshotBeforePackage){$sourceSnapshotBeforePackage.build_inputs_sha256}else{$null}
        build_inputs_hash_after_package=if($sourceSnapshotAfterPackage){$sourceSnapshotAfterPackage.build_inputs_sha256}else{$null}
        build_inputs_hash_end=if($sourceSnapshotEnd){$sourceSnapshotEnd.build_inputs_sha256}else{$null}
        build_inputs_ready_after_configure=if($sourceSnapshotAfterConfigure){[bool]$sourceSnapshotAfterConfigure.build_inputs_ready}else{$false}
        build_inputs_ready_after_build=if($sourceSnapshotAfterBuild){[bool]$sourceSnapshotAfterBuild.build_inputs_ready}else{$false}
        build_inputs_ready_before_package=if($sourceSnapshotBeforePackage){[bool]$sourceSnapshotBeforePackage.build_inputs_ready}else{$false}
        build_inputs_ready_after_package=if($sourceSnapshotAfterPackage){[bool]$sourceSnapshotAfterPackage.build_inputs_ready}else{$false}
        build_inputs_ready_end=if($sourceSnapshotEnd){[bool]$sourceSnapshotEnd.build_inputs_ready}else{$false}
    })
    Set-Gate "SourceSnapshotImmutability" $(if($passed){"PASS"}else{"FAIL"}) "git identity/status and full content snapshot comparison" $(if($passed){0}else{1}) $evidence $(if($passed){"source snapshot remained unchanged at every checkpoint"}else{"source snapshot changed or a required checkpoint is missing"})
    return $passed
}
function Invoke-SoakGate {
    param([string]$Executable,[string]$PackageZip)
    $soakDirectory = Join-Path $validationDirectory "soak"
    $soakCandidateRoot = [IO.Path]::GetFullPath((Join-Path $validationDirectory "soak-candidate-extracted"))
    $validationPrefix = [IO.Path]::GetFullPath($validationDirectory).TrimEnd('\') + '\'
    if (-not $soakCandidateRoot.StartsWith($validationPrefix, [StringComparison]::OrdinalIgnoreCase)) {
        throw "refusing to extract the soak candidate outside the current validation directory"
    }
    $soakHarness = Join-Path $sourceRoot "tools\runtime_stress_test.ps1"
    $soakStdout = Join-Path $validationDirectory "soak-harness.stdout.txt"
    $soakStderr = Join-Path $validationDirectory "soak-harness.stderr.txt"
    $soakCommand = "runtime_stress_test.ps1 -LongRun -SampleId S20-FULL-CATALOG -PackageZip <current-candidate>"
    foreach ($target in @($soakStdout,$soakStderr,(Join-Path $validationDirectory "soak-2h.json"))) {
        if (Test-Path -LiteralPath $target -PathType Leaf) { Remove-Item -LiteralPath $target -Force }
    }
    $startedAt = [DateTime]::UtcNow.ToString("o")
    try {
        if (Test-Path -LiteralPath $soakCandidateRoot) {
            throw "fresh soak candidate extraction directory already exists"
        }
        New-Item -ItemType Directory -Path $soakCandidateRoot | Out-Null
        Expand-Archive -LiteralPath $PackageZip -DestinationPath $soakCandidateRoot
        $candidateExecutables = @(Get-ChildItem -LiteralPath $soakCandidateRoot -Filter "tpt-zh-omnipack.exe" -Recurse -File)
        if ($candidateExecutables.Count -ne 1) {
            throw "soak candidate must contain exactly one target executable"
        }
        $candidateExecutable = $candidateExecutables[0].FullName
        $candidateExecutableSha256 = Get-Sha256Hex $candidateExecutable
        $buildExecutableSha256 = Get-Sha256Hex $Executable
        if ($candidateExecutableSha256 -ne $buildExecutableSha256) {
            throw "soak candidate executable bytes differ from the verified release executable"
        }
        & powershell.exe -NoProfile -ExecutionPolicy Bypass -File $soakHarness -Executable $candidateExecutable -SampleId S20-FULL-CATALOG -LongRun -PackageZip $PackageZip -PackageVersion $version -OutputDirectory $soakDirectory *> $soakStdout
        $harnessExit = $LASTEXITCODE
        if ($harnessExit -ne 0) { throw "soak harness exit=$harnessExit" }
        $resultPath = Get-ChildItem $soakDirectory -Filter result.json -Recurse | Sort-Object LastWriteTimeUtc -Descending | Select-Object -First 1 -ExpandProperty FullName
        if (-not $resultPath) { throw "soak result.json is absent" }
        $artifactDirectory = Split-Path $resultPath -Parent
        $assessment = Join-Path $validationDirectory "soak-2h.json"
        & $python (Join-Path $sourceRoot "tools\analyze_stress_result.py") $artifactDirectory --output $assessment *> $soakStderr
        $analysisExit = $LASTEXITCODE
        $r = if (Test-Path $assessment) { Get-Content $assessment -Raw | ConvertFrom-Json } else { $null }
        $candidateHash = Get-Sha256Hex $PackageZip
        if ($r) {
            $r | Add-Member -NotePropertyName harness_run_id -NotePropertyValue $r.run_id -Force
            $r.run_id = $runId
            $r | Add-Member -NotePropertyName commit -NotePropertyValue $commitAtStart -Force
            $r.candidate_sha256 = $candidateHash
            $r | Add-Member -NotePropertyName candidate_extracted_to_fresh_directory -NotePropertyValue $true -Force
            $r | Add-Member -NotePropertyName candidate_target_executable_count -NotePropertyValue $candidateExecutables.Count -Force
            $r | Add-Member -NotePropertyName executable_source -NotePropertyValue "candidate_zip" -Force
            $r | Add-Member -NotePropertyName candidate_executable_sha256 -NotePropertyValue $candidateExecutableSha256 -Force
            $r | Add-Member -NotePropertyName build_executable_sha256 -NotePropertyValue $buildExecutableSha256 -Force
            [IO.File]::WriteAllText($assessment, ($r | ConvertTo-Json -Depth 12) + [Environment]::NewLine, [Text.UTF8Encoding]::new($false))
        }
        $passed = $analysisExit -eq 0 -and $r -and $r.schema -eq "omnipack-release-evidence" -and $r.schema_version -eq 1 -and $r.test -eq "soak_2h" -and $r.status -eq "PASS" -and $r.passed -eq $true -and $r.source_commit -eq $commitAtStart -and $r.long_run_gate_pass -eq $true -and $r.performance_gate_pass -eq $true -and $r.public_zip_sha256 -eq $candidateHash -and [double]$r.wall_clock_seconds -ge 7200.0 -and $r.candidate_extracted_to_fresh_directory -eq $true -and $r.candidate_target_executable_count -eq 1 -and $r.executable_source -eq "candidate_zip" -and $r.candidate_executable_sha256 -eq $candidateExecutableSha256 -and $r.build_executable_sha256 -eq $candidateExecutableSha256 -and $r.exe_sha256 -eq $candidateExecutableSha256
        $soakStatus = if($passed){"PASS"}else{"FAIL"}
        $soakMessage = if($passed){"7200-second wall-clock soak passed against current candidate ZIP"}else{"soak evidence failed or artifact SHA does not match current candidate"}
        Set-Gate "Soak2Hours" $soakStatus $soakCommand $analysisExit $assessment $soakMessage $startedAt ([DateTime]::UtcNow.ToString("o"))
    } catch {
        $evidence = Join-Path $validationDirectory "soak-2h.json"
        Write-ResultJson $evidence ([ordered]@{schema="omnipack-release-evidence";schema_version=1;test="soak_2h";run_id=$runId;commit=$commitAtStart;candidate_sha256=$currentCandidateSha256;passed=$false;status="FAIL";reason=$_.Exception.Message})
        Set-Gate "Soak2Hours" "FAIL" $soakCommand -1 $evidence $_.Exception.Message $startedAt ([DateTime]::UtcNow.ToString("o"))
    } finally {
        if (Test-Path -LiteralPath $soakCandidateRoot) {
            Remove-Item -LiteralPath $soakCandidateRoot -Recurse -Force
        }
    }
}
function Invoke-ValidationAuditor {
    param([string]$ValidationJson,[string]$ValidationText,[string]$BuildInfo,[string]$Package,[string]$Symbols)
    $auditEvidence = Join-Path $validationDirectory "evidence-integrity.json"
    $auditArgs = @((Join-Path $sourceRoot "tools\release_validation_audit.py"),"--validation-json",$ValidationJson,"--validation-text",$ValidationText,"--build-info",$BuildInfo,"--channel",$Channel,"--output",$auditEvidence)
    if ($Package -and (Test-Path -LiteralPath $Package)) { $auditArgs += @("--package",$Package) }
    if ($Symbols -and (Test-Path -LiteralPath $Symbols)) { $auditArgs += @("--symbols",$Symbols) }
    $auditStdout = "$auditEvidence.stdout.txt"; $auditStderr = "$auditEvidence.stderr.txt"
    foreach ($target in @($auditEvidence,$auditStdout,$auditStderr)) { if (Test-Path -LiteralPath $target) { Remove-Item -LiteralPath $target -Force } }
    $startedAt = [DateTime]::UtcNow.ToString("o")
    $exit = Invoke-ProcessCapture $python $auditArgs $auditStdout $auditStderr 120
    $finishedAt = [DateTime]::UtcNow.ToString("o")
    $docResult = if (Test-Path -LiteralPath $auditEvidence) { Get-Content $auditEvidence -Raw | ConvertFrom-Json } else { $null }
    $hashOk = $docResult -and $docResult.evidence_hash_integrity -eq $true
    $semanticOk = $docResult -and $docResult.evidence_semantic_integrity -eq $true
    $docOk = $docResult -and $docResult.documentation_consistency -eq $true
    # The combined auditor output is diagnostic input, not the raw evidence for
    # three different gates.  Give every meta gate its own identity-bound raw
    # document so strict gate_name validation cannot be defeated (or made to
    # fail spuriously) by reusing one mutable JSON file.
    $metaGateSpecs = @(
        [ordered]@{Name="EvidenceHashIntegrity";Passed=$hashOk;File="evidence-hash-integrity.json";Command="release_validation_audit.py --hash";PassMessage="evidence hashes match";FailMessage="evidence hash audit failed"},
        [ordered]@{Name="EvidenceSemanticIntegrity";Passed=$semanticOk;File="evidence-semantic-integrity.json";Command="release_validation_audit.py --semantic";PassMessage="gate and evidence semantics match";FailMessage="evidence semantic audit failed"},
        [ordered]@{Name="DocumentationConsistency";Passed=$docOk;File="documentation-consistency.json";Command="release_validation_audit.py --channel $Channel";PassMessage="validation JSON, text, build info, and package agree";FailMessage="documentation consistency audit failed"}
    )
    foreach ($spec in $metaGateSpecs) {
        $perGateEvidence = Join-Path $validationDirectory $spec.File
        $perGateDocument = if ($docResult) {
            $docResult | ConvertTo-Json -Depth 20 | ConvertFrom-Json
        } else {
            [pscustomobject]@{
                schema="omnipack-release-evidence";schema_version=1;test="release_validation_audit"
                evidence_hash_integrity=$false;evidence_semantic_integrity=$false;documentation_consistency=$false
                reason="release validation auditor did not produce readable JSON evidence"
            }
        }
        $perGateStatus = if ($spec.Passed) { "PASS" } else { "FAIL" }
        $perGateDocument | Add-Member -NotePropertyName gate_name -NotePropertyValue $spec.Name -Force
        $perGateDocument | Add-Member -NotePropertyName run_id -NotePropertyValue $runId -Force
        $perGateDocument | Add-Member -NotePropertyName commit -NotePropertyValue $commitAtStart -Force
        $perGateDocument | Add-Member -NotePropertyName status -NotePropertyValue $perGateStatus -Force
        $perGateDocument | Add-Member -NotePropertyName passed -NotePropertyValue ([bool]$spec.Passed) -Force
        $perGateDocument | Add-Member -NotePropertyName candidate_sha256 -NotePropertyValue $currentCandidateSha256 -Force
        $perGateDocument | Add-Member -NotePropertyName symbols_sha256 -NotePropertyValue $currentSymbolsSha256 -Force
        $perGateDocument | Add-Member -NotePropertyName symbols_member_sha256 -NotePropertyValue $currentSymbolsMemberSha256 -Force
        $perGateDocument | Add-Member -NotePropertyName gate_started_at -NotePropertyValue $startedAt -Force
        $perGateDocument | Add-Member -NotePropertyName gate_finished_at -NotePropertyValue $finishedAt -Force
        [IO.File]::WriteAllText($perGateEvidence, ($perGateDocument | ConvertTo-Json -Depth 20) + [Environment]::NewLine, [Text.UTF8Encoding]::new($false))
        $perGateMessage = if ($spec.Passed) { $spec.PassMessage } else { $spec.FailMessage }
        Set-Gate $spec.Name $perGateStatus $spec.Command $(if($spec.Passed){0}else{1}) $perGateEvidence $perGateMessage $startedAt $finishedAt
    }
    return ($exit -eq 0 -and $hashOk -and $semanticOk -and $docOk)
}
function Write-AggregateValidation {
    param([string]$JsonPath,[string]$TextPath,[string]$Status,[string[]]$Blocking,[string]$Scope="complete")
    $commit = (& $git -C $sourceRoot rev-parse HEAD).Trim()
    $postPackageEvidence = if ($Scope -eq "pre_package") { "external_sidecar_required" } else { "included_in_this_document" }
    $buildInputSnapshot = @($sourceSnapshotAfterPackage,$sourceSnapshotBeforePackage,$sourceSnapshotAfterBuild,$sourceSnapshotAfterConfigure) | Where-Object { $null -ne $_ } | Select-Object -First 1
    $aggregate = [ordered]@{schema="omnipack-release-validation";schema_version=1;version=$version;channel=$Channel;run_id=$runId;commit=$commit;source_snapshot_sha256=$sourceSnapshotAtStart.source_worktree_sha256;build_inputs_sha256=if($buildInputSnapshot){$buildInputSnapshot.build_inputs_sha256}else{$null};candidate_sha256=$currentCandidateSha256;symbols_sha256=$currentSymbolsSha256;symbols_member_sha256=$currentSymbolsMemberSha256;validation_scope=$Scope;post_package_evidence=$postPackageEvidence;status=$Status;final_status=$Status;gates=$results;blocking_items=$Blocking}
    Write-ResultJson $JsonPath $aggregate
    $lines = @("TPT-ZH OmniPack $version Release Validation","","Commit: $commit","Channel: $Channel","Run ID: $runId","Source snapshot SHA256: $($sourceSnapshotAtStart.source_worktree_sha256)","Build inputs SHA256: $(if($buildInputSnapshot){$buildInputSnapshot.build_inputs_sha256}else{$null})","Candidate SHA256: $currentCandidateSha256","Symbols SHA256: $currentSymbolsSha256","Symbols member SHA256: $currentSymbolsMemberSha256","")
    foreach ($item in $results.Values) { $lines += "$($item.Name): $($item.Status) | exit=$($item.ExitCode) | evidence=$($item.Evidence)" }
    $lines += "", "FINAL STATUS: $Status"
    [IO.File]::WriteAllLines($TextPath,$lines,[Text.UTF8Encoding]::new($false))
}

$dirty = @(& $git -C $sourceRoot status --porcelain=v1 --untracked-files=all 2>$null | Where-Object { $_.Trim() })
    $commitAtStart = (& $git -C $sourceRoot rev-parse HEAD).Trim()
$branchAtStart = (& $git -C $sourceRoot branch --show-current).Trim()
$sourceSnapshotAtStart = Get-SourceSnapshot "start"
$sourceSnapshotAfterConfigure = $null
$sourceSnapshotAfterBuild = $null
$sourceSnapshotBeforePackage = $null
$sourceSnapshotAfterPackage = $null
$sourceEvidence = Join-Path $validationDirectory "source-tree-clean.json"
    Write-ResultJson $sourceEvidence ([ordered]@{
    schema="omnipack-release-evidence";schema_version=1;test="source_tree_clean";gate_name="SourceTreeClean";run_id=$runId;commit=$commitAtStart
    gate_started_at=[DateTime]::UtcNow.ToString("o");gate_finished_at=[DateTime]::UtcNow.ToString("o")
    passed=($dirty.Count -eq 0); status=if($dirty.Count -eq 0){"PASS"}else{"NOT_TESTED"}
    candidate_sha256=$currentCandidateSha256;symbols_sha256=$currentSymbolsSha256; branch=$branchAtStart; porcelain_output=($dirty -join "`n")
    content_hash=$sourceSnapshotAtStart.source_worktree_sha256; tracked_files=$sourceSnapshotAtStart.tracked_files
})
if ($dirty.Count -eq 0) {
    Set-Gate "SourceTreeClean" "PASS" "git status --porcelain=v1 --untracked-files=all" 0 $sourceEvidence "source tree is clean"
} else {
    Set-Gate "SourceTreeClean" "NOT_TESTED" "git status --porcelain=v1 --untracked-files=all" 1 $sourceEvidence "source tree is dirty"
    if ($Channel -eq "stable" -or -not $AllowDirtyValidation) {
        $failure = "RELEASE BLOCKED: source worktree is dirty"
        Write-AggregateValidation (Join-Path $validationDirectory "RELEASE-VALIDATION.json") (Join-Path $validationDirectory "RELEASE-VALIDATION.txt") "NOT READY FOR 1.1.0 STABLE" @("SourceTreeClean") "preflight"
        exit 1
    }
}

try {
    $setupArgs = if (Test-Path -LiteralPath (Join-Path $buildDirectory "build.ninja")) {
        @("setup", "--reconfigure", $buildDirectory, $sourceRoot, "-Dbuildtype=release", "-Ddebug=true", "-Dstrip=false", "-Dstatic=prebuilt", "-Dsdl_backend=sdl3", "-Dbuild_tests=true", "-Drelease_label=$version")
    } else {
        @("setup", $buildDirectory, $sourceRoot, "-Dbuildtype=release", "-Ddebug=true", "-Dstrip=false", "-Dstatic=prebuilt", "-Dsdl_backend=sdl3", "-Dbuild_tests=true", "-Drelease_label=$version")
    }
    $configureOk = Invoke-GateProcess "Configure" $meson $setupArgs (Join-Path $validationDirectory "configure")
    if ($configureOk) {
        $sourceSnapshotAfterConfigure = Get-SourceSnapshot "after-configure"
        if ($sourceSnapshotAfterConfigure.build_inputs_ready -ne $true) {
            throw "configured release build inputs are absent, modified, or do not match the tracked Meson wrap"
        }
    }
    $buildOk = Invoke-GateProcess "Build" $ninja @("-C",$buildDirectory) (Join-Path $validationDirectory "build")
    $exe = Join-Path $buildDirectory "tpt-zh-omnipack.exe"
    if ($buildOk) {
        $sourceSnapshotAfterBuild = Get-SourceSnapshot "after-build"
        Invoke-GateProcess "UnitTests" $meson @("test","-C",$buildDirectory,"--suite","static","--print-errorlogs","-t","4") (Join-Path $validationDirectory "unit-tests") -TimeoutSeconds 1200 | Out-Null
        Invoke-GateProcess "AtmosphereBench" $meson @("test","-C",$buildDirectory,"--suite","static","--print-errorlogs","--no-rebuild","atmospherebench-contract") (Join-Path $validationDirectory "atmosphere-bench") | Out-Null
        Invoke-GateProcess "MassConservation" $meson @("test","-C",$buildDirectory,"--suite","static","--print-errorlogs","--no-rebuild","omni-atmosphere-cpu-mvp","omni-atmosphere-multispecies") (Join-Path $validationDirectory "mass-conservation") | Out-Null
        Invoke-GateProcess "OmniSaveRoundtrip" $meson @("test","-C",$buildDirectory,"--suite","static","--print-errorlogs","--no-rebuild","omni-save-roundtrip-probe") (Join-Path $validationDirectory "omni-save-roundtrip") -TimeoutSeconds 300 | Out-Null

        $gpuJson = Join-Path $validationDirectory "gpu-validation.json"
        $gpuOk = Invoke-GateProcess "GPUNumericalValidation" $exe @("--gpu-validate","--gpu-validate-json",$gpuJson) $gpuJson -TimeoutSeconds 300 -Validate {
            if (-not (Test-Path $gpuJson)) { return $false }
            $r = Get-Content $gpuJson -Raw | ConvertFrom-Json
            return ($r.schema -eq "omnipack-release-evidence" -and $r.schema_version -eq 1 -and $r.test -eq "gpu_validation" -and
                $r.status -eq "PASS" -and $r.passed -eq $true -and $r.supported -eq $true -and $r.fallback -eq $false -and
                $r.backend -eq "SDL_GPU Vulkan" -and $r.reason -eq "PASS" -and $r.first_mismatch_index -eq -1 -and
                $r.max_abs_error -is [ValueType] -and [double]$r.max_abs_error -ge 0 -and [double]$r.max_abs_error -le 0.001 -and
                $r.max_rel_error -is [ValueType] -and [double]$r.max_rel_error -ge 0 -and [double]$r.max_rel_error -le 0.01)
        }
        $probe = Join-Path $validationDirectory "gpu-probe.txt"
        Invoke-GateProcess "SDL3Runtime" $exe @("--gpu-probe") $probe -TimeoutSeconds 300 -Validate {
            $r = Read-KeyValueEvidence "$probe.stdout.txt"
            return ($r.sdl_backend -eq "SDL3" -and $r.production_thermal_diffusion_built -eq "true" -and
                $r.gpu_supported -eq "true" -and $r.spirv_supported -eq "true" -and
                $r.gpu_backend -eq "vulkan" -and $r.compute_poc_executed -eq "true" -and
                $r.deterministic_compare -eq "true" -and $r.fallback_cpu -eq "false" -and
                $r.cuda_backend_available -eq "false" -and
                $r.cuda_backend_status -eq "not_implemented_optional_future_backend")
        } | Out-Null
        $fallbackJson = Join-Path $validationDirectory "cpu-fallback.json"
        Invoke-GateProcess "CPUFallbackValidation" $exe @("--cpu-fallback-validate","--cpu-fallback-json",$fallbackJson) $fallbackJson -TimeoutSeconds 300 -Validate {
            if (-not (Test-Path $fallbackJson)) { return $false }
            $r = Get-Content $fallbackJson -Raw | ConvertFrom-Json
            $massResidual = if ($null -ne $r.mass_residual_kg) { [double]$r.mass_residual_kg } else { [double]::NaN }
            $energyResidual = if ($null -ne $r.energy_residual_j) { [double]$r.energy_residual_j } else { [double]::NaN }
            return ($r.schema -eq "omnipack-release-evidence" -and $r.schema_version -eq 1 -and $r.test -eq "cpu_fallback" -and
                $r.status -eq "PASS" -and $r.passed -eq $true -and $r.backend -eq "CPU" -and $r.fallback -eq $true -and
                $r.reason -eq "production_runtime_failure_same_step_cpu_fallback" -and
                $r.initialization_failure_forced -eq $true -and $r.initialization_backend_prearmed -eq $true -and
                $r.initialization_backend_reset_to_cpu -eq $true -and $r.initialization_cpu_path_executed -eq $true -and
                $r.initialization_control_state_match -eq $true -and $r.runtime_failure_injected -eq $true -and
                $r.runtime_executor_invocations -eq 1 -and $r.runtime_executor_input_valid -eq $true -and
                $r.runtime_failure_observed -eq $true -and $r.production_atmosphere_step_executed -eq $true -and
                $r.same_step_cpu_fallback -eq $true -and $r.backend_reset_to_cpu -eq $true -and
                $r.backend_available_after_failure -eq $false -and $r.cpu_control_state_match -eq $true -and
                $r.state_changed_same_step -eq $true -and $r.control_nonfinite_cells -eq 0 -and
                $r.initialization_nonfinite_cells -eq 0 -and $r.runtime_nonfinite_cells -eq 0 -and
                $r.nonfinite_cells -eq 0 -and $r.mass_residual_finite -eq $true -and
                $r.energy_residual_finite -eq $true -and $r.mass_residual_within_tolerance -eq $true -and
                $r.energy_residual_within_tolerance -eq $true -and
                -not [double]::IsNaN($massResidual) -and -not [double]::IsInfinity($massResidual) -and
                -not [double]::IsNaN($energyResidual) -and -not [double]::IsInfinity($energyResidual) -and
                $r.runtime_failure_code -eq "injected_runtime_thermal_failure" -and
                $r.backend_detail -eq "runtime thermal diffusion fallback: injected_runtime_thermal_failure")
        } | Out-Null
    }

    $rawExe = Join-Path $buildDirectory "tpt-zh-omnipack.exe"
    $releaseExe = Join-Path $releaseDirectory "tpt-zh-omnipack.exe"
    $symbolFile = Join-Path $symbolsDirectory "tpt-zh-omnipack.debug"
    Invoke-GateProcess "DebugSymbolsSeparated" $python @((Join-Path $sourceRoot "tools\prepare_windows_release.py"),"--raw-executable",$rawExe,"--executable",$releaseExe,"--symbols",$symbolFile,"--objcopy",$objcopy,"--strip",$strip) (Join-Path $validationDirectory "symbols") -TimeoutSeconds 300 | Out-Null
    Invoke-GateProcess "ReleaseBinaryStripped" $python @((Join-Path $sourceRoot "tools\release_binary_audit.py"),"--executable",$releaseExe,"--symbols",$symbolFile,"--objdump",$objdump,"--strings",$strings) (Join-Path $validationDirectory "binary") -TimeoutSeconds 300 | Out-Null

    $officialCorpus = if ($OfficialSaveCorpus) {
        if ([IO.Path]::IsPathRooted($OfficialSaveCorpus)) { [IO.Path]::GetFullPath($OfficialSaveCorpus) } else { Join-Path $sourceRoot $OfficialSaveCorpus }
    } else {
        Join-Path $sourceRoot "tests\saves\official-tpt"
    }
    $officialManifest = Join-Path $officialCorpus "provenance.json"
    $provenanceEvidence = Join-Path $validationDirectory "official-tpt-provenance.json"
    $provenanceArgs = @((Join-Path $sourceRoot "tools\official_tpt_provenance.py"),"--corpus",$officialCorpus,"--manifest",$officialManifest,"--repository",$sourceRoot,"--git",$git,"--remote","official","--run-id",$runId,"--commit",$commitAtStart,"--output",$provenanceEvidence)
    $provenanceOk = Invoke-GateProcess "OfficialTPTCorpusProvenance" $python $provenanceArgs $provenanceEvidence -TimeoutSeconds 1200 -ExitTwoIsNotTested -Validate {
        if (-not (Test-Path -LiteralPath $provenanceEvidence)) { return $false }
        $r = Get-Content -LiteralPath $provenanceEvidence -Raw | ConvertFrom-Json
        return ($r.schema -eq "omnipack-release-evidence" -and $r.schema_version -eq 1 -and $r.test -eq "official_tpt_provenance" -and $r.run_id -eq $runId -and $r.commit -eq $commitAtStart -and $r.status -eq "PASS" -and $r.passed -eq $true -and $r.revision_exists -eq $true -and $r.revision_reachable_from_official_remote -eq $true -and $r.files_verified -gt 0 -and $r.files_failed -eq 0)
    }
    $results["OfficialTPTCorpusProvenance"].Command = "official_tpt_provenance.py --corpus <official-corpus> --repository <official-git-remote>"
    $officialEvidence = Join-Path $validationDirectory "official-tpt-save.json"
    $officialProbe = Join-Path $buildDirectory "official_save_roundtrip_probe.exe"
    $officialArgs = @((Join-Path $sourceRoot "tools\official_save_compatibility.py"),"--corpus",$officialCorpus,"--provenance-evidence",$provenanceEvidence,"--probe",$officialProbe,"--run-id",$runId,"--commit",$commitAtStart,"--output",$officialEvidence)
    $officialOk = Invoke-GateProcess "OfficialTPTSaveCompatibility" $python $officialArgs $officialEvidence -TimeoutSeconds 1200 -ExitTwoIsNotTested -Validate {
        if (-not (Test-Path $officialEvidence)) { return $false }
        $r = Get-Content $officialEvidence -Raw | ConvertFrom-Json
        return ($r.schema -eq "omnipack-release-evidence" -and $r.schema_version -eq 1 -and $r.test -eq "official_tpt_save_compatibility" -and $r.run_id -eq $runId -and $r.commit -eq $commitAtStart -and $r.status -eq "PASS" -and $r.passed -eq $true -and $r.files_total -gt 0 -and $r.files_passed -eq $r.files_total -and $r.files_failed -eq 0 -and $r.coverage_contract -eq "official-tpt-save-coverage-v1" -and $r.coverage_passed -eq $true -and @($r.coverage_missing).Count -eq 0)
    }
    $results["OfficialTPTSaveCompatibility"].Command = "official_save_compatibility.py --corpus <provenance-verified-official-corpus> --probe <fresh-build-probe>"
    $prePackageMandatory = @("SourceTreeClean","Configure","Build","UnitTests","AtmosphereBench","MassConservation","OmniSaveRoundtrip","OfficialTPTCorpusProvenance","OfficialTPTSaveCompatibility","SDL3Runtime","GPUNumericalValidation","CPUFallbackValidation","ReleaseBinaryStripped","DebugSymbolsSeparated")
    $blocked = @($prePackageMandatory | Where-Object { -not $results.Contains($_) -or $results[$_].Status -ne "PASS" })
    $finalStatus = if ($blocked.Count -eq 0) { "PRE-PACKAGE VALIDATION COMPLETE - POST-PACKAGE GATES PENDING" } elseif ($Channel -eq "stable") { "NOT READY FOR 1.1.0 STABLE" } else { "RC VALIDATION COMPLETE - STABLE BLOCKED" }
    $validationJson = Join-Path $validationDirectory "RELEASE-VALIDATION.json"
    $validationTxt = Join-Path $validationDirectory "RELEASE-VALIDATION.txt"
    Write-AggregateValidation $validationJson $validationTxt $finalStatus $blocked "pre_package"
    # A stable name is reserved for a fully passing pre-package gate. RC may
    # still produce an auditable candidate so missing external evidence remains
    # visible without ever being relabeled as stable.
    if ($Channel -eq "stable" -and $blocked.Count -gt 0) {
        foreach ($name in @("SourceSnapshotImmutability","SDL3GUI","Soak2Hours","PackageManifest","PackageVerification","SymbolPackageVerification","WindowsPortableExtraction","WindowsCleanMachine","CandidateSHA256","ArtifactImmutability","EvidenceSemanticIntegrity","EvidenceHashIntegrity","DocumentationConsistency","NegativeGateSuite")) {
            Set-NotTested $name "stable package was not created because pre-package mandatory gates are blocked"
        }
        throw "RELEASE BLOCKED: $($blocked -join ', ')"
    }

    $buildInfo = Join-Path $validationDirectory "BUILD-INFO.txt"
    $sourceSnapshotBeforePackage = Get-SourceSnapshot "before-package"
    $commit = (& $git -C $sourceRoot rev-parse HEAD).Trim()
    [IO.File]::WriteAllLines($buildInfo,@("TPT-ZH OmniPack $version","Git commit: $commit","Channel: $Channel","Source snapshot SHA256: $($sourceSnapshotBeforePackage.source_worktree_sha256)","Build inputs SHA256: $($sourceSnapshotBeforePackage.build_inputs_sha256)","Build type: Release with detached DWARF symbols","SDL: 3.4.14","Platform: Windows x64","Compute backend: SDL_GPU Vulkan; CPU fallback; CUDA future optional"),[Text.UTF8Encoding]::new($false))
    $packageArgs = @((Join-Path $sourceRoot "tools\package_test_release.py"),"--source-root",$sourceRoot,"--executable",$releaseExe,"--symbols",$symbolFile,"--output-directory",$outputDirectory,"--version",$version,"--kind",$kind,"--artifact-stem",$artifactStem,"--symbol-artifact-stem",$symbolArtifactStem,"--objdump",$objdump,"--strings",$strings,"--extra-member",$buildInfo,"BUILD-INFO.txt")
    if ($Channel -eq "stable") {
        $packageArgs += @(
            "--expected-source-snapshot",[string]$sourceSnapshotBeforePackage.source_worktree_sha256,
            "--expected-build-inputs-snapshot",[string]$sourceSnapshotBeforePackage.build_inputs_sha256
        )
    }
    if ($Channel -eq "rc") { $packageArgs += @("--extra-member",$validationTxt,"RELEASE-VALIDATION.txt","--extra-member",$validationJson,"RELEASE-VALIDATION.json") }
    if ($Channel -eq "rc" -and $AllowDirtyValidation) { $packageArgs += "--allow-dirty-validation" }
    $packageOk = Invoke-GateProcess "PackageManifest" $python $packageArgs (Join-Path $validationDirectory "package") -TimeoutSeconds 600
    if ($packageOk) {
        $sourceSnapshotAfterPackage = Get-SourceSnapshot "after-package"
        # This archive is the final candidate. Post-package gates must only read
        # it so soak, portable, clean-machine, and hash evidence bind one ZIP.
        $candidateSha256 = Get-Sha256Hex $releaseZip
        $symbolArchiveSha256 = Get-Sha256Hex $symbolsZip
        $currentCandidateSha256 = $candidateSha256
        $currentSymbolsSha256 = $symbolArchiveSha256
        $currentSymbolsMemberSha256 = Get-Sha256Hex $symbolFile
        Set-GateCandidateBinding "PackageManifest" $candidateSha256
        if ($SkipLongSoak) {
            Set-NotTested "Soak2Hours" "explicitly skipped"
        } else {
            Invoke-SoakGate $releaseExe $releaseZip
        }
        $portableEvidence = Join-Path $validationDirectory "windows-portable.json"
        Invoke-GateProcess "WindowsPortableExtraction" powershell.exe @("-NoProfile","-ExecutionPolicy","Bypass","-File",(Join-Path $sourceRoot "tools\test_clean_release.ps1"),"-PackageZip",$releaseZip,"-OutputJson",$portableEvidence,"-RunId",$runId,"-Objdump",$objdump,"-GateName","WindowsPortableExtraction","-ExpectedArtifactSha256",$candidateSha256) $portableEvidence -TimeoutSeconds 300 -Validate {
            Test-PortableRuntimeEvidence $portableEvidence $candidateSha256 $runId
        } | Out-Null
        $guiEvidence = Join-Path $validationDirectory "sdl3-gui.json"
        Invoke-GateProcess "SDL3GUI" powershell.exe @("-NoProfile","-ExecutionPolicy","Bypass","-File",(Join-Path $sourceRoot "tools\test_clean_release.ps1"),"-PackageZip",$releaseZip,"-OutputJson",$guiEvidence,"-RunId",$runId,"-GateName","SDL3GUI","-ExpectedArtifactSha256",$candidateSha256) $guiEvidence -TimeoutSeconds 300 -Validate {
            if (-not (Test-Path -LiteralPath $guiEvidence -PathType Leaf)) { return $false }
            $r = Get-Content -LiteralPath $guiEvidence -Raw | ConvertFrom-Json
            $allInteractions = $r.window_created -eq $true -and $r.frame_rendered -eq $true -and $r.resize -eq $true -and $r.fullscreen_toggle -eq $true -and $r.keyboard -eq $true -and $r.mouse -eq $true -and $r.text_input -eq $true -and $r.clipboard -eq $true -and $r.screenshot_created -eq $true -and $r.clean_shutdown -eq $true -and $r.restart -eq $true
            $screenshotPath = Join-Path $validationDirectory ([string]$r.artifact)
            return ($r.schema -eq "omnipack-release-evidence" -and $r.schema_version -eq 1 -and $r.test -eq "sdl3_gui" -and $r.status -eq "PASS" -and $r.passed -eq $true -and $r.candidate_sha256 -eq $candidateSha256 -and $allInteractions -and (Test-BmpScreenshot $screenshotPath $r.screenshot_width $r.screenshot_height $r.screenshot_bytes))
        } | Out-Null
        $auditOk = Invoke-GateProcess "PackageVerification" $python @((Join-Path $sourceRoot "tools\test_release_audit.py"),$releaseZip,"--version",$version,"--kind",$kind,"--artifact-stem",$artifactStem,"--symbol-artifact-stem",$symbolArtifactStem) (Join-Path $validationDirectory "package-verification") -TimeoutSeconds 300
        $symbolAuditOk = Invoke-GateProcess "SymbolPackageVerification" $python @((Join-Path $sourceRoot "tools\test_release_audit.py"),$symbolsZip,"--symbols","--version",$version,"--artifact-stem",$artifactStem,"--symbol-artifact-stem",$symbolArtifactStem) (Join-Path $validationDirectory "symbol-package-verification") -TimeoutSeconds 300
        $cleanEvidence = Join-Path $validationDirectory "windows-clean-machine.json"
        $cleanMessage = "independent runtime-only Windows job evidence must be imported by the finalizer"
        $cleanStarted = [DateTime]::UtcNow.ToString("o")
        $cleanFinished = [DateTime]::UtcNow.ToString("o")
        Write-ResultJson $cleanEvidence ([ordered]@{schema="omnipack-release-evidence";schema_version=1;test="windows_clean_machine";gate_name="WindowsCleanMachine";run_id=$runId;commit=$commitAtStart;passed=$false;status="NOT_TESTED";reason=$cleanMessage;candidate_filename=[IO.Path]::GetFileName($releaseZip);candidate_sha256=$candidateSha256;symbols_sha256=$symbolArchiveSha256;symbols_member_sha256=$currentSymbolsMemberSha256;gate_started_at=$cleanStarted;gate_finished_at=$cleanFinished;source_checkout_used=$null;candidate_extracted_to_fresh_directory=$false;launch_passed=$false;save_reload_passed=$false;clean_shutdown=$false})
        Set-Gate "WindowsCleanMachine" "NOT_TESTED" "independent runtime-only job" 2 $cleanEvidence $cleanMessage $cleanStarted $cleanFinished
        $hashEvidence = Join-Path $validationDirectory "sha256.json"
        $hashStarted = [DateTime]::UtcNow.ToString("o")
        $sidecars = @("$releaseZip.sha256","$symbolsZip.sha256")
        $hashesPass = $auditOk -and $symbolAuditOk -and @($sidecars | Where-Object { -not (Test-Path $_ -PathType Leaf) }).Count -eq 0
        $hashRecords = @()
        foreach ($zip in @($releaseZip,$symbolsZip)) {
            if (Test-Path $zip) { $hashRecords += [ordered]@{file=[IO.Path]::GetFileName($zip);sha256=(Get-Sha256Hex $zip);size=(Get-Item $zip).Length} }
        }
        $sidecarMatch = $true
        foreach ($zip in @($releaseZip,$symbolsZip)) {
            $sidecar = "$zip.sha256"
            if (-not (Test-Path -LiteralPath $sidecar)) { $sidecarMatch = $false; continue }
            $expectedLine = "$(($hashRecords | Where-Object file -eq ([IO.Path]::GetFileName($zip))).sha256)  $([IO.Path]::GetFileName($zip))"
            if ((Get-Content -LiteralPath $sidecar -Raw).Trim() -ne $expectedLine) { $sidecarMatch = $false }
        }
        $hashesPass = $hashesPass -and $sidecarMatch
        $hashFinished = [DateTime]::UtcNow.ToString("o")
        Write-ResultJson $hashEvidence ([ordered]@{schema="omnipack-release-evidence";schema_version=1;test="candidate_sha256";gate_name="CandidateSHA256";run_id=$runId;commit=$commitAtStart;candidate_sha256=$candidateSha256;symbols_sha256=$symbolArchiveSha256;symbols_member_sha256=$currentSymbolsMemberSha256;gate_started_at=$hashStarted;gate_finished_at=$hashFinished;status=if($hashesPass){"PASS"}else{"FAIL"};passed=$hashesPass;sidecars_match=$sidecarMatch;files=$hashRecords})
        $hashStatus = if($hashesPass){"PASS"}else{"FAIL"}
        $hashExit = if($hashesPass){0}else{1}
        $hashMessage = if($hashesPass){"both ZIPs and sidecars verified"}else{"ZIP or SHA256 verification failed"}
        Set-Gate "CandidateSHA256" $hashStatus "independent ZIP audit and SHA256 recomputation" $hashExit $hashEvidence $hashMessage $hashStarted $hashFinished
        $negativeEvidence = Join-Path $validationDirectory "negative-gate-tests.json"
        Invoke-GateProcess "NegativeGateSuite" $python @(
            (Join-Path $sourceRoot "tools\release_negative_gate_suite.py"),
            "--candidate",$releaseZip,"--symbols",$symbolsZip,
            "--artifact-stem",$artifactStem,"--symbol-artifact-stem",$symbolArtifactStem,
            "--run-id",$runId,"--commit",$commitAtStart,
            "--candidate-sha256",$candidateSha256,
            "--package-version",$version,"--package-kind",$kind,
            "--output",$negativeEvidence
        ) $negativeEvidence -TimeoutSeconds 1200 -Validate {
            if (-not (Test-Path -LiteralPath $negativeEvidence -PathType Leaf)) { return $false }
            $r = Get-Content -LiteralPath $negativeEvidence -Raw | ConvertFrom-Json
            return ($r.schema -eq "omnipack-release-evidence" -and $r.schema_version -eq 1 -and
                $r.test -eq "negative_gate_suite" -and $r.run_id -eq $runId -and
                $r.commit -eq $commitAtStart -and $r.candidate_sha256 -eq $candidateSha256 -and
                $r.status -eq "PASS" -and $r.passed -eq $true -and
                $r.baselines_total -eq $r.baselines_passed -and @($r.baselines_failed).Count -eq 0 -and
                $r.attacks_total -eq $r.attacks_rejected -and @($r.attacks_failed).Count -eq 0)
        } | Out-Null
        $immutableEvidence = Join-Path $validationDirectory "artifact-immutability.json"
        $immutableStarted = [DateTime]::UtcNow.ToString("o")
        $candidateSha256After = Get-Sha256Hex $releaseZip
        $artifactImmutable = $candidateSha256After -eq $candidateSha256
        $immutableFinished = [DateTime]::UtcNow.ToString("o")
        Write-ResultJson $immutableEvidence ([ordered]@{schema="omnipack-release-evidence";schema_version=1;test="artifact_immutability";gate_name="ArtifactImmutability";run_id=$runId;commit=$commitAtStart;candidate_sha256=$candidateSha256;symbols_sha256=$symbolArchiveSha256;symbols_member_sha256=$currentSymbolsMemberSha256;gate_started_at=$immutableStarted;gate_finished_at=$immutableFinished;status=if($artifactImmutable){"PASS"}else{"FAIL"};passed=$artifactImmutable;before_sha256=$candidateSha256;after_sha256=$candidateSha256After})
        Set-Gate "ArtifactImmutability" $(if($artifactImmutable){"PASS"}else{"FAIL"}) "recompute final candidate SHA256 after all package consumers" $(if($artifactImmutable){0}else{1}) $immutableEvidence $(if($artifactImmutable){"all post-package gates consumed one immutable ZIP"}else{"candidate ZIP changed during post-package validation"}) $immutableStarted $immutableFinished
    } else {
        Set-Gate "CandidateSHA256" "FAIL" "not run because package creation failed" -1 (Join-Path $validationDirectory "sha256.json") "package unavailable"
        Set-NotTested "NegativeGateSuite" "package was not created"
        Set-NotTested "ArtifactImmutability" "package was not created"
        Set-NotTested "SDL3GUI" "package was not created"
    }
    Set-SourceSnapshotImmutabilityGate | Out-Null
    $allMandatory = $prePackageMandatory + @("SourceSnapshotImmutability","SDL3GUI","WindowsPortableExtraction","Soak2Hours","PackageManifest","PackageVerification","SymbolPackageVerification","CandidateSHA256","ArtifactImmutability","WindowsCleanMachine","EvidenceSemanticIntegrity","EvidenceHashIntegrity","DocumentationConsistency","NegativeGateSuite")
    $finalBlocked = @($allMandatory | Where-Object { -not $results.Contains($_) -or $results[$_].Status -ne "PASS" })
    $auditInputStatus = if ($finalBlocked.Count -eq 0) { "READY FOR 1.1.0 STABLE" } else { "RC VALIDATION COMPLETE - STABLE BLOCKED" }
    Write-AggregateValidation $validationJson $validationTxt $auditInputStatus $finalBlocked
    if ($packageOk) {
        Invoke-ValidationAuditor $validationJson $validationTxt (Join-Path $validationDirectory "BUILD-INFO.txt") $releaseZip $symbolsZip | Out-Null
    } else {
        Set-NotTested "EvidenceSemanticIntegrity" "package was not created"
        Set-NotTested "EvidenceHashIntegrity" "package was not created"
        Set-NotTested "DocumentationConsistency" "package was not created"
    }
    $finalBlocked = @($allMandatory | Where-Object { -not $results.Contains($_) -or $results[$_].Status -ne "PASS" })
    $postStatus = if ($finalBlocked.Count -eq 0) { "READY FOR 1.1.0 STABLE" } elseif ($Channel -eq "stable") { "NOT READY FOR 1.1.0 STABLE" } else { "RC VALIDATION COMPLETE - STABLE BLOCKED" }
    Write-AggregateValidation $validationJson $validationTxt $postStatus $finalBlocked
    if ($Channel -eq "stable" -and -not $CandidateOnly -and $finalBlocked.Count -gt 0) {
        throw "RELEASE BLOCKED: $($finalBlocked -join ', ')"
    }
    if ($Channel -eq "stable" -and $CandidateOnly) {
        $candidateBlockers = @($finalBlocked | Where-Object { $_ -ne "WindowsCleanMachine" })
        if ($candidateBlockers.Count -gt 0) { throw "CANDIDATE BLOCKED: $($candidateBlockers -join ', ')" }
    }
} catch {
    $failure = $_.Exception.Message
    Write-Error $failure
} finally {
    if ($results.Count -gt 0) {
        $validationJson = Join-Path $validationDirectory "RELEASE-VALIDATION.json"
        $validationTxt = Join-Path $validationDirectory "RELEASE-VALIDATION.txt"
        if ($failure) {
            $blocking = @($results.Keys | Where-Object { $results[$_].Status -ne "PASS" })
            if ($blocking.Count -eq 0) { $blocking = @($failure) }
            Write-AggregateValidation $validationJson $validationTxt "NOT READY FOR 1.1.0 STABLE" $blocking
        }
    }
}
if ($failure) { exit 1 }
if ($Channel -eq "stable" -and $CandidateOnly) {
    Write-Output "release-1.1.0: STAGING_CANDIDATE_READY candidate=$releaseZip validation=$validationJson"
} elseif ($Channel -eq "stable") {
    Write-Output "release-1.1.0: READY_FOR_FINALIZER validation=$validationJson"
} else {
    Write-Output "release-1.1.0: RC_VALIDATION_COMPLETE validation=$validationJson"
}
