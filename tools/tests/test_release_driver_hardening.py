from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[2]


class ReleaseDriverHardeningTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.script = (ROOT / "tools" / "release_1_1_0.ps1").read_text(encoding="utf-8")

    def test_stable_rejects_existing_build_directory(self) -> None:
        self.assertIn(
            'stable release requires an absent, run-clean build directory',
            self.script,
        )
        self.assertIn('$Channel -eq "stable" -and (Test-Path -LiteralPath $buildDirectory)', self.script)

    def test_release_snapshot_requires_verified_wrap_inputs(self) -> None:
        self.assertIn('"--required-wrap",$releaseBuildInputWrap', self.script)
        self.assertIn('$value.build_inputs_sha256', self.script)
        self.assertIn('$sourceSnapshotAfterConfigure.build_inputs_ready -ne $true', self.script)
        self.assertIn('"--expected-build-inputs-snapshot"', self.script)

    def test_external_json_is_preserved_behind_explicit_adapter(self) -> None:
        self.assertIn('trusted_release_driver_fresh_process_adapter', self.script)
        self.assertIn('producer_evidence_sha256', self.script)
        self.assertIn('$producerEvidence = $sourceEvidence', self.script)
        self.assertNotIn(
            '[IO.File]::WriteAllText($sourceEvidence, ($document | ConvertTo-Json',
            self.script,
        )


if __name__ == "__main__":
    unittest.main()
