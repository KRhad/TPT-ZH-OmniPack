#!/usr/bin/env python3
"""Fail-closed audit for organic chemistry batch 1 and POLY deduplication."""

from __future__ import annotations

import argparse
import csv
from pathlib import Path
import re
import sys
from typing import Sequence


EXPECTED = {
    589: ("CH4M", "CH4", "Methane", "甲烷", "SC_GAS", "gas"),
    590: ("ETHA", "ETHA", "Ethane", "乙烷", "SC_GAS", "gas"),
    591: ("PROP", "PROP", "Propane", "丙烷", "SC_GAS", "gas"),
    592: ("BUTA", "BUTA", "Butane", "丁烷", "SC_GAS", "gas"),
    593: ("ETHE", "C2H4", "Ethylene", "乙烯", "SC_GAS", "gas"),
    594: ("METH", "MEOH", "Methanol", "甲醇", "SC_LIQUID", "liquid"),
    595: ("ACET", "ACET", "Acetone", "丙酮", "SC_LIQUID", "liquid"),
    596: ("BENZ", "BENZ", "Benzene", "苯", "SC_LIQUID", "liquid"),
    597: ("TOLU", "TOLU", "Toluene", "甲苯", "SC_LIQUID", "liquid"),
    598: ("GLYC", "GLYC", "Glycerol", "甘油", "SC_LIQUID", "liquid"),
    599: ("ACTA", "ACTA", "Acetic Acid", "乙酸", "SC_LIQUID", "liquid"),
    600: ("UREA", "UREA", "Urea", "尿素", "SC_POWDERS", "powder"),
    601: ("FATS", "FATS", "Fats", "油脂", "SC_LIQUID", "liquid"),
}

REACTIONS = {
    "chemistry.ethylene_polymerisation",
    "chemistry.organic_methane_synthesis",
    "chemistry.organic_methane_steam_reforming",
    "chemistry.organic_ethane_cracking",
    "chemistry.organic_propane_cracking",
    "chemistry.organic_butane_cracking",
    "chemistry.organic_methanol_oxidation",
    "chemistry.organic_ethanol_oxidation",
    "chemistry.organic_acetic_ketonisation",
    "chemistry.organic_acetylene_cyclisation",
    "chemistry.organic_benzene_methylation",
    "chemistry.organic_glycerol_nitration_proxy",
    "chemistry.organic_urea_synthesis",
    "chemistry.organic_urea_hydrolysis",
    "chemistry.organic_fat_saponification",
}


def read_text(path: Path, errors: list[str]) -> str:
    try:
        return path.read_text(encoding="utf-8")
    except (OSError, UnicodeDecodeError) as exc:
        errors.append(f"{path}: cannot read UTF-8 text: {exc}")
        return ""


def read_csv(path: Path, errors: list[str]) -> list[dict[str, str]]:
    try:
        with path.open(encoding="utf-8", newline="") as stream:
            return list(csv.DictReader(stream))
    except (OSError, UnicodeDecodeError, csv.Error) as exc:
        errors.append(f"{path}: cannot read CSV: {exc}")
        return []


def check_registry(root: Path, errors: list[str]) -> None:
    path = root / "docs" / "ELEMENT_REGISTRY.csv"
    rows = read_csv(path, errors)
    by_id = {
        int(row["stable_id"]): row
        for row in rows
        if row.get("stable_id", "").isdigit()
    }
    for stable_id, (meson, code, en_name, zh_name, menu, state) in EXPECTED.items():
        row = by_id.get(stable_id)
        if not row:
            errors.append(f"{path}: missing organic stable ID {stable_id}")
            continue
        expected = {
            "identifier": f"OMNI_PT_{meson}",
            "display_code": code,
            "english_name": en_name,
            "chinese_name": zh_name,
            "menu_category": menu,
            "element_state": state,
            "module": "chemistry",
            "implementation_status": "implemented",
            "default_enabled": "true",
            "is_duplicate": "false",
            "meson_name": meson,
        }
        for field, value in expected.items():
            if row.get(field) != value:
                errors.append(
                    f"{path}: ID {stable_id} {field}={row.get(field)!r}; "
                    f"expected {value!r}"
                )

    poly = next((row for row in rows if row.get("identifier") == "OMNI_PT_POLY"), None)
    if not poly:
        errors.append(f"{path}: stable POLY compatibility target is missing")
    elif (
        poly.get("stable_id") != "367"
        or poly.get("english_name") != "Polyethylene"
        or poly.get("chinese_name") != "聚乙烯"
        or poly.get("is_duplicate") != "false"
    ):
        errors.append(f"{path}: POLY was duplicated or not refined in place")

    compounds = read_csv(root / "docs" / "COMPOUND_REGISTRY.csv", errors)
    compound_ids = {row.get("stable_id") for row in compounds}
    missing_compounds = {"367", *(str(stable_id) for stable_id in EXPECTED)} - compound_ids
    if missing_compounds:
        errors.append(
            "COMPOUND_REGISTRY.csv: missing organic stable IDs "
            + ",".join(sorted(missing_compounds, key=int))
        )

    reactions = read_csv(root / "docs" / "REACTION_REGISTRY.csv", errors)
    reaction_ids = {row.get("reaction_id") for row in reactions}
    missing_reactions = sorted(REACTIONS - reaction_ids)
    if missing_reactions:
        errors.append(
            "REACTION_REGISTRY.csv: missing organic reactions "
            + ",".join(missing_reactions)
        )


