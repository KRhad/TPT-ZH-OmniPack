from __future__ import annotations

import importlib.util
import os
from pathlib import Path
import sys
import tempfile
import unittest
from unittest import mock


ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / "tools" / "prepare_windows_release.py"


def import_script(name: str, path: Path):
    spec = importlib.util.spec_from_file_location(name, path)
    assert spec is not None and spec.loader is not None
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


prepare_windows_release = import_script("prepare_windows_release", SCRIPT)


class PrepareWindowsReleaseTests(unittest.TestCase):
    def test_source_date_epoch_defaults_to_earliest_zip_timestamp(self) -> None:
        with mock.patch.dict(os.environ, {}, clear=True):
            self.assertEqual(
                prepare_windows_release.source_date_epoch(),
                prepare_windows_release.MIN_SOURCE_DATE_EPOCH,
            )

    def test_source_date_epoch_respects_environment_and_explicit_override(self) -> None:
        with mock.patch.dict(os.environ, {"SOURCE_DATE_EPOCH": "1785456000"}, clear=True):
            self.assertEqual(prepare_windows_release.source_date_epoch(), 1785456000)
            self.assertEqual(
                prepare_windows_release.source_date_epoch(315532801),
                315532801,
            )

    def test_source_date_epoch_rejects_invalid_or_pre_1980_values(self) -> None:
        with mock.patch.dict(os.environ, {"SOURCE_DATE_EPOCH": "invalid"}, clear=True):
            with self.assertRaisesRegex(ValueError, "must be an integer"):
                prepare_windows_release.source_date_epoch()
        with self.assertRaisesRegex(ValueError, "on or after 1980-01-01"):
            prepare_windows_release.source_date_epoch(315532799)

    def test_prepare_pins_identical_epoch_for_objcopy_and_strip(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            root = Path(temporary_directory)
            raw_executable = root / "raw.exe"
            executable = root / "release" / "app.exe"
            symbols = root / "symbols" / "app.debug"
            raw_executable.write_bytes(b"MZfixture")

            def fake_run(command: list[str], *, env=None) -> None:
                self.assertIsNotNone(env)
                self.assertEqual(env["SOURCE_DATE_EPOCH"], "315532800")
                if "--only-keep-debug" in command:
                    Path(command[-1]).write_bytes(b"symbols")

            with (
                mock.patch.dict(os.environ, {}, clear=True),
                mock.patch.object(prepare_windows_release, "run", side_effect=fake_run) as run,
            ):
                prepare_windows_release.prepare(
                    raw_executable,
                    executable,
                    symbols,
                    "objcopy",
                    "strip",
                )

            self.assertEqual(run.call_count, 3)
            self.assertIn("--add-gnu-debuglink=app.debug", run.call_args_list[2].args[0])
            self.assertEqual(executable.read_bytes(), raw_executable.read_bytes())
            self.assertEqual(symbols.read_bytes(), b"symbols")


if __name__ == "__main__":
    unittest.main()
