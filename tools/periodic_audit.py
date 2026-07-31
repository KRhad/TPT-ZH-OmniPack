#!/usr/bin/env python3
"""Fail-closed audit for periodic-table runtime data and implemented family batches."""

from __future__ import annotations

import argparse
import csv
from pathlib import Path
import re
import sys
from typing import Sequence


EXPECTED_NOBLE_ELEMENTS = {
    370: "HE",
    375: "NE",
    379: "AR",
    390: "KR",
    405: "XE",
    431: "RN",
    461: "OG",
}

EXPECTED_ALKALI_ELEMENTS = {
    376: "NA",
    380: "K",
    406: "CS",
    432: "FR",
}

EXPECTED_ALKALINE_EARTH_ELEMENTS = {
    371: "BE",
    381: "CA",
    391: "SR",
    407: "BA",
    433: "RA",
}

EXPECTED_BORON_GROUP_ELEMENTS = {
    372: "B",
    385: "GA",
    401: "IN",
    428: "TL",
    456: "NH",
}

EXPECTED_CARBON_GROUP_ELEMENTS = {
    386: "GE",
    457: "FL",
}

EXPECTED_NITROGEN_GROUP_ELEMENTS = {
    373: "N",
    377: "P",
    387: "AS",
    402: "SB",
    429: "BI",
    458: "MC",
}

EXPECTED_OXYGEN_GROUP_ELEMENTS = {
    378: "S",
    388: "SE",
    403: "TE",
    459: "LV",
}

EXPECTED_HALOGEN_ELEMENTS = {
    374: "F",
    389: "BR",
    404: "I",
    430: "AT",
    460: "TS",
}

EXPECTED_FIRST_TRANSITION_ELEMENTS = {
    382: "SC",
    383: "V",
    384: "MN",
}

EXPECTED_SECOND_TRANSITION_ELEMENTS = {
    392: "Y",
    393: "ZR",
    394: "NB",
    395: "TC",
    396: "RU",
    397: "RH",
    398: "PD",
    399: "AG",
    400: "CD",
}

EXPECTED_NEW_ELEMENTS = (
    EXPECTED_NOBLE_ELEMENTS
    | EXPECTED_ALKALI_ELEMENTS
    | EXPECTED_ALKALINE_EARTH_ELEMENTS
    | EXPECTED_BORON_GROUP_ELEMENTS
    | EXPECTED_CARBON_GROUP_ELEMENTS
    | EXPECTED_NITROGEN_GROUP_ELEMENTS
    | EXPECTED_OXYGEN_GROUP_ELEMENTS
    | EXPECTED_HALOGEN_ELEMENTS
    | EXPECTED_FIRST_TRANSITION_ELEMENTS
    | EXPECTED_SECOND_TRANSITION_ELEMENTS
)


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


def check_registry(root: Path, errors: list[str]) -> None:
    path = root / "docs" / "ELEMENT_REGISTRY.csv"
    rows = read_csv(path, errors)
    by_id: dict[int, dict[str, str]] = {}
    for row in rows:
        try:
            by_id[int(row["stable_id"])] = row
        except (KeyError, ValueError):
            continue
    for stable_id, name in EXPECTED_NEW_ELEMENTS.items():
        row = by_id.get(stable_id)
        if row is None:
            errors.append(f"{path}: missing periodic stable ID {stable_id}")
            continue
        expected = {
            "identifier": f"OMNI_PT_{name}",
            "meson_name": name,
            "module": "periodic",
            "implementation_status": "implemented",
            "default_enabled": "true",
            "source_file": f"src/simulation/elements/{name}.cpp",
        }
        for field, value in expected.items():
            if row.get(field) != value:
                errors.append(
                    f"{path}: ID {stable_id} {field}={row.get(field)!r}; expected {value!r}"
                )
    expected_ids = set(range(370, 462))
    present_ids = {stable_id for stable_id in by_id if 370 <= stable_id <= 461}
    if present_ids != expected_ids:
        errors.append(f"{path}: periodic allocation must explicitly cover stable IDs 370..461")


