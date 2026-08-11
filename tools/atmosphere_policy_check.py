#!/usr/bin/env python3
"""Validate and recompute the Phase 5 Atmosphere budget/time-policy matrix."""

from __future__ import annotations

import argparse
from decimal import Decimal, InvalidOperation, ROUND_CEILING, ROUND_FLOOR
import json
from pathlib import Path
import re
from typing import Any


DATA_RELATIVE_PATH = Path("resources/omnicore/v1/atmosphere-policy-candidates.json")
DECIMAL_RE = re.compile(r"^(0|[1-9][0-9]*)(\.[0-9]+)?$")
EXPECTED_POLICIES = {
    "direct_real_acoustic_60hz_explicit_hllc",
    "budget_limited_uniform_acoustic_scaling",
    "hybrid_all_speed_event_local_compressible",
}
EXPECTED_PROHIBITIONS = {
    "reference_frame_is_not_presentation_fps_coupling",
    "reference_machine_budget_is_not_cross_hardware_release_requirement",
    "acoustic_reference_is_not_runtime_material_database",
    "rejected_policy_is_not_solver_selection",
    "hybrid_2d_physical_policy_is_not_implemented",
    "physical_time_policy_remains_unselected",
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


def _decimal(value: Any, label: str, errors: list[str], *, positive: bool = True) -> Decimal | None:
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


def _integer(value: Any, label: str, errors: list[str]) -> int | None:
    parsed = _decimal(value, label, errors)
    if parsed is None:
        return None
    integral = parsed.to_integral_value()
    if parsed != integral:
        errors.append(f"{label}: expected integer string")
        return None
    return int(integral)


def _measure(value: Any, label: str, unit: str, errors: list[str]) -> Decimal | None:
    if not isinstance(value, dict) or set(value) != {"value", "unit"}:
        errors.append(f"{label}: expected value/unit object")
        return None
    if value.get("unit") != unit:
        errors.append(f"{label}.unit: expected {unit}")
    return _decimal(value.get("value"), f"{label}.value", errors)


def _require_keys(value: Any, label: str, keys: set[str], errors: list[str]) -> dict[str, Any] | None:
    if not isinstance(value, dict):
        errors.append(f"{label}: expected object")
        return None
    missing = sorted(keys - set(value))
    unknown = sorted(set(value) - keys)
    if missing:
        errors.append(f"{label}: missing fields: {', '.join(missing)}")
    if unknown:
        errors.append(f"{label}: unknown fields: {', '.join(unknown)}")
    return value


def audit_document(document: Any) -> tuple[list[str], dict[str, str]]:
    errors: list[str] = []
    derived: dict[str, str] = {}
    top_keys = {
        "schema_version", "document_type", "dataset_version", "selection_status",
        "reference_grid", "time_reference", "acoustic_reference",
        "solver_measurement", "legacy_reference", "phase5_reference_budget",
        "policy_evaluations", "prohibitions",
    }
    root = _require_keys(document, "document", top_keys, errors)
    if root is None:
        return errors, derived
    if root.get("schema_version") != 1:
        errors.append("document.schema_version: expected 1")
    if root.get("document_type") != "omnicore_atmosphere_policy_candidates":
        errors.append("document.document_type: unsupported document type")
    if root.get("selection_status") != "unselected":
        errors.append("document.selection_status: physical-time policy must remain unselected")

    grid = _require_keys(root.get("reference_grid"), "reference_grid", {"cells_x", "cells_y", "cell_count", "cell_length"}, errors)
    time = _require_keys(root.get("time_reference"), "time_reference", {"ticks_per_game_second", "seconds_per_tick_candidate", "presentation_fps_derived"}, errors)
    acoustic = _require_keys(root.get("acoustic_reference"), "acoustic_reference", {"speed", "temperature", "material", "usage", "provenance"}, errors)
    solver = _require_keys(root.get("solver_measurement"), "solver_measurement", {"candidate", "source_commit", "strict_double_single_thread", "milliseconds_per_substep", "target_cfl", "authoritative_bytes_per_cell", "working_bytes_per_cell", "measurement_scope"}, errors)
    legacy = _require_keys(root.get("legacy_reference"), "legacy_reference", {"source_commit", "scene", "strict_milliseconds_per_tick", "measurement_scope"}, errors)
    budget = _require_keys(root.get("phase5_reference_budget"), "phase5_reference_budget", {"status", "frame_reference_milliseconds", "atmosphere_fraction", "atmosphere_milliseconds_per_tick", "authoritative_bytes_per_cell_maximum", "working_bytes_per_cell_maximum", "scope", "provenance"}, errors)
    if None in (grid, time, acoustic, solver, legacy, budget):
        return errors, derived

    cells_x = _integer(grid.get("cells_x"), "reference_grid.cells_x", errors)
    cells_y = _integer(grid.get("cells_y"), "reference_grid.cells_y", errors)
    cell_count = _integer(grid.get("cell_count"), "reference_grid.cell_count", errors)
    cell_length = _measure(grid.get("cell_length"), "reference_grid.cell_length", "metre", errors)
    ticks = _decimal(time.get("ticks_per_game_second"), "time_reference.ticks_per_game_second", errors)
    seconds_per_tick = _measure(time.get("seconds_per_tick_candidate"), "time_reference.seconds_per_tick_candidate", "second", errors)
    sound_speed = _measure(acoustic.get("speed"), "acoustic_reference.speed", "metre_per_second", errors)
    _measure(acoustic.get("temperature"), "acoustic_reference.temperature", "kelvin", errors)
    step_ms = _decimal(solver.get("milliseconds_per_substep"), "solver_measurement.milliseconds_per_substep", errors)
    cfl = _decimal(solver.get("target_cfl"), "solver_measurement.target_cfl", errors)
    state_bytes = _integer(solver.get("authoritative_bytes_per_cell"), "solver_measurement.authoritative_bytes_per_cell", errors)
    working_bytes = _integer(solver.get("working_bytes_per_cell"), "solver_measurement.working_bytes_per_cell", errors)
    _decimal(legacy.get("strict_milliseconds_per_tick"), "legacy_reference.strict_milliseconds_per_tick", errors)
    frame_ms = _decimal(budget.get("frame_reference_milliseconds"), "phase5_reference_budget.frame_reference_milliseconds", errors)
    atmosphere_fraction = _decimal(budget.get("atmosphere_fraction"), "phase5_reference_budget.atmosphere_fraction", errors)
    atmosphere_budget_ms = _decimal(budget.get("atmosphere_milliseconds_per_tick"), "phase5_reference_budget.atmosphere_milliseconds_per_tick", errors)
    state_budget = _integer(budget.get("authoritative_bytes_per_cell_maximum"), "phase5_reference_budget.authoritative_bytes_per_cell_maximum", errors)
    working_budget = _integer(budget.get("working_bytes_per_cell_maximum"), "phase5_reference_budget.working_bytes_per_cell_maximum", errors)

    if cells_x is not None and cells_y is not None and cell_count != cells_x * cells_y:
        errors.append("reference_grid.cell_count: grid identity failed")
    if time.get("presentation_fps_derived") is not False:
        errors.append("time_reference.presentation_fps_derived: must be false")
    if ticks is not None and seconds_per_tick is not None and abs(seconds_per_tick - Decimal(1) / ticks) > Decimal("1e-16"):
        errors.append("time_reference: tick interval identity failed")
    if acoustic.get("usage") != "cfl_feasibility_reference_only":
        errors.append("acoustic_reference.usage: runtime-property claim is forbidden")
    provenance = acoustic.get("provenance")
    if (
        not isinstance(provenance, dict)
        or provenance.get("source_locator") != "https://ntrs.nasa.gov/citations/19720017735"
        or provenance.get("access_date") != "2026-08-11"
        or provenance.get("temperature_validity") != "approximately_293_kelvin"
        or provenance.get("redistribution_status") != "public_us_government_work"
        or "Public Use Permitted" not in provenance.get("license_evidence", "")
    ):
        errors.append("acoustic_reference.provenance: NASA source/license binding drifted")
    if solver.get("candidate") != "fvm_hllc_rusanov_fallback" or solver.get("strict_double_single_thread") is not True:
        errors.append("solver_measurement: expected strict-double single-thread HLLC checkpoint")
    if budget.get("status") != "accepted_reference_machine_target":
        errors.append("phase5_reference_budget.status: reference-machine target must be explicit")
    if frame_ms is not None and atmosphere_fraction is not None and atmosphere_budget_ms is not None and abs(frame_ms * atmosphere_fraction - atmosphere_budget_ms) > Decimal("1e-15"):
        errors.append("phase5_reference_budget: atmosphere fraction identity failed")
    if state_bytes is not None and state_budget is not None and state_bytes > state_budget:
        errors.append("solver_measurement: authoritative memory exceeds budget")
    if working_bytes is not None and working_budget is not None and working_bytes > working_budget:
        errors.append("solver_measurement: working memory exceeds budget")

    policies = root.get("policy_evaluations")
    if not isinstance(policies, list):
        errors.append("policy_evaluations: expected array")
        policies_by_id: dict[str, Any] = {}
    else:
        policies_by_id = {item.get("id"): item for item in policies if isinstance(item, dict)}
        if set(policies_by_id) != EXPECTED_POLICIES or len(policies_by_id) != len(policies):
            errors.append("policy_evaluations: required unique policy set drifted")

    if all(value is not None for value in (sound_speed, seconds_per_tick, cfl, cell_length, step_ms, atmosphere_budget_ms)):
        required_substeps = int((sound_speed * seconds_per_tick / (cfl * cell_length)).to_integral_value(rounding=ROUND_CEILING))
        atmosphere_ms = step_ms * required_substeps
        maximum_substeps = int((atmosphere_budget_ms / step_ms).to_integral_value(rounding=ROUND_FLOOR))
        maximum_signal_speed = (
            Decimal(maximum_substeps) * cfl * cell_length / seconds_per_tick
        ).quantize(Decimal("0.000000000001"))
        derived = {
            "required_real_acoustic_substeps": str(required_substeps),
            "required_real_acoustic_milliseconds": str(atmosphere_ms),
            "maximum_budgeted_substeps": str(maximum_substeps),
            "maximum_budgeted_signal_speed": str(maximum_signal_speed.normalize()),
        }
        direct = policies_by_id.get("direct_real_acoustic_60hz_explicit_hllc", {})
        if direct.get("status") != "rejected_reference_machine_budget" or direct.get("expected_required_substeps") != str(required_substeps) or direct.get("expected_atmosphere_milliseconds_per_tick") != str(atmosphere_ms):
            errors.append("policy_evaluations.direct_real_acoustic: recomputed rejection drifted")
        scaled = policies_by_id.get("budget_limited_uniform_acoustic_scaling", {})
        if scaled.get("status") != "rejected_default_realism" or scaled.get("expected_maximum_substeps") != str(maximum_substeps):
            errors.append("policy_evaluations.acoustic_scaling: budget result drifted")
        measured_limit = _measure(scaled.get("expected_maximum_signal_speed"), "policy_evaluations.acoustic_scaling.expected_maximum_signal_speed", "metre_per_second", errors)
        if measured_limit is not None and measured_limit != maximum_signal_speed:
            errors.append("policy_evaluations.acoustic_scaling: signal-speed result drifted")
        hybrid = policies_by_id.get("hybrid_all_speed_event_local_compressible", {})
        if hybrid.get("status") != "mixed_region_1d_probe_passed_2d_physical_domain_budget_unimplemented":
            errors.append("policy_evaluations.hybrid: 1d/physical-policy boundary drifted")

    prohibitions = root.get("prohibitions")
    if not isinstance(prohibitions, list) or set(prohibitions) != EXPECTED_PROHIBITIONS or len(prohibitions) != len(EXPECTED_PROHIBITIONS):
        errors.append("prohibitions: required policy boundaries drifted")
    return errors, derived


def audit(source_root: Path) -> tuple[list[str], dict[str, str]]:
    try:
        document = load_json_strict(source_root / DATA_RELATIVE_PATH)
    except (OSError, ValidationFailure) as exc:
        return [str(exc)], {}
    return audit_document(document)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--quiet", action="store_true")
    args = parser.parse_args()
    errors, derived = audit(args.source_root.resolve())
    if errors:
        for error in errors:
            print(f"atmosphere-policy: ERROR {error}")
        return 1
    if not args.quiet:
        print("Atmosphere policy matrix passed: " + ", ".join(f"{key}={value}" for key, value in derived.items()))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
