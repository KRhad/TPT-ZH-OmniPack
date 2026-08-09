from __future__ import annotations

import csv
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[2]
COMPARATOR_PATH = ROOT / "tools" / "load_boundary_compare.py"
LUA_PATH = ROOT / "tools" / "runtime" / "characterization_scenarios.lua"
WRAPPER_PATH = ROOT / "tools" / "runtime_generate_characterization.ps1"
SPEC = importlib.util.spec_from_file_location("load_boundary_compare", COMPARATOR_PATH)
assert SPEC and SPEC.loader
comparator = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(comparator)


def particle(
    ordinal: int,
    runtime_id: int,
    *,
    pixel_x: int | None = None,
    pixel_y: int = 0,
    **overrides: int | float,
) -> dict[str, int | float]:
    px = ordinal if pixel_x is None else pixel_x
    row: dict[str, int | float] = {}
    for field in comparator.PARTICLE_FIELDS:
        row[field] = 0.0 if field in comparator.PARTICLE_FLOAT_FIELDS else 0
    row.update(
        {
            "save_ordinal": ordinal,
            "runtime_id": runtime_id,
            "pixel_x": px,
            "pixel_y": pixel_y,
            "type": 1,
            "x": float(px),
            "y": float(pixel_y),
            "temp": 295.15,
        }
    )
    row.update(overrides)
    return row


def cell(cx: int, cy: int, **overrides: int | float) -> dict[str, int | float]:
    row: dict[str, int | float] = {}
    for field in comparator.CELL_FIELDS:
        row[field] = 0.0 if field in comparator.CELL_FLOAT_FIELDS else 0
    row.update({"cx": cx, "cy": cy, "ambient_heat": 295.15})
    row.update(overrides)
    return row


def setting(name: str, kind: str, value: str) -> dict[str, str]:
    return {"name": name, "kind": kind, "value": value}


def write_csv(path: Path, fields: tuple[str, ...], rows: list[dict[str, object]]) -> None:
    with path.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=fields)
        writer.writeheader()
        writer.writerows(rows)


def write_capture(
    root: Path,
    prefix: str,
    particles: list[dict[str, object]],
    cells: list[dict[str, object]],
    settings: list[dict[str, str]],
) -> tuple[Path, Path, Path]:
    particle_path = root / f"{prefix}-particles.csv"
    cell_path = root / f"{prefix}-cells.csv"
    setting_path = root / f"{prefix}-settings.csv"
    write_csv(particle_path, comparator.PARTICLE_FIELDS, particles)
    write_csv(cell_path, comparator.CELL_FIELDS, cells)
    write_csv(setting_path, comparator.SETTING_FIELDS, settings)
    return particle_path, cell_path, setting_path