def check_source_map(root: Path, errors: list[str]) -> None:
    path = root / "docs" / "PERIODIC_ELEMENT_SOURCE_MAP.csv"
    rows = read_csv(path, errors)
    if len(rows) != 118:
        errors.append(f"{path}: expected 118 periodic rows and found {len(rows)}")
        return
    if [int(row["atomic_number"]) for row in rows] != list(range(1, 119)):
        errors.append(f"{path}: atomic numbers are not the complete ordered range 1..118")
    implemented = [row for row in rows if row.get("status") == "implemented"]
    if len(implemented) != 76:
        errors.append(
            f"{path}: expected 76 implemented mappings after the second transition batch "
            f"and found {len(implemented)}"
        )
    by_number = {int(row["atomic_number"]): row for row in rows}
    if by_number.get(1, {}).get("stable_id") != "148":
        errors.append(f"{path}: hydrogen must continue to reuse stable ID 148")
    expected_atomic = {
        2: 370, 4: 371, 10: 375, 11: 376, 12: 261,
        18: 379, 19: 380, 20: 381, 36: 390, 38: 391,
        54: 405, 55: 406, 56: 407, 86: 431, 87: 432,
        88: 433, 118: 461, 5: 372, 13: 256, 31: 385,
        49: 401, 81: 428, 113: 456, 6: 28, 14: 187,
        32: 386, 50: 259, 82: 258, 114: 457,
        7: 373, 15: 377, 33: 387, 51: 402, 83: 429, 115: 458,
        8: 61, 16: 378, 34: 388, 52: 403, 84: 182, 116: 459,
        9: 374, 17: 360, 35: 389, 53: 404, 85: 430, 117: 460,
        21: 382, 22: 144, 23: 383, 24: 262, 25: 384,
        26: 76, 27: 263, 28: 260, 29: 257, 30: 265,
        39: 392, 40: 393, 41: 394, 42: 264, 43: 395,
        44: 396, 45: 397, 46: 398, 47: 399, 48: 400,
    }
    for atomic_number, stable_id in expected_atomic.items():
        row = by_number.get(atomic_number, {})
        if row.get("stable_id") != str(stable_id) or row.get("status") != "implemented":
            errors.append(
                f"{path}: atomic number {atomic_number} is not implemented at stable ID {stable_id}"
            )


