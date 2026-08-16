#!/usr/bin/env python3
"""Exercise release-gate bypass attempts; every attack must be rejected."""

from __future__ import annotations

import argparse
from datetime import datetime, timezone
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import re
import shutil
import struct
import sys
import tempfile
import zipfile


TOOLS = Path(__file__).resolve().parent


def load(name: str, path: Path):
    spec = importlib.util.spec_from_file_location(name, path)
    if not spec or not spec.loader:
        raise RuntimeError(f"cannot import {path}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


audit = load("negative_release_validation_audit", TOOLS / "release_validation_audit.py")
provenance = load("negative_official_tpt_provenance", TOOLS / "official_tpt_provenance.py")
package_audit = load("negative_test_release_audit", TOOLS / "test_release_audit.py")
packager = load("negative_package_test_release", TOOLS / "package_test_release.py")
finalizer = load("negative_release_finalizer", TOOLS / "finalize_release_1_1_0.py")

RUN_ID = "20260814T041500Z-8f31c1c7"
COMMIT = "a" * 40
CANDIDATE = "B" * 64
SYMBOLS = "C" * 64
SYMBOL_MEMBER = "D" * 64


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest().upper()


def candidate_executable_digest(path: Path) -> str:
    with zipfile.ZipFile(path) as archive:
        members = [
            info for info in archive.infolist()
            if not info.is_dir()
            and info.filename.replace("\\", "/").endswith("/tpt-zh-omnipack.exe")
        ]
        if len(members) != 1:
            raise ValueError("candidate fixture must contain exactly one target executable")
        return hashlib.sha256(archive.read(members[0])).hexdigest().upper()


def write_json(path: Path, value: object) -> None:
    path.write_text(json.dumps(value, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


def portable_runtime_payload(
    *, run_id: str | None = None, candidate_sha256: str | None = None,
    overrides: dict[str, object] | None = None,
) -> dict[str, object]:
    run_id = run_id or RUN_ID
    candidate_sha256 = candidate_sha256 or CANDIDATE
    value: dict[str, object] = {
        "schema": audit.EVIDENCE_SCHEMA,
        "schema_version": audit.EVIDENCE_SCHEMA_VERSION,
        "payload_schema_version": audit.PORTABLE_RUNTIME_PAYLOAD_SCHEMA_VERSION,
        "test": "portable_runtime",
        "run_id": run_id,
        "candidate_sha256": candidate_sha256,
        "status": "PASS",
        "passed": True,
        "initial_particle_count": 2,
        "post_step_particle_count": 2,
        "final_particle_count": 2,
        "atmosphere_non_finite_cells": 0,
    }
    value.update({field: True for field in audit.PORTABLE_RUNTIME_TRUE_FIELDS})
    value.update(overrides or {})
    return value


def portable_runtime_outer(
    candidate: Path, *, candidate_sha256: str | None = None,
    overrides: dict[str, object] | None = None,
) -> dict[str, object]:
    candidate_sha256 = candidate_sha256 or CANDIDATE
    value: dict[str, object] = {
        "status": "PASS",
        "passed": True,
        "candidate_filename": candidate.name,
        "candidate_extracted_to_fresh_directory": True,
        "candidate_source_markers_checked": True,
        "candidate_source_tree_indicators": [],
        "runtime_target_executable_count": 1,
        "sanitized_path_used": True,
        "runtime_payload_schema_version": audit.PORTABLE_RUNTIME_PAYLOAD_SCHEMA_VERSION,
        "clean_shutdown": True,
        "initial_particle_count": 2,
        "post_step_particle_count": 2,
        "final_particle_count": 2,
        "atmosphere_non_finite_cells": 0,
    }
    value.update({field: True for field in audit.PORTABLE_RUNTIME_TRUE_FIELDS})
    value.update(overrides or {})
    return value


def cpu_fallback_payload(
    *, overrides: dict[str, object] | None = None,
) -> dict[str, object]:
    value: dict[str, object] = {
        "status": "PASS",
        "passed": True,
        "backend": "CPU",
        "fallback": True,
        "reason": "production_runtime_failure_same_step_cpu_fallback",
        "runtime_executor_invocations": 1,
        "backend_available_after_failure": False,
        "control_nonfinite_cells": 0,
        "initialization_nonfinite_cells": 0,
        "runtime_nonfinite_cells": 0,
        "nonfinite_cells": 0,
        "mass_residual_kg": 0.0,
        "energy_residual_j": 0.0,
        "runtime_failure_code": "injected_runtime_thermal_failure",
        "backend_detail": "runtime thermal diffusion fallback: injected_runtime_thermal_failure",
    }
    value.update({field: True for field in audit.CPU_FALLBACK_TRUE_FIELDS})
    value.update(overrides or {})
    return value


def soak_payload(
    candidate_executable_sha256: str,
    *,
    overrides: dict[str, object] | None = None,
) -> dict[str, object]:
    value: dict[str, object] = {
        "status": "PASS", "passed": True,
        "sample_id": "S20-FULL-CATALOG", "long_run_requested": True,
        "wall_clock_seconds": 7200.0, "warmup_seconds": 60.0,
        "sample_seconds": 7200.0, "heartbeat_interval_seconds": 60.0,
        "maximum_heartbeat_gap_seconds": 60.0,
        "long_run_gate_pass": True, "performance_gate_pass": True,
        "duration_pass": True, "runtime_pass": True,
        "sample_execution_pass": True, "event_evidence_complete": True,
        "signal_evidence_complete": True, "signal_behavior_pass": True,
        "signal_stop_pass": True, "fixture_evidence_complete": True,
        "fixture_activity_pass": True, "scenario_behavior_pass": True,
        "long_run_evidence_complete": True, "long_run_duration_pass": True,
        "long_run_behavior_pass": True,
        "long_run_save_load_cycles": 10, "long_run_language_switches": 10,
        "long_run_module_toggle_cycles": 10,
        "long_run_settings_recovery_pass": True,
        "omni_atmosphere_active": True, "public_zip_sha256": CANDIDATE,
        "source_commit": COMMIT, "nan_count": 0, "inf_count": 0,
        "stalls": 0, "simulation_steps": 1000, "heartbeat_count": 121,
        "heartbeat_progress_pass": True, "heartbeat_timing_pass": True,
        "finite_state_pass": True, "atmosphere_range_pass": True,
        "atmosphere_mass_closure_pass": True, "heartbeat_summary_match": True,
        "long_run_diagnostics_pass": True,
        "candidate_extracted_to_fresh_directory": True,
        "candidate_target_executable_count": 1,
        "executable_source": "candidate_zip",
        "candidate_executable_sha256": candidate_executable_sha256,
        "build_executable_sha256": candidate_executable_sha256,
        "exe_sha256": candidate_executable_sha256,
        "peak_working_set_bytes": 128 * 1024 * 1024,
        "peak_private_bytes": 96 * 1024 * 1024,
        "frame_samples": 7200, "process_samples": 7200,
        "memory_process_samples": 7200,
        "observed_particle_min": 100, "observed_particle_max": 200,
        "particle_tail_first": 150, "particle_tail_last": 150,
        "unbounded_growth": False,
        "working_set_tail_first": 100 * 1024 * 1024,
        "working_set_tail_last": 100 * 1024 * 1024,
        "private_tail_first": 80 * 1024 * 1024,
        "private_tail_last": 80 * 1024 * 1024,
        "memory_leak_suspected": False,
        "atmosphere_mass_initial_kg": 1.0,
        "atmosphere_mass_final_kg": 1.0,
        "atmosphere_mass_min_kg": 1.0,
        "atmosphere_mass_max_kg": 1.0,
        "atmosphere_mass_residual_abs_max_kg": 0.0,
        "species_mass_residual_abs_max_kg": 0.0,
        "minimum_density_kg_m3": 1.0, "maximum_density_kg_m3": 1.0,
        "minimum_pressure_pa": 100000.0, "maximum_pressure_pa": 100000.0,
        "minimum_temperature_k": 293.15, "maximum_temperature_k": 293.15,
        "result_json_sha256": "8" * 64,
        "input_ops": {"bytes": 128, "sha256": "6" * 64},
        "output_ops": {"bytes": 256, "sha256": "7" * 64},
    }
    value.update(overrides or {})
    return value


def portable_runtime_case(
    root: Path,
    candidate: Path,
    *,
    inner_overrides: dict[str, object] | None = None,
    outer_overrides: dict[str, object] | None = None,
) -> tuple[bool, bool]:
    stdout = root / "windows_portable_extraction.stdout.txt"
    stderr = root / "windows_portable_extraction.stderr.txt"
    inner = root / "windows_portable_extraction.inner.json"
    stdout.write_text("portable runtime PASS\n", encoding="utf-8")
    stderr.write_text("", encoding="utf-8")
    write_json(inner, portable_runtime_payload(overrides=inner_overrides))
    raw = portable_runtime_outer(candidate)
    raw.update({
        "project_build_tool_dependency_used": True,
        "runtime_stdout": stdout.name,
        "runtime_stdout_sha256": digest(stdout),
        "runtime_stderr": stderr.name,
        "runtime_stderr_sha256": digest(stderr),
        "runtime_inner_evidence": inner.name,
        "runtime_inner_evidence_sha256": digest(inner),
    })
    raw.update(outer_overrides or {})
    return semantic_case(
        root,
        "WindowsPortableExtraction",
        "windows_portable_extraction",
        raw,
        evidence_candidate=CANDIDATE,
        candidate_path=candidate,
    )


def write_solid_bmp(path: Path, width: int, height: int, color: int = 0) -> None:
    row_stride = ((width * 24 + 31) // 32) * 4
    pixel_bytes = bytearray(row_stride * height)
    blue, green, red = color & 0xFF, (color >> 8) & 0xFF, (color >> 16) & 0xFF
    for y in range(height):
        for x in range(width):
            offset = y * row_stride + x * 3
            pixel_bytes[offset:offset + 3] = bytes((blue, green, red))
    header = b"BM" + struct.pack("<IHHI", 54 + len(pixel_bytes), 0, 0, 54)
    dib = struct.pack("<IiiHHIIiiII", 40, width, height, 1, 24, 0, len(pixel_bytes), 0, 0, 0, 0)
    path.write_bytes(header + dib + pixel_bytes)


def write_two_color_bmp(path: Path, width: int, height: int) -> None:
    row_stride = ((width * 24 + 31) // 32) * 4
    pixel_bytes = bytearray(row_stride * height)
    colors = ((0x11, 0x72, 0xA9), (0xE8, 0xD4, 0x3A))
    for y in range(height):
        for x in range(width):
            blue, green, red = colors[0 if x < width // 2 else 1]
            offset = y * row_stride + x * 3
            pixel_bytes[offset:offset + 3] = bytes((blue, green, red))
    header = b"BM" + struct.pack("<IHHI", 54 + len(pixel_bytes), 0, 0, 54)
    dib = struct.pack("<IiiHHIIiiII", 40, width, height, 1, 24, 0, len(pixel_bytes), 0, 0, 0, 0)
    path.write_bytes(header + dib + pixel_bytes)


def semantic_case(
    root: Path,
    gate_name: str,
    test: str,
    raw: dict[str, object],
    *,
    gate_status: str = "PASS",
    evidence_status: str = "PASS",
    evidence_passed: bool = True,
    evidence_run_id: str | None = None,
    evidence_candidate: str | None = None,
    decorate_raw: bool = True,
    candidate_path: Path | None = None,
    symbols_path: Path | None = None,
) -> tuple[bool, bool]:
    raw = dict(raw)
    if evidence_run_id is None:
        evidence_run_id = RUN_ID
    if evidence_candidate is None and gate_name in audit.CANDIDATE_BOUND_GATES:
        evidence_candidate = CANDIDATE
    if decorate_raw:
        raw.setdefault("schema", audit.EVIDENCE_SCHEMA)
        raw.setdefault("schema_version", audit.EVIDENCE_SCHEMA_VERSION)
        raw.setdefault("test", test)
        raw.setdefault("gate_name", gate_name)
        raw.setdefault("run_id", RUN_ID)
        raw.setdefault("commit", COMMIT)
        raw.setdefault("candidate_sha256", evidence_candidate)
        if gate_name in audit.SYMBOL_BOUND_GATES:
            raw.setdefault("symbols_sha256", SYMBOLS)
            raw.setdefault("symbols_member_sha256", SYMBOL_MEMBER)
        raw.setdefault("gate_started_at", "2000-01-01T00:00:00+00:00")
        raw.setdefault("gate_finished_at", "2000-01-01T00:00:01+00:00")
    raw_path = root / "raw.json"
    write_json(raw_path, raw)
    evidence = {
        "schema": audit.EVIDENCE_SCHEMA,
        "schema_version": audit.EVIDENCE_SCHEMA_VERSION,
        "test": test,
        "gate_name": gate_name,
        "run_id": evidence_run_id,
        "commit": COMMIT,
        "status": evidence_status,
        "passed": evidence_passed,
        "exit_code": 0 if evidence_status == "PASS" else 1,
        "candidate_sha256": evidence_candidate,
        "symbols_sha256": SYMBOLS if gate_name in audit.SYMBOL_BOUND_GATES else None,
        "symbols_member_sha256": SYMBOL_MEMBER if gate_name in audit.SYMBOL_BOUND_GATES else None,
        "gate_started_at": "2000-01-01T00:00:00+00:00",
        "gate_finished_at": "2000-01-01T00:00:01+00:00",
        "source_evidence": raw_path.name,
        "source_evidence_sha256": digest(raw_path),
    }
    evidence_path = root / "gate.json"
    write_json(evidence_path, evidence)
    validation = {
        "run_id": RUN_ID,
        "commit": COMMIT,
        "source_snapshot_sha256": "A" * 64,
        "build_inputs_sha256": "9" * 64,
        "candidate_sha256": CANDIDATE,
        "symbols_sha256": SYMBOLS,
        "symbols_member_sha256": SYMBOL_MEMBER,
        "gates": {
            gate_name: {
                "Status": gate_status,
                "ExitCode": 0 if gate_status == "PASS" else 1,
                "Evidence": evidence_path.name,
                "EvidenceSha256": digest(evidence_path),
            }
        },
    }
    return audit.audit_evidence(
        validation, root, candidate_path=candidate_path, symbols_path=symbols_path
    )[:2]


def trusted_adapter_case(
    root: Path,
    *,
    producer_hash: str | None = None,
    producer_overrides: dict[str, object] | None = None,
    adapter_overrides: dict[str, object] | None = None,
) -> tuple[bool, bool]:
    producer = root / "adapter-producer.json"
    producer_value: dict[str, object] = {
        "schema": audit.EVIDENCE_SCHEMA,
        "schema_version": audit.EVIDENCE_SCHEMA_VERSION,
        "test": "gpu_validation",
        "status": "PASS",
        "passed": True,
        "supported": True,
        "fallback": False,
        "backend": "SDL_GPU Vulkan",
        "reason": "PASS",
        "max_abs_error": 0.0,
        "max_rel_error": 0.0,
        "first_mismatch_index": -1,
    }
    producer_value.update(producer_overrides or {})
    write_json(producer, producer_value)

    adapter_value = dict(producer_value)
    adapter_value.update(adapter_overrides or {})
    adapter_value.update({
        "gate_name": "GPUNumericalValidation",
        "run_id": RUN_ID,
        "commit": COMMIT,
        "candidate_sha256": None,
        "gate_started_at": "2000-01-01T00:00:00+00:00",
        "gate_finished_at": "2000-01-01T00:00:01+00:00",
        "identity_binding": "trusted_release_driver_fresh_process_adapter",
        "producer_evidence": producer.name,
        "producer_evidence_sha256": producer_hash or digest(producer),
    })
    adapter = root / "adapter-bound.json"
    write_json(adapter, adapter_value)
    envelope = root / "adapter-gate.json"
    write_json(envelope, {
        "schema": audit.EVIDENCE_SCHEMA,
        "schema_version": audit.EVIDENCE_SCHEMA_VERSION,
        "test": "gpu_validation",
        "gate_name": "GPUNumericalValidation",
        "run_id": RUN_ID,
        "commit": COMMIT,
        "status": "PASS",
        "passed": True,
        "exit_code": 0,
        "candidate_sha256": None,
        "gate_started_at": "2000-01-01T00:00:00+00:00",
        "gate_finished_at": "2000-01-01T00:00:01+00:00",
        "source_evidence": adapter.name,
        "source_evidence_sha256": digest(adapter),
    })
    validation = {
        "run_id": RUN_ID,
        "commit": COMMIT,
        "source_snapshot_sha256": "A" * 64,
        "build_inputs_sha256": "9" * 64,
        "candidate_sha256": CANDIDATE,
        "symbols_sha256": SYMBOLS,
        "symbols_member_sha256": SYMBOL_MEMBER,
        "gates": {
            "GPUNumericalValidation": {
                "Status": "PASS",
                "ExitCode": 0,
                "Evidence": envelope.name,
                "EvidenceSha256": digest(envelope),
            }
        },
    }
    return audit.audit_evidence(validation, root)[:2]


def clean_machine_adapter_case(
    root: Path,
    candidate: Path,
    *,
    evidence_candidate: str | None = None,
    producer_overrides: dict[str, object] | None = None,
    binding_overrides: dict[str, object] | None = None,
    omit_binding_metadata: bool = False,
    replace_validator_after_binding: bool = False,
) -> tuple[bool, bool]:
    evidence_candidate = evidence_candidate or CANDIDATE
    stdout = root / "windows_clean_machine.stdout.txt"
    stderr = root / "windows_clean_machine.stderr.txt"
    inner = root / "windows_clean_machine.inner.json"
    stdout.write_text("", encoding="utf-8")
    stderr.write_text("", encoding="utf-8")
    write_json(inner, portable_runtime_payload(candidate_sha256=evidence_candidate))
    validator = root / "runtime-validator" / "test_clean_release.ps1"
    validator.parent.mkdir(parents=True, exist_ok=True)
    validator.write_text("# trusted runtime validator\n", encoding="utf-8")
    validator_sha = digest(validator)
    producer_value: dict[str, object] = {
        "schema": audit.EVIDENCE_SCHEMA,
        "schema_version": audit.EVIDENCE_SCHEMA_VERSION,
        "test": "windows_clean_machine",
        "run_id": RUN_ID,
        "status": "PASS",
        "passed": True,
        "candidate_filename": candidate.name,
        "candidate_sha256": evidence_candidate,
        "source_checkout_used": False,
        "project_build_tool_dependency_used": False,
        "source_tree_indicators": [],
        "source_markers_checked": True,
        "development_tool_probe_completed": True,
        "development_tools_detected": {
            name: None for name in (
                "gcc.exe", "g++.exe", "meson.exe", "ninja.exe", "glslc.exe", "bash.exe"
            )
        },
        "runtime_validator": "test_clean_release.ps1",
        "runtime_validator_sha256": validator_sha,
        "validator_binding_checked": True,
        "runtime_job_kind": "runtime-only",
        "runtime_stdout": stdout.name,
        "runtime_stdout_sha256": digest(stdout),
        "runtime_stderr": stderr.name,
        "runtime_stderr_sha256": digest(stderr),
        "runtime_inner_evidence": inner.name,
        "runtime_inner_evidence_sha256": digest(inner),
        "gate_started_at": "2000-01-01T00:00:00+00:00",
        "gate_finished_at": "2000-01-01T00:00:01+00:00",
    }
    producer_value.update(portable_runtime_outer(candidate, candidate_sha256=evidence_candidate))
    producer_value.update(producer_overrides or {})
    producer = root / "windows-clean-machine.json"
    write_json(producer, producer_value)
    binding_value: dict[str, object] = {
        "schema": audit.EVIDENCE_SCHEMA,
        "schema_version": audit.EVIDENCE_SCHEMA_VERSION,
        "test": "runtime_validator_binding",
        "run_id": RUN_ID,
        "commit": COMMIT,
        "status": "PASS",
        "passed": True,
        "candidate_filename": candidate.name,
        "candidate_sha256": evidence_candidate,
        "runtime_validator_filename": "test_clean_release.ps1",
        "runtime_validator_sha256": validator_sha,
        "clean_machine_evidence_filename": producer.name,
        "clean_machine_evidence_sha256": digest(producer),
        "created_at": "2000-01-01T00:00:02+00:00",
    }
    binding_value.update(binding_overrides or {})
    binding = root / "runtime-validator-binding.json"
    write_json(binding, binding_value)
    adapter_value = dict(producer_value)
    adapter_value.update({
        "gate_name": "WindowsCleanMachine",
        "run_id": RUN_ID,
        "commit": COMMIT,
        "candidate_sha256": evidence_candidate,
        "gate_started_at": "2000-01-01T00:00:00+00:00",
        "gate_finished_at": "2000-01-01T00:00:01+00:00",
        "identity_binding": "trusted_finalizer_import_adapter",
        "producer_evidence": producer.name,
        "producer_evidence_sha256": digest(producer),
    })
    if not omit_binding_metadata:
        adapter_value.update({
            "runtime_validator_binding": binding.name,
            "runtime_validator_binding_sha256": digest(binding),
            "runtime_validator_artifact": validator.relative_to(root).as_posix(),
            "runtime_validator_artifact_sha256": validator_sha,
        })
    adapter = root / "bound-windowscleanmachine.json"
    write_json(adapter, adapter_value)
    if replace_validator_after_binding:
        validator.write_text("# replaced runtime validator\n", encoding="utf-8")
    envelope = root / "gate-windowscleanmachine.json"
    write_json(envelope, {
        "schema": audit.EVIDENCE_SCHEMA,
        "schema_version": audit.EVIDENCE_SCHEMA_VERSION,
        "test": "windows_clean_machine",
        "gate_name": "WindowsCleanMachine",
        "run_id": RUN_ID,
        "commit": COMMIT,
        "status": "PASS",
        "passed": True,
        "exit_code": 0,
        "candidate_sha256": evidence_candidate,
        "gate_started_at": "2000-01-01T00:00:00+00:00",
        "gate_finished_at": "2000-01-01T00:00:01+00:00",
        "source_evidence": adapter.name,
        "source_evidence_sha256": digest(adapter),
    })
    validation = {
        "run_id": RUN_ID,
        "commit": COMMIT,
        "source_snapshot_sha256": "A" * 64,
        "build_inputs_sha256": "9" * 64,
        "candidate_sha256": CANDIDATE,
        "symbols_sha256": SYMBOLS,
        "symbols_member_sha256": SYMBOL_MEMBER,
        "gates": {
            "WindowsCleanMachine": {
                "Status": "PASS",
                "ExitCode": 0,
                "Evidence": envelope.name,
                "EvidenceSha256": digest(envelope),
            }
        },
    }
    return audit.audit_evidence(validation, root, candidate_path=candidate)[:2]


def official_compatibility_case(
    root: Path,
    *,
    compatibility_status: str = "PASS",
    compatibility_passed: bool = True,
    compatibility_row_overrides: dict[str, object] | None = None,
) -> tuple[bool, bool]:
    revision = "c" * 40
    fixture_sha = "1" * 64
    started = "2000-01-01T00:00:00+00:00"
    finished = "2000-01-01T00:00:01+00:00"
    provenance_raw = {
        "schema": audit.EVIDENCE_SCHEMA,
        "schema_version": audit.EVIDENCE_SCHEMA_VERSION,
        "test": "official_tpt_provenance",
        "gate_name": "OfficialTPTCorpusProvenance",
        "run_id": RUN_ID, "commit": COMMIT,
        "status": "PASS", "passed": True,
        "candidate_sha256": None, "gate_started_at": started,
        "gate_finished_at": finished,
        "repository": provenance.OFFICIAL_REPOSITORY,
        "revision": revision, "revision_exists": True,
        "revision_reachable_from_official_remote": True,
        "files_total": 1, "files_verified": 1, "files_failed": 0,
        "files": [{
            "path": "tests/saves/official.cps", "match": True,
            "upstream_sha256": fixture_sha, "manifest_sha256": fixture_sha,
            "fixture_sha256": fixture_sha,
        }],
    }
    compatibility_raw = {
        "schema": audit.EVIDENCE_SCHEMA,
        "schema_version": audit.EVIDENCE_SCHEMA_VERSION,
        "test": "official_tpt_save_compatibility",
        "gate_name": "OfficialTPTSaveCompatibility",
        "run_id": RUN_ID, "commit": COMMIT,
        "status": compatibility_status, "passed": compatibility_passed,
        "candidate_sha256": None, "gate_started_at": started,
        "gate_finished_at": finished,
        "source_repository": provenance.OFFICIAL_REPOSITORY,
        "source_revision": revision,
        "probe_sha256": "2" * 64,
        "files_total": 1, "files_passed": 1, "files_failed": 0,
        "files": [{
            "path": "tests/saves/official.cps", "fixture_sha256": fixture_sha,
            "probe_sha256": "2" * 64,
            "provenance_hash_binding_passed": True,
            "passed": True, "exit_code": 0,
            "load": True, "missing_elements_zero": True, "initial_load_state_validate": True,
            "simulate": True, "save": True,
            "reload": True, "state_validate": True,
            "input_particles": 1, "initial_loaded_particles": 1,
            "output_particles": 1, "initial_particle_inventory": True,
            "negative_block_map": True, "negative_legacy_field": True,
            "negative_sign": True, "negative_validity_mask": True,
            "negative_deterministic_frame": True,
            "negative_simulation_option": True,
            "negative_codec_roundtrip": True,
        }],
    }
    compatibility_raw["files"][0].update(compatibility_row_overrides or {})
    gates: dict[str, dict[str, object]] = {}
    for gate_name, test, raw, prefix in (
        ("OfficialTPTCorpusProvenance", "official_tpt_provenance", provenance_raw, "provenance"),
        ("OfficialTPTSaveCompatibility", "official_tpt_save_compatibility", compatibility_raw, "compatibility"),
    ):
        raw_path = root / f"{prefix}-raw.json"
        write_json(raw_path, raw)
        envelope = {
            "schema": audit.EVIDENCE_SCHEMA,
            "schema_version": audit.EVIDENCE_SCHEMA_VERSION,
            "test": test, "gate_name": gate_name,
            "run_id": RUN_ID, "commit": COMMIT,
            "status": "PASS", "passed": True, "exit_code": 0,
            "candidate_sha256": None, "symbols_sha256": None,
            "symbols_member_sha256": None,
            "gate_started_at": started, "gate_finished_at": finished,
            "source_evidence": raw_path.name,
            "source_evidence_sha256": digest(raw_path),
        }
        envelope_path = root / f"{prefix}-gate.json"
        write_json(envelope_path, envelope)
        gates[gate_name] = {
            "Status": "PASS", "ExitCode": 0,
            "Evidence": envelope_path.name,
            "EvidenceSha256": digest(envelope_path),
        }
    validation = {
        "run_id": RUN_ID, "commit": COMMIT,
        "source_snapshot_sha256": "A" * 64,
        "build_inputs_sha256": "9" * 64,
        "candidate_sha256": CANDIDATE,
        "symbols_sha256": SYMBOLS,
        "symbols_member_sha256": SYMBOL_MEMBER,
        "gates": gates,
    }
    return audit.audit_evidence(validation, root)[:2]


class FakeUpstream:
    def __init__(self, *, url: str = provenance.OFFICIAL_REPOSITORY,
                 exists: bool = True, reachable: bool = True,
                 blobs: dict[str, bytes] | None = None) -> None:
        self.url = url
        self.exists = exists
        self.reachable = reachable
        self.blobs = blobs or {}

    def repository_url(self) -> str:
        return self.url

    def fetch(self) -> None:
        return None

    def revision_exists(self, revision: str) -> bool:
        return self.exists

    def revision_reachable(self, revision: str) -> bool:
        return self.reachable

    def blob(self, revision: str, path: str) -> bytes:
        if path not in self.blobs:
            raise FileNotFoundError(path)
        return self.blobs[path]


def provenance_attacks(root: Path) -> tuple[dict[str, bool], bool, list[str]]:
    corpus = root / "corpus"
    path = "tests/saves/official.cps"
    fixture = corpus / path
    fixture.parent.mkdir(parents=True)
    upstream_bytes = b"official-git-object"
    fixture.write_bytes(upstream_bytes)
    revision = "c" * 40
    sha = hashlib.sha256(upstream_bytes).hexdigest()
    base = {
        "schema": "omnipack-official-tpt-save-corpus-v1",
        "corpus_id": "negative-suite",
        "source_repository": provenance.OFFICIAL_REPOSITORY,
        "source_revision": revision,
        "retrieved_at": "2026-08-14",
        "redistribution": {"status": "local_only_not_for_redistribution", "basis": "negative test"},
        "files": [{
            "path": path,
            "sha256": sha,
            "source_locator": f"{provenance.OFFICIAL_REPOSITORY}/blob/{revision}/{path}",
        }],
    }
    baseline = provenance.validate_provenance(
        corpus,
        base,
        FakeUpstream(blobs={path: upstream_bytes}),
        run_id=RUN_ID,
        commit=COMMIT,
    )
    baseline_semantic_errors = audit.validate_raw_semantics(
        "OfficialTPTCorpusProvenance",
        baseline,
        root=root,
        candidate_sha256=CANDIDATE,
        commit=COMMIT,
    )
    baseline_passed = (
        baseline.get("passed") is True
        and baseline.get("status") == "PASS"
        and not baseline_semantic_errors
    )
    details: list[str] = []
    if not baseline_passed:
        details.append(
            "official provenance positive baseline was rejected: "
            + json.dumps(baseline, ensure_ascii=False, sort_keys=True)
        )
        details.extend(
            f"official provenance semantic baseline: {error}"
            for error in baseline_semantic_errors
        )

    attacks: list[tuple[str, dict[str, object], FakeUpstream, bytes]] = []
    fake = json.loads(json.dumps(base)); fake["source_revision"] = "0" * 40
    attacks.append(("fake_official_revision", fake, FakeUpstream(exists=False, blobs={path: upstream_bytes}), upstream_bytes))
    wrong_path = json.loads(json.dumps(base)); wrong_path["files"][0]["path"] = "tests/saves/missing.cps"; wrong_path["files"][0]["source_locator"] = f"{provenance.OFFICIAL_REPOSITORY}/blob/{revision}/tests/saves/missing.cps"
    attacks.append(("wrong_official_path", wrong_path, FakeUpstream(blobs={path: upstream_bytes}), upstream_bytes))
    attacks.append(("modified_official_fixture", json.loads(json.dumps(base)), FakeUpstream(blobs={path: upstream_bytes}), b"modified"))
    wrong_hash = json.loads(json.dumps(base)); wrong_hash["files"][0]["sha256"] = "0" * 64
    attacks.append(("wrong_official_manifest_hash", wrong_hash, FakeUpstream(blobs={path: upstream_bytes}), upstream_bytes))
    wrong_locator = json.loads(json.dumps(base)); wrong_locator["files"][0]["source_locator"] = f"{provenance.OFFICIAL_REPOSITORY}/blob/{revision}/different.cps"
    attacks.append(("wrong_official_locator", wrong_locator, FakeUpstream(blobs={path: upstream_bytes}), upstream_bytes))
    wrong_repo = json.loads(json.dumps(base)); wrong_repo["source_repository"] = "https://example.invalid/fake"
    attacks.append(("wrong_official_repository", wrong_repo, FakeUpstream(blobs={path: upstream_bytes}), upstream_bytes))
    results: dict[str, bool] = {}
    for name, manifest, upstream, local_bytes in attacks:
        fixture.write_bytes(local_bytes)
        result = provenance.validate_provenance(corpus, manifest, upstream, run_id=RUN_ID, commit=COMMIT)
        results[name] = baseline_passed and result.get("passed") is False and result.get("status") == "FAIL"
    return results, baseline_passed, details


def promotion_attacks(
    root: Path, candidate: Path | None, symbols: Path | None
) -> tuple[dict[str, bool], bool, list[str]]:
    if candidate is None or symbols is None or not candidate.is_file() or not symbols.is_file():
        return ({name: False for name in (
            "promotion_wrong_candidate_filename", "promotion_wrong_symbols_candidate_filename",
            "promotion_wrong_stable_filename", "promotion_wrong_stable_sha256",
            "promotion_wrong_stable_symbols_filename", "promotion_wrong_stable_symbols_sha256",
            "promotion_byte_identity_false", "promotion_atomic_rename_false",
            "promotion_atomic_directory_false", "promotion_lock_false",
            "promotion_marker_false", "promotion_stable_target_preexists",
        )}, False, ["promotion positive baseline artifacts are absent"])
    candidate_sha = digest(candidate)
    symbols_sha = digest(symbols)
    base = {
        "schema": audit.EVIDENCE_SCHEMA,
        "schema_version": audit.EVIDENCE_SCHEMA_VERSION,
        "test": "candidate_promotion",
        "run_id": RUN_ID,
        "commit": COMMIT,
        "status": "PASS",
        "passed": True,
        "candidate_filename": f"TPT-ZH-OmniPack-1.1.0-staging-{RUN_ID}-Windows-x64-SDL3.zip",
        "candidate_sha256": candidate_sha,
        "stable_filename": "TPT-ZH-OmniPack-1.1.0-Windows-x64-SDL3.zip",
        "stable_sha256_expected": candidate_sha,
        "symbols_candidate_filename": f"TPT-ZH-OmniPack-1.1.0-staging-{RUN_ID}-Windows-x64-Symbols.zip",
        "symbols_sha256": symbols_sha,
        "stable_symbols_filename": "TPT-ZH-OmniPack-1.1.0-Windows-x64-Symbols.zip",
        "stable_symbols_sha256_expected": symbols_sha,
        "symbols_member_sha256": SYMBOL_MEMBER,
        "promotion_phase": "prepared_for_atomic_directory_publish",
        "stable_copy_prepared": True,
        "stable_sha256_observed": candidate_sha,
        "stable_symbols_copy_prepared": True,
        "stable_symbols_sha256_observed": symbols_sha,
        "byte_for_byte_identity": True,
        "atomic_rename_only": True,
        "atomic_directory_publish": True,
        "exclusive_output_lock_acquired": True,
        "promotion_complete_marker_required": True,
        "stable_names_absent_before_final_audit": True,
        "gate_started_at": "2000-01-01T00:00:00+00:00",
        "gate_finished_at": "2000-01-01T00:00:01+00:00",
    }
    mutations: dict[str, tuple[str, object]] = {
        "promotion_wrong_candidate_filename": ("candidate_filename", "old-candidate.zip"),
        "promotion_wrong_symbols_candidate_filename": ("symbols_candidate_filename", "old-symbols.zip"),
        "promotion_wrong_stable_filename": ("stable_filename", "wrong-stable.zip"),
        "promotion_wrong_stable_sha256": ("stable_sha256_expected", "E" * 64),
        "promotion_wrong_stable_symbols_filename": ("stable_symbols_filename", "wrong-symbols.zip"),
        "promotion_wrong_stable_symbols_sha256": ("stable_symbols_sha256_expected", "F" * 64),
        "promotion_byte_identity_false": ("byte_for_byte_identity", False),
        "promotion_atomic_rename_false": ("atomic_rename_only", False),
        "promotion_atomic_directory_false": ("atomic_directory_publish", False),
        "promotion_lock_false": ("exclusive_output_lock_acquired", False),
        "promotion_marker_false": ("promotion_complete_marker_required", False),
        "promotion_stable_target_preexists": ("stable_names_absent_before_final_audit", False),
    }
    baseline_errors = audit.validate_raw_semantics(
        "CandidatePromotion", base, root=root,
        candidate_sha256=candidate_sha,
        candidate_path=candidate, symbols_path=symbols,
    )
    if baseline_errors:
        return (
            {name: False for name in mutations},
            False,
            [f"promotion positive baseline: {error}" for error in baseline_errors],
        )
    results: dict[str, bool] = {}
    for name, (field, value) in mutations.items():
        raw = dict(base)
        raw[field] = value
        # semantic_case cannot carry real artifact paths; evaluate the raw
        # schema directly against the exact candidate/symbol bytes so a broken
        # baseline cannot make every mutation look rejected.
        errors = audit.validate_raw_semantics(
            "CandidatePromotion", raw, root=root,
            candidate_sha256=candidate_sha,
            candidate_path=candidate, symbols_path=symbols,
        )
        results[name] = bool(errors)
    return results, True, []


def corrupt_manifest(candidate: Path, destination: Path) -> None:
    with zipfile.ZipFile(candidate) as source, zipfile.ZipFile(destination, "w", zipfile.ZIP_DEFLATED) as target:
        for info in source.infolist():
            data = source.read(info.filename)
            if info.filename.endswith("/PACKAGE-MANIFEST.sha256"):
                data = data.replace(b"0", b"1", 1) if b"0" in data else b"0" + data
            if info.filename.endswith("/MANIFEST.txt"):
                data += b"ignored-garbage-line\n"
            target.writestr(info, data)


def semantic_positive_baselines(
    root: Path, candidate: Path
) -> tuple[dict[str, bool], list[str]]:
    baselines: dict[str, bool] = {}
    details: list[str] = []

    def check(
        name: str,
        gate_name: str,
        test: str,
        raw: dict[str, object],
        *,
        candidate_bound: bool = False,
    ) -> None:
        hash_ok, semantic_ok = semantic_case(
            root,
            gate_name,
            test,
            raw,
            evidence_candidate=CANDIDATE if candidate_bound else None,
            candidate_path=candidate if candidate_bound else None,
        )
        passed = hash_ok and semantic_ok
        baselines[name] = passed
        if not passed:
            details.append(f"semantic positive baseline was rejected: {name}")

    stdout = root / "baseline-command.stdout.txt"
    stderr = root / "baseline-command.stderr.txt"
    stdout.write_text("baseline command passed\n", encoding="utf-8")
    stderr.write_text("", encoding="utf-8")
    check(
        "command_envelope",
        "Build",
        "build",
        {
            "status": "PASS", "passed": True, "exit_code": 0,
            "stdout": stdout.name, "stdout_sha256": digest(stdout),
            "stderr": stderr.name, "stderr_sha256": digest(stderr),
        },
    )

    fixture_sha = "1" * 64
    compatibility_baseline = {
        "schema": audit.EVIDENCE_SCHEMA,
        "schema_version": audit.EVIDENCE_SCHEMA_VERSION,
        "test": "official_tpt_save_compatibility",
        "status": "PASS", "passed": True,
        "source_repository": provenance.OFFICIAL_REPOSITORY,
        "source_revision": "c" * 40,
        "probe_sha256": "2" * 64,
        "files_total": 1, "files_passed": 1, "files_failed": 0,
        "files": [{
            "path": "tests/saves/official.cps",
            "fixture_sha256": fixture_sha,
            "probe_sha256": "2" * 64,
            "provenance_hash_binding_passed": True,
            "passed": True, "exit_code": 0,
            "load": True, "missing_elements_zero": True, "initial_load_state_validate": True,
            "simulate": True, "save": True,
            "reload": True, "state_validate": True,
            "input_particles": 1, "initial_loaded_particles": 1,
            "output_particles": 1, "initial_particle_inventory": True,
            "negative_block_map": True, "negative_legacy_field": True,
            "negative_sign": True, "negative_validity_mask": True,
            "negative_deterministic_frame": True,
            "negative_simulation_option": True,
            "negative_codec_roundtrip": True,
        }],
    }
    compatibility_errors = audit.validate_raw_semantics(
        "OfficialTPTSaveCompatibility",
        compatibility_baseline,
        root=root,
        candidate_sha256=CANDIDATE,
        commit=COMMIT,
    )
    baselines["official_compatibility_schema"] = not compatibility_errors
    details.extend(
        f"official compatibility positive baseline: {error}"
        for error in compatibility_errors
    )

    portable_hash_ok, portable_semantic_ok = portable_runtime_case(root, candidate)
    baselines["portable_runtime"] = portable_hash_ok and portable_semantic_ok
    if not baselines["portable_runtime"]:
        details.append("portable runtime positive baseline was rejected")

    clean_hash_ok, clean_semantic_ok = clean_machine_adapter_case(root, candidate)
    baselines["clean_runtime"] = clean_hash_ok and clean_semantic_ok
    if not baselines["clean_runtime"]:
        details.append("trusted clean runtime positive baseline was rejected")

    check(
        "source_tree_clean",
        "SourceTreeClean",
        "source_tree_clean",
        {
            "status": "PASS", "passed": True, "porcelain_output": "",
            "content_hash": "A" * 64, "tracked_files": 1,
        },
    )
    snapshot = {
        "status": "PASS", "passed": True,
        "commit_start": COMMIT, "commit_end": COMMIT,
        "branch_start": "integration/omnicore-vnext",
        "branch_end": "integration/omnicore-vnext",
        "git_status_start": "", "git_status_end": "",
    }
    for phase in ("start", "after_configure", "after_build", "before_package", "after_package", "end"):
        snapshot[f"content_hash_{phase}"] = "A" * 64
        snapshot[f"tracked_files_{phase}"] = 1
    for phase in ("after_configure", "after_build", "before_package", "after_package", "end"):
        snapshot[f"build_inputs_hash_{phase}"] = "9" * 64
        snapshot[f"build_inputs_ready_{phase}"] = True
    check(
        "source_snapshot_immutability",
        "SourceSnapshotImmutability",
        "source_snapshot_immutability",
        snapshot,
    )

    check(
        "artifact_immutability",
        "ArtifactImmutability",
        "artifact_immutability",
        {
            "status": "PASS", "passed": True,
            "before_sha256": CANDIDATE, "after_sha256": CANDIDATE,
        },
        candidate_bound=True,
    )

    candidate_executable_sha = candidate_executable_digest(candidate)
    soak = soak_payload(candidate_executable_sha)
    check("soak_2h", "Soak2Hours", "soak_2h", soak, candidate_bound=True)

    screenshot = root / "baseline-gui.bmp"
    write_two_color_bmp(screenshot, 4, 4)
    gui = {
        "status": "PASS", "passed": True, "artifact": screenshot.name,
        "screenshot_width": 4, "screenshot_height": 4,
        "screenshot_bytes": screenshot.stat().st_size,
        "screenshot_pixel_count": 16, "screenshot_nonzero_pixels": 16,
        "screenshot_distinct_colors": 2,
    }
    for field in (
        "window_created", "frame_rendered", "resize", "fullscreen_toggle",
        "keyboard", "mouse", "text_input", "clipboard", "screenshot_created",
        "clean_shutdown", "restart",
    ):
        gui[field] = True
    check("gui", "SDL3GUI", "sdl3_gui", gui, candidate_bound=True)

    check(
        "gpu_validation",
        "GPUNumericalValidation",
        "gpu_validation",
        {
            "status": "PASS", "passed": True, "supported": True,
            "fallback": False, "backend": "SDL_GPU Vulkan", "reason": "PASS",
            "max_abs_error": 0.0, "max_rel_error": 0.0,
            "first_mismatch_index": -1,
        },
    )
    check(
        "cpu_fallback_production_path",
        "CPUFallbackValidation",
        "cpu_fallback",
        cpu_fallback_payload(),
    )
    adapter_hash_ok, adapter_semantic_ok = trusted_adapter_case(root)
    baselines["trusted_adapter_semantic_binding"] = (
        adapter_hash_ok and adapter_semantic_ok
    )
    if not baselines["trusted_adapter_semantic_binding"]:
        details.append("trusted adapter positive baseline was rejected")
    check(
        "meta_evidence_semantic_integrity",
        "EvidenceSemanticIntegrity",
        "release_validation_audit",
        {
            "status": "PASS", "passed": True,
            "evidence_semantic_integrity": True,
            "evidence": [], "documentation_errors": [],
        },
    )
    return baselines, details


def run(candidate: Path | None, symbols: Path | None, artifact_stem: str | None,
        symbol_artifact_stem: str | None) -> tuple[dict[str, bool], dict[str, bool], list[str]]:
    results: dict[str, bool] = {}
    baselines: dict[str, bool] = {}
    details: list[str] = []
    with tempfile.TemporaryDirectory() as temporary:
        root = Path(temporary)

        if candidate is not None and candidate.is_file():
            try:
                semantic_baselines, semantic_details = semantic_positive_baselines(
                    root, candidate
                )
            except (OSError, ValueError, zipfile.BadZipFile) as exc:
                # A malformed candidate is a failed positive baseline, not an
                # infrastructure crash that can hide the remaining attacks.
                baselines["semantic_evidence_families"] = False
                details.append(f"semantic positive baselines rejected candidate: {exc}")
            else:
                baselines.update(semantic_baselines)
                details.extend(semantic_details)
        else:
            baselines["semantic_evidence_families"] = False
            details.append("semantic positive baselines require the actual staging candidate")

        compatibility_hash_ok, compatibility_semantic_ok = official_compatibility_case(root)
        baselines["official_compatibility_envelope_binding"] = (
            compatibility_hash_ok and compatibility_semantic_ok
        )
        if not baselines["official_compatibility_envelope_binding"]:
            details.append("official compatibility envelope/binding positive baseline was rejected")

        try:
            packager.package_stems(packager.STABLE_VERSION)
        except ValueError:
            results["stable_packager_direct_stable_name"] = True
        else:
            results["stable_packager_direct_stable_name"] = False

        stdout = root / "generic.stdout.txt"
        stderr = root / "generic.stderr.txt"
        stdout.write_text("command output\n", encoding="utf-8")
        stderr.write_text("", encoding="utf-8")
        stream_raw = {
            "status": "PASS", "passed": True, "exit_code": 0,
            "stdout": stdout.name, "stdout_sha256": digest(stdout),
            "stderr": stderr.name, "stderr_sha256": digest(stderr),
        }

        _, semantic = official_compatibility_case(
            root, compatibility_status="NOT_TESTED", compatibility_passed=False
        )
        results["aggregate_pass_with_not_tested_evidence"] = not semantic

        _, semantic = official_compatibility_case(
            root, compatibility_status="FAIL", compatibility_passed=False
        )
        results["aggregate_pass_with_failed_evidence"] = not semantic

        _, semantic = official_compatibility_case(
            root,
            compatibility_row_overrides={
                "initial_loaded_particles": 0,
                "initial_particle_inventory": True,
            },
        )
        results["official_initial_load_particle_loss"] = not semantic

        _, semantic = semantic_case(
            root, "Build", "build", stream_raw,
            evidence_run_id="19990101T000000Z-deadbeef",
        )
        results["stale_previous_run_evidence"] = not semantic

        _, semantic = semantic_case(
            root, "Build", "build", stream_raw,
            decorate_raw=False,
        )
        results["raw_evidence_identity_missing"] = not semantic

        meta_validation = {
            "run_id": RUN_ID, "commit": COMMIT, "source_snapshot_sha256": "A" * 64,
            "build_inputs_sha256": "9" * 64,
            "candidate_sha256": CANDIDATE, "symbols_sha256": SYMBOLS,
            "symbols_member_sha256": SYMBOL_MEMBER,
            "gates": {
                name: {
                    "Status": "PASS", "ExitCode": 0,
                    "Evidence": f"missing-{name}.json", "EvidenceSha256": "E" * 64,
                }
                for name in audit.META_GATES
            },
        }
        hash_ok, semantic_ok, _ = audit.audit_evidence(meta_validation, root)
        results["meta_gate_missing_evidence"] = not (hash_ok and semantic_ok)

        nonzero_exit = dict(stream_raw)
        nonzero_exit["exit_code"] = 1
        _, semantic = semantic_case(root, "Build", "build", nonzero_exit)
        results["generic_pass_with_nonzero_exit"] = not semantic
        _, semantic = semantic_case(
            root, "Build", "build", {"status": "PASS", "passed": True, "exit_code": 0},
        )
        results["generic_declaration_only_pass"] = not semantic

        _, semantic = semantic_case(
            root, "WindowsPortableExtraction", "windows_portable_extraction",
            {"status": "PASS", "passed": True}, evidence_candidate=CANDIDATE,
            candidate_path=candidate,
        )
        results["portable_declaration_only_pass"] = not semantic

        _, semantic = portable_runtime_case(
            root, candidate,
            inner_overrides={"post_step_finite_passed": False},
            outer_overrides={"post_step_finite_passed": False},
        )
        results["portable_post_step_false_pass"] = not semantic

        _, semantic = portable_runtime_case(
            root, candidate,
            inner_overrides={"post_step_finite_passed": False},
        )
        results["portable_outer_inner_semantic_mismatch"] = not semantic

        _, semantic = portable_runtime_case(
            root, candidate,
            outer_overrides={"runtime_inner_evidence_sha256": "0" * 64},
        )
        results["portable_inner_hash_mismatch"] = not semantic

        _, semantic = portable_runtime_case(
            root, candidate,
            outer_overrides={"runtime_target_executable_count": 2},
        )
        results["portable_multiple_executables"] = not semantic

        _, semantic = portable_runtime_case(
            root, candidate,
            outer_overrides={
                "candidate_source_tree_indicators": ["candidate/.git"],
            },
        )
        results["portable_candidate_source_marker"] = not semantic

        _, semantic = semantic_case(
            root,
            "CPUFallbackValidation",
            "cpu_fallback",
            {
                "status": "PASS", "passed": True, "backend": "CPU",
                "fallback": True,
                "reason": "GPU_init_failed_CPU_reference_continues",
            },
        )
        results["cpu_fallback_reference_only_declaration"] = not semantic

        _, semantic = semantic_case(
            root, "CPUFallbackValidation", "cpu_fallback",
            cpu_fallback_payload(overrides={"initialization_backend_prearmed": False}),
        )
        results["cpu_fallback_initialization_not_prearmed"] = not semantic

        _, semantic = semantic_case(
            root, "CPUFallbackValidation", "cpu_fallback",
            cpu_fallback_payload(overrides={"runtime_executor_invocations": 0}),
        )
        results["cpu_fallback_executor_not_invoked"] = not semantic

        _, semantic = semantic_case(
            root, "CPUFallbackValidation", "cpu_fallback",
            cpu_fallback_payload(overrides={"same_step_cpu_fallback": False}),
        )
        results["cpu_fallback_not_same_step"] = not semantic

        _, semantic = semantic_case(
            root, "CPUFallbackValidation", "cpu_fallback",
            cpu_fallback_payload(overrides={
                "backend_reset_to_cpu": False,
                "backend_available_after_failure": True,
            }),
        )
        results["cpu_fallback_backend_not_reset"] = not semantic

        _, semantic = semantic_case(
            root, "CPUFallbackValidation", "cpu_fallback",
            cpu_fallback_payload(overrides={"cpu_control_state_match": False}),
        )
        results["cpu_fallback_state_mismatch"] = not semantic

        _, semantic = semantic_case(
            root, "CPUFallbackValidation", "cpu_fallback",
            cpu_fallback_payload(overrides={
                "runtime_nonfinite_cells": 1,
                "nonfinite_cells": 1,
            }),
        )
        results["cpu_fallback_nonfinite_state"] = not semantic

        old_marker = os.environ.get("OMNI_CLEAN_MACHINE")
        os.environ["OMNI_CLEAN_MACHINE"] = "true"
        try:
            _, semantic = clean_machine_adapter_case(
                root, candidate, producer_overrides={"source_checkout_used": True},
            )
        finally:
            if old_marker is None:
                os.environ.pop("OMNI_CLEAN_MACHINE", None)
            else:
                os.environ["OMNI_CLEAN_MACHINE"] = old_marker
        results["clean_machine_environment_marker_on_checkout"] = not semantic

        _, semantic = clean_machine_adapter_case(
            root, candidate, evidence_candidate="D" * 64,
        )
        results["wrong_candidate_sha_evidence"] = not semantic

        _, semantic = clean_machine_adapter_case(
            root, candidate, omit_binding_metadata=True,
        )
        results["clean_machine_missing_validator_binding"] = not semantic

        _, semantic = clean_machine_adapter_case(
            root, candidate,
            producer_overrides={"runtime_validator_sha256": "E" * 64},
        )
        results["clean_machine_wrong_validator_sha"] = not semantic

        _, semantic = clean_machine_adapter_case(
            root, candidate,
            binding_overrides={"clean_machine_evidence_sha256": "E" * 64},
        )
        results["clean_machine_binding_clean_hash_mismatch"] = not semantic

        _, semantic = clean_machine_adapter_case(
            root, candidate, replace_validator_after_binding=True,
        )
        results["clean_machine_replaced_validator"] = not semantic

        dirty = {"status": "PASS", "passed": True, "porcelain_output": " M src/file.cpp"}
        _, semantic = semantic_case(root, "SourceTreeClean", "source_tree_clean", dirty)
        results["dirty_source_claimed_clean"] = not semantic

        changed = {
            "status": "PASS", "passed": True, "commit_start": COMMIT,
            "commit_end": COMMIT, "branch_start": "main", "branch_end": "main",
            "git_status_start": "", "git_status_end": "",
            "content_hash_start": "A" * 64,
            "content_hash_after_configure": "A" * 64,
            "content_hash_after_build": "E" * 64,
            "content_hash_before_package": "A" * 64,
            "content_hash_after_package": "A" * 64,
            "content_hash_end": "A" * 64,
            "tracked_files_start": 1, "tracked_files_after_configure": 1,
            "tracked_files_after_build": 1,
            "tracked_files_before_package": 1, "tracked_files_after_package": 1,
            "tracked_files_end": 1,
        }
        for phase in ("after_configure", "after_build", "before_package", "after_package", "end"):
            changed[f"build_inputs_hash_{phase}"] = "9" * 64
            changed[f"build_inputs_ready_{phase}"] = True
        _, semantic = semantic_case(root, "SourceSnapshotImmutability", "source_snapshot_immutability", changed)
        results["source_content_changed_after_build_start"] = not semantic

        changed_build_input = json.loads(json.dumps(changed))
        for phase in ("start", "after_configure", "after_build", "before_package", "after_package", "end"):
            changed_build_input[f"content_hash_{phase}"] = "A" * 64
        changed_build_input["build_inputs_hash_after_build"] = "8" * 64
        _, semantic = semantic_case(
            root, "SourceSnapshotImmutability", "source_snapshot_immutability",
            changed_build_input,
        )
        results["modified_build_input_after_configure"] = not semantic

        wrong_build_inputs_aggregate = json.loads(json.dumps(changed_build_input))
        for phase in ("after_configure", "after_build", "before_package", "after_package", "end"):
            wrong_build_inputs_aggregate[f"build_inputs_hash_{phase}"] = "8" * 64
        _, semantic = semantic_case(
            root, "SourceSnapshotImmutability", "source_snapshot_immutability",
            wrong_build_inputs_aggregate,
        )
        results["build_inputs_aggregate_mismatch"] = not semantic

        snapshot_mismatch = {
            "status": "PASS", "passed": True, "commit_start": COMMIT,
            "commit_end": COMMIT, "branch_start": "main", "branch_end": "main",
            "git_status_start": "", "git_status_end": "",
            "content_hash_start": "C" * 64, "content_hash_after_configure": "C" * 64,
            "content_hash_after_build": "C" * 64,
            "content_hash_before_package": "C" * 64, "content_hash_after_package": "C" * 64,
            "content_hash_end": "C" * 64,
            "tracked_files_start": 1, "tracked_files_after_configure": 1,
            "tracked_files_after_build": 1,
            "tracked_files_before_package": 1, "tracked_files_after_package": 1,
            "tracked_files_end": 1,
        }
        for phase in ("after_configure", "after_build", "before_package", "after_package", "end"):
            snapshot_mismatch[f"build_inputs_hash_{phase}"] = "9" * 64
            snapshot_mismatch[f"build_inputs_ready_{phase}"] = True
        _, semantic = semantic_case(root, "SourceSnapshotImmutability", "source_snapshot_immutability", snapshot_mismatch)
        results["source_snapshot_aggregate_mismatch"] = not semantic

        _, semantic = semantic_case(root, "SourceTreeClean", "source_tree_clean", {"status": "PASS", "passed": True})
        results["source_clean_missing_porcelain"] = not semantic

        immutable = {"status": "PASS", "passed": True, "before_sha256": CANDIDATE, "after_sha256": "F" * 64}
        _, semantic = semantic_case(
            root, "ArtifactImmutability", "artifact_immutability", immutable,
            evidence_candidate=CANDIDATE, candidate_path=candidate,
        )
        results["candidate_modified_after_validation"] = not semantic

        candidate_executable_sha = candidate_executable_digest(candidate)
        short_soak = soak_payload(
            candidate_executable_sha,
            overrides={"wall_clock_seconds": 60.0, "simulation_steps": 1},
        )
        _, semantic = semantic_case(
            root, "Soak2Hours", "soak_2h", short_soak,
            evidence_candidate=CANDIDATE, candidate_path=candidate,
        )
        results["short_or_skipped_soak"] = not semantic

        nonfinite_soak = soak_payload(
            candidate_executable_sha,
            overrides={"wall_clock_seconds": float("nan")},
        )
        _, semantic = semantic_case(
            root, "Soak2Hours", "soak_2h", nonfinite_soak,
            evidence_candidate=CANDIDATE, candidate_path=candidate,
        )
        results["soak_nonfinite_summary"] = not semantic

        memory_soak = soak_payload(
            candidate_executable_sha,
            overrides={"memory_leak_suspected": True},
        )
        _, semantic = semantic_case(
            root, "Soak2Hours", "soak_2h", memory_soak,
            evidence_candidate=CANDIDATE, candidate_path=candidate,
        )
        results["soak_memory_leak_claim"] = not semantic

        residual_soak = soak_payload(
            candidate_executable_sha,
            overrides={"species_mass_residual_abs_max_kg": 1.0},
        )
        _, semantic = semantic_case(
            root, "Soak2Hours", "soak_2h", residual_soak,
            evidence_candidate=CANDIDATE, candidate_path=candidate,
        )
        results["soak_mass_residual_out_of_bounds"] = not semantic

        soak_identity = dict(short_soak)
        soak_identity["wall_clock_seconds"] = 7200
        soak_identity["simulation_steps"] = 1000
        soak_identity["public_zip_sha256"] = "E" * 64
        soak_identity["source_commit"] = COMMIT
        _, semantic = semantic_case(
            root, "Soak2Hours", "soak_2h", soak_identity,
            evidence_candidate=CANDIDATE, candidate_path=candidate,
        )
        results["soak_wrong_public_zip"] = not semantic
        soak_identity["public_zip_sha256"] = CANDIDATE
        soak_identity["source_commit"] = "f" * 40
        _, semantic = semantic_case(
            root, "Soak2Hours", "soak_2h", soak_identity,
            evidence_candidate=CANDIDATE, candidate_path=candidate,
        )
        results["soak_wrong_source_commit"] = not semantic

        soak_source = dict(short_soak)
        soak_source["wall_clock_seconds"] = 7200
        soak_source["simulation_steps"] = 1000
        soak_source["executable_source"] = "workspace_executable"
        _, semantic = semantic_case(
            root, "Soak2Hours", "soak_2h", soak_source,
            evidence_candidate=CANDIDATE, candidate_path=candidate,
        )
        results["soak_not_from_candidate_zip"] = not semantic

        soak_executable = dict(short_soak)
        soak_executable["wall_clock_seconds"] = 7200
        soak_executable["simulation_steps"] = 1000
        soak_executable["exe_sha256"] = "0" * 64
        _, semantic = semantic_case(
            root, "Soak2Hours", "soak_2h", soak_executable,
            evidence_candidate=CANDIDATE, candidate_path=candidate,
        )
        results["soak_wrong_executable_sha"] = not semantic

        gui = {"status": "PASS", "passed": True, "artifact": "missing.bmp", "screenshot_width": 1, "screenshot_height": 1, "screenshot_bytes": 54}
        for field in ("window_created", "frame_rendered", "resize", "fullscreen_toggle", "keyboard", "mouse", "text_input", "clipboard", "screenshot_created", "clean_shutdown", "restart"):
            gui[field] = True
        _, semantic = semantic_case(
            root, "SDL3GUI", "sdl3_gui", gui,
            evidence_candidate=CANDIDATE, candidate_path=candidate,
        )
        results["gui_missing_screenshot"] = not semantic

        blank_bmp = root / "blank.bmp"
        write_solid_bmp(blank_bmp, 4, 4, 0)
        blank_gui = dict(gui)
        blank_gui.update({
            "artifact": blank_bmp.name,
            "screenshot_width": 4,
            "screenshot_height": 4,
            "screenshot_bytes": blank_bmp.stat().st_size,
            "screenshot_pixel_count": 16,
            "screenshot_nonzero_pixels": 0,
            "screenshot_distinct_colors": 1,
        })
        _, semantic = semantic_case(
            root, "SDL3GUI", "sdl3_gui", blank_gui,
            evidence_candidate=CANDIDATE, candidate_path=candidate,
        )
        results["gui_blank_screenshot"] = not semantic

        gpu = {
            "status": "PASS", "passed": True, "supported": True, "fallback": False,
            "backend": "CPU", "reason": "PASS", "max_abs_error": 0.0,
            "max_rel_error": 0.0, "first_mismatch_index": -1,
        }
        _, semantic = semantic_case(root, "GPUNumericalValidation", "gpu_validation", gpu)
        results["gpu_wrong_backend"] = not semantic
        gpu.update({
            "backend": "SDL_GPU Vulkan", "max_abs_error": 999.0,
            "max_rel_error": 999.0, "first_mismatch_index": 123,
        })
        _, semantic = semantic_case(root, "GPUNumericalValidation", "gpu_validation", gpu)
        results["gpu_inconsistent_error_metrics"] = not semantic

        _, semantic = trusted_adapter_case(root, producer_hash="E" * 64)
        results["wrong_producer_evidence_hash"] = not semantic

        _, semantic = trusted_adapter_case(
            root,
            producer_overrides={
                "supported": False,
                "fallback": True,
                "backend": "CPU",
                "reason": "fallback_cpu",
                "max_abs_error": 999.0,
                "max_rel_error": 999.0,
                "first_mismatch_index": 12,
            },
            adapter_overrides={
                "supported": True,
                "fallback": False,
                "backend": "SDL_GPU Vulkan",
                "reason": "PASS",
                "max_abs_error": 0.0,
                "max_rel_error": 0.0,
                "first_mismatch_index": -1,
            },
        )
        results["producer_adapter_semantic_mismatch"] = not semantic

        published_documents = (
            root / "published-RELEASE-VALIDATION.json",
            root / "published-RELEASE-VALIDATION.txt",
            root / "published-BUILD-INFO.txt",
        )
        for index, document in enumerate(published_documents):
            document.write_text(f"published-baseline-{index}\n", encoding="utf-8")
            finalizer.write_sidecar(document)
        published_bundle = root / "published-evidence.zip"
        with zipfile.ZipFile(published_bundle, "w", zipfile.ZIP_DEFLATED) as archive:
            for document in published_documents:
                archive.write(document, document.name)
        published_marker = {
            "validation_filename": published_documents[0].name,
            "validation_sha256": digest(published_documents[0]),
            "validation_text_filename": published_documents[1].name,
            "validation_text_sha256": digest(published_documents[1]),
            "build_info_filename": published_documents[2].name,
            "build_info_sha256": digest(published_documents[2]),
        }
        try:
            finalizer.validate_published_document_set(
                bundle=published_bundle,
                documents=published_documents,
                marker=published_marker,
            )
        except ValueError as exc:
            baselines["published_document_bundle_binding"] = False
            details.append(f"published document positive baseline: {exc}")
        else:
            baselines["published_document_bundle_binding"] = True
        published_documents[0].write_text("forged-published-validation\n", encoding="utf-8")
        finalizer.write_sidecar(published_documents[0])
        published_marker["validation_sha256"] = digest(published_documents[0])
        try:
            finalizer.validate_published_document_set(
                bundle=published_bundle,
                documents=published_documents,
                marker=published_marker,
            )
        except ValueError:
            results["published_document_disagrees_with_frozen_bundle"] = True
        else:
            results["published_document_disagrees_with_frozen_bundle"] = False

        semantic_bundle = root / "completion-audited-evidence.zip"
        with zipfile.ZipFile(semantic_bundle, "w", zipfile.ZIP_DEFLATED) as archive:
            archive.writestr("current-run.json", b'{"status":"PASS","passed":true}\n')
        audited_bundle_sha = digest(semantic_bundle)
        baselines["completion_bundle_identity_binding"] = True
        replacement = semantic_bundle.with_suffix(".replacement.zip")
        with zipfile.ZipFile(replacement, "w", zipfile.ZIP_DEFLATED) as archive:
            archive.writestr("current-run.json", b'{"status":"FAIL","passed":false}\n')
        os.replace(replacement, semantic_bundle)
        try:
            finalizer.validate_bundle_audit_identity(
                {"passed": True, "bundle_sha256": audited_bundle_sha},
                semantic_bundle,
                phase="negative-suite-completion",
            )
        except ValueError:
            results["evidence_bundle_replaced_after_semantic_audit"] = True
        else:
            results["evidence_bundle_replaced_after_semantic_audit"] = False

        provenance_results, provenance_baseline, provenance_details = provenance_attacks(root)
        results.update(provenance_results)
        baselines["official_provenance"] = provenance_baseline
        details.extend(provenance_details)
        promotion_results, promotion_baseline, promotion_details = promotion_attacks(
            root, candidate, symbols
        )
        results.update(promotion_results)
        baselines["candidate_promotion"] = promotion_baseline
        details.extend(promotion_details)

        ready_gates = {
            name: {"Status": "PASS"}
            for name in audit.STABLE_MANDATORY_GATES
        }
        ready = {
            "schema": "omnipack-release-validation", "schema_version": 1,
            "version": "1.1.0", "channel": "stable",
            "status": "READY FOR 1.1.0 STABLE", "final_status": "READY FOR 1.1.0 STABLE",
            "run_id": RUN_ID, "commit": COMMIT, "source_snapshot_sha256": "A" * 64,
            "build_inputs_sha256": "9" * 64,
            "candidate_sha256": CANDIDATE, "symbols_sha256": SYMBOLS,
            "symbols_member_sha256": SYMBOL_MEMBER, "blocking_items": [], "gates": ready_gates,
        }
        docs_package = root / "documentation-baseline.zip"
        with zipfile.ZipFile(docs_package, "w", zipfile.ZIP_DEFLATED) as archive:
            archive.writestr(
                "TPT-ZH-OmniPack-1.1.0/README.txt",
                "1.1.0\nSDL_GPU Vulkan\nCPU fallback\nCUDA not implemented future optional\n",
            )
        docs_ready = json.loads(json.dumps(ready))
        docs_ready["candidate_sha256"] = digest(docs_package)
        validation_text = root / "documentation-baseline.txt"
        validation_text.write_text(
            "\n".join([
                "TPT-ZH OmniPack 1.1.0 Release Validation",
                f"Commit: {COMMIT}", "Channel: stable", f"Run ID: {RUN_ID}",
                "Source snapshot SHA256: " + "A" * 64,
                "Build inputs SHA256: " + "9" * 64,
                f"Candidate SHA256: {docs_ready['candidate_sha256']}",
                f"Symbols SHA256: {SYMBOLS}",
                f"Symbols member SHA256: {SYMBOL_MEMBER}",
                *(f"{name}: PASS" for name in ready_gates),
                "FINAL STATUS: READY FOR 1.1.0 STABLE", "",
            ]),
            encoding="utf-8",
        )
        build_info = root / "documentation-build-info.txt"
        build_info.write_text(
            "\n".join([
                "TPT-ZH OmniPack 1.1.0", f"Git commit: {COMMIT}",
                "Channel: stable", "Source snapshot SHA256: " + "A" * 64,
                "Build inputs SHA256: " + "9" * 64,
                "SDL_GPU Vulkan", "CPU fallback", "CUDA future optional", "",
            ]),
            encoding="utf-8",
        )
        docs_baseline_ok, docs_baseline_errors = audit.audit_docs(
            docs_ready, validation_text, build_info, docs_package, "stable"
        )
        baselines["documentation"] = docs_baseline_ok
        details.extend(
            f"documentation positive baseline: {error}"
            for error in docs_baseline_errors
        )
        docs_ok, _ = audit.audit_docs(ready, None, None, None, "stable")
        results["ready_missing_release_documents"] = not docs_ok
        ready_with_failed_supplemental = json.loads(json.dumps(ready))
        ready_with_failed_supplemental["gates"]["PackageVerification"]["Status"] = "FAIL"
        ready_with_failed_supplemental["blocking_items"] = ["PackageVerification"]
        ready_with_failed_supplemental["candidate_sha256"] = digest(docs_package)
        docs_ok, _ = audit.audit_docs(
            ready_with_failed_supplemental,
            validation_text,
            build_info,
            docs_package,
            "stable",
        )
        results["supplemental_gate_failure"] = not docs_ok

        if (candidate and candidate.is_file() and symbols and symbols.is_file()
                and artifact_stem and symbol_artifact_stem):
            candidate_baseline_errors = package_audit.audit_package(
                candidate, version="1.1.0", kind="release",
                artifact_stem=artifact_stem,
                symbol_artifact_stem=symbol_artifact_stem,
            )
            symbol_baseline_errors = package_audit.audit_package(
                symbols, True, version="1.1.0",
                artifact_stem=artifact_stem,
                symbol_artifact_stem=symbol_artifact_stem,
            )
            if candidate_baseline_errors or symbol_baseline_errors:
                baselines["candidate_package"] = not candidate_baseline_errors
                baselines["symbols_package"] = not symbol_baseline_errors
                results["package_manifest_corruption"] = False
                results["symbols_missing"] = False
                details.extend(
                    f"candidate baseline audit: {error}"
                    for error in candidate_baseline_errors
                )
                details.extend(
                    f"symbols baseline audit: {error}"
                    for error in symbol_baseline_errors
                )
            else:
                baselines["candidate_package"] = True
                baselines["symbols_package"] = True
                corrupt = root / "corrupt.zip"
                corrupt_manifest(candidate, corrupt)
                errors = package_audit.audit_package(
                    corrupt, version="1.1.0", kind="release",
                    artifact_stem=artifact_stem,
                    symbol_artifact_stem=symbol_artifact_stem,
                )
                results["package_manifest_corruption"] = bool(errors)

                missing_symbols = root / "missing-symbols.zip"
                errors = package_audit.audit_package(
                    missing_symbols, True, version="1.1.0",
                    artifact_stem=artifact_stem,
                    symbol_artifact_stem=symbol_artifact_stem,
                )
                results["symbols_missing"] = bool(errors)
        else:
            baselines["candidate_package"] = False
            baselines["symbols_package"] = False
            results["package_manifest_corruption"] = False
            results["symbols_missing"] = False
            details.append("candidate, symbols, or artifact stems were not supplied")
    return results, baselines, details


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--candidate", type=Path, required=True)
    parser.add_argument("--symbols", type=Path, required=True)
    parser.add_argument("--artifact-stem", required=True)
    parser.add_argument("--symbol-artifact-stem", required=True)
    parser.add_argument("--run-id", required=True)
    parser.add_argument("--commit", required=True)
    parser.add_argument("--candidate-sha256", required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    global RUN_ID, COMMIT, CANDIDATE
    RUN_ID, COMMIT, CANDIDATE = args.run_id, args.commit, args.candidate_sha256.upper()
    try:
        if re.fullmatch(r"[0-9]{8}T[0-9]{6}Z-[0-9a-f]{8}", RUN_ID) is None:
            raise ValueError("run_id is invalid")
        if re.fullmatch(r"[0-9a-f]{40}", COMMIT) is None:
            raise ValueError("commit is invalid")
        if audit.SHA256_RE.fullmatch(CANDIDATE) is None:
            raise ValueError("candidate SHA256 is invalid")
        if not args.candidate.is_file() or not args.symbols.is_file():
            raise ValueError("candidate or symbols archive is absent")
        if any(part in args.artifact_stem for part in ("/", "\\", "..")):
            raise ValueError("candidate artifact stem is unsafe")
        if any(part in args.symbol_artifact_stem for part in ("/", "\\", "..")):
            raise ValueError("symbols artifact stem is unsafe")
        expected_candidate_name = f"{args.artifact_stem}.zip"
        expected_symbols_name = f"{args.symbol_artifact_stem}.zip"
        if args.candidate.name != expected_candidate_name or args.symbols.name != expected_symbols_name:
            raise ValueError("candidate or symbols filename is not bound to run_id")
        observed_candidate_sha256 = digest(args.candidate)
        if observed_candidate_sha256 != CANDIDATE:
            raise ValueError("candidate SHA256 argument does not match candidate bytes")

        results, baselines, details = run(
            args.candidate, args.symbols, args.artifact_stem, args.symbol_artifact_stem
        )
        missing = sorted(audit.REQUIRED_NEGATIVE_ATTACKS.difference(results))
        failed = sorted(name for name, rejected in results.items() if not rejected)
        failed.extend(f"missing:{name}" for name in missing)
        failed_baselines = sorted(name for name, accepted in baselines.items() if not accepted)
        passed = not failed and not failed_baselines
        value = {
            "schema": audit.EVIDENCE_SCHEMA,
            "schema_version": audit.EVIDENCE_SCHEMA_VERSION,
            "test": "negative_gate_suite",
            "run_id": RUN_ID,
            "commit": COMMIT,
            "candidate_sha256": CANDIDATE,
            "candidate_sha256_observed": observed_candidate_sha256,
            "status": "PASS" if passed else "FAIL",
            "passed": passed,
            "baselines_total": len(baselines),
            "baselines_passed": sum(baselines.values()),
            "baselines_failed": failed_baselines,
            "baselines": baselines,
            "attacks_total": len(results),
            "attacks_rejected": sum(results.values()),
            "attacks_failed": failed,
            "attacks": results,
            "details": details,
            "finished_at": datetime.now(timezone.utc).isoformat(),
        }
        args.output.parent.mkdir(parents=True, exist_ok=True)
        write_json(args.output, value)
        return 0 if passed else 1
    except Exception as exc:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        write_json(args.output, {
            "schema": audit.EVIDENCE_SCHEMA, "schema_version": audit.EVIDENCE_SCHEMA_VERSION,
            "test": "negative_gate_suite", "run_id": RUN_ID, "commit": COMMIT,
            "candidate_sha256": CANDIDATE, "status": "FAIL", "passed": False,
            "reason": str(exc),
        })
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
