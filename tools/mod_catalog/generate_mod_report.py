#!/usr/bin/env python3
"""Generate machine-readable and Markdown reports from extracted mod metadata."""

from __future__ import annotations

import argparse
from pathlib import Path
import re
from typing import Any, Sequence

from catalog_common import SOURCE_CATALOG_COLUMNS, read_json, write_csv


ELEMENT_COLUMNS = (
    "source_mod", "source_commit", "source_file", "source_identifier", "source_id",
    "source_name", "source_code", "category", "state", "color", "properties_summary",
    "update_function", "graphics_function", "dependencies", "reaction_count",
    "reaction_targets", "global_scan", "particle_creation", "particle_deletion",
    "performance_risk", "save_risk", "license_status", "port_decision",
)

FEATURE_COLUMNS = (
    "source_mod", "source_commit", "feature_type", "source_files", "description",
    "core_area", "performance_risk", "save_risk", "license_status", "port_decision", "notes",
)


def compatible_license(row: dict[str, Any]) -> bool:
    value = str(row.get("license", ""))
    return row.get("license_verified") == "true" and any(
        token in value for token in ("GPL-3.0", "MIT", "Apache-2.0", "MPL")
    )


def decision(
    element: dict[str, Any],
    mod: dict[str, Any],
    duplicate: dict[str, Any],
    core: dict[str, Any],
) -> str:
    if mod.get("source_type") in {"binary_only", "unavailable", "documentation_only"}:
        return "D_reject"
    if not compatible_license(mod):
        return "C_reference_only"
    if duplicate.get("classification") in {"exact_identifier", "complete_duplicate", "different_name_behavior_duplicate"}:
        return "D_reject"
    if (
        element.get("performance_risk") == "low"
        and element.get("save_risk") == "low"
        and duplicate.get("classification") == "unique_candidate"
        and (
            "/elements/" in str(element.get("source_file", "")).lower()
            or "element_" in str(element.get("source_file", "")).lower()
        )
    ):
        return "A_direct_port"
    return "B_rewrite_port"


def list_value(value: Any) -> str:
    if isinstance(value, list):
        return ";".join(str(item) for item in value)
    if isinstance(value, dict):
        return ";".join(f"{key}={value[key]}" for key in sorted(value))
    return str(value)


def concept_key(element: dict[str, Any]) -> tuple[str, str]:
    """Collapse repeated fork definitions without pretending behavior variants are identical."""
    normalize = lambda value: re.sub(r"[^a-z0-9]+", "", str(value).lower())
    name = normalize(element.get("source_name", ""))
    code = normalize(element.get("source_code", ""))
    if name or code:
        return name, code
    return normalize(element.get("source_identifier", "")), ""


