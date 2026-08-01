#!/usr/bin/env python3
"""Fail-closed checks for disabled-module save loading and read-only protection."""

from __future__ import annotations

import argparse
import json
from pathlib import Path
import sys
from typing import Sequence


def read_text(path: Path, errors: list[str]) -> str:
    try:
        return path.read_text(encoding="utf-8")
    except (OSError, UnicodeDecodeError) as exc:
        errors.append(f"{path}: cannot read UTF-8 text: {exc}")
        return ""


def check_source(root: Path, errors: list[str]) -> None:
    omni = read_text(root / "src" / "gui" / "game" / "OmniContent.cpp", errors)
    controller = read_text(root / "src" / "gui" / "game" / "GameController.cpp", errors)
    prompt = read_text(
        root / "src" / "gui" / "dialogues" / "SaveCompatibilityPrompt.cpp", errors
    )
    if not omni or not controller or not prompt:
        return

    omni_markers = {
        "detector": "FindDisabledOmniSaveModules(GameSave const &save)",
        "direct particle type": "inspectType(type);",
        "carried type metadata": "CarriesTypeIn & (1U << propertyIndex)",
        "packed carried type decoding": "inspectType(TYP(*property));",
        "runtime catalog identity guard": 'record->identifier.starts_with("OMNI_PT_")',
        "disabled module check": (
            "restriction == OmniSelectionRestriction::ModuleDisabled"
        ),
        "compatibility alias module check": (
            "restriction == OmniSelectionRestriction::CompatibilityAlias"
        ),
        "recoverable official carrier": "IsOmniRecoverableScrap(particle)",
    }
    for label, marker in omni_markers.items():
        if marker not in omni:
            errors.append(f"OmniContent.cpp: missing {label}: {marker!r}")

    controller_markers = {
        "load gate": "void GameController::RequestSaveLoad(",
        "file load gate": "RequestSaveLoad(*save, [this, pendingFile]",
        "server load gate": "RequestSaveLoad(*(*pendingSave)->GetGameSave()",
        "search routing": "LoadSave(search->TakeLoadedSave());",
        "read-only local save guard": "void GameController::OpenLocalSaveWindow(bool asCurrent)\n{\n\tif (readOnlySave)",
        "read-only upload guard": "void GameController::OpenSaveWindow()\n{\n\tif (readOnlySave)",
        "read-only update guard": "void GameController::SaveAsCurrent()\n{\n\tif (readOnlySave)",
        "clear reset": "HistorySnapshot();\n\treadOnlySave = false;",
    }
    for label, marker in controller_markers.items():
        if marker not in controller:
            errors.append(f"GameController.cpp: missing {label}: {marker!r}")

    prompt_markers = {
        "cancel": 'Localization::Ref().Tr("dialog.cancel")',
        "read-only choice": 'Localization::Ref().Tr("gamecontroller.load_read_only")',
        "normal choice": 'Localization::Ref().Tr("gamecontroller.load_normally")',
    }
    for label, marker in prompt_markers.items():
        if marker not in prompt:
            errors.append(f"SaveCompatibilityPrompt.cpp: missing {label}: {marker!r}")


def check_localization(root: Path, errors: list[str]) -> None:
    required = {
        "gamecontroller.disabled_modules_title",
        "gamecontroller.disabled_modules_message_prefix",
        "gamecontroller.disabled_modules_message_suffix",
        "gamecontroller.load_normally",
        "gamecontroller.load_read_only",
        "gamecontroller.read_only_save",
    }
    for filename in ("en-US.json", "zh-CN.json"):
        path = root / "src" / "lang" / filename
        text = read_text(path, errors)
        if not text:
            continue
        try:
            catalog = json.loads(text)
        except json.JSONDecodeError as exc:
            errors.append(f"{path}: invalid JSON: {exc}")
            continue
        missing = sorted(required - catalog.keys())
        if missing:
            errors.append(f"{path}: missing save compatibility keys: {', '.join(missing)}")


def audit(root: Path) -> list[str]:
    errors: list[str] = []
    check_source(root, errors)
    check_localization(root, errors)
    return errors


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Audit disabled-module save warnings and read-only save protection."
    )
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
            print(f"save-compatibility-audit: ERROR {error}", file=sys.stderr)
        print(f"save-compatibility-audit: FAIL ({len(errors)} errors)", file=sys.stderr)
        return 1
    if not args.quiet:
        print("save-compatibility-audit: PASS (gated load, carried types, read-only saves)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
