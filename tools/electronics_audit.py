#!/usr/bin/env python3
"""Fail-closed audit for the first electronic and special-material batch."""

from __future__ import annotations

import argparse
import csv
from pathlib import Path
import re
import sys
from typing import Sequence


EXPECTED = {
    622: ("GAAS", "Gallium Arsenide", "砷化镓", "SC_ELEC"),
    623: ("GANI", "Gallium Nitride", "氮化镓", "SC_ELEC"),
    624: ("FRIT", "Ferrite", "铁氧体", "SC_ELEC"),
    625: ("PMAG", "Permanent Magnet", "永磁材料", "SC_ELEC"),
    626: ("SMAG", "Soft Magnet", "软磁材料", "SC_ELEC"),
    627: ("PZCR", "Piezoelectric Ceramic", "压电陶瓷", "SC_ELEC"),
    628: ("TELC", "Thermoelectric Material", "热电材料", "SC_ELEC"),
    629: ("SUPC", "Superconductor", "超导材料", "SC_ELEC"),
    630: ("GRPH", "Graphene", "石墨烯", "SC_ELEC"),
    631: ("CNTB", "Carbon Nanotube", "碳纳米管", "SC_ELEC"),
    632: ("AERG", "Aerogel", "气凝胶", "SC_SPECIAL"),
    633: ("CFRP", "Carbon-Fiber Composite", "碳纤维复合材料", "SC_SOLIDS"),
    634: ("LCOB", "Lithium Cobalt Oxide", "钴酸锂", "SC_ELEC"),
    635: ("GRAN", "Graphite Anode", "石墨负极", "SC_ELEC"),
    636: ("SELE", "Solid Electrolyte", "固态电解质", "SC_ELEC"),
    637: ("ITOX", "Indium Tin Oxide", "氧化铟锡", "SC_ELEC"),
    638: ("PCMT", "Phase-Change Material", "相变存储材料", "SC_ELEC"),
    639: ("ECHR", "Electrochromic Material", "电致变色材料", "SC_ELEC"),
    640: ("PHRS", "Photoresist", "光刻胶", "SC_ELEC"),
    641: ("DIEL", "Dielectric Ceramic", "介电陶瓷", "SC_ELEC"),
}

REACTIONS = {
    "electronics.synthesis_superconductor",
    "electronics.synthesis_solid_electrolyte",
    "electronics.synthesis_permanent_magnet",
    "electronics.synthesis_piezoelectric_ceramic",
    "electronics.synthesis_lithium_cobalt_oxide",
    "electronics.synthesis_indium_tin_oxide",
    "electronics.synthesis_phase_change_material",
    "electronics.synthesis_electrochromic_material",
    "electronics.synthesis_gallium_arsenide",
    "electronics.synthesis_gallium_nitride",
    "electronics.synthesis_ferrite",
    "electronics.synthesis_soft_magnet",
    "electronics.synthesis_thermoelectric_material",
    "electronics.synthesis_carbon_nanotube",
    "electronics.synthesis_carbon_fibre_composite",
    "electronics.synthesis_photoresist",
    "electronics.synthesis_dielectric_ceramic",
    "electronics.synthesis_aerogel",
    "electronics.synthesis_graphite_anode",
    "electronics.synthesis_graphene",
    "electronics.silicon_p_doping",
    "electronics.silicon_n_doping",
    "electronics.gaas_photoconduction",
    "electronics.gani_electroluminescence",
    "electronics.ferrite_pulse_loss",
    "electronics.magnetic_deflection",
    "electronics.piezoelectric_pressure",
    "electronics.thermoelectric_gradient",
    "electronics.superconducting_conduction",
    "electronics.nanotube_pressure_collapse",
    "electronics.aerogel_pressure_collapse",
    "electronics.graphene_oxidation",
    "electronics.battery_charge_transfer",
    "electronics.battery_thermal_runaway",
    "electronics.phase_change_memory",
    "electronics.electrochromic_toggle",
    "electronics.photoresist_development",
    "electronics.dielectric_charge_discharge",
}


def read_text(path: Path, errors: list[str]) -> str:
    try:
        return path.read_text(encoding="utf-8")
    except (OSError, UnicodeDecodeError) as exc:
        errors.append(f"{path}: cannot read UTF-8 text: {exc}")
        return ""


def read_csv(path: Path, errors: list[str]) -> list[dict[str, str]]:
    try:
        with path.open(encoding="utf-8-sig", newline="") as stream:
            return list(csv.DictReader(stream))
    except (OSError, UnicodeDecodeError, csv.Error) as exc:
        errors.append(f"{path}: cannot read CSV: {exc}")
        return []


