from __future__ import annotations

import copy
import importlib.util
import json
from pathlib import Path
import sys
import unittest


SCRIPT = Path(__file__).resolve().parents[1] / "alchemy_audit.py"
SPEC = importlib.util.spec_from_file_location("alchemy_audit", SCRIPT)
assert SPEC is not None and SPEC.loader is not None
alchemy_audit = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = alchemy_audit
SPEC.loader.exec_module(alchemy_audit)

ROOT = Path(__file__).resolve().parents[2]
DOCUMENT = json.loads((ROOT / "docs" / "ALCHEMY_PROGRESSION.json").read_text(encoding="utf-8"))


def validate(document: dict) -> list[str]:
    errors: list[str] = []
    registry = alchemy_audit.read_registry(ROOT / "docs" / "ELEMENT_REGISTRY.csv", errors)
    languages = {
        name: alchemy_audit.read_json(ROOT / "src" / "lang" / f"{name}.json", errors)
        for name in ("en-US", "zh-CN")
    }
    errors.extend(alchemy_audit.validate_document(ROOT, document, registry, languages))
    return errors


class AlchemyAuditTests(unittest.TestCase):
    def test_repository_passes(self) -> None:
        self.assertEqual(alchemy_audit.audit(ROOT), [])

    def test_missing_stage_is_rejected(self) -> None:
        document = copy.deepcopy(DOCUMENT)
        document["stages"].pop()
        self.assertTrue(any("exactly ten stages" in error for error in validate(document)))

    def test_forward_dependency_and_dead_input_are_rejected(self) -> None:
        document = copy.deepcopy(DOCUMENT)
        document["stages"][0]["inputs"].append("DEFAULT_PT_DMND")
        document["stages"][0]["conditions"].append(
            {"type": "multi_stage", "requires": ["A10-MASTERY-LOOP"]}
        )
        errors = validate(document)
        self.assertTrue(any("unreachable inputs" in error for error in errors))
        self.assertTrue(any("forward or unknown dependency" in error for error in errors))

    def test_numeric_id_is_rejected(self) -> None:
        document = copy.deepcopy(DOCUMENT)
        document["stages"][1]["outputs"][0] = 6
        self.assertTrue(any("not a stable identifier" in error for error in validate(document)))

    def test_missing_condition_coverage_is_rejected(self) -> None:
        document = copy.deepcopy(DOCUMENT)
        for stage in document["stages"]:
            stage["conditions"] = [
                condition for condition in stage["conditions"] if condition["type"] != "filtering"
            ]
        self.assertTrue(any("condition coverage differs" in error for error in validate(document)))

    def test_unknown_localization_key_is_rejected(self) -> None:
        document = copy.deepcopy(DOCUMENT)
        document["stages"][0]["hint_key"] = "alchemy.stage.missing"
        self.assertTrue(any("alchemy.stage.missing" in error for error in validate(document)))

    def test_runtime_stage_drift_is_rejected(self) -> None:
        service_path = ROOT / "src" / "simulation" / "OmniAlchemy.cpp"
        service = service_path.read_text(encoding="utf-8").replace(
            '.id = "A01-STEAM-CYCLE"', '.id = "A01-DRIFTED"', 1
        )
        errors: list[str] = []
        original = alchemy_audit.read_text
        try:
            alchemy_audit.read_text = lambda path, sink: (
                service if path == service_path else original(path, sink)
            )
            alchemy_audit.check_runtime_contract(ROOT, DOCUMENT, errors)
        finally:
            alchemy_audit.read_text = original
        self.assertTrue(any("missing runtime stage A01-STEAM-CYCLE" in error for error in errors))

    def test_lua_stamp_bypass_guard_removal_is_rejected(self) -> None:
        lua_path = ROOT / "src" / "lua" / "LuaSimulation.cpp"
        lua = lua_path.read_text(encoding="utf-8").replace(
            "FindLockedAlchemySaveElements(*gameSave)",
            "removedLockedAlchemySaveElements(*gameSave)",
            1,
        )
        errors: list[str] = []
        original = alchemy_audit.read_text
        try:
            alchemy_audit.read_text = lambda path, sink: (
                lua if path == lua_path else original(path, sink)
            )
            alchemy_audit.check_runtime_contract(ROOT, DOCUMENT, errors)
        finally:
            alchemy_audit.read_text = original
        self.assertTrue(any("FindLockedAlchemySaveElements" in error for error in errors))

    def test_repeated_serialise_parse_import_removal_is_rejected(self) -> None:
        probe_path = ROOT / "tools" / "alchemy_state_probe.cpp"
        probe = probe_path.read_text(encoding="utf-8").replace(
            "iterationSave.Serialise().second",
            "removedSerialiseStep().second",
            1,
        )
        errors: list[str] = []
        original = alchemy_audit.read_text
        try:
            alchemy_audit.read_text = lambda path, sink: (
                probe if path == probe_path else original(path, sink)
            )
            alchemy_audit.check_runtime_contract(ROOT, DOCUMENT, errors)
        finally:
            alchemy_audit.read_text = original
        self.assertTrue(any("iterationSave.Serialise().second" in error for error in errors))


if __name__ == "__main__":
    unittest.main()
