#!/usr/bin/env python3
"""Strict, reproducible en-US/zh-CN localization audit for TPT-ZH-OmniPack.

The default mode is read-only and reports findings to stdout.  A report is only
written when --write-report PATH is supplied.  --check makes release-blocking
findings produce exit status 1; warnings never change the exit status.
"""

from __future__ import annotations

import argparse
from collections import Counter
import csv
from dataclasses import dataclass, field
from datetime import datetime, timezone
import io
import json
from pathlib import Path
import re
import subprocess
import sys
import unicodedata
from typing import Iterable, Sequence


SEVERITY_ERROR = "error"
SEVERITY_WARNING = "warning"

TPT_COLOUR_CODES = frozenset("wgorlbtuU")
LINK_RE = re.compile(r"\{a:([^{}|]+)\|([^{}]*)\}")
PRINTF_RE = re.compile(
    r"%(?!%)"
    r"(?:\d+\$)?"
    r"[-+#0'I]*"
    r"(?:\*|\d+)?"
    r"(?:\.(?:\*|\d+))?"
    r"(?:hh|h|ll|l|j|z|t|L)?"
    r"[diuoxXfFeEgGaAcspn]"
)
BRACE_PLACEHOLDER_RE = re.compile(
    r"(?<!\{)\{"
    r"(?P<field>\d+|[A-Za-z_][A-Za-z0-9_.]*)"
    r"(?:![rsa])?"
    r"(?::[^{}]*)?"
    r"\}(?!\})"
)
ELEMENT_CONSTRUCTOR_RE = re.compile(
    r"\bvoid\s+Element::Element_([A-Z0-9_]+)\s*\("
)
ELEMENT_IDENTIFIER_RE = re.compile(r'\bIdentifier\s*=\s*"([^"]+)"')
ELEMENT_DESCRIPTION_RE = re.compile(
    r'\bDescription\s*=\s*Localization::Ref\(\)\.Tr\(\s*"([^"]+)"',
    re.MULTILINE,
)
MENU_KEY_RE = re.compile(r'\bString\("(sim\.menu\.[^"]+)"\)')
LITERAL_TR_KEY_RE = re.compile(
    r'Localization::Ref\(\)\.Tr\(\s*"([^"]+)"', re.MULTILINE
)
URL_RE = re.compile(r"(?:https?://|irc\.)\S+", re.IGNORECASE)
LATIN_WORD_RE = re.compile(r"[A-Za-z]{3,}")
CJK_RE = re.compile(r"[\u3400-\u4DBF\u4E00-\u9FFF\uF900-\uFAFF]")
STRONG_MOJIBAKE_RE = re.compile(
    r"\uFFFD|锟斤拷|烫烫烫|屯屯屯|ï¿½", re.IGNORECASE
)
LIKELY_LATIN1_MOJIBAKE_RE = re.compile(
    r"(?:Ã[\x80-\u024F]|Â[\x80-\u024F]|â.|ðŸ)"
)

MAX_FINDINGS_IN_REPORT = 300

ENCYCLOPEDIA_REGISTRY_COLUMNS = (
    "identifier",
    "menu_category",
    "element_state",
    "save_compatibility",
    "implementation_status",
    "test_status",
)
ENCYCLOPEDIA_ENUM_TOKEN_RE = re.compile(r"^[A-Za-z0-9_-]+$")
MENU_CATEGORY_LOCALIZATION_KEYS = {
    "SC_WALL": "sim.menu.walls",
    "SC_ELEC": "sim.menu.electronics",
    "SC_POWERED": "sim.menu.powered",
    "SC_SENSOR": "sim.menu.sensors",
    "SC_FORCE": "sim.menu.force",
    "SC_EXPLOSIVE": "sim.menu.explosives",
    "SC_GAS": "sim.menu.gases",
    "SC_LIQUID": "sim.menu.liquids",
    "SC_POWDERS": "sim.menu.powders",
    "SC_SOLIDS": "sim.menu.solids",
    "SC_NUCLEAR": "sim.menu.radioactive",
    "SC_SPECIAL": "sim.menu.special",
    "SC_LIFE": "sim.menu.gol",
    "SC_TOOL": "sim.menu.tools",
    "SC_FAVORITES": "sim.menu.favorites",
    "SC_DECO": "sim.menu.deco",
    "RESERVED": "encyclopedia.value.category.RESERVED",
}
ENCYCLOPEDIA_ENUM_PREFIXES = {
    "element_state": "encyclopedia.value.state.",
    "save_compatibility": "encyclopedia.value.save.",
    "implementation_status": "encyclopedia.value.implementation.",
    "test_status": "encyclopedia.value.test.",
}


@dataclass(frozen=True)
class Finding:
    severity: str
    code: str
    message: str
    key: str | None = None

    def render(self) -> str:
        location = f" `{self.key}`" if self.key else ""
        return f"`{self.code}`{location}：{self.message}"


@dataclass
class Catalog:
    label: str
    path: Path
    entries: dict[str, str] = field(default_factory=dict)
    duplicate_keys: list[str] = field(default_factory=list)
    valid: bool = False


@dataclass
class AuditResult:
    source_root: Path
    en_path: Path
    zh_path: Path
    findings: list[Finding] = field(default_factory=list)
    stats: dict[str, int] = field(default_factory=dict)

    @property
    def errors(self) -> list[Finding]:
        return [item for item in self.findings if item.severity == SEVERITY_ERROR]

    @property
    def warnings(self) -> list[Finding]:
        return [
            item for item in self.findings if item.severity == SEVERITY_WARNING
        ]

    def add(
        self,
        severity: str,
        code: str,
        message: str,
        key: str | None = None,
    ) -> None:
        self.findings.append(Finding(severity, code, message, key))


