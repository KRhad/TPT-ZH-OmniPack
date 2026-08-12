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
        catalog = json.loads((root / "resources/omnicore/v1/catalog.json").read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        return [f"runtime data: {exc}"]

    if data.get("runtime_version") != "1.0.8":
        errors.append("runtime version must be 1.0.8")
    if data.get("runtime_contract") != "compiled_transaction_plan_v1":
        errors.append("runtime must remain an explicit transaction planner")
    if data.get("runtime_consumption") != "compiled_and_simulation_integrated":
        errors.append("runtime consumption must record the integrated 1.0.8 transaction")
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
    if "chemistry-2e" in str(thermochemistry.get("source_locator", "")) or thermochemistry.get(
        "redistribution_status"
    ) != "redistributable_with_attribution":
        errors.append("runtime thermochemistry must use the audited CC BY OpenStax edition")
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

    if catalog.get("dataset_version") != "1.0.8-chemistry.1" or catalog.get(
        "runtime_consumption"
    ) is not True:
        errors.append("audited runtime catalog is not active for 1.0.8")
    catalog_reactions = catalog.get("reactions", [])
    if not isinstance(catalog_reactions, list) or not catalog_reactions or catalog_reactions[0].get(
        "id"
    ) != "reaction.carbon-oxidation":
        errors.append("audited catalog/runtime reaction identity drifted")

    header = (root / "src/simulation/OmniReactionRuntime.h").read_text(encoding="utf-8")
    runtime = (root / "src/simulation/OmniReactionRuntime.cpp").read_text(encoding="utf-8")
    atmosphere_header = (root / "src/simulation/OmniAtmosphere.h").read_text(encoding="utf-8")
    atmosphere_source = (root / "src/simulation/OmniAtmosphere.cpp").read_text(encoding="utf-8")
    simulation_header = (root / "src/simulation/Simulation.h").read_text(encoding="utf-8")
    simulation_source = (root / "src/simulation/Simulation.cpp").read_text(encoding="utf-8")
    coal_source = (root / "src/simulation/elements/COAL.cpp").read_text(encoding="utf-8")
    lua_source = (root / "src/lua/LuaSimulation.cpp").read_text(encoding="utf-8")
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
    for token in ("OmniAtmosphereReactionTransfer", "ApplyReactionSpeciesTransfer"):
        if token not in atmosphere_header or token not in atmosphere_source:
            errors.append(f"atomic atmosphere transaction is missing {token}")
    if "OmniChemistryMetrics" not in simulation_header:
        errors.append("Simulation combustion integration is missing OmniChemistryMetrics")
    for token in ("UpdateOmniCarbonCombustion", "TotalOmniParticleCarbonMassKg"):
        if token not in simulation_header or token not in simulation_source:
            errors.append(f"Simulation combustion integration is missing {token}")
    if "IsOmniAtmosphereActive" not in coal_source or "UpdateOmniCarbonCombustion" not in coal_source:
        errors.append("COAL does not isolate Enhanced combustion from Classic")
    if coal_source.find("OmniMetallurgyCoalUpdate") > coal_source.find("UpdateOmniCarbonCombustion"):
        errors.append("Enhanced combustion bypasses the existing metallurgy COAL update")
    if "omniChemistry" not in lua_source or "instrumented_omni_reaction_runtime" not in lua_source:
        errors.append("Lua chemistry diagnostics/profiler integration is missing")
    notice = root / "resources/third_party/OPENSTAX_CHEMISTRY_CC-BY-4.0.txt"
    if not notice.is_file() or "Creative Commons Attribution 4.0" not in notice.read_text(encoding="utf-8"):
        errors.append("OpenStax CC BY attribution notice is missing")
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
