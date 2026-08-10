#!/usr/bin/env python3
"""Fail-closed validation for the 1.0.5 PhysicalScale candidate contract.

This is an offline benchmark contract. It deliberately does not create a runtime
scale, select a physical dt, or consume Legacy Air fields.
"""

from __future__ import annotations

import argparse
from datetime import date
from decimal import Decimal, InvalidOperation
import json
from pathlib import Path
import re
from typing import Any


DATA_RELATIVE_PATH = Path("resources/omnicore/v1/physical-scale-candidates.json")
UNIT_REGISTRY_RELATIVE_PATH = Path("resources/omnicore/v1/units.json")
DECIMAL_RE = re.compile(r"^(0|[1-9][0-9]*)(\.[0-9]+)?$")
TOP_FIELDS = {
    "schema_version", "document_type", "dataset_version", "selection_status",
    "legacy_geometry", "geometry_candidates", "time_candidates", "prohibitions",
}
GEOMETRY_FIELDS = {
    "id", "status", "pixel_length", "atmosphere_cell_length", "effective_depth",
    "derived_metrics", "coordinate_convention", "provenance",
}
TIME_FIELDS = {
    "id", "status", "legacy_ticks_per_second",
    "candidate_physical_seconds_per_tick", "candidate_physical_seconds_per_tick_unit",
    "time_policy", "presentation_fps_derived",
    "provenance",
}
PROVENANCE_FIELDS = {
    "source_title", "source_version", "source_date", "source_locator", "method",
    "confidence", "tuning_status",
}
EXPECTED_PROHIBITIONS = {
    "legacy_tick_is_not_si_seconds",
    "presentation_fps_is_not_physical_dt",
    "legacy_pv_is_not_pascal",
    "legacy_vx_vy_are_not_metres_per_second",
    "legacy_hv_is_not_authoritative_internal_energy",
    "candidate_values_are_not_production_defaults",
}
REQUIRED_CANONICAL_UNITS = {
    "metre": "length",
    "cubic_metre": "volume",
    "second": "time",
    "metre_per_second_squared": "acceleration",
}


class ValidationFailure(ValueError):
    pass


def _reject_constant(value: str) -> None:
    raise ValidationFailure(f"non-finite JSON constant: {value}")


def _reject_duplicate_keys(pairs: list[tuple[str, Any]]) -> dict[str, Any]:
    result: dict[str, Any] = {}
    for key, value in pairs:
        if key in result:
            raise ValidationFailure(f"duplicate JSON key: {key}")
        result[key] = value
    return result


def load_json_strict(path: Path) -> Any:
    payload = path.read_bytes()
    if payload.startswith(b"\xef\xbb\xbf"):
        raise ValidationFailure(f"{path}: UTF-8 BOM is forbidden")
    try:
        return json.loads(
            payload.decode("utf-8", errors="strict"),
            object_pairs_hook=_reject_duplicate_keys,
            parse_constant=_reject_constant,
        )
    except (UnicodeDecodeError, json.JSONDecodeError, ValidationFailure) as exc:
        raise ValidationFailure(f"{path}: invalid JSON: {exc}") from exc


def _object(value: Any, label: str, required: set[str], allowed: set[str], errors: list[str]) -> dict[str, Any] | None:
    if not isinstance(value, dict):
        errors.append(f"{label}: expected object")
        return None
    missing = sorted(required - set(value))
    unknown = sorted(set(value) - allowed)
    if missing:
        errors.append(f"{label}: missing fields: {', '.join(missing)}")
    if unknown:
        errors.append(f"{label}: unknown fields: {', '.join(unknown)}")
    return value


def _string(value: Any, label: str, errors: list[str]) -> str | None:
    if not isinstance(value, str) or not value or value != value.strip() or any(ord(c) < 0x20 for c in value):
        errors.append(f"{label}: expected trimmed nonempty string")
        return None
    return value


def _decimal(value: Any, label: str, errors: list[str], *, positive: bool = False) -> Decimal | None:
    if not isinstance(value, str) or not DECIMAL_RE.fullmatch(value):
        errors.append(f"{label}: expected canonical decimal string")
        return None
    try:
        parsed = Decimal(value)
    except InvalidOperation:
        errors.append(f"{label}: invalid decimal")
        return None
    if not parsed.is_finite() or (positive and parsed <= 0):
        errors.append(f"{label}: expected finite positive value")
        return None
    return parsed


def _date(value: Any, label: str, errors: list[str]) -> None:
    checked = _string(value, label, errors)
    if checked is None:
        return
    try:
        date.fromisoformat(checked)
    except ValueError:
        errors.append(f"{label}: expected ISO date")


def _provenance(value: Any, label: str, errors: list[str]) -> None:
    item = _object(value, label, PROVENANCE_FIELDS, PROVENANCE_FIELDS, errors)
    if item is None:
        return
    for field in ("source_title", "source_version", "source_locator", "method", "confidence", "tuning_status"):
        _string(item.get(field), f"{label}.{field}", errors)
    _date(item.get("source_date"), f"{label}.source_date", errors)
    if item.get("tuning_status") != "unselected":
        errors.append(f"{label}.tuning_status: candidate must remain unselected")


