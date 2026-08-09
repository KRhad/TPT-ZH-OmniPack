from __future__ import annotations

import csv
import importlib.util
from pathlib import Path
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[2]
COMPARATOR_PATH = ROOT / "tools" / "legacy_ledger_compare.py"
LUA_PATH = ROOT / "tools" / "runtime" / "legacy_numerical_ledger.lua"
WRAPPER_PATH = ROOT / "tools" / "runtime_legacy_numerical_ledger.ps1"
GITIGNORE_PATH = ROOT / ".gitignore"
SPEC = importlib.util.spec_from_file_location("legacy_ledger_compare", COMPARATOR_PATH)
assert SPEC and SPEC.loader
comparator = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(comparator)


def ledger_row(step: int, particles: int = 2, **overrides: int | float) -> dict[str, int | float]:
    row: dict[str, int | float] = {}
    for field in comparator.LEDGER_FIELDS:
        row[field] = 0.0 if field in comparator.FLOAT_METRIC_FIELDS else 0
    row.update(
        {
            "step": step,
            "state_hash_fnv1a32": 100 + step,
            "particles": particles,
            "atmosphere_cells": 4,
            "rng_a": 1,
            "rng_b": 2,
            "rng_c": 3,
            "rng_d": 4,
            "property_powder_records": particles,
        }
    )
    row.update(overrides)
    return row


def write_ledger(path: Path, rows: list[dict[str, int | float]]) -> None:
    with path.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=comparator.LEDGER_FIELDS)
        writer.writeheader()
        writer.writerows(rows)


def write_types(
    path: Path,
    rows: list[dict[str, int | str]],
) -> None:
    with path.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=comparator.TYPE_FIELDS)
        writer.writeheader()
        writer.writerows(rows)


def type_rows(step: int, counts: dict[int, int]) -> list[dict[str, int | str]]:
    rows: list[dict[str, int | str]] = [
        {
            "step": step,
            "type": 0,
            "type_identifier": "DEFAULT_PT_NONE",
            "count": 0,
        }
    ]
    for type_id, count in sorted(counts.items()):
        rows.append(
            {
                "step": step,
                "type": type_id,
                "type_identifier": f"DEFAULT_PT_T{type_id}",
                "count": count,
            }
        )
    return rows


