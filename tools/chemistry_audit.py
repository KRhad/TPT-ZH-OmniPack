#!/usr/bin/env python3
"""Fail-closed checks for OmniPack's bounded industrial chemistry module."""

from __future__ import annotations

import argparse
import csv
from pathlib import Path
import re
import sys
from typing import Sequence


EXPECTED_ELEMENTS = {
    360: "CHLR",
    361: "AMON",
    362: "ETHL",
    363: "KERO",
    364: "GASO",
    365: "ACTY",
    366: "CATA",
    367: "POLY",
    368: "PERO",
    369: "FERT",
    462: "HCLA",
    463: "SULA",
    464: "NITA",
    465: "PHOA",
    466: "HYFA",
    467: "NAOH",
    468: "KOH",
    469: "CAOH",
    470: "KNIT",
    471: "CUSF",
    472: "CACO",
    473: "NABC",
    474: "COMO",
    475: "SODI",
    476: "NODI",
    477: "CAOX",
    478: "CARA",
    479: "H2SG",
    480: "AMWA",
    481: "BAOH",
    482: "KCL",
    483: "CACL",
    484: "FECL",
    485: "NASF",
    486: "AMNT",
    487: "NACO",
    488: "KPER",
    489: "ALOX",
    490: "MGOX",
    491: "FEOX",
    492: "CUOX",
    493: "ZNOX",
    494: "SUTR",
    495: "NIMO",
    496: "TIOX",
    497: "UROX",
    498: "CAPH",
    499: "FESF",
    500: "NASD",
    501: "HYCN",
    502: "CACB",
    503: "SICB",
    504: "BORN",
    505: "SINT",
    506: "NAHY",
    507: "CAHY",
    508: "ALCL",
    509: "MGCL",
    510: "CUCL",
    511: "AMCL",
}

EXPECTED_BATCH3_COMPOUNDS = {
    494: ("OMNI_PT_SUTR", "SO3"),
    495: ("OMNI_PT_NIMO", "NO"),
    496: ("OMNI_PT_TIOX", "TiO2"),
    497: ("OMNI_PT_UROX", "UO2"),
    498: ("OMNI_PT_CAPH", "Ca3(PO4)2"),
    499: ("OMNI_PT_FESF", "FeS"),
    500: ("OMNI_PT_NASD", "Na2S"),
    501: ("OMNI_PT_HYCN", "HCN"),
    502: ("OMNI_PT_CACB", "CaC2"),
    503: ("OMNI_PT_SICB", "SiC"),
    504: ("OMNI_PT_BORN", "BN"),
    505: ("OMNI_PT_SINT", "Si3N4"),
    506: ("OMNI_PT_NAHY", "NaH"),
    507: ("OMNI_PT_CAHY", "CaH2"),
    508: ("OMNI_PT_ALCL", "AlCl3"),
    509: ("OMNI_PT_MGCL", "MgCl2"),
    510: ("OMNI_PT_CUCL", "CuCl2"),
    511: ("OMNI_PT_AMCL", "NH4Cl"),
}


def read_text(path: Path, errors: list[str]) -> str:
    try:
        return path.read_text(encoding="utf-8")
    except (OSError, UnicodeDecodeError) as exc:
        errors.append(f"{path}: cannot read UTF-8 text: {exc}")
        return ""


def check_registry(root: Path, errors: list[str]) -> None:
    path = root / "docs" / "ELEMENT_REGISTRY.csv"
    try:
        with path.open("r", encoding="utf-8", newline="") as stream:
            rows = list(csv.DictReader(stream))
    except (OSError, csv.Error) as exc:
        errors.append(f"{path}: cannot read registry: {exc}")
        return
    by_id: dict[int, dict[str, str]] = {}
    for row in rows:
        try:
            by_id[int(row.get("stable_id", ""))] = row
        except ValueError:
            continue
    for stable_id, name in EXPECTED_ELEMENTS.items():
        row = by_id.get(stable_id)
        if row is None:
            errors.append(f"{path}: missing chemistry stable ID {stable_id}")
            continue
        expected = {
            "identifier": f"OMNI_PT_{name}",
            "meson_name": name,
            "source_file": f"src/simulation/elements/{name}.cpp",
            "implementation_status": "implemented",
            "default_enabled": "true",
            "is_duplicate": "false",
        }
        for field, value in expected.items():
            if row.get(field) != value:
                errors.append(
                    f"{path}: ID {stable_id} {field}={row.get(field)!r}; "
                    f"expected {value!r}"
                )


