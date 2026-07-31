#!/usr/bin/env python3
"""Fail-closed audit for the versioned 0.4 alchemy progression graph."""

from __future__ import annotations

import argparse
import csv
import json
from pathlib import Path
import re
import sys
from typing import Any, Sequence


REQUIRED_INITIAL = {
    "DEFAULT_PT_FIRE",
    "DEFAULT_PT_WATR",
    "DEFAULT_PT_STNE",
    "DEFAULT_PT_O2",
}
REQUIRED_CONDITIONS = {
    "temperature",
    "pressure",
    "electric",
    "catalyst",
    "time",
    "structure",
    "cooling",
    "filtering",
    "multi_stage",
}
ALLOWED_MODULES = {"metallurgy", "biology", "nuclear", "chemistry"}
IDENTIFIER_PATTERN = re.compile(r"^(?:DEFAULT|OMNI)_PT_[A-Z0-9]+$")
EXPECTED_MODE_POLICY = {
    "default_enabled": False,
    "normal_sandbox_behavior": "all_implemented_elements_selectable",
    "activation_preference": "Omni.Progress.AlchemyMode",
    "progress_storage": "OPS.omniAlchemy",
    "paste_imports_progress": False,
    "locked_lua_behavior": "reject_without_mutation",
    "module_disabled_behavior": "complete_stage_and_preserve_hidden_unlocks",
    "corrupt_progress_behavior": "reset_to_initial_without_modifying_source_save",
}
EXPECTED_BOUNDS = {
    "stage_count": 10,
    "scan_interval_frames": 30,
    "max_particles_per_frame": 8192,
    "max_saved_identifiers": 512,
    "max_saved_stages": 10,
    "max_saved_records": 64,
    "max_identifier_bytes": 64,
    "max_stage_id_bytes": 32,
}


def read_json(path: Path, errors: list[str]) -> dict[str, Any]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, UnicodeDecodeError, json.JSONDecodeError) as exc:
        errors.append(f"{path}: cannot read valid UTF-8 JSON: {exc}")
        return {}
    if not isinstance(value, dict):
        errors.append(f"{path}: top-level value must be an object")
        return {}
    return value


def read_text(path: Path, errors: list[str]) -> str:
    try:
        return path.read_text(encoding="utf-8")
    except (OSError, UnicodeDecodeError) as exc:
        errors.append(f"{path}: cannot read UTF-8 text: {exc}")
        return ""


def read_registry(path: Path, errors: list[str]) -> dict[str, dict[str, str]]:
    try:
        with path.open("r", encoding="utf-8", newline="") as stream:
            rows = list(csv.DictReader(stream))
    except (OSError, UnicodeDecodeError, csv.Error) as exc:
        errors.append(f"{path}: cannot read registry: {exc}")
        return {}
    result: dict[str, dict[str, str]] = {}
    for line_number, row in enumerate(rows, start=2):
        identifier = row.get("identifier", "")
        if not identifier:
            errors.append(f"{path}:{line_number}: missing identifier")
        elif identifier in result:
            errors.append(f"{path}:{line_number}: duplicate identifier {identifier}")
        else:
            result[identifier] = row
    return result


def identifier_fields(condition: dict[str, Any]) -> list[str]:
    values: list[str] = []
    for key in ("target", "heat_target", "cool_target", "medium", "fluid"):
        value = condition.get(key)
        if isinstance(value, str):
            values.append(value)
    targets = condition.get("targets")
    if isinstance(targets, list):
        values.extend(value for value in targets if isinstance(value, str))
    return values


def validate_condition(
    path: Path,
    stage_id: str,
    condition: Any,
    known_identifiers: set[str],
    errors: list[str],
) -> str | None:
    if not isinstance(condition, dict):
        errors.append(f"{path}: {stage_id} condition must be an object")
        return None
    kind = condition.get("type")
    if kind not in REQUIRED_CONDITIONS:
        errors.append(f"{path}: {stage_id} has unknown condition type {kind!r}")
        return None
    for identifier in identifier_fields(condition):
        if identifier not in known_identifiers:
            errors.append(f"{path}: {stage_id} condition references unknown identifier {identifier!r}")
    positive_fields = {
        "temperature": ("minimum_kelvin",),
        "pressure": ("minimum_absolute_pressure",),
        "electric": ("minimum_count",),
        "catalyst": ("minimum_count",),
        "time": ("continuous_frames",),
        "structure": ("maximum_radius",),
        "cooling": ("minimum_kelvin", "maximum_kelvin"),
        "filtering": ("minimum_count_each",),
        "multi_stage": (),
    }[kind]
    for field in positive_fields:
        value = condition.get(field)
        if not isinstance(value, (int, float)) or isinstance(value, bool) or value <= 0:
            errors.append(f"{path}: {stage_id} {kind}.{field} must be positive")
    if kind == "structure":
        targets = condition.get("targets")
        if not isinstance(targets, list) or len(targets) < 2 or len(targets) != len(set(targets)):
            errors.append(f"{path}: {stage_id} structure needs at least two unique targets")
    if kind == "multi_stage":
        requires = condition.get("requires")
        if not isinstance(requires, list) or not requires or not all(isinstance(value, str) for value in requires):
            errors.append(f"{path}: {stage_id} multi_stage.requires must be a non-empty string list")
    return kind


