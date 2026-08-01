#!/usr/bin/env python3
"""Fail-closed audit for organic biomolecule and polymer batch 2."""

from __future__ import annotations

import argparse
import csv
from pathlib import Path
import re
import sys
from typing import Sequence


EXPECTED = {
    602: ("GLUC", "GLUC", "Glucose", "葡萄糖", "SC_POWDERS", "powder"),
    603: ("STRC", "STRC", "Starch", "淀粉", "SC_POWDERS", "powder"),
    604: ("CELU", "CELU", "Cellulose", "纤维素", "SC_SOLIDS", "solid"),
    605: ("PRPE", "PRPE", "Propylene", "丙烯", "SC_GAS", "gas"),
    606: ("BDIE", "BDIE", "Butadiene", "丁二烯", "SC_GAS", "gas"),
    607: ("VCHL", "VCHL", "Vinyl Chloride", "氯乙烯", "SC_GAS", "gas"),
    608: ("STYR", "STYR", "Styrene", "苯乙烯", "SC_LIQUID", "liquid"),
    609: ("TFET", "TFET", "Tetrafluoroethylene", "四氟乙烯", "SC_GAS", "gas"),
    610: ("ADIP", "ADIP", "Adipic Acid", "己二酸", "SC_POWDERS", "powder"),
    611: ("DIAM", "DIAM", "Diamine", "己二胺", "SC_POWDERS", "powder"),
    612: ("ERES", "ERES", "Epoxy Resin", "环氧树脂", "SC_LIQUID", "liquid"),
    613: ("PPLY", "PPLY", "Polypropylene", "聚丙烯", "SC_SOLIDS", "solid"),
    614: ("PVCL", "PVCL", "Polyvinyl Chloride", "聚氯乙烯", "SC_SOLIDS", "solid"),
    615: ("PSTY", "PSTY", "Polystyrene", "聚苯乙烯", "SC_SOLIDS", "solid"),
    616: ("NYLN", "NYLN", "Nylon", "尼龙", "SC_SOLIDS", "solid"),
    617: ("RUBR", "RUBR", "Rubber", "橡胶", "SC_SOLIDS", "solid"),
    618: ("EPXY", "EPXY", "Cured Epoxy", "固化环氧", "SC_SOLIDS", "solid"),
    619: ("PTFE", "PTFE", "Polytetrafluoroethylene", "聚四氟乙烯", "SC_SOLIDS", "solid"),
    620: ("BITM", "BITM", "Bitumen", "沥青", "SC_SOLIDS", "solid"),
    621: ("EACT", "EACT", "Ethyl Acetate", "乙酸乙酯", "SC_LIQUID", "liquid"),
}

REACTIONS = {
    "chemistry.organic_starch_hydrolysis",
    "chemistry.organic_cellulose_hydrolysis",
    "chemistry.organic_glucose_fermentation",
    "chemistry.organic_propylene_polymerisation",
    "chemistry.organic_butadiene_polymerisation",
    "chemistry.organic_vinyl_chloride_polymerisation",
    "chemistry.organic_styrene_polymerisation",
    "chemistry.organic_tfe_polymerisation",
    "chemistry.organic_nylon_condensation",
    "chemistry.organic_epoxy_curing",
    "chemistry.organic_esterification",
    "chemistry.organic_bitumen_residue",
    "chemistry.organic_pvc_decomposition",
}

