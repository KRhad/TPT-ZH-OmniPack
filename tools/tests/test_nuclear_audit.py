from __future__ import annotations

import importlib.util
from pathlib import Path
import sys
import unittest


SCRIPT = Path(__file__).resolve().parents[1] / "nuclear_audit.py"
SPEC = importlib.util.spec_from_file_location("nuclear_audit", SCRIPT)
assert SPEC is not None and SPEC.loader is not None
nuclear_audit = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = nuclear_audit
SPEC.loader.exec_module(nuclear_audit)

ROOT = Path(__file__).resolve().parents[2]


class NuclearAuditTests(unittest.TestCase):
    def test_repository_passes(self) -> None:
        self.assertEqual(nuclear_audit.audit(ROOT), [])

    def test_missing_budget_is_rejected(self) -> None:
        source = (ROOT / "src" / "simulation" / "OmniNuclear.cpp").read_text(
            encoding="utf-8"
        )
        errors: list[str] = []
        original = nuclear_audit.read_text
        try:
            nuclear_audit.read_text = lambda _path, _errors: source.replace(
                "NuclearEventsPerFrame = 512", "NuclearEventsPerFrame = 0"
            )
            nuclear_audit.check_engine(ROOT, errors)
        finally:
            nuclear_audit.read_text = original
        self.assertTrue(any("per-frame event budget" in error for error in errors))

    def test_generator_without_fuel_guard_is_rejected(self) -> None:
        source = (ROOT / "src" / "simulation" / "OmniNuclear.cpp").read_text(
            encoding="utf-8"
        )
        errors: list[str] = []
        original = nuclear_audit.read_text
        try:
            nuclear_audit.read_text = lambda _path, _errors: source.replace(
                "if (fuel.index < 0)", "if (fuel.index == -2)"
            )
            nuclear_audit.check_engine(ROOT, errors)
        finally:
            nuclear_audit.read_text = original
        self.assertTrue(any("fuel requirement" in error for error in errors))

    def test_generator_single_emission_guard_is_rejected(self) -> None:
        source = (ROOT / "src" / "simulation" / "OmniNuclear.cpp").read_text(
            encoding="utf-8"
        )
        errors: list[str] = []
        original = nuclear_audit.read_text
        try:
            nuclear_audit.read_text = lambda _path, _errors: source.replace(
                "|| (parts[i].tmp4 & NuclearSparkEmitted)", ""
            )
            nuclear_audit.check_engine(ROOT, errors)
        finally:
            nuclear_audit.read_text = original
        self.assertTrue(any("one neutron per spark guard" in error for error in errors))

    def test_missing_waste_stabilization_is_rejected(self) -> None:
        source = (ROOT / "src" / "simulation" / "OmniNuclear.cpp").read_text(
            encoding="utf-8"
        )
        errors: list[str] = []
        original = nuclear_audit.read_text
        try:
            nuclear_audit.read_text = lambda _path, _errors: source.replace(
                "WasteStabilization", "RemovedWastePath"
            )
            nuclear_audit.check_engine(ROOT, errors)
        finally:
            nuclear_audit.read_text = original
        self.assertTrue(any("four-module waste stabilization" in error for error in errors))


if __name__ == "__main__":
    unittest.main()
