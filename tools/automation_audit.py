#!/usr/bin/env python3
"""Fail-closed audit for the official-element 0.3 automation design."""

from __future__ import annotations

import argparse
import csv
import hashlib
import json
from pathlib import Path
import sys
from typing import Any, Sequence


REQUIRED_CAPABILITIES = {
    "sensor",
    "filtering",
    "delay",
    "counting",
    "storage",
    "valve",
    "feeding",
    "waste_discharge",
    "shutdown",
    "alarm",
    "interlock",
}

REQUIRED_SCENARIOS = {
    "thermostatic-furnace",
    "automatic-alloy",
    "fuel-control",
    "nutrient-dosing",
    "pathogen-disinfection",
    "reactor-cooling",
    "emergency-stop",
    "waste-transfer",
    "integrated-factory",
}

REQUIRED_CHALLENGES = {
    "A01-TEMPERATURE",
    "A02-FEEDING",
    "A03-SORTING",
    "A04-DISINFECTION",
    "A05-EMERGENCY-STOP",
    "A06-MULTI-MODULE",
}

REQUIRED_METRICS = {
    "frames",
    "input_events",
    "output_events",
    "peak_events_per_frame",
    "blocked_events",
    "stop_event_delta",
    "recovery_assertions",
}

MODULES = {"metallurgy", "biology", "nuclear", "chemistry"}
LEGACY_AUTOMATION_IDS = range(392, 424)

EXPECTED_POLICY: dict[str, Any] = {
    "normal_save_load_path_only": True,
    "read_only_bypass_allowed": False,
    "module_gate_bypass_allowed": False,
    "official_behavior_mutation_allowed": False,
    "new_automation_elements": 0,
    "automation_owns_stable_ids": False,
    "periodic_id_first": 370,
    "periodic_id_last": 461,
}

SOURCE_BOUND_MARKERS = {
    "src/simulation/elements/TSNS.cpp": ("if (rd > 25)", "parts[i].life = 0"),
    "src/simulation/elements/DTEC.cpp": ("if (rd > 25)", "parts[i].life = 0"),
    "src/simulation/elements/LSNS.cpp": ("if (rd > 25)", "parts[i].life = 0"),
    "src/simulation/elements/VSNS.cpp": ("if (rd > 25)", "parts[i].life = 0"),
    "src/simulation/elements/LDTC.cpp": ("if (detectLength < 0)", "We're out of bounds"),
    "src/simulation/elements/DLAY.cpp": ("parts[i].life--", "oldl==1"),
    "src/simulation/elements/STOR.cpp": ("!parts[i].tmp", "parts[i].life = 10"),
    "src/simulation/elements/CRAY.cpp": ("int partsRemaining = 255", "if (parts[i].tmp)"),
    "src/simulation/elements/PSTN.cpp": ("DEFAULT_LIMIT     = 0x1F", "MAX_FRAME         = 0x0F"),
    "src/simulation/elements/PIPE.cpp": ("count >= 2", "PPIP_TMPFLAG_PAUSED"),
    "src/simulation/elements/PPIP.cpp": ("coord_stack_limit = XRES*YRES", "coord_stack_size>=coord_stack_limit"),
    "src/simulation/elements/WIFI.cpp": ("CHANNELS-1", "wireless[parts[i].tmp][1] = 1"),
    "src/simulation/elements/SWCH.cpp": ("life!=10", "life==10"),
}

OMNI_BUDGET_MARKERS = {
    "src/simulation/OmniMetallurgy.cpp": ("ReactionsPerFrame = 2048", "currentTick + 1"),
    "src/simulation/OmniBiology.cpp": ("BiologyEventsPerFrame = 1024", "currentTick + 1"),
    "src/simulation/OmniNuclear.cpp": ("NuclearEventsPerFrame = 512", "currentTick + 1"),
    "src/simulation/OmniChemistry.cpp": ("ChemistryReactionsPerFrame = 1536", "currentTick + 1"),
}


def read_text(path: Path, errors: list[str]) -> str:
    try:
        return path.read_text(encoding="utf-8")
    except (OSError, UnicodeDecodeError) as exc:
        errors.append(f"{path}: cannot read UTF-8 text: {exc}")
        return ""


def read_json(path: Path, errors: list[str]) -> dict[str, Any]:
    text = read_text(path, errors)
    if not text:
        return {}
    try:
        value = json.loads(text)
    except json.JSONDecodeError as exc:
        errors.append(f"{path}: invalid JSON: {exc}")
        return {}
    if not isinstance(value, dict):
        errors.append(f"{path}: top-level JSON value must be an object")
        return {}
    return value


