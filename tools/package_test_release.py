#!/usr/bin/env python3
"""Create deterministic test, release-candidate, release, and symbol ZIP archives."""

from __future__ import annotations

import argparse
from datetime import datetime, timezone
import hashlib
import mmap
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile
from typing import Iterable, Sequence
import zipfile


TOOLS_DIRECTORY = Path(__file__).resolve().parent
if str(TOOLS_DIRECTORY) not in sys.path:
    sys.path.insert(0, str(TOOLS_DIRECTORY))
from source_snapshot import snapshot as full_worktree_snapshot


VERSION = "0.1.0-test"
DEV_VERSION = "0.2.0-dev"
AUTOMATION_VERSION = "0.3.0-dev"
PREVIOUS_PRIVATE_TEST_VERSION = "0.6.0-dev"
PRIVATE_TEST_VERSION = "0.7.0-dev"
RELEASE_CANDIDATE_VERSION = "1.0.0-rc9"
FINAL_VERSION = "1.0.0"
STABLE_VERSION = "1.1.0"
RELEASE_CANDIDATE_1_1_0_VERSION = "1.1.0-rc1"
PACKAGE_STEM = f"TPT-ZH-OmniPack-{VERSION}-Windows-x64"
SYMBOL_PACKAGE_STEM = f"TPT-ZH-OmniPack-{VERSION}-Symbols-Windows-x64"
EXECUTABLE_NAME = "tpt-zh-omnipack.exe"
SYMBOL_NAME = "tpt-zh-omnipack.debug"
PREBUILT_LICENSE_ROOT = (
    "subprojects/tpt-libs-prebuilt-x86_64-windows-mingw-static-release-"
    "v20251019131007/licenses"
)
RELEASE_BUILD_INPUT_WRAPS = (
    "subprojects/tpt-libs-prebuilt-x86_64-windows-mingw-static-release-"
    "v20251019131007.wrap",
)
LIBRARY_LICENSE_DOCUMENTS = tuple(
    (
        f"{PREBUILT_LICENSE_ROOT}/{name}.LICENSE",
        f"LICENSES/LIBRARIES/{name}.LICENSE.txt",
    )
    for name in (
        "bzip2",
        "fftw3f",
        "jsoncpp",
        "libcurl",
        "libpng",
        "lua5.1",
        "lua5.2",
        "luajit",
        "mbedtls",
        "nghttp2",
        "sdl2",
        "zlib",
    )
)
SDL3_LICENSE_DOCUMENT = (
    "resources/third_party/SDL3_ZLIB_LICENSE.txt",
    "LICENSES/LIBRARIES/sdl3.LICENSE.txt",
)
DOCUMENTS = (
    ("LICENSE", "LICENSE"),
    ("README.md", "README.en.md"),
    ("README.zh-CN.md", "README.zh-CN.md"),
    ("changelog.txt", "CHANGELOG.en.txt"),
    ("CHANGELOG.zh-CN.md", "CHANGELOG.zh-CN.md"),
    ("docs/TEST_RELEASE.md", "TESTING.zh-CN.md"),
    ("docs/THIRD_PARTY_SOURCES.md", "SOURCE-AND-LICENSES.zh-CN.md"),
    ("docs/KNOWN_ISSUES.md", "KNOWN-ISSUES.zh-CN.md"),
    ("docs/AI_DISCLOSURE.md", "AI-DISCLOSURE.zh-CN.md"),
    ("docs/FONT_AUDIT.md", "FONT-AUDIT.md"),
    ("docs/THIRD_PARTY_LICENSE_MANIFEST.csv", "LICENSES/THIRD-PARTY-MANIFEST.csv"),
    ("resources/third_party/GNU_UNIFONT_COPYING.txt", "LICENSES/GNU-UNIFONT-OFL-1.1.txt"),
    ("resources/third_party/FUSION_PIXEL_FONT_OFL-1.1.txt", "LICENSES/FUSION-PIXEL-FONT-OFL-1.1.txt"),
    ("resources/third_party/FUSION_PIXEL_FONT_ARK_PIXEL_OFL-1.1.txt", "LICENSES/FUSION-PIXEL-FONT-ARK-PIXEL-OFL-1.1.txt"),
    ("resources/third_party/FUSION_PIXEL_FONT_CUBIC_11_OFL-1.1.txt", "LICENSES/FUSION-PIXEL-FONT-CUBIC-11-OFL-1.1.txt"),
    ("resources/third_party/FUSION_PIXEL_FONT_GALMURI_OFL-1.1.txt", "LICENSES/FUSION-PIXEL-FONT-GALMURI-OFL-1.1.txt"),
    ("resources/third_party/OPENSTAX_CHEMISTRY_CC-BY-4.0.txt", "LICENSES/OPENSTAX-CHEMISTRY-CC-BY-4.0.txt"),
    *LIBRARY_LICENSE_DOCUMENTS,
    SDL3_LICENSE_DOCUMENT,
)
FINAL_DOCUMENTS = (
    ("LICENSE", "LICENSE"),
    ("docs/RELEASE_1.0.0_README.en.md", "README.en.md"),
    ("docs/RELEASE_1.0.0_README.zh-CN.md", "README.zh-CN.md"),
    ("docs/RELEASE_1.0.0_CHANGELOG.en.txt", "CHANGELOG.en.txt"),
    ("docs/RELEASE_1.0.0_CHANGELOG.zh-CN.md", "CHANGELOG.zh-CN.md"),
    ("docs/THIRD_PARTY_SOURCES.md", "SOURCE-AND-LICENSES.zh-CN.md"),
    ("docs/AI_DISCLOSURE.md", "AI-DISCLOSURE.zh-CN.md"),
    ("docs/THIRD_PARTY_LICENSE_MANIFEST.csv", "LICENSES/THIRD-PARTY-MANIFEST.csv"),
    ("resources/third_party/GNU_UNIFONT_COPYING.txt", "LICENSES/GNU-UNIFONT-OFL-1.1.txt"),
    ("resources/third_party/FUSION_PIXEL_FONT_OFL-1.1.txt", "LICENSES/FUSION-PIXEL-FONT-OFL-1.1.txt"),
    ("resources/third_party/FUSION_PIXEL_FONT_ARK_PIXEL_OFL-1.1.txt", "LICENSES/FUSION-PIXEL-FONT-ARK-PIXEL-OFL-1.1.txt"),
    ("resources/third_party/FUSION_PIXEL_FONT_CUBIC_11_OFL-1.1.txt", "LICENSES/FUSION-PIXEL-FONT-CUBIC-11-OFL-1.1.txt"),
    ("resources/third_party/FUSION_PIXEL_FONT_GALMURI_OFL-1.1.txt", "LICENSES/FUSION-PIXEL-FONT-GALMURI-OFL-1.1.txt"),
    ("resources/third_party/OPENSTAX_CHEMISTRY_CC-BY-4.0.txt", "LICENSES/OPENSTAX-CHEMISTRY-CC-BY-4.0.txt"),
    *LIBRARY_LICENSE_DOCUMENTS,
    SDL3_LICENSE_DOCUMENT,
)
LEGACY_STABLE_DOCUMENTS = (
    ("LICENSE", "LICENSE"),
    ("docs/RELEASE_1.1.0_README.en.md", "README.en.md"),
    ("docs/RELEASE_1.1.0_README.zh-CN.md", "README.zh-CN.md"),
    ("docs/RELEASE_1.1.0_CHANGELOG.en.txt", "CHANGELOG.en.txt"),
    ("docs/RELEASE_1.1.0_CHANGELOG.zh-CN.md", "CHANGELOG.zh-CN.md"),
    ("docs/THIRD_PARTY_SOURCES.md", "SOURCE-AND-LICENSES.zh-CN.md"),
    ("docs/AI_DISCLOSURE.md", "AI-DISCLOSURE.zh-CN.md"),
    ("docs/THIRD_PARTY_LICENSE_MANIFEST.csv", "LICENSES/THIRD-PARTY-MANIFEST.csv"),
    ("resources/third_party/GNU_UNIFONT_COPYING.txt", "LICENSES/GNU-UNIFONT-OFL-1.1.txt"),
    ("resources/third_party/FUSION_PIXEL_FONT_OFL-1.1.txt", "LICENSES/FUSION-PIXEL-FONT-OFL-1.1.txt"),
    ("resources/third_party/FUSION_PIXEL_FONT_ARK_PIXEL_OFL-1.1.txt", "LICENSES/FUSION-PIXEL-FONT-ARK-PIXEL-OFL-1.1.txt"),
    ("resources/third_party/FUSION_PIXEL_FONT_CUBIC_11_OFL-1.1.txt", "LICENSES/FUSION-PIXEL-FONT-CUBIC-11-OFL-1.1.txt"),
    ("resources/third_party/FUSION_PIXEL_FONT_GALMURI_OFL-1.1.txt", "LICENSES/FUSION-PIXEL-FONT-GALMURI-OFL-1.1.txt"),
    ("resources/third_party/OPENSTAX_CHEMISTRY_CC-BY-4.0.txt", "LICENSES/OPENSTAX-CHEMISTRY-CC-BY-4.0.txt"),
    *LIBRARY_LICENSE_DOCUMENTS,
    SDL3_LICENSE_DOCUMENT,
)
ONE_ONE_COMMON_DOCUMENTS = (
    ("LICENSE", "LICENSE"),
    ("docs/THIRD_PARTY_SOURCES.md", "SOURCE-AND-LICENSES.zh-CN.md"),
    ("docs/AI_DISCLOSURE.md", "AI-DISCLOSURE.zh-CN.md"),
    (
        "docs/THIRD_PARTY_LICENSE_MANIFEST.csv",
        "THIRD-PARTY-LICENSES/THIRD-PARTY-MANIFEST.csv",
    ),
    *tuple(
        (
            source,
            "THIRD-PARTY-LICENSES/" + destination.removeprefix("LICENSES/"),
        )
        for source, destination in DOCUMENTS
        if destination.startswith("LICENSES/")
        and source != "docs/THIRD_PARTY_LICENSE_MANIFEST.csv"
    ),
)
ONE_ONE_RC_DOCUMENTS = ONE_ONE_COMMON_DOCUMENTS + (
    ("docs/RELEASE_1.1.0_RC.md", "RELEASE-CANDIDATE.md"),
)
ONE_ONE_STABLE_DOCUMENTS = ONE_ONE_COMMON_DOCUMENTS + (
    ("docs/RELEASE_1.1.0_README.en.md", "README.en.md"),
    ("docs/RELEASE_1.1.0_README.zh-CN.md", "README.zh-CN.md"),
    ("docs/RELEASE_1.1.0_CHANGELOG.en.txt", "CHANGELOG.en.txt"),
    ("docs/RELEASE_1.1.0_CHANGELOG.zh-CN.md", "CHANGELOG.zh-CN.md"),
)
ONE_ONE_RC_RELEASE_EXTRA_MEMBERS = frozenset({
    "BUILD-INFO.txt", "RELEASE-VALIDATION.txt", "RELEASE-VALIDATION.json",
})
ONE_ONE_STABLE_RELEASE_EXTRA_MEMBERS = frozenset({"BUILD-INFO.txt"})
ONE_ONE_PACKAGE_MANIFEST = "PACKAGE-MANIFEST.sha256"
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
RELEASE_CANDIDATE_INSTRUCTIONS = {
    RELEASE_CANDIDATE_VERSION: (
        "docs/RELEASE_CANDIDATE_1.0.0.md",
        "TESTING.zh-CN.md",
    ),
}
VERSIONED_INSTRUCTIONS = {
    **PRIVATE_TEST_INSTRUCTIONS,
    **RELEASE_CANDIDATE_INSTRUCTIONS,
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


def git_worktree_provenance(
    source_root: Path, required_wraps: Sequence[str] = (),
) -> dict[str, str]:
    """Return the full byte snapshot also recorded by the release driver."""
    value = full_worktree_snapshot(source_root, required_wraps)
    return {
        "source_state": str(value["source_state"]),
        "source_worktree_sha256": str(value["source_worktree_sha256"]),
        "source_untracked_files": str(value["source_untracked_files"]),
        "build_inputs_sha256": str(value["build_inputs_sha256"]),
        "build_inputs_ready": str(bool(value["build_inputs_ready"])).lower(),
    }


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
        RELEASE_CANDIDATE_VERSION: ("release-candidate", False),
        FINAL_VERSION: ("release", False),
        RELEASE_CANDIDATE_1_1_0_VERSION: ("release-candidate", False),
        STABLE_VERSION: ("release", False),
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


def validate_artifact_stem(stem: str, label: str) -> str:
    if not re.fullmatch(r"[A-Za-z0-9][A-Za-z0-9._-]*", stem):
        raise ValueError(f"invalid {label}: {stem!r}")
    return stem


def package_stems(
    version: str,
    artifact_stem: str | None = None,
    symbol_artifact_stem: str | None = None,
) -> tuple[str, str]:
    if version == STABLE_VERSION:
        if artifact_stem is None or symbol_artifact_stem is None:
            raise ValueError(
                "stable packaging requires explicit run-bound staging artifact stems; "
                "only the finalizer may create stable filenames"
            )
        candidate_match = re.fullmatch(
            r"TPT-ZH-OmniPack-1\.1\.0-staging-"
            r"([0-9]{8}T[0-9]{6}Z-[0-9a-f]{8})-Windows-x64-SDL3",
            artifact_stem,
        )
        symbols_match = re.fullmatch(
            r"TPT-ZH-OmniPack-1\.1\.0-staging-"
            r"([0-9]{8}T[0-9]{6}Z-[0-9a-f]{8})-Windows-x64-Symbols",
            symbol_artifact_stem,
        )
        if (
            candidate_match is None
            or symbols_match is None
            or candidate_match.group(1) != symbols_match.group(1)
        ):
            raise ValueError(
                "stable package stems must be matching staging names bound to one RUN_ID"
            )
        defaults = (artifact_stem, symbol_artifact_stem)
    elif version == RELEASE_CANDIDATE_1_1_0_VERSION:
        defaults = (
            f"TPT-ZH-OmniPack-{version}-Windows-x64-SDL3",
            f"TPT-ZH-OmniPack-{version}-Windows-x64-Symbols",
        )
    else:
        defaults = (
            f"TPT-ZH-OmniPack-{version}-Windows-x64",
            f"TPT-ZH-OmniPack-{version}-Symbols-Windows-x64",
        )
    package_stem = artifact_stem or defaults[0]
    symbol_stem = symbol_artifact_stem or defaults[1]
    return (
        validate_artifact_stem(package_stem, "artifact stem"),
        validate_artifact_stem(symbol_stem, "symbol artifact stem"),
    )


def development_documents(version: str) -> tuple[tuple[str, str], ...]:
    if version == DEV_VERSION:
        return DEV_DOCUMENTS
    if version == AUTOMATION_VERSION:
        return AUTOMATION_DOCUMENTS
    return ()


def package_documents(version: str) -> tuple[tuple[str, str], ...]:
    if version == STABLE_VERSION:
        return ONE_ONE_STABLE_DOCUMENTS
    if version == FINAL_VERSION:
        return FINAL_DOCUMENTS
    if version == RELEASE_CANDIDATE_1_1_0_VERSION:
        return ONE_ONE_RC_DOCUMENTS
    if version not in VERSIONED_INSTRUCTIONS:
        return DOCUMENTS
    private_source, private_archive = VERSIONED_INSTRUCTIONS[version]
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
    extra_members: Sequence[tuple[str, Path]] = (),
) -> None:
    validate_profile(version, kind, include_examples)
    if executable.name != EXECUTABLE_NAME:
        raise ValueError(f"executable must be named {EXECUTABLE_NAME!r}, got {executable.name!r}")
    if not executable.is_file():
        raise ValueError(f"executable is not a Windows PE file: {executable}")
    with executable.open("rb") as executable_file:
        if executable_file.read(2) != b"MZ":
            raise ValueError(f"executable is not a Windows PE file: {executable}")
        executable_file.seek(0)
        with mmap.mmap(executable_file.fileno(), 0, access=mmap.ACCESS_READ) as image:
            release_markers = (
                version.encode("ascii"),
                version.encode("utf-16le"),
            )
            if any(image.find(marker) < 0 for marker in release_markers):
                raise ValueError(
                    "executable release label does not match requested package "
                    f"version {version}: {executable}"
                )
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
    extra_names: set[str] = set()
    for archive_name, source in extra_members:
        validate_member_name(archive_name)
        if archive_name in extra_names:
            raise ValueError(f"release package repeats an extra member: {archive_name}")
        extra_names.add(archive_name)
        if not source.is_file() or source.stat().st_size == 0:
            raise ValueError(f"release package extra member is missing or empty: {source}")
    if version in {RELEASE_CANDIDATE_1_1_0_VERSION, STABLE_VERSION}:
        required_extras = (
            ONE_ONE_STABLE_RELEASE_EXTRA_MEMBERS
            if version == STABLE_VERSION
            else ONE_ONE_RC_RELEASE_EXTRA_MEMBERS
        )
        if extra_names != required_extras:
            raise ValueError(
                "1.1.0 package extras must be exactly: "
                + ", ".join(sorted(required_extras))
            )
    elif extra_names:
        raise ValueError("only 1.1.0 release packages may include extra members")


