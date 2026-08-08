#!/usr/bin/env python3
"""Synchronize localized element names from ELEMENT_REGISTRY.csv.

The default mode is a read-only check.  Language JSON files are only changed
when --write is supplied.  Registry/source/catalog validation failures always
prevent writes.
"""

from __future__ import annotations

import argparse
import csv
from dataclasses import dataclass, field
import io
import json
import os
from pathlib import Path
import re
import sys
import tempfile
from typing import Sequence


REQUIRED_COLUMNS = (
    "identifier",
    "english_name",
    "chinese_name",
    "stable_id",
    "element_state",
    "implementation_status",
    "source_file",
)
TOMBSTONE_ID = 146
TOMBSTONE_IDENTIFIER = "RESERVED_PT_146"
IDENTIFIER_RE = re.compile(r"^[A-Z][A-Z0-9_]*$")
ELEMENT_CONSTRUCTOR_RE = re.compile(
    r"\bvoid\s+Element::Element_([A-Z0-9_]+)\s*\("
)
ELEMENT_IDENTIFIER_RE = re.compile(r'\bIdentifier\s*=\s*"([^"]+)"')


@dataclass(frozen=True)
class RegistryElement:
    line_number: int
    stable_id: int
    identifier: str
    english_name: str
    chinese_name: str
    source_file: str
    implementation_status: str

    @property
    def description_key(self) -> str:
        return f"sim.elem.{self.identifier}"

    @property
    def name_key(self) -> str:
        return f"{self.description_key}.name"


@dataclass
class Catalog:
    label: str
    path: Path
    entries: dict[str, str] = field(default_factory=dict)
    raw_bytes: bytes = b""
    valid: bool = False


@dataclass(frozen=True)
class SyncPlan:
    label: str
    path: Path
    items: tuple[tuple[str, str], ...]
    changed: bool
    inserted: int
    updated: int
    moved: int


@dataclass
class SyncResult:
    errors: list[str] = field(default_factory=list)
    active_elements: list[RegistryElement] = field(default_factory=list)
    plans: list[SyncPlan] = field(default_factory=list)
    wrote: list[Path] = field(default_factory=list)

    @property
    def changes_needed(self) -> bool:
        return any(plan.changed for plan in self.plans)


def _reject_json_constant(value: str) -> None:
    raise ValueError(f"非法 JSON 常量 {value}")


