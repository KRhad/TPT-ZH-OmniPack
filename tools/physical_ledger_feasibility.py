"""Static, source-bound feasibility inventory for the Legacy physical ledger.

This is deliberately an instrumentation-planning tool, not a conservation solver.
It makes no mass, momentum, energy, positivity or source-attribution claim.  Its
job is to list the current storage and mutation/correction anchors so a future
runtime ledger cannot silently assume that Legacy proxy fields are physical units.
"""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
from typing import Any, Iterable


SCHEMA_VERSION = 1
ROOT = Path(__file__).resolve().parents[1]
SOURCE_ROOTS = (Path("src/simulation"), Path("src/lua"))
SOURCE_SUFFIXES = {".cpp", ".h"}
AUTHORITATIVE_FILES = (
    Path("src/simulation/Particle.h"),
    Path("src/simulation/Air.h"),
    Path("src/simulation/Simulation.h"),
    Path("src/simulation/Element.h"),
    Path("src/simulation/SimulationData.cpp"),
    Path("src/simulation/Simulation.cpp"),
    Path("src/lua/LuaScriptInterface.cpp"),
    Path("src/lua/LuaSimulation.cpp"),
)
LIFECYCLE_DEFINITIONS = {
    "create_part": re.compile(r"\bint\s+Simulation::create_part\s*\("),
    "kill_part": re.compile(r"\bvoid\s+Simulation::kill_part\s*\("),
    "part_change_type": re.compile(r"\bbool\s+Simulation::part_change_type\s*\("),
}
DIRECT_TYPE_ASSIGNMENT = re.compile(
    r"\b(?:[A-Za-z_]\w*\s*->\s*)?parts\s*\[[^\]]+\]\s*\.\s*type\s*=(?!=)"
)
CORRECTION_PATTERNS = {
    "restrict_flt": re.compile(r"\brestrict_flt\s*\("),
    "std_clamp": re.compile(r"\bstd::clamp\s*\("),
    "std_min": re.compile(r"\bstd::min\s*\("),
    "std_max": re.compile(r"\bstd::max\s*\("),
    "min_temp": re.compile(r"\bMIN_TEMP\b"),
    "max_temp": re.compile(r"\bMAX_TEMP\b"),
    "min_pressure": re.compile(r"\bMIN_PRESSURE\b"),
    "max_pressure": re.compile(r"\bMAX_PRESSURE\b"),
}
AIR_EXPLICIT_CAP = re.compile(
    r"\bif\s*\([^\n]*(?:MIN_TEMP|MAX_TEMP|MIN_PRESSURE|MAX_PRESSURE)[^\n]*\)"
)


class AuditError(ValueError):
    """The source no longer matches the deliberate audit contract."""


def _read(root: Path, relative_path: Path) -> str:
    path = root / relative_path
    if not path.is_file():
        raise AuditError(f"required source file is missing: {relative_path.as_posix()}")
    return path.read_text(encoding="utf-8")


def _source_files(root: Path) -> list[Path]:
    files: list[Path] = []
    for source_root in SOURCE_ROOTS:
        directory = root / source_root
        if not directory.is_dir():
            raise AuditError(f"required source root is missing: {source_root.as_posix()}")
        files.extend(
            path.relative_to(root)
            for path in directory.rglob("*")
            if path.is_file() and path.suffix in SOURCE_SUFFIXES
        )
    return sorted(set(files), key=lambda path: path.as_posix())


def _matches(root: Path, files: Iterable[Path], pattern: re.Pattern[str]) -> list[dict[str, Any]]:
    matches: list[dict[str, Any]] = []
    for relative_path in files:
        for line_number, line in enumerate(_read(root, relative_path).splitlines(), start=1):
            if pattern.search(line):
                matches.append(
                    {
                        "path": relative_path.as_posix(),
                        "line": line_number,
                        "text": line.strip(),
                    }
                )
    return matches


def _sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest().upper()


def _git_state(root: Path) -> tuple[str, list[str]]:
    result = subprocess.run(
        ["git", "-C", str(root), "rev-parse", "HEAD"],
        capture_output=True,
        check=False,
        text=True,
        encoding="utf-8",
    )
    head = result.stdout.strip()
    if result.returncode or not re.fullmatch(r"[0-9a-f]{40}", head):
        raise AuditError("cannot resolve repository HEAD")
    status = subprocess.run(
        ["git", "-C", str(root), "status", "--porcelain=v1", "--untracked-files=all"],
        capture_output=True,
        check=False,
        text=True,
        encoding="utf-8",
    )
    if status.returncode:
        raise AuditError("cannot inspect repository worktree state")
    return head, [line for line in status.stdout.splitlines() if line]


def _require_once(name: str, matches: list[dict[str, Any]]) -> dict[str, Any]:
    if len(matches) != 1:
        raise AuditError(f"expected exactly one {name} definition, found {len(matches)}")
    return matches[0]


def _has_field_declaration(text: str, field: str) -> bool:
    return bool(
        re.search(
            rf"\b(?:float|double|int|unsigned\s+int|std::\w+)\s+[^;\n]*\b{re.escape(field)}\b",
            text,
        )
    )


