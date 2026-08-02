#!/usr/bin/env python3
"""Fail-closed audit for OmniPack display codes, prose and gas rendering."""

from __future__ import annotations

import argparse
import csv
import json
from pathlib import Path
import re
import sys
from typing import Sequence


DISPLAY_CODE = re.compile(r"^[A-Z]{3,4}$")
FORBIDDEN_STYLE = re.compile(
    r"\b(?:bounded|proxy|game-scaled|gameplay|global scan|event budget|"
    r"stable-ID|typed scrap|compatibility alias|this batch|first batch|"
    r"future work|periodic carbon mapping|canonical|four-module|"
    r"cross-module|registered route|documented follow-up|simplified model|"
    r"no unlock(?:s|ing)?|direct(?:ly)? select(?:able|ion)?|"
    r"place(?:d)? directly|direct placement|directly place(?:able)?|"
    r"direct sandbox placement|unrestricted sandbox|production prerequisite|"
    r"available (?:immediately|now|by default)|default(?:ly)? available)\b"
    r"|有界|游戏化|代理|玩法|全图扫描|事件预算|稳定 ID|带类型碎料|"
    r"兼容别名|本批|第一批|后续工作|周期表碳的映射|简化规则|"
    r"规范|四模块|跨模块|登记路线|简化模型|无需解锁|直接选择|"
    r"(?:可|仅可)?直接放置|默认可用|默认开放|现在可直接使用",
    re.IGNORECASE,
)
DIRECT_GAS_GRAPHICS = {
    "ACTY": "Graphics = &OmniGasGraphics",
    "AMON": "Graphics = &OmniGasGraphics",
    "COMO": "Graphics = &OmniGasGraphics",
    "H2SG": "Graphics = &OmniGasGraphics",
    "HYCN": "Graphics = &OmniGasGraphics",
    "NIMO": "Graphics = &OmniGasGraphics",
    "NODI": "Graphics = &OmniGasGraphics",
    "SODI": "Graphics = &OmniGasGraphics",
    "SUTR": "Graphics = &OmniGasGraphics",
    "CHLR": "Graphics = &OmniHalogenGraphics",
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
    elements_dir = root / "src" / "simulation" / "elements"
    source_codes: dict[str, str] = {}
    owners: dict[str, str] = {}
    for path in sorted(elements_dir.glob("*.cpp")):
        text = read_text(path, errors)
        identifier = re.search(r'Identifier\s*=\s*"(OMNI_PT_[A-Z0-9]+)"', text)
        if not identifier:
            continue
        name = re.search(r'Name\s*=\s*"([^"]+)"', text)
        if not name:
            errors.append(f"{path}: OmniPack element lacks a literal Name assignment")
            continue
        code = name.group(1)
        source_codes[identifier.group(1)] = code
        if not DISPLAY_CODE.fullmatch(code):
            errors.append(f"{path}: Name {code!r} must be three or four uppercase letters")
        if code in owners:
            errors.append(f"{path}: Name {code!r} duplicates {owners[code]}")
        owners[code] = path.name

    registry_path = root / "docs" / "ELEMENT_REGISTRY.csv"
    registry = read_csv(registry_path, errors)
    active = [
        row for row in registry
        if row.get("identifier", "").startswith("OMNI_PT_")
        and row.get("implementation_status") == "implemented"
    ]
    for row in active:
        identifier = row["identifier"]
        code = row.get("display_code", "")
        if not DISPLAY_CODE.fullmatch(code):
            errors.append(f"{registry_path}: {identifier} has invalid display code {code!r}")
        if row.get("code") != code or source_codes.get(identifier) != code:
            errors.append(f"{registry_path}: {identifier} display code is not synchronized")
        for field in ("english_description", "chinese_description"):
            if FORBIDDEN_STYLE.search(row.get(field, "")):
                errors.append(f"{registry_path}: {identifier} {field} contains editorial wording")

    game_view = read_text(root / "src" / "gui" / "game" / "GameView.cpp", errors)
    for marker in (
        "ui::Point(WINDOWW-16, WINDOWH-32)",
        "ui::Point(WINDOWW-16, WINDOWH-48)",
        "int currentY = WINDOWH-64;",
        "int newInitialX = WINDOWW - 56;",
        "Vec2(RES.X - 1, 18)",
        "RemoveComponent(simulationOptionButton);",
        "AddComponent(simulationOptionButton);",
        "RemoveComponent(displayModeButton);",
        "AddComponent(displayModeButton);",
        "RemoveComponent(pauseButton);",
        "AddComponent(pauseButton);",
        "RemoveComponent(periodicTableButton);",
        "AddComponent(periodicTableButton);",
        "RemoveComponent(elementSearchButton);",
        "AddComponent(elementSearchButton);",
        "periodicTableButton->SetIcon(IconPeriodicTable);",
    ):
        if marker not in game_view:
            errors.append(f"GameView.cpp: missing lower-toolbar clearance marker {marker!r}")
    if "((newInitialX - (WINDOWW - 56)) / buttonStride) * buttonStride" in game_view:
        errors.append("GameView.cpp: obsolete periodic-button scroll snapping is still present")

    gas = read_text(root / "src" / "simulation" / "OmniGasGraphics.cpp", errors)
    for marker in (
        "*pixel_mode &= ~PMODE;",
        "FIRE_BLEND | DECO_FIRE",
        "*firer = *colr / 2",
        "*fireg = *colg / 2",
        "*fireb = *colb / 2",
        "*firea = 125",
    ):
        if marker not in gas:
            errors.append(f"OmniGasGraphics.cpp: missing soft-gas rendering marker {marker!r}")
    for meson, marker in DIRECT_GAS_GRAPHICS.items():
        text = read_text(elements_dir / f"{meson}.cpp", errors)
        if marker not in text:
            errors.append(f"{meson}.cpp: missing gas graphics assignment {marker!r}")
    shared_markers = {
        root / "src" / "simulation" / "OmniOrganics.cpp": "element.Graphics = &OmniGasGraphics",
        root / "src" / "simulation" / "OmniPeriodic.cpp": "OmniGasGraphics(GRAPHICS_FUNC_SUBCALL_ARGS)",
        root / "src" / "simulation" / "OmniIsotopes.cpp": "OmniGasGraphics(GRAPHICS_FUNC_SUBCALL_ARGS)",
    }
    for path, marker in shared_markers.items():
        if marker not in read_text(path, errors):
            errors.append(f"{path}: missing shared gas rendering call {marker!r}")

    icons = read_text(root / "src" / "graphics" / "Icons.h", errors)
    graphics = read_text(root / "src" / "graphics" / "Graphics.cpp", errors)
    if "IconPeriodicTable" not in icons:
        errors.append("Icons.h: missing IconPeriodicTable")
    if "case IconPeriodicTable:" not in graphics:
        errors.append("Graphics.cpp: missing periodic-table icon renderer")
    for marker in (
        "x + column - 1",
        "y + row + 3",
    ):
        if marker not in graphics:
            errors.append(f"Graphics.cpp: missing centred periodic-table icon marker {marker!r}")
    if re.search(r'periodicTableButton\s*=\s*new ui::Button\([\s\S]*?WINDOWH-48[\s\S]*?,\s*"P"\s*,', game_view):
        errors.append("GameView.cpp: periodic-table shortcut still uses the letter P")

    for locale in ("en-US", "zh-CN"):
        path = root / "src" / "lang" / f"{locale}.json"
        try:
            language = json.loads(path.read_text(encoding="utf-8"))
        except (OSError, UnicodeDecodeError, json.JSONDecodeError) as exc:
            errors.append(f"{path}: invalid localization JSON: {exc}")
            continue
        for key, value in language.items():
            if not isinstance(value, str):
                continue
            if (
                key.startswith("sim.elem.OMNI_PT_")
                or key.startswith("periodic.")
                or key in {
                    "sim.elem.DEFAULT_PT_SLCN",
                    "options.omni.chemistry.info",
                    "options.omni.electronics.info",
                }
            ) and FORBIDDEN_STYLE.search(value):
                errors.append(f"{path}: {key} contains editorial wording")

    content_path = root / "docs" / "ELEMENT_CONTENT.csv"
    for row in read_csv(content_path, errors):
        for field, value in row.items():
            if field != "identifier" and FORBIDDEN_STYLE.search(value or ""):
                errors.append(f"{content_path}: {row.get('identifier')} {field} contains editorial wording")
    return errors


def main(argv: Sequence[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--quiet", action="store_true")
    args = parser.parse_args(argv)
    errors = audit(args.source_root.resolve())
    if errors:
        for error in errors:
            print(f"omni-ui-style-audit: ERROR {error}", file=sys.stderr)
        print(f"omni-ui-style-audit: FAIL ({len(errors)} errors)", file=sys.stderr)
        return 1
    if not args.quiet:
        print("omni-ui-style-audit: PASS (three/four-letter codes, prose, toolbar icon and gas rendering)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
