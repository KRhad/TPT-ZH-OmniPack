from __future__ import annotations

import importlib.util
import hashlib
import json
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
        (source / "meson_options.txt").write_text(
            "option('can_install', type: 'combo', choices: ['no', 'yes', 'yes_check', 'auto'], value: 'no')\n",
            encoding="utf-8",
        )
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
        for name in (
            "FUSION_PIXEL_FONT_OFL-1.1.txt",
            "FUSION_PIXEL_FONT_ARK_PIXEL_OFL-1.1.txt",
            "FUSION_PIXEL_FONT_CUBIC_11_OFL-1.1.txt",
            "FUSION_PIXEL_FONT_GALMURI_OFL-1.1.txt",
        ):
            (third_party / name).write_text("OFL\n", encoding="utf-8")
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

    def add_dev_sources(self, source: Path) -> None:
        examples = source / "examples" / "0.2.0"
        examples.mkdir(parents=True)
        filenames = [archive for _, archive in package_test_release.DEV_DOCUMENTS if archive.endswith(".stm")]
        rows = []
        for index, archive_name in enumerate(filenames, start=1):
            filename = Path(archive_name).name
            data = b"OPS1" + b"\0" * 8 + b"BZh" + bytes([index])
            (examples / filename).write_bytes(data)
            rows.append(
                {
                    "id": f"example-{index}",
                    "filename": filename,
                    "bytes": len(data),
                    "sha256": hashlib.sha256(data).hexdigest().upper(),
                }
            )
        executable_hash = hashlib.sha256(
            (source / "tpt-zh-omnipack.exe").read_bytes()
        ).hexdigest().upper()
        source_commit = "b" * 40
        (source / "docs" / "TUTORIALS_0.2.json").write_text(
            "{}\n", encoding="utf-8"
        )
        (examples / "example-spec.json").write_text("{}\n", encoding="utf-8")
        (examples / "manifest.json").write_text(
            json.dumps(
                {
                    "content_version": package_test_release.DEV_VERSION,
                    "source_commit": source_commit,
                    "generator_exe_sha256": executable_hash,
                    "examples": rows,
                }
            ),
            encoding="utf-8",
        )
        (examples / "tutorials-runtime-report.json").write_text(
            json.dumps(
                {
                    "source_commit": source_commit,
                    "executable_sha256": executable_hash,
                    "pass_count": 8,
                }
            ),
            encoding="utf-8",
        )

    def add_automation_sources(self, source: Path) -> None:
        self.add_dev_sources(source)
        (source / "docs" / "AUTOMATION_CAPABILITIES.csv").write_text(
            "capability_id,decision\nsensor,reuse_official\n",
            encoding="utf-8",
        )
        automation = source / "automation" / "0.3.0"
        automation.mkdir(parents=True)
        (automation / "scenario-spec.json").write_text("{}\n", encoding="utf-8")
        (automation / "official-source-sha256.json").write_text(
            "{}\n", encoding="utf-8"
        )
        examples = source / "examples" / "0.3.0"
        examples.mkdir(parents=True)
        filenames = [
            archive
            for _, archive in package_test_release.AUTOMATION_ONLY_DOCUMENTS
            if archive.endswith(".stm")
        ]
        rows = []
        for index, archive_name in enumerate(filenames, start=1):
            filename = Path(archive_name).name
            data = b"OPS1" + b"\0" * 8 + b"BZh" + bytes([32 + index])
            (examples / filename).write_bytes(data)
            rows.append(
                {
                    "id": f"automation-{index}",
                    "filename": filename,
                    "stamp_id": f"03{index:08x}",
                    "bytes": len(data),
                    "sha256": hashlib.sha256(data).hexdigest().upper(),
                }
            )
        executable_hash = hashlib.sha256(
            (source / "tpt-zh-omnipack.exe").read_bytes()
        ).hexdigest().upper()
        source_commit = "c" * 40
        (examples / "manifest.json").write_text(
            json.dumps(
                {
                    "content_version": package_test_release.AUTOMATION_VERSION,
                    "source_commit": source_commit,
                    "source_tree_state": "clean",
                    "generator_exe_sha256": executable_hash,
                    "challenge_ids": [f"A{index:02d}" for index in range(1, 7)],
                    "scenarios": rows,
                }
            ),
            encoding="utf-8",
        )
        (examples / "runtime-report.json").write_text(
            json.dumps(
                {
                    "source_commit": source_commit,
                    "source_tree_state": "clean",
                    "executable_sha256": executable_hash,
                    "scenario_pass_count": 9,
                    "challenge_pass_count": 6,
                    "stop_event_delta_total": 0,
                }
            ),
            encoding="utf-8",
        )

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

    def test_portable_package_rejects_first_run_install_prompt_default(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            source = self.make_source_root(Path(temporary))
            options = source / "meson_options.txt"
            options.write_text(
                options.read_text(encoding="utf-8").replace("value: 'no'", "value: 'auto'"),
                encoding="utf-8",
            )
            with self.assertRaisesRegex(ValueError, "can_install=no"):
                self.build_package(source)

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

    def test_local_dev_package_includes_bound_examples(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            source = self.make_source_root(Path(temporary))
            self.add_dev_sources(source)
            with mock.patch.object(package_test_release, "git_revision", return_value="a" * 40):
                package, _, symbols, _ = package_test_release.build_package(
                    source,
                    source / "tpt-zh-omnipack.exe",
                    source / "tpt-zh-omnipack.debug",
                    source / "dist",
                    version=package_test_release.DEV_VERSION,
                    kind="local-dev",
                    include_examples=True,
                )
            self.assertEqual(
                test_release_audit.audit_package(
                    package,
                    version=package_test_release.DEV_VERSION,
                    kind="local-dev",
                ),
                [],
            )
            self.assertEqual(
                test_release_audit.audit_package(
                    symbols,
                    True,
                    version=package_test_release.DEV_VERSION,
                ),
                [],
            )

    def test_automation_package_includes_both_content_generations(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            source = self.make_source_root(Path(temporary))
            self.add_automation_sources(source)
            with mock.patch.object(
                package_test_release, "git_revision", return_value="a" * 40
            ):
                package, _, symbols, _ = package_test_release.build_package(
                    source,
                    source / "tpt-zh-omnipack.exe",
                    source / "tpt-zh-omnipack.debug",
                    source / "dist",
                    version=package_test_release.AUTOMATION_VERSION,
                    kind="local-dev",
                    include_examples=True,
                )
            self.assertEqual(
                test_release_audit.audit_package(
                    package,
                    version=package_test_release.AUTOMATION_VERSION,
                    kind="local-dev",
                ),
                [],
            )
            self.assertEqual(
                test_release_audit.audit_package(
                    symbols,
                    True,
                    version=package_test_release.AUTOMATION_VERSION,
                ),
                [],
            )

    def test_local_dev_profile_must_be_explicit(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            source = self.make_source_root(Path(temporary))
            with self.assertRaisesRegex(ValueError, "requires kind=local-dev"):
                package_test_release.build_package(
                    source,
                    source / "tpt-zh-omnipack.exe",
                    source / "tpt-zh-omnipack.debug",
                    source / "dist",
                    version=package_test_release.DEV_VERSION,
                )


if __name__ == "__main__":
    unittest.main()
