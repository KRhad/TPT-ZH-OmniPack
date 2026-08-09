from __future__ import annotations

import argparse
import csv
import json
import math
import re
from pathlib import Path
from typing import Any, Iterable


PARTICLE_FIELDS = (
    "save_ordinal",
    "runtime_id",
    "pixel_x",
    "pixel_y",
    "type",
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
PARTICLE_UINT32_FIELDS = ("flags", "dcolour")
PARTICLE_PAYLOAD_FIELDS = tuple(
    field
    for field in PARTICLE_FIELDS
    if field not in ("save_ordinal", "runtime_id", "pixel_x", "pixel_y")
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
CELL_PAYLOAD_FIELDS = CELL_FIELDS[2:]

SETTING_FIELDS = ("name", "kind", "value")
SETTING_KINDS = ("bool", "int", "float")

INT32_MIN = -(2**31)
INT32_MAX = 2**31 - 1
UINT32_MAX = 2**32 - 1
MAX_PARTICLE_ID = 235007
MAX_ELEMENT_ID = 1023
_INT_RE = re.compile(r"^-?[0-9]+$")
_NAME_RE = re.compile(r"^[a-z][a-z0-9_]*$")


def _require_fields(
    actual: Iterable[str] | None, expected: tuple[str, ...], path: Path
) -> None:
    if tuple(actual or ()) != expected:
        raise ValueError(
            f"unexpected CSV fields in {path}: actual={tuple(actual or ())}, "
            f"expected={expected}"
        )


def _reject_extra_columns(raw: dict[str | None, str], path: Path) -> None:
    if None in raw:
        raise ValueError(f"extra CSV columns in {path}")


def _parse_int(text: str, field: str, path: Path) -> int:
    if not _INT_RE.fullmatch(text):
        raise ValueError(f"invalid integer {field}={text!r} in {path}")
    value = int(text, 10)
    if not INT32_MIN <= value <= INT32_MAX:
        raise ValueError(f"int32 field out of range {field}={text!r} in {path}")
    return value


def _parse_nonnegative_int(text: str, field: str, path: Path) -> int:
    value = _parse_int(text, field, path)
    if value < 0:
        raise ValueError(f"negative field {field}={text!r} in {path}")
    return value


def _parse_uint32(text: str, field: str, path: Path) -> int:
    if not _INT_RE.fullmatch(text):
        raise ValueError(f"invalid uint32 {field}={text!r} in {path}")
    value = int(text, 10)
    if not 0 <= value <= UINT32_MAX:
        raise ValueError(f"uint32 field out of range {field}={text!r} in {path}")
    return value


def _parse_float(text: str, field: str, path: Path) -> float:
    try:
        value = float(text)
    except ValueError as exc:
        raise ValueError(f"invalid float {field}={text!r} in {path}") from exc
    if not math.isfinite(value):
        raise ValueError(f"non-finite float {field}={text!r} in {path}")
    return value


def _parse_particle(raw: dict[str | None, str], path: Path) -> dict[str, int | float]:
    _reject_extra_columns(raw, path)
    row: dict[str, int | float] = {}
    for field in PARTICLE_FIELDS:
        text = raw[field]
        if text is None:
            raise ValueError(f"missing particle field {field} in {path}")
        if field in PARTICLE_FLOAT_FIELDS:
            row[field] = _parse_float(text, field, path)
        elif field in PARTICLE_UINT32_FIELDS:
            row[field] = _parse_uint32(text, field, path)
        else:
            row[field] = _parse_int(text, field, path)

    if not 0 <= int(row["save_ordinal"]):
        raise ValueError(f"negative save_ordinal in {path}")
    if not 0 <= int(row["runtime_id"]) <= MAX_PARTICLE_ID:
        raise ValueError(f"runtime_id out of range in {path}")
    if int(row["pixel_x"]) < 0 or int(row["pixel_y"]) < 0:
        raise ValueError(f"negative particle pixel coordinate in {path}")
    if not 1 <= int(row["type"]) <= MAX_ELEMENT_ID:
        raise ValueError(f"particle type out of range in {path}")
    return row


def read_particles(path: Path) -> list[dict[str, int | float]]:
    rows: list[dict[str, int | float]] = []
    runtime_ids: set[int] = set()
    previous_sort_key: tuple[int, int, int] | None = None
    with path.open("r", encoding="utf-8", newline="") as handle:
        reader = csv.DictReader(handle)
        _require_fields(reader.fieldnames, PARTICLE_FIELDS, path)
        for expected_ordinal, raw in enumerate(reader):
            row = _parse_particle(raw, path)
            ordinal = int(row["save_ordinal"])
            if ordinal != expected_ordinal:
                raise ValueError(
                    f"non-sequential save_ordinal in {path}: expected {expected_ordinal}, "
                    f"found {ordinal}"
                )
            runtime_id = int(row["runtime_id"])
            if runtime_id in runtime_ids:
                raise ValueError(f"duplicate runtime_id {runtime_id} in {path}")
            runtime_ids.add(runtime_id)
            sort_key = (int(row["pixel_y"]), int(row["pixel_x"]), runtime_id)
            if previous_sort_key is not None and sort_key < previous_sort_key:
                raise ValueError(f"particle rows are not in save order in {path}")
            previous_sort_key = sort_key
            rows.append(row)
    return rows


def _parse_cell(raw: dict[str | None, str], path: Path) -> dict[str, int | float]:
    _reject_extra_columns(raw, path)
    row: dict[str, int | float] = {}
    for field in CELL_FIELDS:
        text = raw[field]
        if text is None:
            raise ValueError(f"missing cell field {field} in {path}")
        if field in CELL_FLOAT_FIELDS:
            row[field] = _parse_float(text, field, path)
        elif field == "gravity_mask":
            row[field] = _parse_uint32(text, field, path)
        else:
            row[field] = _parse_int(text, field, path)
    if int(row["cx"]) < 0 or int(row["cy"]) < 0:
        raise ValueError(f"negative atmosphere cell coordinate in {path}")
    return row


def read_cells(path: Path) -> dict[tuple[int, int], dict[str, int | float]]:
    cells: dict[tuple[int, int], dict[str, int | float]] = {}
    with path.open("r", encoding="utf-8", newline="") as handle:
        reader = csv.DictReader(handle)
        _require_fields(reader.fieldnames, CELL_FIELDS, path)
        for raw in reader:
            row = _parse_cell(raw, path)
            key = (int(row["cx"]), int(row["cy"]))
            if key in cells:
                raise ValueError(f"duplicate atmosphere cell {key} in {path}")
            cells[key] = row
    if not cells:
        raise ValueError(f"empty atmosphere cell export: {path}")
    return cells


def _parse_setting_value(kind: str, text: str, path: Path) -> bool | int | float:
    if kind == "bool":
        if text == "true":
            return True
        if text == "false":
            return False
        raise ValueError(f"invalid boolean setting value {text!r} in {path}")
    if kind == "int":
        return _parse_int(text, "setting.value", path)
    if kind == "float":
        return _parse_float(text, "setting.value", path)
    raise ValueError(f"unknown setting kind {kind!r} in {path}")


def read_settings(path: Path) -> dict[str, dict[str, Any]]:
    settings: dict[str, dict[str, Any]] = {}
    with path.open("r", encoding="utf-8", newline="") as handle:
        reader = csv.DictReader(handle)
        _require_fields(reader.fieldnames, SETTING_FIELDS, path)
        for raw in reader:
            _reject_extra_columns(raw, path)
            name = raw["name"]
            kind = raw["kind"]
            value_text = raw["value"]
            if name is None or not _NAME_RE.fullmatch(name):
                raise ValueError(f"invalid setting name {name!r} in {path}")
            if kind not in SETTING_KINDS:
                raise ValueError(f"invalid setting kind {kind!r} in {path}")
            if value_text is None:
                raise ValueError(f"missing setting value for {name} in {path}")
            if name in settings:
                raise ValueError(f"duplicate setting {name} in {path}")
            settings[name] = {"kind": kind, "value": _parse_setting_value(kind, value_text, path)}
    if not settings:
        raise ValueError(f"empty settings export: {path}")
    return settings


def _different_fields(
    left: dict[str, Any] | None, right: dict[str, Any] | None, fields: Iterable[str]
) -> list[str]:
    if left is None or right is None:
        return ["presence"]
    return [field for field in fields if left[field] != right[field]]


def _new_summary() -> dict[str, dict[str, int | float]]:
    return {}


def _update_summary(
    summary: dict[str, dict[str, int | float]],
    fields: Iterable[str],
    left: dict[str, Any] | None,
    right: dict[str, Any] | None,
) -> None:
    if left is None or right is None:
        record = summary.setdefault("presence", {"count": 0, "max_abs": 0.0})
        record["count"] = int(record["count"]) + 1
        return
    for field in fields:
        if left[field] == right[field]:
            continue
        record = summary.setdefault(field, {"count": 0, "max_abs": 0.0})
        record["count"] = int(record["count"]) + 1
        if isinstance(left[field], (int, float)) and isinstance(right[field], (int, float)):
            record["max_abs"] = max(
                float(record["max_abs"]), abs(float(left[field]) - float(right[field]))
            )


def _compare_particles(
    before: list[dict[str, int | float]], after: list[dict[str, int | float]], sample_limit: int
) -> dict[str, Any]:
    summary = _new_summary()
    differences: list[dict[str, Any]] = []
    first: dict[str, Any] | None = None
    aligned_pixels = 0
    runtime_id_equal = 0
    all_ordinals = range(max(len(before), len(after)))
    for ordinal in all_ordinals:
        left = before[ordinal] if ordinal < len(before) else None
        right = after[ordinal] if ordinal < len(after) else None
        fields = _different_fields(left, right, PARTICLE_PAYLOAD_FIELDS)
        if left is not None and right is not None:
            if (left["pixel_x"], left["pixel_y"]) == (right["pixel_x"], right["pixel_y"]):
                aligned_pixels += 1
            else:
                fields = ["pixel_position", *fields]
            if left["runtime_id"] == right["runtime_id"]:
                runtime_id_equal += 1
        if not fields:
            continue
        _update_summary(summary, PARTICLE_PAYLOAD_FIELDS, left, right)
        record = {"save_ordinal": ordinal, "fields": fields, "before": left, "loaded": right}
        if first is None:
            first = record
        if len(differences) < sample_limit:
            differences.append({"save_ordinal": ordinal, "fields": fields})
    return {
        "records": {"before_save": len(before), "loaded": len(after)},
        "save_order_pixel_alignment": {
            "aligned_records": aligned_pixels,
            "misaligned_records": min(len(before), len(after)) - aligned_pixels,
        },
        "runtime_id": {
            "equal_records": runtime_id_equal,
            "changed_records": min(len(before), len(after)) - runtime_id_equal,
            "comparison_semantics": "diagnostic_only_not_particle_identity",
        },
        "payload_field_summary": summary,
        "differing_records": sum(1 for ordinal in all_ordinals if (
            _different_fields(
                before[ordinal] if ordinal < len(before) else None,
                after[ordinal] if ordinal < len(after) else None,
                PARTICLE_PAYLOAD_FIELDS,
            )
            or (
                ordinal < len(before)
                and ordinal < len(after)
                and (before[ordinal]["pixel_x"], before[ordinal]["pixel_y"])
                != (after[ordinal]["pixel_x"], after[ordinal]["pixel_y"])
            )
        )),
        "first_difference": first,
        "difference_sample": differences,
    }


def _compare_cells(
    before: dict[tuple[int, int], dict[str, int | float]],
    after: dict[tuple[int, int], dict[str, int | float]],
    sample_limit: int,
) -> dict[str, Any]:
    summary = _new_summary()
    differences: list[dict[str, Any]] = []
    first: dict[str, Any] | None = None
    for key in sorted(set(before) | set(after), key=lambda item: (item[1], item[0])):
        left = before.get(key)
        right = after.get(key)
        fields = _different_fields(left, right, CELL_PAYLOAD_FIELDS)
        if not fields:
            continue
        _update_summary(summary, CELL_PAYLOAD_FIELDS, left, right)
        record = {"cx": key[0], "cy": key[1], "fields": fields, "before": left, "loaded": right}
        if first is None:
            first = record
        if len(differences) < sample_limit:
            differences.append({"cx": key[0], "cy": key[1], "fields": fields})
    return {
        "records": {"before_save": len(before), "loaded": len(after)},
        "same_coordinate_domain": set(before) == set(after),
        "payload_field_summary": summary,
        "differing_cells": sum(
            1
            for key in set(before) | set(after)
            if _different_fields(before.get(key), after.get(key), CELL_PAYLOAD_FIELDS)
        ),
        "first_difference": first,
        "difference_sample": differences,
    }


def _compare_settings(
    before: dict[str, dict[str, Any]], after: dict[str, dict[str, Any]], sample_limit: int
) -> dict[str, Any]:
    differences: list[dict[str, Any]] = []
    first: dict[str, Any] | None = None
    summary = _new_summary()
    for name in sorted(set(before) | set(after)):
        left = before.get(name)
        right = after.get(name)
        fields = _different_fields(left, right, ("kind", "value"))
        if not fields:
            continue
        _update_summary(summary, ("value",), left, right)
        record = {"name": name, "fields": fields, "before": left, "loaded": right}
        if first is None:
            first = record
        if len(differences) < sample_limit:
            differences.append({"name": name, "fields": fields})
    return {
        "records": {"before_save": len(before), "loaded": len(after)},
        "same_name_domain": set(before) == set(after),
        "field_summary": summary,
        "differing_settings": len(differences) if len(differences) < sample_limit else sum(
            1
            for name in set(before) | set(after)
            if _different_fields(before.get(name), after.get(name), ("kind", "value"))
        ),
        "first_difference": first,
        "difference_sample": differences,
    }


def compare_load_boundary(
    before_particles_path: Path,
    loaded_particles_path: Path,
    before_cells_path: Path,
    loaded_cells_path: Path,
    before_settings_path: Path,
    loaded_settings_path: Path,
    *,
    sample_limit: int = 64,
) -> dict[str, Any]:
    if sample_limit <= 0:
        raise ValueError("sample_limit must be positive")
    before_particles = read_particles(before_particles_path)
    loaded_particles = read_particles(loaded_particles_path)
    before_cells = read_cells(before_cells_path)
    loaded_cells = read_cells(loaded_cells_path)
    before_settings = read_settings(before_settings_path)
    loaded_settings = read_settings(loaded_settings_path)
    particles = _compare_particles(before_particles, loaded_particles, sample_limit)
    cells = _compare_cells(before_cells, loaded_cells, sample_limit)
    settings = _compare_settings(before_settings, loaded_settings, sample_limit)
    return {
        "schema_version": 1,
        "status": "PASS",
        "comparison_kind": "ops_pre_save_vs_loaded_field_attribution",
        "claims": {
            "load_boundary_capture_complete": True,
            "particle_runtime_id_is_stable": False,
            "physical_mass_conservation_evaluated": False,
            "physical_momentum_conservation_evaluated": False,
            "physical_energy_conservation_evaluated": False,
            "source_sink_attribution_evaluated": False,
            "bit_exact_in_memory_checkpoint_claimed": False,
        },
        "matching": {
            "particle": "OPS serialization order: rounded pixel y/x then pre-save runtime ID",
            "cell": "cell coordinate",
            "setting": "setting name",
        },
        "particles": particles,
        "cells": cells,
        "settings": settings,
        "sample_limit": sample_limit,
    }


def _write_json(path: Path, value: dict[str, Any]) -> None:
    with path.open("w", encoding="utf-8", newline="\n") as handle:
        handle.write(json.dumps(value, indent=2, sort_keys=True, allow_nan=False) + "\n")


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Attribute field differences across a Legacy OPS save/load boundary"
    )
    parser.add_argument("--before-particles", type=Path, required=True)
    parser.add_argument("--loaded-particles", type=Path, required=True)
    parser.add_argument("--before-cells", type=Path, required=True)
    parser.add_argument("--loaded-cells", type=Path, required=True)
    parser.add_argument("--before-settings", type=Path, required=True)
    parser.add_argument("--loaded-settings", type=Path, required=True)
    parser.add_argument("--sample-limit", type=int, default=64)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    result = compare_load_boundary(
        args.before_particles,
        args.loaded_particles,
        args.before_cells,
        args.loaded_cells,
        args.before_settings,
        args.loaded_settings,
        sample_limit=args.sample_limit,
    )
    _write_json(args.output, result)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
