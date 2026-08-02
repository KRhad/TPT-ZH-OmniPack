from __future__ import annotations

import importlib.util
from pathlib import Path
import sys
import tempfile
import unittest


SCRIPT = Path(__file__).resolve().parents[1] / "element_content_audit.py"
SPEC = importlib.util.spec_from_file_location("element_content_audit", SCRIPT)
assert SPEC is not None and SPEC.loader is not None
element_content_audit = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = element_content_audit
SPEC.loader.exec_module(element_content_audit)

ROOT = Path(__file__).resolve().parents[2]


class ElementContentAuditTests(unittest.TestCase):
    def test_repository_passes(self) -> None:
        self.assertEqual(element_content_audit.audit(ROOT), [])

    def test_missing_implemented_element_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            docs = root / "docs"
            docs.mkdir()
            (docs / "ELEMENT_REGISTRY.csv").write_bytes(
                (ROOT / "docs" / "ELEMENT_REGISTRY.csv").read_bytes()
            )
            content = (ROOT / "docs" / "ELEMENT_CONTENT.csv").read_text(
                encoding="utf-8"
            )
            retained = [line for line in content.splitlines() if "OMNI_PT_ALUM," not in line]
            (docs / "ELEMENT_CONTENT.csv").write_text(
                "\n".join(retained) + "\n", encoding="utf-8", newline=""
            )
            errors = element_content_audit.audit(root)
        self.assertTrue(any("missing implemented OmniPack elements" in error for error in errors))

    def test_unknown_element_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            docs = root / "docs"
            docs.mkdir()
            (docs / "ELEMENT_REGISTRY.csv").write_bytes(
                (ROOT / "docs" / "ELEMENT_REGISTRY.csv").read_bytes()
            )
            content = (ROOT / "docs" / "ELEMENT_CONTENT.csv").read_text(
                encoding="utf-8"
            )
            extra = (
                "OMNI_PT_UNKNOWN,Recipe,配方,Production,生产,Use,用途,Hazard,危险\n"
            )
            (docs / "ELEMENT_CONTENT.csv").write_text(
                content + extra, encoding="utf-8", newline=""
            )
            errors = element_content_audit.audit(root)
        self.assertTrue(any("has no implemented OmniPack element" in error for error in errors))

    def test_noncanonical_identifier_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            docs = root / "docs"
            docs.mkdir()
            (docs / "ELEMENT_REGISTRY.csv").write_bytes(
                (ROOT / "docs" / "ELEMENT_REGISTRY.csv").read_bytes()
            )
            content = (ROOT / "docs" / "ELEMENT_CONTENT.csv").read_text(
                encoding="utf-8"
            )
            (docs / "ELEMENT_CONTENT.csv").write_text(
                content.replace("OMNI_PT_ALUM,", "omni_PT_ALUM,", 1),
                encoding="utf-8",
                newline="",
            )
            errors = element_content_audit.audit(root)
        self.assertTrue(any("canonical registry spelling" in error for error in errors))

    def test_empty_bilingual_production_pair_is_allowed(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            content = Path(temp) / "content.csv"
            content.write_text(
                "identifier,recipe_en,recipe_zh,production_en,production_zh,use_en,use_zh,hazard_en,hazard_zh\n"
                "OMNI_PT_TEST,Recipe,配方,,,Use,用途,Hazard,危险\n",
                encoding="utf-8",
                newline="",
            )
            _, errors = element_content_audit.element_content_catalog.read_content_records(content)
        self.assertEqual([], errors)

    def test_one_sided_production_pair_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            content = Path(temp) / "content.csv"
            content.write_text(
                "identifier,recipe_en,recipe_zh,production_en,production_zh,use_en,use_zh,hazard_en,hazard_zh\n"
                "OMNI_PT_TEST,Recipe,配方,Production,,Use,用途,Hazard,危险\n",
                encoding="utf-8",
                newline="",
            )
            _, errors = element_content_audit.element_content_catalog.read_content_records(content)
        self.assertTrue(any("must both be empty or both be populated" in error for error in errors))


if __name__ == "__main__":
    unittest.main()
