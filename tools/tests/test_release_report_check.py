from __future__ import annotations

import importlib.util
from pathlib import Path
import sys
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / "tools" / "release_report_check.py"
SPEC = importlib.util.spec_from_file_location("release_report_contract", SCRIPT)
assert SPEC is not None and SPEC.loader is not None
check = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = check
SPEC.loader.exec_module(check)


class ReleaseReportCheckTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.report_text = (
            ROOT / "dist" / "release-report-0.1.0-test.md"
        ).read_text(encoding="utf-8")

    def test_repository_report_has_required_machine_fields(self) -> None:
        fields = check.parse_report(self.report_text)
        self.assertEqual([], check.validate_values(fields))
        self.assertEqual("false", fields["release_ready"])

    def test_missing_required_field_is_rejected(self) -> None:
        fields = check.parse_report(self.report_text)
        del fields["ops_roundtrip_test"]
        errors = check.validate_values(fields)
        self.assertTrue(any("ops_roundtrip_test" in error for error in errors))

    def test_non_machine_value_is_rejected(self) -> None:
        fields = check.parse_report(self.report_text)
        fields["stress_test"] = "probably"
        errors = check.validate_values(fields)
        self.assertTrue(any("stress_test" in error for error in errors))

    def test_release_ready_cannot_override_failed_gates(self) -> None:
        fields = check.parse_report(self.report_text)
        fields["release_ready"] = "true"
        errors = check.validate_values(fields)
        self.assertTrue(any("release_ready=true conflicts" in error for error in errors))
        conflict = next(error for error in errors if "release_ready=true conflicts" in error)
        self.assertIn("en_gui_test", conflict)
        self.assertIn("readonly_upload_block_test", conflict)

    def test_artifact_hash_mismatch_is_rejected(self) -> None:
        fields = check.parse_report(self.report_text)
        fields["symbols_zip_sha256"] = "A" * 64
        fields["source_zip_sha256"] = "not_tested"
        with tempfile.TemporaryDirectory() as temporary:
            dist = Path(temporary)
            public = dist / check.HASH_ARTIFACTS["public_zip_sha256"].format(
                version=fields["version"]
            )
            symbols = dist / check.HASH_ARTIFACTS["symbols_zip_sha256"].format(
                version=fields["version"]
            )
            public.write_bytes(b"public")
            symbols.write_bytes(b"symbols")
            fields["public_zip_sha256"] = check.sha256(public)
            errors = check.verify_artifacts(fields, dist)
        self.assertTrue(any("symbols_zip_sha256 mismatch" in error for error in errors))


if __name__ == "__main__":
    unittest.main()