def read_csv(path: Path, errors: list[str]) -> list[dict[str, str]]:
    try:
        with path.open("r", encoding="utf-8", newline="") as stream:
            return list(csv.DictReader(stream))
    except (OSError, csv.Error, UnicodeDecodeError) as exc:
        errors.append(f"{path}: cannot read CSV: {exc}")
        return []


def split_pipe(value: str) -> list[str]:
    return [item for item in value.split("|") if item]


def unique_by(
    rows: list[dict[str, str]], field: str, path: Path, errors: list[str]
) -> dict[str, dict[str, str]]:
    result: dict[str, dict[str, str]] = {}
    for line_number, row in enumerate(rows, start=2):
        value = row.get(field, "")
        if not value:
            errors.append(f"{path}:{line_number}: missing {field}")
        elif value in result:
            errors.append(f"{path}:{line_number}: duplicate {field} {value!r}")
        else:
            result[value] = row
    return result


def read_meson_slots(root: Path, errors: list[str]) -> list[str | None]:
    path = root / "src" / "simulation" / "elements" / "meson.build"
    text = read_text(path, errors)
    if not text:
        return []
    marker = "simulation_elem_names = ["
    if marker not in text:
        errors.append(f"{path}: simulation_elem_names list is missing")
        return []
    body = text.split(marker, 1)[1].split("\n]", 1)[0]
    slots: list[str | None] = []
    for line_number, raw in enumerate(body.splitlines(), start=2):
        value = raw.split("#", 1)[0].strip().rstrip(",").strip()
        if not value:
            continue
        if value == "disabler()":
            slots.append(None)
        elif len(value) >= 2 and value[0] == value[-1] == "'":
            slots.append(value[1:-1])
        else:
            errors.append(f"{path}:{line_number}: unsupported element slot {value!r}")
    return slots


