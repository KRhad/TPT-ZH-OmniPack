#!/usr/bin/env python3
"""Classify candidate duplication against official, OmniPack and other mods."""

from __future__ import annotations

import argparse
import csv
from difflib import SequenceMatcher
from pathlib import Path
import re
from typing import Any, Sequence

from catalog_common import read_json, write_json


def normalized(value: Any) -> str:
    return re.sub(r"[^a-z0-9]+", "", str(value).lower())


def tokens(value: Any) -> set[str]:
    return set(re.findall(r"[A-Za-z][A-Za-z0-9_]+", str(value).lower()))


def jaccard(left: set[str], right: set[str]) -> float:
    if not left and not right:
        return 1.0
    return len(left & right) / len(left | right) if left | right else 0.0


def current_records(path: Path) -> list[dict[str, Any]]:
    with path.open("r", encoding="utf-8", newline="") as stream:
        rows = list(csv.DictReader(stream))
    return [
        {
            "identifier": row["identifier"],
            "code": row["display_code"],
            "name": row["english_name"],
            "zh_name": row["chinese_name"],
            "state": row["element_state"],
            "properties": row["english_description"],
            "module": row["module"],
            "stable_id": row["stable_id"],
        }
        for row in rows
        if row.get("implementation_status") == "implemented"
    ]


def similarity(candidate: dict[str, Any], current: dict[str, Any]) -> tuple[float, dict[str, float]]:
    name = SequenceMatcher(None, normalized(candidate.get("source_name", "")), normalized(current["name"])).ratio()
    code = 1.0 if normalized(candidate.get("source_code", "")) == normalized(current["code"]) else 0.0
    state = 1.0 if candidate.get("state") == current.get("state") and candidate.get("state") != "unknown" else 0.0
    prop = jaccard(tokens(candidate.get("properties", {})), tokens(current.get("properties", "")))
    score = 0.4 * max(name, code) + 0.2 * state + 0.4 * prop
    return score, {"name": name, "code": code, "state": state, "properties": prop}


def classify(candidate: dict[str, Any], current: list[dict[str, Any]]) -> dict[str, Any]:
    identifier = normalized(candidate.get("source_identifier", ""))
    exact = next((row for row in current if normalized(row["identifier"]) == identifier), None)
    comparisons = [(similarity(candidate, row), row) for row in current]
    comparisons.sort(key=lambda item: item[0][0], reverse=True)
    (score, components), best = comparisons[0] if comparisons else ((0.0, {}), {})
    same_name = normalized(candidate.get("source_name", "")) == normalized(best.get("name", ""))
    same_code = normalized(candidate.get("source_code", "")) == normalized(best.get("code", ""))
    if exact:
        decision = "exact_identifier"
        best = exact
        score = 1.0
    elif (same_name or same_code) and score >= 0.75:
        decision = "complete_duplicate"
    elif same_name or same_code:
        decision = "same_name_different_behavior"
    elif score >= 0.88:
        decision = "different_name_behavior_duplicate"
    elif best.get("module") == "official" and max(components.get("name", 0), components.get("code", 0)) == 1.0:
        decision = "official_enhancement"
    else:
        decision = "unique_candidate"
    return {
        "source_mod": candidate.get("source_mod", ""),
        "source_identifier": candidate.get("source_identifier", ""),
        "source_name": candidate.get("source_name", ""),
        "source_code": candidate.get("source_code", ""),
        "classification": decision,
        "matched_identifier": best.get("identifier", ""),
        "matched_name": best.get("name", ""),
        "matched_stable_id": best.get("stable_id", ""),
        "score": round(score, 4),
        "components": components,
    }


def cross_mod_duplicates(candidates: list[dict[str, Any]]) -> list[dict[str, Any]]:
    groups: dict[tuple[str, str], list[dict[str, Any]]] = {}
    for candidate in candidates:
        key = (normalized(candidate.get("source_name", "")), normalized(candidate.get("source_code", "")))
        if key == ("", ""):
            continue
        groups.setdefault(key, []).append(candidate)
    return [
        {
            "normalized_name": key[0],
            "normalized_code": key[1],
            "candidates": [
                f"{row.get('source_mod')}:{row.get('source_identifier')}" for row in rows
            ],
        }
        for key, rows in sorted(groups.items())
        if len({row.get("source_mod") for row in rows}) > 1
    ]


def markdown(classifications: list[dict[str, Any]], cross: list[dict[str, Any]]) -> str:
    counts: dict[str, int] = {}
    for row in classifications:
        counts[row["classification"]] = counts.get(row["classification"], 0) + 1
    lines = [
        "# 模组候选去重报告", "",
        "本报告由结构化属性、名称、代号、状态和属性 token 综合生成；自动分类只是移植审查输入，不能替代人工行为比较。", "",
        "## 统计", "",
    ]
    for key in sorted(counts):
        lines.append(f"- `{key}`：{counts[key]}")
    lines.extend(["", "## 与当前内容的高相似候选", "", "| 来源 | 候选 | 分类 | 当前匹配 | 分数 |", "|---|---|---|---|---:|"])
    for row in classifications:
        if row["classification"] == "unique_candidate":
            continue
        lines.append(
            f"| {row['source_mod']} | {row['source_identifier']} | {row['classification']} | "
            f"{row['matched_identifier']} | {row['score']:.4f} |"
        )
    lines.extend(["", "## 跨模组同名/同代号组", ""])
    if not cross:
        lines.append("当前扫描未发现跨模组精确同名同代号组。")
    else:
        for group in cross:
            lines.append("- " + "; ".join(group["candidates"]))
    return "\n".join(lines) + "\n"


def main(argv: Sequence[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-root", type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument("--elements", type=Path)
    parser.add_argument("--registry", type=Path)
    parser.add_argument("--json-output", type=Path)
    parser.add_argument("--report-output", type=Path)
    args = parser.parse_args(argv)
    root = args.source_root.resolve()
    candidates = read_json(args.elements or root / "external/metadata/candidate_elements.json", [])
    current = current_records(args.registry or root / "docs/ELEMENT_REGISTRY.csv")
    classifications = [classify(candidate, current) for candidate in candidates]
    cross = cross_mod_duplicates(candidates)
    payload = {"classifications": classifications, "cross_mod_groups": cross}
    json_output = args.json_output or root / "external/metadata/duplicates.json"
    report_output = args.report_output or root / "docs/MOD_DUPLICATE_REPORT.md"
    write_json(json_output, payload)
    report_output.write_text(markdown(classifications, cross), encoding="utf-8")
    print(
        f"detect-duplicates: PASS candidates={len(candidates)} "
        f"cross_groups={len(cross)} output={report_output}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