def file_bytes(source: Path | bytes) -> bytes:
    return source if isinstance(source, bytes) else source.read_bytes()


def package_file_manifest(members: Iterable[tuple[str, Path | bytes]]) -> bytes:
    return ("\n".join(
        f"{sha256_bytes(file_bytes(source))}  {name}"
        for name, source in sorted(members)
    ) + "\n").encode("utf-8")


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest().upper()


def manifest(
    revision: str,
    members: Iterable[tuple[str, Path | bytes]],
    build_epoch: int,
    kind: str,
    version: str = VERSION,
    source_provenance: dict[str, str] | None = None,
) -> str:
    lines = [
        "format=2",
        f"kind={kind}",
        f"version={version}",
        f"revision={revision}",
        f"build_epoch={build_epoch}",
    ]
    if source_provenance is not None:
        provenance_fields = (
            "source_state",
            "source_worktree_sha256",
            "source_untracked_files",
            "build_inputs_sha256",
            "build_inputs_ready",
        )
        lines.extend(
            f"{key}={source_provenance[key]}"
            for key in provenance_fields
            if key in source_provenance
        )
    for name, source in members:
        data = file_bytes(source)
        lines.append(f"member={name}|{len(data)}|{sha256_bytes(data)}")
    return "\n".join(lines) + "\n"


