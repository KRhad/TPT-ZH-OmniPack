#!/usr/bin/env python3
"""Fail-closed static checks for the bounded Phase 3 metallurgy module."""

from __future__ import annotations

import argparse
import csv
from pathlib import Path
import re
import sys
from typing import Sequence


EXPECTED_ELEMENTS = {
    256: "ALUM",
    257: "COPR",
    258: "LEAD",
    259: "TIN",
    260: "NICL",
    261: "MAGN",
    262: "CHRM",
    263: "COBT",
    264: "MOLY",
    265: "ZINC",
    266: "CHRC",
    267: "COKE",
    268: "STEL",
    269: "BRNZ",
    270: "BRAS",
    271: "SSIL",
    272: "NCRM",
    273: "ALMG",
    274: "TSTL",
    275: "SLAG",
    276: "FLUX",
    277: "CRUC",
    278: "MSCR",
}

EXPECTED_ALLOY_INPUTS = {
    "PT_SSIL": {"PT_STEL": 4, "PT_CHRM": 1, "PT_NICL": 1},
    "PT_TSTL": {"PT_STEL": 4, "PT_COBT": 1, "PT_MOLY": 1},
    "PT_BRNZ": {"PT_COPR": 3, "PT_TIN": 1},
    "PT_BRAS": {"PT_COPR": 3, "PT_ZINC": 1},
    "PT_NCRM": {"PT_NICL": 4, "PT_CHRM": 1},
    "PT_ALMG": {"PT_ALUM": 4, "PT_MAGN": 1},
}

