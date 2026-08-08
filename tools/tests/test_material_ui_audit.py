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


if __name__ == "__main__":
    unittest.main()