def _reject_json_constant(value: str) -> None:
    raise ValueError(f"JSON 非法常量 {value}")


def _summarise_keys(keys: Iterable[str], limit: int = 12) -> str:
    ordered = sorted(set(keys))
    shown = ", ".join(f"`{key}`" for key in ordered[:limit])
    if len(ordered) > limit:
        shown += f"，另有 {len(ordered) - limit} 项"
    return shown or "无"


def _load_catalog(path: Path, label: str, result: AuditResult) -> Catalog:
    catalog = Catalog(label=label, path=path)
    try:
        raw = path.read_bytes()
    except OSError as exc:
        result.add(
            SEVERITY_ERROR,
            "json.read",
            f"无法读取 {label}：{exc}",
        )
        return catalog

    if raw.startswith(b"\xef\xbb\xbf"):
        result.add(
            SEVERITY_WARNING,
            "json.utf8_bom",
            f"{label} 含 UTF-8 BOM；当前可解析，但建议保存为无 BOM UTF-8。",
        )
    try:
        text = raw.decode("utf-8-sig")
    except UnicodeDecodeError as exc:
        result.add(
            SEVERITY_ERROR,
            "json.utf8",
            f"{label} 不是有效 UTF-8：{exc}",
        )
        return catalog

    duplicates: list[str] = []

    def object_pairs_hook(pairs: list[tuple[str, object]]) -> dict[str, object]:
        obj: dict[str, object] = {}
        for key, value in pairs:
            if key in obj:
                duplicates.append(key)
            obj[key] = value
        return obj

    try:
        parsed = json.loads(
            text,
            object_pairs_hook=object_pairs_hook,
            parse_constant=_reject_json_constant,
        )
    except (json.JSONDecodeError, ValueError) as exc:
        result.add(
            SEVERITY_ERROR,
            "json.syntax",
            f"{label} 严格 JSON 解析失败：{exc}",
        )
        return catalog

    catalog.duplicate_keys = duplicates
    if duplicates:
        result.add(
            SEVERITY_ERROR,
            "json.duplicate_key",
            f"{label} 有 {len(duplicates)} 个重复键：{_summarise_keys(duplicates)}",
        )

    if not isinstance(parsed, dict):
        result.add(
            SEVERITY_ERROR,
            "json.top_level",
            f"{label} 顶层必须是 JSON 对象，实际为 {type(parsed).__name__}。",
        )
        return catalog

    invalid_values = [
        key for key, value in parsed.items() if not isinstance(value, str)
    ]
    if invalid_values:
        result.add(
            SEVERITY_ERROR,
            "json.flat_string_object",
            f"{label} 必须是平面字符串对象；非字符串值："
            f"{_summarise_keys(invalid_values)}",
        )
        return catalog

    blank_keys = [key for key in parsed if not key.strip()]
    if blank_keys:
        result.add(
            SEVERITY_ERROR,
            "json.blank_key",
            f"{label} 含空白键，共 {len(blank_keys)} 个。",
        )

    catalog.entries = dict(parsed)
    catalog.valid = True
    return catalog


def _colour_tokens(value: str) -> tuple[tuple[str, ...], tuple[str, ...]]:
    tokens: list[str] = []
    invalid: list[str] = []
    index = 0
    while index < len(value):
        if (
            value[index] == "\\"
            and index + 1 < len(value)
            and value[index + 1] == "b"
        ):
            if index + 2 >= len(value):
                invalid.append(r"\b<eof>")
                index += 2
                continue
            code = value[index + 2]
            if code in TPT_COLOUR_CODES:
                tokens.append(code)
            else:
                invalid.append(r"\b" + code)
            index += 3
            continue
        if value[index] == "\x08":
            if index + 1 >= len(value):
                invalid.append("0x08<eof>")
                index += 1
                continue
            code = value[index + 1]
            if code in TPT_COLOUR_CODES:
                tokens.append(code)
            else:
                invalid.append(f"0x08{code}")
            index += 2
            continue
        index += 1
    return tuple(tokens), tuple(invalid)


def _end_control_positions(value: str) -> tuple[int, ...]:
    positions: list[int] = []
    index = 0
    while index < len(value):
        if value[index] == "\x0e":
            positions.append(index)
            index += 1
            continue
        if value.startswith(r"\x0E", index):
            positions.append(index)
            index += 5
            continue
        index += 1
    return tuple(positions)


def _newline_signature(value: str) -> tuple[int, bool, bool, tuple[int, ...]]:
    runs = tuple(len(match.group(0)) for match in re.finditer(r"\n+", value))
    return value.count("\n"), value.startswith("\n"), value.endswith("\n"), runs


def _links(
    value: str,
) -> tuple[tuple[tuple[str, str], ...], tuple[str, ...]]:
    matches = list(LINK_RE.finditer(value))
    malformed: list[str] = []
    if value.count("{a:") != len(matches):
        malformed.append("存在未闭合或格式错误的 `{a:URL|文本}` 标记")
    for match in matches:
        url, label = match.groups()
        if not url.strip():
            malformed.append("链接 URL 为空")
        if not label:
            malformed.append(f"链接 `{url}` 的显示文本为空")
        suffix = value[match.end() :]
        if not (suffix.startswith(r"\x0E") or suffix.startswith("\x0e")):
            malformed.append(f"链接 `{url}` 后缺少结束控制 `\\x0E`/0x0E")
    return tuple(match.groups() for match in matches), tuple(malformed)


