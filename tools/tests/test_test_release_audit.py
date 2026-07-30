from __future__ import annotations

import importlib.util
from pathlib import Path
import sys
import tempfile
import unittest
import warnings
import zipfile


ROOT = Path(__file__).resolve().parents[2]
PACKAGE_SCRIPT = ROOT / "tools" / "package_test_release.py"
AUDIT_SCRIPT = ROOT / "tools" / "test_release_audit.py"


def import_script(name: str, path: Path):
    spec = importlib.util.spec_from_file_location(name, path)
    assert spec is not None and spec.loader is not None
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


package_test_release = import_script("package_test_release", PACKAGE_SCRIPT)
test_release_audit = import_script("test_release_audit", AUDIT_SCRIPT)


class TestReleaseAuditTests(unittest.TestCase):
    def make_source_root(self, root: Path) -> Path:
        source = root / "source"
        (source / "docs").mkdir(parents=True)
        (source / "LICENSE").write_text("GPL test\n", encoding="utf-8")
        (source / "README.zh-CN.md").write_text("Readme\n", encoding="utf-8")
        (source / "CHANGELOG.zh-CN.md").write_text("Changes\n", encoding="utf-8")
        (source / "docs" / "TEST_RELEASE.md").write_text(
            "Windows x64\n不属于本测试版\n已知测试限制\n",
            encoding="utf-8",
        )
        executable = source / "tpt-zh-omnipack.exe"
        executable.write_bytes(b"MZ test executable")
        return source

    def test_generated_package_passes(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            source = self.make_source_root(Path(temporary))
            package, _ = package_test_release.build_package(
                source, source / "tpt-zh-omnipack.exe", source / "dist"
            )
            self.assertEqual(test_release_audit.audit_package(package), [])

    def test_unexpected_personal_data_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            source = self.make_source_root(Path(temporary))
            package, _ = package_test_release.build_package(
                source, source / "tpt-zh-omnipack.exe", source / "dist"
            )
            with zipfile.ZipFile(package, "a") as archive:
                archive.writestr("TPT-ZH-OmniPack-Test-Windows-x64/powder.pref", "{}")
            errors = test_release_audit.audit_package(package)
            self.assertTrue(any("forbidden" in error for error in errors))

    def test_manifest_hash_tampering_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            source = self.make_source_root(Path(temporary))
            package, _ = package_test_release.build_package(
                source, source / "tpt-zh-omnipack.exe", source / "dist"
            )
            with warnings.catch_warnings():
                warnings.simplefilter("ignore", UserWarning)
                with zipfile.ZipFile(package, "a") as archive:
                    archive.writestr(
                        "TPT-ZH-OmniPack-Test-Windows-x64/tpt-zh-omnipack.exe",
                        b"MZ modified executable",
                    )
            errors = test_release_audit.audit_package(package)
            self.assertTrue(
                any("duplicate ZIP member" in error or "SHA-256" in error for error in errors)
            )


if __name__ == "__main__":
    unittest.main()
