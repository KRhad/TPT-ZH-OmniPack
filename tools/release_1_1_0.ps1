param(
    [ValidateSet("rc", "stable")]
    [string] $Channel = "rc",
    [string] $BuildDirectory = "build-release-1.1.0",
    [string] $OutputDirectory = "dist\1.1.0",
    [string] $OfficialSaveCorpus = "",
    [switch] $SkipLongSoak,
    [switch] $AllowDirtyValidation
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
$version = if ($Channel -eq "stable") { "1.1.0" } else { "1.1.0-rc1" }
$kind = if ($Channel -eq "stable") { "release" } else { "release-candidate" }
$buildDirectory = if ([IO.Path]::IsPathRooted($BuildDirectory)) { $BuildDirectory } else { Join-Path $sourceRoot $BuildDirectory }
$outputDirectory = if ([IO.Path]::IsPathRooted($OutputDirectory)) { $OutputDirectory } else { Join-Path $sourceRoot $OutputDirectory }
$validationDirectory = Join-Path $outputDirectory "validation-$version"
$releaseDirectory = Join-Path $outputDirectory "release"
$symbolsDirectory = Join-Path $outputDirectory "symbols"
$releaseZip = Join-Path $outputDirectory "TPT-ZH-OmniPack-$version-Windows-x64-SDL3.zip"
$symbolsZip = Join-Path $outputDirectory "TPT-ZH-OmniPack-$version-Windows-x64-Symbols.zip"
$packageOk = $false
New-Item -ItemType Directory -Force $validationDirectory,$releaseDirectory,$symbolsDirectory | Out-Null

$results = [ordered]@{}
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
function Find-EvidenceFile {
    param([string]$Path)
    foreach ($candidate in @($Path, "$Path.json", "$Path.stdout.txt")) {
        if (Test-Path -LiteralPath $candidate -PathType Leaf) { return (Resolve-Path -LiteralPath $candidate).Path }
    }
    return $null
}
function Set-Gate {
    param([string]$Name,[string]$Status,[string]$Command,[int]$ExitCode,[string]$Evidence,[string]$Message)
    $evidenceFile = Find-EvidenceFile $Evidence
    $evidenceRelative = if ($evidenceFile) { Get-RelativeEvidencePath $evidenceFile } else { Get-RelativeEvidencePath $Evidence }
    $evidenceHash = if ($evidenceFile) { Get-Sha256Hex $evidenceFile } else { $null }
    $results[$Name] = [ordered]@{
        Name=$Name; Status=$Status; Command=$Command; ExitCode=$ExitCode
        Evidence=$evidenceRelative; EvidenceSha256=$evidenceHash; Message=$Message
    }
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
    param([string]$Name,[string]$FilePath,[string[]]$Arguments,[string]$Evidence,[int]$TimeoutSeconds=600,[scriptblock]$Validate)
    $stdout = "$Evidence.stdout.txt"; $stderr = "$Evidence.stderr.txt"
    $command = ($FilePath + " " + ($Arguments -join " "))
    try {
        $exit = Invoke-ProcessCapture $FilePath $Arguments $stdout $stderr $TimeoutSeconds
        $ok = ($exit -eq 0)
        $stderrText = if (Test-Path -LiteralPath $stderr) { Get-Content $stderr -Raw -ErrorAction SilentlyContinue } else { "" }
        $message = if ($ok) { "process exited 0" } else { if ($stderrText) { $stderrText.Trim() } else { "process exited $exit" } }
        if ($Validate) { $ok = $ok -and (& $Validate) }
        $status = if ($ok) { "PASS" } else { "FAIL" }
        Set-Gate $Name $status $command $exit $Evidence $message
        return $ok
    } catch {
        Set-Gate $Name "FAIL" $command -1 $Evidence $_.Exception.Message
        return $false
    }
}
function Set-NotTested {
    param([string]$Name,[string]$Reason)
    $evidence = Join-Path $validationDirectory ($Name.ToLowerInvariant() + ".json")
    Write-ResultJson $evidence ([ordered]@{test=$Name; passed=$false; status="NOT_TESTED"; reason=$Reason})
    Set-Gate $Name "NOT_TESTED" "not executed" -1 $evidence $Reason
}
function Invoke-SoakGate {
    param([string]$Executable,[string]$PackageZip)
    $soakDirectory = Join-Path $validationDirectory "soak"
    $soakHarness = Join-Path $sourceRoot "tools\runtime_stress_test.ps1"
    $soakStdout = Join-Path $validationDirectory "soak-harness.stdout.txt"
    $soakStderr = Join-Path $validationDirectory "soak-harness.stderr.txt"
    $soakCommand = "runtime_stress_test.ps1 -LongRun -SampleId S20-FULL-CATALOG -PackageZip <current-candidate>"
    try {
        & powershell.exe -NoProfile -ExecutionPolicy Bypass -File $soakHarness -Executable $Executable -SampleId S20-FULL-CATALOG -LongRun -PackageZip $PackageZip -PackageVersion $version -OutputDirectory $soakDirectory *> $soakStdout
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
        $passed = $analysisExit -eq 0 -and $r -and $r.long_run_gate_pass -eq $true -and $r.performance_gate_pass -eq $true -and $r.public_zip_sha256 -eq $candidateHash -and [double]$r.wall_clock_seconds -ge 7200.0
        $soakStatus = if($passed){"PASS"}else{"FAIL"}
        $soakMessage = if($passed){"7200-second wall-clock soak passed against current candidate ZIP"}else{"soak evidence failed or artifact SHA does not match current candidate"}
        Set-Gate "Soak2Hours" $soakStatus $soakCommand $analysisExit $assessment $soakMessage
    } catch {
        $evidence = Join-Path $validationDirectory "soak-2h.json"
        Write-ResultJson $evidence ([ordered]@{test="soak_2h";passed=$false;status="FAIL";reason=$_.Exception.Message})
        Set-Gate "Soak2Hours" "FAIL" $soakCommand -1 $evidence $_.Exception.Message
    }
}
function Invoke-ValidationAuditor {
    param([string]$ValidationJson,[string]$ValidationText,[string]$BuildInfo,[string]$Package)
    $auditEvidence = Join-Path $validationDirectory "evidence-integrity.json"
    $auditArgs = @((Join-Path $sourceRoot "tools\release_validation_audit.py"),"--validation-json",$ValidationJson,"--validation-text",$ValidationText,"--build-info",$BuildInfo,"--channel",$Channel,"--output",$auditEvidence)
    if ($Package -and (Test-Path -LiteralPath $Package)) { $auditArgs += @("--package",$Package) }
    $ok = Invoke-GateProcess "EvidenceIntegrity" $python $auditArgs $auditEvidence -TimeoutSeconds 120 -Validate {
        if (-not (Test-Path -LiteralPath $auditEvidence)) { return $false }
        $r = Get-Content $auditEvidence -Raw | ConvertFrom-Json
        return ($r.test -eq "release_validation_audit" -and $r.evidence_integrity -eq $true -and $r.passed -eq $true)
    }
    $docResult = if (Test-Path -LiteralPath $auditEvidence) { Get-Content $auditEvidence -Raw | ConvertFrom-Json } else { $null }
    $docOk = $ok -and $docResult -and $docResult.documentation_consistency -eq $true
    $docStatus = if ($docOk) { "PASS" } else { "FAIL" }
    $docMessage = if ($docOk) { "validation JSON, text, build info, and package agree" } else { "documentation consistency audit failed" }
    Set-Gate "DocumentationConsistency" $docStatus "release_validation_audit.py --channel $Channel" $(if($docOk){0}else{1}) $auditEvidence $docMessage
    return $ok
}
function Write-AggregateValidation {
    param([string]$JsonPath,[string]$TextPath,[string]$Status,[string[]]$Blocking,[string]$Scope="complete")
    $commit = (& $git -C $sourceRoot rev-parse HEAD).Trim()
    $postPackageEvidence = if ($Scope -eq "pre_package") { "external_sidecar_required" } else { "included_in_this_document" }
    $aggregate = [ordered]@{version=$version;channel=$Channel;commit=$commit;validation_scope=$Scope;post_package_evidence=$postPackageEvidence;status=$Status;final_status=$Status;gates=$results;blocking_items=$Blocking}
    Write-ResultJson $JsonPath $aggregate
    $lines = @("TPT-ZH OmniPack $version Release Validation","","Commit: $commit","Channel: $Channel","")
    foreach ($item in $results.Values) { $lines += "$($item.Name): $($item.Status) | exit=$($item.ExitCode) | evidence=$($item.Evidence)" }
    $lines += "", "FINAL STATUS: $Status"
    [IO.File]::WriteAllLines($TextPath,$lines,[Text.UTF8Encoding]::new($false))
}

$dirty = @(& $git -C $sourceRoot status --porcelain=v1 --untracked-files=all 2>$null | Where-Object { $_.Trim() })
    $commitAtStart = (& $git -C $sourceRoot rev-parse HEAD).Trim()
$branchAtStart = (& $git -C $sourceRoot branch --show-current).Trim()
$sourceEvidence = Join-Path $validationDirectory "source-tree-clean.json"
    Write-ResultJson $sourceEvidence ([ordered]@{
    test="source_tree_clean"; passed=($dirty.Count -eq 0); status=if($dirty.Count -eq 0){"PASS"}else{"NOT_TESTED"}
    commit=$commitAtStart; branch=$branchAtStart; porcelain_output=($dirty -join "`n")
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
    Invoke-GateProcess "Configure" $meson $setupArgs (Join-Path $validationDirectory "configure") | Out-Null
    $buildOk = Invoke-GateProcess "Build" $ninja @("-C",$buildDirectory) (Join-Path $validationDirectory "build")
    $exe = Join-Path $buildDirectory "tpt-zh-omnipack.exe"
    if ($buildOk) {
        Invoke-GateProcess "UnitTests" $meson @("test","-C",$buildDirectory,"--suite","static","--print-errorlogs","-t","4") (Join-Path $validationDirectory "unit-tests") -TimeoutSeconds 1200 | Out-Null
        Invoke-GateProcess "AtmosphereBench" $meson @("test","-C",$buildDirectory,"--suite","static","--print-errorlogs","--no-rebuild","atmospherebench-contract") (Join-Path $validationDirectory "atmosphere-bench") | Out-Null
        Invoke-GateProcess "MassConservation" $meson @("test","-C",$buildDirectory,"--suite","static","--print-errorlogs","--no-rebuild","omni-atmosphere-cpu-mvp","omni-atmosphere-multispecies") (Join-Path $validationDirectory "mass-conservation") | Out-Null
        Invoke-GateProcess "OmniSaveReload" $meson @("test","-C",$buildDirectory,"--suite","static","--print-errorlogs","--no-rebuild","omni-save-roundtrip-probe") (Join-Path $validationDirectory "omni-save-roundtrip") -TimeoutSeconds 300 | Out-Null

        $gpuJson = Join-Path $validationDirectory "gpu-validation.json"
        $gpuOk = Invoke-GateProcess "GPUNumericalValidation" $exe @("--gpu-validate","--gpu-validate-json",$gpuJson) $gpuJson -TimeoutSeconds 300 -Validate {
            if (-not (Test-Path $gpuJson)) { return $false }
            $r = Get-Content $gpuJson -Raw | ConvertFrom-Json
            return ($r.test -eq "gpu_validation" -and $r.passed -eq $true -and $r.supported -eq $true)
        }
        $probe = Join-Path $validationDirectory "gpu-probe.txt"
        Invoke-GateProcess "SDLGPU" $exe @("--gpu-probe") $probe -TimeoutSeconds 300 | Out-Null
        $fallbackJson = Join-Path $validationDirectory "cpu-fallback.json"
        Invoke-GateProcess "CPUFallback" $exe @("--cpu-fallback-validate","--cpu-fallback-json",$fallbackJson) $fallbackJson -TimeoutSeconds 300 -Validate {
            if (-not (Test-Path $fallbackJson)) { return $false }
            $r = Get-Content $fallbackJson -Raw | ConvertFrom-Json
            return ($r.test -eq "cpu_fallback" -and $r.passed -eq $true -and $r.fallback -eq $true)
        } | Out-Null
        $guiJson = Join-Path $validationDirectory "sdl3-gui.json"
        Invoke-GateProcess "SDL3GUI" $exe @("--gui-smoke-test","--gui-smoke-json",$guiJson) $guiJson -TimeoutSeconds 120 -Validate {
            if (-not (Test-Path $guiJson)) { return $false }
            $r = Get-Content $guiJson -Raw | ConvertFrom-Json
            return ($r.test -eq "sdl3_gui" -and $r.passed -eq $true -and (Test-Path (Join-Path $validationDirectory "gui-smoke.bmp")))
        } | Out-Null
    }

    $rawExe = Join-Path $buildDirectory "tpt-zh-omnipack.exe"
    $releaseExe = Join-Path $releaseDirectory "tpt-zh-omnipack.exe"
    $symbolFile = Join-Path $symbolsDirectory "tpt-zh-omnipack.debug"
    Invoke-GateProcess "DebugSymbolsSeparated" $python @((Join-Path $sourceRoot "tools\prepare_windows_release.py"),"--raw-executable",$rawExe,"--executable",$releaseExe,"--symbols",$symbolFile,"--objcopy",$objcopy,"--strip",$strip) (Join-Path $validationDirectory "symbols") -TimeoutSeconds 300 | Out-Null
    Invoke-GateProcess "ExecutableStripped" $python @((Join-Path $sourceRoot "tools\release_binary_audit.py"),"--executable",$releaseExe,"--symbols",$symbolFile,"--objdump",$objdump,"--strings",$strings) (Join-Path $validationDirectory "binary") -TimeoutSeconds 300 | Out-Null

    $officialCorpus = if ($OfficialSaveCorpus) {
        if ([IO.Path]::IsPathRooted($OfficialSaveCorpus)) { [IO.Path]::GetFullPath($OfficialSaveCorpus) } else { Join-Path $sourceRoot $OfficialSaveCorpus }
    } else {
        Join-Path $sourceRoot "tests\saves\official-tpt"
    }
    $officialManifest = Join-Path $officialCorpus "provenance.json"
    $officialEvidence = Join-Path $validationDirectory "official-tpt-save.json"
    $officialProbe = Join-Path $buildDirectory "official_save_roundtrip_probe.exe"
    $officialArgs = @((Join-Path $sourceRoot "tools\official_save_compatibility.py"),"--corpus",$officialCorpus,"--provenance-manifest",$officialManifest,"--probe",$officialProbe,"--output",$officialEvidence)
    $officialOk = Invoke-GateProcess "OfficialTPTSaveCompatibility" $python $officialArgs $officialEvidence -TimeoutSeconds 1200 -Validate {
        if (-not (Test-Path $officialEvidence)) { return $false }
        $r = Get-Content $officialEvidence -Raw | ConvertFrom-Json
        return ($r.test -eq "official_tpt_save_compatibility" -and $r.passed -eq $true -and $r.provenance_valid -eq $true -and $r.provenance_manifest_sha256 -match '^[0-9A-F]{64}$' -and $r.files_tested -gt 0 -and $r.files_failed -eq 0)
    }
    $results["OfficialTPTSaveCompatibility"].Command = "official_save_compatibility.py --corpus <official-corpus> --provenance-manifest <official-corpus>/provenance.json --probe <fresh-build-probe>"
    if (-not $officialOk -and (Test-Path $officialEvidence)) {
        $officialResult = Get-Content $officialEvidence -Raw | ConvertFrom-Json
        if ($officialResult.status -eq "NOT_TESTED") { $results["OfficialTPTSaveCompatibility"].Status = "NOT_TESTED" }
    }
    $prePackageMandatory = @("SourceTreeClean","Configure","Build","UnitTests","AtmosphereBench","MassConservation","OmniSaveReload","GPUNumericalValidation","CPUFallback","SDL3GUI","ExecutableStripped","DebugSymbolsSeparated","OfficialTPTSaveCompatibility")
    $blocked = @($prePackageMandatory | Where-Object { -not $results.Contains($_) -or $results[$_].Status -ne "PASS" })
    $finalStatus = if ($blocked.Count -eq 0) { "PRE-PACKAGE VALIDATION COMPLETE - POST-PACKAGE GATES PENDING" } elseif ($Channel -eq "stable") { "NOT READY FOR 1.1.0 STABLE" } else { "RC VALIDATION COMPLETE - STABLE BLOCKED" }
    $validationJson = Join-Path $validationDirectory "RELEASE-VALIDATION.json"
    $validationTxt = Join-Path $validationDirectory "RELEASE-VALIDATION.txt"
    Write-AggregateValidation $validationJson $validationTxt $finalStatus $blocked "pre_package"
    # A stable name is reserved for a fully passing pre-package gate. RC may
    # still produce an auditable candidate so missing external evidence remains
    # visible without ever being relabeled as stable.
    if ($Channel -eq "stable" -and $blocked.Count -gt 0) {
        foreach ($name in @("PackageManifest","PackageVerification","SymbolPackageVerification","WindowsPortableExtraction","WindowsCleanMachine","SHA256","ArtifactImmutability","EvidenceIntegrity","DocumentationConsistency")) {
            Set-NotTested $name "stable package was not created because pre-package mandatory gates are blocked"
        }
        throw "RELEASE BLOCKED: $($blocked -join ', ')"
    }

    $buildInfo = Join-Path $validationDirectory "BUILD-INFO.txt"
    $commit = (& $git -C $sourceRoot rev-parse HEAD).Trim()
    [IO.File]::WriteAllLines($buildInfo,@("TPT-ZH OmniPack $version","Git commit: $commit","Channel: $Channel","Build type: Release with detached DWARF symbols","SDL: 3.4.14","Platform: Windows x64","Compute backend: SDL_GPU Vulkan; CPU fallback; CUDA future optional"),[Text.UTF8Encoding]::new($false))
    $packageArgs = @((Join-Path $sourceRoot "tools\package_test_release.py"),"--source-root",$sourceRoot,"--executable",$releaseExe,"--symbols",$symbolFile,"--output-directory",$outputDirectory,"--version",$version,"--kind",$kind,"--objdump",$objdump,"--strings",$strings,"--extra-member",$buildInfo,"BUILD-INFO.txt","--extra-member",$validationTxt,"RELEASE-VALIDATION.txt","--extra-member",$validationJson,"RELEASE-VALIDATION.json")
    if ($Channel -eq "rc" -and $AllowDirtyValidation) { $packageArgs += "--allow-dirty-validation" }
    $packageOk = Invoke-GateProcess "PackageManifest" $python $packageArgs (Join-Path $validationDirectory "package") -TimeoutSeconds 600
    if ($packageOk) {
        # This archive is the final candidate. Post-package gates must only read
        # it so soak, portable, clean-machine, and hash evidence bind one ZIP.
        $candidateSha256 = Get-Sha256Hex $releaseZip
        if ($SkipLongSoak) {
            Set-NotTested "Soak2Hours" "explicitly skipped"
        } else {
            Invoke-SoakGate $releaseExe $releaseZip
        }
        $portableEvidence = Join-Path $validationDirectory "windows-portable.json"
        Invoke-GateProcess "WindowsPortableExtraction" powershell.exe @("-NoProfile","-ExecutionPolicy","Bypass","-File",(Join-Path $sourceRoot "tools\test_clean_release.ps1"),"-PackageZip",$releaseZip,"-OutputJson",$portableEvidence,"-Objdump",$objdump,"-GateName","WindowsPortableExtraction","-ExpectedArtifactSha256",$candidateSha256) $portableEvidence -TimeoutSeconds 300 | Out-Null
        $auditOk = Invoke-GateProcess "PackageVerification" $python @((Join-Path $sourceRoot "tools\test_release_audit.py"),$releaseZip,"--version",$version,"--kind",$kind) (Join-Path $validationDirectory "package-verification") -TimeoutSeconds 300
        $symbolAuditOk = Invoke-GateProcess "SymbolPackageVerification" $python @((Join-Path $sourceRoot "tools\test_release_audit.py"),$symbolsZip,"--symbols","--version",$version) (Join-Path $validationDirectory "symbol-package-verification") -TimeoutSeconds 300
        $cleanEvidence = Join-Path $validationDirectory "windows-clean-machine.json"
        $cleanScript = Join-Path $sourceRoot "tools\test_clean_release.ps1"
        if ($env:OMNI_CLEAN_MACHINE -eq "true") {
            Invoke-GateProcess "WindowsCleanMachine" powershell.exe @("-NoProfile","-ExecutionPolicy","Bypass","-File",$cleanScript,"-PackageZip",$releaseZip,"-OutputJson",$cleanEvidence,"-Objdump",$objdump,"-GateName","WindowsCleanMachine","-ExpectedArtifactSha256",$candidateSha256) $cleanEvidence -TimeoutSeconds 300 -Validate {
                if (-not (Test-Path -LiteralPath $cleanEvidence)) { return $false }
                $r = Get-Content -LiteralPath $cleanEvidence -Raw | ConvertFrom-Json
                return ($r.test -eq "windows_clean_machine" -and $r.passed -eq $true -and $r.status -eq "PASS" -and $r.artifact_sha256 -eq $candidateSha256 -and $r.environment -eq "clean-machine" -and $r.source_tree_present -eq $false -and $r.msys2_present -eq $false -and $r.developer_environment_present -eq $false -and $r.launch -eq $true -and $r.save_reload -eq $true)
            } | Out-Null
        } else {
            $cleanMessage = "OMNI_CLEAN_MACHINE=true is absent; clean-machine validation was not executed"
            Write-ResultJson $cleanEvidence ([ordered]@{test="windows_clean_machine";passed=$false;status="NOT_TESTED";reason=$cleanMessage;artifact_sha256=$candidateSha256;environment="not_tested";source_tree_present=$null;msys2_present=$null;developer_environment_present=$null;launch=$false;save_reload=$false})
            Set-Gate "WindowsCleanMachine" "NOT_TESTED" "test_clean_release.ps1 -GateName WindowsCleanMachine" 2 $cleanEvidence $cleanMessage
        }
        $hashEvidence = Join-Path $validationDirectory "sha256.json"
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
        Write-ResultJson $hashEvidence ([ordered]@{test="package_sha256";passed=$hashesPass;sidecars_match=$sidecarMatch;files=$hashRecords})
        $hashStatus = if($hashesPass){"PASS"}else{"FAIL"}
        $hashExit = if($hashesPass){0}else{1}
        $hashMessage = if($hashesPass){"both ZIPs and sidecars verified"}else{"ZIP or SHA256 verification failed"}
        Set-Gate "SHA256" $hashStatus "independent ZIP audit and SHA256 recomputation" $hashExit $hashEvidence $hashMessage
        $immutableEvidence = Join-Path $validationDirectory "artifact-immutability.json"
        $candidateSha256After = Get-Sha256Hex $releaseZip
        $artifactImmutable = $candidateSha256After -eq $candidateSha256
        Write-ResultJson $immutableEvidence ([ordered]@{test="artifact_immutability";passed=$artifactImmutable;before_sha256=$candidateSha256;after_sha256=$candidateSha256After})
        Set-Gate "ArtifactImmutability" $(if($artifactImmutable){"PASS"}else{"FAIL"}) "recompute final candidate SHA256 after all package consumers" $(if($artifactImmutable){0}else{1}) $immutableEvidence $(if($artifactImmutable){"all post-package gates consumed one immutable ZIP"}else{"candidate ZIP changed during post-package validation"})
    } else {
        Set-Gate "SHA256" "FAIL" "not run because package creation failed" -1 (Join-Path $validationDirectory "sha256.json") "package unavailable"
        Set-NotTested "ArtifactImmutability" "package was not created"
    }
    $allMandatory = $prePackageMandatory + @("Soak2Hours","PackageManifest","PackageVerification","SymbolPackageVerification","WindowsPortableExtraction","WindowsCleanMachine","SHA256","ArtifactImmutability","EvidenceIntegrity","DocumentationConsistency")
    $finalBlocked = @($allMandatory | Where-Object { -not $results.Contains($_) -or $results[$_].Status -ne "PASS" })
    $auditInputStatus = if ($finalBlocked.Count -eq 0) { "READY FOR 1.1.0 STABLE" } else { "RC VALIDATION COMPLETE - STABLE BLOCKED" }
    Write-AggregateValidation $validationJson $validationTxt $auditInputStatus $finalBlocked
    if ($packageOk) {
        Invoke-ValidationAuditor $validationJson $validationTxt (Join-Path $validationDirectory "BUILD-INFO.txt") $releaseZip | Out-Null
    } else {
        Set-NotTested "EvidenceIntegrity" "package was not created"
        Set-NotTested "DocumentationConsistency" "package was not created"
    }
    $finalBlocked = @($allMandatory | Where-Object { -not $results.Contains($_) -or $results[$_].Status -ne "PASS" })
    $postStatus = if ($finalBlocked.Count -eq 0) { "READY FOR 1.1.0 STABLE" } elseif ($Channel -eq "stable") { "NOT READY FOR 1.1.0 STABLE" } else { "RC VALIDATION COMPLETE - STABLE BLOCKED" }
    Write-AggregateValidation $validationJson $validationTxt $postStatus $finalBlocked
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
Write-Output "release-1.1.0: PASS $validationJson"
