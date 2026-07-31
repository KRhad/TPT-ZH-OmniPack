#!/usr/bin/env python3
"""Fail-closed audit for compiled OmniPack encyclopedia content."""

from __future__ import annotations

import argparse
import importlib.util
from pathlib import Path
import sys
from typing import Sequence


CATALOG_SCRIPT = Path(__file__).with_name("generate_element_catalog.py")
CATALOG_SPEC = importlib.util.spec_from_file_location(
    "element_content_catalog", CATALOG_SCRIPT
)
assert CATALOG_SPEC is not None and CATALOG_SPEC.loader is not None
element_content_catalog = importlib.util.module_from_spec(CATALOG_SPEC)
sys.modules[CATALOG_SPEC.name] = element_content_catalog
CATALOG_SPEC.loader.exec_module(element_content_catalog)


def audit_with_records(root: Path) -> tuple[dict[str, dict[str, str]], list[str]]:
    return element_content_catalog.audit_content_registry(
        root / "docs" / "ELEMENT_REGISTRY.csv",
        root / "docs" / "ELEMENT_CONTENT.csv",
    )


def audit(root: Path) -> list[str]:
    _, errors = audit_with_records(root)
    return errors


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Audit complete bilingual encyclopedia content for implemented OmniPack elements."
    )
    parser.add_argument(
        "--source-root", type=Path, default=Path(__file__).resolve().parents[1]
    )
    parser.add_argument("--quiet", action="store_true")
    return parser


def main(argv: Sequence[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    records, errors = audit_with_records(args.source_root.resolve())
    if errors:
        for error in errors:
            print(f"element-content-audit: ERROR {error}", file=sys.stderr)
        print(f"element-content-audit: FAIL ({len(errors)} errors)", file=sys.stderr)
        return 1
    if not args.quiet:
        print(
            "element-content-audit: PASS "
            f"({len(records)} implemented OmniPack elements, bilingual content)"
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
