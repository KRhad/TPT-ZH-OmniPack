from __future__ import annotations

import csv
import importlib.util
import json
from pathlib import Path
import shutil
import sys
import tempfile
import unittest


SCRIPT = Path(__file__).resolve().parents[1] / "automation_audit.py"
SPEC = importlib.util.spec_from_file_location("automation_audit", SCRIPT)
assert SPEC is not None and SPEC.loader is not None
automation_audit = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = automation_audit
SPEC.loader.exec_module(automation_audit)

ROOT = Path(__file__).resolve().parents[2]


class AutomationAuditTests(unittest.TestCase):
    def test_repository_passes(self) -> None:
        self.assertEqual(automation_audit.audit(ROOT), [])

    def test_periodic_slot_claimed_by_wrong_module_is_rejected(self) -> None:
        rows = automation_audit.read_csv(ROOT / "docs" / "ELEMENT_REGISTRY.csv", [])
        target = next(row for row in rows if row["stable_id"] == "392")
        target["module"] = "nuclear"
        errors: list[str] = []
        original = automation_audit.read_csv
        try:
            automation_audit.read_csv = lambda path, sink: (
                rows if path.name == "ELEMENT_REGISTRY.csv" else original(path, sink)
            )
            automation_audit.check_capabilities(ROOT, errors)
        finally:
            automation_audit.read_csv = original
        self.assertTrue(any("former automation ID 392 must belong to 'periodic'" in error for error in errors))

    def test_missing_required_scenario_is_rejected(self) -> None:
        source = json.loads(
            (ROOT / "automation" / "0.3.0" / "scenario-spec.json").read_text(
                encoding="utf-8"
            )
        )
        source["scenarios"] = [
            row for row in source["scenarios"] if row["id"] != "integrated-factory"
        ]
        errors: list[str] = []
        original = automation_audit.read_json
        try:
            automation_audit.read_json = lambda path, sink: (
                source if path.name == "scenario-spec.json" else original(path, sink)
            )
            automation_audit.check_scenarios(ROOT, errors)
        finally:
            automation_audit.read_json = original
        self.assertTrue(any("scenario set differs" in error for error in errors))
        self.assertTrue(any("unknown scenario" in error for error in errors))

    def test_official_source_hash_mismatch_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            relative = Path("src/simulation/elements/DLAY.cpp")
            target = root / relative
            target.parent.mkdir(parents=True)
            shutil.copy2(ROOT / relative, target)
            target.write_text(
                target.read_text(encoding="utf-8") + "\n// mutation\n",
                encoding="utf-8",
            )
            manifest_dir = root / "automation" / "0.3.0"
            manifest_dir.mkdir(parents=True)
            expected = {
                "files": {
                    relative.as_posix(): automation_audit.hashlib.sha256(
                        (ROOT / relative).read_bytes()
                    ).hexdigest().upper()
                }
            }
            (manifest_dir / "official-source-sha256.json").write_text(
                json.dumps(expected), encoding="utf-8"
            )
            errors: list[str] = []
            automation_audit.check_source_fingerprints(
                root, {relative.as_posix()}, errors
            )
        self.assertTrue(any("fingerprint changed" in error for error in errors))

    def test_missing_stop_measurement_is_rejected(self) -> None:
        lua_path = ROOT / "tools" / "runtime" / "automation_regression.lua"
        mutated = lua_path.read_text(encoding="utf-8").replace(
            "raw_step(32)", "raw_step_removed(32)", 1
        )
        errors: list[str] = []
        original = automation_audit.read_text
        try:
            automation_audit.read_text = lambda path, sink: (
                mutated if path == lua_path else original(path, sink)
            )
            automation_audit.check_runtime_contract(ROOT, errors)
        finally:
            automation_audit.read_text = original
        self.assertTrue(any("raw_step(32)" in error for error in errors))

    def test_missing_module_runtime_gate_is_rejected(self) -> None:
        source_path = ROOT / "src" / "simulation" / "OmniChemistry.cpp"
        mutated = source_path.read_text(encoding="utf-8").replace(
            "OmniModuleRuntimeEnabled", "RemovedModuleRuntimeGate", 1
        )
        errors: list[str] = []
        original = automation_audit.read_text
        try:
            automation_audit.read_text = lambda path, sink: (
                mutated if path == source_path else original(path, sink)
            )
            automation_audit.check_module_bounds(ROOT, errors)
        finally:
            automation_audit.read_text = original
        self.assertTrue(any("OmniModuleRuntimeEnabled" in error for error in errors))


if __name__ == "__main__":
    unittest.main()