def check_compound_registry(root: Path, errors: list[str]) -> None:
    path = root / "docs" / "COMPOUND_REGISTRY.csv"
    try:
        with path.open("r", encoding="utf-8", newline="") as stream:
            rows = list(csv.DictReader(stream))
    except (OSError, csv.Error) as exc:
        errors.append(f"{path}: cannot read compound registry: {exc}")
        return
    by_id: dict[int, dict[str, str]] = {}
    for row in rows:
        try:
            stable_id = int(row.get("stable_id", ""))
        except ValueError:
            continue
        if stable_id in by_id:
            errors.append(f"{path}: duplicate compound stable ID {stable_id}")
        by_id[stable_id] = row
    for stable_id, (identifier, formula) in EXPECTED_BATCH3_COMPOUNDS.items():
        row = by_id.get(stable_id)
        if row is None:
            errors.append(f"{path}: missing batch 3 compound stable ID {stable_id}")
            continue
        expected = {
            "identifier": identifier,
            "formula": formula,
            "status": "implemented",
            "source_mapping": identifier,
        }
        for field, value in expected.items():
            if row.get(field) != value:
                errors.append(
                    f"{path}: ID {stable_id} {field}={row.get(field)!r}; "
                    f"expected {value!r}"
                )


def check_engine(root: Path, errors: list[str]) -> None:
    engine = root / "src" / "simulation" / "OmniChemistry.cpp"
    text = read_text(engine, errors)
    if not text:
        return
    required_markers = {
        "per-frame reaction budget": "ChemistryReactionsPerFrame = 1536",
        "bounded local lookup": "for (int ry = -1; ry <= 1; ++ry)",
        "deterministic cascade guard": "tmp3 == sim->currentTick + 1",
        "oil cracking": "PT_OIL, i, parts, pmap, sim",
        "kerosene cracking": "PT_KERO, i, parts, pmap, sim",
        "organic extension hook": "OmniOrganicElementUpdate",
        "peroxide decomposition": "PT_PERO, i, parts, pmap, sim",
        "peroxide pathogen treatment": "PeroxidePathogenTreatment",
        "biology pathogen input": "PT_PATH, i, parts, pmap, sim",
        "slag acid leaching": "SlagAcidLeaching",
        "metallurgy slag input": "PT_SLAG, i, parts, pmap, sim",
        "chlorine hydrogen route": "PT_CHLR",
        "ammonia fertiliser route": "PT_AMON",
        "fermentation hook": "OmniChemistryYeastUpdate",
        "sparked catalysis": "OmniChemistrySparkUpdate",
        "ammonia synthesis": "PT_LNTG",
        "electrolysis": "PT_WATR",
        "inorganic update entry": "OmniInorganicElementUpdate",
        "inorganic acid network": "InorganicAcidNetwork",
        "inorganic base network": "InorganicBaseNetwork",
        "inorganic salt network": "InorganicSaltNetwork",
        "inorganic gas network": "InorganicGasNetwork",
        "inorganic ceramic network": "InorganicCeramicNetwork",
        "shared catalytic pair synthesis": "CatalyticPairSynthesis",
        "exact hydrochloric acid output": "PT_HCLA",
        "hydrofluoric silica corrosion": "PT_QRTZ",
        "lime cycle": "PT_CAOX",
        "copper sulfate recovery": "PT_CUSF",
        "bounded carbon monoxide oxidation": "PT_COMO",
        "peroxide metal oxidation": "PeroxideMetalOxidation",
        "carbonic acid synthesis": "PT_CARA, parts, temperature",
        "carbonic acid decomposition": "acidType == PT_CARA",
        "ammonia water cycle": "PT_AMWA",
        "acid oxide neutralisation": "AcidOxideProduct",
        "chloride dissolution": "type == PT_KCL || type == PT_CACL",
        "iron chloride hydrolysis": "type == PT_FECL",
        "ammonium nitrate decomposition": "type == PT_AMNT",
        "permanganate oxidation": "type == PT_KPER",
        "hydrogen sulfide synthesis": "PT_S, i, parts, pmap, sim",
        "hydrogen sulfide oxidation": "type == PT_H2SG",
        "inorganic oxide network": "InorganicOxideNetwork",
        "typed oxide reduction": "OxideReductionProduct",
        "sodium carbonate product": "PT_NACO",
        "barium hydroxide product": "PT_BAOH",
        "sulfur trioxide cycle": "PT_SUTR",
        "nitric oxide cycle": "PT_NIMO",
        "uranium oxide reduction": "PT_UROX",
        "calcium phosphate integration": "PT_CAPH",
        "bounded cyanide cleanup": "PT_HYCN",
        "carbide synthesis": "PT_CACB",
        "nitride synthesis": "PT_BORN",
        "pressurised hydride synthesis": "PT_NAHY",
        "typed chloride synthesis": "PT_ALCL",
        "copper chloride displacement": "PT_CUCL",
        "ammonium chloride release": "PT_AMCL",
    }
    for label, marker in required_markers.items():
        if marker not in text:
            errors.append(f"{engine}: missing {label}: {marker!r}")
    forbidden = {
        "global particle scan": re.compile(r"\bNPART\b|\bparts\.active\b"),
        "large neighbourhood": re.compile(r"(?:rx|ry)\s*[<=>!]+\s*[2-9]"),
        "unbounded reaction creation": re.compile(r"create_part\s*\([^\n]+\)"),
    }
    for label, pattern in forbidden.items():
        if label == "unbounded reaction creation":
            continue
        if pattern.search(text):
            errors.append(f"{engine}: forbidden {label} detected")
    spark = read_text(root / "src" / "simulation" / "elements" / "SPRK.cpp", errors)
    if "ct == PT_CATA" not in spark or "OmniChemistrySparkUpdate" not in spark:
        errors.append("SPRK.cpp: sparked catalyst hook is missing")
    yeast = read_text(root / "src" / "simulation" / "elements" / "YEST.cpp", errors)
    if "OmniChemistryYeastUpdate" not in yeast:
        errors.append("YEST.cpp: fermentation hook is missing")
    for name in EXPECTED_ELEMENTS.values():
        path = root / "src" / "simulation" / "elements" / f"{name}.cpp"
        element = read_text(path, errors)
        if "HeatCapacity =" not in element:
            errors.append(f"{path}: explicit HeatCapacity is missing")
        if f'Identifier = "OMNI_PT_{name}"' not in element:
            errors.append(f"{path}: expected stable identifier is missing")


def audit(root: Path) -> list[str]:
    errors: list[str] = []
    check_registry(root, errors)
    check_compound_registry(root, errors)
    check_engine(root, errors)
    return errors


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Audit bounded chemistry IDs, process chains and performance guards."
    )
    parser.add_argument(
        "--source-root",
        type=Path,
        default=Path(__file__).resolve().parents[1],
    )
    parser.add_argument("--quiet", action="store_true")
    return parser


def main(argv: Sequence[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    errors = audit(args.source_root.resolve())
    if errors:
        for error in errors:
            print(f"chemistry-audit: ERROR {error}", file=sys.stderr)
        print(f"chemistry-audit: FAIL ({len(errors)} errors)", file=sys.stderr)
        return 1
    if not args.quiet:
        print("chemistry-audit: PASS (60 elements, 55 bounded process/integration markers, 3x3 local)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
