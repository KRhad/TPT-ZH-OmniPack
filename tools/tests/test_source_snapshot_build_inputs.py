from __future__ import annotations

import hashlib
import importlib.util
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
import zipfile


ROOT = Path(__file__).resolve().parents[2]


def import_script(name: str, path: Path):
    spec = importlib.util.spec_from_file_location(name, path)
    assert spec and spec.loader
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


source_snapshot = import_script(
    "source_snapshot_build_input_tests", ROOT / "tools" / "source_snapshot.py"
)


class SourceSnapshotBuildInputTests(unittest.TestCase):
    WRAP = "subprojects/test-release.wrap"
    DIRECTORY = "test-release"
    ARCHIVE = "test-release.zip"

    def make_root(self, parent: Path) -> Path:
        root = parent / "repo"
        (root / "subprojects" / "packagecache").mkdir(parents=True)
        (root / "subprojects" / self.DIRECTORY).mkdir()
        (root / "tracked.txt").write_text("tracked\n", encoding="utf-8")
        archive = root / "subprojects" / "packagecache" / self.ARCHIVE
        members = {
            "meson.build": b"project('fixture')\n",
            "lib/payload.bin": b"verified build input\0",
        }
        with zipfile.ZipFile(archive, "w", zipfile.ZIP_DEFLATED) as bundle:
            for relative, data in members.items():
                bundle.writestr(f"{self.DIRECTORY}/{relative}", data)
                target = root / "subprojects" / self.DIRECTORY / relative
                target.parent.mkdir(parents=True, exist_ok=True)
                target.write_bytes(data)
        archive_sha = hashlib.sha256(archive.read_bytes()).hexdigest()
        (root / self.WRAP).write_text(
            "\n".join([
                "[wrap-file]",
                f"directory = {self.DIRECTORY}",
                "source_url = https://example.invalid/test-release.zip",
                f"source_filename = {self.ARCHIVE}",
                f"source_hash = {archive_sha}",
                "",
            ]),
            encoding="utf-8",
        )
        subprocess.run(["git", "init", "-q"], cwd=root, check=True)
        subprocess.run(["git", "add", "tracked.txt", self.WRAP], cwd=root, check=True)
        return root

    def test_verified_archive_and_extracted_tree_are_bound(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = self.make_root(Path(temporary))
            value = source_snapshot.snapshot(root, [self.WRAP])
            self.assertTrue(value["build_inputs_ready"])
            self.assertEqual(value["build_inputs_total"], 1)
            self.assertRegex(value["build_inputs_sha256"], r"^[0-9A-F]{64}$")
            record = value["build_inputs"][0]
            self.assertTrue(record["archive_hash_matches"])
            self.assertTrue(record["extracted_matches_archive"])
            self.assertEqual(record["archive_tree_sha256"], record["extracted_tree_sha256"])

    def test_modified_ignored_dependency_cannot_keep_ready_snapshot(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = self.make_root(Path(temporary))
            before = source_snapshot.snapshot(root, [self.WRAP])
            (root / "subprojects" / self.DIRECTORY / "lib" / "payload.bin").write_bytes(
                b"tampered build input\0"
            )
            after = source_snapshot.snapshot(root, [self.WRAP])
            self.assertFalse(after["build_inputs_ready"])
            self.assertNotEqual(before["build_inputs_sha256"], after["build_inputs_sha256"])
            self.assertFalse(after["build_inputs"][0]["extracted_matches_archive"])

    def test_wrong_archive_hash_is_fail_closed(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = self.make_root(Path(temporary))
            wrap = root / self.WRAP
            lines = wrap.read_text(encoding="utf-8").splitlines()
            wrap.write_text(
                "\n".join(
                    "source_hash = " + "0" * 64 if line.startswith("source_hash = ") else line
                    for line in lines
                ) + "\n",
                encoding="utf-8",
            )
            value = source_snapshot.snapshot(root, [self.WRAP])
            self.assertFalse(value["build_inputs_ready"])
            self.assertFalse(value["build_inputs"][0]["archive_hash_matches"])


if __name__ == "__main__":
    unittest.main()
