#!/usr/bin/env python3
"""Fail-closed static/data contract for OmniReactionRuntime v1."""

from __future__ import annotations

import argparse
import json
from pathlib import Path


SPECIES = ["species.carbon", "species.oxygen", "species.carbon-dioxide"]


def check(root: Path) -> list[str]:
    errors: list[str] = []
    try:
        data = json.loads((root / "resources/omnicore/v1/omni-reaction-runtime-v1.json").read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        return [f"runtime data: {exc}"]

    if data.get("runtime_version") != "1.0.8":
        errors.append("runtime version must be 1.0.8")
    if data.get("runtime_contract") != "compiled_transaction_plan_v1":
        errors.append("runtime must remain an explicit transaction planner")
    if data.get("authoritative_species_order") != SPECIES:
        errors.append("species order drifted")
    species = data.get("species")
    if not isinstance(species, list) or [entry.get("id") for entry in species if isinstance(entry, dict)] != SPECIES:
        errors.append("species records drifted")
    reaction = data.get("reactions", [{}])[0] if isinstance(data.get("reactions"), list) and data.get("reactions") else {}
    if not isinstance(reaction, dict) or reaction.get("equation") != "C(s) + O2(g) -> CO2(g)":
        errors.append("first controlled reaction drifted")
    if reaction.get("enthalpy_change_j_mol") != -393500.0:
        errors.append("carbon oxidation reference enthalpy drifted")
    thermochemistry = reaction.get("thermochemistry", {}) if isinstance(reaction, dict) else {}
    if not isinstance(thermochemistry, dict) or not all(thermochemistry.get(key) for key in (
        "source_title", "source_locator", "source_value", "method", "redistribution_status", "license_or_terms"
    )):
        errors.append("thermochemistry provenance is incomplete")
    kinetics = reaction.get("kinetics", {}) if isinstance(reaction, dict) else {}
    if not isinstance(kinetics, dict) or kinetics.get("method") != "game_tuned" or not kinetics.get(
        "not_a_measured_graphite_kinetic_parameter_set"
    ):
        errors.append("kinetic tuning status must stay explicit")
    boundary = data.get("integration_boundary", {})
    expected_commit = [
        "subtract_condensed_carbon_mass",
        "subtract_atmosphere_o2_mass",
        "add_atmosphere_co2_mass",
        "add_thermal_energy",
        "record_atom_mass_energy_ledger",
    ]
    if not isinstance(boundary, dict) or boundary.get("required_commit") != expected_commit or boundary.get(
        "commit_semantics"
    ) != "single_transaction_or_rollback":
        errors.append("integration ownership/transaction contract drifted")

    header = (root / "src/simulation/OmniReactionRuntime.h").read_text(encoding="utf-8")
    runtime = (root / "src/simulation/OmniReactionRuntime.cpp").read_text(encoding="utf-8")
    meson = (root / "meson.build").read_text(encoding="utf-8")
    for token in ("PlanCarbonOxidation", "OmniCarbonOxidationPlan", "massResidualKg", "energyResidualJ"):
        if token not in header:
            errors.append(f"runtime header missing {token}")
    for token in ("std::expm1", "ValidateReaction", "CarbonOxidationEnthalpyJPerMol", "isCarbonOxidation", "massResidualKg"):
        if token not in runtime:
            errors.append(f"runtime source missing {token}")
    for forbidden in ("Simulation.h", "Particle.h", "OmniAtmosphere.h", "OmniChemistry.h"):
        if forbidden in runtime or forbidden in header:
            errors.append(f"runtime must not own {forbidden}")
    if "'omni-reaction-reference'" not in meson or "cpp_args: strict_numerical_cpp_args" not in meson:
        errors.append("reaction reference target must use strict numerical args")
    if "'omni-reaction-runtime'" not in meson:
        errors.append("reaction probe must be registered")
    return errors


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--source-root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--quiet", action="store_true")
    args = parser.parse_args()
    errors = check(args.source_root.resolve())
    if errors:
        for error in errors:
            print(f"omni-reaction-runtime-contract: ERROR {error}")
        return 1
    if not args.quiet:
        print("OmniReactionRuntime v1 static/data contract passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
