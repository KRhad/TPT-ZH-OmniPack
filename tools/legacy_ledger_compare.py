from __future__ import annotations

import argparse
import csv
import json
import math
from pathlib import Path
import re
from typing import Any, Iterable


META_FIELDS = (
    "step",
    "state_hash_fnv1a32",
    "particles",
    "atmosphere_cells",
    "rng_a",
    "rng_b",
    "rng_c",
    "rng_d",
)
PROPERTY_COUNT_FIELDS = (
    "property_powder_records",
    "property_liquid_records",
    "property_solid_records",
    "property_gas_records",
    "property_energy_records",
    "property_unclassified_records",
)
NONFINITE_COUNT_FIELDS = (
    "particle_nonfinite_x",
    "particle_nonfinite_y",
    "particle_nonfinite_vx",
    "particle_nonfinite_vy",
    "particle_nonfinite_temp",
    "air_nonfinite_pressure",
    "air_nonfinite_velocity_x",
    "air_nonfinite_velocity_y",
    "air_nonfinite_ambient_heat",
)
RANGE_COUNT_FIELDS = (
    "particle_position_out_of_bounds",
    "particle_temp_below_min",
    "particle_temp_above_max",
    "air_pressure_below_min",
    "air_pressure_above_max",
    "air_velocity_x_below_min",
    "air_velocity_x_above_max",
    "air_velocity_y_below_min",
    "air_velocity_y_above_max",
    "air_ambient_heat_below_min",
    "air_ambient_heat_above_max",
)
BOUND_COUNT_FIELDS = (
    "particle_temp_at_min",
    "particle_temp_at_max",
    "air_pressure_at_min",
    "air_pressure_at_max",
    "air_velocity_x_at_min",
    "air_velocity_x_at_max",
    "air_velocity_y_at_min",
    "air_velocity_y_at_max",
    "air_ambient_heat_at_min",
    "air_ambient_heat_at_max",
)
PROXY_SUM_FIELDS = (
    "particle_sum_x",
    "particle_sum_y",
    "particle_sum_velocity_x",
    "particle_sum_velocity_y",
    "particle_sum_speed_squared",
    "particle_sum_temperature",
    "air_sum_pressure",
    "air_sum_velocity_x",
    "air_sum_velocity_y",
    "air_sum_speed_squared",
    "air_sum_ambient_heat",
)
EXTREMA_FIELDS = (
    "particle_min_temperature",
    "particle_max_temperature",
    "particle_max_abs_velocity_x",
    "particle_max_abs_velocity_y",
    "air_min_pressure",
    "air_max_pressure",
    "air_min_ambient_heat",
    "air_max_ambient_heat",
    "air_max_abs_velocity_x",
    "air_max_abs_velocity_y",
)
INTEGER_METRIC_FIELDS = (
    PROPERTY_COUNT_FIELDS
    + NONFINITE_COUNT_FIELDS
    + RANGE_COUNT_FIELDS
    + BOUND_COUNT_FIELDS
)
FLOAT_METRIC_FIELDS = PROXY_SUM_FIELDS + EXTREMA_FIELDS
LEDGER_FIELDS = META_FIELDS + INTEGER_METRIC_FIELDS + FLOAT_METRIC_FIELDS
TYPE_FIELDS = ("step", "type", "type_identifier", "count")
U32_FIELDS = ("state_hash_fnv1a32", "rng_a", "rng_b", "rng_c", "rng_d")
MAX_TYPE_ID = 0x7FFFFFFF


def expected_sample_steps(total_steps: int, sample_interval: int) -> list[int]:
    if total_steps < 1:
        raise ValueError("total_steps must be positive")
    if sample_interval < 1:
        raise ValueError("sample_interval must be positive")
    if sample_interval > total_steps:
        raise ValueError("sample_interval cannot exceed total_steps")
    steps = {0, 1, total_steps}
    steps.update(range(sample_interval, total_steps + 1, sample_interval))
    return sorted(steps)


def _require_fields(
    actual: Iterable[str] | None, expected: tuple[str, ...], path: Path
) -> None:
    if tuple(actual or ()) != expected:
        raise ValueError(
            f"unexpected CSV fields in {path}: actual={tuple(actual or ())}, "
            f"expected={expected}"
        )


def _parse_int(text: str, field: str, path: Path) -> int:
    try:
        return int(text, 10)
    except (TypeError, ValueError) as exc:
        raise ValueError(f"invalid integer {field}={text!r} in {path}") from exc


def _parse_nonnegative(text: str, field: str, path: Path) -> int:
    value = _parse_int(text, field, path)
    if value < 0:
        raise ValueError(f"negative count {field}={value} in {path}")
    return value


