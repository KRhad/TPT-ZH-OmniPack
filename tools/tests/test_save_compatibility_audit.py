from __future__ import annotations

import importlib.util
from pathlib import Path
import sys
import unittest


SCRIPT = Path(__file__).resolve().parents[1] / "save_compatibility_audit.py"
SPEC = importlib.util.spec_from_file_location("save_compatibility_audit", SCRIPT)
assert SPEC is not None and SPEC.loader is not None
save_compatibility_audit = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = save_compatibility_audit
SPEC.loader.exec_module(save_compatibility_audit)

ROOT = Path(__file__).resolve().parents[2]


class SaveCompatibilityAuditTests(unittest.TestCase):
    def test_repository_passes(self) -> None:
        self.assertEqual(save_compatibility_audit.audit(ROOT), [])

    def test_missing_packed_carried_type_detection_is_rejected(self) -> None:
        source_path = ROOT / "src" / "gui" / "game" / "OmniContent.cpp"
        source = source_path.read_text(encoding="utf-8")
        errors: list[str] = []
        original = save_compatibility_audit.read_text
        try:
            save_compatibility_audit.read_text = lambda path, current_errors: (
                source.replace("inspectType(TYP(*property));", "inspectType(*property);")
                if path == source_path
                else original(path, current_errors)
            )
            save_compatibility_audit.check_source(ROOT, errors)
        finally:
            save_compatibility_audit.read_text = original
        self.assertTrue(any("packed carried type" in error for error in errors))

    def test_missing_read_only_upload_guard_is_rejected(self) -> None:
        source_path = ROOT / "src" / "gui" / "game" / "GameController.cpp"
        source = source_path.read_text(encoding="utf-8")
        errors: list[str] = []
        original = save_compatibility_audit.read_text
        try:
            save_compatibility_audit.read_text = lambda path, current_errors: (
                source.replace(
                    "void GameController::OpenSaveWindow()\n{\n\tif (readOnlySave)",
                    "void GameController::OpenSaveWindow()\n{\n\tif (false)",
                )
                if path == source_path
                else original(path, current_errors)
            )
            save_compatibility_audit.check_source(ROOT, errors)
        finally:
            save_compatibility_audit.read_text = original
        self.assertTrue(any("read-only upload guard" in error for error in errors))


if __name__ == "__main__":
    unittest.main()
