#!/usr/bin/env python3
"""Fail-closed audit for periodic-table runtime data and the first noble-gas batch."""

from __future__ import annotations

import argparse
import csv
from pathlib import Path
import re
import sys
from typing import Sequence


EXPECTED_NEW_ELEMENTS = {
    370: "HE",
    375: "NE",
    379: "AR",
    390: "KR",
    405: "XE",
    431: "RN",
    461: "OG",
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
    if len(implemented) != 33:
        errors.append(f"{path}: expected 33 implemented mappings after batch 1 and found {len(implemented)}")
    by_number = {int(row["atomic_number"]): row for row in rows}
    if by_number.get(1, {}).get("stable_id") != "148":
        errors.append(f"{path}: hydrogen must continue to reuse stable ID 148")
    expected_atomic = {2: 370, 10: 375, 18: 379, 36: 390, 54: 405, 86: 431, 118: 461}
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
        "finite discharge photon": "parts[photon].life = 24",
        "finite decay photon": "parts[photon].life = 18",
    }
    for label, marker in required.items():
        if marker not in engine:
            errors.append(f"{engine_path}: missing {label}: {marker!r}")
    if re.search(r"\bNPART\b|parts\.active", engine):
        errors.append(f"{engine_path}: periodic family update must not scan all particles")
    for stable_id, name in EXPECTED_NEW_ELEMENTS.items():
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
        print("periodic-audit: PASS (118 mapped, 33 implemented, 7 new noble gases, 1024/frame)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
