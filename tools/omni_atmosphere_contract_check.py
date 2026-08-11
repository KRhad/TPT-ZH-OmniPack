#!/usr/bin/env python3
"""Fail-closed static contract for the 1.0.6 CPU Reference atmosphere MVP."""

from __future__ import annotations

import argparse
import json
from pathlib import Path


REQUIRED_MARKERS = {
    "src/simulation/OmniPhysicalScale.h": (
        "CellLengthM = 4.0e-3",
        "EffectiveDepthM = 4.0e-3",
        "TimestepS = 1.0 / 60.0",
        "ReferenceDensityKgM3 = 1.225",
    ),
    "src/simulation/OmniAtmosphere.h": (
        "density",
        "momentumX",
        "momentumY",
        "totalEnergy",
        "numericalMassCorrectionKg",
        "StepReference",
    ),
    "src/simulation/OmniAtmosphere.cpp": (
        "RusanovFlux",
        "ApplyFloors",
        "boundaryMassOutKg",
        "ReferenceCompressible",
    ),
    "src/simulation/Simulation.h": (
        "omniAtmosphere",
        "omniSimulationMode = OMNI_CLASSIC",
    ),
    "src/simulation/SimulationSettings.h": (
        "OMNI_CLASSIC",
        "OMNI_ENHANCED",
        "OMNI_SCIENTIFIC",
    ),
    "src/client/GameSave.h": (
        "omniSimulationMode = OMNI_CLASSIC",
    ),
    "tools/runtime/omni_atmosphere_cpu_mvp.lua": (
        "sim.omniSimulationMode(sim.OMNI_ENHANCED)",
        "sim.omniAtmosphere()",
        "OMNI_ATMOSPHERE_STATUS=PASS",
    ),
}


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--source-root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--quiet", action="store_true")
    args = parser.parse_args()
    root = args.source_root.resolve()
    errors: list[str] = []

    contract_path = root / "resources/omnicore/v1/omni-atmosphere-runtime-v1.json"
    try:
        contract = json.loads(contract_path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        errors.append(f"runtime contract: {exc}")
        contract = {}
    expected_contract = {
        "selection_status": "selected_for_1.0.6_cpu_mvp",
        "runtime_state_version": 1,
    }
    for key, expected in expected_contract.items():
        if contract.get(key) != expected:
            errors.append(f"runtime contract: {key} expected {expected!r}")
    scale = contract.get("physical_scale", {})
    for key, expected in {
        "pixel_length_m": "0.001",
        "cell_length_m": "0.004",
        "effective_depth_m": "0.004",
        "timestep_s": "0.0166666666666666667",
    }.items():
        if scale.get(key) != expected:
            errors.append(f"runtime contract physical_scale.{key}: expected {expected}")
    if contract.get("authoritative_state") != ["rho", "rho_u", "rho_v", "rho_E"]:
        errors.append("runtime contract: authoritative state drifted")
    compatibility = contract.get("compatibility", {})
    if compatibility.get("classic_air_replaced") is not False or compatibility.get("save_atmosphere_state") is not False:
        errors.append("runtime contract: Classic or save-state compatibility boundary drifted")
    for relative, markers in REQUIRED_MARKERS.items():
        path = root / relative
        try:
            text = path.read_text(encoding="utf-8")
        except OSError as exc:
            errors.append(f"{relative}: {exc}")
            continue
        for marker in markers:
            if marker not in text:
                errors.append(f"{relative}: missing marker {marker!r}")
    if errors:
        for error in errors:
            print(f"omni-atmosphere-contract: ERROR {error}")
        return 1
    if not args.quiet:
        print("OmniAtmosphere 1.0.6 CPU Reference contract passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