def check_capabilities(root: Path, errors: list[str]) -> set[str]:
    matrix_path = root / "docs" / "AUTOMATION_CAPABILITIES.csv"
    official_path = root / "tools" / "data" / "official_elements_100_0.csv"
    registry_path = root / "docs" / "ELEMENT_REGISTRY.csv"
    rows = read_csv(matrix_path, errors)
    official_rows = read_csv(official_path, errors)
    registry_rows = read_csv(registry_path, errors)
    by_capability = unique_by(rows, "capability_id", matrix_path, errors)
    by_official = unique_by(official_rows, "identifier", official_path, errors)
    by_registry_id = unique_by(registry_rows, "stable_id", registry_path, errors)
    meson_slots = read_meson_slots(root, errors)

    found = set(by_capability)
    if found != REQUIRED_CAPABILITIES:
        errors.append(
            f"{matrix_path}: capability set differs; "
            f"missing={sorted(REQUIRED_CAPABILITIES - found)!r}, "
            f"extra={sorted(found - REQUIRED_CAPABILITIES)!r}"
        )

    used_sources: set[str] = set()
    for capability, row in by_capability.items():
        if row.get("decision") != "reuse_official":
            errors.append(
                f"{matrix_path}: {capability} must use decision=reuse_official"
            )
        identifiers = split_pipe(row.get("official_identifiers", ""))
        ids = split_pipe(row.get("stable_ids", ""))
        sources = split_pipe(row.get("source_files", ""))
        if not identifiers or len(identifiers) != len(ids) or len(ids) != len(sources):
            errors.append(
                f"{matrix_path}: {capability} identifier/ID/source lists must be non-empty and aligned"
            )
            continue
        if len(identifiers) != len(set(identifiers)):
            errors.append(f"{matrix_path}: {capability} repeats an official identifier")

        for identifier, stable_id, source in zip(identifiers, ids, sources):
            official = by_official.get(identifier)
            if official is None:
                errors.append(f"{matrix_path}: {capability} unknown official identifier {identifier}")
                continue
            expected = {
                "stable_id": stable_id,
                "slot_status": "active",
                "source_file": source,
                "official_commit": "bff38ce6959e1c1a7a4d17d0d5d44d127a0dfcbd",
            }
            for field, value in expected.items():
                if official.get(field) != value:
                    errors.append(
                        f"{official_path}: {identifier} {field}={official.get(field)!r}; expected {value!r}"
                    )
            registry = by_registry_id.get(stable_id)
            if registry is None:
                errors.append(f"{registry_path}: missing official stable ID {stable_id}")
            else:
                registry_expected = {
                    "identifier": identifier,
                    "source_file": source,
                    "implementation_status": "implemented",
                    "module": "official",
                }
                for field, value in registry_expected.items():
                    if registry.get(field) != value:
                        errors.append(
                            f"{registry_path}: ID {stable_id} {field}={registry.get(field)!r}; expected {value!r}"
                        )
            used_sources.add(source)

        numeric_limits = {
            "max_scan_radius": (0, 25),
            "max_events_per_trigger": (1, 256),
            "max_network_nodes": (1, 64),
        }
        for field, (minimum, maximum) in numeric_limits.items():
            try:
                number = int(row.get(field, ""))
            except ValueError:
                errors.append(f"{matrix_path}: {capability} {field} must be an integer")
                continue
            if not minimum <= number <= maximum:
                errors.append(
                    f"{matrix_path}: {capability} {field}={number} outside {minimum}..{maximum}"
                )
        for field in ("loop_guard", "failure_state", "metrics", "notes_zh"):
            if not row.get(field, "").strip():
                errors.append(f"{matrix_path}: {capability} missing {field}")

    periodic_rows = read_csv(root / "docs" / "PERIODIC_ELEMENT_SOURCE_MAP.csv", errors)
    periodic_by_id = {row.get("stable_id", ""): row for row in periodic_rows}
    if len(periodic_rows) != 118:
        errors.append("docs/PERIODIC_ELEMENT_SOURCE_MAP.csv: expected 118 rows")
    generated_ids = sorted(
        int(row["stable_id"])
        for row in periodic_rows
        if row.get("implementation_type") == "family_generated"
        and row.get("stable_id", "").isdigit()
    )
    if generated_ids != list(range(370, 462)):
        errors.append("docs/PERIODIC_ELEMENT_SOURCE_MAP.csv: expected generated IDs 370..461")
    for stable_id in LEGACY_AUTOMATION_IDS:
        if str(stable_id) not in periodic_by_id:
            errors.append(f"periodic source map does not own former automation ID {stable_id}")
        row = by_registry_id.get(str(stable_id))
        if row is not None and row.get("module") != "periodic":
            errors.append(
                f"{registry_path}: former automation ID {stable_id} must belong to the periodic module, "
                f"not {row.get('module')!r}"
            )
        if stable_id < len(meson_slots) and meson_slots[stable_id] is not None and row is None:
            errors.append(
                f"{root / 'src/simulation/elements/meson.build'}: periodic ID {stable_id} "
                f"is enabled as {meson_slots[stable_id]!r} without a registry row"
            )
    content = read_text(root / "docs" / "CONTENT_MATRIX.md", errors)
    if "| 周期表固定区 | `370..461` | 92（规划/分批实现） | 原子序数映射固定，不是解锁顺序 |" not in content:
        errors.append("docs/CONTENT_MATRIX.md: missing periodic stable-ID allocation")
    omni_content = read_text(root / "src" / "gui" / "game" / "OmniContent.h", errors)
    for marker in (
        "constexpr int OmniPeriodicFirstId = 370;",
        "constexpr int OmniPeriodicLastId = 461;",
    ):
        if marker not in omni_content:
            errors.append(f"src/gui/game/OmniContent.h: missing {marker!r}")
    return used_sources


def check_source_fingerprints(
    root: Path, used_sources: set[str], errors: list[str]
) -> None:
    manifest_path = root / "automation" / "0.3.0" / "official-source-sha256.json"
    manifest = read_json(manifest_path, errors)
    files = manifest.get("files")
    if not isinstance(files, dict):
        errors.append(f"{manifest_path}: files must be an object")
        return
    file_names = set(files)
    if file_names != used_sources:
        errors.append(
            f"{manifest_path}: source set differs from capability matrix; "
            f"missing={sorted(used_sources - file_names)!r}, extra={sorted(file_names - used_sources)!r}"
        )
    for relative, expected_hash in files.items():
        if not isinstance(relative, str) or not isinstance(expected_hash, str):
            errors.append(f"{manifest_path}: source names and hashes must be strings")
            continue
        path = root / relative
        try:
            data = path.read_bytes()
        except OSError as exc:
            errors.append(f"{path}: cannot read official source: {exc}")
            continue
        actual = hashlib.sha256(data).hexdigest().upper()
        if actual != expected_hash.upper():
            errors.append(
                f"{path}: official behavior fingerprint changed; actual={actual}, expected={expected_hash.upper()}"
            )
        text = data.decode("utf-8", errors="replace")
        name = Path(relative).stem
        if f'Identifier = "DEFAULT_PT_{name}"' not in text:
            errors.append(f"{path}: missing official identifier DEFAULT_PT_{name}")
        if "OMNI_PT_" in text or "OmniAutomation" in text:
            errors.append(f"{path}: OmniPack automation hook found in official element source")
        for marker in SOURCE_BOUND_MARKERS.get(relative, ()):
            if marker not in text:
                errors.append(f"{path}: missing audited bound marker {marker!r}")