def validate_document(
    root: Path,
    document: dict[str, Any],
    registry: dict[str, dict[str, str]],
    languages: dict[str, dict[str, Any]],
) -> list[str]:
    path = root / "docs" / "ALCHEMY_PROGRESSION.json"
    errors: list[str] = []
    if document.get("schema_version") != 1 or document.get("content_version") != "0.4.0-dev":
        errors.append(f"{path}: expected schema_version=1 and content_version=0.4.0-dev")
    progress = document.get("progress_schema")
    if progress != {
        "current": 1,
        "minimum_supported": 1,
        "migration_policy": "identifier_only",
        "unknown_schema_behavior": "reset_to_initial_without_modifying_source_save",
    }:
        errors.append(f"{path}: progress_schema differs from identifier-only schema 1 contract")
    if document.get("mode_policy") != EXPECTED_MODE_POLICY:
        errors.append(f"{path}: mode_policy differs from the fail-closed mode contract")
    if document.get("bounds") != EXPECTED_BOUNDS:
        errors.append(f"{path}: bounds differ from the audited limits")

    declared_conditions = document.get("required_condition_types")
    if not isinstance(declared_conditions, list) or set(declared_conditions) != REQUIRED_CONDITIONS:
        errors.append(f"{path}: required_condition_types must declare the exact nine-condition set")
    initial = document.get("initial_identifiers")
    if not isinstance(initial, list) or set(initial) != REQUIRED_INITIAL or len(initial) != len(REQUIRED_INITIAL):
        errors.append(f"{path}: initial_identifiers must be exactly FIRE/WATR/STNE/O2")

    known_identifiers = {
        identifier
        for identifier, row in registry.items()
        if row.get("implementation_status") == "implemented"
    }
    for identifier in initial if isinstance(initial, list) else []:
        if not isinstance(identifier, str) or not IDENTIFIER_PATTERN.fullmatch(identifier):
            errors.append(f"{path}: initial value {identifier!r} is not a stable identifier")
        elif identifier not in known_identifiers:
            errors.append(f"{path}: initial identifier {identifier} is not implemented")

    stages = document.get("stages")
    if not isinstance(stages, list):
        errors.append(f"{path}: stages must be an array")
        stages = []
    if len(stages) != 10:
        errors.append(f"{path}: expected exactly ten stages, found {len(stages)}")
    stage_ids: set[str] = set()
    available = set(REQUIRED_INITIAL)
    covered_conditions: set[str] = set()
    output_owner: dict[str, str] = {}
    localization_keys: set[str] = set()

    for expected_order, stage in enumerate(stages, start=1):
        if not isinstance(stage, dict):
            errors.append(f"{path}: stages[{expected_order - 1}] must be an object")
            continue
        stage_id = stage.get("id")
        if not isinstance(stage_id, str) or not stage_id or len(stage_id.encode("utf-8")) > EXPECTED_BOUNDS["max_stage_id_bytes"]:
            errors.append(f"{path}: stage {expected_order} has invalid id")
            stage_id = f"<invalid-{expected_order}>"
        elif stage_id in stage_ids:
            errors.append(f"{path}: duplicate stage id {stage_id}")
        stage_ids.add(stage_id)
        if stage.get("order") != expected_order:
            errors.append(f"{path}: {stage_id} order must be {expected_order}")
        for key_field in ("name_key", "hint_key", "notice_key"):
            key = stage.get(key_field)
            if not isinstance(key, str) or not key:
                errors.append(f"{path}: {stage_id} missing {key_field}")
            else:
                localization_keys.add(key)
        modules = stage.get("module_dependencies")
        if not isinstance(modules, list) or len(modules) != len(set(modules)) or set(modules) - ALLOWED_MODULES:
            errors.append(f"{path}: {stage_id} has invalid module_dependencies")

        inputs = stage.get("inputs")
        outputs = stage.get("outputs")
        if not isinstance(inputs, list) or not inputs or len(inputs) != len(set(inputs)):
            errors.append(f"{path}: {stage_id} needs unique non-empty inputs")
            inputs = []
        if not isinstance(outputs, list) or not outputs or len(outputs) != len(set(outputs)):
            errors.append(f"{path}: {stage_id} needs unique non-empty outputs")
            outputs = []
        for role, identifiers in (("input", inputs), ("output", outputs)):
            for identifier in identifiers:
                if not isinstance(identifier, str) or not IDENTIFIER_PATTERN.fullmatch(identifier):
                    errors.append(f"{path}: {stage_id} {role} {identifier!r} is not a stable identifier")
                elif identifier not in known_identifiers:
                    errors.append(f"{path}: {stage_id} {role} {identifier} is not implemented")
        unreachable = set(value for value in inputs if isinstance(value, str)) - available
        if unreachable:
            errors.append(f"{path}: {stage_id} has unreachable inputs {sorted(unreachable)!r}")
        for output in outputs:
            if isinstance(output, str) and output in output_owner:
                errors.append(f"{path}: {stage_id} repeats output {output} from {output_owner[output]}")
            elif isinstance(output, str):
                output_owner[output] = stage_id

        conditions = stage.get("conditions")
        if not isinstance(conditions, list) or not conditions:
            errors.append(f"{path}: {stage_id} needs conditions")
            conditions = []
        stage_condition_types: set[str] = set()
        for condition in conditions:
            kind = validate_condition(path, stage_id, condition, known_identifiers, errors)
            if kind:
                if kind in stage_condition_types:
                    errors.append(f"{path}: {stage_id} repeats condition type {kind}")
                stage_condition_types.add(kind)
                covered_conditions.add(kind)
                if kind == "multi_stage":
                    for dependency in condition.get("requires", []):
                        if dependency not in stage_ids - {stage_id}:
                            errors.append(f"{path}: {stage_id} has forward or unknown dependency {dependency!r}")
        if stage.get("failure_behavior") not in {
            "retain_progress_no_unlock",
            "complete_and_record_official_outputs_when_module_disabled",
        }:
            errors.append(f"{path}: {stage_id} lacks a fail-closed failure behavior")
        available.update(value for value in outputs if isinstance(value, str))

    if covered_conditions != REQUIRED_CONDITIONS:
        errors.append(
            f"{path}: condition coverage differs; missing={sorted(REQUIRED_CONDITIONS - covered_conditions)!r}, "
            f"extra={sorted(covered_conditions - REQUIRED_CONDITIONS)!r}"
        )
    completion = document.get("completion")
    if not isinstance(completion, dict) or completion.get("requires_stage") != "A10-MASTERY-LOOP" or completion.get("grant_policy") != "all_implemented_selectable_identifiers":
        errors.append(f"{path}: completion must be gated by A10 and grant all implemented selectable identifiers")
    elif isinstance(completion.get("notice_key"), str):
        localization_keys.add(completion["notice_key"])

    for language_name, catalog in languages.items():
        for key in sorted(localization_keys):
            value = catalog.get(key)
            if not isinstance(value, str) or not value.strip():
                errors.append(f"src/lang/{language_name}.json: missing non-empty {key}")
    return errors