def _measure(value: Any, label: str, expected_unit: str, errors: list[str]) -> Decimal | None:
    item = _object(value, label, {"value", "unit"}, {"value", "unit"}, errors)
    if item is None:
        return None
    parsed = _decimal(item.get("value"), f"{label}.value", errors, positive=True)
    unit = _string(item.get("unit"), f"{label}.unit", errors)
    if unit != expected_unit:
        errors.append(f"{label}.unit: expected {expected_unit}")
    return parsed


def audit_unit_registry(document: Any) -> list[str]:
    errors: list[str] = []
    if not isinstance(document, dict):
        return ["unit registry: expected object"]
    units = document.get("units")
    if not isinstance(units, list):
        return ["unit registry.units: expected array"]
    declared: dict[str, str] = {}
    for index, unit in enumerate(units):
        if not isinstance(unit, dict):
            errors.append(f"unit registry.units[{index}]: expected object")
            continue
        unit_id = unit.get("id")
        quantity_kind = unit.get("quantity_kind")
        if not isinstance(unit_id, str) or not isinstance(quantity_kind, str):
            errors.append(f"unit registry.units[{index}]: expected id and quantity_kind strings")
            continue
        if unit_id in declared:
            errors.append(f"unit registry.units[{index}]: duplicate unit id {unit_id}")
        declared[unit_id] = quantity_kind
    for unit_id, quantity_kind in REQUIRED_CANONICAL_UNITS.items():
        if declared.get(unit_id) != quantity_kind:
            errors.append(f"unit registry: PhysicalScale requires {unit_id} as {quantity_kind}")
    return errors