def check_scenarios(root: Path, errors: list[str]) -> None:
    path = root / "automation" / "0.3.0" / "scenario-spec.json"
    spec = read_json(path, errors)
    if spec.get("schema_version") != 1 or spec.get("content_version") != "0.3.0-dev":
        errors.append(f"{path}: expected schema_version=1 and content_version=0.3.0-dev")
    if spec.get("safety_policy") != EXPECTED_POLICY:
        errors.append(f"{path}: safety_policy differs from fail-closed 0.3 contract")

    bounds = spec.get("bounds")
    if not isinstance(bounds, dict):
        errors.append(f"{path}: bounds must be an object")
        bounds = {}
    expected_bounds = {
        "max_sensor_radius": 8,
        "max_signal_events_per_frame": 128,
        "max_circuit_particles": 4096,
        "max_network_nodes": 64,
        "max_frames_per_challenge": 240,
        "quiescence_frames": 32,
    }
    if bounds != expected_bounds:
        errors.append(f"{path}: bounds differ from the audited 0.3 limits")

    runtime = spec.get("runtime_contract")
    if not isinstance(runtime, dict) or set(runtime.get("required_metrics", [])) != REQUIRED_METRICS:
        errors.append(f"{path}: runtime contract does not declare the exact required metrics")

    scenarios = spec.get("scenarios")
    if not isinstance(scenarios, list):
        errors.append(f"{path}: scenarios must be an array")
        scenarios = []
    by_scenario: dict[str, dict[str, Any]] = {}
    covered_capabilities: set[str] = set()
    official_codes: set[str] = set()
    for index, scenario in enumerate(scenarios):
        if not isinstance(scenario, dict):
            errors.append(f"{path}: scenarios[{index}] must be an object")
            continue
        scenario_id = scenario.get("id")
        if not isinstance(scenario_id, str) or not scenario_id:
            errors.append(f"{path}: scenarios[{index}] missing id")
            continue
        if scenario_id in by_scenario:
            errors.append(f"{path}: duplicate scenario id {scenario_id}")
            continue
        by_scenario[scenario_id] = scenario
        capabilities = set(scenario.get("capabilities", []))
        unknown = capabilities - REQUIRED_CAPABILITIES
        if unknown:
            errors.append(f"{path}: {scenario_id} unknown capabilities {sorted(unknown)!r}")
        covered_capabilities.update(capabilities)
        modules = set(scenario.get("module_dependencies", []))
        if modules - MODULES:
            errors.append(f"{path}: {scenario_id} unknown modules {sorted(modules - MODULES)!r}")
        if scenario_id == "integrated-factory" and modules != MODULES:
            errors.append(f"{path}: integrated-factory must depend on all four modules")
        official = scenario.get("official_elements", [])
        if not isinstance(official, list) or not official:
            errors.append(f"{path}: {scenario_id} must declare official elements")
        else:
            official_codes.update(str(value) for value in official)
        assertions = scenario.get("success_assertions")
        if not isinstance(assertions, list) or len(assertions) < 5 or len(assertions) != len(set(assertions)):
            errors.append(f"{path}: {scenario_id} needs at least five unique success assertions")
        if not str(scenario.get("failure_state", "")).strip():
            errors.append(f"{path}: {scenario_id} missing failure_state")
        limits = {
            "max_radius": bounds.get("max_sensor_radius", 0),
            "max_frames": bounds.get("max_frames_per_challenge", 0),
            "max_particles": bounds.get("max_circuit_particles", 0),
            "event_budget": bounds.get("max_signal_events_per_frame", 0),
        }
        for field, maximum in limits.items():
            value = scenario.get(field)
            if not isinstance(value, int) or value <= 0 or not isinstance(maximum, int) or value > maximum:
                errors.append(f"{path}: {scenario_id} {field}={value!r} exceeds audited bound {maximum!r}")

    scenario_ids = set(by_scenario)
    if scenario_ids != REQUIRED_SCENARIOS:
        errors.append(
            f"{path}: scenario set differs; missing={sorted(REQUIRED_SCENARIOS - scenario_ids)!r}, "
            f"extra={sorted(scenario_ids - REQUIRED_SCENARIOS)!r}"
        )
    if covered_capabilities != REQUIRED_CAPABILITIES:
        errors.append(f"{path}: scenarios do not cover every required capability")

    challenges = spec.get("challenges")
    if not isinstance(challenges, list):
        errors.append(f"{path}: challenges must be an array")
        challenges = []
    by_challenge: dict[str, dict[str, Any]] = {}
    for index, challenge in enumerate(challenges):
        if not isinstance(challenge, dict):
            errors.append(f"{path}: challenges[{index}] must be an object")
            continue
        challenge_id = challenge.get("id")
        if not isinstance(challenge_id, str) or challenge_id in by_challenge:
            errors.append(f"{path}: invalid or duplicate challenge id {challenge_id!r}")
            continue
        by_challenge[challenge_id] = challenge
        if challenge.get("scenario") not in by_scenario:
            errors.append(f"{path}: {challenge_id} references an unknown scenario")
        if not str(challenge.get("expected_outcome", "")).strip():
            errors.append(f"{path}: {challenge_id} missing expected_outcome")
    challenge_ids = set(by_challenge)
    if challenge_ids != REQUIRED_CHALLENGES:
        errors.append(
            f"{path}: challenge set differs; missing={sorted(REQUIRED_CHALLENGES - challenge_ids)!r}, "
            f"extra={sorted(challenge_ids - REQUIRED_CHALLENGES)!r}"
        )


