#!/usr/bin/env python3
"""Validate one stress artifact directory and write a bounded observation assessment."""

from __future__ import annotations

import argparse
import csv
import hashlib
import json
from pathlib import Path
import sys
from typing import Any, Sequence


REQUIRED_RESULT_FIELDS = {
    "schema_version",
    "sample_id",
    "run_id",
    "source_commit",
    "public_zip_sha256",
    "exe_sha256",
    "input_ops_sha256",
    "output_ops_second_sha256",
    "warmup_seconds",
    "sample_seconds",
    "initial_particles",
    "peak_particles",
    "final_particles",
    "average_fps",
    "one_percent_low_fps",
    "minimum_fps",
    "peak_working_set_bytes",
    "peak_private_bytes",
    "crashed",
    "hung",
    "roundtrip_pass",
    "long_run",
    "smoke_run",
}

FIXTURE_EXPECTATIONS = {
    "S15-PERIODIC-ALL": (118, 118, 118),
    "S16-INORGANIC-DENSE": (50, 50, 50),
    "S17-MATERIALS-DENSE": (21, 21, 21),
    "S18-ISOTOPES-DENSE": (13, 13, 13),
    "S19-ORGANICS-DENSE": (33, 33, 33),
    "S20-FULL-CATALOG": (487, 484, 466),
}


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest().upper()


def read_json(path: Path) -> dict[str, Any]:
    value = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(value, dict):
        raise ValueError(f"{path.name} must contain a JSON object")
    return value


def read_numeric_series(path: Path, fields: Sequence[str]) -> list[dict[str, float]]:
    with path.open(encoding="utf-8-sig", newline="") as stream:
        reader = csv.DictReader(stream)
        if reader.fieldnames is None or any(field not in reader.fieldnames for field in fields):
            raise ValueError(f"{path.name} lacks required columns: {', '.join(fields)}")
        rows: list[dict[str, float]] = []
        for row_number, row in enumerate(reader, start=2):
            try:
                rows.append({field: float(row[field]) for field in fields})
            except (TypeError, ValueError) as exc:
                raise ValueError(f"{path.name}:{row_number}: invalid numeric value") from exc
    if len(rows) < 2:
        raise ValueError(f"{path.name} needs at least two data rows")
    elapsed = [row[fields[0]] for row in rows]
    if any(right < left for left, right in zip(elapsed, elapsed[1:])):
        raise ValueError(f"{path.name} elapsed time is not monotonic")
    return rows


def strictly_observed_monotonic_growth(values: Sequence[float]) -> bool:
    """True only for nondecreasing observations with at least one increase."""
    return (
        len(values) >= 2
        and values[-1] > values[0]
        and all(right >= left for left, right in zip(values, values[1:]))
    )


def tail_values(rows: Sequence[dict[str, float]], field: str, fraction: float) -> list[float]:
    first = max(0, int(len(rows) * (1.0 - fraction)))
    return [row[field] for row in rows[first:]]


def nonnegative_number(value: Any) -> bool:
    return (
        isinstance(value, (int, float))
        and not isinstance(value, bool)
        and value >= 0
    )


def validate_ops(path: Path, expected_hash: str) -> dict[str, Any]:
    data = path.read_bytes()
    if len(data) <= 15 or data[:4] != b"OPS1" or data[12:15] != b"BZh":
        raise ValueError(f"{path.name} is not an OPS1/BZip2 container")
    actual_hash = sha256(path)
    if actual_hash != expected_hash:
        raise ValueError(
            f"{path.name} SHA-256 mismatch: expected {expected_hash}, got {actual_hash}"
        )
    return {"bytes": len(data), "sha256": actual_hash}


