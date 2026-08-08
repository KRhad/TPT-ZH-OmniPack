from __future__ import annotations

import csv
import importlib.util
from pathlib import Path
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / "tools" / "generate_periodic_content_links.py"
SPEC = importlib.util.spec_from_file_location("generate_periodic_content_links", SCRIPT)
assert SPEC is not None and SPEC.loader is not None
module = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(module)


class GeneratePeriodicContentLinksTests(unittest.TestCase):
    def test_repository_indices_render_one_record_per_material(self) -> None:
        rendered = module.render(
            ROOT / "docs" / "PERIODIC_CONTENT_LINKS.csv",
            ROOT / "docs" / "MATERIAL_PERIODIC_INDEX.csv",
            ROOT / "docs" / "ELEMENT_REGISTRY.csv",
        )
        self.assertIn("std::array<PeriodicContentLink, 276>", rendered)
        self.assertEqual(rendered.count("PeriodicContentKind::RelatedMaterial"), 93)
        self.assertIn('"OMNI_PT_EACT", 621, PeriodicContentKind::RelatedMaterial', rendered)
        self.assertIn('"OMNI_PT_CFRP", 633, PeriodicContentKind::RelatedMaterial', rendered)

    def test_related_stable_id_mismatch_is_rejected(self) -> None:
        registry = module.read_registry(ROOT / "docs" / "ELEMENT_REGISTRY.csv")
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "related.csv"
            with (ROOT / "docs" / "MATERIAL_PERIODIC_INDEX.csv").open(
                "r", encoding="utf-8-sig", newline=""
            ) as source:
                rows = list(csv.DictReader(source))
            rows[0]["stable_id"] = "999"
            with path.open("w", encoding="utf-8", newline="") as stream:
                writer = csv.DictWriter(stream, fieldnames=module.RELATED_FIELDS)
                writer.writeheader()
                writer.writerows(rows)
            with self.assertRaisesRegex(ValueError, "stable ID does not match registry"):
                module.read_related_rows(path, registry)


if __name__ == "__main__":
    unittest.main()
