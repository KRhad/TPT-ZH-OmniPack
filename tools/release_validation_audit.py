#!/usr/bin/env python3
"""Fail-closed hash, semantic, run, source, and candidate evidence audit."""

from __future__ import annotations

import argparse
from datetime import datetime, timedelta, timezone
import hashlib
import json
import math
from pathlib import Path, PurePosixPath
import re
import zipfile


EVIDENCE_SCHEMA = "omnipack-release-evidence"
EVIDENCE_SCHEMA_VERSION = 1
# Evidence timestamps are producer-owned JSON fields. Do not use filesystem
# mtimes for provenance: artifact upload/download, archive extraction, and
# byte-for-byte copying can legitimately rewrite or quantise them. Freshness
# is bound by the unique run ID, declared execution interval, and hashed
# source/candidate identities instead.
EVIDENCE_TIMESTAMP_TOLERANCE = timedelta(seconds=2)
EVIDENCE_CLOCK_SKEW_TOLERANCE = timedelta(minutes=5)
RELEASE_RUN_MAX_AGE = timedelta(hours=24)
SOAK_MIN_WALL_CLOCK_SECONDS = 7200.0
SOAK_WALL_CLOCK_TOLERANCE_SECONDS = 2.0
RUN_ID_RE = re.compile(
    r"^(?P<timestamp>[0-9]{8}T[0-9]{6}Z)-(?P<nonce>[0-9a-f]{8})$"
)
PORTABLE_RUNTIME_PAYLOAD_SCHEMA_VERSION = 2
PORTABLE_RUNTIME_TRUE_FIELDS = (
    "launch_passed", "fixture_created", "initial_simulate_passed",
    "initial_save_passed", "initial_parse_passed", "initial_reload_passed",
    "initial_state_validate_passed", "post_step_simulate_passed",
    "post_step_finite_passed", "post_step_inventory_passed",
    "post_step_atmosphere_passed", "post_step_save_passed",
    "post_step_parse_passed", "post_step_reload_passed",
    "post_step_roundtrip_passed", "save_reload_passed",
    "state_validate_passed", "enhanced_mode", "omni_state_present",
    "water_sidecar_roundtrip", "runtime_completion_reached",
)
CPU_FALLBACK_TRUE_FIELDS = (
    "initialization_failure_forced", "initialization_backend_prearmed",
    "initialization_backend_reset_to_cpu", "initialization_cpu_path_executed",
    "initialization_control_state_match", "runtime_failure_injected",
    "runtime_executor_input_valid", "runtime_failure_observed",
    "production_atmosphere_step_executed", "same_step_cpu_fallback",
    "backend_reset_to_cpu", "cpu_control_state_match", "state_changed_same_step",
    "mass_residual_finite", "energy_residual_finite",
    "mass_residual_within_tolerance", "energy_residual_within_tolerance",
)
OFFICIAL_REPOSITORY = "https://github.com/The-Powder-Toy/The-Powder-Toy"
OFFICIAL_TPT_BENCH_REPOSITORY = "https://github.com/The-Powder-Toy/tpt-bench"
OFFICIAL_WEB_API_ORIGIN = "https://powdertoy.co.uk"
OFFICIAL_WEB_STATIC_ORIGIN = "https://static.powdertoy.co.uk"
OFFICIAL_MAINTAINERS = {"jacob1", "Simon"}
OFFICIAL_WEB_SAVE_ALLOWLIST = {
    1249335: {"username": "jacob1", "date": 1738891791, "date_created": 1372986719},
    284: {"username": "Simon", "date": 1276381701, "date_created": 1276381701},
    1101197: {"username": "jacob1", "date": 1427430299, "date_created": 1361045969},
}
OFFICIAL_PROVENANCE_V2 = "omnipack-official-tpt-save-corpus-v2"
SHA256_RE = re.compile(r"^[0-9A-F]{64}$")
REVISION_RE = re.compile(r"^[0-9a-fA-F]{40}$")
OFFICIAL_COVERAGE_CONTRACT = "official-tpt-save-coverage-v1"
OFFICIAL_COVERAGE_REQUIRED = (
    "basic_particles", "powders", "solids", "liquids", "gases",
    "temperature", "pressure", "velocity", "walls", "fans",
    "electronics", "life", "signs", "decoration", "legacy_states",
    "larger_save",
)
OFFICIAL_COVERAGE_METRIC_MAP = {
    "basic_particles": "particles", "powders": "powders",
    "solids": "solids", "liquids": "liquids", "gases": "gases",
    "temperature": "temperature_signals", "pressure": "pressure_cells",
    "velocity": "velocity_signals", "walls": "wall_cells",
    "fans": "fan_cells", "electronics": "electronics_particles",
    "life": "life_particles", "signs": "signs",
    "decoration": "decorated_particles", "legacy_states": "legacy_state",
    "larger_save": "larger_save",
}
OFFICIAL_COVERAGE_METRICS = tuple(
    {"input_bytes", *OFFICIAL_COVERAGE_METRIC_MAP.values()}
)


def official_coverage_categories(metrics: object) -> set[str] | None:
    if not isinstance(metrics, dict) or set(metrics) != set(OFFICIAL_COVERAGE_METRICS):
        return None
    if any(not isinstance(value, int) or isinstance(value, bool) or value < 0 for value in metrics.values()):
        return None
    if metrics.get("legacy_state") not in {0, 1} or metrics.get("larger_save") not in {0, 1}:
        return None
    if metrics.get("larger_save") != int(metrics.get("input_bytes", 0) >= 10000):
        return None
    categories = {
        category for category, metric in OFFICIAL_COVERAGE_METRIC_MAP.items()
        if metrics[metric] > 0 and category != "larger_save"
    }
    if metrics["input_bytes"] >= 10000:
        categories.add("larger_save")
    return categories


def official_provenance_identity_errors(raw: dict[str, object]) -> list[str]:
    """Validate v1 Git-only or v2 explicitly separated official identities."""
    errors: list[str] = []
    if raw.get("provenance_schema") == OFFICIAL_PROVENANCE_V2:
        repositories = raw.get("source_repositories")
        records = raw.get("repositories")
        if not isinstance(repositories, list) or not repositories:
            errors.append("v2 official provenance source repository list is missing")
        elif any(item not in {OFFICIAL_REPOSITORY, OFFICIAL_TPT_BENCH_REPOSITORY, OFFICIAL_WEB_API_ORIGIN} for item in repositories):
            errors.append("v2 official provenance source repository list contains an untrusted origin")
        if not isinstance(records, list) or not records:
            errors.append("v2 official provenance repository records are missing")
        git_records = [
            record for record in records or []
            if isinstance(record, dict) and record.get("source_kind") == "github_git"
        ]
        if not git_records:
            errors.append("v2 official provenance has no Git-object source")
        for record in git_records:
            if (
                record.get("repository") not in {OFFICIAL_REPOSITORY, OFFICIAL_TPT_BENCH_REPOSITORY}
                or not isinstance(record.get("revision"), str)
                or REVISION_RE.fullmatch(str(record.get("revision"))) is None
                or record.get("revision_exists") is not True
                or record.get("revision_reachable_from_official_remote") is not True
            ):
                errors.append("v2 official provenance contains an invalid Git repository record")
        web_records = [
            record for record in records or []
            if isinstance(record, dict) and record.get("source_kind") == "official_web_save"
        ]
        for record in web_records:
            save_id = record.get("save_id")
            allowlisted = OFFICIAL_WEB_SAVE_ALLOWLIST.get(save_id) if isinstance(save_id, int) else None
            if (
                record.get("repository") != OFFICIAL_WEB_API_ORIGIN
                or allowlisted is None
                or record.get("source_date") != allowlisted["date"]
                or record.get("metadata_verified") is not True
                or record.get("content_hash_verified") is not True
            ):
                errors.append("v2 official provenance contains an invalid web-source record")
        if raw.get("revision_exists") is not True or raw.get("revision_reachable_from_official_remote") is not True:
            errors.append("v2 official provenance aggregate Git reachability is not PASS")
        return errors
    if raw.get("repository") != OFFICIAL_REPOSITORY:
        errors.append("official provenance repository identity is invalid")
    if not isinstance(raw.get("revision"), str) or not REVISION_RE.fullmatch(str(raw.get("revision", ""))):
        errors.append("official provenance revision identity is invalid")
    if raw.get("revision_exists") is not True or raw.get("revision_reachable_from_official_remote") is not True:
        errors.append("official provenance revision is absent or not reachable from the official remote")
    return errors


def official_provenance_v2_row_errors(row: object) -> list[str]:
    """Re-check v2 locator and maintainer metadata semantics from raw evidence."""
    if not isinstance(row, dict):
        return ["v2 official provenance row is not an object"]
    errors: list[str] = []
    kind = row.get("source_kind")
    path = row.get("path")
    if not isinstance(path, str) or "\\" in path or not path.lower().endswith((".cps", ".stm")):
        errors.append("v2 official provenance fixture path is invalid")
    elif PurePosixPath(path).is_absolute() or ".." in PurePosixPath(path).parts:
        errors.append("v2 official provenance fixture path escapes corpus")
    if kind == "github_git":
        repository = row.get("source_repository")
        revision = row.get("source_revision")
        source_path = row.get("source_path")
        locator = row.get("source_locator")
        if repository not in {OFFICIAL_REPOSITORY, OFFICIAL_TPT_BENCH_REPOSITORY}:
            errors.append("v2 Git provenance repository is not allowlisted")
        if not isinstance(revision, str) or REVISION_RE.fullmatch(revision) is None:
            errors.append("v2 Git provenance revision is invalid")
        if not isinstance(source_path, str) or "\\" in source_path or ".." in PurePosixPath(source_path).parts:
            errors.append("v2 Git provenance source path is invalid")
        expected = {
            f"{repository}/blob/{revision}/{source_path}",
            f"https://raw.githubusercontent.com/{str(repository).removeprefix('https://github.com/')}/{revision}/{source_path}",
        }
        if locator not in expected:
            errors.append("v2 Git provenance locator does not bind the source path")
    elif kind == "official_web_save":
        save_id = row.get("save_id")
        source_date = row.get("source_date")
        metadata = row.get("metadata")
        if row.get("source_repository") != OFFICIAL_WEB_API_ORIGIN:
            errors.append("v2 web provenance API origin is invalid")
        if not isinstance(save_id, int) or isinstance(save_id, bool) or save_id <= 0:
            errors.append("v2 web provenance save ID is invalid")
        if not isinstance(source_date, int) or isinstance(source_date, bool):
            errors.append("v2 web provenance date is invalid")
        allowlisted = OFFICIAL_WEB_SAVE_ALLOWLIST.get(save_id)
        if allowlisted is None:
            errors.append("v2 web provenance save ID is not allowlisted")
        elif source_date != allowlisted["date"]:
            errors.append("v2 web provenance date is not the pinned maintainer-save date")
        if row.get("source_locator") != f"{OFFICIAL_WEB_API_ORIGIN}/Browse/View.json?ID={save_id}":
            errors.append("v2 web provenance API locator is not exact")
        suffix = PurePosixPath(path).suffix.lower() if isinstance(path, str) else ".cps"
        if row.get("content_locator") != f"{OFFICIAL_WEB_STATIC_ORIGIN}/{save_id}{suffix}":
            errors.append("v2 web provenance content locator is not exact")
        if not isinstance(metadata, dict):
            errors.append("v2 web provenance metadata is missing")
        else:
            if metadata.get("id") != save_id or metadata.get("date") != source_date:
                errors.append("v2 web provenance metadata ID/date mismatch")
            if metadata.get("username") not in OFFICIAL_MAINTAINERS:
                errors.append("v2 web provenance author is not an allowlisted maintainer")
            if allowlisted is not None and (
                metadata.get("username") != allowlisted["username"]
                or metadata.get("date_created") != allowlisted["date_created"]
            ):
                errors.append("v2 web provenance metadata does not match the pinned maintainer save")
            if metadata.get("elevation") != "Mod" or metadata.get("published") is not True or metadata.get("is_banned") is not False:
                errors.append("v2 web provenance metadata does not prove a published maintainer save")
    else:
        errors.append("v2 official provenance source kind is invalid")
    return errors
