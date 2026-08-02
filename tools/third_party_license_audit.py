#!/usr/bin/env python3
"""Fail closed on third-party source, resource, and release-notice drift."""

from __future__ import annotations

import argparse
import csv
import hashlib
import json
from pathlib import Path
import runpy
import subprocess
from typing import Any, Sequence


MANIFEST = "docs/THIRD_PARTY_LICENSE_MANIFEST.csv"
VALID_SOURCE_TYPES = {
    "cpp_source",
    "lua_source",
    "mixed_source",
    "binary_only",
    "documentation_only",
    "unavailable",
}
REQUIRED_LINKED_COMPONENTS = {
    "tpt-upstream",
    "font-fusion-main",
    "font-fusion-ark",
    "font-fusion-cubic11",
    "font-fusion-galmuri",
    "font-unifont",
    "lib-bzip2",
    "lib-fftw3f",
    "lib-jsoncpp",
    "lib-curl",
    "lib-libpng",
    "lib-luajit",
    "lib-mbedtls",
    "lib-nghttp2",
    "lib-sdl2",
    "lib-zlib",
}
PROJECT_SOURCES = {
    "TPT-ZH-OmniPack/original",
    "TPT-ZH-OmniPack/reserved",
}
EXTERNAL_SNAPSHOTS = {
    "cracker1000": "ebbb9aab6aef27d26517682cebbc0a07147a843a",
    "seppo": "c3a8dd171a1c0fefc9a386e7e069f81d91f1514f",
    "cyens_src": "1b74504e4642cd967c0079499b57faa7f37d9668",
    "ultimata": "b74971752433652c033559abea415ec3510ac433",
    "biological_mod": "284a1585db023f62a7147892526899133dd6f41c",
    "nucular_mod": "048080a79006c4d6668a1864a0e29758903c64bb",
}
TRACKED_BINARY_RESOURCE_ALLOWLIST = {
    "resources/font.bz2",
    "resources/icon_cps_xp.ico",
    "resources/icon_exe_xp.ico",
    "resources/save_local.png",
    "resources/save_online.png",
}
TRACKED_BINARY_EXTENSIONS = {".bdf", ".bz2", ".gz", ".ico", ".png"}
REQUIRED_COLUMNS = {
    "component",
    "category",
    "registry_source_mod",
    "source_url",
    "version_or_commit",
    "usage",
    "license",
    "license_file",
    "license_sha256",
    "included_in_binary",
    "notice_in_package",
    "package_path",
    "audit_status",
}


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest().upper()


def read_csv(path: Path) -> list[dict[str, str]]:
    with path.open("r", encoding="utf-8", newline="") as stream:
        return list(csv.DictReader(stream))


def package_documents(root: Path) -> dict[str, str]:
    namespace = runpy.run_path(str(root / "tools/package_test_release.py"))
    return dict(namespace["DOCUMENTS"])


