#!/usr/bin/env python3
"""Generate the complete implemented-material UI classification audit."""

from __future__ import annotations

import argparse
import csv
import io
from pathlib import Path
import re
from typing import Sequence


FIELDS = (
    "stable_id", "identifier", "display_code", "english_name", "chinese_name",
    "record_status", "canonical_identifier", "source_class", "material_category",
    "source_menu", "source_menu_visible", "main_menu_policy", "current_rc9_entry",
    "target_entry", "periodic_atomic_numbers", "periodic_symbols",
    "periodic_relation_groups", "duplicate_concept", "module",
    "module_disable_behavior", "save_compatibility", "description_coverage",
    "source_mod", "source_commit", "source_file", "license",
    "relation_provenance", "confidence", "notes",
)
OFFICIAL_SOURCE = "The-Powder-Toy/The-Powder-Toy"
MENU_VISIBLE = re.compile(r"\bMenuVisible\s*=\s*([01])\s*;")
RC9_ELECTRONICS_MATERIAL_IDS = {
    *range(622, 632),
    *range(634, 642),
}


def read_csv(path: Path) -> list[dict[str, str]]:
    with path.open("r", encoding="utf-8-sig", newline="") as stream:
        return list(csv.DictReader(stream))


def atomic_symbols(root: Path) -> dict[int, str]:
    return {
        int(row["atomic_number"]): row["symbol"]
        for row in read_csv(root / "docs" / "PERIODIC_ELEMENT_SOURCE_MAP.csv")
    }


def menu_visible(root: Path, row: dict[str, str]) -> str:
    source = (root / row["source_file"]).read_text(encoding="utf-8")
    match = MENU_VISIBLE.search(source)
    if not match:
        raise ValueError(f"{row['identifier']}: source lacks literal MenuVisible")
    return "true" if match.group(1) == "1" else "false"


def placement(policy: dict[str, str] | None, row: dict[str, str]) -> str:
    if not policy:
        return (
            f"main_menu:{row['menu_category']}"
            if row["_menu_visible"] == "true"
            else "hidden_or_runtime_only"
        )
    value = policy["main_menu_policy"]
    if value == "periodic_only":
        return "periodic_table"
    if value == "organic_menu":
        return "material_library:organic"
    if value == "alloy_menu":
        return "material_library:alloy_engineering"
    if value == "existing_dedicated_menu":
        return "main_menu:" + policy["standalone_menu"]
    if value == "preserve_existing":
        return "main_menu:" + row["menu_category"]
    return "hidden_or_runtime_only"


def add_entry(entry: str, addition: str) -> str:
    parts = entry.split("+")
    if addition not in parts:
        parts.append(addition)
    return "+".join(parts)


def material_category(
    row: dict[str, str],
    policy: dict[str, str] | None,
    groups: set[str],
    base_kind: str,
) -> str:
    if row["is_duplicate"] == "true":
        return "compatibility_alias"
    if groups:
        priority = (
            "allotrope", "isotope", "oxide", "hydroxide", "acid", "base",
            "salt", "halide", "sulfide", "nitride", "carbide", "hydride",
            "polymer", "organic", "alloy", "mineral", "ceramic", "glass",
            "semiconductor", "composite", "engineering", "element", "other",
        )
        for value in priority:
            if value in groups:
                return "single_element" if value == "element" else value
    if base_kind == "periodic_element":
        return "single_element"
    if base_kind == "isotope":
        return "isotope"
    if base_kind == "inorganic_compound":
        return "inorganic_compound"
    kind = policy["content_kind"] if policy else "official"
    return {
        "organic_material": "organic",
        "alloy_engineering": "engineering",
        "ecology_material": "ecology_material",
        "nuclear_device": "nuclear_device",
        "custom_special": "custom_special",
        "official": "official_sandbox_element",
    }.get(kind, kind)