def check_engine(root: Path, errors: list[str]) -> None:
    path = root / "src" / "simulation" / "OmniOrganics.cpp"
    text = read_text(path, errors)
    markers = {
        "shared chemistry module": "OmniChemistryModuleEnabled(sim)",
        "shared chemistry budget": "OmniConsumeChemistryEvent(sim)",
        "bounded local scan": "for (int ry = -1; ry <= 1; ++ry)",
        "methane synthesis": "bool MethaneSynthesis",
        "steam reforming": "bool MethaneSteamReforming",
        "alkane cracking": "bool CrackHydrocarbon",
        "ethylene polymerisation": "bool EthylenePolymerisation",
        "alcohol oxidation": "bool AlcoholOxidation",
        "acetic ketonisation": "bool AceticAcidKetonisation",
        "aromatic cyclisation": "bool AcetyleneCyclisation",
        "toluene synthesis": "bool BenzeneMethylation",
        "glycerol proxy": "bool GlycerolNitrationProxy",
        "urea synthesis": "bool UreaSynthesis",
        "urea hydrolysis": "bool UreaHydrolysis",
        "fat saponification": "bool FatSaponification",
    }
    for label, marker in markers.items():
        if marker not in text:
            errors.append(f"{path}: missing {label}: {marker!r}")
    if re.search(r"\bNPART\b|parts\.active|for\s*\([^\n]*PT_NUM", text):
        errors.append(f"{path}: global particle or element scan is forbidden")

    chemistry = read_text(root / "src" / "simulation" / "OmniChemistry.cpp", errors)
    if "OmniOrganicElementUpdate(UPDATE_FUNC_SUBCALL_ARGS)" not in chemistry:
        errors.append("OmniChemistry.cpp: organic network is not integrated")
    if "CatalyticPolymerisation" in chemistry:
        errors.append("OmniChemistry.cpp: obsolete acetylene-to-POLY path remains")

    content = read_text(root / "src" / "gui" / "game" / "OmniContent.h", errors)
    for marker in ("OmniOrganicFirstId = 589", "OmniOrganicLastId = 601"):
        if marker not in content:
            errors.append(f"OmniContent.h: missing organic range marker {marker!r}")

    for stable_id, (meson, _code, _en, _zh, menu, _state) in EXPECTED.items():
        element_path = root / "src" / "simulation" / "elements" / f"{meson}.cpp"
        element = read_text(element_path, errors)
        for marker in (
            f'Identifier = "OMNI_PT_{meson}"',
            f"MenuSection = {menu}",
            "MenuVisible = 1",
            "Enabled = 1",
            "Description = Localization::Ref().Tr",
            f"OmniConfigureOrganicElement(*this, PT_{meson})",
        ):
            if marker not in element:
                errors.append(f"{element_path}: missing {marker!r}")

    runtime = read_text(root / "tools" / "runtime" / "organic_regression.lua", errors)
    for marker in (
        'report:write("OMNI_ORGANIC_ELEMENTS=13',
        'report:write("OMNI_ORGANIC_REACTION_PATHS=15',
        'report:write("OMNI_ORGANIC_EVENT_PEAK=",',
    ):
        if marker not in runtime:
            errors.append(f"organic_regression.lua: missing evidence marker {marker!r}")


def audit(root: Path) -> list[str]:
    errors: list[str] = []
    check_registry(root, errors)
    check_engine(root, errors)
    return errors


def main(argv: Sequence[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--source-root", type=Path, default=Path(__file__).resolve().parents[1]
    )
    parser.add_argument("--quiet", action="store_true")
    args = parser.parse_args(argv)
    errors = audit(args.source_root.resolve())
    if errors:
        for error in errors:
            print(f"organic-audit: ERROR {error}", file=sys.stderr)
        print(f"organic-audit: FAIL ({len(errors)} errors)", file=sys.stderr)
        return 1
    if not args.quiet:
        print("organic-audit: PASS (13 new elements, POLY reused, 15 reactions)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
