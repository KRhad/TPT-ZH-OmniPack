from __future__ import annotations

import importlib.util
from pathlib import Path
import sys
import unittest


SCRIPT = Path(__file__).resolve().parents[1] / "metallurgy_audit.py"
SPEC = importlib.util.spec_from_file_location("metallurgy_audit", SCRIPT)
assert SPEC is not None and SPEC.loader is not None
metallurgy_audit = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = metallurgy_audit
SPEC.loader.exec_module(metallurgy_audit)

ROOT = Path(__file__).resolve().parents[2]


class MetallurgyAuditTests(unittest.TestCase):
    def test_repository_passes(self) -> None:
        self.assertEqual(metallurgy_audit.audit(ROOT), [])

    def test_wrong_bronze_ratio_is_rejected(self) -> None:
        source = (
            ROOT / "src" / "simulation" / "OmniMetallurgy.cpp"
        ).read_text(encoding="utf-8")
        mutated = source.replace(
            "Ingredient{ PT_COPR, 3 }",
            "Ingredient{ PT_COPR, 2 }",
            1,
        )
        errors: list[str] = []
        metallurgy_audit.parse_alloy_recipes(mutated, errors)
        self.assertTrue(errors)
        self.assertIn("audited stoichiometry", errors[-1])


if __name__ == "__main__":
    unittest.main()