def _placeholder_tokens(value: str) -> Counter[str]:
    without_links = LINK_RE.sub("", value)
    tokens: list[str] = []
    index = 0
    while index < len(without_links):
        if without_links.startswith("%%", index):
            tokens.append("printf:%%")
            index += 2
            continue
        match = PRINTF_RE.match(without_links, index)
        if match:
            tokens.append("printf:" + match.group(0))
            index = match.end()
            continue
        index += 1
    for match in BRACE_PLACEHOLDER_RE.finditer(without_links):
        tokens.append("brace:" + match.group(0))
    tokens.extend(["brace:{{"] * without_links.count("{{"))
    tokens.extend(["brace:}}"] * without_links.count("}}"))
    return Counter(tokens)


def _clean_visible_text(value: str) -> str:
    value = LINK_RE.sub(lambda match: match.group(2), value)
    value = re.sub(r"\\b[wgorlbtuU]", "", value)
    value = re.sub("\x08[wgorlbtuU]", "", value)
    value = value.replace(r"\x0E", "").replace("\x0e", "")
    value = URL_RE.sub("", value)
    return value


def _display_width(value: str) -> int:
    maximum = 0
    for line in _clean_visible_text(value).splitlines() or [""]:
        width = 0
        for char in line:
            if unicodedata.combining(char):
                continue
            width += 2 if unicodedata.east_asian_width(char) in {"W", "F"} else 1
        maximum = max(maximum, width)
    return maximum


def _width_limit_for_key(key: str) -> int | None:
    lower = key.lower()
    if key.startswith("sim.menu."):
        return 20
    if re.search(r"(?:^|[._])(short|button|tab|title)(?:$|[._])", lower):
        return 32
    if re.search(
        r"(?:^|[._])(ok|yes|no|cancel|close|next|back|label)(?:$|[._])",
        lower,
    ):
        return 24
    return None


def _has_unpaired_surrogate(value: str) -> bool:
    return any(0xD800 <= ord(char) <= 0xDFFF for char in value)


def _audit_value_health(
    label: str,
    entries: dict[str, str],
    result: AuditResult,
) -> None:
    for key, value in entries.items():
        if STRONG_MOJIBAKE_RE.search(value) or _has_unpaired_surrogate(value):
            result.add(
                SEVERITY_ERROR,
                "text.mojibake",
                f"{label} 含替换字符、已知乱码序列或未配对代理项。",
                key,
            )
        elif LIKELY_LATIN1_MOJIBAKE_RE.search(value):
            result.add(
                SEVERITY_WARNING,
                "text.mojibake_suspect",
                f"{label} 疑似 UTF-8/Latin-1 错误解码，需人工复核。",
                key,
            )

        bad_controls = sorted(
            {
                f"U+{ord(char):04X}"
                for char in value
                if ord(char) < 0x20
                and char not in {"\x08", "\x09", "\x0a", "\x0d", "\x0e"}
            }
        )
        if bad_controls:
            result.add(
                SEVERITY_ERROR,
                "text.control_character",
                f"{label} 含不允许的控制字符：{', '.join(bad_controls)}。",
                key,
            )

        _, invalid_colours = _colour_tokens(value)
        if invalid_colours:
            result.add(
                SEVERITY_ERROR,
                "control.invalid_colour",
                f"{label} 含非法 TPT 颜色控制："
                f"{', '.join(repr(item) for item in invalid_colours)}。",
                key,
            )

        _, malformed_links = _links(value)
        if malformed_links:
            result.add(
                SEVERITY_ERROR,
                "control.invalid_link",
                f"{label} 链接控制损坏：{'；'.join(malformed_links)}。",
                key,
            )


def _audit_common_values(
    en: Catalog,
    zh: Catalog,
    result: AuditResult,
) -> None:
    common_keys = sorted(en.entries.keys() & zh.entries.keys())
    for key in common_keys:
        en_value = en.entries[key]
        zh_value = zh.entries[key]

        en_placeholders = _placeholder_tokens(en_value)
        zh_placeholders = _placeholder_tokens(zh_value)
        if en_placeholders != zh_placeholders:
            result.add(
                SEVERITY_ERROR,
                "format.placeholder_mismatch",
                f"占位符不一致：en={dict(en_placeholders)}，"
                f"zh={dict(zh_placeholders)}。",
                key,
            )

        en_colours, _ = _colour_tokens(en_value)
        zh_colours, _ = _colour_tokens(zh_value)
        if en_colours != zh_colours:
            result.add(
                SEVERITY_ERROR,
                "control.colour_mismatch",
                f"颜色控制顺序不一致：en={en_colours}，zh={zh_colours}。",
                key,
            )

        en_newlines = _newline_signature(en_value)
        zh_newlines = _newline_signature(zh_value)
        if en_newlines != zh_newlines:
            result.add(
                SEVERITY_ERROR,
                "control.newline_mismatch",
                f"显式换行结构不一致：en={en_newlines}，zh={zh_newlines}。",
                key,
            )

        en_links, _ = _links(en_value)
        zh_links, _ = _links(zh_value)
        en_urls = tuple(url for url, _ in en_links)
        zh_urls = tuple(url for url, _ in zh_links)
        if en_urls != zh_urls:
            result.add(
                SEVERITY_ERROR,
                "control.link_mismatch",
                f"链接目标不一致：en={en_urls}，zh={zh_urls}。",
                key,
            )

        en_ends = len(_end_control_positions(en_value))
        zh_ends = len(_end_control_positions(zh_value))
        if en_ends != zh_ends:
            result.add(
                SEVERITY_ERROR,
                "control.end_mismatch",
                f"结束控制数量不一致：en={en_ends}，zh={zh_ends}。",
                key,
            )


