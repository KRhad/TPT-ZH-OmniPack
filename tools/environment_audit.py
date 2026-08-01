#!/usr/bin/env python3
"""Fail-closed audit for ecology, pollution, and environmental materials."""

from __future__ import annotations

import argparse
import csv
from pathlib import Path
import re
import sys
from typing import Sequence


EXPECTED = {
    670: ("SOIL", "Soil", "土壤", "SC_POWDERS", "powder"),
    671: ("WWTR", "Wastewater", "污水", "SC_LIQUID", "liquid"),
    672: ("PEST", "Pesticide", "农药", "SC_LIQUID", "liquid"),
    673: ("HMET", "Heavy-Metal Contaminant", "重金属污染物", "SC_POWDERS", "powder"),
    674: ("RCON", "Radioactive Contaminant", "放射性污染物", "SC_NUCLEAR", "powder"),
    675: ("MPLS", "Microplastic", "微塑料", "SC_POWDERS", "powder"),
    676: ("OWST", "Organic Waste", "有机废物", "SC_POWDERS", "powder"),
    677: ("BLOM", "Algal Bloom", "藻华", "SC_LIQUID", "liquid"),
    678: ("MOLD", "Mold", "霉菌", "SC_POWDERS", "powder"),
    679: ("BLOD", "Blood", "血液", "SC_LIQUID", "liquid"),
    680: ("TOXN", "Toxin", "毒素", "SC_LIQUID", "liquid"),
    681: ("AMAT", "Antimicrobial Material", "抗菌材料", "SC_SOLIDS", "solid"),
    682: ("SLUD", "Sludge", "污泥", "SC_LIQUID", "liquid"),
    683: ("SMOG", "Smog", "烟霾", "SC_GAS", "gas"),
    684: ("ARAN", "Acid Rain", "酸雨", "SC_LIQUID", "liquid"),
    685: ("DETG", "Detergent", "洗涤剂", "SC_LIQUID", "liquid"),
}

