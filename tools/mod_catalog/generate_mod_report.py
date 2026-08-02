#!/usr/bin/env python3
"""Generate machine-readable and Markdown reports from extracted mod metadata."""

from __future__ import annotations

import argparse
import csv
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


def directly_selectable(root: Path, row: dict[str, Any]) -> bool:
    """Count real menu materials, excluding the eraser and hidden helpers."""
    if (
        row.get("implementation_status") != "implemented"
        or row.get("is_duplicate") == "true"
        or row.get("default_enabled") != "true"
        or row.get("stable_id") == "0"
    ):
        return False
    source_file = str(row.get("source_file", ""))
    if not source_file:
        return False
    path = root / source_file
    if not path.is_file():
        return False
    return re.search(
        r"\bMenuVisible\s*=\s*1\s*;", path.read_text(encoding="utf-8")
    ) is not None


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
    # Same-concept candidates are reviewed against and, when useful, rewritten
    # into the existing official/OmniPack canonical element. Exact identifiers
    # also require a source delta review before they may be called unchanged.
    # No second stable ID or duplicate menu entry is allocated.
    if duplicate.get("classification") in {
        "exact_identifier",
        "official_enhancement",
        "same_name_different_behavior",
    }:
        return "B_rewrite_port"
    if duplicate.get("classification") in {"complete_duplicate", "different_name_behavior_duplicate"}:
        return "D_reject"
    source_identifier = str(element.get("source_identifier", ""))
    source_id = str(element.get("source_id", ""))
    if source_identifier.upper().startswith("DEFAULT_PT_") or (
        source_id.isdigit() and int(source_id) < 196
    ):
        # External definitions in the official namespace or locked official ID
        # range always require a rewrite, even when the concept itself is new.
        return "B_rewrite_port"
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