EXPECTED_TEST_NAMES = {
    "SourceTreeClean": "source_tree_clean",
    "SourceSnapshotImmutability": "source_snapshot_immutability",
    "Configure": "configure",
    "Build": "build",
    "UnitTests": "unit_tests",
    "AtmosphereBench": "atmosphere_bench",
    "MassConservation": "mass_conservation",
    "OmniSaveRoundtrip": "omni_save_roundtrip",
    "OfficialTPTCorpusProvenance": "official_tpt_provenance",
    "OfficialTPTSaveCompatibility": "official_tpt_save_compatibility",
    "SDL3Runtime": "sdl_gpu_runtime",
    "GPUNumericalValidation": "gpu_validation",
    "CPUFallbackValidation": "cpu_fallback",
    "SDL3GUI": "sdl3_gui",
    "DebugSymbolsSeparated": "debug_symbols_separated",
    "ReleaseBinaryStripped": "release_binary_stripped",
    "PackageManifest": "package_manifest",
    "PackageVerification": "package_verification",
    "SymbolPackageVerification": "symbol_package_verification",
    "WindowsPortableExtraction": "windows_portable_extraction",
    "WindowsCleanMachine": "windows_clean_machine",
    "Soak2Hours": "soak_2h",
    "CandidateSHA256": "candidate_sha256",
    "ArtifactImmutability": "artifact_immutability",
    "EvidenceSemanticIntegrity": "release_validation_audit",
    "EvidenceHashIntegrity": "release_validation_audit",
    "DocumentationConsistency": "release_validation_audit",
    "NegativeGateSuite": "negative_gate_suite",
    "CandidatePromotion": "candidate_promotion",
}
CANDIDATE_BOUND_GATES = {
    "PackageManifest",
    "PackageVerification",
    "WindowsPortableExtraction",
    "WindowsCleanMachine",
    "SDL3GUI",
    "Soak2Hours",
    "CandidateSHA256",
    "ArtifactImmutability",
    "NegativeGateSuite",
    "CandidatePromotion",
}
SYMBOL_BOUND_GATES = {"SymbolPackageVerification", "CandidateSHA256", "ArtifactImmutability", "NegativeGateSuite", "CandidatePromotion"}
META_GATES = {"EvidenceSemanticIntegrity", "EvidenceHashIntegrity", "DocumentationConsistency"}
# These gates are produced by Invoke-GateProcess without a gate-specific
# semantic validator.  A declaration-only JSON document must not be able to
# claim PASS for one of them: the process result and both captured streams are
# part of the evidence contract.
GENERIC_COMMAND_GATES = {
    "Configure", "Build", "UnitTests", "AtmosphereBench", "MassConservation",
    "OmniSaveRoundtrip", "DebugSymbolsSeparated", "ReleaseBinaryStripped",
    "PackageManifest", "PackageVerification", "SymbolPackageVerification",
}
# Keep the name explicit for callers/tests that want to distinguish these from
# the specialized runtime and package gates.
COMMAND_GATES = GENERIC_COMMAND_GATES
SUPPLEMENTAL_GATES = {"PackageVerification", "SymbolPackageVerification"}
STABLE_MANDATORY_GATES = {
    "SourceTreeClean", "SourceSnapshotImmutability", "Configure", "Build",
    "UnitTests", "AtmosphereBench", "MassConservation", "OmniSaveRoundtrip",
    "OfficialTPTCorpusProvenance", "OfficialTPTSaveCompatibility", "SDL3Runtime",
    "GPUNumericalValidation", "CPUFallbackValidation", "SDL3GUI",
    "WindowsPortableExtraction", "Soak2Hours", "ReleaseBinaryStripped",
    "DebugSymbolsSeparated", "PackageManifest", "CandidateSHA256",
    "ArtifactImmutability", "WindowsCleanMachine", "EvidenceSemanticIntegrity",
    "EvidenceHashIntegrity", "DocumentationConsistency", "NegativeGateSuite",
    "CandidatePromotion", "PackageVerification", "SymbolPackageVerification",
}
REQUIRED_NEGATIVE_ATTACKS = {
    "aggregate_pass_with_not_tested_evidence",
    "aggregate_pass_with_failed_evidence",
    "boolean_schema_version",
    "official_boolean_count_confusion",
    "stale_previous_run_evidence",
    "stale_raw_previous_run_evidence",
    "future_gate_timestamp",
    "raw_timestamp_outside_envelope",
    "wrong_commit_evidence",
    "wrong_test_type_evidence",
    "raw_evidence_identity_missing",
    "meta_gate_missing_evidence",
    "generic_pass_with_nonzero_exit",
    "generic_declaration_only_pass",
    "portable_declaration_only_pass",
    "portable_post_step_false_pass",
    "portable_outer_inner_semantic_mismatch",
    "portable_inner_hash_mismatch",
    "portable_multiple_executables",
    "portable_candidate_source_marker",
    "clean_machine_environment_marker_on_checkout",
    "clean_machine_missing_validator_binding",
    "clean_machine_wrong_validator_sha",
    "clean_machine_binding_clean_hash_mismatch",
    "clean_machine_replaced_validator",
    "wrong_candidate_sha_evidence",
    "dirty_source_claimed_clean",
    "source_content_changed_after_build_start",
    "modified_build_input_after_configure",
    "source_snapshot_aggregate_mismatch",
    "source_clean_missing_porcelain",
    "candidate_modified_after_validation",
    "short_or_skipped_soak",
    "soak_nonfinite_summary",
    "soak_memory_leak_claim",
    "soak_mass_residual_out_of_bounds",
    "soak_wrong_public_zip",
    "soak_wrong_source_commit",
    "soak_missing_release_binding",
    "soak_harness_run_mismatch",
    "soak_analyzer_failure_relabelled_pass",
    "soak_result_json_missing",
    "soak_result_json_hash_mismatch",
    "soak_result_json_run_mismatch",
    "soak_result_json_stale",
    "soak_timestamp_duration_mismatch",
    "soak_wall_clock_type_confusion",
    "soak_result_path_relocated",
    "soak_release_run_timestamp_mismatch",
    "soak_not_from_candidate_zip",
    "soak_wrong_executable_sha",
    "gui_missing_screenshot",
    "gui_blank_screenshot",
    "gpu_wrong_backend",
    "gpu_inconsistent_error_metrics",
    "cpu_fallback_reference_only_declaration",
    "cpu_fallback_initialization_not_prearmed",
    "cpu_fallback_executor_not_invoked",
    "cpu_fallback_not_same_step",
    "cpu_fallback_backend_not_reset",
    "cpu_fallback_state_mismatch",
    "cpu_fallback_nonfinite_state",
    "wrong_producer_evidence_hash",
    "producer_adapter_semantic_mismatch",
    "published_document_disagrees_with_frozen_bundle",
    "evidence_bundle_replaced_after_semantic_audit",
    "fake_official_revision",
    "wrong_official_path",
    "modified_official_fixture",
    "wrong_official_manifest_hash",
    "wrong_official_locator",
    "wrong_official_repository",
    "official_initial_load_particle_loss",
    "official_manifest_coverage_spoof",
    "official_runtime_coverage_summary_spoof",
    "official_missing_required_coverage",
    "promotion_wrong_candidate_filename",
    "promotion_wrong_symbols_candidate_filename",
    "promotion_wrong_stable_filename",
    "promotion_wrong_stable_sha256",
    "promotion_wrong_stable_symbols_filename",
    "promotion_wrong_stable_symbols_sha256",
    "promotion_byte_identity_false",
    "promotion_atomic_rename_false",
    "promotion_atomic_directory_false",
    "promotion_lock_false",
    "promotion_marker_false",
    "promotion_stable_target_preexists",
    "promotion_prepared_only",
    "promotion_transaction_incomplete",
    "promotion_stable_before_pre_gate",
    "stable_packager_direct_stable_name",
    "build_inputs_aggregate_mismatch",
    "ready_missing_release_documents",
    "supplemental_gate_failure",
    "package_manifest_corruption",
    "symbols_missing",
}

TRUSTED_ADAPTER_BINDINGS = {
    "trusted_release_driver_fresh_process_adapter",
    "trusted_finalizer_import_adapter",
}
# A trusted adapter may add or normalize only release-run identity and adapter
# metadata.  Every producer-owned result field must remain byte-semantically
# equivalent after JSON decoding, otherwise a fresh hash could still be used
# to launder a failing producer result into a PASS document.
TRUSTED_ADAPTER_METADATA_FIELDS = {
    "gate_name",
    "run_id",
    "commit",
    "candidate_sha256",
    "symbols_sha256",
    "symbols_member_sha256",
    "gate_started_at",
    "gate_finished_at",
    "identity_binding",
    "producer_evidence",
    "producer_evidence_sha256",
    "runtime_validator_binding",
    "runtime_validator_binding_sha256",
    "runtime_validator_artifact",
    "runtime_validator_artifact_sha256",
}


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest().upper()


def write(path: Path, value: object) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


def confined_file(root: Path, raw: object) -> Path | None:
    if not isinstance(raw, str) or not raw:
        return None
    candidate = Path(raw)
    path = candidate.resolve() if candidate.is_absolute() else (root / candidate).resolve()
    try:
        path.relative_to(root.resolve())
    except ValueError:
        return None
    return path if path.is_file() else None


def parse_timestamp(value: object) -> datetime | None:
    if not isinstance(value, str):
        return None
    try:
        parsed = datetime.fromisoformat(value.replace("Z", "+00:00"))
    except ValueError:
        return None
    return parsed if parsed.tzinfo is not None and parsed.utcoffset() is not None else None


def parse_run_id_timestamp(value: object) -> datetime | None:
    """Decode the UTC timestamp prefix carried by a release/harness run ID."""
    if not isinstance(value, str):
        return None
    match = RUN_ID_RE.fullmatch(value)
    if match is None:
        return None
    try:
        return datetime.strptime(
            match.group("timestamp"), "%Y%m%dT%H%M%SZ"
        ).replace(tzinfo=timezone.utc)
    except ValueError:
        return None


def run_id_interval_errors(
    run_id: object,
    started: datetime | None,
    finished: datetime | None,
    *,
    label: str,
    maximum_start_delay: timedelta,
) -> list[str]:
    """Bind a timestamped run ID to the producer interval it names."""
    stamp = parse_run_id_timestamp(run_id)
    if stamp is None or started is None or finished is None:
        return [f"{label} run ID timestamp is absent or invalid"]
    errors: list[str] = []
    if stamp - started > EVIDENCE_TIMESTAMP_TOLERANCE:
        errors.append(f"{label} run ID timestamp is after producer start")
    if started - stamp > maximum_start_delay:
        errors.append(f"{label} run ID timestamp is too old for producer start")
    if stamp - finished > EVIDENCE_TIMESTAMP_TOLERANCE:
        errors.append(f"{label} run ID timestamp is after producer finish")
    return errors


def finite_json_number(value: object) -> float | None:
    """Return a finite JSON number, rejecting strings, booleans and overflow."""
    if not isinstance(value, (int, float)) or isinstance(value, bool):
        return None
    try:
        converted = float(value)
    except (TypeError, ValueError, OverflowError):
        return None
    return converted if math.isfinite(converted) else None


def timestamp_interval_errors(
    started: datetime | None,
    finished: datetime | None,
    *,
    label: str,
) -> list[str]:
    """Validate a producer interval without trusting an unbounded clock claim."""
    errors: list[str] = []
    if started is None or finished is None or finished < started:
        return [f"{label} timestamps are absent or invalid"]
    now = datetime.now(timezone.utc)
    if started > now + EVIDENCE_CLOCK_SKEW_TOLERANCE:
        errors.append(f"{label} start timestamp is implausibly in the future")
    if finished > now + EVIDENCE_CLOCK_SKEW_TOLERANCE:
        errors.append(f"{label} finish timestamp is implausibly in the future")
    return errors


def is_exact_int(value: object, expected: int) -> bool:
    """Return true only for an actual JSON integer, never a bool."""
    return isinstance(value, int) and not isinstance(value, bool) and value == expected


def is_json_int(value: object) -> bool:
    """Return true only for a JSON integer (``true`` is not integer ``1``).

    Python deliberately considers ``bool`` a subclass of ``int``.  Release
    evidence is JSON, however, and a producer claiming ``schema_version: true``
    or ``files_total: true`` must not be accepted as if it had emitted the
    required integer.  Keep this check in one place so every evidence parser
    uses the same fail-closed type boundary.
    """
    return isinstance(value, int) and not isinstance(value, bool)


def json_semantically_equal(left: object, right: object) -> bool:
    """Compare decoded JSON without treating bool as int or dropping types."""
    if type(left) is not type(right):
        return False
    if isinstance(left, dict):
        return left.keys() == right.keys() and all(
            json_semantically_equal(left[key], right[key]) for key in left
        )
    if isinstance(left, list):
        return len(left) == len(right) and all(
            json_semantically_equal(a, b) for a, b in zip(left, right)
        )
    return left == right


