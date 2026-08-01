from __future__ import annotations

import importlib.util
from pathlib import Path
import sys
import unittest


SCRIPT = Path(__file__).resolve().parents[1] / "element_capacity_audit.py"
SPEC = importlib.util.spec_from_file_location("element_capacity_audit", SCRIPT)
assert SPEC is not None and SPEC.loader is not None
element_capacity_audit = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = element_capacity_audit
SPEC.loader.exec_module(element_capacity_audit)

ROOT = Path(__file__).resolve().parents[2]


class ElementCapacityAuditTests(unittest.TestCase):
    def test_repository_passes(self) -> None:
        self.assertEqual(element_capacity_audit.audit(ROOT), [])

    def test_reverting_to_nine_bits_is_rejected(self) -> None:
        path = ROOT / "src/simulation/ElementDefs.h"
        source = path.read_text(encoding="utf-8")
        original = element_capacity_audit.read_text
        errors: list[str] = []
        try:
            element_capacity_audit.read_text = lambda candidate, current: (
                source.replace("PMAPBITS = 10", "PMAPBITS = 9")
                if candidate == path
                else original(candidate, current)
            )
            element_capacity_audit.check_constants(ROOT, errors)
        finally:
            element_capacity_audit.read_text = original
        self.assertTrue(any("10-bit pmap" in error for error in errors))

    def test_missing_corrupt_width_guard_is_rejected(self) -> None:
        path = ROOT / "src/client/GameSave.cpp"
        source = path.read_text(encoding="utf-8")
        original = element_capacity_audit.read_text
        errors: list[str] = []
        try:
            element_capacity_audit.read_text = lambda candidate, current: (
                source.replace(
                    "pmapbits > MaximumOpsPmapBits",
                    "false /* removed upper guard */",
                    1,
                )
                if candidate == path
                else original(candidate, current)
            )
            element_capacity_audit.check_save_path(ROOT, errors)
        finally:
            element_capacity_audit.read_text = original
        self.assertTrue(any("upper bound" in error for error in errors))

    def test_direct_type_must_map_before_current_range_filter(self) -> None:
        path = ROOT / "src/client/GameSave.cpp"
        source = path.read_text(encoding="utf-8")
        original = element_capacity_audit.read_text
        errors: list[str] = []
        try:
            element_capacity_audit.read_text = lambda candidate, current: (
                source.replace(
                    "tempPart.type = paletteLookup(tempPart.type, false);",
                    "/* direct mapping removed */",
                    1,
                )
                if candidate == path
                else original(candidate, current)
            )
            element_capacity_audit.check_save_path(ROOT, errors)
        finally:
            element_capacity_audit.read_text = original
        self.assertTrue(any("direct type maps" in error for error in errors))

    def test_source_palette_must_use_declared_width(self) -> None:
        path = ROOT / "src/client/GameSave.cpp"
        source = path.read_text(encoding="utf-8")
        original = element_capacity_audit.read_text
        errors: list[str] = []
        try:
            element_capacity_audit.read_text = lambda candidate, current: (
                source.replace(
                    "std::vector<int> partMap(UINT32_C(1) << pmapbits, 0);",
                    "std::vector<int> partMap(PT_NUM, 0);",
                    1,
                )
                if candidate == path
                else original(candidate, current)
            )
            element_capacity_audit.check_save_path(ROOT, errors)
        finally:
            element_capacity_audit.read_text = original
        self.assertTrue(any("source-width palette lookup" in error for error in errors))

    def test_high_slot_drift_is_rejected(self) -> None:
        path = ROOT / "src/simulation/elements/meson.build"
        source = path.read_text(encoding="utf-8")
        original = element_capacity_audit.read_text
        errors: list[str] = []
        try:
            element_capacity_audit.read_text = lambda candidate, current: (
                source.replace("'SOLD', # 512", "disabler(), # 512", 1)
                if candidate == path
                else original(candidate, current)
            )
            element_capacity_audit.check_registration(ROOT, errors)
        finally:
            element_capacity_audit.read_text = original
        self.assertTrue(any("slot 512" in error for error in errors))


if __name__ == "__main__":
    unittest.main()
