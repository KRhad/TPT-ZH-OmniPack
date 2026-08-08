#!/usr/bin/env python3
"""Build verified Windows/Android assets and a static GitHub Startup.json."""

from __future__ import annotations

import argparse
import bz2
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess
from typing import Sequence


PROJECT_URL = "https://github.com/KRhad/TPT-ZH-OmniPack"
VERSION_RE = re.compile(r"^(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)$")


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest().upper()


def parse_version(version: str) -> tuple[int, int, int]:
    match = VERSION_RE.fullmatch(version)
    if not match:
        raise ValueError("version must be MAJOR.MINOR.PATCH")
    values = tuple(int(value) for value in match.groups())
    if values[1] > 999 or values[2] > 999:
        raise ValueError("minor and patch must fit the Android versionCode mapping")
    return values


def android_version_code(version: tuple[int, int, int]) -> int:
    return version[0] * 1_000_000 + version[1] * 1_000 + version[2]


def git_text(source_root: Path, *args: str) -> str:
    result = subprocess.run(
        ["git", *args],
        cwd=source_root,
        check=False,
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="replace",
    )
    if result.returncode:
        raise ValueError(result.stderr.strip() or "git command failed")
    return result.stdout.strip()


def build_windows_update(executable: bytes) -> bytes:
    if len(executable) < 2 or executable[:2] != b"MZ":
        raise ValueError("Windows executable is not a PE file")
    if len(executable) > 0xFFFFFFFF:
        raise ValueError("Windows executable is too large for the update format")
    return b"BuTT" + struct.pack("<I", len(executable)) + bz2.compress(executable, 9)


def asset_record(path: str, package_format: str, data: bytes) -> dict[str, object]:
    return {
        "File": path,
        "Format": package_format,
        "Size": len(data),
        "Sha256": sha256(data),
    }


def build_startup_manifest(
    version: str,
    windows_asset: tuple[str, bytes],
    android_asset: tuple[str, bytes],
    *,
    source_revision: str,
    published_at: str,
    changelog: str,
) -> dict[str, object]:
    major, minor, patch = parse_version(version)
    return {
        "SchemaVersion": 1,
        "Product": "TPT-ZH-OmniPack",
        "ProjectURL": PROJECT_URL,
        "SourceRevision": source_revision,
        "PublishedAt": published_at,
        "Session": True,
        "MessageOfTheDay": "",
        "Notifications": [],
        "Updates": {
            "Stable": {
                "Major": major,
                "Minor": minor,
                "Build": patch,
                "Version": version,
                "VersionCode": android_version_code((major, minor, patch)),
                "MinimumVersion": "1.0.0",
                "Changelog": changelog,
                "ReleaseNotesURL": PROJECT_URL,
                "Platforms": {
                    "WIN64": asset_record(
                        windows_asset[0], "butt-executable", windows_asset[1]
                    ),
                    "ANDROIDARM64": asset_record(
                        android_asset[0], "android-apk", android_asset[1]
                    ),
                },
            }
        },
        "PublicTestsIncluded": False,
        "ReleaseReady": False,
    }


def write_bytes_new(path: Path, data: bytes) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    if path.exists():
        raise ValueError(f"output already exists: {path}")
    path.write_bytes(data)


def write_text_new(path: Path, text: str, encoding: str = "utf-8") -> None:
    write_bytes_new(path, text.encode(encoding))


def build_package(
    source_root: Path,
    windows_executable: Path,
    android_apk: Path,
    output_directory: Path,
    version: str,
    changelog: str,
) -> list[Path]:
    version_tuple = parse_version(version)
    source_revision = git_text(source_root, "rev-parse", "HEAD")
    if not re.fullmatch(r"[0-9a-f]{40}", source_revision):
        raise ValueError("could not determine the source revision")
    published_at = git_text(source_root, "show", "-s", "--format=%cI", "HEAD")
    published_at = datetime.fromisoformat(published_at).astimezone(timezone.utc).isoformat().replace("+00:00", "Z")

    windows_exe_data = windows_executable.read_bytes()
    android_data = android_apk.read_bytes()
    if len(android_data) < 4 or android_data[:4] != b"PK\x03\x04":
        raise ValueError("Android package is not an APK/ZIP file")
    windows_data = build_windows_update(windows_exe_data)

    windows_name = f"TPT-ZH-OmniPack-{version}-Windows-x64.update"
    android_name = f"TPT-ZH-OmniPack-{version}-Android-arm64-v8a.apk"
    windows_relative = f"/updates/{windows_name}"
    android_relative = f"/updates/{android_name}"
    manifest = build_startup_manifest(
        version,
        (windows_relative, windows_data),
        (android_relative, android_data),
        source_revision=source_revision,
        published_at=published_at,
        changelog=changelog,
    )

    output_directory.mkdir(parents=True, exist_ok=True)
    windows_path = output_directory / "updates" / windows_name
    android_path = output_directory / "updates" / android_name
    startup_path = output_directory / "Startup.json"
    manifest_path = output_directory / "UPDATE-MANIFEST.txt"
    write_bytes_new(windows_path, windows_data)
    write_bytes_new(android_path, android_data)
    write_text_new(windows_path.with_suffix(windows_path.suffix + ".sha256"), f"{sha256(windows_data)}  {windows_name}\n", "ascii")
    write_text_new(android_path.with_suffix(android_path.suffix + ".sha256"), f"{sha256(android_data)}  {android_name}\n", "ascii")
    write_text_new(startup_path, json.dumps(manifest, ensure_ascii=False, indent=2) + "\n")

    records = [
        "format=1",
        "kind=github-static-update-channel",
        "product=TPT-ZH-OmniPack",
        f"version={version}",
        f"version_code={android_version_code(version_tuple)}",
        f"source_revision={source_revision}",
        f"published_at={published_at}",
        f"asset={windows_name}|{len(windows_data)}|{sha256(windows_data)}|butt-executable",
        f"asset={android_name}|{len(android_data)}|{sha256(android_data)}|android-apk",
        f"startup_json_sha256={sha256(startup_path.read_bytes())}",
        "public_tests_included=false",
        "release_ready=false",
    ]
    write_text_new(manifest_path, "\n".join(records) + "\n", "ascii")
    return [
        startup_path,
        manifest_path,
        windows_path,
        windows_path.with_suffix(windows_path.suffix + ".sha256"),
        android_path,
        android_path.with_suffix(android_path.suffix + ".sha256"),
    ]


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--windows-executable", type=Path, required=True)
    parser.add_argument("--android-apk", type=Path, required=True)
    parser.add_argument("--output-directory", type=Path, required=True)
    parser.add_argument("--version", default="1.0.0")
    parser.add_argument("--changelog", default="TPT-ZH-OmniPack stable update")
    return parser


def main(argv: Sequence[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    try:
        paths = build_package(
            args.source_root.resolve(),
            args.windows_executable.resolve(),
            args.android_apk.resolve(),
            args.output_directory.resolve(),
            args.version,
            args.changelog,
        )
    except (OSError, ValueError, subprocess.SubprocessError) as error:
        print(f"github-update-package: ERROR {error}")
        return 1
    for path in paths:
        print(f"github-update-package: {path}")
    print("github-update-package: PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