def manifest_name(version: str) -> str:
    return "MANIFEST.txt" if version in {FINAL_VERSION, STABLE_VERSION} else "TEST-MANIFEST.txt"


def write_zip(
    path: Path,
    root: str,
    files: Iterable[tuple[str, Path | bytes]],
    manifest_text: str,
    epoch: int,
    manifest_filename: str = "TEST-MANIFEST.txt",
) -> None:
    timestamp = zip_datetime(epoch)
    with zipfile.ZipFile(path, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=9, strict_timestamps=True) as archive:
        for name, source in sorted(files):
            info = zipfile.ZipInfo(f"{root}/{name}", date_time=timestamp)
            info.compress_type = zipfile.ZIP_DEFLATED
            info.external_attr = 0o100644 << 16
            archive.writestr(info, file_bytes(source))
        info = zipfile.ZipInfo(f"{root}/{manifest_filename}", date_time=timestamp)
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
    source_provenance: dict[str, str] | None = None,
    extra_members: Sequence[tuple[str, Path]] = (),
    artifact_stem: str | None = None,
    symbol_artifact_stem: str | None = None,
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
        extra_members=extra_members,
    )
    revision = git_revision(source_root)
    epoch = source_date_epoch()
    output_directory.mkdir(parents=True, exist_ok=True)
    package_stem, symbol_package_stem = package_stems(
        version, artifact_stem, symbol_artifact_stem
    )
    selected_documents = package_documents(version) + (
        development_documents(version) if include_examples else ()
    )
    archive_manifest_name = manifest_name(version)
    normal_files: list[tuple[str, Path | bytes]] = [(EXECUTABLE_NAME, executable)] + [
        (name, source_root / source) for source, name in selected_documents
    ] + list(extra_members)
    if version in {RELEASE_CANDIDATE_1_1_0_VERSION, STABLE_VERSION}:
        normal_files.append(
            (ONE_ONE_PACKAGE_MANIFEST, package_file_manifest(normal_files))
        )
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
            manifest(
                revision, normal_files, epoch, kind, version,
                source_provenance=source_provenance,
            ),
            epoch,
            archive_manifest_name,
        )
        write_zip(
            symbols_temp,
            symbol_package_stem,
            symbol_files,
            manifest(
                revision, symbol_files, epoch, "debug-symbols", version,
                source_provenance=source_provenance,
            ),
            epoch,
            archive_manifest_name,
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
            *VERSIONED_INSTRUCTIONS,
            FINAL_VERSION,
            RELEASE_CANDIDATE_1_1_0_VERSION,
            STABLE_VERSION,
        ),
        default=VERSION,
    )
    parser.add_argument(
        "--kind",
        choices=("public-test", "local-dev", "release-candidate", "release"),
        default="public-test",
    )
    parser.add_argument("--include-examples", action="store_true")
    parser.add_argument(
        "--artifact-stem",
        help="Override the ZIP filename and root directory stem without changing the embedded version.",
    )
    parser.add_argument(
        "--symbol-artifact-stem",
        help="Override the symbol ZIP filename and root directory stem without changing the embedded version.",
    )
    parser.add_argument(
        "--extra-member",
        action="append",
        nargs=2,
        metavar=("SOURCE", "ARCHIVE_NAME"),
        default=[],
        help="Additional audited member; available only to the 1.1.0 release profiles.",
    )
    parser.add_argument(
        "--allow-dirty-validation",
        action="store_true",
        help=(
            "Allow a dirty release-candidate worktree only for local validation; "
            "the manifest records its exact source snapshot digest."
        ),
    )
    parser.add_argument("--objdump", help="Path to objdump for mandatory PE auditing.")
    parser.add_argument("--strings", help="Path to strings for mandatory path auditing.")
    parser.add_argument(
        "--expected-source-snapshot",
        help="Require the package manifest source_worktree_sha256 to match this full byte snapshot.",
    )
    parser.add_argument(
        "--expected-build-inputs-snapshot",
        help="Require the package manifest build_inputs_sha256 to match verified wrap/archive/extracted bytes.",
    )
    return parser


