#!/usr/bin/env python3
"""Build a minimal, auditable Windows x64 test ZIP from a verified executable."""

from __future__ import annotations

import argparse
import hashlib
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
from typing import Iterable, Sequence
import zipfile


PACKAGE_NAME = "TPT-ZH-OmniPack-Test-Windows-x64"
EXECUTABLE_NAME = "tpt-zh-omnipack.exe"
DOCUMENTS = (
    ("LICENSE", "LICENSE"),
    ("README.zh-CN.md", "README.zh-CN.md"),
    ("CHANGELOG.zh-CN.md", "CHANGELOG.zh-CN.md"),
    ("docs/TEST_RELEASE.md", "TESTING.zh-CN.md"),
)


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest().upper()


def git_revision(source_root: Path) -> str:
    completed = subprocess.run(
        ["git", "rev-parse", "HEAD"],
        cwd=source_root,
        check=False,
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="replace",
    )
    if completed.returncode:
        return "unknown"
    revision = completed.stdout.strip()
    return revision if revision else "unknown"


def required_sources(source_root: Path, executable: Path) -> list[Path]:
    sources = [executable]
    for source_name, _ in DOCUMENTS:
        sources.append(source_root / source_name)
    return sources


def validate_sources(source_root: Path, executable: Path) -> list[str]:
    errors: list[str] = []
    if executable.name != EXECUTABLE_NAME:
        errors.append(
            f"executable must be named {EXECUTABLE_NAME!r}, got {executable.name!r}"
        )
    for path in required_sources(source_root, executable):
        if not path.is_file():
            errors.append(f"required test-release source is missing: {path}")
    if executable.is_file():
        with executable.open("rb") as stream:
            if stream.read(2) != b"MZ":
                errors.append(f"executable is not a Windows PE file: {executable}")
    return errors


def manifest_lines(revision: str, executable: Path, documents: Iterable[tuple[str, Path]]) -> str:
    lines = [
        "format=1",
        f"revision={revision}",
        f"file={EXECUTABLE_NAME}",
        f"sha256={sha256(executable)}",
        f"size={executable.stat().st_size}",
    ]
    for archive_name, document in documents:
        lines.append(f"document={archive_name}:{sha256(document)}")
    return "\n".join(lines) + "\n"


def build_package(source_root: Path, executable: Path, output_directory: Path) -> tuple[Path, Path]:
    source_root = source_root.resolve()
    executable = executable.resolve()
    output_directory = output_directory.resolve()
    errors = validate_sources(source_root, executable)
    if errors:
        raise ValueError("\n".join(errors))

    output_directory.mkdir(parents=True, exist_ok=True)
    package_path = output_directory / f"{PACKAGE_NAME}.zip"
    hash_path = output_directory / f"{PACKAGE_NAME}.zip.sha256"
    revision = git_revision(source_root)
    document_sources = [
        (archive_name, source_root / source_name)
        for source_name, archive_name in DOCUMENTS
    ]
    manifest = manifest_lines(revision, executable, document_sources)

    with tempfile.TemporaryDirectory(dir=output_directory) as temporary:
        staged_root = Path(temporary) / PACKAGE_NAME
        staged_root.mkdir()
        shutil.copy2(executable, staged_root / EXECUTABLE_NAME)
        for archive_name, source in document_sources:
            shutil.copy2(source, staged_root / archive_name)
        (staged_root / "TEST-MANIFEST.txt").write_text(manifest, encoding="utf-8", newline="\n")

        temporary_zip = Path(temporary) / package_path.name
        with zipfile.ZipFile(temporary_zip, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
            for path in sorted(staged_root.rglob("*")):
                if path.is_file():
                    archive.write(path, path.relative_to(staged_root.parent).as_posix())
        shutil.move(temporary_zip, package_path)

    hash_path.write_text(
        f"{sha256(package_path)}  {package_path.name}\n", encoding="ascii", newline="\n"
    )
    return package_path, hash_path


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--source-root", type=Path, default=Path(__file__).resolve().parents[1]
    )
    parser.add_argument(
        "--executable", type=Path, required=True, help="Verified Windows executable to package."
    )
    parser.add_argument("--output-directory", type=Path, default=Path("dist"))
    return parser


def main(argv: Sequence[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    source_root = args.source_root.resolve()
    output_directory = args.output_directory
    if not output_directory.is_absolute():
        output_directory = source_root / output_directory
    try:
        package_path, hash_path = build_package(
            source_root, args.executable, output_directory
        )
    except (OSError, ValueError) as exc:
        print(f"test-release-package: ERROR {exc}", file=sys.stderr)
        return 1
    print(f"test-release-package: PASS {package_path}")
    print(f"test-release-package: SHA256 {hash_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