def _audit_untranslated_and_width(
    en: Catalog,
    zh: Catalog,
    result: AuditResult,
) -> None:
    for key in sorted(en.entries.keys() & zh.entries.keys()):
        en_value = en.entries[key]
        zh_value = zh.entries[key]
        if en_value and en_value == zh_value:
            result.add(
                SEVERITY_WARNING,
                "translation.identical",
                "英中值完全相同；可能是代号、URL、格式片段或未翻译文本，需人工分类。",
                key,
            )
        else:
            visible = _clean_visible_text(zh_value)
            latin_words = LATIN_WORD_RE.findall(visible)
            allowed_by_key = (
                key.startswith("sim.gol.")
                or key.startswith("options.deco.")
                or key in {
                    "intro.title_after_version",
                    "login.use_account_not_email_suffix",
                    "gameview.sample.tmp",
                    "gameview.sample.tmp2",
                }
            )
            if (
                latin_words
                and not CJK_RE.search(visible)
                and not allowed_by_key
                and not re.fullmatch(r"[\s\d\W_A-Z]+", visible)
            ):
                result.add(
                    SEVERITY_WARNING,
                    "translation.english_suspect",
                    f"中文值无中日韩文字且含英文词："
                    f"{', '.join(latin_words[:6])}。",
                    key,
                )

        limit = _width_limit_for_key(key)
        if limit is not None:
            width = _display_width(zh_value)
            if width > limit:
                result.add(
                    SEVERITY_WARNING,
                    "layout.width_risk",
                    f"启发式显示宽度 {width} 超过窄 UI 阈值 {limit}；"
                    "必须以实际字体截图复核。",
                    key,
                )


def _read_source(path: Path, result: AuditResult) -> str | None:
    try:
        return path.read_text(encoding="utf-8")
    except (OSError, UnicodeDecodeError) as exc:
        result.add(
            SEVERITY_ERROR,
            "source.read",
            f"无法读取源码 `{path}`：{exc}",
        )
        return None


def _record_missing_group(
    result: AuditResult,
    code: str,
    description: str,
    keys: Iterable[str],
) -> None:
    missing = sorted(set(keys))
    if missing:
        result.add(
            SEVERITY_ERROR,
            code,
            f"{description}，共 {len(missing)} 项：{_summarise_keys(missing)}",
        )


