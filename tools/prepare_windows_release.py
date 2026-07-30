#!/usr/bin/env python3
"""Detach debug data from one Windows PE build without changing its code."""

from __future__ import annotations

import argparse
from pathlib import Path
import shutil
import subprocess
import sys
from typing import Sequence


def run(command: list[str]) -> None:
    completed = subprocess.run(command, check=False, capture_output=True, text=True, encoding="utf-8", errors="replace")
    if completed.returncode:
        raise ValueError("command failed: " + " ".join(command) + "\n" + completed.stdout + completed.stderr)


def prepare(raw_executable: Path, executable: Path, symbols: Path, objcopy: str, strip: str) -> None:
    if not raw_executable.is_file() or raw_executable.read_bytes()[:2] != b"MZ":
        raise ValueError(f"raw executable is not a Windows PE file: {raw_executable}")
    executable.parent.mkdir(parents=True, exist_ok=True)
    symbols.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(raw_executable, executable)
    run([objcopy, "--only-keep-debug", str(executable), str(symbols)])
    run([strip, "--strip-debug", str(executable)])
    if not symbols.is_file() or symbols.stat().st_size == 0:
        raise ValueError("objcopy did not create detached debug symbols")


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--raw-executable", type=Path, required=True)
    parser.add_argument("--executable", type=Path, required=True)
    parser.add_argument("--symbols", type=Path, required=True)
    parser.add_argument("--objcopy", default=shutil.which("objcopy"))
    parser.add_argument("--strip", default=shutil.which("strip"))
    return parser


def main(argv: Sequence[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    if not args.objcopy or not args.strip:
        print("prepare-windows-release: ERROR objcopy and strip are required", file=sys.stderr)
        return 1
    try:
        prepare(args.raw_executable, args.executable, args.symbols, args.objcopy, args.strip)
    except (OSError, ValueError) as exc:
        print(f"prepare-windows-release: ERROR {exc}", file=sys.stderr)
        return 1
    print(f"prepare-windows-release: PASS executable={args.executable} symbols={args.symbols}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
