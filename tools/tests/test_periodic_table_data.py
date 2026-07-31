from __future__ import annotations

import importlib.util
from pathlib import Path
import sys
import unittest


ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / "tools" / "generate_periodic_table_data.py"
SPEC = importlib.util.spec_from_file_location("generate_periodic_table_data", SCRIPT)
assert SPEC and SPEC.loader
generator = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = generator
SPEC.loader.exec_module(generator)


class PeriodicTableDataTests(unittest.TestCase):
    def test_runtime_model_contains_all_118_elements(self) -> None:
        source_map = ROOT / "docs" / "PERIODIC_ELEMENT_SOURCE_MAP.csv"
        rendered = generator.render(source_map)
        self.assertIn("std::array<PeriodicElementRecord, 118>", rendered)
        self.assertEqual(rendered.count("\n\t{ "), 118)
        self.assertIn('"He", "氦", "Helium", "OMNI_PT_HE", 370', rendered)
        self.assertIn('"Og", "鿫", "Oganesson", "OMNI_PT_OG", 461', rendered)

    def test_long_form_positions_and_filters_are_stable(self) -> None:
        self.assertEqual(generator.GROUPS[1], 1)
        self.assertEqual(generator.GROUPS[2], 18)
        self.assertEqual(generator.GROUPS[57], 3)
        self.assertEqual(generator.GROUPS[58], 0)
        self.assertEqual(generator.table_position(58, 6, 0), (7, 3))
        self.assertEqual(generator.table_position(90, 7, 0), (8, 3))
        self.assertEqual(generator.state_for(80), "Liquid")
        self.assertEqual(generator.class_for(14), "Metalloid")
        self.assertIn(86, generator.RADIOACTIVE)

    def test_implementation_requires_identifier(self) -> None:
        rows = generator.read_source_map(
            ROOT / "docs" / "PERIODIC_ELEMENT_SOURCE_MAP.csv"
        )
        implemented = [row for row in rows if row["status"] == "implemented"]
        self.assertEqual(len(implemented), 55)
        self.assertTrue(all(row["official_mapping"] or row["omnipack_mapping"] for row in implemented))

    def test_generator_refuses_to_overwrite_header(self) -> None:
        with self.assertRaisesRegex(ValueError, r"generated \.cpp"):
            generator.validate_output_path(
                ROOT / "src" / "simulation" / "PeriodicTableData.h"
            )


if __name__ == "__main__":
    unittest.main()
