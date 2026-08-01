from __future__ import annotations

import importlib.util
from pathlib import Path
import sys
import unittest


SCRIPT = Path(__file__).resolve().parents[1] / "isotope_audit.py"
SPEC = importlib.util.spec_from_file_location("isotope_audit", SCRIPT)
assert SPEC is not None and SPEC.loader is not None
isotope_audit = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = isotope_audit
SPEC.loader.exec_module(isotope_audit)

ROOT = Path(__file__).resolve().parents[2]


class IsotopeAuditTests(unittest.TestCase):
    def test_repository_passes(self) -> None:
        self.assertEqual(isotope_audit.audit(ROOT), [])

    def test_global_scan_is_rejected(self) -> None:
        source = (ROOT / "src" / "simulation" / "OmniIsotopes.cpp").read_text(
            encoding="utf-8"
        )
        errors: list[str] = []
        original = isotope_audit.read_text
        try:
            isotope_audit.read_text = lambda path, _errors: (
                source + "\nfor (int index = 0; index < NPART; ++index) {}\n"
                if path.name == "OmniIsotopes.cpp"
                else original(path, _errors)
            )
            isotope_audit.check_engine(ROOT, errors)
        finally:
            isotope_audit.read_text = original
        self.assertTrue(any("global particle" in error for error in errors))

    def test_missing_molten_transition_hook_is_rejected(self) -> None:
        source = (ROOT / "src" / "simulation" / "Simulation.cpp").read_text(
            encoding="utf-8"
        )
        errors: list[str] = []
        original = isotope_audit.read_text
        try:
            isotope_audit.read_text = lambda path, _errors: (
                source.replace(
                    "OmniIsotopeOwnsMoltenTransition(parts[i])",
                    "false /* isotope transition hook removed */",
                )
                if path.name == "Simulation.cpp"
                else original(path, _errors)
            )
            isotope_audit.check_engine(ROOT, errors)
        finally:
            isotope_audit.read_text = original
        self.assertTrue(any("generic freezing" in error for error in errors))


if __name__ == "__main__":
    unittest.main()
