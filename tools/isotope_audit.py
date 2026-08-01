#!/usr/bin/env python3
"""Fail-closed audit for the representative isotope and fuel-cycle batch."""

from __future__ import annotations

import argparse
import csv
from pathlib import Path
import re
import sys
from typing import Sequence


EXPECTED = {
    576: ("H2IS", "H-2", "DTER"),
    577: ("H3IS", "H-3", "TRIT"),
    578: ("C14I", "C-14", "CTRC"),
    579: ("CO60", "Co-60", "COGM"),
    580: ("SR90", "Sr-90", "SRBT"),
    581: ("I131", "I-131", "IODR"),
    582: ("CS37", "Cs-137", "CSGM"),
    583: ("TH32", "Th-232", "THRT"),
    584: ("U235", "U-235", "UFIS"),
    585: ("U238", "U-238", "UFRT"),
    586: ("PU39", "Pu-239", "PUTF"),
    587: ("AM41", "Am-241", "AMIS"),
    588: ("CF52", "Cf-252", "CFNS"),
}

EXPECTED_NEUTRON_BEHAVIOR = {
    576: "capture_to_H3IS",
    577: "neutral",
    578: "neutral",
    579: "activation_product",
    580: "neutral",
    581: "neutral",
    582: "neutral",
    583: "fertile_to_NFUL",
    584: "moderated_fissile",
    585: "fertile_to_PU39",
    586: "moderated_fissile",
    587: "neutral",
    588: "unmoderated_fissile_neutron_source",
}


def read_text(path: Path, errors: list[str]) -> str:
    try:
        return path.read_text(encoding="utf-8")
    except (OSError, UnicodeDecodeError) as exc:
        errors.append(f"{path}: cannot read UTF-8 text: {exc}")
        return ""


def read_csv(path: Path, errors: list[str]) -> list[dict[str, str]]:
    try:
        with path.open("r", encoding="utf-8", newline="") as stream:
            return list(csv.DictReader(stream))
    except (OSError, UnicodeDecodeError, csv.Error) as exc:
        errors.append(f"{path}: cannot read CSV: {exc}")
        return []


def check_registries(root: Path, errors: list[str]) -> None:
    element_path = root / "docs" / "ELEMENT_REGISTRY.csv"
    elements = read_csv(element_path, errors)
    by_id = {
        int(row["stable_id"]): row
        for row in elements
        if row.get("stable_id", "").isdigit()
    }
    isotope_path = root / "docs" / "ISOTOPE_REGISTRY.csv"
    isotopes = read_csv(isotope_path, errors)
    isotope_by_id = {
        int(row["stable_id"]): row
        for row in isotopes
        if row.get("stable_id", "").isdigit()
    }
    for stable_id, (name, symbol, display_code) in EXPECTED.items():
        row = by_id.get(stable_id)
        if not row:
            errors.append(f"{element_path}: missing stable isotope ID {stable_id}")
            continue
        expected = {
            "identifier": f"OMNI_PT_{name}",
            "meson_name": name,
            "display_code": display_code,
            "module": "nuclear",
            "menu_category": "SC_NUCLEAR",
            "implementation_status": "implemented",
            "default_enabled": "true",
            "is_duplicate": "false",
        }
        for field, value in expected.items():
            if row.get(field) != value:
                errors.append(
                    f"{element_path}: ID {stable_id} {field}={row.get(field)!r}; "
                    f"expected {value!r}"
                )
        isotope = isotope_by_id.get(stable_id)
        if not isotope:
            errors.append(f"{isotope_path}: missing stable isotope ID {stable_id}")
            continue
        if isotope.get("identifier") != f"OMNI_PT_{name}":
            errors.append(f"{isotope_path}: ID {stable_id} identifier mismatch")
        if isotope.get("symbol") != symbol:
            errors.append(f"{isotope_path}: ID {stable_id} symbol mismatch")
        if isotope.get("status") != "implemented":
            errors.append(f"{isotope_path}: ID {stable_id} is not implemented")
        if isotope.get("tests") != "runtime-tested":
            errors.append(f"{isotope_path}: ID {stable_id} lacks runtime evidence")
        expected_neutron = EXPECTED_NEUTRON_BEHAVIOR[stable_id]
        if isotope.get("neutron_behavior") != expected_neutron:
            errors.append(
                f"{isotope_path}: ID {stable_id} neutron_behavior="
                f"{isotope.get('neutron_behavior')!r}; expected {expected_neutron!r}"
            )
        if stable_id == 576 and isotope.get("heat") != "none":
            errors.append(f"{isotope_path}: stable hydrogen-2 must not claim decay heat")


