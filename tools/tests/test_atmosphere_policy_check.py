from __future__ import annotations

import copy
import importlib.util
from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[2]
TOOL_PATH = ROOT / "tools" / "atmosphere_policy_check.py"
SPEC = importlib.util.spec_from_file_location("atmosphere_policy_check", TOOL_PATH)
assert SPEC is not None and SPEC.loader is not None
tool = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(tool)


class AtmospherePolicyContractTests(unittest.TestCase):
    def setUp(self) -> None:
        self.document = tool.load_json_strict(ROOT / tool.DATA_RELATIVE_PATH)

    def test_repository_policy_matrix_is_valid_and_unselected(self) -> None:
        errors, derived = tool.audit_document(self.document)
        self.assertEqual(errors, [])
        self.assertEqual(self.document["selection_status"], "unselected")
        self.assertEqual(derived["required_real_acoustic_substeps"], "7167")
        self.assertEqual(derived["required_real_acoustic_milliseconds"], "12121.47543")
        self.assertEqual(derived["maximum_budgeted_substeps"], "2")
        self.assertEqual(derived["maximum_budgeted_signal_speed"], "0.096")

    def test_reference_budget_is_explicit_and_current_candidate_fits_memory(self) -> None:
        budget = self.document["phase5_reference_budget"]
        solver = self.document["solver_measurement"]
        self.assertEqual(budget["status"], "accepted_reference_machine_target")
        self.assertLessEqual(int(solver["authoritative_bytes_per_cell"]), int(budget["authoritative_bytes_per_cell_maximum"]))
        self.assertLessEqual(int(solver["working_bytes_per_cell"]), int(budget["working_bytes_per_cell_maximum"]))

    def test_direct_real_acoustic_policy_cannot_claim_budget_pass(self) -> None:
        document = copy.deepcopy(self.document)
        document["policy_evaluations"][0]["status"] = "accepted"
        errors, _ = tool.audit_document(document)
        self.assertTrue(any("recomputed rejection drifted" in error for error in errors))

    def test_substep_result_is_recomputed_fail_closed(self) -> None:
        document = copy.deepcopy(self.document)
        document["policy_evaluations"][0]["expected_required_substeps"] = "2"
        errors, _ = tool.audit_document(document)
        self.assertTrue(any("recomputed rejection drifted" in error for error in errors))

    def test_memory_budget_overflow_is_rejected(self) -> None:
        document = copy.deepcopy(self.document)
        document["solver_measurement"]["working_bytes_per_cell"] = "257"
        errors, _ = tool.audit_document(document)
        self.assertTrue(any("working memory exceeds budget" in error for error in errors))

    def test_presentation_fps_cannot_define_physical_time(self) -> None:
        document = copy.deepcopy(self.document)
        document["time_reference"]["presentation_fps_derived"] = True
        errors, _ = tool.audit_document(document)
        self.assertTrue(any("presentation_fps_derived" in error for error in errors))

    def test_hybrid_components_cannot_claim_coupled_policy_completion(self) -> None:
        document = copy.deepcopy(self.document)
        document["policy_evaluations"][2]["status"] = "selected"
        errors, _ = tool.audit_document(document)
        self.assertTrue(any("component/coupling boundary drifted" in error for error in errors))

    def test_acoustic_reference_is_bound_to_public_nasa_record(self) -> None:
        document = copy.deepcopy(self.document)
        document["acoustic_reference"]["provenance"]["source_locator"] = "memory"
        errors, _ = tool.audit_document(document)
        self.assertTrue(any("NASA source/license binding drifted" in error for error in errors))


if __name__ == "__main__":
    unittest.main()
