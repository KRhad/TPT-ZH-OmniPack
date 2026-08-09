from __future__ import annotations

import importlib.util
import json
from pathlib import Path
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[2]
TOOL_PATH = ROOT / "tools" / "physical_ledger_feasibility.py"
SPEC = importlib.util.spec_from_file_location("physical_ledger_feasibility", TOOL_PATH)
assert SPEC and SPEC.loader
tool = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(tool)


class PhysicalLedgerFeasibilityTest(unittest.TestCase):
    def test_inventory_binds_current_state_and_lifecycle_anchors(self) -> None:
        inventory = tool.build_inventory(ROOT, allow_dirty=True)
        self.assertEqual(inventory["schema_version"], 1)
        self.assertRegex(inventory["source_commit"], r"^[0-9a-f]{40}$")
        self.assertIsInstance(inventory["source_dirty"], bool)
        self.assertEqual(
            inventory["state_storage"]["particle_float_fields"],
            ["x", "y", "vx", "vy", "temp"],
        )
        self.assertEqual(
            inventory["state_storage"]["air_current_float_fields"],
            ["pv", "vx", "vy", "hv", "fvx", "fvy"],
        )
        self.assertEqual(
            inventory["state_storage"]["air_scratch_float_fields"],
            ["opv", "ovx", "ovy", "ohv"],
        )
        self.assertFalse(any(inventory["state_storage"]["particle_authoritative_physical_fields_present"].values()))
        self.assertFalse(any(inventory["state_storage"]["air_authoritative_physical_fields_present"].values()))
        self.assertFalse(inventory["state_storage"]["element_weight_is_authoritative_mass"])
        self.assertFalse(inventory["state_storage"]["element_heat_capacity_is_physical_joules_per_kelvin"])
        self.assertEqual(set(inventory["lifecycle"]["definitions"]), {"create_part", "kill_part", "part_change_type"})
        self.assertGreaterEqual(len(inventory["lifecycle"]["direct_particle_type_assignment_candidates"]), 5)
        self.assertGreater(inventory["correction_inventory"]["lexical_candidate_counts"]["restrict_flt"], 0)
        self.assertGreaterEqual(len(inventory["correction_inventory"]["air_explicit_cap_candidates"]), 12)
        self.assertFalse(inventory["correction_inventory"]["complete_runtime_correction_event_ledger_present"])
        self.assertFalse(inventory["claims"]["physical_mass_conservation_evaluated"])
        self.assertFalse(inventory["claims"]["source_sink_attribution_evaluated"])

    def test_json_output_is_stable_and_newline_terminated(self) -> None:
        inventory = tool.build_inventory(ROOT, allow_dirty=True)
        with tempfile.TemporaryDirectory() as temporary:
            output = Path(temporary) / "inventory.json"
            tool._write_json(output, inventory)
            payload = output.read_bytes()
        self.assertTrue(payload.endswith(b"\n"))
        self.assertNotIn(b"\r\n", payload)
        self.assertEqual(json.loads(payload), inventory)


if __name__ == "__main__":
    unittest.main()
