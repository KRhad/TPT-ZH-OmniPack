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

    def test_client_process_handles_are_disposed_before_cleanup(self) -> None:
        for wrapper in (self.examples, self.ops):
            self.assertIn("$process.Dispose()", wrapper)
            self.assertIn("$process.WaitForExit()", wrapper)

    def test_temporary_directory_cleanup_retries_without_masking_pass(self) -> None:
        self.assertIn("function Remove-IsolatedRoot", self.examples)
        self.assertIn("function Remove-OpsTestRoot", self.ops)
        for wrapper in (self.examples, self.ops):
            self.assertIn("$attempt -le 20", wrapper)
            self.assertIn("Start-Sleep -Milliseconds 100", wrapper)
            self.assertIn("Write-Warning", wrapper)


if __name__ == "__main__":
    unittest.main()
