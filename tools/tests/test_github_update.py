from __future__ import annotations

import bz2
import importlib.util
from pathlib import Path
import struct
import sys
import unittest


ROOT = Path(__file__).resolve().parents[2]


def import_script(name: str, path: Path):
    spec = importlib.util.spec_from_file_location(name, path)
    assert spec is not None and spec.loader is not None
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


package = import_script("package_github_update", ROOT / "tools/package_github_update.py")
audit = import_script("github_update_audit", ROOT / "tools/github_update_audit.py")


class GithubUpdateTests(unittest.TestCase):
    def test_windows_wrapper_roundtrip(self) -> None:
        executable = b"MZ" + bytes(range(256)) * 8
        update = package.build_windows_update(executable)
        self.assertEqual(update[:4], b"BuTT")
        self.assertEqual(struct.unpack("<I", update[4:8])[0], len(executable))
        self.assertEqual(bz2.decompress(update[8:]), executable)

    def test_manifest_routes_both_platforms_with_exact_integrity(self) -> None:
        manifest = package.build_startup_manifest(
            "1.2.3",
            ("/updates/windows.update", b"windows"),
            ("/updates/android.apk", b"android"),
            source_revision="a" * 40,
            published_at="2026-08-08T00:00:00Z",
            changelog="fixture",
        )
        stable = manifest["Updates"]["Stable"]
        self.assertEqual((stable["Major"], stable["Minor"], stable["Build"]), (1, 2, 3))
        self.assertEqual(stable["VersionCode"], 1_002_003)
        self.assertEqual(set(stable["Platforms"]), {"WIN64", "ANDROIDARM64"})
        self.assertEqual(stable["Platforms"]["WIN64"]["Sha256"], package.sha256(b"windows"))
        self.assertFalse(manifest["ReleaseReady"])

    def test_source_contract_is_complete(self) -> None:
        checks = audit.audit_source(ROOT)
        self.assertTrue(checks)
        self.assertTrue(all(checks.values()), checks)


if __name__ == "__main__":
    unittest.main()
