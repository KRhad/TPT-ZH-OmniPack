from __future__ import annotations

import csv
import importlib.util
from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location(
    "generate_periodic_source_map", ROOT / "tools" / "generate_periodic_source_map.py"
)
assert SPEC and SPEC.loader
generator = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(generator)


class PeriodicSourceMapTests(unittest.TestCase):
    def test_generated_map_is_current_and_has_stable_ids(self) -> None:
        rendered = generator.generate(ROOT)
        path = ROOT / "docs" / "PERIODIC_ELEMENT_SOURCE_MAP.csv"
        self.assertEqual(path.read_text(encoding="utf-8"), rendered)
        rows = list(csv.DictReader(rendered.splitlines()))
        self.assertEqual(len(rows), 118)
        self.assertEqual([int(row["atomic_number"]) for row in rows], list(range(1, 119)))
        stable_ids = [int(row["stable_id"]) for row in rows]
        self.assertEqual(len(stable_ids), len(set(stable_ids)))
        generated = [row for row in rows if row["implementation_type"] == "family_generated"]
        self.assertEqual(len(generated), 92)
        self.assertEqual([int(row["stable_id"]) for row in generated], list(range(370, 462)))

    def test_generic_materials_are_not_claimed_as_pure_elements(self) -> None:
        rows = list(csv.DictReader(generator.generate(ROOT).splitlines()))
        mappings = {row["official_mapping"] for row in rows}
        self.assertNotIn("DEFAULT_PT_NBLE", mappings)
        self.assertNotIn("DEFAULT_PT_METL", mappings)
        self.assertNotIn("DEFAULT_PT_LNTG", mappings)


if __name__ == "__main__":
    unittest.main()