def _parse_float(text: str, field: str, path: Path) -> float:
    try:
        value = float(text)
    except (TypeError, ValueError) as exc:
        raise ValueError(f"invalid float {field}={text!r} in {path}") from exc
    if not math.isfinite(value):
        raise ValueError(f"non-finite aggregate {field}={text!r} in {path}")
    return value


def _reject_extra_columns(raw: dict[str | None, Any], path: Path) -> None:
    if raw.get(None):
        raise ValueError(f"unexpected extra CSV columns in {path}: {raw[None]!r}")


def _validate_ledger_row(row: dict[str, int | float], path: Path) -> None:
    particles = int(row["particles"])
    atmosphere_cells = int(row["atmosphere_cells"])
    if atmosphere_cells <= 0:
        raise ValueError(f"atmosphere_cells must be positive in {path}")
    for field in U32_FIELDS:
        if int(row[field]) > 0xFFFFFFFF:
            raise ValueError(f"uint32 field out of range {field}={row[field]} in {path}")

    property_total = sum(int(row[field]) for field in PROPERTY_COUNT_FIELDS)
    if property_total != particles:
        raise ValueError(
            f"Particle property-class counts do not match particles in {path}: "
            f"classes={property_total}, particles={particles}"
        )

    particle_observation_fields = tuple(
        field
        for field in NONFINITE_COUNT_FIELDS + RANGE_COUNT_FIELDS + BOUND_COUNT_FIELDS
        if field.startswith("particle_")
    )
    air_observation_fields = tuple(
        field
        for field in NONFINITE_COUNT_FIELDS + RANGE_COUNT_FIELDS + BOUND_COUNT_FIELDS
        if field.startswith("air_")
    )
    for field in PROPERTY_COUNT_FIELDS + particle_observation_fields:
        if int(row[field]) > particles:
            raise ValueError(
                f"Particle count field exceeds particles {field}={row[field]} "
                f"> {particles} in {path}"
            )

    position_out_of_bounds = int(row["particle_position_out_of_bounds"])
    for field in ("particle_nonfinite_x", "particle_nonfinite_y"):
        if int(row[field]) + position_out_of_bounds > particles:
            raise ValueError(
                f"Particle position counts are inconsistent {field}={row[field]}, "
                f"out_of_bounds={position_out_of_bounds}, particles={particles} in {path}"
            )
    for field in air_observation_fields:
        if int(row[field]) > atmosphere_cells:
            raise ValueError(
                f"Air count field exceeds atmosphere cells {field}={row[field]} "
                f"> {atmosphere_cells} in {path}"
            )

    exclusive_groups = (
        (
            "particle_nonfinite_temp",
            "particle_temp_below_min",
            "particle_temp_above_max",
            "particle_temp_at_min",
            "particle_temp_at_max",
            particles,
        ),
        (
            "air_nonfinite_pressure",
            "air_pressure_below_min",
            "air_pressure_above_max",
            "air_pressure_at_min",
            "air_pressure_at_max",
            atmosphere_cells,
        ),
        (
            "air_nonfinite_velocity_x",
            "air_velocity_x_below_min",
            "air_velocity_x_above_max",
            "air_velocity_x_at_min",
            "air_velocity_x_at_max",
            atmosphere_cells,
        ),
        (
            "air_nonfinite_velocity_y",
            "air_velocity_y_below_min",
            "air_velocity_y_above_max",
            "air_velocity_y_at_min",
            "air_velocity_y_at_max",
            atmosphere_cells,
        ),
        (
            "air_nonfinite_ambient_heat",
            "air_ambient_heat_below_min",
            "air_ambient_heat_above_max",
            "air_ambient_heat_at_min",
            "air_ambient_heat_at_max",
            atmosphere_cells,
        ),
    )
    for *fields, limit in exclusive_groups:
        if sum(int(row[field]) for field in fields) > limit:
            raise ValueError(
                f"mutually exclusive count fields exceed records {fields} in {path}"
            )

    if particles - int(row["particle_nonfinite_temp"]) > 0:
        if float(row["particle_min_temperature"]) > float(row["particle_max_temperature"]):
            raise ValueError(f"Particle temperature extrema are inverted in {path}")
    elif float(row["particle_min_temperature"]) or float(row["particle_max_temperature"]):
        raise ValueError(f"Particle temperature extrema must be zero without finite values in {path}")
    if atmosphere_cells - int(row["air_nonfinite_pressure"]) > 0:
        if float(row["air_min_pressure"]) > float(row["air_max_pressure"]):
            raise ValueError(f"Air pressure extrema are inverted in {path}")
    elif float(row["air_min_pressure"]) or float(row["air_max_pressure"]):
        raise ValueError(f"Air pressure extrema must be zero without finite values in {path}")
    if atmosphere_cells - int(row["air_nonfinite_ambient_heat"]) > 0:
        if float(row["air_min_ambient_heat"]) > float(row["air_max_ambient_heat"]):
            raise ValueError(f"Air heat extrema are inverted in {path}")
    elif float(row["air_min_ambient_heat"]) or float(row["air_max_ambient_heat"]):
        raise ValueError(f"Air heat extrema must be zero without finite values in {path}")
    velocity_extrema = (
        (particles, "particle_nonfinite_vx", "particle_max_abs_velocity_x"),
        (particles, "particle_nonfinite_vy", "particle_max_abs_velocity_y"),
        (atmosphere_cells, "air_nonfinite_velocity_x", "air_max_abs_velocity_x"),
        (atmosphere_cells, "air_nonfinite_velocity_y", "air_max_abs_velocity_y"),
    )
    for records, nonfinite_field, extrema_field in velocity_extrema:
        extrema = float(row[extrema_field])
        if extrema < 0:
            raise ValueError(
                f"negative max-absolute metric {extrema_field}={extrema} in {path}"
            )
        if records - int(row[nonfinite_field]) == 0 and extrema != 0:
            raise ValueError(
                f"{extrema_field} must be zero without finite values in {path}"
            )
    for field in ("particle_sum_speed_squared", "air_sum_speed_squared"):
        if float(row[field]) < 0:
            raise ValueError(f"negative squared-speed sum {field}={row[field]} in {path}")


