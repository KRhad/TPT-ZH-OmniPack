from __future__ import annotations

import importlib.util
from pathlib import Path
import sys
import unittest


ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / "tools" / "organic_audit.py"
SPEC = importlib.util.spec_from_file_location("organic_audit", SCRIPT)
assert SPEC is not None and SPEC.loader is not None
organic_audit = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = organic_audit
SPEC.loader.exec_module(organic_audit)


class OrganicAuditTest(unittest.TestCase):
    def test_repository_passes(self) -> None:
        self.assertEqual([], organic_audit.audit(ROOT))

    def test_stable_range_is_contiguous(self) -> None:
        self.assertEqual(list(range(589, 602)), list(organic_audit.EXPECTED))

    def test_polyethylene_reuses_poly(self) -> None:
        rows = organic_audit.read_csv(ROOT / "docs" / "ELEMENT_REGISTRY.csv", [])
        poly = next(row for row in rows if row["identifier"] == "OMNI_PT_POLY")
        self.assertEqual("367", poly["stable_id"])
        self.assertEqual("Polyethylene", poly["english_name"])

    def test_reaction_set_is_complete(self) -> None:
        self.assertEqual(15, len(organic_audit.REACTIONS))


if __name__ == "__main__":
    unittest.main()