def build_inventory(root: Path = ROOT, *, allow_dirty: bool = False) -> dict[str, Any]:
    root = root.resolve()
    source_commit, status_lines = _git_state(root)
    if status_lines and not allow_dirty:
        raise AuditError("source worktree is dirty; use a committed source or --allow-dirty")
    source_files = _source_files(root)
    particle_text = _read(root, Path("src/simulation/Particle.h"))
    air_text = _read(root, Path("src/simulation/Air.h"))
    simulation_text = _read(root, Path("src/simulation/Simulation.h"))
    element_text = _read(root, Path("src/simulation/Element.h"))

    expected_particle_fields = ("x", "y", "vx", "vy", "temp")
    for field in expected_particle_fields:
        if not _has_field_declaration(particle_text, field):
            raise AuditError(f"Particle.h is missing expected float field: {field}")
    expected_air_current_fields = ("pv", "vx", "vy", "hv", "fvx", "fvy")
    for field in expected_air_current_fields:
        if not _has_field_declaration(simulation_text, field):
            raise AuditError(f"Simulation.h is missing expected Air field: {field}")
    expected_air_scratch_fields = ("opv", "ovx", "ovy", "ohv")
    for field in expected_air_scratch_fields:
        if not _has_field_declaration(air_text, field):
            raise AuditError(f"Air.h is missing expected scratch field: {field}")

    lifecycle_definitions = {
        name: _require_once(name, _matches(root, source_files, pattern))
        for name, pattern in LIFECYCLE_DEFINITIONS.items()
    }
    lifecycle_occurrences = {
        name: len(_matches(root, source_files, re.compile(rf"\b{re.escape(name)}\s*\(")))
        for name in LIFECYCLE_DEFINITIONS
    }
    direct_type_assignments = _matches(root, source_files, DIRECT_TYPE_ASSIGNMENT)
    if not direct_type_assignments:
        raise AuditError("expected at least one direct Particle.type assignment candidate")

    correction_candidates = {
        name: len(_matches(root, source_files, pattern))
        for name, pattern in CORRECTION_PATTERNS.items()
    }
    air_caps = _matches(
        root,
        [Path("src/simulation/Air.cpp")],
        AIR_EXPLICIT_CAP,
    )
    if not air_caps:
        raise AuditError("Air.cpp has no explicit Legacy cap candidates")

    physical_field_names = ("mass", "density", "moles", "enthalpy", "energy")
    particle_physical_fields = {
        field: _has_field_declaration(particle_text, field) for field in physical_field_names
    }
    air_physical_fields = {
        field: _has_field_declaration(air_text, field) for field in physical_field_names
    }
    if any(particle_physical_fields.values()) or any(air_physical_fields.values()):
        raise AuditError("authoritative storage changed; update physical-ledger feasibility rules")

    if "float HeatCapacity" not in element_text or "int Weight" not in element_text:
        raise AuditError("Element physical-like properties changed; review feasibility report")

    bound_sources = {path.as_posix(): _sha256(root / path) for path in AUTHORITATIVE_FILES}
    return {
        "schema_version": SCHEMA_VERSION,
        "source_commit": source_commit,
        "source_dirty": bool(status_lines),
        "source_status_lines": status_lines,
        "source_files_scanned": len(source_files),
        "bound_source_sha256": bound_sources,
        "state_storage": {
            "particle_float_fields": list(expected_particle_fields),
            "air_current_float_fields": list(expected_air_current_fields),
            "air_scratch_float_fields": list(expected_air_scratch_fields),
            "particle_authoritative_physical_fields_present": particle_physical_fields,
            "air_authoritative_physical_fields_present": air_physical_fields,
            "element_weight_is_authoritative_mass": False,
            "element_heat_capacity_is_physical_joules_per_kelvin": False,
        },
        "lifecycle": {
            "definitions": lifecycle_definitions,
            "lexical_occurrences_in_simulation_and_lua": lifecycle_occurrences,
            "direct_particle_type_assignment_candidates": direct_type_assignments,
        },
        "correction_inventory": {
            "lexical_candidate_counts": correction_candidates,
            "air_explicit_cap_candidates": air_caps,
            "complete_runtime_correction_event_ledger_present": False,
        },
        "claims": {
            "physical_mass_conservation_evaluated": False,
            "physical_momentum_conservation_evaluated": False,
            "physical_energy_conservation_evaluated": False,
            "source_sink_attribution_evaluated": False,
            "correction_events_evaluated": False,
            "physical_pressure_positivity_evaluated": False,
        },
        "recommendation": {
            "next_safe_step": "add an optional runtime lifecycle/correction observer before assigning any physical units",
            "must_not_infer": [
                "particle record count is condensed mass",
                "Element.Weight is parcel mass",
                "temperature sum is energy",
                "velocity sum is momentum",
                "pv is conserved gas mass or absolute pressure",
                "bound occupancy is a clamp-event count",
            ],
        },
    }


def _write_json(path: Path, value: dict[str, Any]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, indent=2, sort_keys=True) + "\n", encoding="utf-8", newline="\n")


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Inventory Legacy state and mutation anchors for physical-ledger planning"
    )
    parser.add_argument("--root", type=Path, default=ROOT)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--allow-dirty", action="store_true")
    args = parser.parse_args()
    inventory = build_inventory(args.root, allow_dirty=args.allow_dirty)
    if args.output:
        _write_json(args.output, inventory)
    else:
        print(json.dumps(inventory, indent=2, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