def write_repository_audit(path: Path, mods: list[dict[str, Any]], core_by_mod: dict[str, dict[str, Any]]) -> None:
    lines = [
        "# 模组仓库审计", "",
        "外部仓库只读扫描；`buildable=not_tested` 不能解释为可构建。精确 commit 为空表示尚未克隆。", "",
        "| mod_id | 来源类型 | commit | 许可证 | 元素检测 | 核心修改 | 状态 |",
        "|---|---|---|---|---:|---:|---|",
    ]
    for mod in mods:
        core = core_by_mod.get(mod["mod_id"], {})
        lines.append(
            f"| {mod['mod_id']} | {mod.get('source_type')} | {str(mod.get('source_commit', ''))[:12]} | "
            f"{mod.get('license')} | {mod.get('element_count_detected')} | "
            f"{core.get('core_files_modified', 'not_tested')} | {mod.get('audit_status')} |"
        )
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def write_license_audit(path: Path, mods: list[dict[str, Any]]) -> None:
    lines = [
        "# 模组许可证审计", "",
        "只有仓库根许可证或可核对的许可证文件存在并被识别时，`license_verified=true`。API 标签、仓库描述或论坛口头说明不能单独替代文件证据。", "",
        "本表同时扫描 README 许可证字样、元素源码前 80 行、子模块、非代码资源和嵌套 NOTICE/LICENSE。自动扫描完成不等于版权判断完成；实际移植仍须对选定文件逐项复核并登记来源。", "",
        "| mod_id | 根许可证 | 根证据 | README 证据 | 元素源码扫描 | 资源/嵌套通知 | 子模块 | 初判 | 代码使用边界 |",
        "|---|---|---|---|---:|---|---|---|---|",
    ]
    for mod in mods:
        evidence = list_value(mod.get("license_evidence", [])) or "none"
        scope = mod.get("license_scope_audit", {}) if isinstance(mod.get("license_scope_audit"), dict) else {}
        mentions = scope.get("readme_license_mentions", [])
        readme = list_value(mentions).replace("|", "\\|") if mentions else "none"
        source_count = int(scope.get("element_source_files_scanned", 0) or 0)
        header_count = len(scope.get("element_source_header_notices", []) or [])
        restrictive = len(scope.get("element_source_restrictive_markers", []) or [])
        assets = int(scope.get("asset_files_scanned", 0) or 0)
        nested = len(scope.get("nested_notice_files", []) or [])
        submodules = list_value(scope.get("submodules", [])) or "none"
        if compatible_license(mod) and not restrictive:
            gate, boundary = "A-code-scope", "仅可让选定代码进入逐文件复核；保留作者、commit、文件和 GPL 通知"
        elif compatible_license(mod):
            gate, boundary = "B-header-review", "元素源码存在限制性字样；人工确认前不得复制"
        elif mod.get("license_verified") == "true":
            gate, boundary = "B-review", "许可证明确但兼容性需人工法律审查"
        elif mod.get("source_type") == "binary_only":
            gate, boundary = "D-binary", "禁止反编译和复制"
        else:
            gate, boundary = "C-unknown", "只能记录概念；不得复制代码或资源"
        lines.append(
            f"| {mod['mod_id']} | {mod.get('license')} ({mod.get('license_verified')}) | {evidence} | "
            f"{readme} | {source_count}; header_notice={header_count}; restrictive={restrictive} | "
            f"assets={assets}; nested_notice={nested} | {submodules} | {gate} | {boundary} |"
        )
    lines.extend([
        "", "## 当前门禁", "",
        "- 根许可证自动识别：已执行。",
        "- README、元素源码头、子模块与资源清单自动扫描：已执行于成功检出的仓库。",
        "- 每个最终移植文件的人工作者/许可证/资源兼容复核：尚未完成。",
        "- 因此当前 `license_audit_pass=false`。未知许可证、仅二进制和下载失效来源不得复制。",
    ])
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def write_porting(path: Path, elements: list[dict[str, Any]]) -> None:
    groups = {
        "A_direct_port": "A：直接移植候选",
        "B_rewrite_port": "B：重写移植候选",
        "C_reference_only": "C：仅设计参考",
        "D_reject": "D：拒绝或重复",
    }
    lines = [
        "# 模组移植计划", "",
        "自动决策只用于排序。任何 A 类仍须人工复核许可证、源码文件头、稳定 ID、模块、图鉴、事件预算和 OPS。", "",
    ]
    for key, heading in groups.items():
        rows = [row for row in elements if row["port_decision"] == key]
        lines.extend([f"## {heading}", ""])
        if not rows:
            lines.append("当前扫描无候选。")
        else:
            lines.extend(["| 来源 | identifier | 名称 | 性能 | 存档 |", "|---|---|---|---|---|"])
            for row in rows[:200]:
                lines.append(
                    f"| {row['source_mod']} | {row['source_identifier']} | {row['source_name']} | "
                    f"{row['performance_risk']} | {row['save_risk']} |"
                )
        lines.append("")
    path.write_text("\n".join(lines), encoding="utf-8")