def read_ledger(
    path: Path,
    *,
    total_steps: int | None = None,
    sample_interval: int | None = None,
) -> list[dict[str, int | float]]:
    rows: list[dict[str, int | float]] = []
    with path.open("r", encoding="utf-8", newline="") as handle:
        reader = csv.DictReader(handle)
        _require_fields(reader.fieldnames, LEDGER_FIELDS, path)
        previous_step = -1
        for raw in reader:
            _reject_extra_columns(raw, path)
            row: dict[str, int | float] = {}
            for field in META_FIELDS:
                row[field] = _parse_nonnegative(raw[field], field, path)
            for field in INTEGER_METRIC_FIELDS:
                row[field] = _parse_nonnegative(raw[field], field, path)
            for field in FLOAT_METRIC_FIELDS:
                row[field] = _parse_float(raw[field], field, path)
            step = int(row["step"])
            if step <= previous_step:
                raise ValueError(
                    f"ledger steps are not strictly increasing in {path}: "
                    f"previous={previous_step}, current={step}"
                )
            previous_step = step
            _validate_ledger_row(row, path)
            rows.append(row)
    if not rows:
        raise ValueError(f"empty ledger: {path}")
    if total_steps is not None or sample_interval is not None:
        if total_steps is None or sample_interval is None:
            raise ValueError("total_steps and sample_interval must be provided together")
        expected = expected_sample_steps(total_steps, sample_interval)
        actual = [int(row["step"]) for row in rows]
        if actual != expected:
            raise ValueError(
                f"unexpected sample schedule in {path}: actual={actual}, expected={expected}"
            )
    return rows


