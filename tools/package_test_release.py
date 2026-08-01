#!/usr/bin/env python3
"""Create deterministic public-test and detached-symbol ZIP archives."""

from __future__ import annotations

import argparse
from datetime import datetime, timezone
import hashlib
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile
from typing import Iterable, Sequence
import zipfile


VERSION = "0.1.0-test"
DEV_VERSION = "0.2.0-dev"
AUTOMATION_VERSION = "0.3.0-dev"
PREVIOUS_PRIVATE_TEST_VERSION = "0.6.0-dev"
PRIVATE_TEST_VERSION = "0.7.0-dev"
PACKAGE_STEM = f"TPT-ZH-OmniPack-{VERSION}-Windows-x64"
SYMBOL_PACKAGE_STEM = f"TPT-ZH-OmniPack-{VERSION}-Symbols-Windows-x64"
EXECUTABLE_NAME = "tpt-zh-omnipack.exe"
SYMBOL_NAME = "tpt-zh-omnipack.debug"
DOCUMENTS = (
    ("LICENSE", "LICENSE"),
    ("README.zh-CN.md", "README.zh-CN.md"),
    ("CHANGELOG.zh-CN.md", "CHANGELOG.zh-CN.md"),
    ("docs/TEST_RELEASE.md", "TESTING.zh-CN.md"),
    ("docs/THIRD_PARTY_SOURCES.md", "SOURCE-AND-LICENSES.zh-CN.md"),
    ("docs/KNOWN_ISSUES.md", "KNOWN-ISSUES.zh-CN.md"),
    ("docs/AI_DISCLOSURE.md", "AI-DISCLOSURE.zh-CN.md"),
    ("docs/FONT_AUDIT.md", "FONT-AUDIT.md"),
    ("resources/third_party/GNU_UNIFONT_COPYING.txt", "LICENSES/GNU-UNIFONT-OFL-1.1.txt"),
    ("resources/third_party/FUSION_PIXEL_FONT_OFL-1.1.txt", "LICENSES/FUSION-PIXEL-FONT-OFL-1.1.txt"),
    ("resources/third_party/FUSION_PIXEL_FONT_ARK_PIXEL_OFL-1.1.txt", "LICENSES/FUSION-PIXEL-FONT-ARK-PIXEL-OFL-1.1.txt"),
    ("resources/third_party/FUSION_PIXEL_FONT_CUBIC_11_OFL-1.1.txt", "LICENSES/FUSION-PIXEL-FONT-CUBIC-11-OFL-1.1.txt"),
    ("resources/third_party/FUSION_PIXEL_FONT_GALMURI_OFL-1.1.txt", "LICENSES/FUSION-PIXEL-FONT-GALMURI-OFL-1.1.txt"),
)
DEV_DOCUMENTS = (
    ("docs/TUTORIALS_0.2.json", "TUTORIALS-0.2.0.json"),
    ("examples/0.2.0/example-spec.json", "examples/0.2.0/example-spec.json"),
    ("examples/0.2.0/manifest.json", "examples/0.2.0/manifest.json"),
    ("examples/0.2.0/tutorials-runtime-report.json", "examples/0.2.0/tutorials-runtime-report.json"),
    *tuple(
        (
            f"examples/0.2.0/{ordinal:02d}-{name}.stm",
            f"examples/0.2.0/{ordinal:02d}-{name}.stm",
        )
        for ordinal, name in enumerate(
            (
                "peroxide-pathogen",
                "humus-fertilizer",
                "slag-acid",
                "shield-assembly",
                "waste-stabilization",
                "waste-missing-catalyst",
                "integrated-recovery",
            ),
            start=1,
        )
    ),
)
AUTOMATION_ONLY_DOCUMENTS = (
    ("docs/AUTOMATION_CAPABILITIES.csv", "AUTOMATION-CAPABILITIES.csv"),
    ("automation/0.3.0/scenario-spec.json", "automation/0.3.0/scenario-spec.json"),
    (
        "automation/0.3.0/official-source-sha256.json",
        "automation/0.3.0/official-source-sha256.json",
    ),
    ("examples/0.3.0/manifest.json", "examples/0.3.0/manifest.json"),
    ("examples/0.3.0/runtime-report.json", "examples/0.3.0/runtime-report.json"),
    *tuple(
        (
            f"examples/0.3.0/{ordinal:02d}-{name}.stm",
            f"examples/0.3.0/{ordinal:02d}-{name}.stm",
        )
        for ordinal, name in enumerate(
            (
                "thermostatic-furnace",
                "automatic-alloy",
                "fuel-control",
                "nutrient-dosing",
                "pathogen-disinfection",
                "reactor-cooling",
                "emergency-stop",
                "waste-transfer",
                "integrated-factory",
            ),
            start=1,
        )
    ),
)
AUTOMATION_DOCUMENTS = DEV_DOCUMENTS + AUTOMATION_ONLY_DOCUMENTS
PRIVATE_TEST_INSTRUCTIONS = {
    PREVIOUS_PRIVATE_TEST_VERSION: (
        "docs/PRIVATE_TEST_0.6.0.md",
        "TESTING.zh-CN.md",
    ),
    PRIVATE_TEST_VERSION: (
        "docs/PRIVATE_TEST_0.7.0.md",
        "TESTING.zh-CN.md",
    ),
}
FORBIDDEN_SUFFIXES = (".cps", ".stm", ".pref", ".lua", ".o", ".obj", ".pdb", ".dmp")
FORBIDDEN_COMPONENTS = {".git", "__pycache__", "build", "dist"}
CAN_INSTALL_DEFAULT_RE = re.compile(
    r"option\(\s*['\"]can_install['\"].*?value\s*:\s*['\"]([^'\"]+)['\"]",
    re.DOTALL,
)


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest().upper()