def analyze(directory: Path) -> dict[str, Any]:
    result_path = directory / "result.json"
    frame_path = directory / "frame-series.csv"
    process_path = directory / "process-series.csv"
    input_ops_path = directory / "input-first.stm"
    output_ops_path = directory / "output-second.stm"
    for path in (result_path, frame_path, process_path, input_ops_path, output_ops_path):
        if not path.is_file():
            raise ValueError(f"missing artifact: {path.name}")

    result = read_json(result_path)
    missing = sorted(REQUIRED_RESULT_FIELDS - result.keys())
    if missing:
        raise ValueError(f"result.json lacks fields: {', '.join(missing)}")
    if not isinstance(result["long_run"], bool):
        raise ValueError("result.json long_run must be a boolean")
    frames = read_numeric_series(
        frame_path,
        ("elapsed_seconds", "frames", "particles"),
    )
    processes = read_numeric_series(
        process_path,
        ("elapsed_seconds", "working_set_bytes", "private_bytes"),
    )
    input_ops = validate_ops(input_ops_path, str(result["input_ops_sha256"]))
    output_ops = validate_ops(
        output_ops_path,
        str(result["output_ops_second_sha256"]),
    )

    particle_tail = tail_values(frames, "particles", 0.5)
    working_set_tail = tail_values(processes, "working_set_bytes", 0.25)
    private_tail = tail_values(processes, "private_bytes", 0.25)
    sustained_particle_growth = strictly_observed_monotonic_growth(particle_tail)
    sustained_working_set_growth = strictly_observed_monotonic_growth(
        working_set_tail
    )
    sustained_private_growth = strictly_observed_monotonic_growth(private_tail)
    memory_leak_suspected = (
        sustained_working_set_growth and sustained_private_growth
    )

    duration_pass = (
        not bool(result["smoke_run"])
        and float(result["warmup_seconds"]) >= 60.0
        and float(result["sample_seconds"]) >= 600.0
    )
    runtime_pass = (
        not bool(result["crashed"])
        and not bool(result["hung"])
        and bool(result["roundtrip_pass"])
    )
    sample_execution_pass = (
        duration_pass
        and runtime_pass
        and not sustained_particle_growth
        and not memory_leak_suspected
    )
    event_evidence_complete = nonnegative_number(
        result.get("event_count_total")
    ) and nonnegative_number(
        result.get("event_count_peak_per_frame")
    )
    stop_pass = result.get("scenario_stop_pass")
    recovery_pass = result.get("scenario_recovery_pass")
    stop_event_delta = result.get("stop_event_delta")
    recovery_assertions = result.get("scenario_recovery_assertions")
    scenario_evidence_complete = (
        isinstance(stop_pass, bool)
        and isinstance(recovery_pass, bool)
        and isinstance(stop_event_delta, (int, float))
        and not isinstance(stop_event_delta, bool)
        and nonnegative_number(recovery_assertions)
    )
    scenario_behavior_pass: bool | str
    if scenario_evidence_complete:
        scenario_behavior_pass = (
            stop_pass
            and recovery_pass
            and stop_event_delta == 0
            and recovery_assertions > 0
        )
    else:
        scenario_behavior_pass = "not_tested"
    signal_evidence_complete = (
        nonnegative_number(result.get("signal_count_total"))
        and nonnegative_number(result.get("signal_count_peak_per_frame"))
        and isinstance(result.get("signal_stop_pass"), bool)
    )
    automation_sample = result.get("sample_id") in {
        "S11-AUTOMATION-FACTORY",
        "S12-AUTOMATION-SIGNAL-LOOP",
    }
    signal_behavior_pass: bool | str
    if signal_evidence_complete:
        signal_total = result.get("signal_count_total")
        signal_peak = result.get("signal_count_peak_per_frame")
        peak_particles = result.get("peak_particles")
        signal_behavior_pass = (
            bool(result.get("signal_stop_pass"))
            and isinstance(signal_total, (int, float))
            and not isinstance(signal_total, bool)
            and isinstance(signal_peak, (int, float))
            and not isinstance(signal_peak, bool)
            and isinstance(peak_particles, (int, float))
            and not isinstance(peak_particles, bool)
            and signal_total > 0
            and signal_peak > 0
            and signal_peak <= peak_particles
        )
    else:
        signal_behavior_pass = "not_tested"
    fixture_expected = FIXTURE_EXPECTATIONS.get(str(result.get("sample_id")))
    fixture_type_count = result.get("fixture_type_count")
    fixture_created_type_count = result.get("fixture_created_type_count")
    fixture_visible_type_count = result.get("fixture_visible_type_count")
    if fixture_expected is None:
        fixture_evidence_complete = True
        fixture_activity_pass = True
    else:
        expected_types, expected_created, expected_visible = fixture_expected
        fixture_evidence_complete = (
            isinstance(fixture_type_count, (int, float))
            and not isinstance(fixture_type_count, bool)
            and fixture_type_count == expected_types
            and isinstance(fixture_created_type_count, (int, float))
            and not isinstance(fixture_created_type_count, bool)
            and fixture_created_type_count > 0
            and isinstance(fixture_visible_type_count, (int, float))
            and not isinstance(fixture_visible_type_count, bool)
            and fixture_visible_type_count == expected_visible
            and (
                expected_created is None
                or fixture_created_type_count == expected_created
            )
        )
        fixture_activity_pass = (
            isinstance(result.get("event_count_total"), (int, float))
            and not isinstance(result.get("event_count_total"), bool)
            and result.get("event_count_total") > 0
            and isinstance(result.get("event_count_peak_per_frame"), (int, float))
            and not isinstance(result.get("event_count_peak_per_frame"), bool)
            and result.get("event_count_peak_per_frame") > 0
        )
    base_performance_gate_pass = (
        sample_execution_pass
        and event_evidence_complete
        and scenario_behavior_pass is True
        and fixture_evidence_complete
        and fixture_activity_pass
        and (not automation_sample or signal_behavior_pass is True)
    )

    long_run_requested = result["long_run"]
    if long_run_requested:
        long_run_evidence_complete: bool | str = (
            result.get("sample_id") == "S20-FULL-CATALOG"
            and nonnegative_number(result.get("long_run_save_load_cycles"))
            and nonnegative_number(result.get("long_run_language_switches"))
            and nonnegative_number(result.get("long_run_module_toggle_cycles"))
            and isinstance(result.get("long_run_settings_recovery_pass"), bool)
            and nonnegative_number(
                result.get("long_run_checkpoint_save_ms_total")
            )
            and nonnegative_number(
                result.get("long_run_checkpoint_load_ms_total")
            )
        )
        long_run_duration_pass: bool | str = (
            not bool(result["smoke_run"])
            and float(result["warmup_seconds"]) >= 60.0
            and float(result["sample_seconds"]) >= 7200.0
        )
        long_run_behavior_pass: bool | str = (
            long_run_evidence_complete is True
            and result.get("long_run_save_load_cycles", 0) >= 10
            and result.get("long_run_language_switches", 0) >= 10
            and result.get("long_run_module_toggle_cycles", 0) >= 10
            and result.get("long_run_settings_recovery_pass") is True
        )
        long_run_gate_pass: bool | str = (
            base_performance_gate_pass
            and long_run_duration_pass is True
            and long_run_behavior_pass is True
        )
    else:
        long_run_evidence_complete = "not_tested"
        long_run_duration_pass = "not_tested"
        long_run_behavior_pass = "not_tested"
        long_run_gate_pass = "not_tested"

    performance_gate_pass = (
        base_performance_gate_pass
        and (not long_run_requested or long_run_gate_pass is True)
    )

    return {
        "assessment_schema_version": 1,
        "assessment_status": "PASS",
        "sample_id": result["sample_id"],
        "run_id": result["run_id"],
        "source_commit": result["source_commit"],
        "public_zip_sha256": result["public_zip_sha256"],
        "exe_sha256": result["exe_sha256"],
        "result_json_sha256": sha256(result_path),
        "frame_samples": len(frames),
        "process_samples": len(processes),
        "observed_particle_min": int(min(row["particles"] for row in frames)),
        "observed_particle_max": int(max(row["particles"] for row in frames)),
        "particle_tail_first": int(particle_tail[0]),
        "particle_tail_last": int(particle_tail[-1]),
        "unbounded_growth": sustained_particle_growth,
        "working_set_tail_first": int(working_set_tail[0]),
        "working_set_tail_last": int(working_set_tail[-1]),
        "private_tail_first": int(private_tail[0]),
        "private_tail_last": int(private_tail[-1]),
        "memory_leak_suspected": memory_leak_suspected,
        "input_ops": input_ops,
        "output_ops": output_ops,
        "duration_pass": duration_pass,
        "runtime_pass": runtime_pass,
        "sample_execution_pass": sample_execution_pass,
        "event_evidence_complete": event_evidence_complete,
        "event_count_total": result.get("event_count_total"),
        "event_count_peak_per_frame": result.get("event_count_peak_per_frame"),
        "signal_evidence_complete": signal_evidence_complete,
        "signal_count_total": result.get("signal_count_total"),
        "signal_count_peak_per_frame": result.get("signal_count_peak_per_frame"),
        "signal_behavior_pass": signal_behavior_pass,
        "signal_stop_pass": result.get("signal_stop_pass"),
        "fixture_evidence_complete": fixture_evidence_complete,
        "fixture_activity_pass": fixture_activity_pass,
        "fixture_type_count": fixture_type_count,
        "fixture_created_type_count": fixture_created_type_count,
        "fixture_visible_type_count": fixture_visible_type_count,
        "scenario_behavior_pass": scenario_behavior_pass,
        "stop_event_delta": stop_event_delta,
        "scenario_recovery_assertions": recovery_assertions,
        "long_run_requested": long_run_requested,
        "long_run_evidence_complete": long_run_evidence_complete,
        "long_run_duration_pass": long_run_duration_pass,
        "long_run_behavior_pass": long_run_behavior_pass,
        "long_run_save_load_cycles": result.get("long_run_save_load_cycles"),
        "long_run_language_switches": result.get("long_run_language_switches"),
        "long_run_module_toggle_cycles": result.get("long_run_module_toggle_cycles"),
        "long_run_settings_recovery_pass": result.get(
            "long_run_settings_recovery_pass"
        ),
        "long_run_gate_pass": long_run_gate_pass,
        "performance_gate_pass": performance_gate_pass,
        "classification_note": (
            "unbounded_growth reports only whether the final half of observed "
            "particle samples is nondecreasing with at least one increase; "
            "memory_leak_suspected requires the final quarter of both working-set "
            "and private-byte samples to meet the same finite-observation rule. "
            "This is not a proof of long-term boundedness."
        ),
    }


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("artifact_directory", type=Path)
    parser.add_argument("--output", type=Path)
    return parser


def main(argv: Sequence[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    directory = args.artifact_directory.resolve()
    output = args.output.resolve() if args.output else directory / "assessment.json"
    try:
        assessment = analyze(directory)
        output.write_text(
            json.dumps(assessment, ensure_ascii=False, indent=2) + "\n",
            encoding="utf-8",
            newline="\n",
        )
    except (OSError, ValueError, json.JSONDecodeError) as exc:
        print(f"stress-result-analysis: ERROR {exc}", file=sys.stderr)
        return 1
    print(
        "stress-result-analysis: PASS "
        f"sample={assessment['sample_id']} "
        f"execution={str(assessment['sample_execution_pass']).lower()} "
        f"long_run={str(assessment['long_run_gate_pass']).lower()} "
        f"gate={str(assessment['performance_gate_pass']).lower()} "
        f"output={output}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
