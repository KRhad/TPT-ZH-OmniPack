from __future__ import annotations

import argparse
import csv
import json
import math
from pathlib import Path
from typing import Any, Iterable


TRACE_FIELDS = (
    "step",
    "state_hash_fnv1a32",
    "particles",
    "rng_a",
    "rng_b",
    "rng_c",
    "rng_d",
)
PARTICLE_FIELDS = (
    "id",
    "type",
    "type_identifier",
    "life",
    "ctype",
    "x",
    "y",
    "vx",
    "vy",
    "temp",
    "flags",
    "tmp",
    "tmp2",
    "tmp3",
    "tmp4",
    "dcolour",
)
PARTICLE_FLOAT_FIELDS = ("x", "y", "vx", "vy", "temp")
PARTICLE_INT_FIELDS = tuple(
    field
    for field in PARTICLE_FIELDS
    if field not in PARTICLE_FLOAT_FIELDS and field != "type_identifier"
)
CELL_FIELDS = (
    "cx",
    "cy",
    "pressure",
    "velocity_x",
    "velocity_y",
    "ambient_heat",
    "wall_map",
    "elec_map",
    "fan_velocity_x",
    "fan_velocity_y",
    "gravity_mass",
    "gravity_mask",
    "gravity_force_x",
    "gravity_force_y",
)
CELL_FLOAT_FIELDS = (
    "pressure",
    "velocity_x",
    "velocity_y",
    "ambient_heat",
    "fan_velocity_x",
    "fan_velocity_y",
    "gravity_mass",
    "gravity_force_x",
    "gravity_force_y",
)
CELL_INT_FIELDS = tuple(
    field for field in CELL_FIELDS if field not in CELL_FLOAT_FIELDS
)


def _require_fields(actual: Iterable[str] | None, expected: tuple[str, ...], path: Path) -> None:
    if tuple(actual or ()) != expected:
        raise ValueError(
            f"unexpected CSV fields in {path}: actual={tuple(actual or ())}, "
            f"expected={expected}"
        )


def _parse_int(text: str, field: str, path: Path) -> int:
    try:
        return int(text, 10)
    except ValueError as exc:
        raise ValueError(f"invalid integer {field}={text!r} in {path}") from exc


def _parse_float(text: str, field: str, path: Path) -> float:
    try:
        value = float(text)
    except ValueError as exc:
        raise ValueError(f"invalid float {field}={text!r} in {path}") from exc
    if not math.isfinite(value):
        raise ValueError(f"non-finite float {field}={text!r} in {path}")
    return value


def read_trace(path: Path) -> list[dict[str, int]]:
    with path.open("r", encoding="utf-8", newline="") as handle:
        reader = csv.DictReader(handle)
        _require_fields(reader.fieldnames, TRACE_FIELDS, path)
        rows: list[dict[str, int]] = []
        for expected_step, raw in enumerate(reader):
            row = {field: _parse_int(raw[field], field, path) for field in TRACE_FIELDS}
            if row["step"] != expected_step:
                raise ValueError(
                    f"non-sequential trace in {path}: expected step {expected_step}, "
                    f"found {row['step']}"
                )
            rows.append(row)
    if not rows:
        raise ValueError(f"empty trace: {path}")
    return rows


def compare_traces(left_path: Path, right_path: Path) -> dict[str, Any]:
    left = read_trace(left_path)
    right = read_trace(right_path)
    if len(left) != len(right):
        raise ValueError(
            f"trace length mismatch: left={len(left)}, right={len(right)}"
        )

    differing_fields = TRACE_FIELDS[1:]
    first: dict[str, Any] | None = None
    for left_row, right_row in zip(left, right, strict=True):
        fields = [
            field
            for field in differing_fields
            if left_row[field] != right_row[field]
        ]
        if fields:
            first = {
                "step": left_row["step"],
                "fields": fields,
                "left": left_row,
                "right": right_row,
            }
            break

    return {
        "schema_version": 1,
        "status": "PASS",
        "trace_rows": len(left),
        "compared_through_step": left[-1]["step"],
        "divergence_found": first is not None,
        "first_divergence": first,
    }


def read_particles(path: Path) -> dict[int, dict[str, Any]]:
    particles: dict[int, dict[str, Any]] = {}
    with path.open("r", encoding="utf-8", newline="") as handle:
        reader = csv.DictReader(handle)
        _require_fields(reader.fieldnames, PARTICLE_FIELDS, path)
        for raw in reader:
            row: dict[str, Any] = {"type_identifier": raw["type_identifier"]}
            for field in PARTICLE_INT_FIELDS:
                row[field] = _parse_int(raw[field], field, path)
            for field in PARTICLE_FLOAT_FIELDS:
                row[field] = _parse_float(raw[field], field, path)
            particle_id = row["id"]
            if particle_id in particles:
                raise ValueError(f"duplicate particle ID {particle_id} in {path}")
            particles[particle_id] = row
    return particles


