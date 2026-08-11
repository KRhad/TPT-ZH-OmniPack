#!/usr/bin/env python3
"""Fail-closed data/code contract for the 1.0.7 atmosphere and thermal runtime."""

from __future__ import annotations

import argparse
import json
import math
from pathlib import Path
import re


CHANNELS = ["N2", "O2", "Ar", "CO2", "H2O"]
NUMBER = r"[-+]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][-+]?\d+)?"


def close(left: float, right: float) -> bool:
    return math.isclose(left, right, rel_tol=1.0e-12, abs_tol=1.0e-15)


def load_json(path: Path, errors: list[str]) -> dict[str, object]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        errors.append(f"{path.name}: {exc}")
        return {}
    if not isinstance(value, dict):
        errors.append(f"{path.name}: root must be an object")
        return {}
    return value


def require_provenance(document: dict[str, object], name: str, errors: list[str]) -> None:
    records = document.get("provenance")
    if records is None:
        records = document.get("saturation_models")
    if not isinstance(records, list) or not records:
        errors.append(f"{name}: provenance/source records are required")
        return
    for index, record in enumerate(records):
        if not isinstance(record, dict):
            errors.append(f"{name}: provenance[{index}] must be an object")
            continue
        for field in ("source", "redistribution"):
            if not isinstance(record.get(field), str) or not record[field].strip():
                errors.append(f"{name}: provenance[{index}].{field} is required")


