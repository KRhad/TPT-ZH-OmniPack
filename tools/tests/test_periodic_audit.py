from __future__ import annotations

import importlib.util
from pathlib import Path
import sys
import unittest


ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / "tools" / "periodic_audit.py"
SPEC = importlib.util.spec_from_file_location("periodic_audit", SCRIPT)
assert SPEC and SPEC.loader
periodic_audit = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = periodic_audit
SPEC.loader.exec_module(periodic_audit)


class PeriodicAuditTests(unittest.TestCase):
    def test_repository_passes(self) -> None:
        self.assertEqual(periodic_audit.audit(ROOT), [])

    def test_missing_budget_is_rejected(self) -> None:
        engine_path = ROOT / "src" / "simulation" / "OmniPeriodic.cpp"
        source = engine_path.read_text(encoding="utf-8")
        original = periodic_audit.read_text
        errors: list[str] = []
        try:
            periodic_audit.read_text = lambda path, output: (
                source.replace("PeriodicEventsPerFrame = 1024", "PeriodicEventsPerFrame = 0")
                if path == engine_path else original(path, output)
            )
            periodic_audit.check_engine(ROOT, errors)
        finally:
            periodic_audit.read_text = original
        self.assertTrue(any("single-frame budget" in error for error in errors))

    def test_missing_planned_ui_state_is_rejected(self) -> None:
        ui_path = ROOT / "src" / "gui" / "periodictable" / "PeriodicTableActivity.cpp"
        source = ui_path.read_text(encoding="utf-8")
        original = periodic_audit.read_text
        errors: list[str] = []
        try:
            periodic_audit.read_text = lambda path, output: (
                source.replace("periodic.status.planned", "removed.status")
                if path == ui_path else original(path, output)
            )
            periodic_audit.check_ui(ROOT, errors)
        finally:
            periodic_audit.read_text = original
        self.assertTrue(any("planned state" in error for error in errors))

    def test_missing_molten_alkali_hook_is_rejected(self) -> None:
        lava_path = ROOT / "src" / "simulation" / "elements" / "LAVA.cpp"
        source = lava_path.read_text(encoding="utf-8")
        original = periodic_audit.read_text
        errors: list[str] = []
        try:
            periodic_audit.read_text = lambda path, output: (
                source.replace("OmniMoltenAlkaliUpdate", "removedAlkaliHook")
                if path == lava_path else original(path, output)
            )
            periodic_audit.check_engine(ROOT, errors)
        finally:
            periodic_audit.read_text = original
        self.assertTrue(any("molten alkali" in error for error in errors))

    def test_missing_molten_alkaline_earth_hook_is_rejected(self) -> None:
        lava_path = ROOT / "src" / "simulation" / "elements" / "LAVA.cpp"
        source = lava_path.read_text(encoding="utf-8")
        original = periodic_audit.read_text
        errors: list[str] = []
        try:
            periodic_audit.read_text = lambda path, output: (
                source.replace(
                    "OmniMoltenAlkalineEarthUpdate", "removedAlkalineEarthHook"
                )
                if path == lava_path else original(path, output)
            )
            periodic_audit.check_engine(ROOT, errors)
        finally:
            periodic_audit.read_text = original
        self.assertTrue(any("molten alkaline-earth" in error for error in errors))

    def test_missing_molten_boron_group_hook_is_rejected(self) -> None:
        lava_path = ROOT / "src" / "simulation" / "elements" / "LAVA.cpp"
        source = lava_path.read_text(encoding="utf-8")
        original = periodic_audit.read_text
        errors: list[str] = []
        try:
            periodic_audit.read_text = lambda path, output: (
                source.replace(
                    "OmniMoltenBoronGroupUpdate", "removedBoronGroupHook"
                )
                if path == lava_path else original(path, output)
            )
            periodic_audit.check_engine(ROOT, errors)
        finally:
            periodic_audit.read_text = original
        self.assertTrue(any("molten boron-group" in error for error in errors))

    def test_missing_molten_carbon_group_hook_is_rejected(self) -> None:
        lava_path = ROOT / "src" / "simulation" / "elements" / "LAVA.cpp"
        source = lava_path.read_text(encoding="utf-8")
        original = periodic_audit.read_text
        errors: list[str] = []
        try:
            periodic_audit.read_text = lambda path, output: (
                source.replace(
                    "OmniMoltenCarbonGroupUpdate", "removedCarbonGroupHook"
                )
                if path == lava_path else original(path, output)
            )
            periodic_audit.check_engine(ROOT, errors)
        finally:
            periodic_audit.read_text = original
        self.assertTrue(any("molten carbon-group" in error for error in errors))

    def test_missing_molten_nitrogen_group_hook_is_rejected(self) -> None:
        lava_path = ROOT / "src" / "simulation" / "elements" / "LAVA.cpp"
        source = lava_path.read_text(encoding="utf-8")
        original = periodic_audit.read_text
        errors: list[str] = []
        try:
            periodic_audit.read_text = lambda path, output: (
                source.replace(
                    "OmniMoltenNitrogenGroupUpdate", "removedNitrogenGroupHook"
                )
                if path == lava_path else original(path, output)
            )
            periodic_audit.check_engine(ROOT, errors)
        finally:
            periodic_audit.read_text = original
        self.assertTrue(any("molten nitrogen-group" in error for error in errors))

    def test_missing_molten_oxygen_group_hook_is_rejected(self) -> None:
        lava_path = ROOT / "src" / "simulation" / "elements" / "LAVA.cpp"
        source = lava_path.read_text(encoding="utf-8")
        original = periodic_audit.read_text
        errors: list[str] = []
        try:
            periodic_audit.read_text = lambda path, output: (
                source.replace(
                    "OmniMoltenOxygenGroupUpdate", "removedOxygenGroupHook"
                )
                if path == lava_path else original(path, output)
            )
            periodic_audit.check_engine(ROOT, errors)
        finally:
            periodic_audit.read_text = original
        self.assertTrue(any("molten oxygen-group" in error for error in errors))

    def test_missing_molten_halogen_hook_is_rejected(self) -> None:
        lava_path = ROOT / "src" / "simulation" / "elements" / "LAVA.cpp"
        source = lava_path.read_text(encoding="utf-8")
        original = periodic_audit.read_text
        errors: list[str] = []
        try:
            periodic_audit.read_text = lambda path, output: (
                source.replace("OmniMoltenHalogenUpdate", "removedHalogenHook")
                if path == lava_path else original(path, output)
            )
            periodic_audit.check_engine(ROOT, errors)
        finally:
            periodic_audit.read_text = original
        self.assertTrue(any("molten halogen" in error for error in errors))

    def test_missing_molten_first_transition_hook_is_rejected(self) -> None:
        lava_path = ROOT / "src" / "simulation" / "elements" / "LAVA.cpp"
        source = lava_path.read_text(encoding="utf-8")
        original = periodic_audit.read_text
        errors: list[str] = []
        try:
            periodic_audit.read_text = lambda path, output: (
                source.replace(
                    "OmniMoltenFirstTransitionUpdate", "removedFirstTransitionHook"
                )
                if path == lava_path else original(path, output)
            )
            periodic_audit.check_engine(ROOT, errors)
        finally:
            periodic_audit.read_text = original
        self.assertTrue(any("molten first-transition" in error for error in errors))

    def test_missing_molten_second_transition_hook_is_rejected(self) -> None:
        lava_path = ROOT / "src" / "simulation" / "elements" / "LAVA.cpp"
        source = lava_path.read_text(encoding="utf-8")
        original = periodic_audit.read_text
        errors: list[str] = []
        try:
            periodic_audit.read_text = lambda path, output: (
                source.replace(
                    "OmniMoltenSecondTransitionUpdate", "removedSecondTransitionHook"
                )
                if path == lava_path else original(path, output)
            )
            periodic_audit.check_engine(ROOT, errors)
        finally:
            periodic_audit.read_text = original
        self.assertTrue(any("molten second-transition" in error for error in errors))


if __name__ == "__main__":
    unittest.main()
