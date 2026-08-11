from __future__ import annotations

import argparse
import hashlib
import json
import math
from pathlib import Path
import subprocess
import sys


CASES = ("uniform", "pressure_pulse", "near_vacuum", "sod")
SIGNATURES = ("density_signature", "momentum_signature", "energy_signature")


def parse_output(text: str) -> dict[str, str]:
    result: dict[str, str] = {}
    for line in text.splitlines():
        if "=" not in line:
            continue
        key, value = line.split("=", 1)
        result[key.strip()] = value.strip()
    return result


def run_binary(path: Path) -> tuple[dict[str, str], str]:
    completed = subprocess.run([str(path)], check=False, capture_output=True, text=True)
    if completed.returncode != 0:
        raise RuntimeError(f"precision binary failed: {path} exit={completed.returncode}\n{completed.stdout}\n{completed.stderr}")
    data = parse_output(completed.stdout)
    if data.get("precision_probe_passed") != "true":
        raise RuntimeError(f"precision probe did not pass: {path}")
    return data, hashlib.sha256(path.read_bytes()).hexdigest().upper()


def number(data: dict[str, str], key: str) -> float:
    value = float(data[key])
    if not math.isfinite(value):
        raise RuntimeError(f"non-finite metric: {key}")
    return value


def relative_delta(value: float, reference: float) -> float:
    return abs(value - reference) / max(abs(reference), 1e-30)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--strict-double", type=Path, required=True)
    parser.add_argument("--strict-float", type=Path, required=True)
    parser.add_argument("--fast-float", type=Path, required=True)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()

    paths = {
        "strict_double": args.strict_double.resolve(),
        "strict_float": args.strict_float.resolve(),
        "fast_float": args.fast_float.resolve(),
    }
    runs: dict[str, dict[str, object]] = {}
    raw: dict[str, dict[str, str]] = {}
    for expected, path in paths.items():
        data, digest = run_binary(path)
        if data.get("precision_mode") != expected:
            raise RuntimeError(f"precision mode mismatch: expected={expected} actual={data.get('precision_mode')}")
        raw[expected] = data
        runs[expected] = {
            "path": str(path),
            "sha256": digest,
            "scalar_bytes": int(data["scalar_bytes"]),
            "fast_math_contract": data["fast_math_contract"] == "true",
            "cases": {},
        }
        for case in CASES:
            runs[expected]["cases"][case] = {
                key: number(data, f"{case}_{key}")
                for key in (
                    "mass_drift", "momentum_drift", "energy_drift",
                    "minimum_density", "minimum_pressure", "maximum_pressure",
                    *SIGNATURES, "maximum_cfl",
                )
            }

    comparison: dict[str, dict[str, dict[str, float]]] = {}
    reference = raw["strict_double"]
    for mode in ("strict_float", "fast_float"):
        comparison[mode] = {}
        for case in CASES:
            comparison[mode][case] = {
                metric: relative_delta(number(raw[mode], f"{case}_{metric}"),
                                       number(reference, f"{case}_{metric}"))
                for metric in SIGNATURES
            }

    max_signature_delta = {
        mode: max(delta for case in comparison[mode].values() for delta in case.values())
        for mode in comparison
    }
    strict_float_positive = all(raw["strict_float"][f"{case}_positive"] == "true" for case in CASES)
    fast_float_positive = all(raw["fast_float"][f"{case}_positive"] == "true" for case in CASES)
    result = {
        "schema_version": 1,
        "matrix_status": "RECORDED_NOT_SOLVER_SELECTION",
        "algorithm": "first_order_rusanov_same_template",
        "runs": runs,
        "relative_signature_delta_vs_strict_double": comparison,
        "maximum_signature_delta_vs_strict_double": max_signature_delta,
        "strict_float_positive": strict_float_positive,
        "fast_float_positive": fast_float_positive,
        "fast_math_safety_selected": False,
        "atmosphere_solver_selection": "unselected",
    }
    if not strict_float_positive or not fast_float_positive:
        raise RuntimeError("precision matrix lost positivity")
    encoded = json.dumps(result, indent=2, sort_keys=True) + "\n"
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(encoded, encoding="utf-8")
    sys.stdout.write(encoded)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
