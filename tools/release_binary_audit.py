#!/usr/bin/env python3
"""Inspect a Windows PE release executable and its detached debug symbols."""

from __future__ import annotations

import argparse
import hashlib
from pathlib import Path
import re
import shutil
import subprocess
import sys
from typing import Sequence


DEBUG_SECTION_RE = re.compile(r"\s\.debug(?:_|$)", re.IGNORECASE)
GNU_DEBUGLINK_RE = re.compile(r"\s\.gnu_debuglink\s", re.IGNORECASE)
PATH_MARKERS = ("C:\\Users\\", "/Users/", "\\build-", "/build-")
SECURITY_MARKERS = ("DYNAMIC_BASE", "NX_COMPAT", "HIGH_ENTROPY_VA")
DLL_CHARACTERISTICS_RE = re.compile(r"DllCharacteristics\s+([0-9A-Fa-f]+)")
DLL_CHARACTERISTICS_BITS = {
    "DYNAMIC_BASE": 0x0040,
    "NX_COMPAT": 0x0100,
    "HIGH_ENTROPY_VA": 0x0020,
}
DYNAMIC_RUNTIME_DLLS = ("libgcc_s_seh-1.dll", "libstdc++-6.dll", "libwinpthread-1.dll")


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest().upper()


def find_tool(name: str, explicit: str | None) -> str | None:
    if explicit:
        return explicit
    return shutil.which(name)


def run(tool: str, *arguments: str) -> subprocess.CompletedProcess[str]:
    return subprocess.run([tool, *arguments], check=False, capture_output=True, text=True, encoding="utf-8", errors="replace")


def audit_executable(executable: Path, objdump: str | None, strings: str | None, *, expect_debuglink: bool) -> list[str]:
    errors: list[str] = []
    if not executable.is_file():
        return [f"executable does not exist: {executable}"]
    if executable.read_bytes()[:2] != b"MZ":
        errors.append("executable does not have a PE MZ header")
        return errors
    if not objdump:
        errors.append("objdump is required for release PE auditing")
        return errors
    headers = run(objdump, "-h", str(executable))
    if headers.returncode:
        errors.append(f"objdump section scan failed: {headers.stderr.strip()}")
    else:
        sections = [line for line in headers.stdout.splitlines() if not GNU_DEBUGLINK_RE.search(line)]
        if DEBUG_SECTION_RE.search("\n".join(sections)):
            errors.append("release executable retains a DWARF .debug section")
        if expect_debuglink and not GNU_DEBUGLINK_RE.search(headers.stdout):
            errors.append("release executable is missing its detached-symbol .gnu_debuglink")
    portable = run(objdump, "-p", str(executable))
    if portable.returncode:
        errors.append(f"objdump PE scan failed: {portable.stderr.strip()}")
    else:
        match = DLL_CHARACTERISTICS_RE.search(portable.stdout)
        flags = int(match.group(1), 16) if match else 0
        for marker in SECURITY_MARKERS:
            if not flags & DLL_CHARACTERISTICS_BITS[marker]:
                errors.append(f"PE security flag is absent: {marker}")
        imports = portable.stdout.lower()
        for dll_name in DYNAMIC_RUNTIME_DLLS:
            if f"dll name: {dll_name}" in imports:
                errors.append(f"release executable imports a development runtime DLL: {dll_name}")
    if not strings:
        errors.append("strings is required for release path auditing")
    else:
        output = run(strings, "-a", str(executable))
        if output.returncode:
            errors.append(f"strings scan failed: {output.stderr.strip()}")
        else:
            for marker in PATH_MARKERS:
                if marker.lower() in output.stdout.lower():
                    errors.append(f"release executable leaks a build path marker: {marker}")
    return errors


def audit_pair(executable: Path, symbols: Path | None, objdump: str | None, strings: str | None) -> list[str]:
    errors = audit_executable(executable, objdump, strings, expect_debuglink=symbols is not None)
    if symbols is not None:
        if not symbols.is_file():
            errors.append(f"debug symbols do not exist: {symbols}")
        elif symbols.read_bytes()[:2] != b"MZ":
            errors.append("debug symbols do not have a PE MZ header")
        elif symbols.stat().st_size == 0:
            errors.append("debug symbols are empty")
    return errors


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--executable", type=Path, required=True)
    parser.add_argument("--symbols", type=Path)
    parser.add_argument("--objdump")
    parser.add_argument("--strings")
    return parser


def main(argv: Sequence[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    errors = audit_pair(
        args.executable,
        args.symbols,
        find_tool("objdump", args.objdump),
        find_tool("strings", args.strings),
    )
    if errors:
        for error in errors:
            print(f"release-binary-audit: ERROR {error}", file=sys.stderr)
        return 1
    symbols_sha = sha256(args.symbols) if args.symbols else "none"
    print(
        "release-binary-audit: PASS "
        f"executable_sha256={sha256(args.executable)} symbols_sha256={symbols_sha}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