def audit_document(document: Any) -> list[str]:
    errors: list[str] = []
    root = _object(document, "document", TOP_FIELDS, TOP_FIELDS, errors)
    if root is None:
        return errors
    if root.get("schema_version") != 1:
        errors.append("document.schema_version: expected 1")
    if root.get("document_type") != "omnicore_physical_scale_candidates":
        errors.append("document.document_type: unsupported document type")
    if root.get("selection_status") != "unselected":
        errors.append("document.selection_status: production selection is forbidden")
    _string(root.get("dataset_version"), "document.dataset_version", errors)

    legacy = _object(
        root.get("legacy_geometry"), "legacy_geometry",
        {"cell_pixels", "air_cells_x", "air_cells_y", "particle_pixels_x", "particle_pixels_y", "legacy_tick_is_physical_seconds", "presentation_fps_is_physical_dt"},
        {"cell_pixels", "air_cells_x", "air_cells_y", "particle_pixels_x", "particle_pixels_y", "legacy_tick_is_physical_seconds", "presentation_fps_is_physical_dt"},
        errors,
    )
    if legacy is not None:
        for key, expected in (("cell_pixels", 4), ("air_cells_x", 153), ("air_cells_y", 96), ("particle_pixels_x", 612), ("particle_pixels_y", 384)):
            value = legacy.get(key)
            if value != str(expected):
                errors.append(f"legacy_geometry.{key}: expected legacy value {expected}")
        if legacy.get("legacy_tick_is_physical_seconds") is not False:
            errors.append("legacy_geometry.legacy_tick_is_physical_seconds: must be false")
        if legacy.get("presentation_fps_is_physical_dt") is not False:
            errors.append("legacy_geometry.presentation_fps_is_physical_dt: must be false")

    geometries = root.get("geometry_candidates")
    if not isinstance(geometries, list) or len(geometries) != 1:
        errors.append("geometry_candidates: exactly one benchmark geometry is required")
    else:
        geometry = _object(geometries[0], "geometry_candidates[0]", GEOMETRY_FIELDS, GEOMETRY_FIELDS, errors)
        if geometry is not None:
            if geometry.get("status") != "benchmark_input":
                errors.append("geometry_candidates[0].status: expected benchmark_input")
            _string(geometry.get("id"), "geometry_candidates[0].id", errors)
            pixel = _measure(geometry.get("pixel_length"), "geometry_candidates[0].pixel_length", "metre", errors)
            cell = _measure(geometry.get("atmosphere_cell_length"), "geometry_candidates[0].atmosphere_cell_length", "metre", errors)
            depth = _measure(geometry.get("effective_depth"), "geometry_candidates[0].effective_depth", "metre", errors)
            if pixel is not None and cell is not None and cell != pixel * 4:
                errors.append("geometry_candidates[0]: cell length must equal four pixel lengths")
            if depth is not None and depth <= 0:
                errors.append("geometry_candidates[0].effective_depth: must be positive")
            derived = _object(
                geometry.get("derived_metrics"), "geometry_candidates[0].derived_metrics",
                {"particle_parcel_volume", "atmosphere_cell_volume", "world_width", "world_height"},
                {"particle_parcel_volume", "atmosphere_cell_volume", "world_width", "world_height"},
                errors,
            )
            if derived is not None and pixel is not None and cell is not None and depth is not None:
                parcel = _measure(derived.get("particle_parcel_volume"), "derived_metrics.particle_parcel_volume", "cubic_metre", errors)
                volume = _measure(derived.get("atmosphere_cell_volume"), "derived_metrics.atmosphere_cell_volume", "cubic_metre", errors)
                width = _measure(derived.get("world_width"), "derived_metrics.world_width", "metre", errors)
                height = _measure(derived.get("world_height"), "derived_metrics.world_height", "metre", errors)
                if parcel is not None and parcel != pixel * pixel * depth:
                    errors.append("derived_metrics.particle_parcel_volume: volume identity failed")
                if volume is not None and volume != cell * cell * depth:
                    errors.append("derived_metrics.atmosphere_cell_volume: volume identity failed")
                if width is not None and width != cell * Decimal(153):
                    errors.append("derived_metrics.world_width: legacy geometry identity failed")
                if height is not None and height != cell * Decimal(96):
                    errors.append("derived_metrics.world_height: legacy geometry identity failed")
            convention = _object(
                geometry.get("coordinate_convention"), "coordinate_convention",
                {"origin", "positive_x", "positive_y", "cell_sample", "gravity_vector_unit", "gravity_source"},
                {"origin", "positive_x", "positive_y", "cell_sample", "gravity_vector_unit", "gravity_source"},
                errors,
            )
            if convention is not None:
                if convention.get("origin") != "top_left" or convention.get("positive_x") != "right" or convention.get("positive_y") != "down":
                    errors.append("coordinate_convention: unsupported coordinate convention")
                if convention.get("cell_sample") != "cell_center":
                    errors.append("coordinate_convention.cell_sample: expected cell_center")
                if convention.get("gravity_vector_unit") != "metre_per_second_squared":
                    errors.append("coordinate_convention.gravity_vector_unit: expected SI acceleration unit")
                if convention.get("gravity_source") != "explicit_benchmark_input":
                    errors.append("coordinate_convention.gravity_source: expected explicit_benchmark_input")
            _provenance(geometry.get("provenance"), "geometry_candidates[0].provenance", errors)

    times = root.get("time_candidates")
    if not isinstance(times, list) or len(times) < 3:
        errors.append("time_candidates: at least three unselected policies are required")
    else:
        seen: set[str] = set()
        for index, item in enumerate(times):
            time = _object(item, f"time_candidates[{index}]", TIME_FIELDS, TIME_FIELDS, errors)
            if time is None:
                continue
            identifier = _string(time.get("id"), f"time_candidates[{index}].id", errors)
            if identifier in seen:
                errors.append(f"time_candidates[{index}].id: duplicate id")
            if identifier is not None:
                seen.add(identifier)
            if time.get("status") != "benchmark_input" or time.get("time_policy") != "unselected":
                errors.append(f"time_candidates[{index}]: time policy must remain unselected benchmark input")
            if time.get("presentation_fps_derived") is not False:
                errors.append(f"time_candidates[{index}].presentation_fps_derived: must be false")
            ticks = _decimal(time.get("legacy_ticks_per_second"), f"time_candidates[{index}].legacy_ticks_per_second", errors, positive=True)
            seconds = time.get("candidate_physical_seconds_per_tick")
            seconds_unit = time.get("candidate_physical_seconds_per_tick_unit")
            if seconds is not None:
                parsed_seconds = _decimal(seconds, f"time_candidates[{index}].candidate_physical_seconds_per_tick", errors, positive=True)
                if seconds_unit != "second":
                    errors.append(f"time_candidates[{index}].candidate_physical_seconds_per_tick_unit: expected second")
                if ticks is not None and parsed_seconds is not None and abs(parsed_seconds - (Decimal(1) / ticks)) > Decimal("1e-16"):
                    errors.append(f"time_candidates[{index}]: candidate tick does not match its declared frequency")
            elif seconds_unit is not None:
                errors.append(f"time_candidates[{index}].candidate_physical_seconds_per_tick_unit: must be null when no candidate seconds are declared")
            _provenance(time.get("provenance"), f"time_candidates[{index}].provenance", errors)

    prohibitions = root.get("prohibitions")
    if (
        not isinstance(prohibitions, list)
        or any(not isinstance(item, str) for item in prohibitions)
        or set(prohibitions) != EXPECTED_PROHIBITIONS
    ):
        errors.append("prohibitions: required Legacy and candidate-default prohibitions drifted")
    return errors


def audit(source_root: Path) -> list[str]:
    path = source_root / DATA_RELATIVE_PATH
    unit_registry_path = source_root / UNIT_REGISTRY_RELATIVE_PATH
    try:
        document = load_json_strict(path)
        unit_registry = load_json_strict(unit_registry_path)
    except (OSError, ValidationFailure) as exc:
        return [str(exc)]
    return audit_document(document) + audit_unit_registry(unit_registry)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--quiet", action="store_true")
    args = parser.parse_args()
    errors = audit(args.source_root.resolve())
    if errors:
        for error in errors:
            print(f"physical-scale: ERROR {error}")
        return 1
    if not args.quiet:
        print("PhysicalScale candidate validation passed: selection_status=unselected, geometry_candidates=1, time_candidates>=3")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