def _audit_source_registration(
    en: Catalog,
    zh: Catalog,
    result: AuditResult,
) -> None:
    source_root = result.source_root
    elements_dir = source_root / "src" / "simulation" / "elements"
    simulation_data = source_root / "src" / "simulation" / "SimulationData.cpp"
    src_dir = source_root / "src"

    registered_description_keys: set[str] = set()
    registered_name_keys: set[str] = set()
    registered_elements: set[str] = set()

    if not elements_dir.is_dir():
        result.add(
            SEVERITY_ERROR,
            "source.elements_missing",
            f"元素源码目录不存在：`{elements_dir}`。",
        )
    else:
        for path in sorted(elements_dir.glob("*.cpp")):
            text = _read_source(path, result)
            if text is None:
                continue
            constructors = ELEMENT_CONSTRUCTOR_RE.findall(text)
            if not constructors:
                continue
            identifiers = ELEMENT_IDENTIFIER_RE.findall(text)
            descriptions = ELEMENT_DESCRIPTION_RE.findall(text)
            if len(constructors) != 1:
                result.add(
                    SEVERITY_ERROR,
                    "source.element_constructor",
                    f"`{path.name}` 应恰有一个元素构造函数，实际为 "
                    f"{len(constructors)}。",
                )
                continue
            if len(identifiers) != 1:
                result.add(
                    SEVERITY_ERROR,
                    "source.element_identifier",
                    f"`{path.name}` 应恰有一个字符串 Identifier，实际为 "
                    f"{len(identifiers)}。",
                )
                continue
            identifier = identifiers[0]
            registered_elements.add(identifier)
            expected_description = f"sim.elem.{identifier}"
            expected_name = f"{expected_description}.name"
            registered_description_keys.add(expected_description)
            registered_name_keys.add(expected_name)
            if len(descriptions) != 1:
                result.add(
                    SEVERITY_ERROR,
                    "source.element_description",
                    f"`{path.name}` 的 Description 必须恰好使用一个字面量"
                    "本地化键。",
                    identifier,
                )
            elif descriptions[0] != expected_description:
                result.add(
                    SEVERITY_ERROR,
                    "source.element_description_key",
                    f"说明键应为 `{expected_description}`，实际为 "
                    f"`{descriptions[0]}`。",
                    identifier,
                )

    registered_menu_keys: set[str] = set()
    if not simulation_data.is_file():
        result.add(
            SEVERITY_ERROR,
            "source.menu_registry_missing",
            f"菜单登记源码不存在：`{simulation_data}`。",
        )
    else:
        text = _read_source(simulation_data, result)
        if text is not None:
            registered_menu_keys.update(MENU_KEY_RE.findall(text))
            if not registered_menu_keys:
                result.add(
                    SEVERITY_ERROR,
                    "source.menu_registry_empty",
                    "`SimulationData.cpp` 中没有找到 `sim.menu.*` 登记。",
                )

    literal_tr_keys: set[str] = set()
    if not src_dir.is_dir():
        result.add(
            SEVERITY_ERROR,
            "source.root_missing",
            f"源码目录不存在：`{src_dir}`。",
        )
    else:
        for pattern in ("*.cpp", "*.h"):
            for path in src_dir.rglob(pattern):
                text = _read_source(path, result)
                if text is not None:
                    literal_tr_keys.update(LITERAL_TR_KEY_RE.findall(text))

    _record_missing_group(
        result,
        "registration.element_description_en",
        "英文缺少已登记元素的短说明键",
        registered_description_keys - en.entries.keys(),
    )
    _record_missing_group(
        result,
        "registration.element_description_zh",
        "中文缺少已登记元素的短说明键",
        registered_description_keys - zh.entries.keys(),
    )
    _record_missing_group(
        result,
        "registration.element_name_en",
        "英文缺少已登记元素的正式名称键",
        registered_name_keys - en.entries.keys(),
    )
    _record_missing_group(
        result,
        "registration.element_name_zh",
        "中文缺少已登记元素的正式名称键",
        registered_name_keys - zh.entries.keys(),
    )
    _record_missing_group(
        result,
        "registration.menu_en",
        "英文缺少已登记菜单键",
        registered_menu_keys - en.entries.keys(),
    )
    _record_missing_group(
        result,
        "registration.menu_zh",
        "中文缺少已登记菜单键",
        registered_menu_keys - zh.entries.keys(),
    )
    _record_missing_group(
        result,
        "registration.literal_tr_en",
        "英文缺少源码字面量 `Tr()` 键",
        literal_tr_keys - en.entries.keys(),
    )
    _record_missing_group(
        result,
        "registration.literal_tr_zh",
        "中文缺少源码字面量 `Tr()` 键",
        literal_tr_keys - zh.entries.keys(),
    )

    catalog_element_descriptions = {
        key
        for key in en.entries
        if re.fullmatch(r"sim\.elem\.[^.]+", key)
    }
    orphan_descriptions = catalog_element_descriptions - registered_description_keys
    if orphan_descriptions:
        result.add(
            SEVERITY_WARNING,
            "registration.element_orphan",
            f"英文有 {len(orphan_descriptions)} 个未对应源码元素的说明键："
            f"{_summarise_keys(orphan_descriptions)}",
        )

    catalog_menus = {key for key in en.entries if key.startswith("sim.menu.")}
    orphan_menus = catalog_menus - registered_menu_keys
    if orphan_menus:
        result.add(
            SEVERITY_WARNING,
            "registration.menu_orphan",
            f"英文有 {len(orphan_menus)} 个未对应菜单登记的键："
            f"{_summarise_keys(orphan_menus)}",
        )

    result.stats.update(
        {
            "registered_elements": len(registered_elements),
            "registered_element_descriptions": len(registered_description_keys),
            "registered_element_names": len(registered_name_keys),
            "element_description_missing_en": len(
                registered_description_keys - en.entries.keys()
            ),
            "element_description_missing_zh": len(
                registered_description_keys - zh.entries.keys()
            ),
            "element_name_missing_en": len(
                registered_name_keys - en.entries.keys()
            ),
            "element_name_missing_zh": len(
                registered_name_keys - zh.entries.keys()
            ),
            "registered_menus": len(registered_menu_keys),
            "menu_missing_en": len(registered_menu_keys - en.entries.keys()),
            "menu_missing_zh": len(registered_menu_keys - zh.entries.keys()),
            "literal_tr_keys": len(literal_tr_keys),
            "literal_tr_missing_en": len(literal_tr_keys - en.entries.keys()),
            "literal_tr_missing_zh": len(literal_tr_keys - zh.entries.keys()),
            "orphan_element_descriptions": len(orphan_descriptions),
            "orphan_menus": len(orphan_menus),
        }
    )


def _set_encyclopedia_registry_stats(
    result: AuditResult,
    *,
    rows: int = 0,
    expected: int = 0,
    missing_en: int = 0,
    missing_zh: int = 0,
    unknown_categories: int = 0,
) -> None:
    result.stats.update(
        {
            "encyclopedia_registry_rows": rows,
            "encyclopedia_enum_keys": expected,
            "encyclopedia_enum_missing_en": missing_en,
            "encyclopedia_enum_missing_zh": missing_zh,
            "encyclopedia_unknown_menu_categories": unknown_categories,
        }
    )


