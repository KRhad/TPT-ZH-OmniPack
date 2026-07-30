from __future__ import annotations

import csv
import importlib.util
from pathlib import Path
import sys
import tempfile
import unittest


SCRIPT = Path(__file__).resolve().parents[1] / "reaction_registry_check.py"
SPEC = importlib.util.spec_from_file_location("reaction_registry_check", SCRIPT)
assert SPEC is not None and SPEC.loader is not None
reaction_registry_check = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = reaction_registry_check
SPEC.loader.exec_module(reaction_registry_check)

ROOT = Path(__file__).resolve().parents[2]
REGISTRY = ROOT / "docs" / "REACTION_REGISTRY.csv"


def read_rows() -> list[dict[str, str]]:
    with REGISTRY.open("r", encoding="utf-8", newline="") as stream:
        return list(csv.DictReader(stream))


def audit_rows(rows: list[dict[str, str]]) -> list[str]:
    with tempfile.TemporaryDirectory() as temp:
        path = Path(temp) / "REACTION_REGISTRY.csv"
        with path.open("w", encoding="utf-8", newline="") as stream:
            writer = csv.DictWriter(
                stream, fieldnames=reaction_registry_check.REGISTRY_COLUMNS
            )
            writer.writeheader()
            writer.writerows(rows)
        return reaction_registry_check.audit(ROOT, path)


class ReactionRegistryCheckTests(unittest.TestCase):
    def test_repository_passes(self) -> None:
        self.assertEqual(reaction_registry_check.audit(ROOT), [])

    def test_requested_fields_are_required(self) -> None:
        for field in (
            "reaction_id",
            "inputs",
            "outputs",
            "module",
            "tests",
            "event_budget",
            "failure_mode",
            "cleanup",
        ):
            with self.subTest(field=field):
                rows = read_rows()
                rows[0][field] = ""
                errors = audit_rows(rows)
                self.assertTrue(
                    any(field in error for error in errors),
                    f"blank {field} was not rejected: {errors}",
                )

    def test_duplicate_reaction_id_is_rejected(self) -> None:
        rows = read_rows()
        rows[1]["reaction_id"] = rows[0]["reaction_id"]
        errors = audit_rows(rows)
        self.assertTrue(any("duplicate reaction_id" in error for error in errors))

    def test_unknown_input_element_is_rejected(self) -> None:
        rows = read_rows()
        rows[0]["inputs"] = "PT_DOESNOTEXIST"
        errors = audit_rows(rows)
        self.assertTrue(any("unknown elements" in error for error in errors))

    def test_malformed_material_expression_is_rejected(self) -> None:
        rows = read_rows()
        rows[0]["outputs"] = "PT_SSIL**6"
        errors = audit_rows(rows)
        self.assertTrue(any("invalid material tokens" in error for error in errors))

    def test_invalid_probability_is_rejected(self) -> None:
        rows = read_rows()
        rows[0]["probability"] = "2/1"
        errors = audit_rows(rows)
        self.assertTrue(any("invalid probability" in error for error in errors))

    def test_module_budget_pair_is_rejected(self) -> None:
        rows = read_rows()
        rows[0]["event_budget"] = "512/frame"
        errors = audit_rows(rows)
        self.assertTrue(any("event_budget" in error for error in errors))

    def test_missing_test_evidence_is_rejected(self) -> None:
        rows = read_rows()
        rows[0]["tests"] = "tools/tests/does_not_exist.py"
        errors = audit_rows(rows)
        self.assertTrue(any("test evidence does not exist" in error for error in errors))

    def test_missing_current_source_reaction_is_rejected(self) -> None:
        rows = read_rows()[1:]
        errors = audit_rows(rows)
        self.assertTrue(any("missing current source reactions" in error for error in errors))


if __name__ == "__main__":
    unittest.main()