def main(argv: Sequence[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    source_root = args.source_root.resolve()
    output_directory = args.output_directory if args.output_directory.is_absolute() else source_root / args.output_directory
    try:
        extra_members = [(archive_name, Path(source)) for source, archive_name in args.extra_member]
        source_provenance = None
        if args.allow_dirty_validation and args.kind != "release-candidate":
            raise ValueError(
                "--allow-dirty-validation is only valid for release-candidate packages"
            )
        if args.kind in {"release-candidate", "release"}:
            required_wraps = (
                RELEASE_BUILD_INPUT_WRAPS
                if args.version in {RELEASE_CANDIDATE_1_1_0_VERSION, STABLE_VERSION}
                else ()
            )
            source_provenance = git_worktree_provenance(source_root, required_wraps)
            if required_wraps and source_provenance["build_inputs_ready"] != "true":
                raise ValueError("required release build inputs are absent, modified, or unverified")
            if args.expected_source_snapshot:
                expected_snapshot = args.expected_source_snapshot.upper()
                if not re.fullmatch(r"[0-9A-F]{64}", expected_snapshot):
                    raise ValueError("expected source snapshot must be an uppercase SHA-256")
                if source_provenance["source_worktree_sha256"] != expected_snapshot:
                    raise ValueError(
                        "source snapshot changed before packaging: "
                        f"expected {expected_snapshot}, observed {source_provenance['source_worktree_sha256']}"
                    )
            if args.expected_build_inputs_snapshot:
                expected_build_inputs = args.expected_build_inputs_snapshot.upper()
                if not re.fullmatch(r"[0-9A-F]{64}", expected_build_inputs):
                    raise ValueError("expected build-input snapshot must be an uppercase SHA-256")
                if source_provenance["build_inputs_sha256"] != expected_build_inputs:
                    raise ValueError(
                        "build inputs changed before packaging: "
                        f"expected {expected_build_inputs}, observed {source_provenance['build_inputs_sha256']}"
                    )
            if (
                source_provenance["source_state"] == "dirty"
                and (
                    args.kind == "release"
                    or not args.allow_dirty_validation
                )
            ):
                if args.kind == "release":
                    raise ValueError("release source tree is dirty; commit it before packaging")
                raise ValueError(
                    "release-candidate source tree is dirty; commit it or use "
                    "--allow-dirty-validation for a non-release validation package"
                )
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
            source_provenance=source_provenance,
            extra_members=extra_members,
            artifact_stem=args.artifact_stem,
            symbol_artifact_stem=args.symbol_artifact_stem,
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
