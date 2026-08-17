#!/usr/bin/env python3
"""Import independent runtime evidence and atomically promote one 1.1.0 candidate."""

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
import subprocess
import sys
import tempfile
import uuid
import zipfile
from pathlib import PurePosixPath


TOOLS = Path(__file__).resolve().parent
SCHEMA = "omnipack-release-evidence"
SCHEMA_VERSION = 1
MANDATORY_GATES = (
    "SourceTreeClean", "SourceSnapshotImmutability", "Configure", "Build",
    "UnitTests", "AtmosphereBench", "MassConservation", "OmniSaveRoundtrip",
    "OfficialTPTCorpusProvenance", "OfficialTPTSaveCompatibility", "SDL3Runtime",
    "GPUNumericalValidation", "CPUFallbackValidation", "SDL3GUI",
    "WindowsPortableExtraction", "Soak2Hours", "ReleaseBinaryStripped",
    "DebugSymbolsSeparated", "PackageManifest", "CandidateSHA256",
    "ArtifactImmutability", "WindowsCleanMachine", "EvidenceSemanticIntegrity",
    "EvidenceHashIntegrity", "DocumentationConsistency", "NegativeGateSuite",
    "CandidatePromotion",
)
SUPPLEMENTAL_GATES = ("PackageVerification", "SymbolPackageVerification")
PRE_PROMOTION_GATES = tuple(name for name in MANDATORY_GATES if name != "CandidatePromotion")