def _audit_encyclopedia_registry(
    en: Catalog,
    zh: Catalog,
    result: AuditResult,
) -> None:
    registry_path = result.source_root / "docs" / "ELEMENT_REGISTRY.csv"
    _set_encyclopedia_registry_stats(result)
    if not registry_path.is_file():
        return

    try:
        raw = registry_path.read_bytes()
    except OSError as exc:
        result.add(
            SEVERITY_ERROR,
            "registration.encyclopedia_registry_read",
            f"无法读取元素登记表：{exc}",
        )
        return
    try:
        text = raw.decode("utf-8-sig")
    except UnicodeDecodeError as exc:
        result.add(
            SEVERITY_ERROR,
            "registration.encyclopedia_registry_utf8",
            f"元素登记表不是有效 UTF-8：{exc}",
        )
        return
    try:
        csv_rows = list(
            csv.reader(io.StringIO(text, newline=""), strict=True)
        )
    except csv.Error as exc:
        result.add(
            SEVERITY_ERROR,
            "registration.encyclopedia_registry_csv",
            f"元素登记表 CSV 解析失败：{exc}",
        )
        return
    if not csv_rows:
        result.add(
            SEVERITY_ERROR,
            "registration.encyclopedia_registry_empty",
            "元素登记表为空。",
        )
        return

    header = csv_rows[0]
    duplicate_columns = sorted(
        {column for column in header if header.count(column) > 1}
    )
    if duplicate_columns:
        result.add(
            SEVERITY_ERROR,
            "registration.encyclopedia_registry_columns",
            "元素登记表含重复列：" + ", ".join(duplicate_columns),
        )
        return
    missing_columns = [
        column
        for column in ENCYCLOPEDIA_REGISTRY_COLUMNS
        if column not in header
    ]
    if missing_columns:
        result.add(
            SEVERITY_ERROR,
            "registration.encyclopedia_registry_columns",
            "元素登记表缺少图鉴枚举列：" + ", ".join(missing_columns),
        )
        return

    expected_keys: set[str] = set()
    unknown_categories = 0
    data_rows = 0
    for line_number, values in enumerate(csv_rows[1:], start=2):
        if not values or all(not value.strip() for value in values):
            result.add(
                SEVERITY_ERROR,
                "registration.encyclopedia_registry_row",
                f"元素登记表第 {line_number} 行为空。",
            )
            continue
        data_rows += 1
        if len(values) != len(header):
            result.add(
                SEVERITY_ERROR,
                "registration.encyclopedia_registry_row",
                f"元素登记表第 {line_number} 行有 {len(values)} 列，"
                f"表头为 {len(header)} 列。",
            )
            continue
        row = dict(zip(header, values))
        identifier = row["identifier"].strip() or f"第 {line_number} 行"
        category = row["menu_category"].strip()
        category_key = MENU_CATEGORY_LOCALIZATION_KEYS.get(category)
        if category_key is None:
            unknown_categories += 1
            result.add(
                SEVERITY_ERROR,
                "registration.encyclopedia_category_unknown",
                f"无法映射 menu_category `{category or '<empty>'}`；"
                "必须显式登记对应的本地化键。",
                identifier,
            )
        else:
            expected_keys.add(category_key)

        for column, prefix in ENCYCLOPEDIA_ENUM_PREFIXES.items():
            value = row[column].strip()
            if not value:
                result.add(
                    SEVERITY_ERROR,
                    "registration.encyclopedia_enum_empty",
                    f"登记表字段 `{column}` 为空。",
                    identifier,
                )
                continue
            if not ENCYCLOPEDIA_ENUM_TOKEN_RE.fullmatch(value):
                result.add(
                    SEVERITY_ERROR,
                    "registration.encyclopedia_enum_invalid",
                    f"登记表字段 `{column}` 含不能组成稳定键的值 "
                    f"`{value}`。",
                    identifier,
                )
                continue
            expected_keys.add(prefix + value)

    missing_en = expected_keys - en.entries.keys()
    missing_zh = expected_keys - zh.entries.keys()
    _record_missing_group(
        result,
        "registration.encyclopedia_enum_en",
        "英文缺少图鉴枚举本地化键",
        missing_en,
    )
    _record_missing_group(
        result,
        "registration.encyclopedia_enum_zh",
        "中文缺少图鉴枚举本地化键",
        missing_zh,
    )
    _set_encyclopedia_registry_stats(
        result,
        rows=data_rows,
        expected=len(expected_keys),
        missing_en=len(missing_en),
        missing_zh=len(missing_zh),
        unknown_categories=unknown_categories,
    )


def audit(
    en_path: Path,
    zh_path: Path,
    source_root: Path,
) -> AuditResult:
    result = AuditResult(
        source_root=source_root.resolve(),
        en_path=en_path.resolve(),
        zh_path=zh_path.resolve(),
    )
    en = _load_catalog(en_path, "en-US", result)
    zh = _load_catalog(zh_path, "zh-CN", result)

    result.stats.update(
        {
            "en_keys": len(en.entries),
            "zh_keys": len(zh.entries),
            "duplicate_en": len(en.duplicate_keys),
            "duplicate_zh": len(zh.duplicate_keys),
        }
    )

    if en.valid:
        _audit_value_health("en-US", en.entries, result)
    if zh.valid:
        _audit_value_health("zh-CN", zh.entries, result)

    if en.valid and zh.valid:
        missing = sorted(en.entries.keys() - zh.entries.keys())
        extra = sorted(zh.entries.keys() - en.entries.keys())
        empty_en = sorted(key for key, value in en.entries.items() if not value.strip())
        empty_zh = sorted(key for key, value in zh.entries.items() if not value.strip())
        result.stats.update(
            {
                "missing_zh": len(missing),
                "extra_zh": len(extra),
                "empty_en": len(empty_en),
                "empty_zh": len(empty_zh),
            }
        )
        for key in missing:
            result.add(
                SEVERITY_ERROR,
                "keys.missing_zh",
                "zh-CN 缺少 en-US 基准键。",
                key,
            )
        for key in extra:
            result.add(
                SEVERITY_ERROR,
                "keys.extra_zh",
                "zh-CN 含 en-US 中不存在的废弃或拼写错误键。",
                key,
            )
        for key in empty_en:
            result.add(
                SEVERITY_ERROR,
                "value.empty_en",
                "en-US 值为空或仅含空白。",
                key,
            )
        for key in empty_zh:
            result.add(
                SEVERITY_ERROR,
                "value.empty_zh",
                "zh-CN 值为空或仅含空白。",
                key,
            )

        _audit_common_values(en, zh, result)
        _audit_untranslated_and_width(en, zh, result)
        _audit_source_registration(en, zh, result)
        _audit_encyclopedia_registry(en, zh, result)
    else:
        result.stats.update(
            {
                "missing_zh": 0,
                "extra_zh": 0,
                "empty_en": 0,
                "empty_zh": 0,
                "registered_elements": 0,
                "registered_element_descriptions": 0,
                "registered_element_names": 0,
                "element_description_missing_en": 0,
                "element_description_missing_zh": 0,
                "element_name_missing_en": 0,
                "element_name_missing_zh": 0,
                "registered_menus": 0,
                "menu_missing_en": 0,
                "menu_missing_zh": 0,
                "literal_tr_keys": 0,
                "literal_tr_missing_en": 0,
                "literal_tr_missing_zh": 0,
                "orphan_element_descriptions": 0,
                "orphan_menus": 0,
                "encyclopedia_registry_rows": 0,
                "encyclopedia_enum_keys": 0,
                "encyclopedia_enum_missing_en": 0,
                "encyclopedia_enum_missing_zh": 0,
                "encyclopedia_unknown_menu_categories": 0,
            }
        )

    result.stats.update(
        {
            "placeholder_mismatches": sum(
                item.code == "format.placeholder_mismatch"
                for item in result.findings
            ),
            "colour_mismatches": sum(
                item.code == "control.colour_mismatch"
                for item in result.findings
            ),
            "newline_mismatches": sum(
                item.code == "control.newline_mismatch"
                for item in result.findings
            ),
            "link_mismatches": sum(
                item.code in {"control.link_mismatch", "control.invalid_link"}
                for item in result.findings
            ),
            "end_control_mismatches": sum(
                item.code == "control.end_mismatch" for item in result.findings
            ),
            "invalid_colour_controls": sum(
                item.code == "control.invalid_colour"
                for item in result.findings
            ),
            "suspected_untranslated": sum(
                item.code
                in {"translation.identical", "translation.english_suspect"}
                for item in result.findings
            ),
            "text_corruption_findings": sum(
                item.code
                in {
                    "text.mojibake",
                    "text.mojibake_suspect",
                    "text.control_character",
                }
                for item in result.findings
            ),
            "width_risks": sum(
                item.code == "layout.width_risk" for item in result.findings
            ),
        }
    )
    result.stats["blocking_errors"] = len(result.errors)
    result.stats["warnings"] = len(result.warnings)
    return result


