#!/usr/bin/env python3
"""Create a deterministic, test-free public source archive."""

from __future__ import annotations

import argparse
from dataclasses import dataclass
import hashlib
import os
from pathlib import Path, PurePosixPath
import shutil
import subprocess
import tempfile
from typing import Sequence
import zipfile


FINAL_VERSION = "1.0.0"
SOURCE_STEM = f"TPT-ZH-OmniPack-{FINAL_VERSION}-Source"
EXCLUDED_PREFIXES = (
    ".github/",
    "automation/",
    "dist/",
    "examples/",
    "external/",
    "tests/",
    "tools/runtime/",
    "tools/tests/",
)
ALLOWED_TOOLS = {
    "tools/atmospherebench/AtmosphereBench.cpp",
    "tools/atmospherebench/AtmosphereBench.h",
	"tools/atmospherebench/Hllc2D.cpp",
	"tools/atmospherebench/Hllc2D.h",
	"tools/atmospherebench/HybridMixedRegion1D.cpp",
	"tools/atmospherebench/HybridMixedRegion1D.h",
	"tools/atmospherebench/HybridMixedRegion2D.cpp",
	"tools/atmospherebench/HybridMixedRegion2D.h",
	"tools/atmospherebench/HybridPolicy1D.cpp",
	"tools/atmospherebench/HybridPolicy1D.h",
	"tools/atmospherebench/HybridProjectionCoupling2D.cpp",
	"tools/atmospherebench/HybridProjectionCoupling2D.h",
	"tools/atmospherebench/LbmD2Q9.cpp",
	"tools/atmospherebench/LbmD2Q9.h",
	"tools/atmospherebench/LegacyLike.cpp",
	"tools/atmospherebench/LegacyLike.h",
	"tools/atmospherebench/LowMachProjection2D.cpp",
	"tools/atmospherebench/LowMachProjection2D.h",
	"tools/atmospherebench/LowMachProjection2DPerformance.cpp",
	"tools/atmospherebench/LowMachProjection2DPerformance.h",
	"tools/atmospherebench/PrecisionMatrix.cpp",
	"tools/atmospherebench/Phase5Selection.cpp",
	"tools/atmospherebench/Phase5Selection.h",
	"tools/atmospherebench/Species2D.cpp",
	"tools/atmospherebench/Species2D.h",
    "tools/atmospherebench/main.cpp",
    "tools/atmospherebench/Rusanov1D.cpp",
    "tools/atmospherebench/Rusanov1D.h",
    "tools/build_release_font.py",
	"tools/atmosphere_policy_check.py",
    "tools/element_registry_check.py",
    "tools/generate_content_menu_policy.py",
    "tools/generate_element_catalog.py",
    "tools/generate_periodic_content_links.py",
    "tools/generate_periodic_table_data.py",
    "tools/omnicore_data_check.py",
    "tools/package_source_release.py",
    "tools/physical_scale_check.py",
    "tools/prepare_windows_release.py",
    "tools/refresh_official_element_descriptions.py",
    "tools/sync_element_localization.py",
	"tools/run_atmospherebench.ps1",
	"tools/run_atmosphere_precision_matrix.py",
	"tools/run_phase5_selection.ps1",
}
ROOT_REPLACEMENTS = {
    "README.md": "docs/PUBLIC_GITHUB_README.md",
    "README.zh-CN.md": "docs/RELEASE_1.0.0_README.zh-CN.md",
    "changelog.txt": "docs/RELEASE_1.0.0_CHANGELOG.en.txt",
    "CHANGELOG.zh-CN.md": "docs/RELEASE_1.0.0_CHANGELOG.zh-CN.md",
}
REQUIRED_MEMBERS = {
    "LICENSE",
    "README.md",
    "README.zh-CN.md",
    "meson.build",
    "meson_options.txt",
    "src/PowderToy.cpp",
    "src/Config.template.h",
    "resources/font.bz2",
    "resources/omnicore/v1/README.md",
    "resources/omnicore/v1/catalog.json",
    "resources/omnicore/v1/legacy-material-map.json",
	"resources/omnicore/v1/atmosphere-policy-candidates.json",
    "resources/omnicore/v1/physical-scale-candidates.json",
    "resources/omnicore/v1/schema.json",
    "resources/omnicore/v1/unit-registry.schema.json",
    "resources/omnicore/v1/units.json",
    "tools/generate_element_catalog.py",
    "tools/generate_periodic_table_data.py",
    "tools/generate_content_menu_policy.py",
    "tools/generate_periodic_content_links.py",
    "tools/omnicore_data_check.py",
	"tools/atmosphere_policy_check.py",
    "tools/physical_scale_check.py",
    "tools/run_atmospherebench.ps1",
    "tools/atmospherebench/AtmosphereBench.cpp",
    "tools/atmospherebench/AtmosphereBench.h",
	"tools/atmospherebench/Hllc2D.cpp",
	"tools/atmospherebench/Hllc2D.h",
	"tools/atmospherebench/HybridMixedRegion1D.cpp",
	"tools/atmospherebench/HybridMixedRegion1D.h",
	"tools/atmospherebench/HybridMixedRegion2D.cpp",
	"tools/atmospherebench/HybridMixedRegion2D.h",
	"tools/atmospherebench/HybridPolicy1D.cpp",
	"tools/atmospherebench/HybridPolicy1D.h",
	"tools/atmospherebench/LbmD2Q9.cpp",
	"tools/atmospherebench/LbmD2Q9.h",
	"tools/atmospherebench/LegacyLike.cpp",
	"tools/atmospherebench/LegacyLike.h",
	"tools/atmospherebench/LowMachProjection2D.cpp",
	"tools/atmospherebench/LowMachProjection2D.h",
	"tools/atmospherebench/LowMachProjection2DPerformance.cpp",
	"tools/atmospherebench/LowMachProjection2DPerformance.h",
	"tools/atmospherebench/HybridProjectionCoupling2D.cpp",
	"tools/atmospherebench/HybridProjectionCoupling2D.h",
	"tools/atmospherebench/Phase5Selection.cpp",
	"tools/atmospherebench/Phase5Selection.h",
	"tools/atmospherebench/PrecisionMatrix.cpp",
	"tools/atmospherebench/Species2D.cpp",
	"tools/atmospherebench/Species2D.h",
    "tools/atmospherebench/main.cpp",
    "tools/atmospherebench/Rusanov1D.cpp",
	"tools/atmospherebench/Rusanov1D.h",
	"tools/run_atmosphere_precision_matrix.py",
	"tools/run_phase5_selection.ps1",
}
FORBIDDEN_SUFFIXES = (
    ".debug",
    ".dmp",
    ".exe",
    ".log",
    ".obj",
    ".o",
    ".pdb",
    ".pref",
    ".zip",
)