def check_module_bounds(root: Path, errors: list[str]) -> None:
    for relative, markers in OMNI_BUDGET_MARKERS.items():
        path = root / relative
        text = read_text(path, errors)
        for marker in markers:
            if marker not in text:
                errors.append(f"{path}: missing module budget/loop marker {marker!r}")
        if "NPART" in text or "parts.active" in text:
            errors.append(f"{path}: forbidden whole-particle scan in automated module path")


def check_runtime_contract(root: Path, errors: list[str]) -> None:
    lua_path = root / "tools" / "runtime" / "automation_regression.lua"
    wrapper_path = root / "tools" / "runtime_lua_automation_test.ps1"
    lua = read_text(lua_path, errors)
    wrapper = read_text(wrapper_path, errors)
    for scenario in REQUIRED_SCENARIOS:
        if f'["{scenario}"]' not in lua:
            errors.append(f"{lua_path}: missing runtime scenario {scenario}")
    for challenge in REQUIRED_CHALLENGES:
        if f'"{challenge}"' not in lua:
            errors.append(f"runtime automation contract is missing challenge {challenge}")
    lua_markers = (
        "sim.resetOmniEventMetrics()",
        "sim.omniEventMetrics()",
        "sim.saveStamp(",
        "sim.loadStamp(",
        "sim.clearSim()",
        "raw_step(32)",
        "metrics.stop_event_delta",
        "metrics.recovery_assertions",
        "metrics.peak_events_per_frame <= 128",
        'OMNI_AUTOMATION_STATUS=PASS',
        'OMNI_AUTOMATION_STATUS=FAIL',
    )
    for marker in lua_markers:
        if marker not in lua:
            errors.append(f"{lua_path}: missing runtime evidence marker {marker!r}")
    wrapper_markers = (
        "$process.Dispose()",
        "$process.WaitForExit()",
        'ArgumentList.Add("ddir")',
        'source_tree_state = $sourceTreeState',
        'return "dirty_probe"',
        "Dirty-probe automation artifacts cannot be used as gate evidence",
        "Automation verifier executable does not match the generator executable",
        "OMNI_AUTOMATION_STOP_EVENT_DELTA",
        "OMNI_AUTOMATION_PEAK_EVENTS_PER_FRAME",
        "OMNI_AUTOMATION_SCENARIO_PASS",
        "OMNI_AUTOMATION_CHALLENGE_PASS",
    )
    for marker in wrapper_markers:
        if marker not in wrapper:
            errors.append(f"{wrapper_path}: missing wrapper contract marker {marker!r}")
    for secret in ("GITHUB_PAT_TOKEN", "GITHUB_TOKEN", "GH_TOKEN"):
        if secret not in wrapper:
            errors.append(f"{wrapper_path}: does not remove {secret}")


def audit(root: Path) -> list[str]:
    errors: list[str] = []
    used_sources = check_capabilities(root, errors)
    check_source_fingerprints(root, used_sources, errors)
    check_scenarios(root, errors)
    check_module_bounds(root, errors)
    check_runtime_contract(root, errors)
    return errors


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Audit 0.3 official automation capabilities, bounds, scenarios and reserved IDs."
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
            print(f"automation-audit: ERROR {error}", file=sys.stderr)
        print(f"automation-audit: FAIL ({len(errors)} errors)", file=sys.stderr)
        return 1
    if not args.quiet:
        print(
            "automation-audit: PASS "
            "(11 capabilities, 9 scenarios, 6 challenges, 17 pinned official sources, 0 new elements)"
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
