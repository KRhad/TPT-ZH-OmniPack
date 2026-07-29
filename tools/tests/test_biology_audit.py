from __future__ import annotations

import importlib.util
from pathlib import Path
import sys
import unittest


SCRIPT = Path(__file__).resolve().parents[1] / "biology_audit.py"
SPEC = importlib.util.spec_from_file_location("biology_audit", SCRIPT)
assert SPEC is not None and SPEC.loader is not None
biology_audit = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = biology_audit
SPEC.loader.exec_module(biology_audit)

ROOT = Path(__file__).resolve().parents[2]


class BiologyAuditTests(unittest.TestCase):
    def test_repository_passes(self) -> None:
        self.assertEqual(biology_audit.audit(ROOT), [])

    def test_missing_budget_is_rejected(self) -> None:
        source = (ROOT / "src" / "simulation" / "OmniBiology.cpp").read_text(
            encoding="utf-8"
        )
        errors: list[str] = []
        original = biology_audit.read_text
        try:
            biology_audit.read_text = lambda _path, _errors: source.replace(
                "BiologyEventsPerFrame = 1024",
                "BiologyEventsPerFrame = 0",
            )
            biology_audit.check_engine(ROOT, errors)
        finally:
            biology_audit.read_text = original
        self.assertTrue(any("per-frame event budget" in error for error in errors))

    def test_unavailable_simplified_mode_is_rejected(self) -> None:
        settings_path = ROOT / "src" / "gui" / "game" / "OmniContent.cpp"
        settings = settings_path.read_text(encoding="utf-8")
        errors: list[str] = []
        original = biology_audit.read_text
        try:
            biology_audit.read_text = lambda path, current_errors: (
                settings.replace(
                    "false, true  },\n\t{ OmniSetting::PerformanceProtection",
                    "false, false },\n\t{ OmniSetting::PerformanceProtection",
                )
                if path == settings_path
                else original(path, current_errors)
            )
            biology_audit.check_engine(ROOT, errors)
        finally:
            biology_audit.read_text = original
        self.assertTrue(any("simplified biology setting" in error for error in errors))


if __name__ == "__main__":
    unittest.main()
