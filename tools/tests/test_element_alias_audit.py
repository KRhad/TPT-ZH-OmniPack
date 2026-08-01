from __future__ import annotations

import importlib.util
from pathlib import Path
import sys
import unittest


SCRIPT = Path(__file__).resolve().parents[1] / "element_alias_audit.py"
SPEC = importlib.util.spec_from_file_location("element_alias_audit", SCRIPT)
assert SPEC is not None and SPEC.loader is not None
element_alias_audit = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = element_alias_audit
SPEC.loader.exec_module(element_alias_audit)

ROOT = Path(__file__).resolve().parents[2]


class ElementAliasAuditTests(unittest.TestCase):
    def test_repository_passes(self) -> None:
        self.assertEqual(element_alias_audit.audit(ROOT), [])

    def test_visible_alias_is_rejected(self) -> None:
        original = element_alias_audit.read_text

        def mutated(path: Path, errors: list[str]) -> str:
            text = original(path, errors)
            if path.name == "MSCR.cpp":
                return text.replace("MenuVisible = 0", "MenuVisible = 1", 1)
            return text

        try:
            element_alias_audit.read_text = mutated
            errors = element_alias_audit.audit(ROOT)
        finally:
            element_alias_audit.read_text = original
        self.assertTrue(any("MSCR hidden menu" in error for error in errors))

    def test_new_production_of_legacy_alias_is_rejected(self) -> None:
        original = element_alias_audit.read_text

        def mutated(path: Path, errors: list[str]) -> str:
            text = original(path, errors)
            if path.name == "OmniPeriodic.cpp":
                return text + "\nint forbidden = PT_MSCR;\n"
            return text

        try:
            element_alias_audit.read_text = mutated
            errors = element_alias_audit.audit(ROOT)
        finally:
            element_alias_audit.read_text = original
        self.assertTrue(any("new production" in error for error in errors))

    def test_recoverable_scrap_cannot_fall_through_to_official_update(self) -> None:
        original = element_alias_audit.read_text

        def mutated(path: Path, errors: list[str]) -> str:
            text = original(path, errors)
            if path.name == "BRMT.cpp":
                return text.replace(
                    "if (IsOmniRecoverableScrap(parts[i]))",
                    "if (false && IsOmniRecoverableScrap(parts[i]))",
                    1,
                )
            return text

        try:
            element_alias_audit.read_text = mutated
            errors = element_alias_audit.audit(ROOT)
        finally:
            element_alias_audit.read_text = original
        self.assertTrue(
            any("recoverable BRMT state isolation" in error for error in errors)
        )


if __name__ == "__main__":
    unittest.main()