class LoadBoundaryComparatorTest(unittest.TestCase):
    def compare(
        self,
        before_particles: list[dict[str, object]],
        loaded_particles: list[dict[str, object]],
        before_cells: list[dict[str, object]] | None = None,
        loaded_cells: list[dict[str, object]] | None = None,
        before_settings: list[dict[str, str]] | None = None,
        loaded_settings: list[dict[str, str]] | None = None,
    ) -> dict[str, object]:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            before = write_capture(
                root,
                "before",
                before_particles,
                before_cells or [cell(0, 0)],
                before_settings or [setting("air_mode", "int", "0")],
            )
            loaded = write_capture(
                root,
                "loaded",
                loaded_particles,
                loaded_cells or [cell(0, 0)],
                loaded_settings or [setting("air_mode", "int", "0")],
            )
            return comparator.compare_load_boundary(
                before[0], loaded[0], before[1], loaded[1], before[2], loaded[2]
            )

    def test_equal_payload_with_reallocated_runtime_ids_is_not_particle_identity_failure(self) -> None:
        result = self.compare(
            [particle(0, 50), particle(1, 100)],
            [particle(0, 0), particle(1, 1)],
        )
        particles = result["particles"]
        self.assertEqual(particles["differing_records"], 0)
        self.assertEqual(particles["runtime_id"]["changed_records"], 2)
        self.assertEqual(particles["save_order_pixel_alignment"]["misaligned_records"], 0)
        self.assertFalse(result["claims"]["particle_runtime_id_is_stable"])
        self.assertFalse(result["claims"]["physical_mass_conservation_evaluated"])

    def test_particle_payload_differences_are_attributed_by_save_ordinal(self) -> None:
        result = self.compare(
            [particle(0, 80), particle(1, 81)],
            [particle(0, 0, vx=0.0625, temp=295.0), particle(1, 1)],
        )
        particles = result["particles"]
        self.assertEqual(particles["differing_records"], 1)
        self.assertEqual(particles["first_difference"]["save_ordinal"], 0)
        self.assertEqual(particles["first_difference"]["fields"], ["vx", "temp"])
        self.assertEqual(particles["payload_field_summary"]["vx"]["count"], 1)
        self.assertEqual(particles["payload_field_summary"]["vx"]["max_abs"], 0.0625)

    def test_pixel_misalignment_is_reported_without_using_runtime_id_as_identity(self) -> None:
        result = self.compare(
            [particle(0, 4, pixel_x=4, pixel_y=5)],
            [particle(0, 0, pixel_x=5, pixel_y=5, x=5.0)],
        )
        particles = result["particles"]
        self.assertEqual(particles["save_order_pixel_alignment"]["misaligned_records"], 1)
        self.assertEqual(particles["first_difference"]["fields"], ["pixel_position", "x"])

    def test_particle_presence_difference_is_fail_closed_but_characterized(self) -> None:
        result = self.compare(
            [particle(0, 2), particle(1, 3)],
            [particle(0, 0)],
        )
        particles = result["particles"]
        self.assertEqual(particles["records"], {"before_save": 2, "loaded": 1})
        self.assertEqual(particles["differing_records"], 1)
        self.assertEqual(particles["first_difference"]["save_ordinal"], 1)
        self.assertEqual(particles["first_difference"]["fields"], ["presence"])

    def test_cell_and_setting_differences_are_attributed(self) -> None:
        result = self.compare(
            [particle(0, 0)],
            [particle(0, 0)],
            [cell(0, 0, pressure=1.25), cell(1, 0)],
            [cell(0, 0, pressure=1.0), cell(1, 0)],
            [setting("air_mode", "int", "0"), setting("edge_pressure", "float", "0")],
            [setting("air_mode", "int", "1"), setting("edge_pressure", "float", "0")],
        )
        self.assertEqual(result["cells"]["differing_cells"], 1)
        self.assertEqual(result["cells"]["first_difference"]["fields"], ["pressure"])
        self.assertEqual(result["settings"]["differing_settings"], 1)
        self.assertEqual(result["settings"]["first_difference"]["name"], "air_mode")

    def test_domains_are_exposed_even_when_they_do_not_match(self) -> None:
        result = self.compare(
            [particle(0, 0)],
            [particle(0, 0)],
            [cell(0, 0)],
            [cell(1, 0)],
            [setting("air_mode", "int", "0")],
            [setting("edge_mode", "int", "0")],
        )
        self.assertFalse(result["cells"]["same_coordinate_domain"])
        self.assertFalse(result["settings"]["same_name_domain"])
        self.assertEqual(result["cells"]["differing_cells"], 2)
        self.assertEqual(result["settings"]["differing_settings"], 2)

    def test_nonfinite_particle_and_cell_values_are_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            bad_particles = root / "particles.csv"
            write_csv(
                bad_particles,
                comparator.PARTICLE_FIELDS,
                [particle(0, 0, temp="nan")],
            )
            with self.assertRaisesRegex(ValueError, "non-finite float"):
                comparator.read_particles(bad_particles)
            bad_cells = root / "cells.csv"
            write_csv(bad_cells, comparator.CELL_FIELDS, [cell(0, 0, pressure="inf")])
            with self.assertRaisesRegex(ValueError, "non-finite float"):
                comparator.read_cells(bad_cells)

    def test_particle_schema_order_ordinal_and_runtime_id_are_validated(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            path = root / "particles.csv"
            write_csv(path, comparator.PARTICLE_FIELDS, [particle(1, 0)])
            with self.assertRaisesRegex(ValueError, "non-sequential save_ordinal"):
                comparator.read_particles(path)

            write_csv(
                path,
                comparator.PARTICLE_FIELDS,
                [particle(0, 3, pixel_x=2), particle(1, 2, pixel_x=1)],
            )
            with self.assertRaisesRegex(ValueError, "not in save order"):
                comparator.read_particles(path)

            write_csv(path, comparator.PARTICLE_FIELDS, [particle(0, 0), particle(1, 0)])
            with self.assertRaisesRegex(ValueError, "duplicate runtime_id"):
                comparator.read_particles(path)

    def test_particle_uint32_and_type_ranges_are_validated(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            path = root / "particles.csv"
            write_csv(path, comparator.PARTICLE_FIELDS, [particle(0, 0, dcolour=2**32)])
            with self.assertRaisesRegex(ValueError, "uint32 field out of range"):
                comparator.read_particles(path)
            write_csv(path, comparator.PARTICLE_FIELDS, [particle(0, 0, type=0)])
            with self.assertRaisesRegex(ValueError, "particle type out of range"):
                comparator.read_particles(path)

    def test_cell_schema_duplicate_coordinates_and_extra_columns_are_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            path = root / "cells.csv"
            write_csv(path, comparator.CELL_FIELDS, [cell(0, 0), cell(0, 0)])
            with self.assertRaisesRegex(ValueError, "duplicate atmosphere cell"):
                comparator.read_cells(path)
            path.write_text(
                ",".join(comparator.CELL_FIELDS) + ",extra\n" + ",".join("0" for _ in comparator.CELL_FIELDS) + ",x\n",
                encoding="utf-8",
            )
            with self.assertRaisesRegex(ValueError, "unexpected CSV fields"):
                comparator.read_cells(path)

    def test_settings_schema_kind_value_and_duplicate_name_are_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            path = root / "settings.csv"
            write_csv(path, comparator.SETTING_FIELDS, [setting("air_mode", "bool", "1")])
            with self.assertRaisesRegex(ValueError, "invalid boolean"):
                comparator.read_settings(path)
            write_csv(
                path,
                comparator.SETTING_FIELDS,
                [setting("air_mode", "int", "0"), setting("air_mode", "int", "0")],
            )
            with self.assertRaisesRegex(ValueError, "duplicate setting"):
                comparator.read_settings(path)
            write_csv(path, comparator.SETTING_FIELDS, [setting("AIR MODE", "int", "0")])
            with self.assertRaisesRegex(ValueError, "invalid setting name"):
                comparator.read_settings(path)

    def test_json_writer_is_byte_stable_lf_and_rejects_nan(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "comparison.json"
            comparator._write_json(path, {"b": 2, "a": 1})
            self.assertEqual(path.read_bytes(), b'{\n  "a": 1,\n  "b": 2\n}\n')
            with self.assertRaises(ValueError):
                comparator._write_json(path, {"value": float("nan")})


class LoadBoundarySourceContractTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.lua = LUA_PATH.read_text(encoding="utf-8")
        cls.wrapper = WRAPPER_PATH.read_text(encoding="utf-8")

    def test_comparator_declares_nonphysical_scope_and_save_order_matching(self) -> None:
        source = COMPARATOR_PATH.read_text(encoding="utf-8")
        for text in (
            "ops_pre_save_vs_loaded_field_attribution",
            "rounded pixel y/x then pre-save runtime ID",
            "physical_mass_conservation_evaluated",
            "particle_runtime_id_is_stable",
            "bit_exact_in_memory_checkpoint_claimed",
            "newline=\"\\n\"",
        ):
            self.assertIn(text, source)

    def test_runtime_contract_will_export_particles_cells_and_settings_for_both_sides(self) -> None:
        for text in (
            "dump_load_boundary_particles",
            "dump_load_boundary_cells",
            "dump_load_boundary_settings",
            'dump_load_boundary_capture("before-save")',
            'dump_load_boundary_capture("loaded")',
            'particle_path = prefix .. "-particles.csv"',
            "save_ordinal",
            "load_boundary_particle_file",
            "load_boundary_cell_file",
            "load_boundary_settings_file",
        ):
            self.assertIn(text, self.lua)

    def test_wrapper_contract_will_bind_comparator_and_two_loaded_capture_hashes(self) -> None:
        for text in (
            "load_boundary_compare.py",
            "load-boundary-comparison.json",
            "Cross-restart load-boundary capture differs",
            "load_boundary_comparator_sha256",
            "comparison_kind = [string]$loadBoundaryComparison.comparison_kind",
            "load_boundary_capture_complete",
        ):
            self.assertIn(text, self.wrapper)

    def test_wrapper_uses_its_own_ps5_compatible_file_hash_helper(self) -> None:
        self.assertIn("function Get-FileSha256", self.wrapper)
        self.assertIn("[System.IO.File]::OpenRead", self.wrapper)
        self.assertNotIn("Get-FileHash", self.wrapper)

    def test_wrapper_uses_the_ps5_environment_variables_dictionary(self) -> None:
        self.assertIn(
            "$childEnvironment = $startInfo.EnvironmentVariables", self.wrapper
        )

    def test_wrapper_uses_singular_settings_capture_artifact_name(self) -> None:
        self.assertIn('"settings" { "settings.csv" }', self.wrapper)
        self.assertNotIn('$captureName + "s.csv"', self.wrapper)

    def test_wrapper_closes_frozen_inputs_and_recursive_artifact_inventory(self) -> None:
        for text in (
            "function New-FrozenInput",
            "function Get-ArtifactFileInventory",
            "tool_inputs_frozen_before_execution = $true",
            "source_unchanged_after_execution = $true",
            "Characterization source worktree changed during execution",
            "Characterization executable changed during execution",
            "Characterization build provenance changed during execution",
            "artifact_file_count_excluding_manifest",
            "artifact_files = $artifactFiles",
            "frozen-inputs/$($_.Name)",
        ):
            self.assertIn(text, self.wrapper)


if __name__ == "__main__":
    unittest.main()
