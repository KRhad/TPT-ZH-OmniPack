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
        (source / "README.md").write_text("English readme\n", encoding="utf-8")
        (source / "README.zh-CN.md").write_text("Readme\n", encoding="utf-8")
        (source / "changelog.txt").write_text("English changes\n", encoding="utf-8")
        (source / "CHANGELOG.zh-CN.md").write_text("Changes\n", encoding="utf-8")
        (source / "meson_options.txt").write_text(
            "option('can_install', type: 'combo', choices: ['no', 'yes', 'yes_check', 'auto'], value: 'no')\n",
            encoding="utf-8",
        )
        (source / "docs" / "TEST_RELEASE.md").write_text(
            "Windows x64\n不属于本测试版\n已知测试限制\n未签名\n",
            encoding="utf-8",
        )
        (source / "docs" / "RELEASE_CANDIDATE_1.0.0.md").write_text(
            "1.0.0-rc9\n本地发布候选\n不是正式发布\n487\n484\n466\n301\n165\n"
            "118/118\nrelease_ready=false\n未签名\n",
            encoding="utf-8",
        )
        (source / "docs" / "THIRD_PARTY_SOURCES.md").write_text("Sources\n", encoding="utf-8")
        (source / "docs" / "KNOWN_ISSUES.md").write_text("Issues\n", encoding="utf-8")
        (source / "docs" / "AI_DISCLOSURE.md").write_text("AI\n", encoding="utf-8")
        (source / "docs" / "FONT_AUDIT.md").write_text("Font\n", encoding="utf-8")
        (source / "docs" / "RELEASE_1.0.0_README.en.md").write_text(
            "Final English readme\n", encoding="utf-8"
        )
        (source / "docs" / "RELEASE_1.0.0_README.zh-CN.md").write_text(
            "Final Chinese readme\n", encoding="utf-8"
        )
        (source / "docs" / "RELEASE_1.0.0_CHANGELOG.en.txt").write_text(
            "Final English changes\n", encoding="utf-8"
        )
        (source / "docs" / "RELEASE_1.0.0_CHANGELOG.zh-CN.md").write_text(
            "Final Chinese changes\n", encoding="utf-8"
        )
        (source / "docs" / "THIRD_PARTY_LICENSE_MANIFEST.csv").write_text(
            "component,license\nfixture,MIT\n", encoding="utf-8"
        )
        third_party = source / "resources" / "third_party"
        third_party.mkdir(parents=True)
        (third_party / "GNU_UNIFONT_COPYING.txt").write_text("OFL\n", encoding="utf-8")
        for name in (
            "FUSION_PIXEL_FONT_OFL-1.1.txt",
            "FUSION_PIXEL_FONT_ARK_PIXEL_OFL-1.1.txt",
            "FUSION_PIXEL_FONT_CUBIC_11_OFL-1.1.txt",
            "FUSION_PIXEL_FONT_GALMURI_OFL-1.1.txt",
            "OPENSTAX_CHEMISTRY_CC-BY-4.0.txt",
        ):
            (third_party / name).write_text("OFL\n", encoding="utf-8")
        library_licenses = source / package_test_release.PREBUILT_LICENSE_ROOT
        library_licenses.mkdir(parents=True)
        for name in (
            "bzip2", "fftw3f", "jsoncpp", "libcurl", "libpng", "lua5.1",
            "lua5.2", "luajit", "mbedtls", "nghttp2", "sdl2", "zlib",
        ):
            (library_licenses / f"{name}.LICENSE").write_text(
                f"{name} license\n", encoding="utf-8"
            )
        executable = source / "tpt-zh-omnipack.exe"
        release_labels = (
            package_test_release.VERSION,
            package_test_release.DEV_VERSION,
            package_test_release.AUTOMATION_VERSION,
            package_test_release.PREVIOUS_PRIVATE_TEST_VERSION,
            package_test_release.PRIVATE_TEST_VERSION,
            package_test_release.RELEASE_CANDIDATE_VERSION,
            package_test_release.FINAL_VERSION,
        )
        executable.write_bytes(
            b"MZ test executable\0"
            + b"\0".join(label.encode("ascii") for label in release_labels)
            + b"\0"
            + b"\0\0".join(label.encode("utf-16le") for label in release_labels)
        )
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
            self.assertTrue(any("release label" in error for error in errors))

    def test_packaging_rejects_stale_executable_release_label(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            source = self.make_source_root(Path(temporary))
            stale_version = "1.0.0-rc8"
            (source / "tpt-zh-omnipack.exe").write_bytes(
                b"MZ stale executable\0"
                + stale_version.encode("ascii")
                + b"\0"
                + stale_version.encode("utf-16le")
            )
            with self.assertRaisesRegex(ValueError, "release label"):
                package_test_release.build_package(
                    source,
                    source / "tpt-zh-omnipack.exe",
                    source / "tpt-zh-omnipack.debug",
                    source / "dist",
                    version=package_test_release.RELEASE_CANDIDATE_VERSION,
                    kind="release-candidate",
                    include_examples=False,
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

    def test_private_content_packages_use_versioned_instructions_without_stale_examples(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            source = self.make_source_root(Path(temporary))
            (source / "docs" / "PRIVATE_TEST_0.6.0.md").write_text(
                "0.6.0-dev\n不是 1.0.0 正式版\n451\n118/118\n621\nrelease_ready=false\n",
                encoding="utf-8",
            )
            (source / "docs" / "PRIVATE_TEST_0.7.0.md").write_text(
                "0.7.0-dev\n不是 1.0.0 正式版\n487\n466\n118/118\n685\nrelease_ready=false\n",
                encoding="utf-8",
            )
            private_versions = (
                package_test_release.PREVIOUS_PRIVATE_TEST_VERSION,
                package_test_release.PRIVATE_TEST_VERSION,
            )
            for version in private_versions:
                with self.subTest(version=version), mock.patch.object(
                    package_test_release, "git_revision", return_value="a" * 40
                ):
                    package, _, symbols, _ = package_test_release.build_package(
                        source,
                        source / "tpt-zh-omnipack.exe",
                        source / "tpt-zh-omnipack.debug",
                        source / "dist",
                        version=version,
                        kind="local-dev",
                        include_examples=False,
                    )
                    self.assertEqual(
                        test_release_audit.audit_package(
                            package,
                            version=version,
                            kind="local-dev",
                        ),
                        [],
                    )
                    self.assertEqual(
                        test_release_audit.audit_package(
                            symbols,
                            True,
                            version=version,
                        ),
                        [],
                    )
                    with zipfile.ZipFile(package) as archive:
                        names = archive.namelist()
                        self.assertFalse(
                            any("examples/0.2.0" in name for name in names)
                        )
                        instructions = archive.read(
                            f"TPT-ZH-OmniPack-{version}-Windows-x64/TESTING.zh-CN.md"
                        ).decode("utf-8")
                        self.assertIn(version, instructions)
                        self.assertNotIn("0.1.0-test", instructions)

    def test_current_private_content_audit_rejects_stale_counts(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            source = self.make_source_root(Path(temporary))
            (source / "docs" / "PRIVATE_TEST_0.7.0.md").write_text(
                "0.7.0-dev\n不是 1.0.0 正式版\n451\n118/118\n621\nrelease_ready=false\n",
                encoding="utf-8",
            )
            with mock.patch.object(
                package_test_release, "git_revision", return_value="a" * 40
            ):
                package, _, _, _ = package_test_release.build_package(
                    source,
                    source / "tpt-zh-omnipack.exe",
                    source / "tpt-zh-omnipack.debug",
                    source / "dist",
                    version=package_test_release.PRIVATE_TEST_VERSION,
                    kind="local-dev",
                    include_examples=False,
                )
            errors = test_release_audit.audit_package(
                package,
                version=package_test_release.PRIVATE_TEST_VERSION,
                kind="local-dev",
            )
            self.assertTrue(any("'487'" in error for error in errors))
            self.assertTrue(any("'466'" in error for error in errors))
            self.assertTrue(any("'685'" in error for error in errors))

    def test_release_candidate_profile_is_explicit_and_auditable(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            source = self.make_source_root(Path(temporary))
            with mock.patch.object(
                package_test_release, "git_revision", return_value="d" * 40
            ):
                package, _, symbols, _ = package_test_release.build_package(
                    source,
                    source / "tpt-zh-omnipack.exe",
                    source / "tpt-zh-omnipack.debug",
                    source / "dist",
                    version=package_test_release.RELEASE_CANDIDATE_VERSION,
                    kind="release-candidate",
                    include_examples=False,
                    source_provenance={
                        "source_state": "dirty",
                        "source_worktree_sha256": "B" * 64,
                        "source_untracked_files": "9",
                    },
                )
            self.assertEqual(
                test_release_audit.audit_package(
                    package,
                    version=package_test_release.RELEASE_CANDIDATE_VERSION,
                    kind="release-candidate",
                ),
                [],
            )
            self.assertEqual(
                test_release_audit.audit_package(
                    symbols,
                    True,
                    version=package_test_release.RELEASE_CANDIDATE_VERSION,
                ),
                [],
            )
            with zipfile.ZipFile(package) as archive:
                fields, _ = test_release_audit.parse_manifest(
                    archive.read(
                        "TPT-ZH-OmniPack-1.0.0-rc9-Windows-x64/TEST-MANIFEST.txt"
                    )
                )
                self.assertEqual(fields["kind"], "release-candidate")
                self.assertEqual(fields["source_state"], "dirty")
                self.assertEqual(fields["source_worktree_sha256"], "B" * 64)
                self.assertEqual(fields["source_untracked_files"], "9")
                self.assertFalse(any(name.endswith(".stm") for name in archive.namelist()))

    def test_final_release_omits_test_assets_and_uses_release_manifest(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            source = self.make_source_root(Path(temporary))
            with mock.patch.object(
                package_test_release, "git_revision", return_value="e" * 40
            ):
                package, _, symbols, _ = package_test_release.build_package(
                    source,
                    source / "tpt-zh-omnipack.exe",
                    source / "tpt-zh-omnipack.debug",
                    source / "dist",
                    version=package_test_release.FINAL_VERSION,
                    kind="release",
                    include_examples=False,
                    source_provenance={
                        "source_state": "clean",
                        "source_worktree_sha256": "C" * 64,
                        "source_untracked_files": "0",
                    },
                )
            self.assertEqual(
                test_release_audit.audit_package(
                    package,
                    version=package_test_release.FINAL_VERSION,
                    kind="release",
                ),
                [],
            )
            self.assertEqual(
                test_release_audit.audit_package(
                    symbols,
                    True,
                    version=package_test_release.FINAL_VERSION,
                ),
                [],
            )
            with zipfile.ZipFile(package) as archive:
                names = archive.namelist()
                self.assertTrue(any(name.endswith("/MANIFEST.txt") for name in names))
                self.assertFalse(any("TEST" in name.upper() for name in names))
                self.assertFalse(any(name.endswith("/FONT-AUDIT.md") for name in names))

    def test_worktree_provenance_hashes_selected_file_bytes_not_patch_text(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            source = Path(temporary)
            (source / "tracked.txt").write_bytes(b"tracked\r\ncontent\r\n")
            (source / "new.txt").write_bytes(b"untracked\ncontent\n")

            def git_result(args, **_kwargs):
                if "diff" in args:
                    self.assertIn("--name-only", args)
                    self.assertIn("--ignore-space-at-eol", args)
                    return mock.Mock(
                        returncode=0,
                        stdout=b"tracked.txt\0deleted.txt\0",
                        stderr=b"",
                    )
                self.assertIn("ls-files", args)
                return mock.Mock(
                    returncode=0, stdout=b"new.txt\0", stderr=b""
                )

            with mock.patch.object(
                package_test_release.subprocess, "run", side_effect=git_result
            ):
                first = package_test_release.git_worktree_provenance(source)
                second = package_test_release.git_worktree_provenance(source)
                (source / "tracked.txt").write_bytes(b"tracked\nchanged\n")
                changed = package_test_release.git_worktree_provenance(source)

            self.assertEqual(first, second)
            self.assertEqual(first["source_state"], "dirty")
            self.assertEqual(first["source_untracked_files"], "1")
            self.assertRegex(first["source_worktree_sha256"], r"^[0-9A-F]{64}$")
            self.assertNotEqual(
                first["source_worktree_sha256"],
                changed["source_worktree_sha256"],
            )

    def test_release_candidate_profile_cannot_use_local_dev_kind(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            source = self.make_source_root(Path(temporary))
            with self.assertRaisesRegex(ValueError, "requires kind=release-candidate"):
                package_test_release.build_package(
                    source,
                    source / "tpt-zh-omnipack.exe",
                    source / "tpt-zh-omnipack.debug",
                    source / "dist",
                    version=package_test_release.RELEASE_CANDIDATE_VERSION,
                    kind="local-dev",
                    include_examples=False,
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
