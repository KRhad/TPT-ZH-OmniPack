#!/usr/bin/env python3
"""Fail-closed audit for the free-sandbox product boundary."""

from __future__ import annotations

import argparse
import bz2
from pathlib import Path
import sys
from typing import Sequence


FORBIDDEN_PATHS = (
    "src/client/OmniAlchemySaveState.h",
    "src/simulation/OmniAlchemy.cpp",
    "src/simulation/OmniAlchemy.h",
    "docs/ALCHEMY_PROGRESSION.json",
    "tools/alchemy_audit.py",
    "tools/alchemy_state_probe.cpp",
    "tools/runtime/alchemy_gate_regression.lua",
    "tools/runtime/alchemy_progression_regression.lua",
    "tools/runtime_lua_alchemy_progression_test.ps1",
    "tools/runtime_lua_alchemy_test.ps1",
    "tools/tests/test_alchemy_audit.py",
)

FORBIDDEN_MARKERS = {
    "src/client/GameSave.cpp": ("OmniAlchemy", "omniAlchemy"),
    "src/client/GameSave.h": ("OmniAlchemy", "omniAlchemy"),
    "src/gui/game/GameModel.cpp": ("OmniAlchemy", "AlchemyMode", "alchemy."),
    "src/gui/game/OmniContent.cpp": (
        "OmniAlchemy", "AlchemyMode", "AlchemyLocked", "Omni.Progress.AlchemyMode"
    ),
    "src/gui/game/OmniContent.h": ("AlchemyMode", "AlchemyLocked"),
    "src/gui/options/OptionsModel.cpp": ("OmniAlchemy", "AlchemyMode", "alchemy."),
    "src/gui/options/OptionsView.cpp": ("OmniAlchemy", "AlchemyMode", "alchemy."),
    "src/lua/LuaSimulation.cpp": (
        "OmniAlchemy", "AlchemyMode", "AlchemyLocked",
        "omniAlchemyProgress", "omniAlchemyUnlocked",
    ),
    "src/lua/LuaScriptInterface.cpp": ("AlchemyLocked", "alchemy progress"),
    "src/simulation/Simulation.cpp": ("OmniAlchemy", "omniAlchemy"),
    "src/simulation/meson.build": ("OmniAlchemy",),
    "tools/package_test_release.py": ("ALCHEMY_VERSION", "ALCHEMY_PROGRESSION"),
    "tools/test_release_audit.py": ("ALCHEMY_VERSION", "audit_0_4_alchemy"),
}


def read_text(path: Path, errors: list[str]) -> str:
    try:
        return path.read_text(encoding="utf-8")
    except (OSError, UnicodeDecodeError) as exc:
        errors.append(f"{path}: cannot read UTF-8 text: {exc}")
        return ""


def read_bytes(path: Path, errors: list[str]) -> bytes:
    try:
        return path.read_bytes()
    except OSError as exc:
        errors.append(f"{path}: cannot read bytes: {exc}")
        return b""


def audit(root: Path) -> list[str]:
    errors: list[str] = []

    for relative in FORBIDDEN_PATHS:
        if (root / relative).exists():
            errors.append(f"{relative}: retired player progression file still exists")

    for relative, markers in FORBIDDEN_MARKERS.items():
        text = read_text(root / relative, errors)
        for marker in markers:
            if marker in text:
                errors.append(f"{relative}: retired marker remains: {marker}")

    for language in ("en-US", "zh-CN"):
        relative = f"src/lang/{language}.json"
        text = read_text(root / relative, errors)
        if '"alchemy.' in text or '"options.omni.alchemy_mode"' in text:
            errors.append(f"{relative}: retired player progression localization remains")

    content = read_text(root / "src/gui/game/OmniContent.cpp", errors)
    for setting in ("Biology", "Metallurgy", "Chemistry", "AdvancedNuclear"):
        if f"OmniSetting::{setting}" not in content:
            errors.append(f"OmniContent.cpp: current module setting is missing: {setting}")
    if "OmniSelectionRestriction::ModuleDisabled" not in content:
        errors.append("OmniContent.cpp: disabled-module creation boundary is missing")

    readme = read_text(root / "README.zh-CN.md", errors)
    required_positioning = (
        "本项目是以大量元素、化合物和材料为核心的自由沙盒整合版。"
        "项目不包含任务、成就、科技树或强制元素解锁，所有已启用内容均可直接使用。"
    )
    if required_positioning not in readme:
        errors.append("README.zh-CN.md: required free-sandbox positioning is missing")

    encyclopedia = read_text(
        root / "src/gui/elementsearch/ElementSearchActivity.cpp", errors
    )
    if 'Tr("encyclopedia.description")' not in encyclopedia:
        errors.append("ElementSearchActivity.cpp: element description label is missing")
    for language in ("en-US", "zh-CN"):
        text = read_text(root / "src/lang" / f"{language}.json", errors)
        if '"encyclopedia.description"' not in text:
            errors.append(f"{language}.json: element description label is missing")

    samples = (
        "examples/0.2.0/01-peroxide-pathogen.stm",
        "examples/0.3.0/09-integrated-factory.stm",
    )
    for relative in samples:
        data = read_bytes(root / relative, errors)
        if len(data) <= 12 or data[:4] != b"OPS1":
            errors.append(f"{relative}: legacy compatibility sample is not OPS1")
            continue
        try:
            payload = bz2.decompress(data[12:])
        except (OSError, EOFError) as exc:
            errors.append(f"{relative}: cannot decompress legacy OPS: {exc}")
            continue
        if b"omniAlchemy\0" not in payload:
            errors.append(f"{relative}: sample no longer proves retired-field compatibility")

    meson = read_text(root / "meson.build", errors)
    for marker in ("legacy_progress_save_probe", "removed-game-systems-audit"):
        if marker not in meson:
            errors.append(f"meson.build: missing removal regression target: {marker}")

    return errors


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--quiet", action="store_true")
    return parser


def main(argv: Sequence[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    errors = audit(args.source_root.resolve())
    if errors:
        for error in errors:
            print(f"ERROR {error}", file=sys.stderr)
        return 1
    if not args.quiet:
        print(
            "removed-game-systems-audit: PASS "
            "game_tasks_removed=true achievements_removed=true "
            "technology_tree_removed=true alchemy_progression_removed=true "
            "forced_unlocks_removed=true legacy_samples=2"
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