class LegacyLedgerComparatorTest(unittest.TestCase):
    def test_expected_schedule_always_contains_zero_one_and_final(self) -> None:
        self.assertEqual(
            comparator.expected_sample_steps(25, 10),
            [0, 1, 10, 20, 25],
        )
        self.assertEqual(comparator.expected_sample_steps(1, 1), [0, 1])
        with self.assertRaisesRegex(ValueError, "cannot exceed"):
            comparator.expected_sample_steps(5, 10)

    def test_equal_ledgers_keep_physical_claims_false(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            paths = [root / name for name in ("left.csv", "right.csv", "lt.csv", "rt.csv")]
            rows = [ledger_row(step, particle_sum_temperature=590.3) for step in range(3)]
            write_ledger(paths[0], rows)
            write_ledger(paths[1], rows)
            types = [record for step in range(3) for record in type_rows(step, {1: 2})]
            write_types(paths[2], types)
            write_types(paths[3], types)
            result = comparator.compare_ledgers(
                paths[0], paths[1], paths[2], paths[3], total_steps=2, sample_interval=1
            )

        self.assertIsNone(result["first_sampled_state_hash_divergence"])
        self.assertIsNone(result["first_sampled_metadata_divergence"])
        self.assertIsNone(result["first_sampled_metric_divergence"])
        self.assertTrue(result["left"]["finite_exported_state"])
        self.assertFalse(result["claims"]["physical_mass_conservation_evaluated"])
        self.assertFalse(result["claims"]["physical_energy_conservation_evaluated"])
        self.assertTrue(result["claims"]["legacy_field_proxies_only"])

    def test_first_metric_and_type_divergence_are_reported(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            left_ledger = root / "left.csv"
            right_ledger = root / "right.csv"
            left_types = root / "left-types.csv"
            right_types = root / "right-types.csv"
            left_rows = [ledger_row(step) for step in range(3)]
            right_rows = [ledger_row(step) for step in range(3)]
            right_rows[1]["state_hash_fnv1a32"] = 999
            right_rows[1]["particle_sum_velocity_x"] = 0.25
            write_ledger(left_ledger, left_rows)
            write_ledger(right_ledger, right_rows)
            write_types(
                left_types,
                [record for step in range(3) for record in type_rows(step, {1: 2})],
            )
            right_type_rows = []
            for step in range(3):
                right_type_rows.extend(type_rows(step, {1: 1, 2: 1} if step == 1 else {1: 2}))
            write_types(right_types, right_type_rows)
            result = comparator.compare_ledgers(
                left_ledger,
                right_ledger,
                left_types,
                right_types,
                total_steps=2,
                sample_interval=1,
            )

        self.assertEqual(result["first_sampled_state_hash_divergence"]["step"], 1)
        self.assertEqual(result["first_sampled_metric_divergence"]["step"], 1)
        self.assertIn(
            "particle_sum_velocity_x",
            result["first_sampled_metric_divergence"]["scalar_fields"],
        )
        self.assertEqual(result["first_sampled_metric_divergence"]["type_ids"], [1, 2])

    def test_rng_change_is_metadata_not_state_hash_divergence(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            paths = [root / name for name in ("left.csv", "right.csv", "lt.csv", "rt.csv")]
            left_rows = [ledger_row(step) for step in range(2)]
            right_rows = [ledger_row(step) for step in range(2)]
            right_rows[1]["rng_a"] = 99
            write_ledger(paths[0], left_rows)
            write_ledger(paths[1], right_rows)
            types = [record for step in range(2) for record in type_rows(step, {1: 2})]
            write_types(paths[2], types)
            write_types(paths[3], types)
            result = comparator.compare_ledgers(
                paths[0], paths[1], paths[2], paths[3], total_steps=1, sample_interval=1
            )

        self.assertIsNone(result["first_sampled_state_hash_divergence"])
        self.assertEqual(result["first_sampled_metadata_divergence"]["step"], 1)
        self.assertEqual(result["first_sampled_metadata_divergence"]["fields"], ["rng_a"])

    def test_nonfinite_aggregate_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "ledger.csv"
            row = ledger_row(0)
            row["air_sum_pressure"] = "nan"
            write_ledger(path, [row])
            with self.assertRaisesRegex(ValueError, "non-finite aggregate"):
                comparator.read_ledger(path)

    def test_type_totals_must_match_particle_count(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            ledger = root / "ledger.csv"
            types = root / "types.csv"
            rows = [ledger_row(0)]
            write_ledger(ledger, rows)
            write_types(types, type_rows(0, {1: 1}))
            with self.assertRaisesRegex(ValueError, "do not match particles"):
                comparator.read_type_counts(types, rows)

    def test_extra_columns_bad_sentinel_and_zero_count_type_are_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            ledger = root / "ledger.csv"
            write_ledger(ledger, [ledger_row(0)])
            lines = ledger.read_text(encoding="utf-8").splitlines()
            ledger.write_text(lines[0] + "\n" + lines[1] + ",EXTRA\n", encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "extra CSV columns"):
                comparator.read_ledger(ledger)

            rows = [ledger_row(0)]
            bad_sentinel = root / "bad-sentinel.csv"
            write_types(
                bad_sentinel,
                [
                    {"step": 0, "type": 0, "type_identifier": "NOT_PT_NONE", "count": 0},
                    *type_rows(0, {1: 2})[1:],
                ],
            )
            with self.assertRaisesRegex(ValueError, "invalid PT_NONE sentinel"):
                comparator.read_type_counts(bad_sentinel, rows)

            zero_type = root / "zero-type.csv"
            write_types(zero_type, type_rows(0, {1: 2}) + type_rows(0, {2: 0})[1:])
            with self.assertRaisesRegex(ValueError, "nonzero type has a zero count"):
                comparator.read_type_counts(zero_type, rows)

    def test_cross_field_invariants_and_uint32_are_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            bad_classes = root / "bad-classes.csv"
            row = ledger_row(0)
            row["property_powder_records"] = 0
            write_ledger(bad_classes, [row])
            with self.assertRaisesRegex(ValueError, "property-class counts"):
                comparator.read_ledger(bad_classes)

            bad_u32 = root / "bad-u32.csv"
            row = ledger_row(0)
            row["state_hash_fnv1a32"] = 0x100000000
            write_ledger(bad_u32, [row])
            with self.assertRaisesRegex(ValueError, "uint32 field out of range"):
                comparator.read_ledger(bad_u32)

    def test_valid_unclassified_particle_records_are_accepted(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "unclassified.csv"
            row = ledger_row(0)
            row["property_powder_records"] = 0
            row["property_unclassified_records"] = 2
            write_ledger(path, [row])
            parsed = comparator.read_ledger(path)
        self.assertEqual(parsed[0]["property_unclassified_records"], 2)

        for name in ("FIGH.cpp", "STKM.cpp", "STKM2.cpp"):
            source = (
                ROOT / "src" / "simulation" / "elements" / name
            ).read_text(encoding="utf-8")
            self.assertIn("Properties = PROP_NOCTYPEDRAW;", source)

    def test_impossible_position_and_squared_speed_metrics_are_rejected(self) -> None:
        cases = (
            (
                {"particle_nonfinite_x": 2, "particle_position_out_of_bounds": 1},
                "position counts are inconsistent",
            ),
            (
                {"particle_nonfinite_y": 2, "particle_position_out_of_bounds": 1},
                "position counts are inconsistent",
            ),
            ({"particle_sum_speed_squared": -1.0}, "negative squared-speed sum"),
            ({"air_sum_speed_squared": -1.0}, "negative squared-speed sum"),
        )
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            for index, (overrides, message) in enumerate(cases):
                with self.subTest(overrides=overrides):
                    path = root / f"impossible-{index}.csv"
                    write_ledger(path, [ledger_row(0, **overrides)])
                    with self.assertRaisesRegex(ValueError, message):
                        comparator.read_ledger(path)

    def test_nonfinite_counts_are_exclusive_with_range_and_bound_counts(self) -> None:
        cases = (
            {"particle_nonfinite_temp": 2, "particle_temp_at_min": 1},
            {"air_nonfinite_pressure": 4, "air_pressure_at_max": 1},
            {"air_nonfinite_velocity_x": 4, "air_velocity_x_below_min": 1},
            {"air_nonfinite_velocity_y": 4, "air_velocity_y_at_min": 1},
            {"air_nonfinite_ambient_heat": 4, "air_ambient_heat_above_max": 1},
        )
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            for index, overrides in enumerate(cases):
                with self.subTest(overrides=overrides):
                    path = root / f"exclusive-{index}.csv"
                    write_ledger(path, [ledger_row(0, **overrides)])
                    with self.assertRaisesRegex(ValueError, "mutually exclusive"):
                        comparator.read_ledger(path)

    def test_extrema_must_be_zero_without_finite_values(self) -> None:
        cases = (
            {"particle_nonfinite_temp": 2, "particle_min_temperature": 1.0},
            {"air_nonfinite_pressure": 4, "air_max_pressure": 1.0},
            {"air_nonfinite_ambient_heat": 4, "air_min_ambient_heat": 1.0},
            {"particle_nonfinite_vx": 2, "particle_max_abs_velocity_x": 1.0},
            {"particle_nonfinite_vy": 2, "particle_max_abs_velocity_y": 1.0},
            {"air_nonfinite_velocity_x": 4, "air_max_abs_velocity_x": 1.0},
            {"air_nonfinite_velocity_y": 4, "air_max_abs_velocity_y": 1.0},
        )
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            for index, overrides in enumerate(cases):
                with self.subTest(overrides=overrides):
                    path = root / f"extrema-{index}.csv"
                    write_ledger(path, [ledger_row(0, **overrides)])
                    with self.assertRaisesRegex(ValueError, "must be zero"):
                        comparator.read_ledger(path)

    def test_short_csv_row_is_rejected_as_invalid_input(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "short.csv"
            header = ",".join(comparator.LEDGER_FIELDS)
            short_row = ",".join("0" for _ in comparator.LEDGER_FIELDS[:-1])
            path.write_text(header + "\n" + short_row + "\n", encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "invalid float"):
                comparator.read_ledger(path)

    def test_type_ids_and_identifiers_are_strictly_validated(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            rows = [ledger_row(0)]
            cases = (
                (
                    "none-alias.csv",
                    [
                        *type_rows(0, {})[:1],
                        {
                            "step": 0,
                            "type": 1,
                            "type_identifier": "DEFAULT_PT_NONE",
                            "count": 2,
                        },
                    ],
                    "nonzero type uses PT_NONE",
                ),
                (
                    "large-id.csv",
                    type_rows(0, {comparator.MAX_TYPE_ID + 1: 2}),
                    "type ID out of range",
                ),
                (
                    "reused-identifier.csv",
                    [
                        *type_rows(0, {})[:1],
                        {
                            "step": 0,
                            "type": 1,
                            "type_identifier": "DEFAULT_PT_DUP",
                            "count": 1,
                        },
                        {
                            "step": 0,
                            "type": 2,
                            "type_identifier": "DEFAULT_PT_DUP",
                            "count": 1,
                        },
                    ],
                    "reused by different IDs",
                ),
                (
                    "invalid-identifier.csv",
                    [
                        *type_rows(0, {})[:1],
                        {
                            "step": 0,
                            "type": 1,
                            "type_identifier": "DEFAULT PT BAD",
                            "count": 2,
                        },
                    ],
                    "invalid type identifier",
                ),
            )
            for filename, records, message in cases:
                with self.subTest(filename=filename):
                    path = root / filename
                    write_types(path, records)
                    with self.assertRaisesRegex(ValueError, message):
                        comparator.read_type_counts(path, rows)

    def test_derived_overflow_is_rejected_with_context(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            paths = [root / name for name in ("left.csv", "right.csv", "lt.csv", "rt.csv")]
            left_rows = [ledger_row(0, particle_sum_x=-1e308), ledger_row(1, particle_sum_x=1e308)]
            right_rows = [ledger_row(0), ledger_row(1)]
            write_ledger(paths[0], left_rows)
            write_ledger(paths[1], right_rows)
            types = [record for step in range(2) for record in type_rows(step, {1: 2})]
            write_types(paths[2], types)
            write_types(paths[3], types)
            with self.assertRaisesRegex(ValueError, "particle_sum_x.delta"):
                comparator.compare_ledgers(
                    paths[0], paths[1], paths[2], paths[3], total_steps=1, sample_interval=1
                )

    def test_json_output_has_platform_independent_lf_newlines(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "comparison.json"
            comparator._write_json(path, {"status": "PASS", "nested": {"value": 1}})
            payload = path.read_bytes()
        self.assertNotIn(b"\r\n", payload)
        self.assertTrue(payload.endswith(b"\n"))
        self.assertEqual(
            payload,
            b'{\n  "nested": {\n    "value": 1\n  },\n  "status": "PASS"\n}\n',
        )


class LegacyLedgerSourceContractTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.lua = LUA_PATH.read_text(encoding="utf-8")
        cls.wrapper = WRAPPER_PATH.read_text(encoding="utf-8")
        cls.gitignore = GITIGNORE_PATH.read_text(encoding="utf-8")

    def test_lua_samples_exported_floats_and_uses_compensated_sums(self) -> None:
        for getter in (
            'sim.partProperty(id, "x")',
            'sim.partProperty(id, "y")',
            'sim.partProperty(id, "vx")',
            'sim.partProperty(id, "vy")',
            'sim.partProperty(id, "temp")',
            "sim.pressure",
            "sim.velocityX",
            "sim.velocityY",
            "sim.ambientHeat",
        ):
            self.assertIn(getter, self.lua)
        self.assertIn("local function is_finite", self.lua)
        self.assertIn("local function compensated_add", self.lua)
        self.assertIn("step == 0 or step == 1", self.lua)
        self.assertIn("property_unclassified_records", self.lua)

    def test_lua_and_comparator_reject_physical_conservation_claims(self) -> None:
        for token in (
            'proxy_contract = "legacy_state_proxies_not_physical_units"',
            'physical_mass_conservation_evaluated = "false"',
            'physical_energy_conservation_evaluated = "false"',
            'source_sink_attribution_evaluated = "false"',
            'correction_events_evaluated = "false"',
        ):
            self.assertIn(token, self.lua)
        comparator_source = COMPARATOR_PATH.read_text(encoding="utf-8")
        self.assertIn('"legacy_field_proxies_only": True', comparator_source)
        self.assertIn('"physical_mass_conservation_evaluated": False', comparator_source)

    def test_wrapper_is_private_isolated_and_provenance_bound(self) -> None:
        self.assertIn(
            '[string] $OutputDirectory = "artifacts/vnext-legacy-ledger"',
            self.wrapper,
        )
        self.assertIn("/artifacts/", self.gitignore)
        self.assertIn("function Remove-IsolatedRoot", self.wrapper)
        self.assertIn("CreateNoWindow = $true", self.wrapper)
        self.assertIn("$process.Dispose()", self.wrapper)
        for field in (
            "worktree_state_sha256",
            "lua_sha256",
            "comparator_sha256",
            "wrapper_sha256",
            "executable_sha256",
            "fp_mode",
            "simulation_compile_command",
            "compile_command_count",
            "artifact_files",
            'comparison_kind = "same_source_cpu_fp_mode_legacy_proxy_ledger"',
            'performance_gate = "not_evaluated"',
            'physical_mass_conservation_evaluated = $false',
            'physical_energy_conservation_evaluated = $false',
        ):
            self.assertIn(field, self.wrapper)

    def test_wrapper_freezes_tools_minimizes_environment_and_checks_pairing(self) -> None:
        for token in (
            "Assert-BuildPairCompatible",
            "Assert-FpCompileContract",
            "Executable does not match the Meson powder target",
            '$StartInfo.Environment.Clear()',
            'policy = "minimal_allowlist"',
            "tool_inputs_frozen_before_execution",
            "source worktree changed during execution",
            "Resolve-ProbeOutput",
            'ExpectedName "ledger.csv"',
            'ExpectedName "type-counts.csv"',
            "Stop-ProcessTree",
            "$Process.Kill($true)",
            "taskkill.exe",
            "Invoke-Comparator",
            "result does not match CSV endpoints",
            "observation total mismatch",
            "sampled_states_only",
            "Assert-AllCompileCommandsCompatible",
            "ninja_target_built_before_execution",
            "runtime_directory_dll_inventory",
            "python_reported_version",
            "executable_rehashed_after_execution",
            "retained_test_root",
        ):
            self.assertIn(token, self.wrapper)

    def test_wrapper_hashing_uses_windows_powershell_compatible_apis(self) -> None:
        self.assertIn('[System.Security.Cryptography.SHA256]::Create()', self.wrapper)
        self.assertIn("$hasher.ComputeHash($Bytes)", self.wrapper)
        self.assertNotIn("::HashData(", self.wrapper)
        self.assertNotIn("[Convert]::ToHexString", self.wrapper)
        self.assertIn("function ConvertTo-WindowsProcessArgument", self.wrapper)
        self.assertIn("Set-ProcessArguments -StartInfo $startInfo", self.wrapper)
        self.assertNotIn(".ArgumentList", self.wrapper)


if __name__ == "__main__":
    unittest.main()