def source_class(row: dict[str, str], policy: dict[str, str] | None) -> str:
    if row["source_mod"] == OFFICIAL_SOURCE:
        return "official_original"
    if row["is_duplicate"] == "true":
        return "compatibility_alias"
    if policy and policy["content_kind"] == "custom_special":
        return "omnipack_special"
    return "omnipack_material"


def module_behavior(row: dict[str, str]) -> str:
    if row["source_mod"] == OFFICIAL_SOURCE or row["module"] == "periodic":
        return "always_available;stable_id_locked"
    if row["is_duplicate"] == "true":
        return "not_selectable;legacy_particle_preserved;module_enabled_migration"
    return "disabled_blocks_selection_and_updates;loaded_particles_preserved"


def render(root: Path) -> tuple[str, dict[str, int]]:
    registry = read_csv(root / "docs" / "ELEMENT_REGISTRY.csv")
    policy = {
        row["identifier"]: row
        for row in read_csv(root / "docs" / "CONTENT_MENU_POLICY.csv")
    }
    base_links = read_csv(root / "docs" / "PERIODIC_CONTENT_LINKS.csv")
    related_links = read_csv(root / "docs" / "MATERIAL_PERIODIC_INDEX.csv")
    full_descriptions = {
        row["identifier"]
        for row in read_csv(root / "docs" / "OFFICIAL_ELEMENT_DESCRIPTIONS.csv")
    }
    symbols = atomic_symbols(root)

    links: dict[str, list[dict[str, str]]] = {}
    for link in base_links + related_links:
        links.setdefault(link["tool_identifier"], []).append(link)

    implemented = [
        row for row in registry if row["implementation_status"] == "implemented"
    ]
    stable_ids = [int(row["stable_id"]) for row in implemented]
    if len(stable_ids) != len(set(stable_ids)):
        raise ValueError("implemented registry contains duplicate stable IDs")

    output_rows: list[dict[str, str]] = []
    for row in sorted(implemented, key=lambda item: int(item["stable_id"])):
        row = dict(row)
        row["_menu_visible"] = menu_visible(root, row)
        row_policy = policy.get(row["identifier"])
        if int(row["stable_id"]) >= 196 and not row_policy:
            raise ValueError(f"{row['identifier']}: missing menu policy")
        row_links = links.get(row["identifier"], [])
        base = [link for link in row_links if link["content_kind"] != "related_material"]
        supplemental = [
            link for link in row_links if link["content_kind"] == "related_material"
        ]
        atoms = sorted({
            int(value)
            for link in row_links
            for value in link["related_atomic_numbers"].split("|")
        })
        groups = {link["compound_group"] for link in row_links}
        current_entry = placement(row_policy, row)
        if int(row["stable_id"]) in RC9_ELECTRONICS_MATERIAL_IDS:
            current_entry = "main_menu:SC_ELEC"
        if base and current_entry != "periodic_table":
            current_entry = add_entry(current_entry, "periodic_table")
        target_entry = placement(row_policy, row)
        if base and target_entry != "periodic_table":
            target_entry = add_entry(target_entry, "periodic_table")
        if supplemental:
            target_entry = add_entry(target_entry, "periodic_table")
        canonical_identifier = row["duplicate_of"] or row["identifier"]
        record_status = (
            "compatibility_alias" if row["is_duplicate"] == "true" else "canonical"
        )
        if row["identifier"] in full_descriptions:
            description = "official_wiki_full+runtime_properties"
        elif row["source_mod"] == OFFICIAL_SOURCE:
            description = "official_summary+runtime_properties"
        elif row["is_duplicate"] == "true":
            description = "compatibility_record+runtime_properties"
        else:
            description = "registry_summary+structured_catalog+runtime_properties"
        relation_provenance = ";".join(sorted({
            link.get("source_reference", "") or "docs/PERIODIC_CONTENT_LINKS.csv"
            for link in row_links
        }))
        confidence_values = {
            link.get("confidence", "") or "high" for link in row_links
        }
        confidence = (
            "low" if "low" in confidence_values
            else "medium" if "medium" in confidence_values
            else "high"
        )
        base_kind = base[0]["content_kind"] if base else ""
        output_rows.append({
            "stable_id": row["stable_id"],
            "identifier": row["identifier"],
            "display_code": row["display_code"],
            "english_name": row["english_name"],
            "chinese_name": row["chinese_name"],
            "record_status": record_status,
            "canonical_identifier": canonical_identifier,
            "source_class": source_class(row, row_policy),
            "material_category": material_category(
                row, row_policy, groups, base_kind
            ),
            "source_menu": row["menu_category"],
            "source_menu_visible": row["_menu_visible"],
            "main_menu_policy": (
                row_policy["main_menu_policy"] if row_policy else "official_default"
            ),
            "current_rc9_entry": current_entry,
            "target_entry": target_entry,
            "periodic_atomic_numbers": "|".join(map(str, atoms)),
            "periodic_symbols": "|".join(symbols[value] for value in atoms),
            "periodic_relation_groups": "|".join(sorted(groups)),
            "duplicate_concept": (
                "alias_of:" + row["duplicate_of"]
                if row["duplicate_of"] else "canonical"
            ),
            "module": row["module"],
            "module_disable_behavior": module_behavior(row),
            "save_compatibility": row["save_compatibility"],
            "description_coverage": description,
            "source_mod": row["source_mod"],
            "source_commit": row["source_commit"],
            "source_file": row["source_file"],
            "license": row["license"],
            "relation_provenance": relation_provenance,
            "confidence": confidence,
            "notes": row["notes"],
        })

    stream = io.StringIO(newline="")
    writer = csv.DictWriter(stream, fieldnames=FIELDS, lineterminator="\n")
    writer.writeheader()
    writer.writerows(output_rows)
    stats = {
        "implemented_slots": len(output_rows),
        "canonical": sum(row["record_status"] == "canonical" for row in output_rows),
        "aliases": sum(
            row["record_status"] == "compatibility_alias" for row in output_rows
        ),
        "periodic_before": sum(
            "periodic_table" in row["current_rc9_entry"] for row in output_rows
        ),
        "periodic_after": sum(
            "periodic_table" in row["target_entry"] for row in output_rows
        ),
        "supplemental": len(related_links),
        "menu_migrated": sum(
            row["current_rc9_entry"] != row["target_entry"]
            for row in output_rows
        ),
        "ordinary_main_before": sum(
            "main_menu:" in row["current_rc9_entry"] for row in output_rows
        ),
        "ordinary_main_after": sum(
            "main_menu:" in row["target_entry"] for row in output_rows
        ),
        "ordinary_menu_relocated": sum(
            "main_menu:" in row["current_rc9_entry"]
            and "material_library:" in row["target_entry"]
            for row in output_rows
        ),
    }
    return stream.getvalue(), stats


def main(argv: Sequence[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--source-root", type=Path, default=Path(__file__).resolve().parents[1]
    )
    parser.add_argument("--output", type=Path)
    parser.add_argument("--check", action="store_true")
    parser.add_argument("--quiet", action="store_true")
    args = parser.parse_args(argv)
    root = args.source_root.resolve()
    output = args.output or root / "docs" / "MATERIAL_UI_AUDIT.csv"
    try:
        rendered, stats = render(root)
        if args.check:
            if not output.is_file() or output.read_text(encoding="utf-8-sig") != rendered:
                raise ValueError(f"generated material audit is stale: {output}")
        else:
            output.write_text(rendered, encoding="utf-8", newline="")
    except (OSError, UnicodeDecodeError, csv.Error, ValueError) as exc:
        print(f"material-ui-audit: ERROR {exc}")
        return 1
    if not args.quiet:
        print(
            "material-ui-audit: PASS "
            + " ".join(f"{key}={value}" for key, value in stats.items())
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
