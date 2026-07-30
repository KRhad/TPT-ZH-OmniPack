from __future__ import annotations

import importlib.util
from pathlib import Path
import sys
import tempfile
import unittest
import warnings
from unittest import mock
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
            "Windows x64\n不属于本测试版\n已知测试限制\n未签名\n",
            encoding="utf-8",
        )
        (source / "docs" / "THIRD_PARTY_SOURCES.md").write_text("Sources\n", encoding="utf-8")
        (source / "docs" / "KNOWN_ISSUES.md").write_text("Issues\n", encoding="utf-8")
        (source / "docs" / "AI_DISCLOSURE.md").write_text("AI\n", encoding="utf-8")
        (source / "docs" / "FONT_AUDIT.md").write_text("Font\n", encoding="utf-8")
        third_party = source / "resources" / "third_party"
        third_party.mkdir(parents=True)
        (third_party / "GNU_UNIFONT_COPYING.txt").write_text("OFL\n", encoding="utf-8")
        executable = source / "tpt-zh-omnipack.exe"
        executable.write_bytes(b"MZ test executable")
        (source / "tpt-zh-omnipack.debug").write_bytes(b"MZ test debug symbols")
        return source

    def build_package(self, source: Path) -> tuple[Path, Path]:
        with mock.patch.object(package_test_release, "git_revision", return_value="a" * 40):
            package, _, symbols, _ = package_test_release.build_package(
                source,
                source / "tpt-zh-omnipack.exe",
                source / "tpt-zh-omnipack.debug",
                source / "dist",
            )
        return package, symbols

    def test_generated_package_passes(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            source = self.make_source_root(Path(temporary))
            package, symbols = self.build_package(source)
            self.assertEqual(test_release_audit.audit_package(package), [])
            self.assertEqual(test_release_audit.audit_package(symbols, True), [])

    def test_unexpected_personal_data_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            source = self.make_source_root(Path(temporary))
            package, _ = self.build_package(source)
            with zipfile.ZipFile(package, "a") as archive:
                archive.writestr("TPT-ZH-OmniPack-0.1.0-test-Windows-x64/powder.pref", "{}")
            errors = test_release_audit.audit_package(package)
            self.assertTrue(any("forbidden" in error for error in errors))

    def test_manifest_hash_tampering_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            source = self.make_source_root(Path(temporary))
            package, _ = self.build_package(source)
            with warnings.catch_warnings():
                warnings.simplefilter("ignore", UserWarning)
                with zipfile.ZipFile(package, "a") as archive:
                    archive.writestr(
                        "TPT-ZH-OmniPack-0.1.0-test-Windows-x64/tpt-zh-omnipack.exe",
                        b"MZ modified executable",
                    )
            errors = test_release_audit.audit_package(package)
            self.assertTrue(
                any("duplicate ZIP member" in error or "manifest" in error for error in errors)
            )


if __name__ == "__main__":
    unittest.main()