def check(root: Path) -> list[str]:
    errors: list[str] = []
    atmosphere_path = root / "resources/omnicore/v1/omni-atmosphere-species-runtime-v1.json"
    thermal_path = root / "resources/omnicore/v1/omni-thermal-water-runtime-v1.json"
    atmosphere = load_json(atmosphere_path, errors)
    thermal = load_json(thermal_path, errors)

    if atmosphere.get("runtime_version") != "1.0.7":
        errors.append("atmosphere runtime_version must be 1.0.7")
    if atmosphere.get("authoritative_channels") != CHANNELS:
        errors.append("atmosphere authoritative channel order drifted")
    species = atmosphere.get("species")
    if not isinstance(species, list) or [entry.get("id") for entry in species if isinstance(entry, dict)] != CHANNELS:
        errors.append("atmosphere species registry/order drifted")
        species = []
    mass_fractions = atmosphere.get("reference_mass_fractions")
    if not isinstance(mass_fractions, dict) or list(mass_fractions) != CHANNELS:
        errors.append("atmosphere reference mass-fraction registry/order drifted")
        mass_fractions = {}
    elif not close(sum(float(mass_fractions[channel]) for channel in CHANNELS), 1.0):
        errors.append("atmosphere reference mass fractions do not sum to one")
    require_provenance(atmosphere, "atmosphere", errors)

    if thermal.get("runtime_version") != "1.0.7":
        errors.append("thermal runtime_version must be 1.0.7")
    constants = thermal.get("constants")
    if not isinstance(constants, dict):
        errors.append("thermal constants object is required")
        constants = {}
    require_provenance(thermal, "thermal", errors)

    atmosphere_cpp = (root / "src/simulation/OmniAtmosphere.cpp").read_text(encoding="utf-8")
    thermal_h = (root / "src/simulation/OmniThermal.h").read_text(encoding="utf-8")
    thermal_cpp = (root / "src/simulation/OmniThermal.cpp").read_text(encoding="utf-8")
    scale_h = (root / "src/simulation/OmniPhysicalScale.h").read_text(encoding="utf-8")
    save_h = (root / "src/client/GameSave.h").read_text(encoding="utf-8")
    save_cpp = (root / "src/client/GameSave.cpp").read_text(encoding="utf-8")
    simulation_h = (root / "src/simulation/Simulation.h").read_text(encoding="utf-8")

    gas_match = re.search(rf"UniversalGasConstant\s*=\s*({NUMBER})", atmosphere_cpp)
    if not gas_match or not close(float(gas_match.group(1)), float(atmosphere.get("universal_gas_constant_j_mol_k", math.nan))):
        errors.append("universal gas constant differs between JSON and C++")

    species_pattern = re.compile(
        rf'\{{\s*"([^"]+)"\s*,\s*({NUMBER})\s*,\s*({NUMBER})\s*,\s*({NUMBER})\s*,\s*({NUMBER})\s*,\s*OmniAtmosphereSpeciesPhase::Gas\s*\}}'
    )
    cpp_species = species_pattern.findall(atmosphere_cpp)
    if len(cpp_species) != len(CHANNELS):
        errors.append("C++ common species initializer could not be parsed")
    elif species:
        fields = ("molar_mass_kg_mol", "cp_j_kg_k", "thermal_conductivity_w_m_k", "diffusion_m2_s")
        for cpp, record in zip(cpp_species, species):
            if cpp[0] != record.get("id"):
                errors.append(f"species id mismatch: C++ {cpp[0]} JSON {record.get('id')}")
            for field, cpp_value in zip(fields, cpp[1:]):
                if not close(float(cpp_value), float(record.get(field, math.nan))):
                    errors.append(f"species {cpp[0]} {field} differs between JSON and C++")

    fractions_match = re.search(
        r"OmniEarthLikeAtmosphereMassFractions\(\).*?return\s*\{([^}]+)\}",
        atmosphere_cpp,
        re.DOTALL,
    )
    if not fractions_match:
        errors.append("C++ reference mass fractions could not be parsed")
    elif mass_fractions:
        cpp_fractions = [float(value) for value in re.findall(NUMBER, fractions_match.group(1))]
        json_fractions = [float(mass_fractions[channel]) for channel in CHANNELS]
        if len(cpp_fractions) != len(json_fractions) or any(
            not close(left, right) for left, right in zip(cpp_fractions, json_fractions)
        ):
            errors.append("reference mass fractions differ between JSON and C++")

    thermal_names = {
        "TriplePointTemperatureK": "triple_point_temperature_k",
        "IceSpecificHeatJKgK": "ice_cp_j_kg_k",
        "LiquidSpecificHeatJKgK": "liquid_cp_j_kg_k",
        "VaporSpecificHeatJKgK": "vapor_cp_j_kg_k",
        "LatentHeatFusionJPerKg": "latent_heat_fusion_j_kg",
        "LatentHeatVaporizationJPerKg": "latent_heat_vaporization_j_kg",
    }
    for cpp_name, json_name in thermal_names.items():
        match = re.search(rf"{cpp_name}\s*=\s*({NUMBER})", thermal_h)
        if not match or not close(float(match.group(1)), float(constants.get(json_name, math.nan))):
            errors.append(f"thermal constant {json_name} differs between JSON and C++")
    lower = re.search(rf"FusionLowerK\s*=\s*({NUMBER})", thermal_cpp)
    upper = re.search(rf"FusionUpperK\s*=\s*({NUMBER})", thermal_cpp)
    interval = constants.get("fusion_mushy_interval_k")
    if not lower or not upper or not isinstance(interval, list) or len(interval) != 2 or not close(
        float(lower.group(1)), float(interval[0])
    ) or not close(float(upper.group(1)), float(interval[1])):
        errors.append("thermal mushy interval differs between JSON and C++")

    ownership = thermal.get("particle_water_ownership")
    if not isinstance(ownership, dict):
        errors.append("thermal particle_water_ownership object is required")
    else:
        if ownership.get("managed_element_ids") != ["WATR", "ICEI", "WTRV"]:
            errors.append("managed water element family drifted")
        if ownership.get("particle_abi_changed") is not False:
            errors.append("water ownership must preserve the Particle ABI")
        if ownership.get("sidecar_bytes_per_particle") != 8:
            errors.append("water sidecar must remain 8 bytes per particle")
        if not close(float(ownership.get("particle_parcel_volume_m3", math.nan)), 4.0e-9):
            errors.append("water particle parcel volume drifted")
        if not close(float(ownership.get("default_water_parcel_mass_kg", math.nan)), 4.0e-6):
            errors.append("default water parcel mass drifted")
    required_contract_symbols = {
        "OmniPhysicalScale.h": (scale_h, ("ParticleParcelVolumeM3", "DefaultWaterParcelMassKg")),
        "GameSave.h": (save_h, ("OmniWaterParcelStateVersion = 1", "omniWaterParcelMassKg")),
        "GameSave.cpp": (save_cpp, ("omniWaterParcels", "particle_order_f64_le_mass_kg_v1")),
        "Simulation.h": (simulation_h, ("std::array<double, NPART> omniWaterParcelMassKg",)),
    }
    for filename, (body, symbols) in required_contract_symbols.items():
        for symbol in symbols:
            if symbol not in body:
                errors.append(f"{filename}: missing water ownership contract symbol {symbol}")

    return errors


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--source-root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--quiet", action="store_true")
    args = parser.parse_args()
    errors = check(args.source_root.resolve())
    if errors:
        for error in errors:
            print(f"omni-multispecies-thermal-contract: ERROR {error}")
        return 1
    if not args.quiet:
        print("OmniCore 1.0.7 multispecies/thermal data contract passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
