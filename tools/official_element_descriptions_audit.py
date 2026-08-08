#!/usr/bin/env python3
"""Audit source-checked full descriptions for all canonical official elements."""

from __future__ import annotations

import argparse
import importlib.util
from pathlib import Path
import sys
from typing import Sequence


CATALOG_SCRIPT = Path(__file__).with_name("generate_element_catalog.py")
CATALOG_SPEC = importlib.util.spec_from_file_location(
    "official_description_catalog", CATALOG_SCRIPT
)
assert CATALOG_SPEC is not None and CATALOG_SPEC.loader is not None
catalog = importlib.util.module_from_spec(CATALOG_SPEC)
sys.modules[CATALOG_SPEC.name] = catalog
CATALOG_SPEC.loader.exec_module(catalog)


def audit_with_records(root: Path) -> tuple[dict[str, dict[str, str]], list[str]]:
    return catalog.audit_official_description_registry(
        root / "docs" / "ELEMENT_REGISTRY.csv",
        root / "docs" / "OFFICIAL_ELEMENT_DESCRIPTIONS.csv",
    )


def audit(root: Path) -> list[str]:
    _, errors = audit_with_records(root)
    return errors


def main(argv: Sequence[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--source-root", type=Path, default=Path(__file__).resolve().parents[1]
    )
    parser.add_argument("--quiet", action="store_true")
    args = parser.parse_args(argv)
    records, errors = audit_with_records(args.source_root.resolve())
    if errors:
        for error in errors:
            print(f"official-element-descriptions: ERROR {error}", file=sys.stderr)
        print(
            f"official-element-descriptions: FAIL ({len(errors)} errors)",
            file=sys.stderr,
        )
        return 1
    if not args.quiet:
        print(
            "official-element-descriptions: PASS "
            f"({len(records)} canonical official elements, bilingual full descriptions)"
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