def validate_bmp(path: Path, evidence: dict[str, object]) -> list[str]:
    try:
        data = path.read_bytes()
    except OSError as exc:
        return [f"GUI screenshot cannot be read: {exc}"]
    if len(data) < 54 or data[:2] != b"BM":
        return ["GUI screenshot is not a valid BMP header"]
    pixel_offset = int.from_bytes(data[10:14], "little", signed=False)
    width = int.from_bytes(data[18:22], "little", signed=True)
    raw_height = int.from_bytes(data[22:26], "little", signed=True)
    height = abs(raw_height)
    bits_per_pixel = int.from_bytes(data[28:30], "little", signed=False)
    compression = int.from_bytes(data[30:34], "little", signed=False)
    bitfields = bits_per_pixel == 32 and compression == 3
    if width <= 0 or height <= 0 or raw_height == -(1 << 31) or pixel_offset < 54:
        return ["GUI screenshot dimensions are invalid"]
    if bits_per_pixel not in (24, 32) or (compression != 0 and not bitfields):
        return ["GUI screenshot pixel format is unsupported"]
    rgb_mask = 0x00FFFFFF
    if bitfields:
        if len(data) < 66 or pixel_offset < 66:
            return ["GUI screenshot bitfield masks are truncated"]
        red_mask = int.from_bytes(data[54:58], "little", signed=False)
        green_mask = int.from_bytes(data[58:62], "little", signed=False)
        blue_mask = int.from_bytes(data[62:66], "little", signed=False)
        if (
            not red_mask
            or not green_mask
            or not blue_mask
            or red_mask & green_mask
            or red_mask & blue_mask
            or green_mask & blue_mask
        ):
            return ["GUI screenshot bitfield masks are invalid"]
        rgb_mask = red_mask | green_mask | blue_mask
    bytes_per_pixel = bits_per_pixel // 8
    row_stride = ((width * bits_per_pixel + 31) // 32) * 4
    required = pixel_offset + height * row_stride
    if required > len(data):
        return ["GUI screenshot pixel data is truncated"]
    nonzero_pixels = 0
    first_color: int | None = None
    different_color = False
    for y in range(height):
        row = pixel_offset + y * row_stride
        for x in range(width):
            offset = row + x * bytes_per_pixel
            packed = int.from_bytes(data[offset:offset + bytes_per_pixel], "little", signed=False)
            color = (packed & rgb_mask) if bitfields else (packed & 0x00FFFFFF)
            if color != 0:
                nonzero_pixels += 1
            if first_color is None:
                first_color = color
            elif color != first_color:
                different_color = True
    pixel_count = width * height
    distinct_colors = 0 if first_color is None else (2 if different_color else 1)
    errors = []
    if evidence.get("screenshot_width") != width:
        errors.append("GUI screenshot width does not match evidence")
    if evidence.get("screenshot_height") != height:
        errors.append("GUI screenshot height does not match evidence")
    if evidence.get("screenshot_bytes") != len(data):
        errors.append("GUI screenshot size does not match evidence")
    if evidence.get("screenshot_pixel_count") != pixel_count:
        errors.append("GUI screenshot pixel count does not match evidence")
    if evidence.get("screenshot_nonzero_pixels") != nonzero_pixels:
        errors.append("GUI screenshot nonzero pixel count does not match evidence")
    if evidence.get("screenshot_distinct_colors") != distinct_colors:
        errors.append("GUI screenshot color diversity does not match evidence")
    if nonzero_pixels < max(1, pixel_count // 100) or distinct_colors < 2:
        errors.append("GUI screenshot pixel content is blank or lacks rendered contrast")
    return errors


def validate_portable_runtime_contract(
    root: Path, raw: dict[str, object], candidate_sha256: object
) -> list[str]:
    errors: list[str] = []
    if not is_exact_int(raw.get("runtime_payload_schema_version"), PORTABLE_RUNTIME_PAYLOAD_SCHEMA_VERSION):
        errors.append("portable runtime payload schema version is invalid")
    if raw.get("candidate_source_markers_checked") is not True:
        errors.append("portable runtime candidate source-marker scan was not completed")
    if raw.get("candidate_source_tree_indicators") != []:
        errors.append("portable runtime candidate contains source-tree markers")
    if not is_exact_int(raw.get("runtime_target_executable_count"), 1):
        errors.append("portable runtime candidate must contain exactly one target executable")
    if raw.get("clean_shutdown") is not True:
        errors.append("portable runtime process did not exit successfully")
    for field in PORTABLE_RUNTIME_TRUE_FIELDS:
        if raw.get(field) is not True:
            errors.append(f"portable runtime outer evidence has invalid {field}")
    initial_particles = raw.get("initial_particle_count")
    post_step_particles = raw.get("post_step_particle_count")
    final_particles = raw.get("final_particle_count")
    if (
        not isinstance(initial_particles, int)
        or isinstance(initial_particles, bool)
        or initial_particles < 1
        or post_step_particles != initial_particles
        or final_particles != post_step_particles
    ):
        errors.append("portable runtime outer particle inventory is invalid")
    if not is_exact_int(raw.get("atmosphere_non_finite_cells"), 0):
        errors.append("portable runtime outer atmosphere finite-state count is invalid")
    for field in ("runtime_stdout", "runtime_stderr", "runtime_inner_evidence"):
        artifact = confined_file(root, raw.get(field))
        expected_hash = raw.get(field + "_sha256")
        if (
            artifact is None
            or not isinstance(expected_hash, str)
            or SHA256_RE.fullmatch(expected_hash) is None
            or sha256(artifact) != expected_hash
        ):
            errors.append(f"portable runtime {field} artifact is missing or has a hash mismatch")
    inner_path = confined_file(root, raw.get("runtime_inner_evidence"))
    if inner_path is None:
        return errors
    try:
        inner = json.loads(inner_path.read_text(encoding="utf-8-sig"))
    except (OSError, UnicodeError, json.JSONDecodeError):
        errors.append("portable runtime inner evidence is invalid JSON")
        return errors
    expected_identity = {
        "schema": EVIDENCE_SCHEMA,
        "schema_version": EVIDENCE_SCHEMA_VERSION,
        "payload_schema_version": PORTABLE_RUNTIME_PAYLOAD_SCHEMA_VERSION,
        "test": "portable_runtime",
        "run_id": raw.get("run_id"),
        "candidate_sha256": candidate_sha256,
        "status": "PASS",
        "passed": True,
    }
    for field, expected in expected_identity.items():
        if not json_semantically_equal(inner.get(field), expected):
            errors.append(f"portable runtime inner evidence has invalid {field}")
    for field in PORTABLE_RUNTIME_TRUE_FIELDS:
        if inner.get(field) is not True:
            errors.append(f"portable runtime inner evidence has invalid {field}")
        if raw.get(field) != inner.get(field):
            errors.append(f"portable runtime outer/inner {field} mismatch")
    inner_initial = inner.get("initial_particle_count")
    inner_post = inner.get("post_step_particle_count")
    inner_final = inner.get("final_particle_count")
    if (
        not isinstance(inner_initial, int)
        or isinstance(inner_initial, bool)
        or inner_initial < 1
        or inner_post != inner_initial
        or inner_final != inner_post
    ):
        errors.append("portable runtime inner particle inventory is invalid")
    for field in (
        "initial_particle_count", "post_step_particle_count", "final_particle_count",
        "atmosphere_non_finite_cells",
    ):
        if raw.get(field) != inner.get(field):
            errors.append(f"portable runtime outer/inner {field} mismatch")
    if not is_exact_int(inner.get("atmosphere_non_finite_cells"), 0):
        errors.append("portable runtime inner atmosphere finite-state count is invalid")
    return errors


def validate_raw_semantics(
    gate_name: str,
    raw: dict[str, object],
    *,
    root: Path,
    candidate_sha256: object,
    commit: object = None,
    source_snapshot_sha256: object = None,
    build_inputs_sha256: object = None,
    candidate_path: Path | None = None,
    symbols_path: Path | None = None,
    producer_document: bool = False,
) -> list[str]:
    errors: list[str] = []
    # Generic command gates must prove the process result and both captured
    # streams.  Specialized gates may expose stdout/stderr as optional
    # diagnostics, but a generic PASS cannot be declaration-only evidence.
    if gate_name in COMMAND_GATES:
        if not is_exact_int(raw.get("exit_code"), 0):
            errors.append("command gate PASS evidence must have exit_code=0")
        for field in ("stdout", "stderr"):
            artifact = confined_file(root, raw.get(field))
            expected_hash = raw.get(field + "_sha256")
            if (
                artifact is None
                or not isinstance(expected_hash, str)
                or SHA256_RE.fullmatch(expected_hash) is None
                or sha256(artifact) != expected_hash
            ):
                errors.append(f"command {field} artifact is missing or has a hash mismatch")
    else:
        # Preserve integrity checking for optional stream fields on
        # specialized evidence documents.
        for field in ("stdout", "stderr"):
            if field in raw or field + "_sha256" in raw:
                artifact = confined_file(root, raw.get(field))
                expected_hash = raw.get(field + "_sha256")
                if (
                    artifact is None
                    or not isinstance(expected_hash, str)
                    or SHA256_RE.fullmatch(expected_hash) is None
                    or sha256(artifact) != expected_hash
                ):
                    errors.append(f"command {field} artifact is missing or has a hash mismatch")
    if gate_name in META_GATES:
        # Meta evidence is an audit report which contains a snapshot of other
        # evidence rows.  Its envelope/raw identity and hashes are validated by
        # validate_gate_evidence/audit_evidence, but re-running the nested audit
        # here would recurse through the report (and trust a self-referential
        # result).  Check only the report's direct result contract.
        result_field = {
            "EvidenceHashIntegrity": "evidence_hash_integrity",
            "EvidenceSemanticIntegrity": "evidence_semantic_integrity",
            "DocumentationConsistency": "documentation_consistency",
        }[gate_name]
        expected_passed = raw.get("status") == "PASS"
        if raw.get(result_field) is not expected_passed:
            errors.append(f"meta-gate {result_field} does not match status")
        if not isinstance(raw.get("evidence"), list):
            errors.append("meta-gate audit report evidence rows are missing or not a list")
        if not isinstance(raw.get("documentation_errors"), list):
            errors.append("meta-gate documentation_errors is missing or not a list")
        return errors
    if gate_name == "SDL3GUI":
        required = (
            "window_created", "frame_rendered", "resize", "fullscreen_toggle",
            "keyboard", "mouse", "text_input", "clipboard", "screenshot_created",
            "clean_shutdown", "restart",
        )
        if any(raw.get(field) is not True for field in required):
            errors.append("GUI evidence is missing a required successful interaction")
        screenshot = confined_file(root, raw.get("artifact"))
        if screenshot is None:
            errors.append("GUI screenshot is missing or outside the current run")
        else:
            errors.extend(validate_bmp(screenshot, raw))
    elif gate_name == "WindowsPortableExtraction":
        required_true = (
            "candidate_extracted_to_fresh_directory", "sanitized_path_used",
            "project_build_tool_dependency_used",
        )
        if any(raw.get(field) is not True for field in required_true):
            errors.append("portable extraction evidence is missing a required runtime result")
        if not isinstance(raw.get("candidate_filename"), str) or not raw.get("candidate_filename", "").lower().endswith(".zip"):
            errors.append("portable extraction evidence is missing candidate filename")
        elif candidate_path is not None and raw.get("candidate_filename") != candidate_path.name:
            errors.append("portable extraction candidate filename does not match audited ZIP")
        errors.extend(validate_portable_runtime_contract(root, raw, candidate_sha256))
    elif gate_name == "WindowsCleanMachine":
        required_true = (
            "candidate_extracted_to_fresh_directory", "sanitized_path_used",
        )
        if raw.get("source_checkout_used") is not False:
            errors.append("clean runtime evidence used or detected a source checkout")
        if raw.get("project_build_tool_dependency_used") is not False:
            errors.append("clean runtime evidence used project build tooling")
        if any(raw.get(field) is not True for field in required_true):
            errors.append("clean runtime evidence is missing a required runtime result")
        if candidate_path is None or raw.get("candidate_filename") != candidate_path.name:
            errors.append("clean runtime evidence candidate filename does not match audited ZIP")
        if raw.get("source_tree_indicators") != [] or raw.get("source_markers_checked") is not True:
            errors.append("clean runtime evidence did not prove an empty source-marker scan")
        tools = raw.get("development_tools_detected")
        expected_tools = {"gcc.exe", "g++.exe", "meson.exe", "ninja.exe", "glslc.exe", "bash.exe"}
        if raw.get("development_tool_probe_completed") is not True or not isinstance(tools, dict) or set(tools) != expected_tools:
            errors.append("clean runtime evidence development-tool inventory is incomplete")
        if raw.get("runtime_validator") != "test_clean_release.ps1" or raw.get("runtime_job_kind") != "runtime-only":
            errors.append("clean runtime evidence is missing runtime-only validator attestation")
        if raw.get("validator_binding_checked") is not True:
            errors.append("clean runtime evidence did not confirm validator SHA binding")
        if (
            not isinstance(raw.get("runtime_validator_sha256"), str)
            or SHA256_RE.fullmatch(str(raw.get("runtime_validator_sha256"))) is None
        ):
            errors.append("clean runtime evidence has an invalid runtime validator SHA256")
        if not producer_document:
            if raw.get("identity_binding") != "trusted_finalizer_import_adapter":
                errors.append("clean runtime PASS must be imported by the trusted finalizer adapter")
            binding_path = confined_file(root, raw.get("runtime_validator_binding"))
            binding_hash = raw.get("runtime_validator_binding_sha256")
            validator_path = confined_file(root, raw.get("runtime_validator_artifact"))
            validator_hash = raw.get("runtime_validator_artifact_sha256")
            if (
                binding_path is None
                or binding_path.name != "runtime-validator-binding.json"
                or not isinstance(binding_hash, str)
                or SHA256_RE.fullmatch(binding_hash) is None
                or sha256(binding_path) != binding_hash
            ):
                errors.append("clean runtime validator binding evidence is missing or stale")
            if (
                validator_path is None
                or validator_path.name != "test_clean_release.ps1"
                or not isinstance(validator_hash, str)
                or SHA256_RE.fullmatch(validator_hash) is None
                or sha256(validator_path) != validator_hash
                or raw.get("runtime_validator_sha256") != validator_hash
            ):
                errors.append("clean runtime validator artifact is missing or stale")
            if binding_path is not None:
                try:
                    binding_value = json.loads(binding_path.read_text(encoding="utf-8-sig"))
                except (OSError, UnicodeError, json.JSONDecodeError):
                    binding_value = None
                if not isinstance(binding_value, dict):
                    errors.append("clean runtime validator binding JSON is invalid")
                else:
                    producer_path = confined_file(root, raw.get("producer_evidence"))
                    producer_hash = raw.get("producer_evidence_sha256")
                    expected_binding = {
                        "schema": EVIDENCE_SCHEMA,
                        "schema_version": EVIDENCE_SCHEMA_VERSION,
                        "test": "runtime_validator_binding",
                        "run_id": raw.get("run_id"),
                        "commit": commit,
                        "status": "PASS",
                        "passed": True,
                        "candidate_filename": raw.get("candidate_filename"),
                        "candidate_sha256": candidate_sha256,
                        "runtime_validator_filename": "test_clean_release.ps1",
                        "runtime_validator_sha256": validator_hash,
                        "clean_machine_evidence_filename": "windows-clean-machine.json",
                        "clean_machine_evidence_sha256": producer_hash,
                    }
                    for key, expected in expected_binding.items():
                        if not json_semantically_equal(binding_value.get(key), expected):
                            errors.append(
                                f"clean runtime validator binding has invalid {key}"
                            )
                    if parse_timestamp(binding_value.get("created_at")) is None:
                        errors.append("clean runtime validator binding created_at is invalid")
                    if (
                        producer_path is None
                        or producer_path.name != "windows-clean-machine.json"
                        or not isinstance(producer_hash, str)
                        or SHA256_RE.fullmatch(producer_hash) is None
                        or sha256(producer_path) != producer_hash
                    ):
                        errors.append("clean runtime producer evidence is missing or stale")
        errors.extend(validate_portable_runtime_contract(root, raw, candidate_sha256))
    elif gate_name == "Soak2Hours":
        def finite_metric(name: str, *, minimum: float | None = None,
                          maximum: float | None = None) -> float | None:
            value = finite_json_number(raw.get(name))
            if (
                value is None
                or (minimum is not None and value < minimum)
                or (maximum is not None and value > maximum)
            ):
                errors.append(f"soak metric {name} is absent, non-finite, or outside its bound")
                return None
            return value

        duration = finite_json_number(raw.get("wall_clock_seconds"))
        if duration is None or duration < SOAK_MIN_WALL_CLOCK_SECONDS:
            errors.append("soak wall clock is shorter than 7200 seconds")
        if raw.get("release_binding_passed") is not True:
            errors.append("soak release-run identity binding did not pass")
        harness_run_id = raw.get("harness_run_id")
        artifact_run_id = raw.get("artifact_run_id")
        if (
            not isinstance(harness_run_id, str)
            or re.fullmatch(r"[0-9]{8}T[0-9]{6}Z-[0-9a-f]{8}", harness_run_id) is None
            or artifact_run_id != harness_run_id
        ):
            errors.append("soak harness run identity is absent or does not match its fresh artifact directory")
        result_path = confined_file(root, raw.get("result_json"))
        result_hash = raw.get("result_json_sha256")
        result_value: dict[str, object] | None = None
        canonical_result_path = False
        if result_path is not None:
            try:
                relative_parts = result_path.relative_to(root.resolve()).parts
            except ValueError:
                relative_parts = ()
            canonical_result_path = (
                len(relative_parts) == 5
                and relative_parts[0] == "soak"
                and bool(relative_parts[1])
                and relative_parts[2] == "S20-FULL-CATALOG"
                and relative_parts[3] == artifact_run_id
                and relative_parts[4] == "result.json"
            )
        if (
            result_path is None
            or not canonical_result_path
        ):
            errors.append(
                "soak result.json is missing or not in the canonical "
                "soak/<machine>/S20-FULL-CATALOG/<run_id>/result.json path"
            )
        elif (
            not isinstance(result_hash, str)
            or SHA256_RE.fullmatch(result_hash) is None
            or sha256(result_path) != result_hash
        ):
            errors.append("soak result.json bytes do not match the analyzer hash binding")
        else:
            try:
                parsed_result = json.loads(result_path.read_text(encoding="utf-8-sig"))
            except (OSError, UnicodeError, json.JSONDecodeError):
                errors.append("soak result.json is not valid JSON")
            else:
                if not isinstance(parsed_result, dict):
                    errors.append("soak result.json root is not an object")
                else:
                    result_value = parsed_result
        if result_value is not None:
            if not is_exact_int(result_value.get("schema_version"), 1):
                errors.append("soak result.json schema_version is invalid")
            if result_value.get("run_id") != harness_run_id:
                errors.append("soak result.json run_id does not match the harness/artifact run")
            if result_value.get("long_run") is not True or result_value.get("smoke_run") is not False:
                errors.append("soak result.json is not a formal long-run execution")
            result_started = parse_timestamp(result_value.get("start_time_utc"))
            result_finished = parse_timestamp(result_value.get("end_time_utc"))
            gate_started = parse_timestamp(raw.get("gate_started_at"))
            gate_finished = parse_timestamp(raw.get("gate_finished_at"))
            errors.extend(run_id_interval_errors(
                raw.get("run_id"), gate_started, gate_finished,
                label="soak release", maximum_start_delay=RELEASE_RUN_MAX_AGE,
            ))
            errors.extend(run_id_interval_errors(
                harness_run_id, result_started, result_finished,
                label="soak harness",
                maximum_start_delay=EVIDENCE_CLOCK_SKEW_TOLERANCE,
            ))
            if (
                result_started is None
                or result_finished is None
                or gate_started is None
                or gate_finished is None
                or result_finished < result_started
                or result_started < gate_started
                or result_finished > gate_finished
            ):
                errors.append("soak result.json execution timestamps are outside the current release gate")
            for field in (
                "sample_id", "source_commit", "public_zip_sha256", "exe_sha256",
                "wall_clock_seconds", "warmup_seconds", "sample_seconds",
                "simulation_steps", "nan_count", "inf_count", "stalls",
                "omni_atmosphere_active",
            ):
                if not json_semantically_equal(result_value.get(field), raw.get(field)):
                    errors.append(f"soak analyzer summary disagrees with result.json field: {field}")
            if result_value.get("source_commit") != commit:
                errors.append("soak result.json source commit does not match the aggregate")
            if result_value.get("public_zip_sha256") != candidate_sha256:
                errors.append("soak result.json candidate identity does not match the aggregate")
            result_duration = finite_json_number(result_value.get("wall_clock_seconds"))
            if result_duration is None or result_duration < SOAK_MIN_WALL_CLOCK_SECONDS:
                errors.append("soak result.json wall clock is shorter than 7200 seconds")
            if result_started is not None and result_finished is not None:
                elapsed_seconds = (result_finished - result_started).total_seconds()
                if elapsed_seconds < SOAK_MIN_WALL_CLOCK_SECONDS:
                    errors.append("soak result.json timestamps span less than 7200 seconds")
                if (
                    result_duration is not None
                    and abs(result_duration - elapsed_seconds)
                    > SOAK_WALL_CLOCK_TOLERANCE_SECONDS
                ):
                    errors.append(
                        "soak result.json wall clock does not match its execution timestamps"
                    )
        analyzer_path = confined_file(root, raw.get("analyzer_evidence"))
        analyzer_hash = raw.get("analyzer_evidence_sha256")
        analyzer_value: dict[str, object] | None = None
        if (
            analyzer_path is None
            or analyzer_path.name != "soak-analyzer.json"
            or not isinstance(analyzer_hash, str)
            or SHA256_RE.fullmatch(analyzer_hash) is None
            or sha256(analyzer_path) != analyzer_hash
        ):
            errors.append("soak independent analyzer evidence is missing or stale")
        else:
            try:
                parsed_analyzer = json.loads(
                    analyzer_path.read_text(encoding="utf-8-sig")
                )
            except (OSError, UnicodeError, json.JSONDecodeError):
                errors.append("soak independent analyzer evidence is not valid JSON")
            else:
                if not isinstance(parsed_analyzer, dict):
                    errors.append("soak independent analyzer evidence root is not an object")
                else:
                    analyzer_value = parsed_analyzer
        if analyzer_value is not None:
            expected_analyzer_identity = {
                "schema": EVIDENCE_SCHEMA,
                "schema_version": EVIDENCE_SCHEMA_VERSION,
                "test": "soak_2h",
                "status": "PASS",
                "passed": True,
                "run_id": harness_run_id,
                "candidate_sha256": candidate_sha256,
            }
            for key, expected in expected_analyzer_identity.items():
                if not json_semantically_equal(analyzer_value.get(key), expected):
                    errors.append(f"soak independent analyzer {key} is inconsistent")
            for key, producer_value in analyzer_value.items():
                if key in {
                    "schema", "schema_version", "test", "status", "passed",
                    "run_id", "candidate_sha256",
                }:
                    continue
                if key not in raw or not json_semantically_equal(raw.get(key), producer_value):
                    errors.append(f"soak driver changed analyzer-owned field: {key}")
        if not is_exact_int(raw.get("analysis_exit_code"), 0):
            errors.append("soak independent analyzer did not exit successfully")
        if (
            raw.get("analyzer_schema") != EVIDENCE_SCHEMA
            or not is_exact_int(raw.get("analyzer_schema_version"), EVIDENCE_SCHEMA_VERSION)
            or raw.get("analyzer_test") != "soak_2h"
            or raw.get("analyzer_status") != "PASS"
            or raw.get("analyzer_passed") is not True
        ):
            errors.append("soak analyzer schema or result is not a genuine PASS")
        if (
            raw.get("observed_candidate_sha256") != candidate_sha256
            or raw.get("analyzer_candidate_sha256") != candidate_sha256
        ):
            errors.append("soak observed/analyzer candidate identity does not match current candidate")
        if raw.get("sample_id") != "S20-FULL-CATALOG" or raw.get("long_run_requested") is not True:
            errors.append("soak did not run the mandatory S20 long-run scenario")
        for field in (
            "duration_pass", "runtime_pass", "sample_execution_pass",
            "event_evidence_complete", "signal_evidence_complete",
            "signal_behavior_pass", "signal_stop_pass",
            "fixture_evidence_complete", "fixture_activity_pass",
            "scenario_behavior_pass", "long_run_evidence_complete",
            "long_run_duration_pass", "long_run_behavior_pass",
            "long_run_settings_recovery_pass",
        ):
            if raw.get(field) is not True:
                errors.append(f"soak execution gate is not PASS: {field}")
        for field in (
            "long_run_save_load_cycles", "long_run_language_switches",
            "long_run_module_toggle_cycles",
        ):
            value = raw.get(field)
            if not isinstance(value, int) or isinstance(value, bool) or value < 10:
                errors.append(f"soak did not complete enough {field}")
        if raw.get("long_run_gate_pass") is not True or raw.get("performance_gate_pass") is not True:
            errors.append("soak long-run or performance gate did not pass")
        if raw.get("omni_atmosphere_active") is not True:
            errors.append("soak did not execute with OmniAtmosphere active")
        if any(
            not is_exact_int(raw.get(field), 0)
            for field in ("nan_count", "inf_count", "stalls")
        ):
            errors.append("soak reported non-finite values or simulation stalls")
        if raw.get("public_zip_sha256") != candidate_sha256:
            errors.append("soak public ZIP hash does not match current candidate")
        if commit is None or raw.get("source_commit") != commit:
            errors.append("soak source commit does not match aggregate commit")
        if (
            raw.get("candidate_extracted_to_fresh_directory") is not True
            or not is_exact_int(raw.get("candidate_target_executable_count"), 1)
            or raw.get("executable_source") != "candidate_zip"
        ):
            errors.append("soak did not execute the unique target executable from a fresh candidate ZIP extraction")
        candidate_executable_sha = raw.get("candidate_executable_sha256")
        build_executable_sha = raw.get("build_executable_sha256")
        executed_sha = raw.get("exe_sha256")
        if any(
            not isinstance(value, str) or SHA256_RE.fullmatch(value) is None
            for value in (candidate_executable_sha, build_executable_sha, executed_sha)
        ):
            errors.append("soak executable SHA256 identity is absent or invalid")
        elif not (candidate_executable_sha == build_executable_sha == executed_sha):
            errors.append("soak executed executable identity does not match the candidate/build executable")
        if candidate_path is None or not candidate_path.is_file():
            errors.append("soak executable identity is not bound to an actual candidate ZIP")
        else:
            try:
                with zipfile.ZipFile(candidate_path) as archive:
                    executable_members = [
                        info for info in archive.infolist()
                        if not info.is_dir()
                        and info.filename.replace("\\", "/").endswith("/tpt-zh-omnipack.exe")
                    ]
                    if len(executable_members) != 1:
                        errors.append("candidate ZIP does not contain exactly one target executable for soak binding")
                    else:
                        packaged_executable_sha = hashlib.sha256(
                            archive.read(executable_members[0])
                        ).hexdigest().upper()
                        if executed_sha != packaged_executable_sha:
                            errors.append("soak executable SHA256 does not match the candidate ZIP member bytes")
            except (OSError, KeyError, zipfile.BadZipFile):
                errors.append("candidate ZIP executable bytes cannot be audited for soak binding")
        for field in (
            "heartbeat_progress_pass", "heartbeat_timing_pass", "finite_state_pass",
            "atmosphere_range_pass", "atmosphere_mass_closure_pass",
            "heartbeat_summary_match", "long_run_diagnostics_pass",
        ):
            if raw.get(field) is not True:
                errors.append(f"soak diagnostic gate is not PASS: {field}")
        simulation_steps = raw.get("simulation_steps")
        if not isinstance(simulation_steps, int) or isinstance(simulation_steps, bool) or simulation_steps <= 0:
            errors.append("soak simulation step count is absent or invalid")
        heartbeat_count = raw.get("heartbeat_count")
        if not isinstance(heartbeat_count, int) or isinstance(heartbeat_count, bool) or heartbeat_count < 121:
            errors.append("soak heartbeat count does not cover two hours at 30-60 second cadence")
        finite_metric("warmup_seconds", minimum=60.0)
        finite_metric("sample_seconds", minimum=7200.0)
        finite_metric("heartbeat_interval_seconds", minimum=30.0, maximum=60.0)
        finite_metric("maximum_heartbeat_gap_seconds", minimum=0.0, maximum=60.001)
        for field in (
            "peak_working_set_bytes", "peak_private_bytes", "frame_samples",
            "process_samples", "memory_process_samples", "observed_particle_min",
            "observed_particle_max", "particle_tail_first", "particle_tail_last",
            "working_set_tail_first", "working_set_tail_last",
            "private_tail_first", "private_tail_last",
        ):
            finite_metric(field, minimum=1.0)
        if raw.get("unbounded_growth") is not False:
            errors.append("soak particle population is not proven bounded")
        if raw.get("memory_leak_suspected") is not False:
            errors.append("soak memory diagnostics indicate or omit a leak result")
        for field in (
            "atmosphere_mass_initial_kg", "atmosphere_mass_final_kg",
            "atmosphere_mass_min_kg", "atmosphere_mass_max_kg",
        ):
            finite_metric(field, minimum=0.0)
        for field in (
            "atmosphere_mass_residual_abs_max_kg",
            "species_mass_residual_abs_max_kg",
        ):
            finite_metric(field, minimum=0.0, maximum=1.0e-8)
        density_min = finite_metric("minimum_density_kg_m3", minimum=0.0)
        density_max = finite_metric("maximum_density_kg_m3", minimum=0.0)
        pressure_min = finite_metric("minimum_pressure_pa", minimum=0.0)
        pressure_max = finite_metric("maximum_pressure_pa", minimum=0.0)
        temperature_min = finite_metric("minimum_temperature_k", minimum=0.0)
        temperature_max = finite_metric("maximum_temperature_k", minimum=0.0)
        for label, lower, upper in (
            ("density", density_min, density_max),
            ("pressure", pressure_min, pressure_max),
            ("temperature", temperature_min, temperature_max),
        ):
            if lower is not None and upper is not None and (lower <= 0.0 or upper < lower):
                errors.append(f"soak {label} range is invalid")
        for field in ("input_ops", "output_ops"):
            value = raw.get(field)
            if (
                not isinstance(value, dict)
                or not isinstance(value.get("bytes"), int)
                or isinstance(value.get("bytes"), bool)
                or value.get("bytes", 0) <= 0
                or not isinstance(value.get("sha256"), str)
                or SHA256_RE.fullmatch(str(value.get("sha256", ""))) is None
            ):
                errors.append(f"soak {field} roundtrip artifact identity is invalid")
    elif gate_name == "ArtifactImmutability":
        if raw.get("before_sha256") != candidate_sha256 or raw.get("after_sha256") != candidate_sha256:
            errors.append("artifact immutability hashes do not match current candidate")
        if candidate_path is None or not candidate_path.is_file():
            errors.append("artifact immutability evidence is not bound to an actual candidate ZIP")
        elif sha256(candidate_path) != candidate_sha256:
            errors.append("artifact immutability evidence does not match candidate ZIP bytes")
    elif gate_name == "SourceTreeClean":
        if raw.get("porcelain_output") != "":
            errors.append("source tree clean evidence must contain an explicit empty porcelain_output")
        if (
            not isinstance(source_snapshot_sha256, str)
            or SHA256_RE.fullmatch(source_snapshot_sha256) is None
            or raw.get("content_hash") != source_snapshot_sha256
        ):
            errors.append("source tree clean evidence is not bound to the aggregate source snapshot")
        tracked_files = raw.get("tracked_files")
        if not isinstance(tracked_files, int) or isinstance(tracked_files, bool) or tracked_files <= 0:
            errors.append("source tree clean evidence has no valid tracked-file count")
    elif gate_name == "SourceSnapshotImmutability":
        if raw.get("commit_start") != raw.get("commit_end") or raw.get("branch_start") != raw.get("branch_end"):
            errors.append("source commit or branch changed during the run")
        if raw.get("git_status_start") != raw.get("git_status_end"):
            errors.append("source status changed during the run")
        if raw.get("git_status_start") != "" or raw.get("git_status_end") != "":
            errors.append("stable source snapshot checkpoints must both be clean")
        snapshots = [
            raw.get("content_hash_start"),
            raw.get("content_hash_after_configure"),
            raw.get("content_hash_after_build"),
            raw.get("content_hash_before_package"),
            raw.get("content_hash_after_package"),
            raw.get("content_hash_end"),
        ]
        if any(not isinstance(value, str) or SHA256_RE.fullmatch(value) is None for value in snapshots):
            errors.append("source content snapshots are absent or invalid")
        elif len(set(snapshots)) != 1:
            errors.append("source content snapshot changed during the run")
        if commit is not None and any(raw.get(field) != commit for field in ("commit_start", "commit_end")):
            errors.append("source snapshot commit checkpoints are not bound to aggregate commit")
        if source_snapshot_sha256 is None or not isinstance(source_snapshot_sha256, str) or SHA256_RE.fullmatch(source_snapshot_sha256) is None:
            errors.append("aggregate source snapshot SHA256 is absent or invalid")
        elif any(value != source_snapshot_sha256 for value in snapshots):
            errors.append("source snapshot checkpoints are not bound to aggregate source_snapshot_sha256")
        counts = [
            raw.get("tracked_files_start"), raw.get("tracked_files_after_configure"),
            raw.get("tracked_files_after_build"),
            raw.get("tracked_files_before_package"), raw.get("tracked_files_after_package"),
            raw.get("tracked_files_end"),
        ]
        if any(not is_json_int(value) or value <= 0 for value in counts):
            errors.append("source tracked-file snapshot counts are absent or invalid")
        build_inputs = [
            raw.get("build_inputs_hash_after_configure"),
            raw.get("build_inputs_hash_after_build"),
            raw.get("build_inputs_hash_before_package"),
            raw.get("build_inputs_hash_after_package"),
            raw.get("build_inputs_hash_end"),
        ]
        if any(not isinstance(value, str) or SHA256_RE.fullmatch(value) is None for value in build_inputs):
            errors.append("build-input snapshots are absent or invalid")
        elif len(set(build_inputs)) != 1:
            errors.append("verified build inputs changed during the run")
        if not isinstance(build_inputs_sha256, str) or SHA256_RE.fullmatch(build_inputs_sha256) is None:
            errors.append("aggregate build_inputs_sha256 is absent or invalid")
        elif any(value != build_inputs_sha256 for value in build_inputs):
            errors.append("build-input checkpoints are not bound to aggregate build_inputs_sha256")
        ready_fields = (
            "build_inputs_ready_after_configure", "build_inputs_ready_after_build",
            "build_inputs_ready_before_package", "build_inputs_ready_after_package",
            "build_inputs_ready_end",
        )
        if any(raw.get(field) is not True for field in ready_fields):
            errors.append("one or more release build-input checkpoints are not verified")
    elif gate_name == "OfficialTPTCorpusProvenance":
        errors.extend(official_provenance_identity_errors(raw))
        rows = raw.get("files")
        total = raw.get("files_total")
        if (
            not isinstance(rows, list)
            or not is_json_int(total)
            or total <= 0
            or len(rows) != total
            or not is_json_int(raw.get("files_verified"))
            or raw.get("files_verified") != total
            or not is_exact_int(raw.get("files_failed"), 0)
        ):
            errors.append("official provenance did not verify every file")
        else:
            if any(
                not isinstance(row, dict)
                or not isinstance(row.get("path"), str)
                or row.get("match") is not True
                or not isinstance(row.get("upstream_sha256"), str)
                or SHA256_RE.fullmatch(str(row.get("upstream_sha256", ""))) is None
                or row.get("upstream_sha256") != row.get("manifest_sha256")
                or row.get("upstream_sha256") != row.get("fixture_sha256")
                or (
                    raw.get("provenance_schema") == OFFICIAL_PROVENANCE_V2
                    and row.get("source_kind") not in {"github_git", "official_web_save"}
                )
                for row in rows
            ):
                errors.append("official provenance contains an unverified or hash-inconsistent file row")
            if raw.get("provenance_schema") == OFFICIAL_PROVENANCE_V2:
                for row in rows:
                    errors.extend(official_provenance_v2_row_errors(row))
            if len({row.get("path") for row in rows if isinstance(row, dict)}) != total:
                errors.append("official provenance contains duplicate file paths")
    elif gate_name == "OfficialTPTSaveCompatibility":
        total = raw.get("files_total")
        rows = raw.get("files")
        probe_sha256 = raw.get("probe_sha256")
        if raw.get("provenance_schema") == OFFICIAL_PROVENANCE_V2:
            repositories = raw.get("source_repositories")
            if not isinstance(repositories, list) or not repositories or not all(isinstance(item, str) for item in repositories):
                errors.append("v2 official compatibility source identity is invalid")
        elif raw.get("source_repository") != OFFICIAL_REPOSITORY or not isinstance(raw.get("source_revision"), str) or not REVISION_RE.fullmatch(str(raw.get("source_revision", ""))):
            errors.append("official compatibility source identity is invalid")
        if not isinstance(probe_sha256, str) or SHA256_RE.fullmatch(probe_sha256) is None:
            errors.append("official compatibility probe identity is invalid")
        if (
            not isinstance(rows, list)
            or not is_json_int(total)
            or total <= 0
            or len(rows) != total
            or not is_json_int(raw.get("files_passed"))
            or raw.get("files_passed") != total
            or not is_exact_int(raw.get("files_failed"), 0)
        ):
            errors.append("official compatibility did not pass every provenance-verified save")
        else:
            phases = (
                "load", "missing_elements_zero", "initial_load_state_validate", "simulate", "save", "reload", "state_validate",
                "negative_block_map", "negative_legacy_field", "negative_sign",
                "negative_validity_mask", "negative_deterministic_frame",
                "negative_simulation_option", "negative_codec_roundtrip",
            )
            row_categories: list[set[str]] = []
            row_contract_invalid = False
            for row in rows:
                categories = official_coverage_categories(
                    row.get("coverage_metrics") if isinstance(row, dict) else None
                )
                if (
                    categories is None
                    or row.get("coverage_metrics_valid") is not True
                    or row.get("coverage_categories") != sorted(categories)
                ):
                    row_contract_invalid = True
                    categories = set()
                row_categories.append(categories)
                if raw.get("provenance_schema") == OFFICIAL_PROVENANCE_V2 and isinstance(row, dict):
                    compatibility_identity = dict(row)
                    compatibility_identity["metadata"] = row.get("source_metadata")
                    errors.extend(official_provenance_v2_row_errors(compatibility_identity))
            recomputed_observed = sorted(set().union(*row_categories)) if row_categories else []
            recomputed_missing = sorted(set(OFFICIAL_COVERAGE_REQUIRED).difference(recomputed_observed))
            if any(
                not isinstance(row, dict)
                or not isinstance(row.get("path"), str)
                or row.get("provenance_hash_binding_passed") is not True
                or not isinstance(row.get("fixture_sha256"), str)
                or SHA256_RE.fullmatch(str(row.get("fixture_sha256", ""))) is None
                or row.get("probe_sha256") != probe_sha256
                or row.get("passed") is not True
                or not is_exact_int(row.get("exit_code"), 0)
                or row.get("initial_particle_inventory") is not True
                or not is_json_int(row.get("input_particles"))
                or not is_json_int(row.get("initial_loaded_particles"))
                or row.get("initial_loaded_particles") != row.get("input_particles")
                or not is_json_int(row.get("output_particles"))
                or any(row.get(phase) is not True for phase in phases)
                for row in rows
            ):
                errors.append("official compatibility contains a failed hash binding or runtime phase")
            elif len({row.get("path") for row in rows}) != total:
                errors.append("official compatibility contains duplicate file paths")
            if row_contract_invalid:
                errors.append("official compatibility coverage metrics or per-file categories are invalid")
            if raw.get("coverage_contract") != OFFICIAL_COVERAGE_CONTRACT:
                errors.append("official compatibility coverage contract is invalid")
            if raw.get("coverage_required") != list(OFFICIAL_COVERAGE_REQUIRED):
                errors.append("official compatibility required coverage set is invalid")
            if raw.get("coverage_observed") != recomputed_observed:
                errors.append("official compatibility observed coverage summary is not derived from probe metrics")
            if raw.get("coverage_missing") != recomputed_missing:
                errors.append("official compatibility missing coverage summary is not derived from probe metrics")
            if raw.get("coverage_passed") is not (not recomputed_missing) or recomputed_missing:
                errors.append("official compatibility does not cover every mandatory save category")
    elif gate_name == "GPUNumericalValidation":
        if (
            raw.get("schema") != EVIDENCE_SCHEMA
            or not is_exact_int(raw.get("schema_version"), EVIDENCE_SCHEMA_VERSION)
            or raw.get("test") != "gpu_validation"
            or raw.get("status") != "PASS"
            or raw.get("passed") is not True
            or raw.get("supported") is not True
            or raw.get("fallback") is not False
            or raw.get("backend") != "SDL_GPU Vulkan"
            or raw.get("reason") != "PASS"
        ):
            errors.append("GPU numerical validation schema, backend, or execution mode is invalid")
        max_abs = raw.get("max_abs_error")
        max_rel = raw.get("max_rel_error")
        first_mismatch = raw.get("first_mismatch_index")
        if not isinstance(max_abs, (int, float)) or isinstance(max_abs, bool) or not math.isfinite(max_abs) or max_abs < 0 or max_abs > 1.0e-3:
            errors.append("GPU maximum absolute error is missing, non-finite, or above the release bound")
        if not isinstance(max_rel, (int, float)) or isinstance(max_rel, bool) or not math.isfinite(max_rel) or max_rel < 0 or max_rel > 1.0e-2:
            errors.append("GPU maximum relative error is missing, non-finite, or above the release bound")
        if first_mismatch != -1:
            errors.append("GPU numerical validation reports a mismatching sample")
    elif gate_name == "CPUFallbackValidation":
        executor_invocations = raw.get("runtime_executor_invocations")
        nonfinite_counts = (
            raw.get("control_nonfinite_cells"),
            raw.get("initialization_nonfinite_cells"),
            raw.get("runtime_nonfinite_cells"),
            raw.get("nonfinite_cells"),
        )
        if (
            raw.get("schema") != EVIDENCE_SCHEMA
            or not is_exact_int(raw.get("schema_version"), EVIDENCE_SCHEMA_VERSION)
            or raw.get("test") != "cpu_fallback"
            or raw.get("status") != "PASS"
            or raw.get("passed") is not True
            or raw.get("fallback") is not True
            or raw.get("backend") != "CPU"
            or raw.get("reason") != "production_runtime_failure_same_step_cpu_fallback"
            or any(raw.get(field) is not True for field in CPU_FALLBACK_TRUE_FIELDS)
            or raw.get("backend_available_after_failure") is not False
            or not isinstance(executor_invocations, int)
            or isinstance(executor_invocations, bool)
            or executor_invocations != 1
            or any(
                not isinstance(value, int) or isinstance(value, bool) or value != 0
                for value in nonfinite_counts
            )
            or raw.get("runtime_failure_code") != "injected_runtime_thermal_failure"
            or raw.get("backend_detail") != "runtime thermal diffusion fallback: injected_runtime_thermal_failure"
        ):
            errors.append("CPU fallback evidence does not prove the production same-step runtime failure path")
        for field, tolerance in (("mass_residual_kg", 1.0e-12), ("energy_residual_j", 1.0e-8)):
            value = raw.get(field)
            if (
                not isinstance(value, (int, float))
                or isinstance(value, bool)
                or not math.isfinite(value)
                or abs(value) > tolerance
            ):
                errors.append(f"CPU fallback {field} is missing, non-finite, or outside its bound")
    elif gate_name == "SDL3Runtime":
        stdout = confined_file(root, raw.get("stdout"))
        values: dict[str, str] = {}
        if stdout is not None:
            for line in stdout.read_text(encoding="utf-8", errors="replace").splitlines():
                if "=" in line:
                    key, value = line.split("=", 1)
                    if re.fullmatch(r"[A-Za-z0-9_]+", key):
                        values[key] = value
        expected = {
            "sdl_backend": "SDL3",
            "production_thermal_diffusion_built": "true",
            "gpu_supported": "true",
            "spirv_supported": "true",
            "gpu_backend": "vulkan",
            "compute_poc_executed": "true",
            "deterministic_compare": "true",
            "fallback_cpu": "false",
            "cuda_backend_available": "false",
            "cuda_backend_status": "not_implemented_optional_future_backend",
        }
        if any(values.get(key) != value for key, value in expected.items()):
            errors.append("SDL3 runtime probe did not execute Vulkan compute deterministically")
    elif gate_name == "CandidateSHA256":
        if raw.get("candidate_sha256") != candidate_sha256 or raw.get("sidecars_match") is not True:
            errors.append("candidate SHA256 evidence or sidecars do not match")
        if candidate_path is None or not candidate_path.is_file():
            errors.append("candidate SHA256 evidence is not bound to an actual candidate ZIP")
        elif sha256(candidate_path) != candidate_sha256:
            errors.append("candidate SHA256 evidence does not match candidate ZIP bytes")
        else:
            sidecar = candidate_path.with_suffix(candidate_path.suffix + ".sha256")
            expected_line = f"{candidate_sha256}  {candidate_path.name}"
            if not sidecar.is_file() or sidecar.read_text(encoding="ascii").strip() != expected_line:
                errors.append("candidate ZIP SHA256 sidecar is missing or stale")
        if symbols_path is None or not symbols_path.is_file():
            errors.append("candidate SHA256 evidence is not bound to an actual symbols ZIP")
        elif raw.get("symbols_sha256") != sha256(symbols_path):
            errors.append("candidate SHA256 evidence does not match symbols ZIP bytes")
        else:
            sidecar = symbols_path.with_suffix(symbols_path.suffix + ".sha256")
            expected_line = f"{raw.get('symbols_sha256')}  {symbols_path.name}"
            if not sidecar.is_file() or sidecar.read_text(encoding="ascii").strip() != expected_line:
                errors.append("symbols ZIP SHA256 sidecar is missing or stale")
    elif gate_name == "NegativeGateSuite":
        if raw.get("candidate_sha256_observed") != candidate_sha256:
            errors.append("negative gate suite did not verify the current candidate bytes")
        baseline_total = raw.get("baselines_total")
        baseline_passed = raw.get("baselines_passed")
        baselines = raw.get("baselines")
        baseline_failed = raw.get("baselines_failed")
        if (
            not is_json_int(baseline_total)
            or baseline_total <= 0
            or not is_json_int(baseline_passed)
            or baseline_passed != baseline_total
            or not isinstance(baselines, dict)
            or len(baselines) != baseline_total
            or any(value is not True for value in baselines.values())
            or baseline_failed not in ([], None)
        ):
            errors.append("negative gate suite positive baselines are incomplete or rejected")
        total = raw.get("attacks_total")
        rejected = raw.get("attacks_rejected")
        attacks = raw.get("attacks")
        failed = raw.get("attacks_failed")
        if (
            not is_json_int(total)
            or total <= 0
            or not is_json_int(rejected)
            or rejected != total
        ):
            errors.append("negative gate suite did not reject every attack")
        if not isinstance(attacks, dict) or len(attacks) != total or any(value is not True for value in attacks.values()):
            errors.append("negative gate suite attack matrix is incomplete or contains a bypass")
        if isinstance(attacks, dict):
            missing = sorted(REQUIRED_NEGATIVE_ATTACKS.difference(attacks))
            if missing:
                errors.append(
                    "negative gate suite is missing required attacks: " + ", ".join(missing)
                )
        if failed not in ([], None):
            errors.append("negative gate suite reports failed attacks")
    elif gate_name == "CandidatePromotion":
        run_id = raw.get("run_id")
        expected_candidate = (
            f"TPT-ZH-OmniPack-1.1.0-staging-{run_id}-Windows-x64-SDL3.zip"
        )
        expected_symbols = (
            f"TPT-ZH-OmniPack-1.1.0-staging-{run_id}-Windows-x64-Symbols.zip"
        )
        if raw.get("candidate_filename") != expected_candidate:
            errors.append("promotion candidate filename is not bound to the current run")
        if raw.get("symbols_candidate_filename") != expected_symbols:
            errors.append("promotion symbols filename is not bound to the current run")
        if raw.get("stable_filename") != "TPT-ZH-OmniPack-1.1.0-Windows-x64-SDL3.zip":
            errors.append("promotion stable filename is invalid")
        if raw.get("stable_sha256_expected") != candidate_sha256:
            errors.append("promotion stable identity does not match the candidate")
        if raw.get("stable_symbols_filename") != "TPT-ZH-OmniPack-1.1.0-Windows-x64-Symbols.zip":
            errors.append("promotion stable symbols filename is invalid")
        if raw.get("stable_symbols_sha256_expected") != raw.get("symbols_sha256"):
            errors.append("promotion stable symbols identity does not match the symbols candidate")
        if raw.get("promotion_phase") != "published_and_reaudited":
            errors.append("promotion evidence is not in the completed publication phase")
        if raw.get("publication_state") != "published_and_reaudited":
            errors.append("promotion publication state is not completed")
        if raw.get("transaction_complete") is not True:
            errors.append("promotion transaction is not complete")
        if raw.get("post_publish_audit_passed") is not True:
            errors.append("promotion post-publish audit did not pass")
        if raw.get("stable_copy_published") is not True or raw.get("stable_sha256_observed") != candidate_sha256:
            errors.append("promotion evidence does not bind the published stable candidate bytes")
        if raw.get("stable_symbols_copy_published") is not True or raw.get("stable_symbols_sha256_observed") != raw.get("symbols_sha256"):
            errors.append("promotion evidence does not bind the published stable symbols bytes")
        for field in (
            "byte_for_byte_identity", "atomic_rename_only",
            "atomic_directory_publish", "exclusive_output_lock_acquired",
            "promotion_complete_marker_required",
            "stable_names_absent_before_final_audit",
        ):
            if raw.get(field) is not True:
                errors.append(f"promotion invariant is not proven: {field}")
        pre_gate_finished = parse_timestamp(raw.get("pre_promotion_gate_finished_at"))
        stable_name_created = parse_timestamp(raw.get("stable_name_creation_started_at"))
        if pre_gate_finished is None or stable_name_created is None:
            errors.append("promotion timing evidence is absent or invalid")
        elif stable_name_created < pre_gate_finished:
            errors.append("stable artifact names were created before the pre-promotion gate finished")
        if raw.get("pre_promotion_gate_passed") is not True:
            errors.append("promotion evidence does not prove the pre-promotion gate passed")
        if raw.get("stable_names_absent_before_pre_promotion_gate") is not True:
            errors.append("promotion evidence does not prove stable names were absent before the pre-promotion gate")
        if candidate_path is None or not candidate_path.is_file():
            errors.append("promotion evidence is not bound to an actual staging candidate")
        else:
            if candidate_path.name != raw.get("candidate_filename"):
                errors.append("promotion evidence candidate filename does not match actual ZIP")
            if sha256(candidate_path) != candidate_sha256:
                errors.append("promotion evidence candidate bytes do not match aggregate SHA256")
        if symbols_path is None or not symbols_path.is_file():
            errors.append("promotion evidence is not bound to an actual symbols ZIP")
        else:
            if symbols_path.name != raw.get("symbols_candidate_filename"):
                errors.append("promotion evidence symbols filename does not match actual ZIP")
            if sha256(symbols_path) != raw.get("symbols_sha256"):
                errors.append("promotion evidence symbols bytes do not match aggregate SHA256")
    return errors


def validate_gate_evidence(
    gate_name: str,
    gate: dict[str, object],
    evidence: dict[str, object],
    *,
    root: Path,
    run_id: object,
    commit: object,
    candidate_sha256: object,
    symbols_sha256: object,
    symbols_member_sha256: object,
    source_snapshot_sha256: object = None,
    build_inputs_sha256: object = None,
    candidate_path: Path | None = None,
    symbols_path: Path | None = None,
) -> list[str]:
    errors: list[str] = []
    expected_test = EXPECTED_TEST_NAMES.get(gate_name)
    gate_status = gate.get("Status") or gate.get("status")
    if expected_test is None:
        errors.append("gate has no registered evidence schema")
    if evidence.get("schema") != EVIDENCE_SCHEMA:
        errors.append("evidence schema is invalid")
    if not is_exact_int(evidence.get("schema_version"), EVIDENCE_SCHEMA_VERSION):
        errors.append("evidence schema_version is invalid")
    if evidence.get("test") != expected_test:
        errors.append("evidence test type does not match gate")
    if evidence.get("gate_name") != gate_name:
        errors.append("evidence gate_name does not match aggregate")
    if evidence.get("status") != gate_status:
        errors.append("evidence status does not match aggregate")
    if gate_status == "PASS" and evidence.get("passed") is not True:
        errors.append("PASS gate evidence is not passed=true")
    if gate_status != "PASS" and evidence.get("passed") is not False:
        errors.append("non-PASS gate evidence must be passed=false")
    if gate_status == "PASS" and not is_exact_int(evidence.get("exit_code"), 0):
        errors.append("PASS gate envelope must have exit_code=0")
    if gate_status != "PASS" and evidence.get("exit_code") == 0:
        errors.append("non-PASS gate envelope must not have exit_code=0")
    if evidence.get("run_id") != run_id:
        errors.append("evidence run_id does not match current run")
    if evidence.get("commit") != commit:
        errors.append("evidence commit does not match current source snapshot")
    if gate_name in CANDIDATE_BOUND_GATES:
        if not isinstance(candidate_sha256, str) or not re.fullmatch(r"[0-9A-F]{64}", candidate_sha256):
            errors.append("aggregate candidate_sha256 is absent or invalid")
        elif evidence.get("candidate_sha256") != candidate_sha256:
            errors.append("evidence candidate_sha256 does not match current candidate")
    if gate_name in SYMBOL_BOUND_GATES:
        if not isinstance(symbols_sha256, str) or SHA256_RE.fullmatch(symbols_sha256) is None:
            errors.append("symbol-bound evidence is missing aggregate symbols_sha256")
        elif evidence.get("symbols_sha256") != symbols_sha256:
            errors.append("evidence symbols_sha256 does not match current symbols archive")
        if not isinstance(symbols_member_sha256, str) or SHA256_RE.fullmatch(symbols_member_sha256) is None:
            errors.append("symbol-bound evidence is missing aggregate symbols_member_sha256")
        elif evidence.get("symbols_member_sha256") != symbols_member_sha256:
            errors.append("evidence symbols_member_sha256 does not match current symbol member")
    started = parse_timestamp(evidence.get("gate_started_at"))
    finished = parse_timestamp(evidence.get("gate_finished_at"))
    errors.extend(timestamp_interval_errors(started, finished, label="evidence gate"))

    source_path = confined_file(root, evidence.get("source_evidence"))
    source_hash = evidence.get("source_evidence_sha256")
    if source_path is None:
        errors.append("source evidence is absent or outside current run")
    elif not isinstance(source_hash, str) or sha256(source_path) != source_hash:
        errors.append("source evidence hash mismatch")
    if source_path is not None and source_path.suffix.lower() != ".json":
        errors.append("source evidence must be a JSON evidence document")
    if source_path is not None and source_path.suffix.lower() == ".json":
        try:
            raw = json.loads(source_path.read_text(encoding="utf-8-sig"))
        except (OSError, UnicodeError, json.JSONDecodeError):
            errors.append("source JSON evidence is invalid")
        else:
            if isinstance(raw, dict):
                # The outer envelope hash only proves that the envelope bytes
                # were not changed.  The raw document is an independent
                # semantic assertion and must carry the same identity and
                # result fields.  Requiring every field here prevents a
                # freshly re-hashed, declaration-only JSON from becoming a
                # PASS gate.
                expected_raw = {
                    "schema": EVIDENCE_SCHEMA,
                    "schema_version": EVIDENCE_SCHEMA_VERSION,
                    "test": expected_test,
                    "gate_name": gate_name,
                    "run_id": run_id,
                    "commit": commit,
                    "status": gate_status,
                    "passed": gate_status == "PASS",
                }
                for key, expected in expected_raw.items():
                    if not json_semantically_equal(raw.get(key), expected):
                        errors.append(f"source evidence {key} does not match current gate")
                raw_candidate = raw.get("candidate_sha256")
                if gate_name in CANDIDATE_BOUND_GATES:
                    if raw_candidate != candidate_sha256:
                        errors.append("source evidence candidate identity is absent or stale")
                elif raw_candidate is not None and raw_candidate != candidate_sha256:
                    errors.append("source evidence candidate identity is stale")
                raw_symbols = raw.get("symbols_sha256")
                if gate_name in SYMBOL_BOUND_GATES:
                    if raw_symbols != symbols_sha256:
                        errors.append("source evidence symbols_sha256 is absent or stale")
                    if raw.get("symbols_member_sha256") != symbols_member_sha256:
                        errors.append("source evidence symbols_member_sha256 is absent or stale")
                elif raw_symbols is not None and raw_symbols != symbols_sha256:
                    errors.append("source evidence symbols_sha256 is stale")
                raw_started = parse_timestamp(raw.get("gate_started_at"))
                raw_finished = parse_timestamp(raw.get("gate_finished_at"))
                errors.extend(timestamp_interval_errors(
                    raw_started, raw_finished, label="source evidence",
                ))
                if (
                    started is not None
                    and raw_started is not None
                    and started - raw_started > EVIDENCE_TIMESTAMP_TOLERANCE
                ):
                    errors.append("source evidence starts before its gate envelope")
                if (
                    finished is not None
                    and raw_finished is not None
                    and raw_finished - finished > EVIDENCE_TIMESTAMP_TOLERANCE
                ):
                    errors.append("source evidence finishes after its gate envelope")
                binding = raw.get("identity_binding")
                if binding in TRUSTED_ADAPTER_BINDINGS:
                    producer = confined_file(root, raw.get("producer_evidence"))
                    producer_hash = raw.get("producer_evidence_sha256")
                    if (
                        producer is None
                        or producer == source_path
                        or not isinstance(producer_hash, str)
                        or SHA256_RE.fullmatch(producer_hash) is None
                        or sha256(producer) != producer_hash
                    ):
                        errors.append("trusted driver adapter producer evidence is absent or stale")
                    else:
                        try:
                            producer_raw = json.loads(producer.read_text(encoding="utf-8-sig"))
                        except (OSError, UnicodeError, json.JSONDecodeError):
                            producer_raw = None
                        if not isinstance(producer_raw, dict):
                            errors.append("trusted driver adapter producer JSON is invalid")
                        else:
                            for key, expected in (
                                ("schema", EVIDENCE_SCHEMA),
                                ("schema_version", EVIDENCE_SCHEMA_VERSION),
                                ("test", expected_test),
                                ("status", gate_status),
                                ("passed", gate_status == "PASS"),
                            ):
                                if not json_semantically_equal(producer_raw.get(key), expected):
                                    errors.append(f"trusted driver adapter producer {key} is inconsistent")
                            for key, expected in (
                                ("gate_name", gate_name),
                                ("run_id", run_id),
                                ("commit", commit),
                                ("candidate_sha256", candidate_sha256),
                                ("symbols_sha256", symbols_sha256),
                                ("symbols_member_sha256", symbols_member_sha256),
                            ):
                                if (
                                    key in producer_raw
                                    and producer_raw.get(key) is not None
                                    and producer_raw.get(key) != expected
                                ):
                                    errors.append(
                                        f"trusted driver adapter producer {key} contradicts current identity"
                                    )
                            if any(
                                key in producer_raw
                                for key in (
                                    "identity_binding",
                                    "producer_evidence",
                                    "producer_evidence_sha256",
                                )
                            ):
                                errors.append("trusted driver adapter producer must be original evidence")
                            for key, producer_value in producer_raw.items():
                                if key in TRUSTED_ADAPTER_METADATA_FIELDS:
                                    continue
                                if key not in raw or not json_semantically_equal(
                                    raw[key], producer_value
                                ):
                                    errors.append(
                                        f"trusted driver adapter changed producer-owned field: {key}"
                                    )
                            unexpected_adapter_fields = sorted(
                                set(raw).difference(producer_raw).difference(
                                    TRUSTED_ADAPTER_METADATA_FIELDS
                                )
                            )
                            if unexpected_adapter_fields:
                                errors.append(
                                    "trusted driver adapter introduced producer-owned fields: "
                                    + ", ".join(unexpected_adapter_fields)
                                )
                            if gate_name in META_GATES or gate_status == "PASS":
                                producer_errors = validate_raw_semantics(
                                    gate_name, producer_raw, root=root,
                                    candidate_sha256=candidate_sha256,
                                    commit=commit,
                                    source_snapshot_sha256=source_snapshot_sha256,
                                    build_inputs_sha256=build_inputs_sha256,
                                    candidate_path=candidate_path,
                                    symbols_path=symbols_path,
                                    producer_document=True,
                                )
                                errors.extend(
                                    f"trusted driver adapter producer semantic error: {error}"
                                    for error in producer_errors
                                )
                elif binding is not None or "producer_evidence" in raw or "producer_evidence_sha256" in raw:
                    errors.append("source evidence uses an unrecognized producer adapter")
                if gate_name in META_GATES or gate_status == "PASS":
                    errors.extend(validate_raw_semantics(
                        gate_name, raw, root=root, candidate_sha256=candidate_sha256,
                        commit=commit, source_snapshot_sha256=source_snapshot_sha256,
                        build_inputs_sha256=build_inputs_sha256,
                        candidate_path=candidate_path, symbols_path=symbols_path,
                    ))
            else:
                errors.append("source evidence JSON root is not an object")
    return errors


def gate_raw_document(
    validation: dict[str, object], root: Path, gate_name: str
) -> dict[str, object] | None:
    """Load the raw evidence behind one aggregate gate without trusting it."""
    gates = validation.get("gates")
    record = gates.get(gate_name) if isinstance(gates, dict) else None
    if not isinstance(record, dict):
        return None
    envelope_path = confined_file(root, record.get("Evidence") or record.get("evidence"))
    if envelope_path is None:
        return None
    try:
        envelope = json.loads(envelope_path.read_text(encoding="utf-8-sig"))
    except (OSError, UnicodeError, json.JSONDecodeError):
        return None
    if not isinstance(envelope, dict):
        return None
    raw_path = confined_file(root, envelope.get("source_evidence"))
    if raw_path is None:
        return None
    try:
        raw = json.loads(raw_path.read_text(encoding="utf-8-sig"))
    except (OSError, UnicodeError, json.JSONDecodeError):
        return None
    return raw if isinstance(raw, dict) else None


def validate_official_corpus_binding(
    validation: dict[str, object], root: Path
) -> list[str]:
    """Require compatibility evidence to use exactly the proven corpus bytes."""
    gates = validation.get("gates")
    if not isinstance(gates, dict):
        return ["aggregate gates are unavailable for official corpus binding"]
    compatibility = gates.get("OfficialTPTSaveCompatibility")
    provenance = gates.get("OfficialTPTCorpusProvenance")
    if not isinstance(compatibility, dict) or compatibility.get("Status") != "PASS":
        return []
    if not isinstance(provenance, dict) or provenance.get("Status") != "PASS":
        return ["official compatibility PASS has no PASS provenance gate"]
    provenance_raw = gate_raw_document(validation, root, "OfficialTPTCorpusProvenance")
    compatibility_raw = gate_raw_document(validation, root, "OfficialTPTSaveCompatibility")
    if provenance_raw is None or compatibility_raw is None:
        return ["official provenance or compatibility raw evidence is unavailable"]
    if provenance_raw.get("provenance_schema") == OFFICIAL_PROVENANCE_V2 or compatibility_raw.get("provenance_schema") == OFFICIAL_PROVENANCE_V2:
        if (
            provenance_raw.get("provenance_schema") != OFFICIAL_PROVENANCE_V2
            or compatibility_raw.get("provenance_schema") != OFFICIAL_PROVENANCE_V2
            or provenance_raw.get("source_repositories") != compatibility_raw.get("source_repositories")
        ):
            return ["official compatibility v2 source repositories disagree with provenance"]
    elif (
        provenance_raw.get("repository") != compatibility_raw.get("source_repository")
        or provenance_raw.get("revision") != compatibility_raw.get("source_revision")
    ):
        return ["official compatibility repository or revision disagrees with provenance"]
    provenance_rows = provenance_raw.get("files")
    compatibility_rows = compatibility_raw.get("files")
    if not isinstance(provenance_rows, list) or not isinstance(compatibility_rows, list):
        return ["official corpus binding file inventories are absent"]
    proven = {
        (
            row.get("path"), row.get("source_kind"), row.get("source_repository"),
            row.get("source_revision"), row.get("source_date"), row.get("save_id")
        ): row.get("fixture_sha256")
        for row in provenance_rows
        if isinstance(row, dict)
    }
    exercised = {
        (
            row.get("path"), row.get("source_kind"), row.get("source_repository"),
            row.get("source_revision"), row.get("source_date"), row.get("save_id")
        ): row.get("fixture_sha256")
        for row in compatibility_rows
        if isinstance(row, dict)
    }
    if (
        len(proven) != len(provenance_rows)
        or len(exercised) != len(compatibility_rows)
        or not proven
        or proven != exercised
    ):
        return ["official compatibility files or hashes do not exactly match provenance"]
    return []


def audit_evidence(
    validation: dict[str, object], root: Path, *,
    candidate_path: Path | None = None,
    symbols_path: Path | None = None,
) -> tuple[bool, bool, list[dict[str, object]]]:
    rows: list[dict[str, object]] = []
    hash_passed = True
    semantic_passed = True
    gates = validation.get("gates", {})
    if not isinstance(gates, dict):
        return False, False, [{"error": "gates is not an object"}]
    run_id = validation.get("run_id")
    commit = validation.get("commit")
    source_snapshot_sha256 = validation.get("source_snapshot_sha256")
    build_inputs_sha256 = validation.get("build_inputs_sha256")
    candidate_sha256 = validation.get("candidate_sha256")
    symbols_sha256 = validation.get("symbols_sha256")
    symbols_member_sha256 = validation.get("symbols_member_sha256")
    if not isinstance(run_id, str) or not re.fullmatch(r"[0-9]{8}T[0-9]{6}Z-[0-9a-f]{8}", run_id):
        semantic_passed = False
        rows.append({"gate": "<aggregate>", "hash_passed": True, "semantic_passed": False, "errors": ["aggregate run_id is absent or invalid"]})
    if not isinstance(commit, str) or not re.fullmatch(r"[0-9a-f]{40}", commit):
        semantic_passed = False
        rows.append({"gate": "<aggregate>", "hash_passed": True, "semantic_passed": False, "errors": ["aggregate commit is absent or invalid"]})
    if not isinstance(source_snapshot_sha256, str) or SHA256_RE.fullmatch(source_snapshot_sha256) is None:
        semantic_passed = False
        rows.append({"gate": "<aggregate>", "hash_passed": True, "semantic_passed": False, "errors": ["aggregate source_snapshot_sha256 is absent or invalid"]})
    if not isinstance(build_inputs_sha256, str) or SHA256_RE.fullmatch(build_inputs_sha256) is None:
        semantic_passed = False
        rows.append({"gate": "<aggregate>", "hash_passed": True, "semantic_passed": False, "errors": ["aggregate build_inputs_sha256 is absent or invalid"]})
    if candidate_path is not None:
        if not candidate_path.is_file() or not isinstance(candidate_sha256, str) or sha256(candidate_path) != candidate_sha256:
            semantic_passed = False
            rows.append({"gate": "<artifact>", "hash_passed": False, "semantic_passed": False, "errors": ["candidate path is missing or its bytes do not match aggregate candidate_sha256"]})
    if symbols_path is not None:
        if not symbols_path.is_file() or not isinstance(symbols_sha256, str) or sha256(symbols_path) != symbols_sha256:
            semantic_passed = False
            rows.append({"gate": "<symbols-artifact>", "hash_passed": False, "semantic_passed": False, "errors": ["symbols path is missing or its bytes do not match aggregate symbols_sha256"]})
    for name, value in gates.items():
        if not isinstance(value, dict):
            hash_passed = semantic_passed = False
            rows.append({"gate": name, "hash_passed": False, "semantic_passed": False, "errors": ["gate record is not an object"]})
            continue
        raw = value.get("Evidence") or value.get("evidence")
        expected = value.get("EvidenceSha256") or value.get("evidence_sha256")
        path = confined_file(root, raw)
        row: dict[str, object] = {"gate": name, "evidence": raw, "expected_sha256": expected}
        gate_hash_ok = path is not None and isinstance(expected, str) and sha256(path) == expected
        row["hash_passed"] = gate_hash_ok
        if not gate_hash_ok:
            hash_passed = False
            row["semantic_passed"] = False
            row["errors"] = ["evidence is missing, outside the run, or has a hash mismatch"]
            semantic_passed = False
            rows.append(row)
            continue
        gate_status = value.get("Status") or value.get("status")
        aggregate_exit = value.get("ExitCode") if "ExitCode" in value else value.get("exit_code")
        if gate_status == "PASS" and not is_exact_int(aggregate_exit, 0):
            row["semantic_passed"] = False
            row["errors"] = ["PASS aggregate gate record must have ExitCode=0"]
            semantic_passed = False
            rows.append(row)
            continue
        if gate_status != "PASS" and aggregate_exit == 0:
            row["semantic_passed"] = False
            row["errors"] = ["non-PASS aggregate gate record must not have ExitCode=0"]
            semantic_passed = False
            rows.append(row)
            continue
        try:
            evidence = json.loads(path.read_text(encoding="utf-8-sig"))
        except (OSError, UnicodeError, json.JSONDecodeError):
            errors = ["gate evidence is not valid JSON"]
        else:
            errors = validate_gate_evidence(
                name,
                value,
                evidence if isinstance(evidence, dict) else {},
                root=root,
                run_id=run_id,
                commit=commit,
                candidate_sha256=candidate_sha256,
                symbols_sha256=symbols_sha256,
                symbols_member_sha256=symbols_member_sha256,
                source_snapshot_sha256=source_snapshot_sha256,
                build_inputs_sha256=build_inputs_sha256,
                candidate_path=candidate_path,
                symbols_path=symbols_path,
            )
        row["semantic_passed"] = not errors
        row["errors"] = errors
        if errors:
            semantic_passed = False
        rows.append(row)
    official_binding_errors = validate_official_corpus_binding(validation, root)
    if official_binding_errors:
        semantic_passed = False
        rows.append({
            "gate": "<official-corpus-binding>",
            "hash_passed": True,
            "semantic_passed": False,
            "errors": official_binding_errors,
        })
    return hash_passed, semantic_passed, rows


def archive_text(package: Path) -> str:
    with zipfile.ZipFile(package) as archive:
        chunks = []
        for name in archive.namelist():
            if name.lower().endswith((".md", ".txt", ".json")):
                chunks.append(archive.read(name).decode("utf-8", errors="replace"))
        return "\n".join(chunks)


def audit_docs(
    validation: dict[str, object],
    validation_text: Path | None,
    build_info: Path | None,
    package: Path | None,
    channel: str,
) -> tuple[bool, list[str]]:
    errors: list[str] = []
    gates = validation.get("gates", {})
    if validation.get("schema") != "omnipack-release-validation" or not is_exact_int(validation.get("schema_version"), 1):
        errors.append("validation aggregate schema is invalid")
    if validation.get("channel") != channel:
        errors.append("validation channel disagrees with requested audit channel")
    if validation.get("status") != validation.get("final_status"):
        errors.append("aggregate status and final_status disagree")
    if not re.fullmatch(r"[0-9a-f]{40}", str(validation.get("commit", ""))):
        errors.append("validation commit is invalid")
    ready = validation.get("final_status") == "READY FOR 1.1.0 STABLE"
    if ready:
        blocking = validation.get("blocking_items")
        if not isinstance(blocking, list) or blocking:
            errors.append("READY aggregate must contain an explicit empty blocking_items list")
        for name in sorted(STABLE_MANDATORY_GATES):
            record = gates.get(name) if isinstance(gates, dict) else None
            if not isinstance(record, dict) or record.get("Status") != "PASS":
                errors.append(f"READY aggregate has non-PASS mandatory gate: {name}")
    require_release_documents = channel == "stable" or ready
    for label, path in (
        ("validation text", validation_text),
        ("BUILD-INFO", build_info),
        ("release package", package),
    ):
        if require_release_documents and (path is None or not path.is_file()):
            errors.append(f"{label} is required for stable documentation audit")
        elif path is not None and not path.is_file():
            errors.append(f"{label} path does not exist")
    if validation_text and validation_text.is_file():
        text = validation_text.read_text(encoding="utf-8", errors="replace")
        for name, record in gates.items() if isinstance(gates, dict) else []:
            status = record.get("Status") if isinstance(record, dict) else None
            if status and not re.search(rf"^{re.escape(name)}: {re.escape(status)}\b", text, re.MULTILINE):
                errors.append(f"validation text disagrees with JSON for {name}")
        for marker in (
            str(validation.get("version")), str(validation.get("commit")),
            str(validation.get("run_id")), str(validation.get("candidate_sha256")),
            str(validation.get("source_snapshot_sha256")), str(validation.get("build_inputs_sha256")),
            str(validation.get("symbols_sha256")), str(validation.get("symbols_member_sha256")),
            f"Channel: {channel}", f"FINAL STATUS: {validation.get('final_status')}",
        ):
            if marker not in text:
                errors.append(f"validation text is missing current value: {marker}")
    if build_info and build_info.is_file():
        info = build_info.read_text(encoding="utf-8", errors="replace")
        for marker in (str(validation.get("version")), str(validation.get("commit")), f"Channel: {channel}"):
            if marker not in info:
                errors.append(f"BUILD-INFO is missing current value: {marker}")
        snapshot_marker = f"Source snapshot SHA256: {validation.get('source_snapshot_sha256', '')}"
        if validation.get("source_snapshot_sha256") and snapshot_marker not in info:
            errors.append("BUILD-INFO is missing the current source snapshot digest")
        build_inputs_marker = f"Build inputs SHA256: {validation.get('build_inputs_sha256', '')}"
        if validation.get("build_inputs_sha256") and build_inputs_marker not in info:
            errors.append("BUILD-INFO is missing the current build-input digest")
        for marker in ("SDL_GPU Vulkan", "CPU fallback", "CUDA future optional"):
            if marker not in info:
                errors.append(f"BUILD-INFO is missing backend policy: {marker}")
    if package and package.is_file():
        expected_package_sha = validation.get("candidate_sha256")
        if isinstance(expected_package_sha, str) and SHA256_RE.fullmatch(expected_package_sha):
            if sha256(package) != expected_package_sha:
                errors.append("release package bytes do not match aggregate candidate_sha256")
        text = archive_text(package)
        if channel == "stable":
            forbidden = (
                "1.1.0-rc1", "RELEASE-CANDIDATE", "NOT_TESTED", "NOT TESTED", "SKIPPED",
                "NOT READY FOR 1.1.0 STABLE", "release_ready=false",
            )
            for marker in forbidden:
                if marker in text:
                    errors.append(f"stable package contains stale/conflicting marker: {marker}")
            lowered = text.lower()
            for marker in ("1.1.0", "sdl_gpu vulkan", "cuda", "not implemented"):
                if marker not in lowered:
                    errors.append(f"stable package is missing release policy marker: {marker}")
            with zipfile.ZipFile(package) as archive:
                if any(name.endswith("/RELEASE-CANDIDATE.md") for name in archive.namelist()):
                    errors.append("stable package contains RELEASE-CANDIDATE.md")
                if any(name.endswith(("/RELEASE-VALIDATION.json", "/RELEASE-VALIDATION.txt")) for name in archive.namelist()):
                    errors.append("stable package embeds a pre-promotion validation aggregate")
    return not errors, errors


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--validation-json", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--validation-text", type=Path)
    parser.add_argument("--build-info", type=Path)
    parser.add_argument("--package", type=Path)
    parser.add_argument("--symbols", type=Path)
    parser.add_argument("--channel", choices=("rc", "stable"), required=True)
    args = parser.parse_args()
    try:
        validation = json.loads(args.validation_json.read_text(encoding="utf-8"))
        hash_ok, semantic_ok, evidence_rows = audit_evidence(
            validation, args.validation_json.parent,
            candidate_path=args.package.resolve() if args.package else None,
            symbols_path=args.symbols.resolve() if args.symbols else None,
        )
        docs_ok, doc_errors = audit_docs(validation, args.validation_text, args.build_info, args.package, args.channel)
        passed = hash_ok and semantic_ok and docs_ok
        result = {
            "schema": EVIDENCE_SCHEMA,
            "schema_version": EVIDENCE_SCHEMA_VERSION,
            "test": "release_validation_audit",
            "run_id": validation.get("run_id"),
            "status": "PASS" if passed else "FAIL",
            "passed": passed,
            "evidence_hash_integrity": hash_ok,
            "evidence_semantic_integrity": semantic_ok,
            "documentation_consistency": docs_ok,
            "evidence": evidence_rows,
            "documentation_errors": doc_errors,
        }
        write(args.output, result)
        return 0 if passed else 1
    except (
        OSError, ValueError, OverflowError, json.JSONDecodeError,
        zipfile.BadZipFile,
    ) as exc:
        write(args.output, {
            "schema": EVIDENCE_SCHEMA,
            "schema_version": EVIDENCE_SCHEMA_VERSION,
            "test": "release_validation_audit",
            "status": "FAIL",
            "passed": False,
            "reason": str(exc),
        })
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
