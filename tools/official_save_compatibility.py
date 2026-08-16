#!/usr/bin/env python3
"""Run real OmniPack load/simulate/save/reload over a provenance-verified corpus."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path, PurePosixPath
import re
import subprocess
from typing import Sequence


OFFICIAL_REPOSITORY = "https://github.com/The-Powder-Toy/The-Powder-Toy"
OFFICIAL_TPT_BENCH_REPOSITORY = "https://github.com/The-Powder-Toy/tpt-bench"
OFFICIAL_WEB_API_ORIGIN = "https://powdertoy.co.uk"
SHA256_RE = re.compile(r"^[0-9A-F]{64}$")
REVISION_RE = re.compile(r"^[0-9a-fA-F]{40}$")
COVERAGE_CONTRACT = "official-tpt-save-coverage-v1"
COVERAGE_REQUIRED = (
    "basic_particles", "powders", "solids", "liquids", "gases",
    "temperature", "pressure", "velocity", "walls", "fans",
    "electronics", "life", "signs", "decoration", "legacy_states",
    "larger_save",
)
COVERAGE_METRICS = (
    "input_bytes", "particles", "powders", "solids", "liquids", "gases",
    "temperature_signals", "pressure_cells", "velocity_signals",
    "wall_cells", "fan_cells", "electronics_particles", "life_particles",
    "signs", "decorated_particles", "legacy_state", "larger_save",
)


def coverage_categories(metrics: dict[str, int]) -> set[str]:
    mapping = {
        "basic_particles": "particles", "powders": "powders",
        "solids": "solids", "liquids": "liquids", "gases": "gases",
        "temperature": "temperature_signals", "pressure": "pressure_cells",
        "velocity": "velocity_signals", "walls": "wall_cells",
        "fans": "fan_cells", "electronics": "electronics_particles",
        "life": "life_particles", "signs": "signs",
        "decoration": "decorated_particles", "legacy_states": "legacy_state",
    }
    categories = {category for category, metric in mapping.items() if metrics.get(metric, 0) > 0}
    if metrics.get("input_bytes", 0) >= 10000:
        categories.add("larger_save")
    return categories


def coverage_metrics_are_valid(metrics: dict[str, int]) -> bool:
    return (
        set(metrics) == set(COVERAGE_METRICS)
        and all(isinstance(value, int) and not isinstance(value, bool) and value >= 0 for value in metrics.values())
        and metrics.get("legacy_state") in {0, 1}
        and metrics.get("larger_save") in {0, 1}
        and metrics.get("larger_save") == int(metrics.get("input_bytes", 0) >= 10000)
    )


def safe_fixture_path(raw: object) -> str | None:
    if not isinstance(raw, str) or "\\" in raw:
        return None
    path = PurePosixPath(raw)
    if path.is_absolute() or ".." in path.parts or ":" in raw or path.suffix.lower() not in {".cps", ".stm"}:
        return None
    return str(path)


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest().upper()


def write(path: Path, value: dict[str, object]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


def stdout_integer(stdout: str, name: str) -> int | None:
    matches = re.findall(rf"(?m)^{re.escape(name)}=([0-9]+)\s*$", stdout)
    return int(matches[0]) if len(matches) == 1 else None


def result(run_id: str, commit: str, status: str, reason: str, **extra: object) -> dict[str, object]:
    return {
        "schema": "omnipack-release-evidence",
        "schema_version": 1,
        "test": "official_tpt_save_compatibility",
        "run_id": run_id,
        "commit": commit,
        "status": status,
        "passed": status == "PASS",
        "reason": reason,
        "files_total": 0,
        "files_passed": 0,
        "files_failed": 0,
        "files": [],
        "coverage_contract": COVERAGE_CONTRACT,
        "coverage_required": list(COVERAGE_REQUIRED),
        "coverage_observed": [],
        "coverage_missing": list(COVERAGE_REQUIRED),
        "coverage_passed": False,
        **extra,
    }


def provenance_identity_is_valid(provenance: dict[str, object]) -> bool:
    """Accept v1 Git-only evidence or the explicit v2 mixed-source contract."""
    common_invalid = (
        provenance.get("schema") != "omnipack-release-evidence",
        provenance.get("schema_version") != 1,
        provenance.get("test") != "official_tpt_provenance",
        provenance.get("status") != "PASS",
        provenance.get("passed") is not True,
        provenance.get("revision_exists") is not True,
        provenance.get("revision_reachable_from_official_remote") is not True,
        provenance.get("files_failed") != 0,
    )
    if any(common_invalid):
        return False
    if provenance.get("provenance_schema") == "omnipack-official-tpt-save-corpus-v2":
        repositories = provenance.get("source_repositories")
        rows = provenance.get("repositories")
        if not isinstance(repositories, list) or not repositories:
            return False
        if not isinstance(rows, list) or not rows:
            return False
        # v2 must retain at least one real Git object; website rows are an
        # explicitly separate source kind and never satisfy this requirement.
        git_rows = [
            row for row in rows
            if isinstance(row, dict) and row.get("source_kind") == "github_git"
        ]
        return bool(git_rows) and all(
            row.get("repository") in {OFFICIAL_REPOSITORY, OFFICIAL_TPT_BENCH_REPOSITORY}
            and isinstance(row.get("revision"), str)
            and REVISION_RE.fullmatch(str(row.get("revision"))) is not None
            and row.get("revision_exists") is True
            and row.get("revision_reachable_from_official_remote") is True
            for row in git_rows
        )
    return (
        provenance.get("repository") == OFFICIAL_REPOSITORY
        and isinstance(provenance.get("revision"), str)
        and REVISION_RE.fullmatch(str(provenance.get("revision", ""))) is not None
    )


def main(argv: Sequence[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--corpus", type=Path, required=True)
    parser.add_argument("--provenance-evidence", type=Path, required=True)
    parser.add_argument("--probe", type=Path, required=True)
    parser.add_argument("--run-id", required=True)
    parser.add_argument("--commit", required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args(argv)
    if not args.provenance_evidence.is_file():
        document = result(args.run_id, args.commit, "NOT_TESTED", "official provenance evidence is absent")
        write(args.output, document)
        return 2
    try:
        provenance = json.loads(args.provenance_evidence.read_text(encoding="utf-8"))
    except (OSError, UnicodeError, json.JSONDecodeError):
        document = result(args.run_id, args.commit, "FAIL", "official provenance evidence is invalid")
        write(args.output, document)
        return 1
    if not isinstance(provenance, dict) or any((
        provenance.get("run_id") != args.run_id,
        provenance.get("commit") != args.commit,
        not provenance_identity_is_valid(provenance),
    )):
        document = result(args.run_id, args.commit, "NOT_TESTED", "official provenance gate is not PASS")
        write(args.output, document)
        return 2
    rows = provenance.get("files")
    if not isinstance(rows, list) or not rows:
        document = result(args.run_id, args.commit, "FAIL", "official provenance file inventory is empty")
        write(args.output, document)
        return 1
    if provenance.get("files_total") != len(rows) or provenance.get("files_verified") != len(rows):
        document = result(args.run_id, args.commit, "FAIL", "official provenance inventory counts are inconsistent")
        write(args.output, document)
        return 1
    if not args.probe.is_file():
        document = result(args.run_id, args.commit, "FAIL", "compatibility probe is absent", files_total=len(rows), files_failed=len(rows))
        write(args.output, document)
        return 1
    probe_sha256 = sha256(args.probe)
    records: list[dict[str, object]] = []
    failed = 0
    observed_coverage: set[str] = set()
    required_markers = {
        "load": "official_save_load_pass=true",
        "missing_elements_zero": "official_save_missing_elements_zero=true",
        "initial_load_state_validate": "official_save_initial_load_state_validate_pass=true",
        "simulate": "official_save_simulate_pass=true",
        "save": "official_save_save_pass=true",
        "reload": "official_save_reload_pass=true",
        "state_validate": "official_save_state_validate_pass=true",
        "negative_block_map": "official_save_negative_block_map_rejected=true",
        "negative_legacy_field": "official_save_negative_legacy_field_rejected=true",
        "negative_sign": "official_save_negative_sign_rejected=true",
        "negative_validity_mask": "official_save_negative_validity_mask_rejected=true",
        "negative_deterministic_frame": "official_save_negative_deterministic_frame_rejected=true",
        "negative_simulation_option": "official_save_negative_simulation_option_rejected=true",
        "negative_codec_roundtrip": "official_save_negative_codec_roundtrip_rejected=true",
    }
    seen_paths: set[str] = set()
    for row in rows:
        path = safe_fixture_path(row.get("path")) if isinstance(row, dict) else None
        fixture = args.corpus / Path(path) if path else args.corpus / "invalid"
        phases = {name: False for name in required_markers}
        exit_code = -1
        fixture_sha = None
        hash_binding_passed = False
        input_particles = None
        initial_loaded_particles = None
        output_particles = None
        initial_particle_inventory = False
        coverage_metrics: dict[str, int] = {}
        coverage_metrics_valid = False
        file_coverage: set[str] = set()
        if path and path not in seen_paths and row.get("match") is True and fixture.is_file() and not fixture.is_symlink():
            try:
                fixture.resolve().relative_to(args.corpus.resolve())
                fixture_sha = sha256(fixture)
            except (OSError, ValueError):
                fixture_sha = None
            upstream_sha = row.get("upstream_sha256")
            manifest_sha = row.get("manifest_sha256")
            recorded_fixture_sha = row.get("fixture_sha256")
            hash_binding_passed = (
                isinstance(upstream_sha, str) and SHA256_RE.fullmatch(upstream_sha) is not None
                and upstream_sha == manifest_sha == recorded_fixture_sha == fixture_sha
            )
        if path:
            seen_paths.add(path)
        if hash_binding_passed:
            completed = subprocess.run(
                [str(args.probe), str(fixture)], check=False,
                capture_output=True, text=True, encoding="utf-8", errors="replace",
            )
            exit_code = completed.returncode
            phases = {name: marker in completed.stdout for name, marker in required_markers.items()}
            input_particles = stdout_integer(completed.stdout, "input_particles")
            initial_loaded_particles = stdout_integer(completed.stdout, "initial_loaded_particles")
            output_particles = stdout_integer(completed.stdout, "output_particles")
            parsed_metrics = {
                name: stdout_integer(completed.stdout, "coverage_" + name)
                for name in COVERAGE_METRICS
            }
            coverage_metrics_valid = all(value is not None for value in parsed_metrics.values())
            if coverage_metrics_valid:
                coverage_metrics = {
                    name: int(value) for name, value in parsed_metrics.items()
                    if isinstance(value, int)
                }
                coverage_metrics_valid = coverage_metrics_are_valid(coverage_metrics)
            if coverage_metrics_valid:
                file_coverage = coverage_categories(coverage_metrics)
                observed_coverage.update(file_coverage)
            initial_particle_inventory = (
                input_particles is not None
                and initial_loaded_particles == input_particles
                and output_particles is not None
            )
        passed = (
            hash_binding_passed
            and exit_code == 0
            and all(phases.values())
            and initial_particle_inventory
            and coverage_metrics_valid
        )
        failed += 0 if passed else 1
        records.append({
            "path": path, "fixture_sha256": fixture_sha,
            "source_kind": row.get("source_kind", "github_git"),
            "source_repository": row.get("source_repository", provenance.get("repository")),
            "source_revision": row.get("source_revision", provenance.get("revision")),
            "source_path": row.get("source_path"),
            "source_locator": row.get("source_locator"),
            "content_locator": row.get("content_locator"),
            "source_date": row.get("source_date"),
            "save_id": row.get("save_id"),
            "source_metadata": row.get("metadata"),
            "probe_sha256": probe_sha256,
            "provenance_hash_binding_passed": hash_binding_passed,
            **phases,
            "input_particles": input_particles,
            "initial_loaded_particles": initial_loaded_particles,
            "output_particles": output_particles,
            "initial_particle_inventory": initial_particle_inventory,
            "coverage_metrics": coverage_metrics,
            "coverage_categories": sorted(file_coverage),
            "coverage_metrics_valid": coverage_metrics_valid,
            "passed": passed,
            "exit_code": exit_code,
        })
    missing_coverage = sorted(set(COVERAGE_REQUIRED).difference(observed_coverage))
    coverage_passed = not missing_coverage
    status = "PASS" if failed == 0 and coverage_passed else "FAIL"
    if failed:
        reason = "one or more official saves failed real compatibility phases"
    elif not coverage_passed:
        reason = "official save corpus is compatible but required runtime coverage is incomplete"
    else:
        reason = "all official saves passed real load/simulate/save/reload and coverage"
    document = result(
        args.run_id,
        args.commit,
        status,
        reason,
        source_repository=provenance.get("repository"),
        source_revision=provenance.get("revision"),
        provenance_schema=provenance.get("provenance_schema", "omnipack-official-tpt-save-corpus-v1"),
        source_repositories=provenance.get("source_repositories", [provenance.get("repository")]),
        source_kinds=sorted({
            str(row.get("source_kind", "github_git"))
            for row in records if isinstance(row, dict)
        }),
        probe_sha256=probe_sha256,
        files_total=len(records),
        files_passed=len(records) - failed,
        files_failed=failed,
        files=records,
        coverage_contract=COVERAGE_CONTRACT,
        coverage_required=list(COVERAGE_REQUIRED),
        coverage_observed=sorted(observed_coverage),
        coverage_missing=missing_coverage,
        coverage_passed=coverage_passed,
    )
    write(args.output, document)
    # The aggregate coverage contract is part of the gate result.  A corpus
    # whose individual files pass but which misses a required category is a
    # FAIL and must never exit zero merely because files_failed == 0.
    return 0 if status == "PASS" else 1


if __name__ == "__main__":
    raise SystemExit(main())