def check_engine(root: Path, errors: list[str]) -> None:
    path = root / "src" / "simulation" / "OmniIsotopes.cpp"
    text = read_text(path, errors)
    markers = {
        "shared module gate": "OmniNuclearModuleEnabled(sim)",
        "shared event budget": "OmniConsumeNuclearEvent(sim)",
        "3x3 local scan": "for (int ry = -1; ry <= 1; ++ry)",
        "stable hydrogen capture": "PT_H3IS",
        "bounded hydrogen-isotope ignition": "bool IgniteHydrogenIsotope",
        "cobalt activation": "parts[i].type != PT_COBT",
        "fission products": "PT_SR90, PT_I131, PT_CS37",
        "control rod": "PT_CROD",
        "moderator": "PT_MODR",
        "uranium breeding": "PT_PU39",
        "bounded radiation": "parts[radiation].life = 24",
        "molten timer": "MoltenIsotopeMarker",
        "molten transition ownership": "bool OmniIsotopeOwnsMoltenTransition",
    }
    for label, marker in markers.items():
        if marker not in text:
            errors.append(f"{path}: missing {label}: {marker!r}")
    if re.search(r"\bNPART\b|parts\.active|for\s*\([^\n]*PT_NUM", text):
        errors.append(f"{path}: global particle or element scan is forbidden")

    nuclear = read_text(root / "src" / "simulation" / "OmniNuclear.cpp", errors)
    if "NuclearEventsPerFrame = 512" not in nuclear:
        errors.append("OmniNuclear.cpp: shared nuclear budget is not 512/frame")
    if "bool OmniConsumeNuclearEvent" not in nuclear:
        errors.append("OmniNuclear.cpp: shared isotope budget API missing")

    simulation = read_text(root / "src" / "simulation" / "Simulation.cpp", errors)
    if "OmniIsotopeOwnsMoltenTransition(parts[i])" not in simulation:
        errors.append(
            "Simulation.cpp: typed isotope lava is not protected from generic freezing"
        )

    runtime = read_text(root / "tools" / "runtime" / "isotope_regression.lua", errors)
    if "original_life - molten_timer <= 1" not in runtime:
        errors.append("isotope_regression.lua: molten decay-timer continuity is not tested")
    if 'report:write("OMNI_ISOTOPE_TIMER_CONTINUITY=true' not in runtime:
        errors.append("isotope_regression.lua: timer-continuity evidence is not reported")
    if 'report:write("OMNI_ISOTOPE_IGNITION_PATHS=2' not in runtime:
        errors.append("isotope_regression.lua: hydrogen-isotope ignition is not reported")

    deut = read_text(root / "src" / "simulation" / "elements" / "DEUT.cpp", errors)
    if 'Identifier = "DEFAULT_PT_DEUT"' not in deut or "TYPE_LIQUID" not in deut:
        errors.append("DEUT.cpp: official heavy-water DEUT was unexpectedly replaced")

    for _stable_id, (name, _symbol, _display_code) in EXPECTED.items():
        element_path = root / "src" / "simulation" / "elements" / f"{name}.cpp"
        element = read_text(element_path, errors)
        for marker in (
            f'Identifier = "OMNI_PT_{name}"',
            "MenuSection = SC_NUCLEAR",
            "HeatCapacity =",
            "Description = Localization::Ref().Tr",
            "HighTemperatureTransition = NT",
            "Update = &OmniIsotopeElementUpdate",
            "Create = &OmniIsotopeCreate",
        ):
            if marker not in element:
                errors.append(f"{element_path}: missing {marker!r}")


def audit(root: Path) -> list[str]:
    errors: list[str] = []
    check_registries(root, errors)
    check_engine(root, errors)
    return errors


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--source-root", type=Path, default=Path(__file__).resolve().parents[1]
    )
    parser.add_argument("--quiet", action="store_true")
    return parser


def main(argv: Sequence[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    errors = audit(args.source_root.resolve())
    if errors:
        for error in errors:
            print(f"isotope-audit: ERROR {error}", file=sys.stderr)
        print(f"isotope-audit: FAIL ({len(errors)} errors)", file=sys.stderr)
        return 1
    if not args.quiet:
        print(
            "isotope-audit: PASS (13 stable-ID isotope elements, "
            "3x3 local, shared 512/frame)"
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