def audit_manifest(root: Path) -> tuple[list[str], list[dict[str, str]]]:
    errors: list[str] = []
    path = root / MANIFEST
    if not path.is_file():
        return [f"missing license manifest: {MANIFEST}"], []
    with path.open("r", encoding="utf-8", newline="") as stream:
        reader = csv.DictReader(stream)
        rows = list(reader)
        columns = set(reader.fieldnames or [])
    missing_columns = sorted(REQUIRED_COLUMNS - columns)
    if missing_columns:
        errors.append(f"license manifest is missing columns: {','.join(missing_columns)}")
    components: set[str] = set()
    try:
        documents = package_documents(root)
    except Exception as exc:  # pragma: no cover - exact import error is platform-specific
        errors.append(f"cannot load package document map: {exc}")
        documents = {}
    if documents.get(MANIFEST) != "LICENSES/THIRD-PARTY-MANIFEST.csv":
        errors.append("release package does not include the license manifest")
    for index, row in enumerate(rows, start=2):
        component = row.get("component", "")
        if not component:
            errors.append(f"license manifest row {index} has no component")
        elif component in components:
            errors.append(f"license manifest repeats component: {component}")
        components.add(component)
        if row.get("audit_status") != "verified":
            errors.append(f"license manifest component is not verified: {component}")
        for field in ("source_url", "version_or_commit", "usage", "license"):
            if not row.get(field):
                errors.append(f"license manifest {component} has empty {field}")
        for field in ("included_in_binary", "notice_in_package"):
            if row.get(field) not in {"true", "false"}:
                errors.append(f"license manifest {component} has invalid {field}")
        license_name = row.get("license_file", "")
        license_path = (root / license_name).resolve()
        try:
            license_path.relative_to(root.resolve())
        except ValueError:
            errors.append(f"license file escapes source root: {license_name}")
            continue
        if not license_path.is_file():
            errors.append(f"license file is missing for {component}: {license_name}")
            continue
        actual_hash = sha256(license_path)
        if actual_hash != row.get("license_sha256"):
            errors.append(
                f"license hash mismatch for {component}: {actual_hash}"
            )
        if row.get("notice_in_package") == "true":
            package_path = row.get("package_path", "")
            if documents.get(license_name) != package_path:
                errors.append(
                    f"release package notice mapping is missing for {component}: "
                    f"{license_name} -> {package_path}"
                )
    missing_components = sorted(REQUIRED_LINKED_COMPONENTS - components)
    if missing_components:
        errors.append(
            f"license manifest omits linked components: {','.join(missing_components)}"
        )
    return errors, rows


def audit_catalog(root: Path, manifest_rows: list[dict[str, str]]) -> tuple[list[str], dict[str, int]]:
    errors: list[str] = []
    source_rows = read_csv(root / "docs/MOD_SOURCE_CATALOG.csv")
    if len(source_rows) < 30:
        errors.append(f"mod source catalog is too small: {len(source_rows)}")
    source_by_id: dict[str, dict[str, str]] = {}
    for row in source_rows:
        mod_id = row.get("mod_id", "")
        if not mod_id or mod_id in source_by_id:
            errors.append(f"mod source catalog has missing or duplicate mod_id: {mod_id}")
        source_by_id[mod_id] = row
        if row.get("source_type") not in VALID_SOURCE_TYPES:
            errors.append(f"mod {mod_id} has invalid source_type")
        if not row.get("audit_status"):
            errors.append(f"mod {mod_id} has no audit_status")
        if row.get("license_verified") == "true" and row.get("license") in {"", "unknown"}:
            errors.append(f"mod {mod_id} claims a verified unknown license")
    element_rows = read_csv(root / "docs/MOD_ELEMENT_CATALOG.csv")
    for row in element_rows:
        if row.get("license_status") != "verified" and row.get("port_decision") in {
            "A_direct_port",
            "B_rewrite_port",
        }:
            errors.append(
                f"unverified candidate is eligible for porting: "
                f"{row.get('source_mod')}:{row.get('source_identifier')}"
            )
    registry_manifest = {
        row["registry_source_mod"]: row
        for row in manifest_rows
        if row.get("registry_source_mod")
    }
    registry_rows = read_csv(root / "docs/ELEMENT_REGISTRY.csv")
    ledger = (root / "docs/PORTING_LEDGER.md").read_text(encoding="utf-8")
    used_external = 0
    for row in registry_rows:
        if row.get("implementation_status") != "implemented":
            continue
        source_mod = row.get("source_mod", "")
        if source_mod in PROJECT_SOURCES:
            continue
        manifest = registry_manifest.get(source_mod)
        if manifest is None:
            errors.append(
                f"implemented registry source has no license manifest row: {source_mod}"
            )
            continue
        if row.get("license") != "GPL-3.0-only":
            errors.append(
                f"implemented external element has unexpected license: {row.get('identifier')}"
            )
        allowed_commits = set(manifest.get("version_or_commit", "").split(";"))
        if row.get("source_commit") not in allowed_commits:
            errors.append(
                f"implemented external element has unpinned commit: "
                f"{row.get('identifier')}:{row.get('source_commit')}"
            )
        if source_mod == "The-Powder-Toy/The-Powder-Toy":
            continue
        if row.get("identifier", "") not in ledger:
            errors.append(
                f"implemented external element is absent from porting ledger: "
                f"{row.get('identifier')}"
            )
        used_external += 1
    return errors, {
        "mods_cataloged": len(source_rows),
        "mods_license_verified": sum(
            row.get("license_verified") == "true" for row in source_rows
        ),
        "mods_license_unknown_or_unverified": sum(
            row.get("license_verified") != "true" for row in source_rows
        ),
        "candidate_elements": len(element_rows),
        "implemented_external_elements": used_external,
    }