CANONICAL_MERGES = {
    "DEFAULT_PT_OIL",
    "DEFAULT_PT_WAX",
    "DEFAULT_PT_MWAX",
    "DEFAULT_PT_PLNT",
    "DEFAULT_PT_WOOD",
    "OMNI_PT_POLY",
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


def check_registries(root: Path, errors: list[str]) -> None:
    registry_path = root / "docs" / "ELEMENT_REGISTRY.csv"
    rows = read_csv(registry_path, errors)
    by_id = {
        int(row["stable_id"]): row
        for row in rows
        if row.get("stable_id", "").isdigit()
    }
    for stable_id, (meson, code, en_name, zh_name, menu, state) in EXPECTED.items():
        row = by_id.get(stable_id)
        if not row:
            errors.append(f"{registry_path}: missing organic batch 2 stable ID {stable_id}")
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
                    f"{registry_path}: ID {stable_id} {field}={row.get(field)!r}; "
                    f"expected {value!r}"
                )

    identifiers = {row.get("identifier") for row in rows}
    missing_merges = sorted(CANONICAL_MERGES - identifiers)
    if missing_merges:
        errors.append("ELEMENT_REGISTRY.csv: missing canonical merges " + ",".join(missing_merges))
    forbidden_duplicates = {
        "OMNI_PT_LUBE",
        "OMNI_PT_PARA",
        "OMNI_PT_CELLULOSE_MIXTURE",
        "OMNI_PT_POLYETHYLENE2",
    } & identifiers
    if forbidden_duplicates:
        errors.append(
            "ELEMENT_REGISTRY.csv: duplicate canonical materials added "
            + ",".join(sorted(forbidden_duplicates))
        )

    compounds = read_csv(root / "docs" / "COMPOUND_REGISTRY.csv", errors)
    compound_ids = {row.get("stable_id") for row in compounds}
    missing_compounds = {str(stable_id) for stable_id in EXPECTED} - compound_ids
    if missing_compounds:
        errors.append(
            "COMPOUND_REGISTRY.csv: missing organic batch 2 stable IDs "
            + ",".join(sorted(missing_compounds, key=int))
        )

    content = read_csv(root / "docs" / "ELEMENT_CONTENT.csv", errors)
    content_ids = {row.get("identifier") for row in content}
    missing_content = {
        f"OMNI_PT_{meson}" for meson, *_rest in EXPECTED.values()
    } - content_ids
    if missing_content:
        errors.append(
            "ELEMENT_CONTENT.csv: missing organic batch 2 identifiers "
            + ",".join(sorted(missing_content))
        )

    reactions = read_csv(root / "docs" / "REACTION_REGISTRY.csv", errors)
    reaction_ids = {row.get("reaction_id") for row in reactions}
    missing_reactions = sorted(REACTIONS - reaction_ids)
    if missing_reactions:
        errors.append(
            "REACTION_REGISTRY.csv: missing organic batch 2 reactions "
            + ",".join(missing_reactions)
        )


def check_engine(root: Path, errors: list[str]) -> None:
    path = root / "src" / "simulation" / "OmniOrganics.cpp"
    text = read_text(path, errors)
    markers = {
        "shared chemistry module": "OmniChemistryModuleEnabled(sim)",
        "shared chemistry budget": "OmniConsumeChemistryEvent(sim)",
        "bounded local scan": "for (int ry = -1; ry <= 1; ++ry)",
        "pair polymerisation": "bool PairPolymerisation",
        "biomolecule hydrolysis": "bool BiomoleculeHydrolysis",
        "nylon condensation": "bool NylonCondensation",
        "esterification": "bool Esterification",
        "bitumen residue": "bool BitumenResidue",
        "PVC decomposition": "bool PvcThermalDecomposition",
        "propylene route": "PT_PRPE, PT_PPLY",
        "rubber route": "PT_BDIE, PT_RUBR",
        "PVC route": "PT_VCHL, PT_PVCL",
        "polystyrene route": "PT_STYR, PT_PSTY",
        "PTFE route": "PT_TFET, PT_PTFE",
        "epoxy route": "PT_ERES, PT_EPXY",
    }
    for label, marker in markers.items():
        if marker not in text:
            errors.append(f"{path}: missing {label}: {marker!r}")
    if re.search(r"\bNPART\b|parts\.active|for\s*\([^\n]*PT_NUM", text):
        errors.append(f"{path}: global particle or element scan is forbidden")

    chemistry = read_text(root / "src" / "simulation" / "OmniChemistry.cpp", errors)
    glucose_position = chemistry.find("FindLocal(x, y, PT_GLUC")
    plant_position = chemistry.find("FindLocal(x, y, PT_PLNT", glucose_position)
    if glucose_position < 0 or plant_position < glucose_position:
        errors.append("OmniChemistry.cpp: official yeast does not prefer GLUC before PLNT fallback")

    content = read_text(root / "src" / "gui" / "game" / "OmniContent.h", errors)
    for marker in (
        "OmniOrganicFirstId = 589",
        "OmniOrganicLastId = 621",
        "OmniElectronicsFirstId = 622",
        "OmniFutureContentFirstId = 670",
    ):
        if marker not in content:
            errors.append(f"OmniContent.h: missing organic batch 2 range marker {marker!r}")

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

    runtime = read_text(root / "tools" / "runtime" / "organic_batch2_regression.lua", errors)
    for marker in (
        'report:write("OMNI_ORGANIC_BATCH2_ELEMENTS=20',
        'report:write("OMNI_ORGANIC_BATCH2_REACTION_PATHS=13',
        'report:write("OMNI_ORGANIC_BATCH2_EVENT_PEAK=",',
    ):
        if marker not in runtime:
            errors.append(f"organic_batch2_regression.lua: missing evidence marker {marker!r}")


def audit(root: Path) -> list[str]:
    errors: list[str] = []
    check_registries(root, errors)
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
            print(f"organic-batch2-audit: ERROR {error}", file=sys.stderr)
        print(f"organic-batch2-audit: FAIL ({len(errors)} errors)", file=sys.stderr)
        return 1
    if not args.quiet:
        print("organic-batch2-audit: PASS (20 elements, 13 reactions, 6 canonical merges)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