def read_type_counts(
    path: Path, ledger_rows: list[dict[str, int | float]]
) -> dict[int, dict[int, dict[str, int | str]]]:
    expected_steps = [int(row["step"]) for row in ledger_rows]
    by_step: dict[int, dict[int, dict[str, int | str]]] = {
        step: {} for step in expected_steps
    }
    identifiers: dict[int, str] = {}
    identifier_types: dict[str, int] = {}
    with path.open("r", encoding="utf-8", newline="") as handle:
        reader = csv.DictReader(handle)
        _require_fields(reader.fieldnames, TYPE_FIELDS, path)
        for raw in reader:
            _reject_extra_columns(raw, path)
            step = _parse_nonnegative(raw["step"], "step", path)
            type_id = _parse_nonnegative(raw["type"], "type", path)
            count = _parse_nonnegative(raw["count"], "count", path)
            identifier = raw["type_identifier"]
            if not identifier or not re.fullmatch(r"[A-Z0-9_]+", identifier):
                raise ValueError(f"invalid type identifier {identifier!r} in {path}")
            if type_id > MAX_TYPE_ID:
                raise ValueError(f"type ID out of range {type_id} in {path}")
            if type_id == 0:
                if identifier != "DEFAULT_PT_NONE" or count != 0:
                    raise ValueError(f"invalid PT_NONE sentinel at step {step} in {path}")
            else:
                if identifier == "DEFAULT_PT_NONE":
                    raise ValueError(
                        f"nonzero type uses PT_NONE identifier at step {step} in {path}"
                    )
                if count == 0:
                    raise ValueError(
                        f"nonzero type has a zero count step={step} type={type_id} in {path}"
                    )
            if step not in by_step:
                raise ValueError(f"type count has unknown step {step} in {path}")
            if type_id in by_step[step]:
                raise ValueError(
                    f"duplicate type count step={step} type={type_id} in {path}"
                )
            previous_identifier = identifiers.setdefault(type_id, identifier)
            if previous_identifier != identifier:
                raise ValueError(
                    f"type identifier drift for {type_id} in {path}: "
                    f"{previous_identifier!r} != {identifier!r}"
                )
            previous_type_id = identifier_types.setdefault(identifier, type_id)
            if previous_type_id != type_id:
                raise ValueError(
                    f"type identifier reused by different IDs in {path}: "
                    f"{identifier!r} maps to {previous_type_id} and {type_id}"
                )
            by_step[step][type_id] = {
                "type": type_id,
                "type_identifier": identifier,
                "count": count,
            }

    ledger_by_step = {int(row["step"]): row for row in ledger_rows}
    for step in expected_steps:
        records = by_step[step]
        sentinel = records.get(0)
        if (
            not sentinel
            or sentinel["count"] != 0
            or sentinel["type_identifier"] != "DEFAULT_PT_NONE"
        ):
            raise ValueError(f"missing zero-count PT_NONE sentinel at step {step} in {path}")
        total = sum(int(record["count"]) for type_id, record in records.items() if type_id)
        particles = int(ledger_by_step[step]["particles"])
        if total != particles:
            raise ValueError(
                f"type counts do not match particles at step {step} in {path}: "
                f"types={total}, particles={particles}"
            )
    return by_step


def _finite_calculation(value: float, context: str) -> float:
    if not math.isfinite(value):
        raise ValueError(f"non-finite derived value for {context}")
    return value


def _endpoint_proxy_drift(
    rows: list[dict[str, int | float]],
) -> dict[str, dict[str, float | None]]:
    first = rows[0]
    last = rows[-1]
    result: dict[str, dict[str, float | None]] = {}
    for field in PROXY_SUM_FIELDS:
        initial = float(first[field])
        final = float(last[field])
        delta = _finite_calculation(final - initial, f"{field}.delta")
        relative = (
            _finite_calculation(delta / abs(initial), f"{field}.relative_to_initial")
            if initial
            else None
        )
        result[field] = {
            "initial": initial,
            "final": final,
            "delta": delta,
            "relative_to_initial": relative,
        }
    return result


def _side_summary(
    rows: list[dict[str, int | float]],
) -> dict[str, Any]:
    nonfinite_by_field = {
        field: sum(int(row[field]) for row in rows) for field in NONFINITE_COUNT_FIELDS
    }
    range_by_field = {
        field: sum(int(row[field]) for row in rows) for field in RANGE_COUNT_FIELDS
    }
    bound_by_field = {
        field: sum(int(row[field]) for row in rows) for field in BOUND_COUNT_FIELDS
    }
    return {
        "finite_exported_state": not any(nonfinite_by_field.values()),
        "range_contract_pass": not any(range_by_field.values()),
        "nonfinite_observations": nonfinite_by_field,
        "range_violation_observations": range_by_field,
        "bound_occupancy_observations": bound_by_field,
        "endpoint_proxy_drift": _endpoint_proxy_drift(rows),
    }