def _read_registry(path: Path, result: SyncResult) -> list[RegistryElement]:
    try:
        raw = path.read_bytes()
    except OSError as exc:
        result.errors.append(f"无法读取登记表 `{path}`：{exc}")
        return []
    try:
        text = raw.decode("utf-8-sig")
    except UnicodeDecodeError as exc:
        result.errors.append(f"登记表不是有效 UTF-8：{exc}")
        return []

    try:
        rows = list(csv.reader(io.StringIO(text, newline=""), strict=True))
    except csv.Error as exc:
        result.errors.append(f"登记表 CSV 解析失败：{exc}")
        return []
    if not rows:
        result.errors.append("登记表为空。")
        return []

    header = rows[0]
    if not header or any(not column.strip() for column in header):
        result.errors.append("登记表表头含空列名。")
        return []
    if len(set(header)) != len(header):
        duplicates = sorted(
            {column for column in header if header.count(column) > 1}
        )
        result.errors.append(f"登记表含重复列：{', '.join(duplicates)}")
        return []
    missing_columns = [column for column in REQUIRED_COLUMNS if column not in header]
    if missing_columns:
        result.errors.append(
            "登记表缺少必需列：" + ", ".join(missing_columns)
        )
        return []

    parsed_rows: list[tuple[int, dict[str, str]]] = []
    for line_number, values in enumerate(rows[1:], start=2):
        if not values or all(not value.strip() for value in values):
            result.errors.append(f"登记表第 {line_number} 行为空。")
            continue
        if len(values) != len(header):
            result.errors.append(
                f"登记表第 {line_number} 行有 {len(values)} 列，"
                f"表头为 {len(header)} 列。"
            )
            continue
        parsed_rows.append((line_number, dict(zip(header, values))))
    if result.errors:
        return []

    seen_identifiers: dict[str, int] = {}
    seen_ids: dict[int, int] = {}
    active: list[RegistryElement] = []
    tombstone_seen = False

    for line_number, row in parsed_rows:
        identifier = row["identifier"].strip()
        if not identifier:
            result.errors.append(f"登记表第 {line_number} 行 identifier 为空。")
            continue
        if not IDENTIFIER_RE.fullmatch(identifier):
            result.errors.append(
                f"登记表第 {line_number} 行 identifier 非法：{identifier!r}。"
            )
        folded = identifier.casefold()
        if folded in seen_identifiers:
            result.errors.append(
                f"登记表 identifier 重复：第 {seen_identifiers[folded]} 行与"
                f"第 {line_number} 行均为 {identifier!r}。"
            )
        else:
            seen_identifiers[folded] = line_number

        stable_id_text = row["stable_id"].strip()
        try:
            stable_id = int(stable_id_text, 10)
        except ValueError:
            result.errors.append(
                f"登记表第 {line_number} 行 stable_id 非整数："
                f"{stable_id_text!r}。"
            )
            continue
        if stable_id < 0:
            result.errors.append(
                f"登记表第 {line_number} 行 stable_id 不能为负数。"
            )
        if stable_id in seen_ids:
            result.errors.append(
                f"登记表 stable_id 重复：第 {seen_ids[stable_id]} 行与"
                f"第 {line_number} 行均为 {stable_id}。"
            )
        else:
            seen_ids[stable_id] = line_number

        english_name_raw = row["english_name"]
        chinese_name_raw = row["chinese_name"]
        english_name = english_name_raw.strip()
        chinese_name = chinese_name_raw.strip()
        source_file = row["source_file"].strip()
        status = row["implementation_status"].strip().casefold()
        element_state = row["element_state"].strip().casefold()

        if stable_id == TOMBSTONE_ID:
            tombstone_seen = True
            if identifier != TOMBSTONE_IDENTIFIER:
                result.errors.append(
                    f"ID {TOMBSTONE_ID} 必须是 {TOMBSTONE_IDENTIFIER}，"
                    f"实际为 {identifier}。"
                )
            if element_state != "reserved" or status != "reserved":
                result.errors.append(
                    f"ID {TOMBSTONE_ID} tombstone 必须同时标记为 reserved。"
                )
            if source_file:
                result.errors.append(
                    f"ID {TOMBSTONE_ID} tombstone 不得登记源码文件。"
                )
            continue

        has_active_source = bool(source_file) and status not in {
            "planned",
            "reserved",
            "removed",
        }
        if status in {"implemented", "partial", "disabled"} and not source_file:
            result.errors.append(
                f"登记表第 {line_number} 行状态为 {status}，但 source_file 为空。"
            )
        if not has_active_source:
            continue

        if not english_name:
            result.errors.append(
                f"登记表第 {line_number} 行 active 元素英文名称为空。"
            )
        if not chinese_name:
            result.errors.append(
                f"登记表第 {line_number} 行 active 元素中文名称为空。"
            )
        if english_name_raw != english_name:
            result.errors.append(
                f"登记表第 {line_number} 行英文名称含首尾空白。"
            )
        if chinese_name_raw != chinese_name:
            result.errors.append(
                f"登记表第 {line_number} 行中文名称含首尾空白。"
            )
        active.append(
            RegistryElement(
                line_number=line_number,
                stable_id=stable_id,
                identifier=identifier,
                english_name=english_name,
                chinese_name=chinese_name,
                source_file=source_file,
                implementation_status=status,
            )
        )

    if not tombstone_seen:
        result.errors.append(
            f"登记表缺少 ID {TOMBSTONE_ID} tombstone。"
        )
    return sorted(active, key=lambda item: item.stable_id)


def _safe_source_path(
    source_root: Path, relative: str, result: SyncResult
) -> Path | None:
    candidate = Path(relative)
    if candidate.is_absolute():
        result.errors.append(f"source_file 必须是仓库相对路径：{relative!r}。")
        return None
    resolved = (source_root / candidate).resolve()
    try:
        resolved.relative_to(source_root.resolve())
    except ValueError:
        result.errors.append(f"source_file 越出仓库：{relative!r}。")
        return None
    return resolved


def _source_identifiers(source_root: Path, result: SyncResult) -> set[str]:
    elements_dir = source_root / "src" / "simulation" / "elements"
    if not elements_dir.is_dir():
        result.errors.append(f"元素源码目录不存在：`{elements_dir}`。")
        return set()
    found: set[str] = set()
    for path in sorted(elements_dir.glob("*.cpp")):
        try:
            text = path.read_text(encoding="utf-8")
        except (OSError, UnicodeDecodeError) as exc:
            result.errors.append(f"无法读取元素源码 `{path}`：{exc}")
            continue
        constructors = ELEMENT_CONSTRUCTOR_RE.findall(text)
        if not constructors:
            continue
        identifiers = ELEMENT_IDENTIFIER_RE.findall(text)
        if len(constructors) != 1 or len(identifiers) != 1:
            result.errors.append(
                f"元素源码 `{path}` 必须恰有一个构造函数和一个 Identifier。"
            )
            continue
        identifier = identifiers[0]
        if identifier in found:
            result.errors.append(f"源码 Identifier 重复：{identifier}。")
        found.add(identifier)
    return found