def write_rejections(path: Path, elements: list[dict[str, Any]], duplicates: dict[str, dict[str, Any]]) -> None:
    lines = ["# 模组候选拒绝日志", "", "| 来源 | 候选 | 原因 |", "|---|---|---|"]
    for row in elements:
        if row["port_decision"] != "D_reject":
            continue
        duplicate = duplicates.get(f"{row['source_mod']}\0{row['source_identifier']}", {})
        reason = duplicate.get("classification", "source unavailable or binary-only")
        lines.append(f"| {row['source_mod']} | {row['source_identifier']} | {reason} |")
    if len(lines) == 4:
        lines.append("| none | none | 当前扫描无自动拒绝项 |")
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main(argv: Sequence[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-root", type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument("--clean-build-pass", choices=("true", "false", "not_tested"), default="not_tested")
    parser.add_argument("--element-registry-pass", choices=("true", "false", "not_tested"), default="not_tested")
    parser.add_argument("--reaction-registry-pass", choices=("true", "false", "not_tested"), default="not_tested")
    parser.add_argument("--license-audit-pass", choices=("true", "false", "not_tested"), default="false")
    args = parser.parse_args(argv)
    root = args.source_root.resolve()
    mods = read_json(root / "external/metadata/repository_scan.json", [])
    cpp_elements = read_json(root / "external/metadata/cpp_elements.json", [])
    lua_elements = read_json(root / "external/metadata/lua_elements.json", [])
    elements = read_json(root / "external/metadata/candidate_elements.json", [])
    reactions = read_json(root / "external/metadata/reactions.json", [])
    core = read_json(root / "external/metadata/core_modifications.json", [])
    duplicate_payload = read_json(root / "external/metadata/duplicates.json", {"classifications": []})
    reaction_by_key = {
        (row["source_mod"], row["source_file"], row["source_identifier"]): row for row in reactions
    }
    core_by_mod = {row["mod_id"]: row for row in core}
    mod_by_id = {row["mod_id"]: row for row in mods}
    duplicate_by_key = {
        f"{row['source_mod']}\0{row['source_identifier']}": row
        for row in duplicate_payload.get("classifications", [])
    }

    catalog_rows: list[dict[str, Any]] = []
    for element in elements:
        key = (element["source_mod"], element["source_file"], element["source_identifier"])
        reaction = reaction_by_key.get(key, {})
        duplicate = duplicate_by_key.get(f"{element['source_mod']}\0{element['source_identifier']}", {})
        mod = mod_by_id.get(element["source_mod"], {})
        port = decision(element, mod, duplicate, core_by_mod.get(element["source_mod"], {}))
        row = {**element, **{
            "reaction_count": reaction.get("reaction_count", 0),
            "reaction_targets": list_value(reaction.get("reaction_targets", [])),
            "dependencies": list_value(element.get("dependencies", [])),
            "properties_summary": list_value(element.get("properties_summary", "")),
            "global_scan": str(bool(reaction.get("global_scan", element.get("global_scan", False)))).lower(),
            "port_decision": port,
        }}
        catalog_rows.append(row)

    write_csv(root / "docs/MOD_ELEMENT_CATALOG.csv", ELEMENT_COLUMNS, catalog_rows)
    for row in mods:
        if row["mod_id"] in core_by_mod:
            detail = core_by_mod[row["mod_id"]]
            row.update({
                "core_files_modified": detail["core_files_modified"],
                "save_format_modified": str(detail["save_format_modified"]).lower(),
                "network_code_modified": str(detail["network_code_modified"]).lower(),
                "ui_modified": str(detail["ui_modified"]).lower(),
            })
    write_csv(root / "docs/MOD_SOURCE_CATALOG.csv", SOURCE_CATALOG_COLUMNS, mods)

    feature_rows: list[dict[str, Any]] = []
    for detail in core:
        mod = mod_by_id.get(detail["mod_id"], {})
        for area, count in detail.get("category_counts", {}).items():
            feature_rows.append({
                "source_mod": detail["mod_id"],
                "source_commit": mod.get("source_commit", ""),
                "feature_type": "core_modification",
                "source_files": count,
                "description": f"{count} changed files in {area}",
                "core_area": area,
                "performance_risk": "review",
                "save_risk": "high" if area in {"save", "protocol"} else "review",
                "license_status": "verified" if mod.get("license_verified") == "true" else "unknown",
                "port_decision": "B_rewrite_port" if compatible_license(mod) else "C_reference_only",
                "notes": "Do not merge the external core wholesale",
            })
    write_csv(root / "docs/MOD_FEATURE_CATALOG.csv", FEATURE_COLUMNS, feature_rows)

    write_repository_audit(root / "docs/MOD_REPOSITORY_AUDIT.md", mods, core_by_mod)
    write_license_audit(root / "docs/MOD_LICENSE_AUDIT.md", mods)
    write_porting(root / "docs/MOD_PORTING_PLAN.md", catalog_rows)
    write_rejections(root / "docs/MOD_REJECTION_LOG.md", catalog_rows, duplicate_by_key)

    cloned = sum(row.get("audit_status") == "scanned" for row in mods)
    raw_downloaded = sum(row.get("audit_status") == "downloaded_source" for row in mods)
    compatible = sum(compatible_license(row) for row in mods)
    decisions = {key: sum(row["port_decision"] == key for row in catalog_rows) for key in (
        "A_direct_port", "B_rewrite_port", "C_reference_only", "D_reject"
    )}
    keys_by_decision = {
        key: {concept_key(row) for row in catalog_rows if row["port_decision"] == key}
        for key in decisions
    }
    direct_keys = keys_by_decision["A_direct_port"]
    rewrite_keys = keys_by_decision["B_rewrite_port"] - direct_keys
    licensed_keys = direct_keys | rewrite_keys
    reference_keys = keys_by_decision["C_reference_only"] - licensed_keys
    candidate_keys = licensed_keys | reference_keys
    rejected_keys = keys_by_decision["D_reject"] - candidate_keys
    lines = [
        "# 模组提取报告", "", "```text",
        f"repositories_discovered={len(mods)}",
        f"repositories_cloned={cloned}",
        f"raw_sources_downloaded={raw_downloaded}",
        "repositories_buildable=not_tested",
        f"mods_cataloged={len(mods)}",
        f"mods_with_source={sum(row.get('source_type') in {'cpp_source', 'lua_source', 'mixed_source'} for row in mods)}",
        f"mods_binary_only={sum(row.get('source_type') == 'binary_only' for row in mods)}",
        f"mods_license_compatible={compatible}",
        f"mods_license_unknown={sum(row.get('license_verified') != 'true' for row in mods)}",
        f"cpp_elements_detected={len(cpp_elements)}",
        f"lua_elements_detected={len(lua_elements)}",
        f"candidate_definition_records={len(catalog_rows)}",
        f"unique_candidate_elements={len(candidate_keys)}",
        f"licensed_source_candidates={len(licensed_keys)}",
        f"duplicate_candidates={len(rejected_keys)}",
        f"duplicate_definition_records={decisions['D_reject']}",
        f"rejected_candidates={len(rejected_keys)}",
        f"direct_port_candidates={len(direct_keys)}",
        f"rewrite_candidates={len(rewrite_keys)}",
        f"reference_only_candidates={len(reference_keys)}",
        "elements_ported=0", "elements_rewritten=0",
        "elements_rejected=0",
        "periodic_elements_sourced=not_tested", "periodic_elements_remaining=118",
        "total_omnipack_elements=48",
        f"clean_build_pass={args.clean_build_pass}",
        f"element_registry_pass={args.element_registry_pass}",
        f"reaction_registry_pass={args.reaction_registry_pass}",
        f"license_audit_pass={args.license_audit_pass}",
        "```", "",
        "数值只代表当前克隆集和自动扫描。候选数按标准化名称与代号折叠重复分叉；行为差异仍保留在去重报告中。",
        "`duplicate_definition_records` 是自动拒绝的重复源码定义数，不等于已经人工拒绝的独立材料。",
        "`license_audit_pass=true` 只能由完成逐文件、README、子模块和资源复核后的显式参数写入。",
    ]
    (root / "docs/MOD_EXTRACTION_REPORT.md").write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(f"generate-mod-report: PASS mods={len(mods)} elements={len(catalog_rows)} features={len(feature_rows)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
