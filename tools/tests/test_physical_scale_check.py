from __future__ import annotations

import copy
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[2]
TOOL_PATH = ROOT / "tools" / "physical_scale_check.py"
SPEC = importlib.util.spec_from_file_location("physical_scale_check", TOOL_PATH)
assert SPEC is not None and SPEC.loader is not None
tool = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(tool)


class PhysicalScaleContractTests(unittest.TestCase):
    def setUp(self) -> None:
        self.document = tool.load_json_strict(ROOT / tool.DATA_RELATIVE_PATH)

    def test_repository_candidate_is_unselected_and_valid(self) -> None:
        self.assertEqual(tool.audit_document(self.document), [])
        self.assertEqual(self.document["selection_status"], "unselected")

    def test_legacy_geometry_is_frozen(self) -> None:
        geometry = self.document["legacy_geometry"]
        self.assertEqual(geometry["cell_pixels"], "4")
        self.assertEqual((geometry["air_cells_x"], geometry["air_cells_y"]), ("153", "96"))
        self.assertEqual((geometry["particle_pixels_x"], geometry["particle_pixels_y"]), ("612", "384"))

    def test_volume_identity_failure_is_rejected(self) -> None:
        document = copy.deepcopy(self.document)
        document["geometry_candidates"][0]["derived_metrics"]["atmosphere_cell_volume"]["value"] = "0.000000063"
        errors = tool.audit_document(document)
        self.assertTrue(any("volume identity failed" in error for error in errors))

    def test_geometry_mismatch_is_rejected(self) -> None:
        document = copy.deepcopy(self.document)
        document["geometry_candidates"][0]["atmosphere_cell_length"]["value"] = "0.005"
        errors = tool.audit_document(document)
        self.assertTrue(any("cell length must equal" in error for error in errors))

    def test_time_policy_cannot_be_accepted(self) -> None:
        document = copy.deepcopy(self.document)
        document["time_candidates"][0]["time_policy"] = "accepted"
        errors = tool.audit_document(document)
        self.assertTrue(any("must remain unselected" in error for error in errors))

    def test_fps_cannot_become_physical_dt(self) -> None:
        document = copy.deepcopy(self.document)
        document["legacy_geometry"]["presentation_fps_is_physical_dt"] = True
        errors = tool.audit_document(document)
        self.assertTrue(any("presentation_fps_is_physical_dt" in error for error in errors))

    def test_nan_and_unknown_fields_are_rejected(self) -> None:
        document = copy.deepcopy(self.document)
        document["geometry_candidates"][0]["pixel_length"]["value"] = "NaN"
        document["geometry_candidates"][0]["legacy_pv_pascal"] = "1"
        errors = tool.audit_document(document)
        self.assertTrue(any("canonical decimal" in error for error in errors))
        self.assertTrue(any("unknown fields" in error for error in errors))

    def test_derived_metric_unit_is_not_interchangeable(self) -> None:
        document = copy.deepcopy(self.document)
        document["geometry_candidates"][0]["derived_metrics"]["atmosphere_cell_volume"]["unit"] = "metre"
        errors = tool.audit_document(document)
        self.assertTrue(any("expected cubic_metre" in error for error in errors))

    def test_candidate_seconds_require_registered_si_second(self) -> None:
        document = copy.deepcopy(self.document)
        document["time_candidates"][0]["candidate_physical_seconds_per_tick_unit"] = "metre"
        errors = tool.audit_document(document)
        self.assertTrue(any("expected second" in error for error in errors))

    def test_physical_scale_units_must_be_in_the_canonical_registry(self) -> None:
        registry = tool.load_json_strict(ROOT / tool.UNIT_REGISTRY_RELATIVE_PATH)
        registry["units"] = [
            unit for unit in registry["units"] if unit["id"] != "cubic_metre"
        ]
        errors = tool.audit_unit_registry(registry)
        self.assertTrue(any("requires cubic_metre as volume" in error for error in errors))

    def test_coordinate_and_gravity_contracts_are_fail_closed(self) -> None:
        document = copy.deepcopy(self.document)
        convention = document["geometry_candidates"][0]["coordinate_convention"]
        convention["cell_sample"] = "corner"
        convention["gravity_source"] = 1
        errors = tool.audit_document(document)
        self.assertTrue(any("cell_sample: expected cell_center" in error for error in errors))
        self.assertTrue(any("gravity_source: expected explicit_benchmark_input" in error for error in errors))

    def test_strict_loader_rejects_duplicate_keys(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "duplicate.json"
            path.write_text('{"schema_version":1,"schema_version":1}\n', encoding="utf-8")
            with self.assertRaisesRegex(tool.ValidationFailure, "duplicate JSON key"):
                tool.load_json_strict(path)


if __name__ == "__main__":
    unittest.main()
