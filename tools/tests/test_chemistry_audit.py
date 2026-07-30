from __future__ import annotations

import importlib.util
from pathlib import Path
import sys
import unittest


SCRIPT = Path(__file__).resolve().parents[1] / "chemistry_audit.py"
SPEC = importlib.util.spec_from_file_location("chemistry_audit", SCRIPT)
assert SPEC is not None and SPEC.loader is not None
chemistry_audit = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = chemistry_audit
SPEC.loader.exec_module(chemistry_audit)

ROOT = Path(__file__).resolve().parents[2]


class ChemistryAuditTests(unittest.TestCase):
    def test_repository_passes(self) -> None:
        self.assertEqual(chemistry_audit.audit(ROOT), [])

    def test_missing_budget_is_rejected(self) -> None:
        source = (ROOT / "src" / "simulation" / "OmniChemistry.cpp").read_text(
            encoding="utf-8"
        )
        errors: list[str] = []
        original = chemistry_audit.read_text
        try:
            chemistry_audit.read_text = lambda _path, _errors: source.replace(
                "ChemistryReactionsPerFrame = 1536",
                "ChemistryReactionsPerFrame = 0",
            )
            chemistry_audit.check_engine(ROOT, errors)
        finally:
            chemistry_audit.read_text = original
        self.assertTrue(any("per-frame reaction budget" in error for error in errors))

    def test_missing_peroxide_treatment_is_rejected(self) -> None:
        source = (ROOT / "src" / "simulation" / "OmniChemistry.cpp").read_text(
            encoding="utf-8"
        )
        errors: list[str] = []
        original = chemistry_audit.read_text
        try:
            chemistry_audit.read_text = lambda _path, _errors: source.replace(
                "PeroxidePathogenTreatment", "RemovedPeroxideTreatment"
            )
            chemistry_audit.check_engine(ROOT, errors)
        finally:
            chemistry_audit.read_text = original
        self.assertTrue(any("peroxide pathogen treatment" in error for error in errors))

    def test_missing_slag_leaching_is_rejected(self) -> None:
        source = (ROOT / "src" / "simulation" / "OmniChemistry.cpp").read_text(
            encoding="utf-8"
        )
        errors: list[str] = []
        original = chemistry_audit.read_text
        try:
            chemistry_audit.read_text = lambda _path, _errors: source.replace(
                "SlagAcidLeaching", "RemovedSlagLeaching"
            )
            chemistry_audit.check_engine(ROOT, errors)
        finally:
            chemistry_audit.read_text = original
        self.assertTrue(any("slag acid leaching" in error for error in errors))


if __name__ == "__main__":
    unittest.main()