RECIPE_RE = re.compile(
    r"\{\s*\{\s*"
    r"Ingredient\{\s*(PT_[A-Z0-9]+)\s*,\s*(\d+)\s*\}\s*,\s*"
    r"Ingredient\{\s*(PT_[A-Z0-9]+)\s*,\s*(\d+)\s*\}\s*,\s*"
    r"Ingredient\{\s*(PT_[A-Z0-9]+)\s*,\s*(\d+)\s*\}\s*\}\s*,\s*"
    r"(\d+)\s*,\s*(PT_[A-Z0-9]+)\s*,",
)


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
    for line_number, row in enumerate(rows, start=2):
        try:
            stable_id = int(row.get("stable_id", ""))
        except ValueError:
            continue
        if stable_id in by_id:
            errors.append(f"{path}:{line_number}: duplicate stable ID {stable_id}")
        by_id[stable_id] = row

    for stable_id, meson_name in EXPECTED_ELEMENTS.items():
        row = by_id.get(stable_id)
        if row is None:
            errors.append(f"{path}: missing metallurgy stable ID {stable_id}")
            continue
        expected = {
            "identifier": f"OMNI_PT_{meson_name}",
            "meson_name": meson_name,
            "source_file": f"src/simulation/elements/{meson_name}.cpp",
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


def parse_alloy_recipes(text: str, errors: list[str]) -> None:
    found: dict[str, dict[str, int]] = {}
    for match in RECIPE_RE.finditer(text):
        ingredient_count = int(match.group(7))
        product = match.group(8)
        ingredients: dict[str, int] = {}
        for offset in (1, 3, 5):
            material = match.group(offset)
            count = int(match.group(offset + 1))
            if material != "PT_NONE" and count:
                ingredients[material] = count
        if ingredient_count != len(ingredients):
            errors.append(
                f"OmniMetallurgy.cpp: {product} ingredientCount "
                f"{ingredient_count} does not match {len(ingredients)} entries"
            )
        if product in found:
            errors.append(
                f"OmniMetallurgy.cpp: duplicate alloy product {product}"
            )
        found[product] = ingredients

    if found != EXPECTED_ALLOY_INPUTS:
        errors.append(
            "OmniMetallurgy.cpp: alloy recipe table differs from the "
            f"audited stoichiometry; found={found!r}"
        )
    for product, ingredients in found.items():
        if sum(ingredients.values()) > 9:
            errors.append(
                f"OmniMetallurgy.cpp: {product} exceeds the 3x3 input capacity"
            )


def check_engine(root: Path, errors: list[str]) -> None:
    engine_path = root / "src" / "simulation" / "OmniMetallurgy.cpp"
    text = read_text(engine_path, errors)
    if not text:
        return

    parse_alloy_recipes(text, errors)
    required_markers = {
        "single bounded neighbourhood": "MaxLocalParticles = 9",
        "per-frame reaction budget": "ReactionsPerFrame = 2048",
        "deterministic local collection": "CollectLocalParticles(",
        "same-frame cascade guard": "tmp3 == sim->currentTick + 1",
        "steel iron ratio": "PT_IRON, 4, PhaseMatch::Molten",
        "steel coke input": "PT_COKE, 1, PhaseMatch::Direct",
        "molten-compatible flux": (
            "PT_FLUX, 1, PhaseMatch::DirectOrMolten"
        ),
        "steel carbon off-gas": "PT_CO2",
        "steel slag output": "PT_SLAG",
        "pressure scrap preservation": "BreakIntoScrap(",
        "scrap ctype recovery": "parts[i].ctype = sourceType",
        "charcoal hold time": "++parts[i].tmp3 < 60",
        "coke hold time": "++parts[i].tmp3 < 90",
    }
    for label, marker in required_markers.items():
        if marker not in text:
            errors.append(f"{engine_path}: missing {label}: {marker!r}")

    forbidden = {
        "global particle scan": re.compile(r"\bNPART\b|\bparts\.active\b"),
        "recursive particle creation": re.compile(
            r"OmniMetallurgyLavaUpdate[\s\S]*create_part\s*\("
        ),
    }
    for label, pattern in forbidden.items():
        if pattern.search(text):
            errors.append(f"{engine_path}: forbidden {label} detected")
    alloy_section = text.split("bool TryAlloyRecipes", 1)
    if len(alloy_section) != 2:
        errors.append(f"{engine_path}: TryAlloyRecipes definition is missing")
    else:
        alloy_body = alloy_section[1].split("bool TrySteelRecipe", 1)[0]
        if "rng.chance" in alloy_body:
            errors.append(
                f"{engine_path}: forbidden probabilistic alloy matching detected"
            )

    fire = read_text(
        root / "src" / "simulation" / "elements" / "FIRE.cpp", errors
    )
    if (
        "t == PT_LAVA && OmniMetallurgyLavaUpdate"
        not in fire
        or "return 0;" not in fire
    ):
        errors.append(
            "FIRE.cpp: metallurgy LAVA result is not propagated before legacy"
        )

    base = read_text(
        root / "src" / "simulation" / "elements" / "BASE.cpp", errors
    )
    if "rt >= PT_ALUM && rt <= PT_TSTL" not in base or "PT_MSCR" not in base:
        errors.append(
            "BASE.cpp: custom conductive metals do not preserve ctype in MSCR"
        )

    mscr = read_text(
        root / "src" / "simulation" / "elements" / "MSCR.cpp", errors
    )
    if "CarriesTypeIn = 1U << FIELD_CTYPE" not in mscr:
        errors.append("MSCR.cpp: ctype is not declared as save-carried data")

    lead = read_text(
        root / "src" / "simulation" / "elements" / "LEAD.cpp", errors
    )
    if "PROP_NEUTABSORB" not in lead:
        errors.append("LEAD.cpp: neutron shielding property is missing")

    spark = read_text(
        root / "src" / "simulation" / "elements" / "SPRK.cpp", errors
    )
    if "case PT_NCRM:" not in spark:
        errors.append("SPRK.cpp: nichrome heating behavior is missing")

    for element_name in EXPECTED_ELEMENTS.values():
        element_path = (
            root
            / "src"
            / "simulation"
            / "elements"
            / f"{element_name}.cpp"
        )
        element_text = read_text(element_path, errors)
        if "HeatCapacity =" not in element_text:
            errors.append(
                f"{element_path}: explicit HeatCapacity is missing"
            )


def audit(root: Path) -> list[str]:
    errors: list[str] = []
    check_registry(root, errors)
    check_engine(root, errors)
    return errors


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Audit bounded metallurgy IDs, recipes and performance guards."
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
    root = args.source_root.resolve()
    errors = audit(root)
    if errors:
        for error in errors:
            print(f"metallurgy-audit: ERROR {error}", file=sys.stderr)
        print(
            f"metallurgy-audit: FAIL ({len(errors)} errors)",
            file=sys.stderr,
        )
        return 1
    if not args.quiet:
        print(
            "metallurgy-audit: PASS "
            "(23 elements, 6 alloy recipes, 1 steel recipe, 3x3 bounded)"
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