@dataclass(frozen=True)
class SourceEntry:
    name: str
    data: bytes
    executable: bool


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest().upper()


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest().upper()


def git_output(source_root: Path, *args: str) -> bytes:
    completed = subprocess.run(
        ["git", *args],
        cwd=source_root,
        check=False,
        capture_output=True,
    )
    if completed.returncode:
        raise ValueError(
            completed.stderr.decode("utf-8", errors="replace").strip()
            or f"git {' '.join(args)} failed"
        )
    return completed.stdout


def is_excluded(name: str) -> bool:
    lowered = name.lower()
    if any(name.startswith(prefix) for prefix in EXCLUDED_PREFIXES):
        return True
    if lowered.endswith(FORBIDDEN_SUFFIXES):
        return True
    if name.startswith("tools/"):
        return name not in ALLOWED_TOOLS and not name.startswith("tools/data/")
    if name.startswith("docs/"):
        basename = PurePosixPath(name).name.upper()
        if any(token in basename for token in ("AUDIT", "EVIDENCE", "PRIVATE_TEST", "TEST_")):
            return True
        if basename in {
            "BASELINE_BUILD.MD",
            "FINAL_VALIDATION.MD",
            "PERFORMANCE_BASELINE.MD",
            "PROGRESS.MD",
            "RELEASE_CANDIDATE_1.0.0.MD",
            "RELEASE_HARDENING.MD",
            "VERSION_GATES.MD",
        }:
            return True
    return False


def tracked_entries(source_root: Path) -> list[SourceEntry]:
    records = git_output(
        source_root,
        "ls-tree",
        "-r",
        "-z",
        "--full-tree",
        "HEAD",
    ).split(b"\0")
    entries: list[SourceEntry] = []
    for record in records:
        if not record:
            continue
        metadata, encoded_name = record.split(b"\t", 1)
        mode, object_type, _object_id = metadata.decode("ascii").split(" ", 2)
        name = encoded_name.decode("utf-8")
        if object_type != "blob" or is_excluded(name):
            continue
        source_name = ROOT_REPLACEMENTS.get(name, name)
        source = source_root / PurePosixPath(source_name)
        if not source.is_file():
            raise ValueError(f"tracked source file is missing: {source_name}")
        entries.append(
            SourceEntry(
                name=name,
                data=source.read_bytes(),
                executable=mode == "100755",
            )
        )
    entries.sort(key=lambda entry: entry.name)
    names = {entry.name for entry in entries}
    missing = sorted(REQUIRED_MEMBERS - names)
    if missing:
        raise ValueError("source archive is missing required members: " + ", ".join(missing))
    forbidden = sorted(
        name
        for name in names
        if name.startswith(("tests/", "tools/tests/", "tools/runtime/"))
        or "/test_" in name.lower()
        or name.lower().endswith("_test.ps1")
    )
    if forbidden:
        raise ValueError("source archive contains test assets: " + ", ".join(forbidden))
    return entries


