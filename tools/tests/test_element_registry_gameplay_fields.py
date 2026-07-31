from __future__ import annotations

import csv
import importlib.util
from pathlib import Path
import sys
import tempfile
import unittest
from unittest import mock


ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / "tools" / "element_registry_check.py"
SPEC = importlib.util.spec_from_file_location("element_registry_gameplay_check", SCRIPT)
assert SPEC is not None and SPEC.loader is not None
element_registry_check = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = element_registry_check
SPEC.loader.exec_module(element_registry_check)


class ElementRegistryGameplayFieldsTest(unittest.TestCase):
    def validate_rows(self, mutate) -> set[str]:
        source = ROOT / "docs" / "ELEMENT_REGISTRY.csv"
        with source.open(encoding="utf-8", newline="") as stream:
            reader = csv.DictReader(stream)
            rows = list(reader)
        mutate(rows)
        with tempfile.TemporaryDirectory() as temporary:
            registry = Path(temporary) / "ELEMENT_REGISTRY.csv"
            with registry.open("w", encoding="utf-8", newline="") as stream:
                writer = csv.DictWriter(
                    stream,
                    fieldnames=element_registry_check.REGISTRY_COLUMNS,
                    lineterminator="\n",
                )
                writer.writeheader()
                writer.writerows(rows)
            findings, _ = element_registry_check.validate_repository(
                ROOT,
                registry,
                ROOT / "tools" / "data" / "official_elements_100_0.csv",
            )
        return {finding.code for finding in findings.errors}

    def test_current_registry_passes(self) -> None:
        findings, stats = element_registry_check.validate_repository(
            ROOT,
            ROOT / "docs" / "ELEMENT_REGISTRY.csv",
            ROOT / "tools" / "data" / "official_elements_100_0.csv",
        )
        self.assertEqual([], findings.errors)
        self.assertEqual(462, stats["registry_rows"])

    def test_alias_drift_is_rejected(self) -> None:
        def mutate(rows):
            next(row for row in rows if row["identifier"] == "OMNI_PT_ALUM")[
                "code"
            ] = "BROKEN"

        self.assertIn("REGISTRY_ALIAS", self.validate_rows(mutate))

    def test_missing_omnipack_cleanup_is_rejected(self) -> None:
        def mutate(rows):
            next(row for row in rows if row["identifier"] == "OMNI_PT_PATH")[
                "cleanup"
            ] = ""

        self.assertIn("REGISTRY_GAMEPLAY", self.validate_rows(mutate))

    def test_official_gameplay_claim_is_rejected(self) -> None:
        def mutate(rows):
            next(row for row in rows if row["identifier"] == "DEFAULT_PT_DUST")[
                "production"
            ] = "Place dust."

        self.assertIn("REGISTRY_UPSTREAM_GAMEPLAY", self.validate_rows(mutate))

    def test_reserved_slot_claim_is_rejected(self) -> None:
        def mutate(rows):
            next(row for row in rows if row["identifier"] == "RESERVED_PT_196")[
                "uses"
            ] = "Future element."

        self.assertIn("REGISTRY_RESERVED_GAMEPLAY", self.validate_rows(mutate))

    def test_omnipack_description_without_element_name_is_rejected(self) -> None:
        def mutate(rows):
            next(row for row in rows if row["identifier"] == "OMNI_PT_ALUM")[
                "english_description"
            ] = "Light conductive aluminium."

        self.assertIn("REGISTRY_DESCRIPTION_PREFIX", self.validate_rows(mutate))

    def test_omnipack_registry_and_language_description_drift_is_rejected(self) -> None:
        def mutate(rows):
            next(row for row in rows if row["identifier"] == "OMNI_PT_ALUM")[
                "english_description"
            ] = "Aluminium: Deliberate drift for the negative test."

        self.assertIn(
            "REGISTRY_LANGUAGE_DESCRIPTION",
            self.validate_rows(mutate),
        )

    def test_omnipack_language_name_and_description_drift_is_rejected(self) -> None:
        original_load_language = element_registry_check.load_language

        def load_language(path, findings):
            catalog = original_load_language(path, findings)
            if path.name == "zh-CN.json":
                catalog["sim.elem.OMNI_PT_ALUM.name"] = "错误名称"
                catalog["sim.elem.OMNI_PT_ALUM"] = "铝：故意制造语言包漂移。"
            return catalog

        with mock.patch.object(
            element_registry_check,
            "load_language",
            side_effect=load_language,
        ):
            findings, _ = element_registry_check.validate_repository(
                ROOT,
                ROOT / "docs" / "ELEMENT_REGISTRY.csv",
                ROOT / "tools" / "data" / "official_elements_100_0.csv",
            )
        codes = {finding.code for finding in findings.errors}
        self.assertIn("REGISTRY_LANGUAGE_NAME", codes)
        self.assertIn("REGISTRY_LANGUAGE_DESCRIPTION", codes)


if __name__ == "__main__":
    unittest.main()
