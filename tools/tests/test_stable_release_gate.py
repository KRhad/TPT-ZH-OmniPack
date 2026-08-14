from __future__ import annotations

import json
from pathlib import Path
import hashlib
import os
import shutil
import subprocess
import sys
import tempfile
import unittest
import zipfile


ROOT = Path(__file__).resolve().parents[2]


class StableReleaseGateTests(unittest.TestCase):
    def test_official_save_gate_fails_closed_without_corpus(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            output = root / "official.json"
            completed = subprocess.run([
                sys.executable, str(ROOT / "tools/official_save_compatibility.py"),
                "--corpus", str(root / "missing"), "--probe", str(root / "missing.exe"),
                "--provenance-manifest", str(root / "missing" / "provenance.json"),
                "--output", str(output),
            ], check=False)
            self.assertNotEqual(completed.returncode, 0)
            result = json.loads(output.read_text(encoding="utf-8"))
            self.assertFalse(result["passed"])
            self.assertEqual(result["status"], "NOT_TESTED")

    def test_official_save_gate_requires_hash_bound_provenance(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            corpus = root / "corpus"
            corpus.mkdir()
            fixture = corpus / "official.cps"
            fixture.write_bytes(b"not-a-real-save")
            revision = "a" * 40
            manifest = corpus / "provenance.json"
            manifest.write_text(json.dumps({
                "schema": "omnipack-official-tpt-save-corpus-v1",
                "corpus_id": "test-corpus",
                "source_repository": "https://github.com/The-Powder-Toy/The-Powder-Toy",
                "source_revision": revision,
                "retrieved_at": "2026-08-14",
                "redistribution": {
                    "status": "local_only_not_for_redistribution",
                    "basis": "test-only local fixture",
                },
                "files": [{
                    "path": fixture.name,
                    "sha256": "0" * 64,
                    "source_locator": f"https://github.com/The-Powder-Toy/The-Powder-Toy/blob/{revision}/tests/official.cps",
                }],
            }), encoding="utf-8")
            output = root / "official.json"
            completed = subprocess.run([
                sys.executable, str(ROOT / "tools/official_save_compatibility.py"),
                "--corpus", str(corpus), "--provenance-manifest", str(manifest),
                "--probe", str(root / "missing.exe"), "--output", str(output),
            ], check=False)
            self.assertNotEqual(completed.returncode, 0)
            result = json.loads(output.read_text(encoding="utf-8"))
            self.assertEqual(result["status"], "FAIL")
            self.assertFalse(result["provenance_valid"])
            self.assertIn("SHA-256 mismatch", result["reason"])

    def test_official_save_gate_accepts_valid_manifest_before_probe(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            corpus = root / "corpus"
            corpus.mkdir()
            fixture = corpus / "official.cps"
            fixture.write_bytes(b"not-a-real-save")
            revision = "b" * 40
            manifest = corpus / "provenance.json"
            manifest.write_text(json.dumps({
                "schema": "omnipack-official-tpt-save-corpus-v1",
                "corpus_id": "test-corpus",
                "source_repository": "https://github.com/The-Powder-Toy/The-Powder-Toy",
                "source_revision": revision,
                "retrieved_at": "2026-08-14",
                "redistribution": {
                    "status": "local_only_not_for_redistribution",
                    "basis": "test-only local fixture",
                },
                "files": [{
                    "path": fixture.name,
                    "sha256": hashlib.sha256(fixture.read_bytes()).hexdigest(),
                    "source_locator": f"https://raw.githubusercontent.com/The-Powder-Toy/The-Powder-Toy/{revision}/tests/official.cps",
                }],
            }), encoding="utf-8")
            output = root / "official.json"
            completed = subprocess.run([
                sys.executable, str(ROOT / "tools/official_save_compatibility.py"),
                "--corpus", str(corpus), "--provenance-manifest", str(manifest),
                "--probe", str(root / "missing.exe"), "--output", str(output),
            ], check=False)
            self.assertNotEqual(completed.returncode, 0)
            result = json.loads(output.read_text(encoding="utf-8"))
            self.assertEqual(result["status"], "FAIL")
            self.assertTrue(result["provenance_valid"])
            self.assertEqual(result["reason"], "compatibility probe is absent")

    def test_release_script_is_fail_closed_and_evidence_bound(self) -> None:
        script = (ROOT / "tools/release_1_1_0.ps1").read_text(encoding="utf-8")
        self.assertIn("Start-Process", script)
        self.assertIn("ConvertFrom-Json", script)
        self.assertIn('Status -ne "PASS"', script)
        self.assertIn('"OfficialTPTSaveCompatibility"', script)
        self.assertIn('"WindowsCleanMachine"', script)
        self.assertIn('"Soak2Hours"', script)
        self.assertNotIn("continue-on-error", script)
        self.assertNotIn("--force-pass", script)
        self.assertIn("[string] $OfficialSaveCorpus", script)
        self.assertIn('"--provenance-manifest"', script)
        self.assertIn('"Channel: $Channel"', script)

    def test_gpu_validation_unsupported_cannot_exit_zero(self) -> None:
        source = (ROOT / "src/common/platform/SDLGPU.cpp").read_text(encoding="utf-8")
        self.assertIn("return kShaderUnavailable", source)
        self.assertIn("return kGPUDeviceCreationFailure", source)
        self.assertIn("WriteValidationJson", source)
        self.assertIn("gpu_validation_passed", source)

    def test_release_gate_records_source_and_evidence_integrity(self) -> None:
        script = (ROOT / "tools/release_1_1_0.ps1").read_text(encoding="utf-8")
        self.assertIn('"SourceTreeClean"', script)
        self.assertIn("EvidenceSha256", script)
        self.assertIn("EvidenceIntegrity", script)
        self.assertIn("DocumentationConsistency", script)
        self.assertIn("WindowsPortableExtraction", script)
        self.assertIn('OMNI_CLEAN_MACHINE=true', (ROOT / "tools/test_clean_release.ps1").read_text(encoding="utf-8"))

    def test_clean_machine_branch_is_marker_gated_and_executes_runner(self) -> None:
        script = (ROOT / "tools/release_1_1_0.ps1").read_text(encoding="utf-8")
        self.assertIn('if ($env:OMNI_CLEAN_MACHINE -eq "true")', script)
        self.assertIn('Invoke-GateProcess "WindowsCleanMachine" powershell.exe', script)
        self.assertIn('"-ExpectedArtifactSha256",$candidateSha256', script)
        self.assertIn('"ArtifactImmutability"', script)
        self.assertIn("Post-package gates must only read", script)
        self.assertNotIn('Invoke-GateProcess "FinalPackage"', script)
        self.assertNotIn('Invoke-GateProcess "FinalPackageVerification"', script)

    def test_clean_machine_runner_rejects_missing_marker_and_does_not_auto_pass(self) -> None:
        powershell = shutil.which("powershell.exe") or shutil.which("pwsh")
        if not powershell:
            self.skipTest("PowerShell is unavailable")
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            package = root / "empty.zip"
            with zipfile.ZipFile(package, "w"):
                pass
            script = ROOT / "tools/test_clean_release.ps1"
            output_without = root / "without-marker.json"
            env_without = os.environ.copy()
            env_without.pop("OMNI_CLEAN_MACHINE", None)
            without = subprocess.run([
                powershell, "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", str(script),
                "-PackageZip", str(package), "-OutputJson", str(output_without),
                "-GateName", "WindowsCleanMachine",
            ], check=False, env=env_without)
            self.assertNotEqual(without.returncode, 0)
            without_result = json.loads(output_without.read_text(encoding="utf-8-sig"))
            self.assertIn("requires OMNI_CLEAN_MACHINE=true", without_result["reason"])

            output_with = root / "with-marker.json"
            env_with = os.environ.copy()
            env_with["OMNI_CLEAN_MACHINE"] = "true"
            with_marker = subprocess.run([
                powershell, "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", str(script),
                "-PackageZip", str(package), "-OutputJson", str(output_with),
                "-GateName", "WindowsCleanMachine",
            ], check=False, env=env_with)
            self.assertNotEqual(with_marker.returncode, 0)
            with_result = json.loads(output_with.read_text(encoding="utf-8-sig"))
            self.assertNotIn("requires OMNI_CLEAN_MACHINE=true", with_result["reason"])
            self.assertFalse(with_result["passed"])

    def test_release_scripts_do_not_depend_on_optional_get_file_hash_cmdlet(self) -> None:
        for name in ("release_1_1_0.ps1", "test_clean_release.ps1", "runtime_stress_test.ps1"):
            source = (ROOT / "tools" / name).read_text(encoding="utf-8")
            self.assertIn("function Get-Sha256Hex", source)
            self.assertNotIn("Get-FileHash", source)

    def test_evidence_tampering_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            evidence = root / "gate.json"
            evidence.write_text('{"passed":true}\n', encoding="utf-8")
            import hashlib
            digest = hashlib.sha256(evidence.read_bytes()).hexdigest().upper()
            validation = root / "RELEASE-VALIDATION.json"
            validation.write_text(json.dumps({
                "gates": {"Build": {"Status": "PASS", "Evidence": "gate.json", "EvidenceSha256": digest}}
            }), encoding="utf-8")
            evidence.write_text('{"passed":false}\n', encoding="utf-8")
            output = root / "audit.json"
            completed = subprocess.run([
                sys.executable, str(ROOT / "tools/release_validation_audit.py"),
                "--validation-json", str(validation), "--channel", "rc",
                "--output", str(output),
            ], check=False)
            self.assertNotEqual(completed.returncode, 0)
            result = json.loads(output.read_text(encoding="utf-8"))
            self.assertFalse(result["evidence_integrity"])


if __name__ == "__main__":
    unittest.main()