def read_cells(path: Path) -> dict[tuple[int, int], dict[str, Any]]:
    cells: dict[tuple[int, int], dict[str, Any]] = {}
    with path.open("r", encoding="utf-8", newline="") as handle:
        reader = csv.DictReader(handle)
        _require_fields(reader.fieldnames, CELL_FIELDS, path)
        for raw in reader:
            row: dict[str, Any] = {}
            for field in CELL_INT_FIELDS:
                row[field] = _parse_int(raw[field], field, path)
            for field in CELL_FLOAT_FIELDS:
                row[field] = _parse_float(raw[field], field, path)
            key = (row["cx"], row["cy"])
            if key in cells:
                raise ValueError(f"duplicate atmosphere cell {key} in {path}")
            cells[key] = row
    return cells


def _different_fields(
    left: dict[str, Any] | None,
    right: dict[str, Any] | None,
    fields: tuple[str, ...],
) -> list[str]:
    if left is None or right is None:
        return ["presence"]
    return [field for field in fields if left[field] != right[field]]


def _update_numeric_summary(
    summary: dict[str, dict[str, float | int]],
    fields: Iterable[str],
    left: dict[str, Any] | None,
    right: dict[str, Any] | None,
) -> None:
    if left is None or right is None:
        presence = summary.setdefault("presence", {"count": 0, "max_abs": 0.0})
        presence["count"] = int(presence["count"]) + 1
        return
    for field in fields:
        if left[field] == right[field]:
            continue
        record = summary.setdefault(field, {"count": 0, "max_abs": 0.0})
        record["count"] = int(record["count"]) + 1
        if isinstance(left[field], (int, float)) and isinstance(
            right[field], (int, float)
        ):
            record["max_abs"] = max(
                float(record["max_abs"]), abs(float(left[field]) - float(right[field]))
            )


def _particle_pixel(row: dict[str, Any]) -> tuple[int, int]:
    return (math.floor(row["x"] + 0.5), math.floor(row["y"] + 0.5))


def _neighbors(
    particles: dict[int, dict[str, Any]],
    centers: set[tuple[int, int]],
    radius: int,
) -> list[dict[str, Any]]:
    result = []
    for particle_id in sorted(particles):
        row = particles[particle_id]
        px, py = _particle_pixel(row)
        if any(abs(px - cx) <= radius and abs(py - cy) <= radius for cx, cy in centers):
            result.append(row)
    return result


def _cell_pair(
    key: tuple[int, int],
    left_cells: dict[tuple[int, int], dict[str, Any]],
    right_cells: dict[tuple[int, int], dict[str, Any]],
) -> dict[str, Any]:
    left = left_cells.get(key)
    right = right_cells.get(key)
    return {
        "cx": key[0],
        "cy": key[1],
        "fields": _different_fields(left, right, CELL_FIELDS[2:]),
        "left": left,
        "right": right,
    }