def audit_tracked_resources(root: Path) -> tuple[list[str], int]:
    errors: list[str] = []
    completed = subprocess.run(
        ["git", "ls-files"],
        cwd=root,
        check=False,
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="replace",
    )
    if completed.returncode:
        return [f"git ls-files failed: {completed.stderr.strip()}"], 0
    tracked_assets = 0
    for name in completed.stdout.splitlines():
        normalized = name.replace("\\", "/")
        if normalized.startswith(("external/repositories/", "external/lua-mods/")):
            if not normalized.endswith("/.gitkeep"):
                errors.append(f"read-only external source is tracked: {normalized}")
        if Path(normalized).suffix.lower() not in TRACKED_BINARY_EXTENSIONS:
            continue
        tracked_assets += 1
        if (
            normalized in TRACKED_BINARY_RESOURCE_ALLOWLIST
            or normalized.startswith("resources/generated_icons/")
            or normalized.startswith("resources/third_party/")
        ):
            continue
        errors.append(f"tracked binary resource is not license-classified: {normalized}")
    return errors, tracked_assets


def verify_external_snapshots(root: Path) -> list[str]:
    errors: list[str] = []
    expected_license_hash = sha256(root / "LICENSE")
    for directory, expected_commit in EXTERNAL_SNAPSHOTS.items():
        path = root / "external/repositories" / directory
        completed = subprocess.run(
            ["git", "rev-parse", "HEAD"],
            cwd=path,
            check=False,
            capture_output=True,
            text=True,
            encoding="utf-8",
            errors="replace",
        )
        if completed.returncode or completed.stdout.strip() != expected_commit:
            errors.append(f"external snapshot commit mismatch: {directory}")
        license_path = path / "LICENSE"
        if not license_path.is_file() or sha256(license_path) != expected_license_hash:
            errors.append(f"external snapshot GPL license mismatch: {directory}")
    return errors


def audit(root: Path, live_snapshots: bool = False) -> tuple[list[str], dict[str, Any]]:
    errors, manifest_rows = audit_manifest(root)
    catalog_errors, catalog_stats = audit_catalog(root, manifest_rows)
    resource_errors, tracked_assets = audit_tracked_resources(root)
    errors.extend(catalog_errors)
    errors.extend(resource_errors)
    if live_snapshots:
        errors.extend(verify_external_snapshots(root))
    report: dict[str, Any] = {
        "schema_version": 1,
        "third_party_license_audit": not errors,
        "manifest_components": len(manifest_rows),
        "linked_components_required": len(REQUIRED_LINKED_COMPONENTS),
        "tracked_binary_resources": tracked_assets,
        "external_snapshots_verified": len(EXTERNAL_SNAPSHOTS) if live_snapshots and not errors else 0,
        **catalog_stats,
        "errors": errors,
    }
    return errors, report


def main(argv: Sequence[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--source-root", type=Path, default=Path(__file__).resolve().parents[1]
    )
    parser.add_argument("--verify-external-snapshots", action="store_true")
    parser.add_argument("--report", type=Path)
    parser.add_argument("--quiet", action="store_true")
    args = parser.parse_args(argv)
    root = args.source_root.resolve()
    errors, report = audit(root, args.verify_external_snapshots)
    if args.report:
        report_path = args.report
        if not report_path.is_absolute():
            report_path = root / report_path
        report_path.parent.mkdir(parents=True, exist_ok=True)
        report_path.write_text(
            json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8"
        )
    if errors:
        for error in errors:
            print(f"third-party-license-audit: FAIL: {error}")
        return 1
    if not args.quiet:
        print(
            "third-party-license-audit: PASS "
            f"components={report['manifest_components']} "
            f"mods={report['mods_cataloged']} "
            f"implemented_external={report['implemented_external_elements']}"
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