def _git_commit(source_root: Path) -> str:
    try:
        completed = subprocess.run(
            ["git", "-C", str(source_root), "rev-parse", "HEAD"],
            check=True,
            capture_output=True,
            text=True,
            timeout=5,
        )
    except (OSError, subprocess.SubprocessError):
        return "无法读取"
    return completed.stdout.strip() or "无法读取"


def _metric_rows(result: AuditResult) -> list[tuple[str, int]]:
    stats = result.stats
    return [
        ("英文键总数", stats["en_keys"]),
        ("中文键总数", stats["zh_keys"]),
        ("中文缺失键", stats["missing_zh"]),
        ("中文多余键", stats["extra_zh"]),
        ("英文/中文重复键", stats["duplicate_en"] + stats["duplicate_zh"]),
        ("英文/中文空值", stats["empty_en"] + stats["empty_zh"]),
        ("占位符错误", stats["placeholder_mismatches"]),
        ("颜色控制错误", stats["colour_mismatches"] + stats["invalid_colour_controls"]),
        ("换行结构错误", stats["newline_mismatches"]),
        ("链接控制错误", stats["link_mismatches"]),
        ("结束控制错误", stats["end_control_mismatches"]),
        ("疑似未翻译/保留英文项", stats["suspected_untranslated"]),
        ("乱码/非法字符项", stats["text_corruption_findings"]),
        ("宽度风险", stats["width_risks"]),
        ("源码登记元素", stats["registered_elements"]),
        ("缺少英文/中文元素短说明", stats["element_description_missing_en"] + stats["element_description_missing_zh"]),
        ("缺少英文/中文元素正式名称", stats["element_name_missing_en"] + stats["element_name_missing_zh"]),
        ("源码登记菜单", stats["registered_menus"]),
        ("缺少英文/中文菜单键", stats["menu_missing_en"] + stats["menu_missing_zh"]),
        ("源码字面量 Tr() 键", stats["literal_tr_keys"]),
        ("缺少英文/中文 Tr() 键", stats["literal_tr_missing_en"] + stats["literal_tr_missing_zh"]),
        ("图鉴登记表行", stats["encyclopedia_registry_rows"]),
        ("图鉴枚举所需键", stats["encyclopedia_enum_keys"]),
        ("缺少英文/中文图鉴枚举键", stats["encyclopedia_enum_missing_en"] + stats["encyclopedia_enum_missing_zh"]),
        ("未知图鉴菜单类别", stats["encyclopedia_unknown_menu_categories"]),
        ("发布阻塞错误", stats["blocking_errors"]),
        ("人工复核警告", stats["warnings"]),
    ]