def check_runtime_contract(root: Path, document: dict[str, Any], errors: list[str]) -> None:
    service_path = root / "src" / "simulation" / "OmniAlchemy.cpp"
    service = read_text(service_path, errors)
    header = read_text(root / "src" / "simulation" / "OmniAlchemy.h", errors)
    save_header = read_text(root / "src" / "client" / "OmniAlchemySaveState.h", errors)
    game_save = read_text(root / "src" / "client" / "GameSave.cpp", errors)
    simulation = read_text(root / "src" / "simulation" / "Simulation.cpp", errors)
    content = read_text(root / "src" / "gui" / "game" / "OmniContent.cpp", errors)
    model = read_text(root / "src" / "gui" / "game" / "GameModel.cpp", errors)
    options_view = read_text(root / "src" / "gui" / "options" / "OptionsView.cpp", errors)
    lua_simulation = read_text(root / "src" / "lua" / "LuaSimulation.cpp", errors)
    lua_interface = read_text(root / "src" / "lua" / "LuaScriptInterface.cpp", errors)
    probe = read_text(root / "tools" / "alchemy_state_probe.cpp", errors)
    gate_lua = read_text(root / "tools" / "runtime" / "alchemy_gate_regression.lua", errors)
    progression_lua = read_text(root / "tools" / "runtime" / "alchemy_progression_regression.lua", errors)
    gate_wrapper = read_text(root / "tools" / "runtime_lua_alchemy_test.ps1", errors)
    progression_wrapper = read_text(root / "tools" / "runtime_lua_alchemy_progression_test.ps1", errors)
    meson = read_text(root / "meson.build", errors)

    stages = document.get("stages", [])
    if isinstance(stages, list):
        for index, stage in enumerate(stages):
            if not isinstance(stage, dict) or not isinstance(stage.get("id"), str):
                continue
            stage_id = stage["id"]
            marker = f'.id = "{stage_id}"'
            start = service.find(marker)
            end = service.find("\n\t{", start + len(marker)) if start >= 0 and index + 1 < len(stages) else len(service)
            if start < 0:
                errors.append(f"{service_path}: missing runtime stage {stage_id}")
                continue
            block = service[start:end]
            for key_field, code_field in (
                ("notice_key", "noticeKey"),
                ("hint_key", "hintKey"),
                ("name_key", "nameKey"),
            ):
                value = stage.get(key_field)
                if isinstance(value, str) and f'.{code_field} = "{value}"' not in block:
                    errors.append(f"{service_path}: {stage_id} runtime {code_field} differs from JSON")
            identifiers: list[str] = []
            for field in ("inputs", "outputs"):
                values = stage.get(field, [])
                if isinstance(values, list):
                    identifiers.extend(value for value in values if isinstance(value, str))
            for condition in stage.get("conditions", []):
                if isinstance(condition, dict):
                    identifiers.extend(identifier_fields(condition))
            for identifier in set(identifiers):
                runtime_name = "PT_" + identifier.split("_PT_", 1)[1]
                if runtime_name not in block:
                    errors.append(f"{service_path}: {stage_id} missing runtime identifier {runtime_name}")

    for marker in (
        "constexpr int AlchemyScanIntervalFrames = 30;",
        "constexpr int AlchemyScanParticleBudget = 8192;",
        "completedStages.size() == stages.size()",
        'pendingNoticeKeys.emplace_back("alchemy.mastery.notice")',
    ):
        if marker not in service:
            errors.append(f"{service_path}: missing bounded progression marker {marker!r}")
    for marker in (
        "PositionSamplesPerType = 8",
        "OmniAlchemySaveState Export() const",
        "bool Observe(Simulation const &simulation)",
    ):
        if marker not in header:
            errors.append(f"src/simulation/OmniAlchemy.h: missing {marker!r}")
    for marker in (
        "OmniAlchemyProgressSchemaVersion = 1",
        "OmniAlchemyMaxSavedIdentifiers = 512",
        "OmniAlchemyMaxSavedRecords = 64",
        "std::array<int, OmniAlchemyStageCount> dwellFrames",
        "std::array<bool, OmniAlchemyStageCount> coolingArmed",
    ):
        if marker not in save_header:
            errors.append(f"src/client/OmniAlchemySaveState.h: missing {marker!r}")
    for marker in (
        '"schema", "unlocked", "completed", "records", "dwellFrames", "coolingArmed"',
        "object.size() != knownKeys.size()",
        "Invalid omniAlchemy progress; using fail-closed initial state",
        'b["omniAlchemy"] = Bson::Type::objectValue',
    ):
        if marker not in game_save:
            errors.append(f"src/client/GameSave.cpp: missing strict persistence marker {marker!r}")
    if "gameSave.omniAlchemy = OmniAlchemy::Ref().Export();" not in simulation:
        errors.append("src/simulation/Simulation.cpp: alchemy progress is not saved with OPS")

    content_markers = (
        "OmniSetting::AlchemyMode",
        "false, true",
        "OmniSelectionRestriction::ModuleDisabled",
        "OmniSelectionRestriction::AlchemyLocked",
        "GetOmniSetting(OmniSetting::AlchemyMode)",
        "OmniAlchemy::Ref().IsElementUnlocked(elementId)",
        "FindLockedAlchemySaveElements",
    )
    for marker in content_markers:
        if marker not in content:
            errors.append(f"src/gui/game/OmniContent.cpp: missing unified gate marker {marker!r}")
    for marker in (
        "OmniAlchemy::Ref().Import(saveData.omniAlchemy);",
        "OmniAlchemy::Ref().Reset();",
        "OmniAlchemy::Ref().Observe(*sim)",
        "RefreshOmniContentSettings();",
        "Paste rejected because it contains elements locked by current alchemy progress",
    ):
        if marker not in model:
            errors.append(f"src/gui/game/GameModel.cpp: missing load/reset/observe marker {marker!r}")
    for marker in (
        'Localization::Ref().Tr("alchemy.progress.button")',
        "OmniAlchemyStageCount",
        "alchemy.StageNameKey(index)",
        "state.records",
        'Localization::Ref().Tr("alchemy.progress.current_hint_prefix")',
    ):
        if marker not in options_view:
            errors.append(f"src/gui/options/OptionsView.cpp: missing progress view marker {marker!r}")

    for function_name in (
        "partChangeType", "partCreate", "createParts", "createLine", "createBox", "floodParts", "loadStamp",
    ):
        if f"static int {function_name}(lua_State *L)" not in lua_simulation:
            errors.append(f"src/lua/LuaSimulation.cpp: missing guarded function {function_name}")
    for marker in (
        "RequireOmniElementCreation",
        "RequireOmniTool",
        "FindLockedAlchemySaveElements(*gameSave)",
        "omniAlchemyProgress",
        "omniAlchemyUnlocked",
    ):
        if marker not in lua_simulation:
            errors.append(f"src/lua/LuaSimulation.cpp: missing no-bypass marker {marker!r}")
    if 'property.Name == "type"' not in lua_interface or "IsOmniElementCreationAllowed" not in lua_interface:
        errors.append("src/lua/LuaScriptInterface.cpp: partProperty(type) lacks the unified gate")

    evidence_markers = {
        "tools/alchemy_state_probe.cpp": (
            "stage_roundtrip=10", "repeated_loads=100", "serialise_parse_import=100",
            "iterationSave.Serialise().second", "GameSave parsedProgress(serialisedProgress)",
            "alchemy.Import(parsedProgress.omniAlchemy)", "multi_save_isolation=true",
            "corrupt_fail_closed=true", "unknown_schema_rejected=true", "mastery=true",
        ),
        "tools/runtime/alchemy_gate_regression.lua": (
            "partCreate", "partChangeType", "partProperty(type)", "createParts", "createLine",
            "createBox", "floodParts", "toolBrush", "toolLine", "toolBox", "loadStamp",
            "DEFAULT_PT_DUST", "DEFAULT_PT_FIRE",
        ),
        "tools/runtime/alchemy_progression_regression.lua": (
            "stage_1()", "stage_10()", "OMNI_ALCHEMY_STAGES=10", "OMNI_ALCHEMY_MASTERY=true",
        ),
        "tools/runtime_lua_alchemy_test.ps1": (
            "GITHUB_PAT_TOKEN", "$process.Dispose()", "gate_modes=2", "assertions=22",
        ),
        "tools/runtime_lua_alchemy_progression_test.ps1": (
            "GITHUB_PAT_TOKEN", "$process.Dispose()", "OMNI_ALCHEMY_PROGRESSION_STATUS=PASS",
        ),
    }
    texts = {
        "tools/alchemy_state_probe.cpp": probe,
        "tools/runtime/alchemy_gate_regression.lua": gate_lua,
        "tools/runtime/alchemy_progression_regression.lua": progression_lua,
        "tools/runtime_lua_alchemy_test.ps1": gate_wrapper,
        "tools/runtime_lua_alchemy_progression_test.ps1": progression_wrapper,
    }
    for relative, markers in evidence_markers.items():
        for marker in markers:
            if marker not in texts[relative]:
                errors.append(f"{relative}: missing evidence marker {marker!r}")
    for marker in (
        "alchemy_state_probe = executable(",
        "'alchemy-audit'",
        "'alchemy-state-probe'",
    ):
        if marker not in meson:
            errors.append(f"meson.build: missing alchemy static-test marker {marker!r}")


def audit(root: Path) -> list[str]:
    errors: list[str] = []
    document = read_json(root / "docs" / "ALCHEMY_PROGRESSION.json", errors)
    registry = read_registry(root / "docs" / "ELEMENT_REGISTRY.csv", errors)
    languages = {
        name: read_json(root / "src" / "lang" / f"{name}.json", errors)
        for name in ("en-US", "zh-CN")
    }
    errors.extend(validate_document(root, document, registry, languages))
    check_runtime_contract(root, document, errors)
    return errors


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Audit the versioned 0.4 alchemy graph and localization contract.")
    parser.add_argument("--source-root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--quiet", action="store_true")
    return parser


def main(argv: Sequence[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    root = args.source_root.resolve()
    errors = audit(root)
    if errors:
        for error in errors:
            print(f"alchemy-audit: ERROR {error}", file=sys.stderr)
        print(f"alchemy-audit: FAIL ({len(errors)} errors)", file=sys.stderr)
        return 1
    if not args.quiet:
        print("alchemy-audit: PASS (schema=1, stages=10, conditions=9, initial=4, graph_acyclic=true)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