def git_revision(source_root: Path) -> str:
    completed = subprocess.run(
        ["git", "rev-parse", "HEAD"], cwd=source_root, check=False,
        capture_output=True, text=True, encoding="utf-8", errors="replace",
    )
    if completed.returncode:
        raise ValueError(f"cannot determine Git revision: {completed.stderr.strip()}")
    revision = completed.stdout.strip()
    if len(revision) != 40:
        raise ValueError(f"unexpected Git revision: {revision!r}")
    return revision


def source_date_epoch() -> int:
    value = os.environ.get("SOURCE_DATE_EPOCH")
    if value is None:
        return 315532800  # 1980-01-01, the earliest ZIP timestamp.
    try:
        epoch = int(value)
    except ValueError as exc:
        raise ValueError("SOURCE_DATE_EPOCH must be an integer") from exc
    if epoch < 315532800:
        raise ValueError("SOURCE_DATE_EPOCH must be on or after 1980-01-01")
    return epoch


def zip_datetime(epoch: int) -> tuple[int, int, int, int, int, int]:
    timestamp = datetime.fromtimestamp(epoch, tz=timezone.utc)
    return timestamp.year, timestamp.month, timestamp.day, timestamp.hour, timestamp.minute, timestamp.second


def validate_profile(version: str, kind: str, include_examples: bool) -> None:
    expected = {
        VERSION: ("public-test", False),
        DEV_VERSION: ("local-dev", True),
        AUTOMATION_VERSION: ("local-dev", True),
        **{
            private_version: ("local-dev", False)
            for private_version in PRIVATE_TEST_INSTRUCTIONS
        },
    }
    if version not in expected:
        raise ValueError(f"unsupported package version: {version}")
    expected_kind, expected_examples = expected[version]
    if kind != expected_kind or include_examples != expected_examples:
        raise ValueError(
            f"version {version} requires kind={expected_kind} "
            f"and include_examples={str(expected_examples).lower()}"
        )


def package_stems(version: str) -> tuple[str, str]:
    return (
        f"TPT-ZH-OmniPack-{version}-Windows-x64",
        f"TPT-ZH-OmniPack-{version}-Symbols-Windows-x64",
    )


def development_documents(version: str) -> tuple[tuple[str, str], ...]:
    if version == DEV_VERSION:
        return DEV_DOCUMENTS
    if version == AUTOMATION_VERSION:
        return AUTOMATION_DOCUMENTS
    return ()