def render_report(result: AuditResult, invocation: str) -> str:
    status = "不合格（存在发布阻塞错误）" if result.errors else "静态审计通过"
    generated = datetime.now(timezone.utc).replace(microsecond=0).isoformat()
    lines = [
        "# 简体中文本地化自动审计",
        "",
        "> 本报告由 `tools/i18n_audit.py` 生成。错误是发布阻塞项；"
        "警告必须人工复核，但不会单独令 `--check` 失败。",
        "",
        "## 审计对象",
        "",
        f"- 生成时间（UTC）：`{generated}`",
        f"- 当前提交：`{_git_commit(result.source_root)}`",
        f"- 英文文件：`{result.en_path}`",
        f"- 中文文件：`{result.zh_path}`",
        f"- 命令：`{invocation}`",
        "",
        "## 结论",
        "",
        f"**{status}**",
        "",
        "| 指标 | 数量 |",
        "|---|---:|",
    ]
    lines.extend(f"| {label} | {value} |" for label, value in _metric_rows(result))

    lines.extend(
        [
            "",
            "## 元素与菜单登记",
            "",
            f"- 源码元素：{result.stats['registered_elements']}；"
            f"短说明缺失（英/中）："
            f"{result.stats['element_description_missing_en']}/"
            f"{result.stats['element_description_missing_zh']}；"
            f"正式名称缺失（英/中）："
            f"{result.stats['element_name_missing_en']}/"
            f"{result.stats['element_name_missing_zh']}。",
            f"- 源码菜单：{result.stats['registered_menus']}；"
            f"菜单键缺失（英/中）：{result.stats['menu_missing_en']}/"
            f"{result.stats['menu_missing_zh']}。",
            f"- 源码字面量 `Tr()` 键：{result.stats['literal_tr_keys']}；"
            f"缺失（英/中）：{result.stats['literal_tr_missing_en']}/"
            f"{result.stats['literal_tr_missing_zh']}。",
            "",
            "元素四字符 `Name` 不视为正式英文/中文名称。当前审计约定正式名称键为 "
            "`sim.elem.<Identifier>.name`，现有 "
            "`sim.elem.<Identifier>` 继续作为短说明键。",
            "",
            "## 发布阻塞错误",
            "",
        ]
    )
    if result.errors:
        for finding in result.errors[:MAX_FINDINGS_IN_REPORT]:
            lines.append(f"- {finding.render()}")
        if len(result.errors) > MAX_FINDINGS_IN_REPORT:
            lines.append(
                f"- 其余 {len(result.errors) - MAX_FINDINGS_IN_REPORT} 项已省略。"
            )
    else:
        lines.append("- 无。")

    lines.extend(["", "## 人工复核警告", ""])
    if result.warnings:
        for finding in result.warnings[:MAX_FINDINGS_IN_REPORT]:
            lines.append(f"- {finding.render()}")
        if len(result.warnings) > MAX_FINDINGS_IN_REPORT:
            lines.append(
                f"- 其余 {len(result.warnings) - MAX_FINDINGS_IN_REPORT} 项已省略。"
            )
    else:
        lines.append("- 无。")

    lines.extend(
        [
            "",
            "## 审计规则说明",
            "",
            "- 严格 UTF-8/JSON、重复键、平面字符串对象、键集合、空值、"
            "占位符及 TPT 控制符属于可自动判定门禁。",
            "- 颜色控制支持 `\\b` 加 `wgorlbtuU`；链接必须使用 "
            "`{a:URL|文本}` 并紧跟 `\\x0E` 或实际 0x0E。",
            "- 换行比较换行总数、首尾状态及连续换行段；任何差异均需先修正或"
            "明确调整审计策略。",
            "- 疑似英文和宽度检查是启发式。宽度按 Unicode 东亚宽度估算，"
            "不能替代实际 TPT 字体渲染截图。",
            "- 当前语言加载器对字面量 `\\x0E` 的运行时转换能力仍需由 C++ "
            "测试单独验证；本脚本验证英中结构一致性。",
            "",
        ]
    )
    return "\n".join(lines)


def render_console(result: AuditResult) -> str:
    status = "FAIL" if result.errors else "PASS"
    lines = [
        f"i18n audit: {status}",
        f"keys: en-US={result.stats['en_keys']}, zh-CN={result.stats['zh_keys']}, "
        f"missing={result.stats['missing_zh']}, extra={result.stats['extra_zh']}",
        f"errors={result.stats['blocking_errors']}, "
        f"warnings={result.stats['warnings']}",
    ]
    for finding in result.errors:
        lines.append(f"ERROR {finding.render()}")
    for finding in result.warnings:
        lines.append(f"WARNING {finding.render()}")
    return "\n".join(lines)


def _default_source_root() -> Path:
    return Path(__file__).resolve().parents[1]


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="严格审计 TPT en-US/zh-CN 本地化及源码登记。"
    )
    parser.add_argument(
        "--source-root",
        type=Path,
        default=_default_source_root(),
        help="仓库根目录；默认由脚本位置推导。",
    )
    parser.add_argument(
        "--en",
        type=Path,
        help="en-US JSON；默认 SOURCE_ROOT/src/lang/en-US.json。",
    )
    parser.add_argument(
        "--zh",
        type=Path,
        help="zh-CN JSON；默认 SOURCE_ROOT/src/lang/zh-CN.json。",
    )
    parser.add_argument(
        "--check",
        action="store_true",
        help="存在发布阻塞错误时返回退出码 1；警告不影响退出码。",
    )
    parser.add_argument(
        "--write-report",
        type=Path,
        metavar="PATH",
        help="显式写入 Markdown 报告；未提供时绝不写文件。",
    )
    return parser


def main(argv: Sequence[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)
    source_root = args.source_root.resolve()
    en_path = (args.en or source_root / "src" / "lang" / "en-US.json").resolve()
    zh_path = (args.zh or source_root / "src" / "lang" / "zh-CN.json").resolve()
    result = audit(en_path, zh_path, source_root)
    print(render_console(result))

    if args.write_report is not None:
        report_path = args.write_report.resolve()
        invocation_parts = ["python", "tools/i18n_audit.py"]
        if args.check:
            invocation_parts.append("--check")
        invocation_parts.extend(["--write-report", str(args.write_report)])
        try:
            report_path.parent.mkdir(parents=True, exist_ok=True)
            report_path.write_text(
                render_report(result, " ".join(invocation_parts)),
                encoding="utf-8",
                newline="\n",
            )
        except OSError as exc:
            print(f"无法写入报告 `{report_path}`：{exc}", file=sys.stderr)
            return 2
        print(f"report written: {report_path}")

    if args.check and result.errors:
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