def compare_ledgers(
    left_ledger_path: Path,
    right_ledger_path: Path,
    left_types_path: Path,
    right_types_path: Path,
    *,
    total_steps: int,
    sample_interval: int,
) -> dict[str, Any]:
    left = read_ledger(
        left_ledger_path,
        total_steps=total_steps,
        sample_interval=sample_interval,
    )
    right = read_ledger(
        right_ledger_path,
        total_steps=total_steps,
        sample_interval=sample_interval,
    )
    if len(left) != len(right):
        raise ValueError(f"ledger row count mismatch: left={len(left)}, right={len(right)}")
    if [row["step"] for row in left] != [row["step"] for row in right]:
        raise ValueError("ledger sample steps do not match")

    left_types = read_type_counts(left_types_path, left)
    right_types = read_type_counts(right_types_path, right)

    first_sampled_state_hash_divergence: dict[str, Any] | None = None
    first_sampled_metadata_divergence: dict[str, Any] | None = None
    first_sampled_metric_divergence: dict[str, Any] | None = None
    max_abs_mode_difference: dict[str, dict[str, float | int]] = {}

    compared_metric_fields = INTEGER_METRIC_FIELDS + FLOAT_METRIC_FIELDS
    for left_row, right_row in zip(left, right, strict=True):
        step = int(left_row["step"])
        if (
            first_sampled_state_hash_divergence is None
            and left_row["state_hash_fnv1a32"] != right_row["state_hash_fnv1a32"]
        ):
            first_sampled_state_hash_divergence = {
                "step": step,
                "left": int(left_row["state_hash_fnv1a32"]),
                "right": int(right_row["state_hash_fnv1a32"]),
            }
        if first_sampled_metadata_divergence is None:
            metadata_fields = [
                field
                for field in ("particles", "atmosphere_cells", "rng_a", "rng_b", "rng_c", "rng_d")
                if left_row[field] != right_row[field]
            ]
            if metadata_fields:
                first_sampled_metadata_divergence = {
                    "step": step,
                    "fields": metadata_fields,
                    "left": {field: left_row[field] for field in metadata_fields},
                    "right": {field: right_row[field] for field in metadata_fields},
                }

        scalar_fields = [
            field
            for field in compared_metric_fields
            if left_row[field] != right_row[field]
        ]
        type_ids = sorted(set(left_types[step]) | set(right_types[step]))
        differing_type_ids = [
            type_id
            for type_id in type_ids
            if left_types[step].get(type_id) != right_types[step].get(type_id)
        ]
        if first_sampled_metric_divergence is None and (scalar_fields or differing_type_ids):
            first_sampled_metric_divergence = {
                "step": step,
                "scalar_fields": scalar_fields,
                "type_ids": differing_type_ids,
                "left": {field: left_row[field] for field in scalar_fields},
                "right": {field: right_row[field] for field in scalar_fields},
                "left_types": [left_types[step].get(type_id) for type_id in differing_type_ids],
                "right_types": [right_types[step].get(type_id) for type_id in differing_type_ids],
            }

        for field in PROXY_SUM_FIELDS + EXTREMA_FIELDS:
            difference = _finite_calculation(
                abs(float(left_row[field]) - float(right_row[field])),
                f"{field}.mode_difference.step_{step}",
            )
            record = max_abs_mode_difference.get(field)
            if record is None or difference > float(record["max_abs"]):
                max_abs_mode_difference[field] = {
                    "max_abs": difference,
                    "step": step,
                    "left": float(left_row[field]),
                    "right": float(right_row[field]),
                }

    return {
        "schema_version": 1,
        "status": "PASS",
        "comparison_kind": "same_source_cpu_fp_mode_legacy_proxy_ledger",
        "total_steps": total_steps,
        "sample_interval": sample_interval,
        "samples": len(left),
        "sample_steps": [int(row["step"]) for row in left],
        "first_sampled_state_hash_divergence": first_sampled_state_hash_divergence,
        "first_sampled_metadata_divergence": first_sampled_metadata_divergence,
        "first_sampled_metric_divergence": first_sampled_metric_divergence,
        "left": _side_summary(left),
        "right": _side_summary(right),
        "max_abs_mode_difference": max_abs_mode_difference,
        "claims": {
            "legacy_field_proxies_only": True,
            "physical_mass_conservation_evaluated": False,
            "physical_energy_conservation_evaluated": False,
            "physical_momentum_conservation_evaluated": False,
            "source_sink_attribution_evaluated": False,
            "correction_events_evaluated": False,
            "sampled_states_only": True,
            "exported_particle_float_subset_only": True,
        },
    }


def _write_json(path: Path, value: dict[str, Any]) -> None:
    path.write_text(
        json.dumps(value, indent=2, sort_keys=True, allow_nan=False) + "\n",
        encoding="utf-8",
    )


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Compare Legacy numerical integrity and proxy ledgers"
    )
    parser.add_argument("--left-ledger", type=Path, required=True)
    parser.add_argument("--right-ledger", type=Path, required=True)
    parser.add_argument("--left-types", type=Path, required=True)
    parser.add_argument("--right-types", type=Path, required=True)
    parser.add_argument("--total-steps", type=int, required=True)
    parser.add_argument("--sample-interval", type=int, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    if args.total_steps < 1 or args.sample_interval < 1:
        parser.error("total steps and sample interval must be positive")
    result = compare_ledgers(
        args.left_ledger,
        args.right_ledger,
        args.left_types,
        args.right_types,
        total_steps=args.total_steps,
        sample_interval=args.sample_interval,
    )
    _write_json(args.output, result)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
