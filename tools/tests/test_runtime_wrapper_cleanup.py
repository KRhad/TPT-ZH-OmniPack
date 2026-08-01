from __future__ import annotations

from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[2]


class RuntimeWrapperCleanupContractTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.examples = (ROOT / "tools" / "runtime_lua_examples_test.ps1").read_text(
            encoding="utf-8"
        )
        cls.ops = (ROOT / "tools" / "runtime_lua_ops_roundtrip_test.ps1").read_text(
            encoding="utf-8"
        )
        cls.automation = (
            ROOT / "tools" / "runtime_lua_automation_test.ps1"
        ).read_text(encoding="utf-8")

    def test_client_process_handles_are_disposed_before_cleanup(self) -> None:
        for wrapper in (self.examples, self.ops, self.automation):
            self.assertIn("$process.Dispose()", wrapper)
            self.assertIn("$process.WaitForExit()", wrapper)

    def test_temporary_directory_cleanup_retries_without_masking_pass(self) -> None:
        self.assertIn("function Remove-IsolatedRoot", self.examples)
        self.assertIn("function Remove-OpsTestRoot", self.ops)
        self.assertIn("function Remove-IsolatedRoot", self.automation)
        for wrapper in (self.examples, self.ops, self.automation):
            self.assertIn("$attempt -le 20", wrapper)
            self.assertIn("Start-Sleep -Milliseconds 100", wrapper)
            self.assertIn("Write-Warning", wrapper)

    def test_examples_default_directory_stays_inside_source_root(self) -> None:
        self.assertIn('"..\\examples\\0.2.0"', self.examples)
        self.assertNotIn('"..\\..\\examples\\0.2.0"', self.examples)

    def test_repository_evidence_is_read_only_by_default(self) -> None:
        for wrapper in (self.examples, self.automation):
            self.assertIn('[string] $Mode = "Verify"', wrapper)
            self.assertIn('[switch] $UpdateSourceArtifacts', wrapper)
            self.assertIn('$Mode -in @("Generate", "All")', wrapper)
            self.assertIn('-not $UpdateSourceArtifacts', wrapper)
            self.assertIn('Refusing to replace repository', wrapper)


if __name__ == "__main__":
    unittest.main()