def check_engine(root: Path, errors: list[str]) -> None:
    engine_path = root / "src" / "simulation" / "OmniPeriodic.cpp"
    engine = read_text(engine_path, errors)
    required = {
        "single-frame budget": "PeriodicEventsPerFrame = 1024",
        "tick reset": "reactionBudget.tick != sim->currentTick",
        "event metric": "sim->RecordOmniEvent()",
        "local X scan": "for (int rx = -1; rx <= 1; ++rx)",
        "local Y scan": "for (int ry = -1; ry <= 1; ++ry)",
        "helium cryogenic behaviour": "parts[i].type != PT_HE || parts[i].temp >= 20.0f",
        "xenon discharge tuning": "case PT_XE:",
        "radon decay": "sourceType == PT_RN ? PT_POLO : PT_RN",
        "alkali water reaction": "ReactAlkaliWithWaterOrAcid",
        "alkali acid salt route": "int residue = acid ? PT_SALT : PT_CAUS",
        "alkali oxygen reaction": "OxidiseHotAlkali",
        "alkali vaporisation": "VaporiseHotAlkali",
        "francium decay": "sourceType != PT_FR || parts[i].type != PT_FR",
        "alkaline-earth water reaction": "ReactAlkalineEarthWithWaterOrAcid",
        "alkaline-earth oxygen reaction": "OxidiseHotAlkalineEarth",
        "alkaline-earth vaporisation": "VaporiseHotAlkalineEarth",
        "radium decay": "sourceType != PT_RA || parts[i].type != PT_RA",
        "element-specific flame colour": "parts[oxygen].dcolour = properties.flameColour",
        "finite discharge photon": "parts[photon].life = 24",
        "finite decay photon": "parts[photon].life = 18",
        "boron neutron capture": "CaptureBoronNeutron",
        "gallium aluminium embrittlement": "EmbrittleAluminiumWithGallium",
        "boron-group acid and caustic chemistry": "ReactBoronGroupWithAcidOrCaustic",
        "boron-group oxidation": "OxidiseHotBoronGroup",
        "boron-group vaporisation": "VaporiseHotBoronGroup",
        "nihonium decay": "sourceType != PT_NH || parts[i].type != PT_NH",
        "carbon-group acid chemistry": "ReactCarbonGroupWithAcid",
        "carbon-group oxidation": "OxidiseHotCarbonGroup",
        "carbon-group vaporisation": "VaporiseHotCarbonGroup",
        "germanium discharge": "ExciteGermanium",
        "tin pest": "EmbrittleColdTin",
        "flerovium decay": "sourceType != PT_FL || parts[i].type != PT_FL",
        "nitrogen discharge": "ExciteNitrogen",
        "nitrogen-group acid chemistry": "ReactNitrogenGroupWithAcid",
        "nitrogen-group oxidation": "OxidiseHotNitrogenGroup",
        "nitrogen-group vaporisation": "VaporiseHotNitrogenGroup",
        "moscovium decay": "sourceType != PT_MC || parts[i].type != PT_MC",
        "oxygen-group oxidation": "OxidiseHotOxygenGroup",
        "oxygen-group vaporisation": "VaporiseHotOxygenGroup",
        "selenium photoelectric excitation": "ExciteSelenium",
        "livermorium decay": "sourceType != PT_LV || parts[i].type != PT_LV",
        "halogen hydrogen chemistry": "ReactHalogenWithHydrogen",
        "fluorine water chemistry": "ReactFluorineWithWater",
        "halogen metal chemistry": "ReactHalogenWithMetal",
        "halogen disinfection": "DisinfectWithHalogen",
        "halogen vaporisation": "VaporiseHalogen",
        "radioactive halogen decay": "DecayRadioactiveHalogen",
        "first-transition acid chemistry": "ReactFirstTransitionWithAcid",
        "first-transition oxidation": "OxidiseHotFirstTransition",
        "first-transition vaporisation": "VaporiseFirstTransition",
        "scandium discharge": "ExciteScandium",
        "vanadium tool-steel alloy": "AlloyVanadiumToolSteel",
        "manganese steel deoxidation": "DeoxidiseSteelWithManganese",
        "second-transition acid chemistry": "ReactSecondTransitionWithAcid",
        "second-transition oxidation": "OxidiseHotSecondTransition",
        "second-transition vaporisation": "VaporiseSecondTransition",
        "yttrium discharge": "ExciteYttrium",
        "zirconium steam oxidation": "ReactZirconiumWithSteam",
        "technetium decay": "DecayTechnetium",
        "platinum-group catalysis": "CatalyseHydrogenWithPlatinumGroup",
        "palladium hydrogen absorption": "AbsorbPalladiumHydrogen",
        "palladium hydrogen release": "ReleasePalladiumHydrogen",
        "silver tarnish": "TarnishSilverWithSulfur",
    }
    for label, marker in required.items():
        if marker not in engine:
            errors.append(f"{engine_path}: missing {label}: {marker!r}")
    if re.search(r"\bNPART\b|parts\.active", engine):
        errors.append(f"{engine_path}: periodic family update must not scan all particles")
    for stable_id, name in EXPECTED_NOBLE_ELEMENTS.items():
        path = root / "src" / "simulation" / "elements" / f"{name}.cpp"
        text = read_text(path, errors)
        for marker in (
            f'Identifier = "OMNI_PT_{name}"',
            "HeatCapacity =",
            "Update = &OmniNobleGasUpdate",
            "Graphics = &OmniNobleGasGraphics",
            "Create = &OmniNobleGasCreate",
        ):
            if marker not in text:
                errors.append(f"{path}: missing periodic element marker {marker!r}")
    for stable_id, name in EXPECTED_ALKALI_ELEMENTS.items():
        path = root / "src" / "simulation" / "elements" / f"{name}.cpp"
        text = read_text(path, errors)
        for marker in (
            f'Identifier = "OMNI_PT_{name}"',
            "HeatCapacity =",
            "Update = &OmniAlkaliMetalUpdate",
            "Create = &OmniAlkaliMetalCreate",
        ):
            if marker not in text:
                errors.append(f"{path}: missing periodic element marker {marker!r}")
    for stable_id, name in EXPECTED_ALKALINE_EARTH_ELEMENTS.items():
        path = root / "src" / "simulation" / "elements" / f"{name}.cpp"
        text = read_text(path, errors)
        for marker in (
            f'Identifier = "OMNI_PT_{name}"',
            "HeatCapacity =",
            "Update = &OmniAlkalineEarthMetalUpdate",
            "Create = &OmniAlkalineEarthMetalCreate",
        ):
            if marker not in text:
                errors.append(f"{path}: missing periodic element marker {marker!r}")
    for stable_id, name in EXPECTED_BORON_GROUP_ELEMENTS.items():
        path = root / "src" / "simulation" / "elements" / f"{name}.cpp"
        text = read_text(path, errors)
        for marker in (
            f'Identifier = "OMNI_PT_{name}"',
            "HeatCapacity =",
            "Update = &OmniBoronGroupUpdate",
            "Create = &OmniBoronGroupCreate",
        ):
            if marker not in text:
                errors.append(f"{path}: missing periodic element marker {marker!r}")
    for stable_id, name in EXPECTED_CARBON_GROUP_ELEMENTS.items():
        path = root / "src" / "simulation" / "elements" / f"{name}.cpp"
        text = read_text(path, errors)
        for marker in (
            f'Identifier = "OMNI_PT_{name}"',
            "HeatCapacity =",
            "Update = &OmniCarbonGroupUpdate",
            "Graphics = &OmniCarbonGroupGraphics",
            "Create = &OmniCarbonGroupCreate",
        ):
            if marker not in text:
                errors.append(f"{path}: missing periodic element marker {marker!r}")
    for stable_id, name in EXPECTED_NITROGEN_GROUP_ELEMENTS.items():
        path = root / "src" / "simulation" / "elements" / f"{name}.cpp"
        text = read_text(path, errors)
        for marker in (
            f'Identifier = "OMNI_PT_{name}"',
            "HeatCapacity =",
            "Update = &OmniNitrogenGroupUpdate",
            "Graphics = &OmniNitrogenGroupGraphics",
            "Create = &OmniNitrogenGroupCreate",
        ):
            if marker not in text:
                errors.append(f"{path}: missing periodic element marker {marker!r}")
    for stable_id, name in EXPECTED_OXYGEN_GROUP_ELEMENTS.items():
        path = root / "src" / "simulation" / "elements" / f"{name}.cpp"
        text = read_text(path, errors)
        for marker in (
            f'Identifier = "OMNI_PT_{name}"',
            "HeatCapacity =",
            "Update = &OmniOxygenGroupUpdate",
            "Graphics = &OmniOxygenGroupGraphics",
            "Create = &OmniOxygenGroupCreate",
        ):
            if marker not in text:
                errors.append(f"{path}: missing periodic element marker {marker!r}")
    for stable_id, name in EXPECTED_HALOGEN_ELEMENTS.items():
        path = root / "src" / "simulation" / "elements" / f"{name}.cpp"
        text = read_text(path, errors)
        for marker in (
            f'Identifier = "OMNI_PT_{name}"',
            "HeatCapacity =",
            "Update = &OmniHalogenUpdate",
            "Graphics = &OmniHalogenGraphics",
            "Create = &OmniHalogenCreate",
        ):
            if marker not in text:
                errors.append(f"{path}: missing periodic element marker {marker!r}")
    for stable_id, name in EXPECTED_FIRST_TRANSITION_ELEMENTS.items():
        path = root / "src" / "simulation" / "elements" / f"{name}.cpp"
        text = read_text(path, errors)
        for marker in (
            f'Identifier = "OMNI_PT_{name}"',
            "HeatCapacity =",
            "Update = &OmniFirstTransitionUpdate",
            "Graphics = &OmniFirstTransitionGraphics",
        ):
            if marker not in text:
                errors.append(f"{path}: missing periodic element marker {marker!r}")
    for stable_id, name in EXPECTED_SECOND_TRANSITION_ELEMENTS.items():
        path = root / "src" / "simulation" / "elements" / f"{name}.cpp"
        text = read_text(path, errors)
        for marker in (
            f'Identifier = "OMNI_PT_{name}"',
            "HeatCapacity =",
            "Update = &OmniSecondTransitionUpdate",
            "Graphics = &OmniSecondTransitionGraphics",
            "Create = &OmniSecondTransitionCreate",
        ):
            if marker not in text:
                errors.append(f"{path}: missing periodic element marker {marker!r}")
    magnesium = read_text(
        root / "src" / "simulation" / "elements" / "MAGN.cpp", errors
    )
    for marker in ("OmniAlkalineEarthMetalUpdate", "OmniMetallurgyMetalUpdate"):
        if marker not in magnesium:
            errors.append(f"MAGN.cpp: missing combined magnesium update marker {marker!r}")
    aluminium = read_text(
        root / "src" / "simulation" / "elements" / "ALUM.cpp", errors
    )
    for marker in ("OmniBoronGroupUpdate", "OmniMetallurgyMetalUpdate"):
        if marker not in aluminium:
            errors.append(f"ALUM.cpp: missing combined aluminium update marker {marker!r}")
    diamond = read_text(
        root / "src" / "simulation" / "elements" / "DMND.cpp", errors
    )
    for marker in ('Identifier = "DEFAULT_PT_DMND"', "Meltable = 0", "HighTemperature = ITH"):
        if marker not in diamond:
            errors.append(f"DMND.cpp: missing inert carbon mapping marker {marker!r}")
    silicon = read_text(
        root / "src" / "simulation" / "elements" / "SLCN.cpp", errors
    )
    if "OmniCarbonGroupUpdate" not in silicon:
        errors.append("SLCN.cpp: shared carbon-group update marker is missing")
    for name in ("TIN", "LEAD"):
        text = read_text(
            root / "src" / "simulation" / "elements" / f"{name}.cpp", errors
        )
        for marker in ("OmniCarbonGroupUpdate", "OmniMetallurgyMetalUpdate"):
            if marker not in text:
                errors.append(f"{name}.cpp: missing combined update marker {marker!r}")
    lead = read_text(
        root / "src" / "simulation" / "elements" / "LEAD.cpp", errors
    )
    if "PROP_NEUTABSORB" not in lead:
        errors.append("LEAD.cpp: neutron-absorption property is missing")
    lava = read_text(root / "src" / "simulation" / "elements" / "LAVA.cpp", errors)
    if "OmniMoltenAlkaliUpdate" not in lava:
        errors.append("LAVA.cpp: molten alkali ctype update hook is missing")
    if "OmniMoltenAlkalineEarthUpdate" not in lava:
        errors.append("LAVA.cpp: molten alkaline-earth ctype update hook is missing")
    if "OmniMoltenBoronGroupUpdate" not in lava:
        errors.append("LAVA.cpp: molten boron-group ctype update hook is missing")
    if "OmniMoltenCarbonGroupUpdate" not in lava:
        errors.append("LAVA.cpp: molten carbon-group ctype update hook is missing")
    if "OmniMoltenNitrogenGroupUpdate" not in lava:
        errors.append("LAVA.cpp: molten nitrogen-group ctype update hook is missing")
    if "OmniMoltenOxygenGroupUpdate" not in lava:
        errors.append("LAVA.cpp: molten oxygen-group ctype update hook is missing")
    if "OmniMoltenHalogenUpdate" not in lava:
        errors.append("LAVA.cpp: molten halogen ctype update hook is missing")
    if "OmniMoltenFirstTransitionUpdate" not in lava:
        errors.append("LAVA.cpp: molten first-transition ctype update hook is missing")
    if "OmniMoltenSecondTransitionUpdate" not in lava:
        errors.append("LAVA.cpp: molten second-transition ctype update hook is missing")
    liquid_nitrogen = read_text(
        root / "src" / "simulation" / "elements" / "LNTG.cpp", errors
    )
    if "HighTemperatureTransition = PT_N" not in liquid_nitrogen:
        errors.append("LNTG.cpp: liquid nitrogen to periodic nitrogen transition is missing")
    oxygen = read_text(root / "src" / "simulation" / "elements" / "O2.cpp", errors)
    if 'Identifier = "DEFAULT_PT_O2"' not in oxygen or "LowTemperatureTransition = PT_LO2" not in oxygen:
        errors.append("O2.cpp: official oxygen mapping or liquid-oxygen transition is missing")
    liquid_oxygen = read_text(
        root / "src" / "simulation" / "elements" / "LO2.cpp", errors
    )
    if "HighTemperatureTransition = PT_O2" not in liquid_oxygen:
        errors.append("LO2.cpp: liquid oxygen to official oxygen transition is missing")
    polonium = read_text(
        root / "src" / "simulation" / "elements" / "POLO.cpp", errors
    )
    for marker in (
        'Identifier = "DEFAULT_PT_POLO"',
        "parts[i].tmp2 >= 10",
        "sim->part_change_type(i,x,y,PT_PLUT)",
    ):
        if marker not in polonium:
            errors.append(f"POLO.cpp: missing official polonium marker {marker!r}")
    chlorine = read_text(
        root / "src" / "simulation" / "elements" / "CHLR.cpp", errors
    )
    for marker in (
        'Identifier = "OMNI_PT_CHLR"',
        "OmniChemistryElementUpdate",
        "OmniHalogenUpdate",
    ):
        if marker not in chlorine:
            errors.append(f"CHLR.cpp: missing combined chlorine marker {marker!r}")
    titanium = read_text(
        root / "src" / "simulation" / "elements" / "TTAN.cpp", errors
    )
    for marker in ('Identifier = "DEFAULT_PT_TTAN"', "HighTemperature = 1941.0f", "bmap_blockair"):
        if marker not in titanium:
            errors.append(f"TTAN.cpp: missing official titanium marker {marker!r}")
    iron = read_text(
        root / "src" / "simulation" / "elements" / "IRON.cpp", errors
    )
    for marker in ('Identifier = "DEFAULT_PT_IRON"', "case PT_SLTW:", "case PT_O2:"):
        if marker not in iron:
            errors.append(f"IRON.cpp: missing official iron marker {marker!r}")
    for name in ("CHRM", "COBT", "NICL", "COPR", "ZINC"):
        text = read_text(
            root / "src" / "simulation" / "elements" / f"{name}.cpp", errors
        )
        for marker in (f'Identifier = "OMNI_PT_{name}"', "OmniMetallurgyMetalUpdate"):
            if marker not in text:
                errors.append(f"{name}.cpp: missing reused transition marker {marker!r}")