REACTIONS = {
    "biology.soil_water_absorption",
    "biology.soil_vapour_release",
    "biology.soil_formation",
    "biology.soil_toxin_adsorption",
    "biology.wastewater_biofiltration",
    "biology.wastewater_eutrophication",
    "biology.pesticide_treatment",
    "biology.heavy_metal_bio_capture",
    "biology.heavy_metal_water_contamination",
    "biology.radioactive_contaminant_decay",
    "biology.radioactive_water_contamination",
    "biology.microplastic_capture",
    "biology.microplastic_pyrolysis",
    "biology.organic_waste_composting",
    "biology.organic_waste_pyrolysis",
    "biology.bloom_oxygen_depletion",
    "biology.bloom_lifetime_collapse",
    "biology.mold_growth",
    "biology.mold_lifetime",
    "biology.blood_infection",
    "biology.blood_oxygenation",
    "biology.blood_coagulation",
    "biology.toxin_damage",
    "biology.antimicrobial_surface",
    "biology.sludge_drying",
    "biology.sludge_composting",
    "biology.smog_acid_rain",
    "biology.acid_rain_neutralisation",
    "biology.acid_rain_soil_damage",
    "biology.acid_rain_corrosion",
    "biology.detergent_oil_wash",
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
    for stable_id, (code, en_name, zh_name, menu, state) in EXPECTED.items():
        row = by_id.get(stable_id)
        expected = {
            "identifier": f"OMNI_PT_{code}",
            "display_code": code,
            "english_name": en_name,
            "chinese_name": zh_name,
            "menu_category": menu,
            "element_state": state,
            "module": "biology",
            "implementation_status": "implemented",
            "default_enabled": "true",
            "is_duplicate": "false",
            "meson_name": code,
        }
        if not row:
            errors.append(f"{registry_path}: missing environment stable ID {stable_id}")
            continue
        for field, value in expected.items():
            if row.get(field) != value:
                errors.append(
                    f"{registry_path}: ID {stable_id} {field}={row.get(field)!r}; "
                    f"expected {value!r}"
                )

    expected_ids = {f"OMNI_PT_{code}" for code, *_ in EXPECTED.values()}
    for filename in ("ELEMENT_CONTENT.csv", "ELEMENT_USAGE_MATRIX.csv"):
        found = {row.get("identifier") for row in read_csv(root / "docs" / filename, errors)}
        missing = sorted(expected_ids - found)
        if missing:
            errors.append(f"{filename}: missing environment identifiers {','.join(missing)}")

    reaction_ids = {
        row.get("reaction_id")
        for row in read_csv(root / "docs" / "REACTION_REGISTRY.csv", errors)
    }
    missing_reactions = sorted(REACTIONS - reaction_ids)
    if missing_reactions:
        errors.append(
            "REACTION_REGISTRY.csv: missing environment reactions "
            + ",".join(missing_reactions)
        )

    engine_path = root / "src" / "simulation" / "OmniEnvironment.cpp"
    engine = read_text(engine_path, errors)
    markers = (
        "OmniConsumeBiologyEvent(sim)",
        "OmniBiologyModuleEnabled(sim)",
        "bool UpdateSoil",
        "bool UpdateWastewater",
        "bool UpdateRadioactiveContaminant",
        "bool UpdateBloom",
        "bool UpdateMould",
        "bool UpdateBlood",
        "bool UpdateAntimicrobialMaterial",
        "bool UpdateSmog",
        "bool UpdateAcidRain",
        "bool UpdateDetergent",
        "return OmniGasGraphics(GRAPHICS_FUNC_SUBCALL_ARGS)",
    )
    for marker in markers:
        if marker not in engine:
            errors.append(f"{engine_path}: missing implementation marker {marker!r}")
    if re.search(r"\bNPART\b|parts\.active|for\s*\([^\n]*PT_NUM", engine):
        errors.append(f"{engine_path}: global particle or element scan is forbidden")

    biology = read_text(root / "src" / "simulation" / "OmniBiology.cpp", errors)
    if "BiologyEventsPerFrame = 1024" not in biology:
        errors.append("OmniBiology.cpp: shared ecology event budget is missing")
    if biology.count("sim->RecordOmniEvent()") != 1:
        errors.append("OmniBiology.cpp: shared ecology event recorder must have one owner")

    content = read_text(root / "src" / "gui" / "game" / "OmniContent.h", errors)
    for marker in (
        "OmniEnvironmentFirstId = 670",
        "OmniEnvironmentLastId = 685",
        "OmniFutureContentFirstId = 686",
    ):
        if marker not in content:
            errors.append(f"OmniContent.h: missing environment range marker {marker!r}")
    routing = read_text(root / "src" / "gui" / "game" / "OmniContent.cpp", errors)
    if "elementId <= OmniEnvironmentLastId" not in routing:
        errors.append("OmniContent.cpp: environment range is not routed through Biology")

    for stable_id, (code, _en, _zh, menu, _state) in EXPECTED.items():
        path = root / "src" / "simulation" / "elements" / f"{code}.cpp"
        source = read_text(path, errors)
        for marker in (
            f'Identifier = "OMNI_PT_{code}"',
            f'Name = "{code}"',
            f"MenuSection = {menu}",
            "MenuVisible = 1",
            "Enabled = 1",
            "Properties = ",
            f"OmniConfigureEnvironmentElement(*this, PT_{code})",
        ):
            if marker not in source:
                errors.append(f"{path}: missing {marker!r}")
        if len(code) != 4 or code != code.upper():
            errors.append(f"{path}: display code is not four uppercase characters")

    runtime = read_text(root / "tools" / "runtime" / "environment_regression.lua", errors)
    for marker in (
        'report:write("OMNI_ENVIRONMENT_ELEMENTS=16',
        'report:write("OMNI_ENVIRONMENT_IDS=670-685',
        'report:write("OMNI_ENVIRONMENT_BEHAVIOURS=17',
        'report:write("OMNI_ENVIRONMENT_EVENT_PEAK=",',
    ):
        if marker not in runtime:
            errors.append(f"environment_regression.lua: missing evidence marker {marker!r}")
    return errors


def main(argv: Sequence[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--quiet", action="store_true")
    args = parser.parse_args(argv)
    errors = audit(args.source_root.resolve())
    if errors:
        for error in errors:
            print(f"environment-audit: ERROR {error}", file=sys.stderr)
        print(f"environment-audit: FAIL ({len(errors)} errors)", file=sys.stderr)
        return 1
    if not args.quiet:
        print("environment-audit: PASS (16 elements, 31 reactions, shared 1024/frame budget)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