def load_module(name: str, path: Path):
    spec = importlib.util.spec_from_file_location(name, path)
    if not spec or not spec.loader:
        raise RuntimeError(f"cannot import {path}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


auditor = load_module("finalizer_release_validation_audit", TOOLS / "release_validation_audit.py")
package_audit = load_module("finalizer_test_release_audit", TOOLS / "test_release_audit.py")


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest().upper()


def read_json(path: Path) -> dict[str, object]:
    value = json.loads(path.read_text(encoding="utf-8-sig"))
    if not isinstance(value, dict):
        raise ValueError(f"JSON root is not an object: {path}")
    return value


def write_json(path: Path, value: object) -> None:
    temporary = path.with_suffix(path.suffix + ".tmp")
    path.parent.mkdir(parents=True, exist_ok=True)
    with temporary.open("w", encoding="utf-8", newline="\n") as stream:
        stream.write(json.dumps(value, ensure_ascii=False, indent=2) + "\n")
        stream.flush()
        os.fsync(stream.fileno())
    os.replace(temporary, path)


def now() -> str:
    return datetime.now(timezone.utc).isoformat()


def status_of(validation: dict[str, object], name: str) -> object:
    gates = validation.get("gates")
    record = gates.get(name) if isinstance(gates, dict) else None
    return record.get("Status") if isinstance(record, dict) else None


def blockers(validation: dict[str, object], names: tuple[str, ...]) -> list[str]:
    return [name for name in names if status_of(validation, name) != "PASS"]


def validate_sidecar(path: Path) -> None:
    sidecar = path.with_suffix(path.suffix + ".sha256")
    expected = f"{sha256(path)}  {path.name}\n"
    if not sidecar.is_file() or sidecar.read_text(encoding="ascii") != expected:
        raise ValueError(f"SHA256 sidecar does not match {path.name}")


def validate_bundle_audit_identity(
    audit: dict[str, object], bundle: Path, *, phase: str
) -> str:
    observed = sha256(bundle)
    if audit.get("passed") is not True or audit.get("bundle_sha256") != observed:
        raise ValueError(f"evidence bundle changed after the {phase} semantic audit")
    return observed


def symbol_member_sha256(path: Path, symbol_stem: str) -> str:
    member = f"{symbol_stem}/tpt-zh-omnipack.debug"
    with zipfile.ZipFile(path) as archive:
        try:
            data = archive.read(member)
        except KeyError as exc:
            raise ValueError("symbol archive is missing tpt-zh-omnipack.debug") from exc
    return hashlib.sha256(data).hexdigest().upper()


def validate_source_manifest(
    package: Path, stem: str, *, commit: str, source_snapshot_sha256: str,
    build_inputs_sha256: str,
) -> None:
    """Bind a packaged MANIFEST to the exact aggregate source snapshot."""
    member = f"{stem}/MANIFEST.txt"
    try:
        with zipfile.ZipFile(package) as archive:
            fields, _ = package_audit.parse_manifest(archive.read(member))
    except KeyError as exc:
        raise ValueError(f"package is missing source manifest: {member}") from exc
    expected = {
        "revision": commit,
        "source_state": "clean",
        "source_worktree_sha256": source_snapshot_sha256,
        "source_untracked_files": "0",
        "build_inputs_sha256": build_inputs_sha256,
        "build_inputs_ready": "true",
    }
    mismatches = [
        f"{key}={fields.get(key)!r}, expected {value!r}"
        for key, value in expected.items()
        if fields.get(key) != value
    ]
    if mismatches:
        raise ValueError("package source manifest is not bound to the aggregate: " + "; ".join(mismatches))


def git_stdout(repository: Path, *arguments: str) -> str:
    completed = subprocess.run(
        ["git", "-C", str(repository), *arguments],
        check=False,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        encoding="utf-8",
        errors="replace",
    )
    if completed.returncode != 0:
        detail = completed.stderr.strip() or completed.stdout.strip() or f"exit {completed.returncode}"
        raise ValueError(f"cannot inspect trusted finalizer checkout: {detail}")
    return completed.stdout


def validate_checkout_identity(commit: str, repository: Path | None = None) -> None:
    checkout = (repository or TOOLS.parent).resolve()
    head = git_stdout(checkout, "rev-parse", "--verify", "HEAD").strip().lower()
    if head != commit:
        raise ValueError(
            f"trusted finalizer checkout HEAD {head!r} does not match aggregate commit {commit!r}"
        )
    status = git_stdout(
        checkout, "status", "--porcelain=v1", "--untracked-files=all",
    )
    if status:
        raise ValueError("trusted finalizer checkout is not clean")


def validate_runtime_validator_binding(
    binding: dict[str, object], *, binding_path: Path, downloaded_validator: Path,
    trusted_validator: Path, clean_evidence_path: Path, clean: dict[str, object],
    run_id: str, commit: str, candidate_filename: str, candidate_sha256: str,
) -> str:
    if not downloaded_validator.is_file() or downloaded_validator.name != "test_clean_release.ps1":
        raise ValueError("downloaded runtime validator is absent or misnamed")
    if not trusted_validator.is_file() or trusted_validator.name != "test_clean_release.ps1":
        raise ValueError("trusted checkout runtime validator is absent or misnamed")
    if not clean_evidence_path.is_file() or clean_evidence_path.name != "windows-clean-machine.json":
        raise ValueError("clean-machine evidence is absent or misnamed")
    artifact_root = binding_path.parent.resolve()
    if clean_evidence_path.resolve().parent != artifact_root:
        raise ValueError("clean evidence and validator binding are not from one runtime artifact")
    try:
        downloaded_validator.resolve().relative_to(artifact_root)
    except ValueError as exc:
        raise ValueError("downloaded validator is outside the runtime artifact") from exc
    if downloaded_validator.resolve() == trusted_validator.resolve():
        raise ValueError("downloaded and trusted validators must be independently located")
    downloaded_sha = sha256(downloaded_validator)
    trusted_sha = sha256(trusted_validator)
    exact = {
        "schema": SCHEMA,
        "schema_version": SCHEMA_VERSION,
        "test": "runtime_validator_binding",
        "run_id": run_id,
        "commit": commit,
        "status": "PASS",
        "passed": True,
        "candidate_filename": candidate_filename,
        "candidate_sha256": candidate_sha256,
        "runtime_validator_filename": "test_clean_release.ps1",
        "runtime_validator_sha256": trusted_sha,
        "clean_machine_evidence_filename": "windows-clean-machine.json",
        "clean_machine_evidence_sha256": sha256(clean_evidence_path),
    }
    errors = [
        f"{key}={binding.get(key)!r}, expected {expected!r}"
        for key, expected in exact.items()
        if not auditor.json_semantically_equal(binding.get(key), expected)
    ]
    if not binding.get("created_at"):
        errors.append("created_at is absent")
    if downloaded_sha != trusted_sha:
        errors.append("downloaded validator SHA256 differs from trusted checkout validator")
    if clean.get("runtime_validator") != "test_clean_release.ps1":
        errors.append("clean evidence runtime_validator identity is invalid")
    if clean.get("runtime_validator_sha256") != trusted_sha:
        errors.append("clean evidence runtime_validator_sha256 differs from trusted validator")
    if clean.get("validator_binding_checked") is not True:
        errors.append("clean evidence did not confirm validator binding")
    if errors:
        raise ValueError("runtime validator binding is invalid: " + "; ".join(errors))
    if binding_path.name != "runtime-validator-binding.json":
        raise ValueError("runtime validator binding evidence is misnamed")
    return trusted_sha


def validate_clean_evidence(
    value: dict[str, object], *, root: Path, run_id: str, candidate_sha256: str,
    runtime_validator_sha256: str,
) -> None:
    exact = {
        "schema": SCHEMA,
        "schema_version": SCHEMA_VERSION,
        "test": "windows_clean_machine",
        "run_id": run_id,
        "status": "PASS",
        "passed": True,
        "candidate_sha256": candidate_sha256,
        "candidate_filename": f"TPT-ZH-OmniPack-1.1.0-staging-{run_id}-Windows-x64-SDL3.zip",
        "source_checkout_used": False,
        "source_tree_indicators": [],
        "source_markers_checked": True,
        "development_tool_probe_completed": True,
        "runtime_validator": "test_clean_release.ps1",
        "runtime_validator_sha256": runtime_validator_sha256,
        "validator_binding_checked": True,
        "runtime_job_kind": "runtime-only",
        "project_build_tool_dependency_used": False,
        "candidate_extracted_to_fresh_directory": True,
        "candidate_source_markers_checked": True,
        "candidate_source_tree_indicators": [],
        "runtime_target_executable_count": 1,
        "sanitized_path_used": True,
        "runtime_payload_schema_version": auditor.PORTABLE_RUNTIME_PAYLOAD_SCHEMA_VERSION,
        "clean_shutdown": True,
    }
    exact.update({field: True for field in auditor.PORTABLE_RUNTIME_TRUE_FIELDS})
    errors = [
        f"{key}={value.get(key)!r}, expected {expected!r}"
        for key, expected in exact.items()
        if not auditor.json_semantically_equal(value.get(key), expected)
    ]
    if not value.get("gate_started_at") or not value.get("gate_finished_at"):
        errors.append("gate timestamps are absent")
    tools = value.get("development_tools_detected")
    expected_tools = {"gcc.exe", "g++.exe", "meson.exe", "ninja.exe", "glslc.exe", "bash.exe"}
    if not isinstance(tools, dict) or set(tools) != expected_tools:
        errors.append("clean-machine development-tool probe inventory is incomplete")
    errors.extend(auditor.validate_portable_runtime_contract(root, value, candidate_sha256))
    if errors:
        raise ValueError("clean-machine evidence is not a semantic PASS: " + "; ".join(errors))


def import_clean_evidence(source: Path, root: Path) -> tuple[dict[str, object], Path]:
    value = read_json(source)
    source_root = source.parent.resolve()
    for field, target_name in (
        ("runtime_stdout", "windows_clean_machine.stdout.txt"),
        ("runtime_stderr", "windows_clean_machine.stderr.txt"),
        ("runtime_inner_evidence", "windows_clean_machine.inner.json"),
    ):
        raw = value.get(field)
        if raw != target_name:
            raise ValueError(
                f"clean-machine evidence {field} is not the expected runtime artifact name"
            )
        artifact = (source_root / target_name).resolve()
        try:
            artifact.relative_to(source_root)
        except ValueError as exc:
            raise ValueError(f"clean-machine {field} escapes its artifact directory") from exc
        expected = value.get(field + "_sha256")
        if not artifact.is_file() or not isinstance(expected, str) or sha256(artifact) != expected:
            raise ValueError(f"clean-machine {field} is missing or stale")
        destination = root / target_name
        if destination.resolve() != artifact:
            destination.parent.mkdir(parents=True, exist_ok=True)
            copy_verified(artifact, destination, expected)
    destination_json = root / "windows-clean-machine.json"
    source_sha256 = sha256(source)
    if destination_json.resolve() != source.resolve():
        copy_verified(source, destination_json, source_sha256)
    if sha256(destination_json) != source_sha256:
        raise ValueError("clean-machine producer bytes changed during import")
    return read_json(destination_json), destination_json


def set_gate(
    validation: dict[str, object], root: Path, name: str, test: str,
    status: str, raw_path: Path, message: str, *, candidate_sha256: str | None,
    started_at: object = None, finished_at: object = None,
    adapter_metadata: dict[str, object] | None = None,
) -> None:
    run_id = validation["run_id"]
    commit = validation["commit"]
    producer_relative = raw_path.resolve().relative_to(root.resolve()).as_posix()
    raw = read_json(raw_path)
    expected_test = auditor.EXPECTED_TEST_NAMES.get(name, test)
    for key, expected in (
        ("schema", SCHEMA), ("schema_version", SCHEMA_VERSION),
        ("test", expected_test), ("status", status), ("passed", status == "PASS"),
    ):
        if not auditor.json_semantically_equal(raw.get(key), expected):
            raise ValueError(f"raw evidence for {name} has inconsistent {key}")
    identity = {
        "gate_name": name,
        "run_id": run_id,
        "commit": commit,
        "candidate_sha256": candidate_sha256,
        "symbols_sha256": validation.get("symbols_sha256"),
        "symbols_member_sha256": validation.get("symbols_member_sha256"),
    }
    for key, expected in identity.items():
        if key in raw and raw.get(key) is not None and raw.get(key) != expected:
            raise ValueError(f"raw evidence for {name} has stale {key}")
    source_time = datetime.fromtimestamp(raw_path.stat().st_mtime, tz=timezone.utc).isoformat()
    bound = dict(raw)
    bound.update(identity)
    bound["gate_started_at"] = started_at or raw.get("gate_started_at") or source_time
    bound["gate_finished_at"] = finished_at or raw.get("gate_finished_at") or now()
    bound["identity_binding"] = "trusted_finalizer_import_adapter"
    bound["producer_evidence"] = producer_relative
    bound["producer_evidence_sha256"] = sha256(raw_path)
    for key, value in (adapter_metadata or {}).items():
        if key in raw and raw.get(key) != value:
            raise ValueError(f"raw evidence for {name} contradicts adapter metadata {key}")
        bound[key] = value
    bound_path = root / f"bound-{name.lower()}.json"
    write_json(bound_path, bound)
    raw_relative = bound_path.relative_to(root).as_posix()
    envelope = {
        "schema": SCHEMA, "schema_version": SCHEMA_VERSION, "test": test,
        "gate_name": name, "run_id": run_id, "commit": commit,
        "status": status, "passed": status == "PASS", "exit_code": 0 if status == "PASS" else 1,
        "candidate_sha256": candidate_sha256, "symbols_sha256": validation.get("symbols_sha256"),
        "symbols_member_sha256": validation.get("symbols_member_sha256"),
        "gate_started_at": bound["gate_started_at"], "gate_finished_at": bound["gate_finished_at"],
        "source_evidence": raw_relative, "source_evidence_sha256": sha256(bound_path),
        "message": message,
    }
    envelope_path = root / f"gate-{name.lower()}.json"
    write_json(envelope_path, envelope)
    gates = validation.setdefault("gates", {})
    if not isinstance(gates, dict):
        raise ValueError("aggregate gates is not an object")
    gates[name] = {
        "Name": name, "Status": status, "Command": "finalize_release_1_1_0.py",
        "ExitCode": 0 if status == "PASS" else 1,
        "Evidence": envelope_path.relative_to(root).as_posix(),
        "EvidenceSha256": sha256(envelope_path), "Message": message,
    }


def write_validation_text(validation: dict[str, object], path: Path) -> None:
    lines = [
        f"TPT-ZH OmniPack {validation.get('version')} Release Validation", "",
        f"Commit: {validation.get('commit')}", f"Channel: {validation.get('channel')}",
        f"Run ID: {validation.get('run_id')}", f"Candidate SHA256: {validation.get('candidate_sha256')}",
        f"Source snapshot SHA256: {validation.get('source_snapshot_sha256')}",
        f"Build inputs SHA256: {validation.get('build_inputs_sha256')}",
        f"Symbols SHA256: {validation.get('symbols_sha256')}",
        f"Symbols member SHA256: {validation.get('symbols_member_sha256')}", "",
    ]
    gates = validation.get("gates", {})
    if isinstance(gates, dict):
        for name, record in gates.items():
            if isinstance(record, dict):
                lines.append(f"{name}: {record.get('Status')} | exit={record.get('ExitCode')} | evidence={record.get('Evidence')}")
    lines.extend(["", f"FINAL STATUS: {validation.get('final_status')}"])
    temporary = path.with_suffix(path.suffix + ".tmp")
    with temporary.open("w", encoding="utf-8", newline="\n") as stream:
        stream.write("\n".join(lines) + "\n")
        stream.flush()
        os.fsync(stream.fileno())
    os.replace(temporary, path)


def run_audit(
    validation: dict[str, object], root: Path, validation_json: Path,
    validation_text: Path, build_info: Path, package: Path,
    symbols_package: Path | None = None,
    *,
    candidate_artifact: Path | None = None,
    symbols_artifact: Path | None = None,
) -> None:
    write_json(validation_json, validation)
    write_validation_text(validation, validation_text)
    hash_ok, semantic_ok, rows = auditor.audit_evidence(
        validation, root,
        candidate_path=candidate_artifact or package,
        symbols_path=symbols_artifact or symbols_package,
    )
    docs_ok, doc_errors = auditor.audit_docs(validation, validation_text, build_info, package, "stable")
    passed = hash_ok and semantic_ok and docs_ok
    raw = root / "evidence-integrity.json"
    write_json(raw, {
        "schema": SCHEMA, "schema_version": SCHEMA_VERSION,
        "test": "release_validation_audit", "run_id": validation.get("run_id"),
        "commit": validation.get("commit"), "candidate_sha256": validation.get("candidate_sha256"),
        "symbols_sha256": validation.get("symbols_sha256"),
        "status": "PASS" if passed else "FAIL", "passed": passed,
        "gate_name": "EvidenceSemanticIntegrity",
        "gate_started_at": now(), "gate_finished_at": now(),
        "evidence_hash_integrity": hash_ok, "evidence_semantic_integrity": semantic_ok,
        "documentation_consistency": docs_ok, "evidence": rows,
        "documentation_errors": doc_errors,
    })
    for name, ok, message in (
        ("EvidenceHashIntegrity", hash_ok, "all current-run evidence hashes match"),
        ("EvidenceSemanticIntegrity", semantic_ok, "gate and evidence semantics match"),
        ("DocumentationConsistency", docs_ok, "generated validation documents are consistent"),
    ):
        per_gate = root / (name.replace("Evidence", "evidence-").replace("Documentation", "documentation-").lower() + ".json")
        per_gate_raw = dict(read_json(raw))
        per_gate_raw["gate_name"] = name
        per_gate_raw["status"] = "PASS" if ok else "FAIL"
        per_gate_raw["passed"] = bool(ok)
        per_gate_raw["gate_started_at"] = per_gate_raw.get("gate_started_at") or now()
        per_gate_raw["gate_finished_at"] = now()
        write_json(per_gate, per_gate_raw)
        set_gate(validation, root, name, "release_validation_audit", "PASS" if ok else "FAIL", per_gate, message, candidate_sha256=validation.get("candidate_sha256"))
    write_json(validation_json, validation)
    write_validation_text(validation, validation_text)
    if not passed:
        raise ValueError("final evidence audit failed: " + "; ".join(doc_errors or ["hash or semantic mismatch"]))


def run_negative_suite(
    validation: dict[str, object], root: Path, candidate: Path, symbols: Path,
    artifact_stem: str, symbol_artifact_stem: str,
) -> None:
    raw = root / "negative-gate-tests.json"
    started = now()
    completed = subprocess.run([
        sys.executable, str(TOOLS / "release_negative_gate_suite.py"),
        "--candidate", str(candidate), "--symbols", str(symbols),
        "--artifact-stem", artifact_stem, "--symbol-artifact-stem", symbol_artifact_stem,
        "--run-id", str(validation["run_id"]), "--commit", str(validation["commit"]),
        "--candidate-sha256", str(validation["candidate_sha256"]),
        "--package-version", "1.1.0", "--package-kind", "release",
        "--output", str(raw),
    ], check=False, capture_output=True, text=True)
    result = read_json(raw) if raw.is_file() else {}
    passed = completed.returncode == 0 and result.get("status") == "PASS" and result.get("passed") is True and result.get("attacks_total", 0) == result.get("attacks_rejected", -1)
    set_gate(validation, root, "NegativeGateSuite", "negative_gate_suite", "PASS" if passed else "FAIL", raw, "all bypass attempts were rejected" if passed else "one or more bypass attempts were not rejected", candidate_sha256=str(validation["candidate_sha256"]), started_at=started)
    if not passed:
        raise ValueError("negative gate suite failed: " + completed.stderr.strip())


def reset_candidate_promotion_gate(
    validation: dict[str, object], root: Path, *, candidate_sha256: str
) -> None:
    """Clear retry-era promotion evidence before the pre-promotion audit.

    A failed post-publish attempt is rolled back, but its raw gate documents
    can remain in the run directory.  Every retry must start with an explicit
    NOT_TESTED CandidatePromotion record so stale PASS bytes cannot satisfy a
    later aggregate or get copied into the final evidence bundle.
    """
    raw = root / "candidate-promotion.json"
    cleanup_paths = (
        raw,
        root / "bound-candidatepromotion.json",
        root / "gate-candidatepromotion.json",
    )
    for path in cleanup_paths:
        path.unlink(missing_ok=True)
        path.with_suffix(path.suffix + ".sha256").unlink(missing_ok=True)
    started = now()
    finished = now()
    write_json(raw, {
        "schema": SCHEMA, "schema_version": SCHEMA_VERSION,
        "test": "candidate_promotion", "gate_name": "CandidatePromotion",
        "run_id": validation["run_id"], "commit": validation["commit"],
        "status": "NOT_TESTED", "passed": False,
        "candidate_sha256": candidate_sha256,
        "symbols_sha256": validation.get("symbols_sha256"),
        "symbols_member_sha256": validation.get("symbols_member_sha256"),
        "gate_started_at": started, "gate_finished_at": finished,
        "reason": "stable promotion has not started",
    })
    set_gate(
        validation, root, "CandidatePromotion", "candidate_promotion",
        "NOT_TESTED", raw, "stable promotion has not started",
        candidate_sha256=candidate_sha256, started_at=started,
        finished_at=finished,
    )


def refresh_artifact_immutability(
    validation: dict[str, object], root: Path, candidate: Path,
) -> None:
    expected = str(validation["candidate_sha256"])
    observed = sha256(candidate)
    passed = observed == expected
    raw = root / "artifact-immutability.json"
    write_json(raw, {
        "schema": SCHEMA, "schema_version": SCHEMA_VERSION,
        "test": "artifact_immutability", "run_id": validation["run_id"],
        "commit": validation["commit"], "candidate_sha256": expected,
        "status": "PASS" if passed else "FAIL", "passed": passed,
        "before_sha256": expected, "after_sha256": observed,
        "scope": "after independent clean runtime and negative-gate validation",
    })
    set_gate(
        validation, root, "ArtifactImmutability", "artifact_immutability",
        "PASS" if passed else "FAIL", raw,
        "candidate bytes remained unchanged through all pre-promotion consumers",
        candidate_sha256=expected,
    )
    if not passed:
        raise ValueError("candidate changed after independent runtime validation")


def write_sidecar(path: Path) -> Path:
    sidecar = path.with_suffix(path.suffix + ".sha256")
    temporary = sidecar.with_suffix(sidecar.suffix + ".tmp")
    with temporary.open("w", encoding="ascii", newline="\n") as stream:
        stream.write(f"{sha256(path)}  {path.name}\n")
        stream.flush()
        os.fsync(stream.fileno())
    os.replace(temporary, sidecar)
    return sidecar


def create_evidence_bundle(root: Path, output: Path) -> None:
    temporary = output.with_suffix(output.suffix + ".tmp")
    with zipfile.ZipFile(temporary, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
        for path in sorted(root.rglob("*")):
            if path.is_file() and path.resolve() not in {output.resolve(), temporary.resolve()} and not path.name.endswith(".tmp"):
                archive.write(path, path.relative_to(root).as_posix())
    with temporary.open("rb+") as stream:
        os.fsync(stream.fileno())
    os.replace(temporary, output)


def validate_bundle_documents(bundle: Path, documents: tuple[Path, ...]) -> None:
    with zipfile.ZipFile(bundle) as archive:
        names = archive.namelist()
        for document in documents:
            if names.count(document.name) != 1:
                raise ValueError(
                    f"evidence bundle does not contain exactly one {document.name}"
                )
            if archive.read(document.name) != document.read_bytes():
                raise ValueError(
                    f"published {document.name} bytes disagree with the frozen evidence bundle"
                )


def validate_published_document_set(
    *,
    bundle: Path,
    documents: tuple[Path, Path, Path],
    marker: dict[str, object],
) -> None:
    fields = (
        ("validation_filename", "validation_sha256", documents[0]),
        ("validation_text_filename", "validation_text_sha256", documents[1]),
        ("build_info_filename", "build_info_sha256", documents[2]),
    )
    for filename_field, hash_field, document in fields:
        validate_sidecar(document)
        if marker.get(filename_field) != document.name or marker.get(hash_field) != sha256(document):
            raise ValueError(
                f"published promotion marker does not bind {document.name}"
            )
    validate_bundle_documents(bundle, documents)


def audit_frozen_evidence_bundle(
    bundle: Path,
    *,
    package: Path,
    symbols_package: Path,
    candidate_artifact: Path,
    symbols_artifact: Path,
    run_id: str,
    commit: str,
    candidate_sha256: str,
    symbols_sha256: str,
    symbols_member_sha256: str,
) -> dict[str, object]:
    """Re-audit the exact bytes of the evidence ZIP before publication.

    The directory used to create the ZIP has already passed an audit.  This
    second pass deliberately treats the ZIP as the source of truth: it
    extracts it into a fresh directory, verifies safe/unique members, then
    runs the same hash, semantic, and documentation auditors against the
    extracted snapshot.  A ZIP CRC check alone is not sufficient evidence
    that the frozen bundle still represents the audited release run.
    """
    if not bundle.is_file():
        raise ValueError("frozen evidence bundle is missing")
    audited_bundle_sha256 = sha256(bundle)
    if not package.is_file() or sha256(package) != candidate_sha256:
        raise ValueError("prepared stable package bytes do not match the candidate identity")
    if not symbols_package.is_file() or sha256(symbols_package) != symbols_sha256:
        raise ValueError("prepared stable symbols bytes do not match the symbols identity")
    with tempfile.TemporaryDirectory(prefix="omnipack-evidence-audit-") as temporary:
        extract_root = Path(temporary)
        with zipfile.ZipFile(bundle) as archive:
            if archive.testzip() is not None:
                raise ValueError("frozen evidence bundle is corrupt")
            names = archive.namelist()
            seen: set[str] = set()
            seen_casefold: set[str] = set()
            for info in archive.infolist():
                name = info.filename
                if not name or name in seen:
                    raise ValueError("frozen evidence bundle has duplicate or empty member names")
                seen.add(name)
                folded = name.casefold()
                if folded in seen_casefold:
                    raise ValueError("frozen evidence bundle has a case-insensitive member collision")
                seen_casefold.add(folded)
                relative = PurePosixPath(name)
                if (
                    relative.is_absolute()
                    or "\\" in name
                    or ".." in relative.parts
                    or (relative.parts and ":" in relative.parts[0])
                ):
                    raise ValueError("frozen evidence bundle contains an unsafe member path")
                # Symlink entries are not expected in a self-contained evidence
                # archive and would make extraction semantics platform-dependent.
                mode = (info.external_attr >> 16) & 0o170000
                if mode == 0o120000:
                    raise ValueError("frozen evidence bundle contains a symlink member")
            required = {"RELEASE-VALIDATION.json", "RELEASE-VALIDATION.txt", "BUILD-INFO.txt", "run-metadata.json"}
            missing = sorted(required.difference(names))
            if missing:
                raise ValueError("frozen evidence bundle is missing: " + ", ".join(missing))
            archive.extractall(extract_root)

        validation_path = extract_root / "RELEASE-VALIDATION.json"
        validation_text = extract_root / "RELEASE-VALIDATION.txt"
        build_info = extract_root / "BUILD-INFO.txt"
        metadata_path = extract_root / "run-metadata.json"
        validation = read_json(validation_path)
        metadata = read_json(metadata_path)
        if (
            validation.get("run_id") != run_id
            or validation.get("commit") != commit
            or validation.get("candidate_sha256") != candidate_sha256
            or validation.get("symbols_sha256") != symbols_sha256
            or validation.get("symbols_member_sha256") != symbols_member_sha256
            or validation.get("status") != "READY FOR 1.1.0 STABLE"
            or validation.get("final_status") != "READY FOR 1.1.0 STABLE"
            or validation.get("blocking_items") != []
        ):
            raise ValueError("frozen validation aggregate identity or READY state is inconsistent")
        if (
            metadata.get("run_id") != run_id
            or metadata.get("commit") != commit
            or metadata.get("candidate_sha256") != candidate_sha256
            or metadata.get("symbols_sha256") != symbols_sha256
            or metadata.get("symbols_member_sha256") != symbols_member_sha256
            or metadata.get("final_status") != "READY FOR 1.1.0 STABLE"
        ):
            raise ValueError("frozen run metadata identity is inconsistent")
        hash_ok, semantic_ok, rows = auditor.audit_evidence(
            validation, extract_root,
            candidate_path=candidate_artifact,
            symbols_path=symbols_artifact,
        )
        docs_ok, doc_errors = auditor.audit_docs(
            validation, validation_text, build_info, package, "stable"
        )
        if not hash_ok or not semantic_ok or not docs_ok:
            details = doc_errors or [
                row for row in rows if not row.get("hash_passed") or not row.get("semantic_passed")
            ]
            raise ValueError("frozen evidence bundle audit failed: " + json.dumps(details, ensure_ascii=False))
        if sha256(bundle) != audited_bundle_sha256:
            raise ValueError("frozen evidence bundle changed during semantic audit")
        return {
            "status": "PASS",
            "passed": True,
            "bundle_sha256": audited_bundle_sha256,
            "members_total": len(names),
            "hash_integrity": True,
            "semantic_integrity": True,
            "documentation_consistency": True,
        }


def copy_verified(source: Path, destination: Path, expected_sha256: str) -> None:
    temporary = destination.with_suffix(destination.suffix + ".tmp")
    shutil.copyfile(source, temporary)
    if sha256(temporary) != expected_sha256:
        temporary.unlink(missing_ok=True)
        raise ValueError(f"copied bytes do not match expected identity: {source.name}")
    with temporary.open("rb+") as stream:
        os.fsync(stream.fileno())
    os.replace(temporary, destination)


def acquire_output_lock(path: Path, *, run_id: str, commit: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    descriptor = os.open(path, os.O_WRONLY | os.O_CREAT | os.O_EXCL)
    try:
        payload = json.dumps({
            "schema": SCHEMA,
            "schema_version": SCHEMA_VERSION,
            "test": "release_promotion_lock",
            "run_id": run_id,
            "commit": commit,
            "pid": os.getpid(),
            "created_at": now(),
        }, ensure_ascii=False, indent=2).encode("utf-8") + b"\n"
        os.write(descriptor, payload)
        os.fsync(descriptor)
    finally:
        os.close(descriptor)


def remove_owned_lock(path: Path, run_id: str) -> None:
    try:
        value = read_json(path)
    except (OSError, ValueError, json.JSONDecodeError):
        return
    if value.get("run_id") == run_id and value.get("pid") == os.getpid():
        path.unlink(missing_ok=True)


def remove_transaction(path: Path, output: Path) -> None:
    if not path.exists():
        return
    parent = output.parent.resolve()
    resolved = path.resolve()
    if resolved.parent != parent or not path.name.startswith(f".{output.name}.promotion-"):
        raise ValueError(f"refusing to remove unexpected promotion path: {path}")
    shutil.rmtree(path)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--validation-json", type=Path, required=True)
    parser.add_argument("--candidate", type=Path, required=True)
    parser.add_argument("--symbols", type=Path, required=True)
    parser.add_argument("--clean-machine-evidence", type=Path, required=True)
    parser.add_argument("--runtime-validator-binding", type=Path, required=True)
    parser.add_argument("--runtime-validator", type=Path, required=True)
    parser.add_argument("--output-directory", type=Path, required=True)
    args = parser.parse_args()

    validation_json = args.validation_json.resolve()
    root = validation_json.parent
    validation_text = root / "RELEASE-VALIDATION.txt"
    build_info = root / "BUILD-INFO.txt"
    candidate = args.candidate.resolve()
    symbols = args.symbols.resolve()
    output = args.output_directory.resolve()
    validation: dict[str, object] = {}
    published = False
    stable: Path | None = None
    stable_symbols: Path | None = None
    evidence_bundle: Path | None = None
    transaction: Path | None = None
    transaction_owned = False
    lock_path: Path | None = None
    lock_acquired = False
    rewrite_failure_aggregate = False
    try:
        validation = read_json(validation_json)
        if (
            validation.get("schema") != "omnipack-release-validation"
            or not auditor.is_exact_int(validation.get("schema_version"), 1)
        ):
            raise ValueError("aggregate schema is invalid")
        if validation.get("version") != "1.1.0" or validation.get("channel") != "stable":
            raise ValueError("finalizer accepts only a 1.1.0 stable staging run")
        run_id = validation.get("run_id")
        commit = validation.get("commit")
        expected_sha = validation.get("candidate_sha256")
        expected_symbols_sha = validation.get("symbols_sha256")
        expected_symbols_member_sha = validation.get("symbols_member_sha256")
        expected_source_snapshot = validation.get("source_snapshot_sha256")
        expected_build_inputs = validation.get("build_inputs_sha256")
        if not isinstance(run_id, str) or not re.fullmatch(r"[0-9]{8}T[0-9]{6}Z-[0-9a-f]{8}", run_id):
            raise ValueError("run_id is invalid")
        if not isinstance(commit, str) or not re.fullmatch(r"[0-9a-f]{40}", commit):
            raise ValueError("commit is invalid")
        if not isinstance(expected_sha, str) or not re.fullmatch(r"[0-9A-F]{64}", expected_sha):
            raise ValueError("candidate SHA256 is invalid")
        if not isinstance(expected_symbols_sha, str) or not re.fullmatch(r"[0-9A-F]{64}", expected_symbols_sha):
            raise ValueError("symbols SHA256 is invalid")
        if not isinstance(expected_symbols_member_sha, str) or not re.fullmatch(r"[0-9A-F]{64}", expected_symbols_member_sha):
            raise ValueError("symbols member SHA256 is invalid")
        if not isinstance(expected_source_snapshot, str) or not re.fullmatch(r"[0-9A-F]{64}", expected_source_snapshot):
            raise ValueError("source snapshot SHA256 is invalid")
        if not isinstance(expected_build_inputs, str) or not re.fullmatch(r"[0-9A-F]{64}", expected_build_inputs):
            raise ValueError("build inputs SHA256 is invalid")
        artifact_stem = f"TPT-ZH-OmniPack-1.1.0-staging-{run_id}-Windows-x64-SDL3"
        symbol_stem = f"TPT-ZH-OmniPack-1.1.0-staging-{run_id}-Windows-x64-Symbols"
        if candidate.name != artifact_stem + ".zip" or symbols.name != symbol_stem + ".zip":
            raise ValueError("candidate filenames are not bound to this run_id")
        # From this point the input is unambiguously this finalizer's stable
        # staging aggregate.  Earlier schema/channel/name rejections must leave
        # foreign or RC evidence byte-for-byte untouched.
        rewrite_failure_aggregate = True
        if sha256(candidate) != expected_sha:
            raise ValueError("candidate bytes do not match aggregate candidate_sha256")
        if sha256(symbols) != expected_symbols_sha:
            raise ValueError("symbols bytes do not match aggregate symbols_sha256")
        validate_sidecar(candidate)
        validate_sidecar(symbols)
        if symbol_member_sha256(symbols, symbol_stem) != expected_symbols_member_sha:
            raise ValueError("symbols member bytes do not match aggregate symbols_member_sha256")
        candidate_errors = package_audit.audit_package(
            candidate, False, version="1.1.0", kind="release",
            artifact_stem=artifact_stem, symbol_artifact_stem=symbol_stem,
        )
        if candidate_errors:
            raise ValueError("candidate package manifest audit failed: " + "; ".join(candidate_errors))
        symbol_errors = package_audit.audit_package(
            symbols, True, version="1.1.0", artifact_stem=artifact_stem,
            symbol_artifact_stem=symbol_stem,
        )
        if symbol_errors:
            raise ValueError("symbol package manifest audit failed: " + "; ".join(symbol_errors))
        validate_source_manifest(
            candidate, artifact_stem, commit=commit,
            source_snapshot_sha256=expected_source_snapshot,
            build_inputs_sha256=expected_build_inputs,
        )
        validate_source_manifest(
            symbols, symbol_stem, commit=commit,
            source_snapshot_sha256=expected_source_snapshot,
            build_inputs_sha256=expected_build_inputs,
        )
        validate_checkout_identity(commit)

        clean_source = args.clean_machine_evidence.resolve()
        binding_path = args.runtime_validator_binding.resolve()
        downloaded_validator = args.runtime_validator.resolve()
        clean = read_json(clean_source)
        binding = read_json(binding_path)
        trusted_validator_sha = validate_runtime_validator_binding(
            binding,
            binding_path=binding_path,
            downloaded_validator=downloaded_validator,
            trusted_validator=TOOLS / "test_clean_release.ps1",
            clean_evidence_path=clean_source,
            clean=clean,
            run_id=run_id,
            commit=commit,
            candidate_filename=candidate.name,
            candidate_sha256=expected_sha,
        )
        clean, clean_raw = import_clean_evidence(clean_source, root)
        binding_snapshot = root / "runtime-validator-binding.json"
        validator_snapshot = root / "runtime-validator" / "test_clean_release.ps1"
        if binding_snapshot.resolve() != binding_path:
            copy_verified(binding_path, binding_snapshot, sha256(binding_path))
        if validator_snapshot.resolve() != downloaded_validator:
            validator_snapshot.parent.mkdir(parents=True, exist_ok=True)
            copy_verified(
                downloaded_validator, validator_snapshot, sha256(downloaded_validator),
            )
        if read_json(binding_snapshot) != binding or sha256(validator_snapshot) != trusted_validator_sha:
            raise ValueError("runtime validator evidence snapshot changed during import")
        validate_runtime_validator_binding(
            binding,
            binding_path=binding_snapshot,
            downloaded_validator=validator_snapshot,
            trusted_validator=TOOLS / "test_clean_release.ps1",
            clean_evidence_path=clean_raw,
            clean=clean,
            run_id=run_id,
            commit=commit,
            candidate_filename=candidate.name,
            candidate_sha256=expected_sha,
        )
        validate_clean_evidence(
            clean, root=root, run_id=run_id, candidate_sha256=expected_sha,
            runtime_validator_sha256=trusted_validator_sha,
        )
        set_gate(validation, root, "WindowsCleanMachine", "windows_clean_machine", "PASS", clean_raw,
                 "independent runtime-only Windows job passed against the exact candidate",
                 candidate_sha256=expected_sha, started_at=clean.get("gate_started_at"),
                 finished_at=clean.get("gate_finished_at"), adapter_metadata={
                     "runtime_validator_binding": binding_snapshot.relative_to(root).as_posix(),
                     "runtime_validator_binding_sha256": sha256(binding_snapshot),
                     "runtime_validator_artifact": validator_snapshot.relative_to(root).as_posix(),
                     "runtime_validator_artifact_sha256": trusted_validator_sha,
                 })

        validate_checkout_identity(commit)
        run_negative_suite(validation, root, candidate, symbols, artifact_stem, symbol_stem)
        refresh_artifact_immutability(validation, root, candidate)
        reset_candidate_promotion_gate(
            validation, root, candidate_sha256=expected_sha,
        )
        stable_names_absent_before_pre_promotion_gate = not output.exists()
        stale_transactions = tuple(
            output.parent.glob(f".{output.name}.promotion-{run_id}-*")
        ) if output.parent.exists() else ()
        if not stable_names_absent_before_pre_promotion_gate:
            raise ValueError(f"promotion output already exists before pre-promotion gate: {output}")
        if stale_transactions:
            raise ValueError(
                "same-run promotion transaction already exists before pre-promotion gate: "
                + ", ".join(path.name for path in stale_transactions)
            )
        validation["status"] = validation["final_status"] = "FINAL AUDIT BEFORE PROMOTION"
        validation["blocking_items"] = []
        run_audit(validation, root, validation_json, validation_text, build_info, candidate, symbols)
        pre_blockers = blockers(validation, PRE_PROMOTION_GATES + SUPPLEMENTAL_GATES)
        if pre_blockers:
            raise ValueError("pre-promotion mandatory gates are blocked: " + ", ".join(pre_blockers))
        if sha256(candidate) != expected_sha:
            raise ValueError("candidate changed during finalization")
        pre_promotion_gate_finished_at = now()

        output.parent.mkdir(parents=True, exist_ok=True)
        if output.exists():
            raise ValueError(f"promotion output already exists: {output}")
        lock_path = output.with_name(f".{output.name}.promotion.lock")
        acquire_output_lock(lock_path, run_id=run_id, commit=commit)
        lock_acquired = True
        if output.exists():
            raise ValueError(f"promotion output appeared after lock acquisition: {output}")
        transaction = output.with_name(
            f".{output.name}.promotion-{run_id}-{uuid.uuid4().hex[:8]}"
        )
        try:
            transaction.resolve().relative_to(root.resolve())
        except ValueError:
            pass
        else:
            raise ValueError("promotion output must be outside the validation run directory")
        transaction.mkdir()
        transaction_owned = True
        stable = transaction / "TPT-ZH-OmniPack-1.1.0-Windows-x64-SDL3.zip"
        stable_symbols = transaction / "TPT-ZH-OmniPack-1.1.0-Windows-x64-Symbols.zip"
        evidence_bundle = transaction / "TPT-ZH-OmniPack-1.1.0-Validation-Evidence.zip"
        stable_name_creation_started_at = now()
        if stable_name_creation_started_at < pre_promotion_gate_finished_at:
            raise ValueError("promotion timing clock moved backwards")

        # Stable-named copies first exist only inside this unpublished,
        # lock-owned transaction. CandidatePromotion is intentionally absent
        # from the aggregate until the directory has actually been published
        # and the published bytes have passed their post-publish audits.
        copy_verified(candidate, stable, expected_sha)
        copy_verified(symbols, stable_symbols, expected_symbols_sha)
        write_sidecar(stable)
        write_sidecar(stable_symbols)
        prepared_marker = transaction / "PROMOTION-PREPARED.json"
        write_json(prepared_marker, {
            "schema": SCHEMA, "schema_version": SCHEMA_VERSION,
            "test": "candidate_promotion_prepared", "status": "PREPARED", "passed": False,
            "run_id": run_id, "commit": commit,
            "publication_model": "exclusive-lock-atomic-directory-publish",
            "transaction_prepared": True,
            "release_filename": stable.name, "release_sha256": expected_sha,
            "symbols_filename": stable_symbols.name, "symbols_sha256": expected_symbols_sha,
            "symbols_member_sha256": expected_symbols_member_sha,
            "candidate_promotion_gate_status": "NOT_YET_COMPLETED",
            "pre_promotion_gate_finished_at": pre_promotion_gate_finished_at,
            "stable_name_creation_started_at": stable_name_creation_started_at,
            "pre_promotion_gate_passed": True,
            "stable_names_absent_before_pre_promotion_gate": stable_names_absent_before_pre_promotion_gate,
            "prepared_at": now(),
        })
        write_sidecar(prepared_marker)
        if sha256(stable) != expected_sha or sha256(stable_symbols) != expected_symbols_sha:
            raise ValueError("promotion transaction changed artifact bytes")
        if package_audit.audit_package(
            stable, False, version="1.1.0", kind="release",
            artifact_stem=artifact_stem, symbol_artifact_stem=symbol_stem,
        ):
            raise ValueError("prepared stable package manifest audit failed")
        if package_audit.audit_package(
            stable_symbols, True, version="1.1.0",
            artifact_stem=artifact_stem, symbol_artifact_stem=symbol_stem,
        ):
            raise ValueError("prepared stable symbols manifest audit failed")

        # This is the only directory publication operation.  The published
        # directory initially carries an explicit PREPARED marker; no READY
        # aggregate or CandidatePromotion PASS exists yet.
        if output.exists():
            raise ValueError(f"promotion output appeared before atomic publish: {output}")
        transaction.rename(output)
        published = True
        transaction_owned = False
        transaction = None
        stable = output / stable.name
        stable_symbols = output / stable_symbols.name
        evidence_bundle = output / evidence_bundle.name
        prepared_marker = output / prepared_marker.name
        validate_sidecar(stable)
        validate_sidecar(stable_symbols)
        validate_sidecar(prepared_marker)
        published_marker = read_json(prepared_marker)
        if (
            published_marker.get("run_id") != run_id
            or published_marker.get("status") != "PREPARED"
            or published_marker.get("transaction_prepared") is not True
            or published_marker.get("release_sha256") != sha256(stable)
            or published_marker.get("symbols_sha256") != sha256(stable_symbols)
            or published_marker.get("candidate_promotion_gate_status") != "NOT_YET_COMPLETED"
            or published_marker.get("pre_promotion_gate_finished_at") != pre_promotion_gate_finished_at
            or published_marker.get("stable_name_creation_started_at") != stable_name_creation_started_at
            or published_marker.get("pre_promotion_gate_passed") is not True
            or published_marker.get("stable_names_absent_before_pre_promotion_gate") is not True
        ):
            raise ValueError("published prepared marker does not bind the stable artifact set")
        if package_audit.audit_package(
            stable, False, version="1.1.0", kind="release",
            artifact_stem=artifact_stem, symbol_artifact_stem=symbol_stem,
        ):
            raise ValueError("published stable package manifest audit failed")
        if package_audit.audit_package(
            stable_symbols, True, version="1.1.0",
            artifact_stem=artifact_stem, symbol_artifact_stem=symbol_stem,
        ):
            raise ValueError("published stable symbols manifest audit failed")
        for artifact in (stable, stable_symbols, prepared_marker):
            validate_sidecar(artifact)
        published_marker = read_json(prepared_marker)
        if (
            published_marker.get("release_sha256") != sha256(stable)
            or published_marker.get("symbols_sha256") != sha256(stable_symbols)
        ):
            raise ValueError("post-publish prepared identities changed during audit")

        # CandidatePromotion becomes PASS only now: the directory rename has
        # happened, the published package bytes are unchanged, and both
        # published package audits passed. The final aggregate and evidence
        # bundle are built from this completed-state gate truth.
        promotion_raw = root / "candidate-promotion.json"
        write_json(promotion_raw, {
            "schema": SCHEMA, "schema_version": SCHEMA_VERSION, "test": "candidate_promotion",
            "run_id": run_id, "commit": commit, "status": "PASS", "passed": True,
            "candidate_filename": artifact_stem + ".zip", "candidate_sha256": expected_sha,
            "stable_filename": stable.name, "stable_sha256_expected": expected_sha,
            "symbols_candidate_filename": symbol_stem + ".zip", "symbols_sha256": expected_symbols_sha,
            "stable_symbols_filename": stable_symbols.name,
            "stable_symbols_sha256_expected": expected_symbols_sha,
            "symbols_member_sha256": expected_symbols_member_sha,
            "promotion_phase": "published_and_reaudited",
            "publication_state": "published_and_reaudited",
            "transaction_complete": True,
            "post_publish_audit_passed": True,
            "stable_copy_published": True,
            "stable_sha256_observed": sha256(stable),
            "stable_symbols_copy_published": True,
            "stable_symbols_sha256_observed": sha256(stable_symbols),
            "byte_for_byte_identity": True, "atomic_rename_only": True,
            "atomic_directory_publish": True,
            "exclusive_output_lock_acquired": True,
            "promotion_complete_marker_required": True,
            "stable_names_absent_before_final_audit": True,
            "pre_promotion_gate_finished_at": pre_promotion_gate_finished_at,
            "stable_name_creation_started_at": stable_name_creation_started_at,
            "pre_promotion_gate_passed": True,
            "stable_names_absent_before_pre_promotion_gate": stable_names_absent_before_pre_promotion_gate,
            "gate_started_at": now(), "gate_finished_at": now(),
        })
        set_gate(
            validation, root, "CandidatePromotion", "candidate_promotion", "PASS",
            promotion_raw, "published stable bytes passed post-publish audit",
            candidate_sha256=expected_sha,
        )
        validation["status"] = validation["final_status"] = "READY FOR 1.1.0 STABLE"
        validation["blocking_items"] = []
        run_audit(
            validation, root, validation_json, validation_text, build_info,
            stable, stable_symbols,
            candidate_artifact=candidate, symbols_artifact=symbols,
        )
        final_blockers = blockers(validation, MANDATORY_GATES + SUPPLEMENTAL_GATES)
        if final_blockers:
            raise ValueError("final mandatory gates are blocked: " + ", ".join(final_blockers))
        if (
            sha256(candidate) != expected_sha or sha256(stable) != expected_sha
            or sha256(symbols) != expected_symbols_sha
            or sha256(stable_symbols) != expected_symbols_sha
        ):
            raise ValueError("published artifact identity changed before evidence freeze")

        write_json(root / "run-metadata.json", {
            "schema": SCHEMA, "schema_version": SCHEMA_VERSION, "test": "run_metadata",
            "run_id": run_id, "commit": commit, "candidate_sha256": expected_sha,
            "stable_filename": stable.name, "stable_sha256_expected": expected_sha,
            "symbols_filename": stable_symbols.name, "symbols_sha256": expected_symbols_sha,
            "symbols_member_sha256": expected_symbols_member_sha,
            "final_status": "READY FOR 1.1.0 STABLE",
            "publication_model": "exclusive-lock-atomic-directory-publish",
            "publication_state": "published_and_reaudited",
        })
        create_evidence_bundle(root, evidence_bundle)
        completion_bundle_audit = audit_frozen_evidence_bundle(
            evidence_bundle,
            package=stable,
            symbols_package=stable_symbols,
            candidate_artifact=candidate,
            symbols_artifact=symbols,
            run_id=run_id,
            commit=commit,
            candidate_sha256=expected_sha,
            symbols_sha256=expected_symbols_sha,
            symbols_member_sha256=expected_symbols_member_sha,
        )
        completion_evidence_sha = validate_bundle_audit_identity(
            completion_bundle_audit, evidence_bundle, phase="completion"
        )
        published_validation_json = output / "RELEASE-VALIDATION.json"
        published_validation_text = output / "RELEASE-VALIDATION.txt"
        published_build_info = output / "BUILD-INFO.txt"
        copy_verified(validation_json, published_validation_json, sha256(validation_json))
        copy_verified(validation_text, published_validation_text, sha256(validation_text))
        copy_verified(build_info, published_build_info, sha256(build_info))
        published_documents = (
            published_validation_json, published_validation_text, published_build_info,
        )
        write_sidecar(evidence_bundle)
        for document in published_documents:
            write_sidecar(document)
        for artifact in (stable, stable_symbols, evidence_bundle, prepared_marker):
            validate_sidecar(artifact)
        published_marker = read_json(prepared_marker)
        if (
            published_marker.get("release_sha256") != sha256(stable)
            or published_marker.get("symbols_sha256") != sha256(stable_symbols)
        ):
            raise ValueError("published identities changed during completion audit")

        completion_release_sha = sha256(stable)
        completion_symbols_sha = sha256(stable_symbols)
        completion_document_hashes = tuple(sha256(document) for document in published_documents)
        completion_prepared_sha = sha256(prepared_marker)

        complete_marker = output / "PROMOTION-COMPLETE.json"
        write_json(complete_marker, {
            "schema": SCHEMA, "schema_version": SCHEMA_VERSION,
            "test": "candidate_promotion_complete", "status": "PASS", "passed": True,
            "run_id": run_id, "commit": commit,
            "publication_model": "exclusive-lock-atomic-directory-publish",
            "publication_state": "published_and_reaudited",
            "transaction_complete": True, "post_publish_audit_passed": True,
            "release_filename": stable.name, "release_sha256": completion_release_sha,
            "symbols_filename": stable_symbols.name, "symbols_sha256": completion_symbols_sha,
            "symbols_member_sha256": expected_symbols_member_sha,
            "evidence_filename": evidence_bundle.name,
            "evidence_sha256": completion_evidence_sha,
            "validation_filename": published_documents[0].name,
            "validation_sha256": completion_document_hashes[0],
            "validation_text_filename": published_documents[1].name,
            "validation_text_sha256": completion_document_hashes[1],
            "build_info_filename": published_documents[2].name,
            "build_info_sha256": completion_document_hashes[2],
            "prepared_marker_filename": prepared_marker.name,
            "prepared_marker_sha256": completion_prepared_sha,
            "evidence_bundle_audit_passed": completion_bundle_audit["passed"],
            "evidence_bundle_members_total": completion_bundle_audit["members_total"],
            "completed_at": now(),
        })
        write_sidecar(complete_marker)
        validate_sidecar(complete_marker)
        complete = read_json(complete_marker)
        validate_published_document_set(
            bundle=evidence_bundle,
            documents=published_documents,
            marker=complete,
        )
        if (
            complete.get("status") != "PASS"
            or complete.get("transaction_complete") is not True
            or complete.get("post_publish_audit_passed") is not True
            or complete.get("release_sha256") != completion_release_sha
            or sha256(stable) != completion_release_sha
            or complete.get("symbols_sha256") != completion_symbols_sha
            or sha256(stable_symbols) != completion_symbols_sha
            or complete.get("evidence_sha256") != completion_evidence_sha
            or sha256(evidence_bundle) != completion_evidence_sha
            or complete.get("prepared_marker_sha256") != completion_prepared_sha
            or sha256(prepared_marker) != completion_prepared_sha
            or tuple(sha256(document) for document in published_documents) != completion_document_hashes
        ):
            raise ValueError("published completion marker does not bind the re-audited artifact set")
        print(json.dumps({
            "status": "READY FOR 1.1.0 STABLE", "run_id": run_id, "commit": commit,
            "release": str(stable), "release_sha256": sha256(stable),
            "symbols": str(stable_symbols), "symbols_sha256": sha256(stable_symbols),
            "evidence": str(evidence_bundle), "evidence_sha256": sha256(evidence_bundle),
        }, ensure_ascii=False, indent=2))
        if lock_path and lock_acquired:
            remove_owned_lock(lock_path, run_id)
            lock_acquired = False
        return 0
    except Exception as exc:
        rollback_failed = False
        try:
            if published and output.is_dir():
                # `published` becomes true only immediately after this
                # process atomically renamed its lock-owned transaction.  Do
                # not trust a potentially damaged marker to decide whether we
                # can remove the directory we just created; that would leave
                # a partially validated stable-named output behind.
                rollback = output.with_name(
                    f".{output.name}.promotion-{validation.get('run_id')}-rollback-{uuid.uuid4().hex[:8]}"
                )
                output.rename(rollback)
                remove_transaction(rollback, output)
                published = False
            if transaction is not None and transaction_owned:
                remove_transaction(transaction, output)
        except (OSError, ValueError, json.JSONDecodeError) as rollback_error:
            rollback_failed = True
            print(f"finalize-release: rollback failed: {rollback_error}", file=sys.stderr)
        if lock_path and lock_acquired and not rollback_failed:
            remove_owned_lock(lock_path, str(validation.get("run_id", "")))
            lock_acquired = False
        if rewrite_failure_aggregate:
            try:
                validation = read_json(validation_json)
                validation["status"] = validation["final_status"] = "NOT READY FOR 1.1.0 STABLE"
                validation["blocking_items"] = [str(exc)]
                write_json(validation_json, validation)
                write_validation_text(validation, validation_text)
            except Exception:
                pass
        print(f"finalize-release: ERROR {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