def compare_captures(
    left_particle_path: Path,
    right_particle_path: Path,
    left_cell_path: Path,
    right_cell_path: Path,
    *,
    step: int,
    left_hash: int,
    right_hash: int,
    cell_size: int = 4,
    neighbor_radius: int = 2,
    sample_limit: int = 64,
) -> dict[str, Any]:
    left_particles = read_particles(left_particle_path)
    right_particles = read_particles(right_particle_path)
    left_cells = read_cells(left_cell_path)
    right_cells = read_cells(right_cell_path)
    if set(left_cells) != set(right_cells):
        raise ValueError("atmosphere cell domains differ")

    particle_summary: dict[str, dict[str, float | int]] = {}
    particle_differences: list[dict[str, Any]] = []
    first_particle: dict[str, Any] | None = None
    compared_particle_fields = PARTICLE_FIELDS[1:]
    for particle_id in sorted(set(left_particles) | set(right_particles)):
        left = left_particles.get(particle_id)
        right = right_particles.get(particle_id)
        fields = _different_fields(left, right, compared_particle_fields)
        if not fields:
            continue
        _update_numeric_summary(
            particle_summary,
            tuple(field for field in compared_particle_fields if field != "type_identifier"),
            left,
            right,
        )
        record = {"id": particle_id, "fields": fields, "left": left, "right": right}
        if first_particle is None:
            first_particle = record
        if len(particle_differences) < sample_limit:
            particle_differences.append(
                {"id": particle_id, "fields": fields}
            )

    cell_summary: dict[str, dict[str, float | int]] = {}
    cell_differences: list[dict[str, Any]] = []
    first_cell: dict[str, Any] | None = None
    compared_cell_fields = CELL_FIELDS[2:]
    for key in sorted(left_cells, key=lambda item: (item[1], item[0])):
        left = left_cells[key]
        right = right_cells[key]
        fields = _different_fields(left, right, compared_cell_fields)
        if not fields:
            continue
        _update_numeric_summary(cell_summary, compared_cell_fields, left, right)
        record = _cell_pair(key, left_cells, right_cells)
        if first_cell is None:
            first_cell = record
        if len(cell_differences) < sample_limit:
            cell_differences.append({"cx": key[0], "cy": key[1], "fields": fields})

    centers: set[tuple[int, int]] = set()
    if first_particle:
        for side in ("left", "right"):
            row = first_particle[side]
            if row is not None:
                centers.add(_particle_pixel(row))

    particle_cells: list[dict[str, Any]] = []
    for px, py in sorted(centers, key=lambda item: (item[1], item[0])):
        key = (px // cell_size, py // cell_size)
        if key in left_cells:
            pair = _cell_pair(key, left_cells, right_cells)
            pair["particle_x"] = px
            pair["particle_y"] = py
            particle_cells.append(pair)

    particle_difference_count = sum(
        int(record["count"]) for record in particle_summary.values()
    )
    differing_particle_ids = len(
        set(left_particles) | set(right_particles)
    ) - sum(
        1
        for particle_id in set(left_particles) & set(right_particles)
        if not _different_fields(
            left_particles[particle_id], right_particles[particle_id], compared_particle_fields
        )
    )
    differing_cell_count = sum(
        1
        for key in left_cells
        if _different_fields(left_cells[key], right_cells[key], compared_cell_fields)
    )
    hash_differs = left_hash != right_hash
    exposed_difference = bool(first_particle or first_cell)

    primary_domain = "none"
    if first_particle:
        primary_domain = "particle"
    elif first_cell:
        primary_domain = "atmosphere"

    return {
        "schema_version": 1,
        "status": "PASS",
        "step": step,
        "left_state_hash_fnv1a32": left_hash,
        "right_state_hash_fnv1a32": right_hash,
        "hash_differs": hash_differs,
        "unexplained_hash_divergence": hash_differs and not exposed_difference,
        "particle_records": {
            "left": len(left_particles),
            "right": len(right_particles),
        },
        "atmosphere_cells": len(left_cells),
        "differing_particle_ids": differing_particle_ids,
        "particle_field_difference_events": particle_difference_count,
        "differing_atmosphere_cells": differing_cell_count,
        "particle_field_summary": particle_summary,
        "atmosphere_field_summary": cell_summary,
        "difference_field": {
            "primary_domain": primary_domain,
            "particle_id": first_particle["id"] if first_particle else None,
            "particle_fields": first_particle["fields"] if first_particle else [],
            "atmosphere_cell": (
                {"cx": first_cell["cx"], "cy": first_cell["cy"]}
                if first_cell
                else None
            ),
            "atmosphere_fields": first_cell["fields"] if first_cell else [],
        },
        "first_particle_difference": first_particle,
        "first_atmosphere_difference": first_cell,
        "particle_atmosphere_cells": particle_cells,
        "neighbor_radius_pixels": neighbor_radius,
        "neighbor_state": {
            "centers": [
                {"x": center[0], "y": center[1]}
                for center in sorted(centers, key=lambda item: (item[1], item[0]))
            ],
            "left": _neighbors(left_particles, centers, neighbor_radius) if centers else [],
            "right": _neighbors(right_particles, centers, neighbor_radius) if centers else [],
        },
        "particle_difference_sample": particle_differences,
        "atmosphere_difference_sample": cell_differences,
        "sample_limit": sample_limit,
    }


def _write_json(path: Path, value: dict[str, Any]) -> None:
    path.write_text(
        json.dumps(value, indent=2, sort_keys=True, allow_nan=False) + "\n",
        encoding="utf-8",
    )


def main() -> int:
    parser = argparse.ArgumentParser(description="Compare OmniCore differential traces")
    subparsers = parser.add_subparsers(dest="command", required=True)

    trace_parser = subparsers.add_parser("trace")
    trace_parser.add_argument("--left", type=Path, required=True)
    trace_parser.add_argument("--right", type=Path, required=True)
    trace_parser.add_argument("--output", type=Path, required=True)

    capture_parser = subparsers.add_parser("capture")
    capture_parser.add_argument("--left-particles", type=Path, required=True)
    capture_parser.add_argument("--right-particles", type=Path, required=True)
    capture_parser.add_argument("--left-cells", type=Path, required=True)
    capture_parser.add_argument("--right-cells", type=Path, required=True)
    capture_parser.add_argument("--step", type=int, required=True)
    capture_parser.add_argument("--left-hash", type=int, required=True)
    capture_parser.add_argument("--right-hash", type=int, required=True)
    capture_parser.add_argument("--cell-size", type=int, default=4)
    capture_parser.add_argument("--neighbor-radius", type=int, default=2)
    capture_parser.add_argument("--sample-limit", type=int, default=64)
    capture_parser.add_argument("--output", type=Path, required=True)

    args = parser.parse_args()
    if args.command == "trace":
        result = compare_traces(args.left, args.right)
    else:
        if args.step < 0 or args.cell_size <= 0 or args.neighbor_radius < 0:
            parser.error("step/radius must be nonnegative and cell size must be positive")
        if args.sample_limit <= 0:
            parser.error("sample limit must be positive")
        result = compare_captures(
            args.left_particles,
            args.right_particles,
            args.left_cells,
            args.right_cells,
            step=args.step,
            left_hash=args.left_hash,
            right_hash=args.right_hash,
            cell_size=args.cell_size,
            neighbor_radius=args.neighbor_radius,
            sample_limit=args.sample_limit,
        )
    _write_json(args.output, result)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