def _validate_sources(
    source_root: Path,
    elements: Sequence[RegistryElement],
    result: SyncResult,
) -> None:
    registry_identifiers = {element.identifier for element in elements}
    source_identifiers = _source_identifiers(source_root, result)
    missing_registry = sorted(source_identifiers - registry_identifiers)
    extra_registry = sorted(registry_identifiers - source_identifiers)
    if missing_registry:
        result.errors.append(
            "源码 active 元素未登记：" + ", ".join(missing_registry)
        )
    if extra_registry:
        result.errors.append(
            "登记表 active 元素没有对应源码：" + ", ".join(extra_registry)
        )

    for element in elements:
        source_path = _safe_source_path(source_root, element.source_file, result)
        if source_path is None:
            continue
        if not source_path.is_file():
            result.errors.append(
                f"{element.identifier} 的 source_file 不存在："
                f"`{element.source_file}`。"
            )
            continue
        try:
            text = source_path.read_text(encoding="utf-8")
        except (OSError, UnicodeDecodeError) as exc:
            result.errors.append(
                f"无法读取 {element.identifier} 的源码：{exc}"
            )
            continue
        identifiers = ELEMENT_IDENTIFIER_RE.findall(text)
        if identifiers != [element.identifier]:
            result.errors.append(
                f"{element.identifier} 的 source_file Identifier 不一致："
                f"{identifiers!r}。"
            )


def _load_catalog(path: Path, label: str, result: SyncResult) -> Catalog:
    catalog = Catalog(label=label, path=path)
    try:
        raw = path.read_bytes()
    except OSError as exc:
        result.errors.append(f"无法读取 {label}：{exc}")
        return catalog
    catalog.raw_bytes = raw
    try:
        text = raw.decode("utf-8-sig")
    except UnicodeDecodeError as exc:
        result.errors.append(f"{label} 不是有效 UTF-8：{exc}")
        return catalog

    duplicate_keys: list[str] = []

    def object_pairs_hook(pairs: list[tuple[str, object]]) -> dict[str, object]:
        obj: dict[str, object] = {}
        for key, value in pairs:
            if key in obj:
                duplicate_keys.append(key)
            obj[key] = value
        return obj

    try:
        parsed = json.loads(
            text,
            object_pairs_hook=object_pairs_hook,
            parse_constant=_reject_json_constant,
        )
    except (json.JSONDecodeError, ValueError) as exc:
        result.errors.append(f"{label} 严格 JSON 解析失败：{exc}")
        return catalog
    if duplicate_keys:
        result.errors.append(
            f"{label} 有重复键：" + ", ".join(sorted(set(duplicate_keys)))
        )
        return catalog
    if not isinstance(parsed, dict) or any(
        not isinstance(value, str) for value in parsed.values()
    ):
        result.errors.append(f"{label} 必须是平面字符串 JSON 对象。")
        return catalog
    catalog.entries = dict(parsed)
    catalog.valid = True
    return catalog


def _missing_descriptions(
    catalog: Catalog,
    elements: Sequence[RegistryElement],
) -> list[str]:
    return [
        element.description_key
        for element in elements
        if not catalog.entries.get(element.description_key, "").strip()
    ]


def _plan_catalog(
    catalog: Catalog,
    elements: Sequence[RegistryElement],
    name_selector: str,
) -> SyncPlan:
    names_by_description: dict[str, tuple[str, str]] = {}
    for element in elements:
        value = (
            element.english_name
            if name_selector == "english"
            else element.chinese_name
        )
        names_by_description[element.description_key] = (element.name_key, value)

    expected_name_keys = {
        name_key
        for name_key, _ in names_by_description.values()
    }
    original_items = tuple(catalog.entries.items())
    original_keys = [key for key, _ in original_items]
    original_positions = {key: index for index, key in enumerate(original_keys)}
    desired_items: list[tuple[str, str]] = []
    inserted = 0
    updated = 0
    moved = 0

    for key, value in original_items:
        if key in expected_name_keys:
            continue
        desired_items.append((key, value))
        expected = names_by_description.get(key)
        if expected is None:
            continue
        name_key, name_value = expected
        if name_key not in catalog.entries:
            inserted += 1
        else:
            if catalog.entries[name_key] != name_value:
                updated += 1
            if original_positions[name_key] != original_positions[key] + 1:
                moved += 1
        desired_items.append((name_key, name_value))

    desired = tuple(desired_items)
    return SyncPlan(
        label=catalog.label,
        path=catalog.path,
        items=desired,
        changed=desired != original_items,
        inserted=inserted,
        updated=updated,
        moved=moved,
    )


