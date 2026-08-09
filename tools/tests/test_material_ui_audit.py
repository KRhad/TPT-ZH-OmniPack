from __future__ import annotations

import csv
import importlib.util
import io
from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / "tools" / "generate_material_ui_audit.py"
SPEC = importlib.util.spec_from_file_location("generate_material_ui_audit", SCRIPT)
assert SPEC is not None and SPEC.loader is not None
module = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(module)


class MaterialUiAuditTests(unittest.TestCase):
    def test_repository_audit_covers_every_implemented_slot(self) -> None:
        rendered, stats = module.render(ROOT)
        rows = list(csv.DictReader(io.StringIO(rendered)))
        self.assertEqual(stats["implemented_slots"], 488)
        self.assertEqual(stats["canonical"], 487)
        self.assertEqual(stats["aliases"], 1)
        self.assertEqual(stats["periodic_before"], 183)
        self.assertEqual(stats["periodic_after"], 276)
        self.assertEqual(stats["ordinary_menu_relocated"], 18)
        self.assertEqual(len(rows), 488)
        self.assertEqual(len({row["stable_id"] for row in rows}), 488)

    def test_supplemental_routes_reuse_the_canonical_record(self) -> None:
        rendered, _ = module.render(ROOT)
        rows = {
            row["identifier"]: row for row in csv.DictReader(io.StringIO(rendered))
        }
        ethanol = rows["OMNI_PT_ETHL"]
        self.assertEqual(ethanol["stable_id"], "362")
        self.assertNotIn("periodic_table", ethanol["current_rc9_entry"])
        self.assertIn("periodic_table", ethanol["target_entry"])
        self.assertEqual(ethanol["periodic_atomic_numbers"], "1|6|8")
        alias = rows["OMNI_PT_MSCR"]
        self.assertEqual(alias["record_status"], "compatibility_alias")
        self.assertEqual(alias["canonical_identifier"], "DEFAULT_PT_BRMT")

    def test_material_routes_follow_declared_menu_policy(self) -> None:
        rendered, _ = module.render(ROOT)
        rows = list(csv.DictReader(io.StringIO(rendered)))
        by_policy: dict[str, list[dict[str, str]]] = {}
        for row in rows:
            by_policy.setdefault(row["main_menu_policy"], []).append(row)

        periodic_only = by_policy["periodic_only"]
        self.assertEqual(len(periodic_only), 165)
        self.assertTrue(all(
            row["target_entry"] == "periodic_table" for row in periodic_only
        ))

        organic = by_policy["organic_menu"]
        self.assertEqual(len(organic), 38)
        self.assertTrue(all(
            row["target_entry"] == "material_library:organic+periodic_table"
            for row in organic
        ))

        alloy = by_policy["alloy_menu"]
        self.assertEqual(len(alloy), 55)
        self.assertTrue(all(
            row["target_entry"]
            == "material_library:alloy_engineering+periodic_table"
            for row in alloy
        ))

        hidden = by_policy["hidden"]
        self.assertEqual(len(hidden), 1)
        self.assertEqual(hidden[0]["target_entry"], "hidden_or_runtime_only")


if __name__ == "__main__":
    unittest.main()
