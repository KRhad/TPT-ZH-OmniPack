from __future__ import annotations

import csv
import importlib.util
from pathlib import Path
import sys
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / "tools" / "third_party_license_audit.py"


def import_script():
    spec = importlib.util.spec_from_file_location("third_party_license_audit", SCRIPT)
    assert spec is not None and spec.loader is not None
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


audit = import_script()


class ThirdPartyLicenseAuditTests(unittest.TestCase):
    def test_repository_license_closure_passes(self) -> None:
        errors, report = audit.audit(ROOT)
        self.assertEqual(errors, [])
        self.assertTrue(report["third_party_license_audit"])
        self.assertEqual(report["manifest_components"], 24)
        self.assertEqual(report["implemented_external_elements"], 22)

    def test_manifest_hash_drift_fails_closed(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            (root / "docs").mkdir()
            (root / "tools").mkdir()
            (root / "NOTICE.txt").write_text("actual notice\n", encoding="utf-8")
            (root / "tools" / "package_test_release.py").write_text(
                "DOCUMENTS = ((\"docs/THIRD_PARTY_LICENSE_MANIFEST.csv\", "
                "\"LICENSES/THIRD-PARTY-MANIFEST.csv\"), "
                "(\"NOTICE.txt\", \"NOTICE.txt\"))\n",
                encoding="utf-8",
            )
            fields = sorted(audit.REQUIRED_COLUMNS)
            with (root / audit.MANIFEST).open("w", encoding="utf-8", newline="") as stream:
                writer = csv.DictWriter(stream, fieldnames=fields)
                writer.writeheader()
                writer.writerow(
                    {
                        "component": "fixture",
                        "category": "fixture",
                        "registry_source_mod": "",
                        "source_url": "https://example.invalid/fixture",
                        "version_or_commit": "1",
                        "usage": "fixture",
                        "license": "MIT",
                        "license_file": "NOTICE.txt",
                        "license_sha256": "0" * 64,
                        "included_in_binary": "false",
                        "notice_in_package": "true",
                        "package_path": "NOTICE.txt",
                        "audit_status": "verified",
                    }
                )
            errors, _ = audit.audit_manifest(root)
        self.assertTrue(any("license hash mismatch for fixture" in error for error in errors))


if __name__ == "__main__":
    unittest.main()