def package_documents(version: str) -> tuple[tuple[str, str], ...]:
    if version not in PRIVATE_TEST_INSTRUCTIONS:
        return DOCUMENTS
    private_source, private_archive = PRIVATE_TEST_INSTRUCTIONS[version]
    return tuple(
        (source, archive)
        for source, archive in DOCUMENTS
        if archive != private_archive
    ) + ((private_source, private_archive),)


def validate_member_name(name: str, allow_example_stamp: bool = False) -> None:
    path = Path(name)
    if path.is_absolute() or ".." in path.parts or any(component in FORBIDDEN_COMPONENTS for component in path.parts):
        raise ValueError(f"unsafe archive member name: {name}")
    allowed_stamp = (
        allow_example_stamp
        and (
            name.startswith("examples/0.2.0/")
            or name.startswith("examples/0.3.0/")
        )
        and name.lower().endswith(".stm")
    )
    if name.lower().endswith(FORBIDDEN_SUFFIXES) and not allowed_stamp:
        raise ValueError(f"forbidden archive member name: {name}")


def validate_sources(
    source_root: Path,
    executable: Path,
    symbols: Path,
    version: str = VERSION,
    kind: str = "public-test",
    include_examples: bool = False,
) -> None:
    validate_profile(version, kind, include_examples)
    if executable.name != EXECUTABLE_NAME:
        raise ValueError(f"executable must be named {EXECUTABLE_NAME!r}, got {executable.name!r}")
    if not executable.is_file() or executable.read_bytes()[:2] != b"MZ":
        raise ValueError(f"executable is not a Windows PE file: {executable}")
    if not symbols.is_file() or symbols.stat().st_size == 0:
        raise ValueError(f"debug symbol file is missing or empty: {symbols}")
    options_path = source_root / "meson_options.txt"
    if not options_path.is_file():
        raise ValueError(f"Meson options are missing: {options_path}")
    match = CAN_INSTALL_DEFAULT_RE.search(options_path.read_text(encoding="utf-8"))
    if match is None or match.group(1) != "no":
        actual = match.group(1) if match else "missing"
        raise ValueError(
            "portable release must default can_install=no to avoid a first-run "
            f"association prompt (got {actual})"
        )
    for source_name, archive_name in package_documents(version):
        validate_member_name(archive_name)
        source = source_root / source_name
        if not source.is_file():
            raise ValueError(f"required public-release source is missing: {source}")
    if include_examples:
        for source_name, archive_name in development_documents(version):
            validate_member_name(archive_name, allow_example_stamp=True)
            source = source_root / source_name
            if not source.is_file() or source.stat().st_size == 0:
                raise ValueError(f"required local-dev source is missing or empty: {source}")


def manifest(
    revision: str,
    members: Iterable[tuple[str, Path]],
    build_epoch: int,
    kind: str,
    version: str = VERSION,
) -> str:
    lines = [
        "format=2",
        f"kind={kind}",
        f"version={version}",
        f"revision={revision}",
        f"build_epoch={build_epoch}",
    ]
    for name, path in members:
        lines.append(f"member={name}|{path.stat().st_size}|{sha256(path)}")
    return "\n".join(lines) + "\n"


def write_zip(path: Path, root: str, files: Iterable[tuple[str, Path]], manifest_text: str, epoch: int) -> None:
    timestamp = zip_datetime(epoch)
    with zipfile.ZipFile(path, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=9, strict_timestamps=True) as archive:
        for name, source in sorted(files):
            info = zipfile.ZipInfo(f"{root}/{name}", date_time=timestamp)
            info.compress_type = zipfile.ZIP_DEFLATED
            info.external_attr = 0o100644 << 16
            archive.writestr(info, source.read_bytes())
        info = zipfile.ZipInfo(f"{root}/TEST-MANIFEST.txt", date_time=timestamp)
        info.compress_type = zipfile.ZIP_DEFLATED
        info.external_attr = 0o100644 << 16
        archive.writestr(info, manifest_text.encode("utf-8"))