def _atomic_write_json(plan: SyncPlan) -> None:
    payload = (
        json.dumps(
            dict(plan.items),
            ensure_ascii=False,
            indent=2,
            allow_nan=False,
        )
        + "\n"
    )
    temporary_name: str | None = None
    try:
        with tempfile.NamedTemporaryFile(
            mode="w",
            encoding="utf-8",
            newline="\n",
            dir=plan.path.parent,
            prefix=f".{plan.path.name}.",
            suffix=".tmp",
            delete=False,
        ) as temporary:
            temporary.write(payload)
            temporary.flush()
            os.fsync(temporary.fileno())
            temporary_name = temporary.name
        os.replace(temporary_name, plan.path)
        temporary_name = None
    finally:
        if temporary_name is not None:
            try:
                Path(temporary_name).unlink()
            except OSError:
                pass


def synchronize(
    source_root: Path,
    registry_path: Path,
    en_path: Path,
    zh_path: Path,
    *,
    write: bool,
) -> SyncResult:
    result = SyncResult()
    elements = _read_registry(registry_path, result)
    result.active_elements = elements
    _validate_sources(source_root, elements, result)
    en = _load_catalog(en_path, "en-US", result)
    zh = _load_catalog(zh_path, "zh-CN", result)

    if en.valid:
        missing = _missing_descriptions(en, elements)
        if missing:
            result.errors.append(
                f"en-US 缺少或清空了 {len(missing)} 个元素短说明："
                + ", ".join(missing[:12])
                + (f"，另有 {len(missing) - 12} 项" if len(missing) > 12 else "")
            )
    if zh.valid:
        missing = _missing_descriptions(zh, elements)
        if missing:
            result.errors.append(
                f"zh-CN 缺少或清空了 {len(missing)} 个元素短说明："
                + ", ".join(missing[:12])
                + (f"，另有 {len(missing) - 12} 项" if len(missing) > 12 else "")
            )

    if result.errors or not (en.valid and zh.valid):
        return result

    result.plans = [
        _plan_catalog(en, elements, "english"),
        _plan_catalog(zh, elements, "chinese"),
    ]
    if write:
        for plan in result.plans:
            if not plan.changed:
                continue
            try:
                _atomic_write_json(plan)
            except OSError as exc:
                result.errors.append(f"无法写入 {plan.label}：{exc}")
                return result
            result.wrote.append(plan.path)
    return result


def _default_source_root() -> Path:
    return Path(__file__).resolve().parents[1]


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="从 ELEMENT_REGISTRY.csv 检查或同步元素正式名称。"
    )
    parser.add_argument(
        "--source-root",
        type=Path,
        default=_default_source_root(),
        help="仓库根目录；默认从脚本路径推导。",
    )
    parser.add_argument(
        "--registry",
        type=Path,
        help="登记表路径；默认 SOURCE_ROOT/docs/ELEMENT_REGISTRY.csv。",
    )
    parser.add_argument(
        "--en",
        type=Path,
        help="英文语言文件；默认 SOURCE_ROOT/src/lang/en-US.json。",
    )
    parser.add_argument(
        "--zh",
        type=Path,
        help="中文语言文件；默认 SOURCE_ROOT/src/lang/zh-CN.json。",
    )
    parser.add_argument(
        "--write",
        action="store_true",
        help="显式机械更新语言 JSON；未提供时只检查且绝不写文件。",
    )
    return parser


def _render_summary(result: SyncResult, write: bool) -> str:
    lines = [
        f"element localization sync: active={len(result.active_elements)}, "
        f"mode={'write' if write else 'check'}"
    ]
    for error in result.errors:
        lines.append(f"ERROR {error}")
    for plan in result.plans:
        lines.append(
            f"{plan.label}: changed={str(plan.changed).lower()}, "
            f"inserted={plan.inserted}, updated={plan.updated}, moved={plan.moved}"
        )
    if result.wrote:
        lines.append("written: " + ", ".join(str(path) for path in result.wrote))
    if not result.errors and not result.changes_needed:
        lines.append("PASS element names already match the registry")
    elif not result.errors and write:
        lines.append("PASS element names synchronized")
    elif not result.errors:
        lines.append("FAIL synchronization required; rerun with --write")
    return "\n".join(lines)


def main(argv: Sequence[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    source_root = args.source_root.resolve()
    registry_path = (
        args.registry or source_root / "docs" / "ELEMENT_REGISTRY.csv"
    ).resolve()
    en_path = (
        args.en or source_root / "src" / "lang" / "en-US.json"
    ).resolve()
    zh_path = (
        args.zh or source_root / "src" / "lang" / "zh-CN.json"
    ).resolve()
    result = synchronize(
        source_root,
        registry_path,
        en_path,
        zh_path,
        write=args.write,
    )
    print(_render_summary(result, args.write))
    if result.errors:
        return 1
    if result.changes_needed and not args.write:
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