def audit(root: Path) -> list[str]:
    errors: list[str] = []
    registry_path = root / "docs" / "ELEMENT_REGISTRY.csv"
    rows = read_csv(registry_path, errors)
    by_id = {int(row["stable_id"]): row for row in rows if row.get("stable_id", "").isdigit()}
    for stable_id, (code, en_name, zh_name, menu) in EXPECTED.items():
        row = by_id.get(stable_id)
        expected = {
            "identifier": f"OMNI_PT_{code}", "display_code": code,
            "english_name": en_name, "chinese_name": zh_name,
            "menu_category": menu, "element_state": "solid", "module": "electronics",
            "implementation_status": "implemented", "default_enabled": "true",
            "is_duplicate": "false", "meson_name": code,
        }
        if not row:
            errors.append(f"{registry_path}: missing electronics stable ID {stable_id}")
            continue
        for field, value in expected.items():
            if row.get(field) != value:
                errors.append(f"{registry_path}: ID {stable_id} {field}={row.get(field)!r}; expected {value!r}")
    for stable_id in range(642, 670):
        row = by_id.get(stable_id)
        if not row or row.get("implementation_status") != "reserved":
            errors.append(f"{registry_path}: electronics reserve ID {stable_id} is not locked")

    expected_ids = {f"OMNI_PT_{code}" for code, *_ in EXPECTED.values()}
    for filename in ("ELEMENT_CONTENT.csv", "ELEMENT_USAGE_MATRIX.csv"):
        found = {row.get("identifier") for row in read_csv(root / "docs" / filename, errors)}
        missing = sorted(expected_ids - found)
        if missing:
            errors.append(f"{filename}: missing electronics identifiers {','.join(missing)}")
    reaction_ids = {row.get("reaction_id") for row in read_csv(root / "docs" / "REACTION_REGISTRY.csv", errors)}
    missing_reactions = sorted(REACTIONS - reaction_ids)
    if missing_reactions:
        errors.append("REACTION_REGISTRY.csv: missing electronics reactions " + ",".join(missing_reactions))

    engine_path = root / "src" / "simulation" / "OmniElectronics.cpp"
    engine = read_text(engine_path, errors)
    markers = (
        "ElectronicsEventsPerFrame = 1024", "OmniElectronicsModuleEnabled(sim)",
        "bool CatalyticSynthesis", "bool Photoconduction", "bool ThermoelectricGradient",
        "bool ColdSuperconduct", "bool PiezoelectricPressure", "bool MagneticDeflection",
        "bool CarbonNanomaterialBehaviour", "bool BatteryThermalRunaway",
        "bool PhaseChangeMemory", "bool ElectrochromicToggle", "bool PhotoresistExposure",
        "bool DielectricCharge", "bool TransferBatteryCharge", "parts[i].tmp >= 50",
    )
    for marker in markers:
        if marker not in engine:
            errors.append(f"{engine_path}: missing implementation marker {marker!r}")
    if re.search(r"\bNPART\b|parts\.active|for\s*\([^\n]*PT_NUM", engine):
        errors.append(f"{engine_path}: global particle or element scan is forbidden")

    content = read_text(root / "src" / "gui" / "game" / "OmniContent.h", errors)
    for marker in (
        "OmniElectronicsFirstId = 622", "OmniElectronicsLastId = 669",
        "OmniFutureContentFirstId = 670",
    ):
        if marker not in content:
            errors.append(f"OmniContent.h: missing electronics range marker {marker!r}")
    spark = read_text(root / "src" / "simulation" / "elements" / "SPRK.cpp", errors)
    if "OmniElectronicsSparkUpdate" not in spark:
        errors.append("SPRK.cpp: electronics spark integration is missing")

    for stable_id, (code, _en, _zh, menu) in EXPECTED.items():
        path = root / "src" / "simulation" / "elements" / f"{code}.cpp"
        text = read_text(path, errors)
        for marker in (
            f'Identifier = "OMNI_PT_{code}"', f'Name = "{code}"', f"MenuSection = {menu}",
            "MenuVisible = 1", "Enabled = 1", "Properties = TYPE_SOLID",
            f"OmniConfigureElectronicsElement(*this, PT_{code})",
        ):
            if marker not in text:
                errors.append(f"{path}: missing {marker!r}")

    runtime = read_text(root / "tools" / "runtime" / "electronics_regression.lua", errors)
    for marker in (
        'report:write("OMNI_ELECTRONICS_ELEMENTS=20',
        'report:write("OMNI_ELECTRONICS_IDS=622-641',
        'report:write("OMNI_ELECTRONICS_SYNTHESIS_PATHS=22',
        'report:write("OMNI_ELECTRONICS_EVENT_PEAK=",',
    ):
        if marker not in runtime:
            errors.append(f"electronics_regression.lua: missing evidence marker {marker!r}")
    return errors


def main(argv: Sequence[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--quiet", action="store_true")
    args = parser.parse_args(argv)
    errors = audit(args.source_root.resolve())
    if errors:
        for error in errors:
            print(f"electronics-audit: ERROR {error}", file=sys.stderr)
        print(f"electronics-audit: FAIL ({len(errors)} errors)", file=sys.stderr)
        return 1
    if not args.quiet:
        print("electronics-audit: PASS (20 elements, 38 reactions, 28 reserved slots)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