def write_hash(path: Path) -> Path:
    hash_path = path.with_suffix(path.suffix + ".sha256")
    hash_path.write_text(f"{sha256(path)}  {path.name}\n", encoding="ascii", newline="\n")
    return hash_path


def build_package(
    source_root: Path,
    executable: Path,
    symbols: Path,
    output_directory: Path,
    version: str = VERSION,
    kind: str = "public-test",
    include_examples: bool = False,
) -> tuple[Path, Path, Path, Path]:
    source_root = source_root.resolve()
    executable = executable.resolve()
    symbols = symbols.resolve()
    output_directory = output_directory.resolve()
    validate_sources(
        source_root,
        executable,
        symbols,
        version=version,
        kind=kind,
        include_examples=include_examples,
    )
    revision = git_revision(source_root)
    epoch = source_date_epoch()
    output_directory.mkdir(parents=True, exist_ok=True)
    package_stem, symbol_package_stem = package_stems(version)
    selected_documents = package_documents(version) + (
        development_documents(version) if include_examples else ()
    )
    normal_files = [(EXECUTABLE_NAME, executable)] + [
        (name, source_root / source) for source, name in selected_documents
    ]
    symbol_files = [(SYMBOL_NAME, symbols)]
    package_path = output_directory / f"{package_stem}.zip"
    symbols_path = output_directory / f"{symbol_package_stem}.zip"
    with tempfile.TemporaryDirectory(dir=output_directory) as temporary:
        temporary = Path(temporary)
        normal_temp = temporary / package_path.name
        symbols_temp = temporary / symbols_path.name
        write_zip(
            normal_temp,
            package_stem,
            normal_files,
            manifest(revision, normal_files, epoch, kind, version),
            epoch,
        )
        write_zip(
            symbols_temp,
            symbol_package_stem,
            symbol_files,
            manifest(revision, symbol_files, epoch, "debug-symbols", version),
            epoch,
        )
        shutil.move(normal_temp, package_path)
        shutil.move(symbols_temp, symbols_path)
    return package_path, write_hash(package_path), symbols_path, write_hash(symbols_path)


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--executable", type=Path, required=True)
    parser.add_argument("--symbols", type=Path, required=True)
    parser.add_argument("--output-directory", type=Path, default=Path("dist"))
    parser.add_argument(
        "--version",
        choices=(
            VERSION,
            DEV_VERSION,
            AUTOMATION_VERSION,
            *PRIVATE_TEST_INSTRUCTIONS,
        ),
        default=VERSION,
    )
    parser.add_argument("--kind", choices=("public-test", "local-dev"), default="public-test")
    parser.add_argument("--include-examples", action="store_true")
    parser.add_argument("--objdump", help="Path to objdump for mandatory PE auditing.")
    parser.add_argument("--strings", help="Path to strings for mandatory path auditing.")
    return parser


def main(argv: Sequence[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    source_root = args.source_root.resolve()
    output_directory = args.output_directory if args.output_directory.is_absolute() else source_root / args.output_directory
    try:
        audit_command = [
            sys.executable,
            str(Path(__file__).with_name("release_binary_audit.py")),
            "--executable",
            str(args.executable),
            "--symbols",
            str(args.symbols),
        ]
        if args.objdump:
            audit_command.extend(["--objdump", args.objdump])
        if args.strings:
            audit_command.extend(["--strings", args.strings])
        audit = subprocess.run(audit_command, check=False, capture_output=True, text=True)
        if audit.returncode:
            raise ValueError("release binary audit failed: " + audit.stderr.strip())
        package, package_hash, symbols, symbols_hash = build_package(
            source_root,
            args.executable,
            args.symbols,
            output_directory,
            version=args.version,
            kind=args.kind,
            include_examples=args.include_examples,
        )
    except (OSError, ValueError) as exc:
        print(f"test-release-package: ERROR {exc}", file=sys.stderr)
        return 1
    print(f"test-release-package: PASS {package}")
    print(f"test-release-package: SHA256 {package_hash}")
    print(f"test-release-package: SYMBOLS {symbols}")
    print(f"test-release-package: SYMBOLS-SHA256 {symbols_hash}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