def check_ui(root: Path, errors: list[str]) -> None:
    ui_path = root / "src" / "gui" / "periodictable" / "PeriodicTableActivity.cpp"
    ui = read_text(ui_path, errors)
    required = {
        "118-row runtime model": "GetPeriodicTableData()",
        "atomic-number display": "String::Build(atomicNumber)",
        "Chinese search": "record.chineseName",
        "English search": "record.englishName",
        "identifier search": "record.identifier",
        "state filter": "MatchesState",
        "metal-class filter": "MatchesClass",
        "radioactivity filter": "MatchesRadioactivity",
        "expandable f block": "showSeries = !showSeries",
        "planned state": "periodic.status.planned",
        "module-disabled state": "periodic.status.module_disabled",
        "direct selection": "gameController->SetActiveTool(0, tool)",
    }
    for label, marker in required.items():
        if marker not in ui:
            errors.append(f"{ui_path}: missing {label}: {marker!r}")
    controller = read_text(root / "src" / "gui" / "game" / "GameController.cpp", errors)
    view = read_text(root / "src" / "gui" / "game" / "GameView.cpp", errors)
    if "OpenPeriodicTable" not in controller or "PeriodicTableActivity" not in controller:
        errors.append("GameController.cpp: periodic table entry point is missing")
    if "SDL_SCANCODE_T" not in view or "periodic.table.tooltip" not in view:
        errors.append("GameView.cpp: periodic table button or keyboard shortcut is missing")


def audit(root: Path) -> list[str]:
    root = root.resolve()
    errors: list[str] = []
    check_registry(root, errors)
    check_source_map(root, errors)
    check_engine(root, errors)
    check_ui(root, errors)
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
    errors = audit(args.source_root)
    if errors:
        for error in errors:
            print(f"periodic-audit: ERROR {error}", file=sys.stderr)
        print(f"periodic-audit: FAIL ({len(errors)} errors)", file=sys.stderr)
        return 1
    if not args.quiet:
        print("periodic-audit: PASS (118 mapped, 76 implemented, 50 new periodic elements, 1024/frame)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