def source_epoch() -> int:
    value = os.environ.get("SOURCE_DATE_EPOCH", "315532800")
    epoch = int(value)
    if epoch < 315532800:
        raise ValueError("SOURCE_DATE_EPOCH must be on or after 1980-01-01")
    return epoch


def zip_datetime(epoch: int) -> tuple[int, int, int, int, int, int]:
    from datetime import datetime, timezone

    stamp = datetime.fromtimestamp(epoch, tz=timezone.utc)
    return stamp.year, stamp.month, stamp.day, stamp.hour, stamp.minute, stamp.second


def write_archive(
    archive_path: Path,
    stem: str,
    entries: list[SourceEntry],
    revision: str,
    epoch: int,
) -> None:
    timestamp = zip_datetime(epoch)
    manifest_lines = [
        "format=1",
        "kind=source",
        f"version={FINAL_VERSION}",
        f"revision={revision}",
        f"file_count={len(entries)}",
    ]
    manifest_lines.extend(
        f"member={entry.name}|{len(entry.data)}|{sha256_bytes(entry.data)}"
        for entry in entries
    )
    with zipfile.ZipFile(
        archive_path,
        "w",
        compression=zipfile.ZIP_DEFLATED,
        compresslevel=9,
        strict_timestamps=True,
    ) as archive:
        for entry in entries:
            info = zipfile.ZipInfo(f"{stem}/{entry.name}", date_time=timestamp)
            info.compress_type = zipfile.ZIP_DEFLATED
            info.external_attr = (0o100755 if entry.executable else 0o100644) << 16
            archive.writestr(info, entry.data)
        info = zipfile.ZipInfo(f"{stem}/SOURCE-MANIFEST.txt", date_time=timestamp)
        info.compress_type = zipfile.ZIP_DEFLATED
        info.external_attr = 0o100644 << 16
        archive.writestr(info, ("\n".join(manifest_lines) + "\n").encode("utf-8"))


def build_source_archive(source_root: Path, output_directory: Path) -> tuple[Path, Path]:
    source_root = source_root.resolve()
    output_directory = output_directory.resolve()
    if git_output(source_root, "status", "--porcelain"):
        raise ValueError("source worktree is dirty; commit changes before packaging")
    revision = git_output(source_root, "rev-parse", "HEAD").decode("ascii").strip()
    if len(revision) != 40:
        raise ValueError(f"unexpected Git revision: {revision!r}")
    entries = tracked_entries(source_root)
    output_directory.mkdir(parents=True, exist_ok=True)
    archive_path = output_directory / f"{SOURCE_STEM}.zip"
    hash_path = archive_path.with_suffix(archive_path.suffix + ".sha256")
    if archive_path.exists() or hash_path.exists():
        raise ValueError(f"source package output already exists: {archive_path}")
    with tempfile.TemporaryDirectory(dir=output_directory) as temporary:
        temporary_archive = Path(temporary) / archive_path.name
        write_archive(temporary_archive, SOURCE_STEM, entries, revision, source_epoch())
        shutil.move(temporary_archive, archive_path)
    hash_path.write_text(
        f"{sha256(archive_path)}  {archive_path.name}\n",
        encoding="ascii",
        newline="\n",
    )
    return archive_path, hash_path


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--output-directory", type=Path, default=Path("dist/1.0.0"))
    parser.add_argument("--version", choices=(FINAL_VERSION,), default=FINAL_VERSION)
    return parser


def main(argv: Sequence[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    source_root = args.source_root.resolve()
    output_directory = (
        args.output_directory
        if args.output_directory.is_absolute()
        else source_root / args.output_directory
    )
    try:
        archive, hash_path = build_source_archive(source_root, output_directory)
    except (OSError, ValueError, zipfile.BadZipFile) as exc:
        print(f"source-release-package: ERROR {exc}")
        return 1
    print(f"source-release-package: PASS {archive}")
    print(f"source-release-package: SHA256 {hash_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