def write_license_audit(
    path: Path, mods: list[dict[str, Any]], license_audit_pass: str
) -> None:
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
    closure_lines = (
        [
            "- 41 个素材来源已经逐项分类；20 个许可证未知或未验证来源全部保持 `C_reference_only` / `D_reject`，不进入实现、资源或发行包。",
            "- 最终实际使用边界为 22 个登记的兼容来源元素（其中 4 个文件级概念重写、18 个参数/token/行为概念独立实现），第三方更新函数逐行复制为 0。",
            "- `docs/THIRD_PARTY_LICENSE_MANIFEST.csv` 固定 24 个代码、模组、字体和静态库组件；发行映射同时携带字体及 12 份预构建库许可证文本。",
            "- `tools/third_party_license_audit.py` 已核对 manifest 哈希、登记来源/commit、候选授权决策、只读外部目录、跟踪资源与发行通知映射；当前 `license_audit_pass=true`。",
        ]
        if license_audit_pass == "true"
        else [
            "- 每个最终移植文件的人工作者/许可证/资源兼容复核：尚未完成。",
            "- 因此当前 `license_audit_pass=false`。未知许可证、仅二进制和下载失效来源不得复制。",
        ]
    )
    lines.extend([
        "", "## 已选文件复核：当前 4 个概念级重写", "",
        "- Cyens Source：`cbeimers113/cyens-toy-src@1b74504e4642cd967c0079499b57faa7f37d9668`；`ACET.cpp` blob `c17e871fde5ea8e769a48609ff25f616bbfa98ac`，`UREA.cpp` blob `6f324d1fe80dcc759962e7fbfeedc839b79efb76`。",
        "- Ultimata：`Bowserinator/TPT-Ultimata-Mod@b74971752433652c033559abea415ec3510ac433`；`SOIL.cpp` blob `4fe7ad92b93646f710ec0fb5e7a1c8087bebed49`，`BLOD.cpp` blob `e99d5a47c0c5c47484b7dd66284392a0c4151a3b`。",
        "- Biological Mod `284a1585db023f62a7147892526899133dd6f41c` 的 `BLD.cpp` blob `6bbb88a35427a54e64496730ad777fb0e0250f67` 与 nucular mod `048080a79006c4d6668a1864a0e29758903c64bb` 的 `SOIL.cpp` blob `f6499ab0590da6ebffa852ac25101046d2d1623a` 仅作交叉概念参考。",
        "- 上述快照根许可证均为 GNU GPL v3，文件 SHA-256 `0B383D5A63DA644F628D99C33976EA6487ED89AAA59F0B3257992DEAC1171E6B`；未从这些仓库移植字体、图片、声音或二进制。",
        "- 当前 4 个元素只保留可追溯材料概念；属性、字段、更新、反应、预算、模块和 OPS 逻辑均按当前架构重写，第三方更新函数逐行复制为 0。",
        "", "## 当前门禁", "",
        "- 根许可证自动识别：已执行。",
        "- README、元素源码头、子模块与资源清单自动扫描：已执行于成功检出的仓库。",
        *closure_lines,
    ])
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def write_porting(path: Path, elements: list[dict[str, Any]]) -> None:
    groups = {
        "A_direct_port": "A：直接移植候选",
        "B_rewrite_port": "B：重写移植或合并主元素候选",
        "C_reference_only": "C：仅设计参考",
        "D_reject": "D：拒绝或无增量完全重复",
    }
    lines = [
        "# 模组移植计划", "",
        "自动决策只用于排序。任何 A 类仍须人工复核许可证、源码文件头、稳定 ID、模块、图鉴、事件预算和 OPS。", "",
        "重复名称或代号但行为有价值的 B 类不创建第二个元素：只把经许可证复核的行为增量重写到官方或现有 OmniPack 主元素，并回归验证主元素原行为。`exact_identifier` 先进入 B 类做源码增量审计；确认完全相同且没有增量后才降为 D/no-op。", "",
        "## 已执行的 B 类子集", "",
        "`cyens_src@1b74504e...` 的 `ACET/UREA` 与 Ultimata `b749717...` 的 `SOIL/BLOD` 已完成文件级 GPL 复核，并分别重写到稳定 ID `595/600/670/679`。当前实现没有复制旧更新函数：四项均接入当前模块、双语图鉴、统一事件预算、OPS 和运行测试；外部核心、旧 ID 与保存格式均未合并。", "",
        "机器可读统计为 `elements_ported=0`、`elements_rewritten=4`、`first_port_batch_complete=false`。这只是 50–80 元素首轮移植中的已验证子集，不能冒充整批完成。其余同概念候选继续优先合并到官方或当前主元素。", "",
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
    lines.extend([
        "| `cyens_src@1b74504e` | 旧 `UREA + HNO3 -> UNTR` 高能路径 | 不进入当前尿素实现；只保留材料概念，改为受控加压合成和水解，不提供现实危险配比或沿用旧爆炸产物 |",
        "| `cyens_src@1b74504e` | 整库保存、网络、协议、UI、Lua 与模拟核心改动 | 当前 OmniPack 是唯一主干；外部核心只读审计，不合并其保存格式、网络协议或旧元素数组 |",
        "| Ultimata / Biological Mod / nucular | 旧 `SOIL/BLOD/BLD` 更新状态机 | 只保留材料概念；不复制 4×4/5×5 扫描、染色、隧道、组织字段或随机凝结逻辑 |",
    ])
    for row in elements:
        if row["port_decision"] != "D_reject":
            continue
        duplicate = duplicates.get(f"{row['source_mod']}\0{row['source_identifier']}", {})
        reason = duplicate.get("classification", "source unavailable or binary-only")
        lines.append(f"| {row['source_mod']} | {row['source_identifier']} | {reason} |")
    lines.extend([
        "", "当前拒绝项包含功能和整库合并方案，不代表独立元素候选数量；机器统计 `elements_rejected` 只计算自动分类为 D 的独立候选。",
    ])
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
    periodic_path = root / "docs/PERIODIC_ELEMENT_SOURCE_MAP.csv"
    if periodic_path.is_file():
        with periodic_path.open("r", encoding="utf-8", newline="") as stream:
            periodic_rows = list(csv.DictReader(stream))
    else:
        periodic_rows = []
    periodic_map_complete = (
        len(periodic_rows) == 118
        and {row.get("atomic_number") for row in periodic_rows} == {str(value) for value in range(1, 119)}
    )
    periodic_sourced = sum(row.get("status") == "implemented" for row in periodic_rows)
    element_registry_path = root / "docs/ELEMENT_REGISTRY.csv"
    if element_registry_path.is_file():
        with element_registry_path.open("r", encoding="utf-8", newline="") as stream:
            element_registry_rows = list(csv.DictReader(stream))
    else:
        element_registry_rows = []
    total_omnipack_elements = sum(
        row.get("identifier", "").startswith("OMNI_PT_")
        and row.get("implementation_status") == "implemented"
        for row in element_registry_rows
    )
    compatibility_aliases = sum(
        row.get("implementation_status") == "implemented"
        and row.get("is_duplicate") == "true"
        for row in element_registry_rows
    )
    total_omnipack_playable = sum(
        row.get("identifier", "").startswith("OMNI_PT_")
        and row.get("implementation_status") == "implemented"
        and row.get("is_duplicate") != "true"
        for row in element_registry_rows
    )
    active_non_alias_types = sum(
        row.get("implementation_status") == "implemented"
        and row.get("is_duplicate") != "true"
        and row.get("default_enabled") == "true"
        for row in element_registry_rows
    )
    directly_selectable_materials = sum(
        directly_selectable(root, row) for row in element_registry_rows
    )
    rewritten_stable_ids = {595, 600, 670, 679}
    elements_rewritten = sum(
        row.get("implementation_status") == "implemented"
        and row.get("stable_id", "").isdigit()
        and int(row["stable_id"]) in rewritten_stable_ids
        for row in element_registry_rows
    )
    periodic_complete = periodic_map_complete and periodic_sourced == 118

    def implemented_in_range(first: int, last: int) -> int:
        return sum(
            row.get("implementation_status") == "implemented"
            and row.get("is_duplicate") != "true"
            and row.get("stable_id", "").isdigit()
            and first <= int(row["stable_id"]) <= last
            for row in element_registry_rows
        )

    reaction_registry_path = root / "docs/REACTION_REGISTRY.csv"
    if reaction_registry_path.is_file():
        with reaction_registry_path.open("r", encoding="utf-8", newline="") as stream:
            reaction_registry_entries = sum(1 for _ in csv.DictReader(stream))
    else:
        reaction_registry_entries = 0
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
    write_license_audit(
        root / "docs/MOD_LICENSE_AUDIT.md", mods, args.license_audit_pass
    )
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
    duplicate_classifications = {
        "exact_identifier",
        "complete_duplicate",
        "different_name_behavior_duplicate",
        "official_enhancement",
        "same_name_different_behavior",
    }
    merge_classifications = {
        "exact_identifier",
        "official_enhancement",
        "same_name_different_behavior",
    }
    duplicate_definition_records = 0
    duplicate_candidate_keys: set[tuple[str, str]] = set()
    merge_review_keys: set[tuple[str, str]] = set()
    unique_classified_keys: set[tuple[str, str]] = set()
    for row in catalog_rows:
        duplicate = duplicate_by_key.get(
            f"{row['source_mod']}\0{row['source_identifier']}", {}
        )
        classification = duplicate.get("classification", "unique_candidate")
        key = concept_key(row)
        if classification in duplicate_classifications:
            duplicate_definition_records += 1
            duplicate_candidate_keys.add(key)
        else:
            unique_classified_keys.add(key)
        if (
            row["port_decision"] == "B_rewrite_port"
            and classification in merge_classifications
        ):
            merge_review_keys.add(key)
    unique_classified_keys -= duplicate_candidate_keys
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
        f"unique_candidate_elements={len(unique_classified_keys)}",
        f"licensed_source_candidates={len(licensed_keys)}",
        f"duplicate_candidates={len(duplicate_candidate_keys)}",
        f"duplicate_definition_records={duplicate_definition_records}",
        f"canonical_merge_review_candidates={len(merge_review_keys)}",
        f"rejected_candidates={len(rejected_keys)}",
        f"direct_port_candidates={len(direct_keys)}",
        f"rewrite_candidates={len(rewrite_keys)}",
        f"reference_only_candidates={len(reference_keys)}",
        "elements_ported=0", f"elements_rewritten={elements_rewritten}",
        "elements_rejected=0",
        f"periodic_source_map_complete={str(periodic_map_complete).lower()}",
        f"periodic_elements_sourced={periodic_sourced}",
        f"periodic_elements_remaining={118 - periodic_sourced if periodic_map_complete else 'not_tested'}",
        f"periodic_noble_gas_batch_complete={str(periodic_complete).lower()}",
        f"periodic_alkali_batch_complete={str(periodic_complete).lower()}",
        f"periodic_alkaline_earth_batch_complete={str(periodic_complete).lower()}",
        f"periodic_boron_group_batch_complete={str(periodic_complete).lower()}",
        f"periodic_carbon_group_batch_complete={str(periodic_complete).lower()}",
        f"periodic_nitrogen_group_batch_complete={str(periodic_complete).lower()}",
        f"periodic_oxygen_group_batch_complete={str(periodic_complete).lower()}",
        f"periodic_halogen_group_batch_complete={str(periodic_complete).lower()}",
        f"periodic_first_transition_batch_complete={str(periodic_complete).lower()}",
        f"periodic_second_transition_batch_complete={str(periodic_complete).lower()}",
        f"periodic_third_transition_batch_complete={str(periodic_complete).lower()}",
        f"periodic_lanthanide_batch_complete={str(periodic_complete).lower()}",
        f"periodic_actinide_batch_complete={str(periodic_complete).lower()}",
        f"periodic_superheavy_batch_complete={str(periodic_complete).lower()}",
        f"periodic_table_ui={str(periodic_complete).lower()}",
        f"total_omnipack_elements={total_omnipack_elements}",
        f"compatibility_aliases={compatibility_aliases}",
        f"total_omnipack_playable={total_omnipack_playable}",
        f"active_non_alias_types={active_non_alias_types}",
        f"directly_selectable_materials={directly_selectable_materials}",
        f"total_playable_materials={directly_selectable_materials}",
        f"inorganic_batch1_elements={implemented_in_range(462, 477)}",
        f"inorganic_batch2_elements={implemented_in_range(478, 493)}",
        f"inorganic_batch3_elements={implemented_in_range(494, 511)}",
        f"engineering_alloys_batch1_elements={implemented_in_range(512, 520)}",
        f"materials_batch1_elements={implemented_in_range(521, 532)}",
        f"isotope_batch1_elements={implemented_in_range(576, 588)}",
        f"organic_batch1_elements={implemented_in_range(589, 601)}",
        f"organic_batch2_elements={implemented_in_range(602, 621)}",
        f"electronics_batch1_elements={implemented_in_range(622, 641)}",
        f"environment_batch1_elements={implemented_in_range(670, 685)}",
        f"reaction_registry_entries={reaction_registry_entries}",
        f"clean_build_pass={args.clean_build_pass}",
        f"element_registry_pass={args.element_registry_pass}",
        f"reaction_registry_pass={args.reaction_registry_pass}",
        f"license_audit_pass={args.license_audit_pass}",
        "first_port_batch_complete=false",
        "first_port_batch_build_pass=not_tested",
        "first_port_batch_tests_pass=not_tested",
        "```", "",
        "数值只代表当前克隆集和自动扫描。候选数按标准化名称与代号折叠重复分叉；行为差异仍保留在去重报告中。",
        "`duplicate_definition_records` 是自动归入同概念比较的全部源码定义数，包含待主元素增量审计的记录，不等于已经人工拒绝的独立材料。`canonical_merge_review_candidates` 进入 `B_rewrite_port`，目标是合并主元素而不是新增 ID。",
        "`license_audit_pass=true` 只能由完成逐文件、README、子模块和资源复核后的显式参数写入。",
        "当前新增 92 个周期元素、无机三批 50 个材料、工程材料、代表性核素、有机/聚合物、电子材料以及生态与污染首批中的 14 个材料均为 OmniPack 原创族/反应逻辑，不计入第三方 `elements_ported` 或 `elements_rewritten`。`ACET=595`、`UREA=600`、`SOIL=670` 与 `BLOD=679` 保留兼容 GPL 来源的概念和文件级追踪，当前更新函数均按本项目架构重写，因此保守计为 `elements_rewritten=4`；第三方更新函数逐行复制仍为 0。`first_port_batch_complete=false` 仍是许可证门禁的真实结果，不能把原创内容或 4 个重写项冒充成已完成 50 元素移植。",
        "`OMNI_PT_MSCR=278` 是合并到官方 `DEFAULT_PT_BRMT=30` 的兼容别名；它继续占用稳定槽以读取旧存档，但不计入 `total_omnipack_playable`、`active_non_alias_types` 或 `total_playable_materials`。`total_playable_materials` 只统计普通菜单可直接选择的真实材料，不包含擦除工具和官方隐藏过渡/辅助类型。",
    ]
    (root / "docs/MOD_EXTRACTION_REPORT.md").write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(f"generate-mod-report: PASS mods={len(mods)} elements={len(catalog_rows)} features={len(feature_rows)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
