from __future__ import annotations

import contextlib
import csv
import importlib.util
import io
import json
from pathlib import Path
import sys
import tempfile
import unittest


SCRIPT = Path(__file__).resolve().parents[1] / "i18n_audit.py"
SPEC = importlib.util.spec_from_file_location("i18n_audit", SCRIPT)
assert SPEC is not None and SPEC.loader is not None
i18n_audit = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = i18n_audit
SPEC.loader.exec_module(i18n_audit)


class I18nAuditTests(unittest.TestCase):
    def test_supplementary_cjk_text_is_recognized(self) -> None:
        self.assertIsNotNone(i18n_audit.CJK_RE.search("𫓧"))

    def make_repo(
        self,
        root: Path,
        en: dict[str, str],
        zh: dict[str, str],
        *,
        raw_en: str | None = None,
    ) -> None:
        lang = root / "src" / "lang"
        elements = root / "src" / "simulation" / "elements"
        lang.mkdir(parents=True)
        elements.mkdir(parents=True)
        (lang / "en-US.json").write_text(
            raw_en if raw_en is not None else json.dumps(en, ensure_ascii=False),
            encoding="utf-8",
        )
        (lang / "zh-CN.json").write_text(
            json.dumps(zh, ensure_ascii=False),
            encoding="utf-8",
        )
        (elements / "WATR.cpp").write_text(
            """
void Element::Element_WATR()
{
    Identifier = "DEFAULT_PT_WATR";
    Name = "WATR";
    Description = Localization::Ref().Tr("sim.elem.DEFAULT_PT_WATR");
}
""".strip(),
            encoding="utf-8",
        )
        (root / "src" / "simulation" / "SimulationData.cpp").write_text(
            '{0, String("sim.menu.liquids"), 0, 1},',
            encoding="utf-8",
        )

    @staticmethod
    def clean_catalogs() -> tuple[dict[str, str], dict[str, str]]:
        en = {
            "message": "Value %s\\bg{a:https://example.test|Open}\\x0E\n",
            "sim.elem.DEFAULT_PT_WATR": "Water.",
            "sim.elem.DEFAULT_PT_WATR.name": "Water",
            "sim.menu.liquids": "Liquids",
        }
        zh = {
            "message": "数值 %s\\bg{a:https://example.test|打开}\\x0E\n",
            "sim.elem.DEFAULT_PT_WATR": "水。",
            "sim.elem.DEFAULT_PT_WATR.name": "水",
            "sim.menu.liquids": "液体",
        }
        return en, zh

    def run_main(self, argv: list[str]) -> int:
        with contextlib.redirect_stdout(io.StringIO()):
            with contextlib.redirect_stderr(io.StringIO()):
                return i18n_audit.main(argv)

    def write_encyclopedia_registry(self, root: Path) -> None:
        docs = root / "docs"
        docs.mkdir(parents=True, exist_ok=True)
        columns = (
            "identifier",
            "menu_category",
            "element_state",
            "save_compatibility",
            "implementation_status",
            "test_status",
        )
        rows = (
            {
                "identifier": "DEFAULT_PT_WATR",
                "menu_category": "SC_LIQUID",
                "element_state": "liquid",
                "save_compatibility": "official-locked",
                "implementation_status": "implemented",
                "test_status": "source-verified",
            },
            {
                "identifier": "RESERVED_PT_146",
                "menu_category": "RESERVED",
                "element_state": "reserved",
                "save_compatibility": "reserved-slot",
                "implementation_status": "reserved",
                "test_status": "lock-verified",
            },
        )
        with (docs / "ELEMENT_REGISTRY.csv").open(
            "w", encoding="utf-8", newline=""
        ) as registry:
            writer = csv.DictWriter(registry, fieldnames=columns)
            writer.writeheader()
            writer.writerows(rows)

    @staticmethod
    def add_encyclopedia_enum_keys(
        en: dict[str, str], zh: dict[str, str]
    ) -> None:
        values = {
            "encyclopedia.value.category.RESERVED": (
                "Reserved slot",
                "保留槽位",
            ),
            "encyclopedia.value.state.liquid": ("Liquid", "液体"),
            "encyclopedia.value.state.reserved": ("Reserved", "保留"),
            "encyclopedia.value.save.official-locked": (
                "Official ID locked",
                "官方 ID 已锁定",
            ),
            "encyclopedia.value.save.reserved-slot": (
                "Reserved save slot",
                "保留存档槽位",
            ),
            "encyclopedia.value.implementation.implemented": (
                "Implemented",
                "已实现",
            ),
            "encyclopedia.value.implementation.reserved": (
                "Reserved",
                "保留",
            ),
            "encyclopedia.value.test.source-verified": (
                "Source verified",
                "来源已验证",
            ),
            "encyclopedia.value.test.lock-verified": (
                "ID lock verified",
                "ID 锁定已验证",
            ),
        }
        for key, (en_value, zh_value) in values.items():
            en[key] = en_value
            zh[key] = zh_value

    def test_clean_check_passes_and_does_not_write_implicitly(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            en, zh = self.clean_catalogs()
            self.make_repo(root, en, zh)
            exit_code = self.run_main(["--source-root", str(root), "--check"])
            self.assertEqual(exit_code, 0)
            self.assertFalse((root / "docs" / "I18N_AUDIT.md").exists())

    def test_report_is_only_written_when_explicit(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            en, zh = self.clean_catalogs()
            self.make_repo(root, en, zh)
            report = root / "output" / "audit.md"
            exit_code = self.run_main(
                [
                    "--source-root",
                    str(root),
                    "--check",
                    "--write-report",
                    str(report),
                ]
            )
            self.assertEqual(exit_code, 0)
            self.assertTrue(report.is_file())
            self.assertIn("静态审计通过", report.read_text(encoding="utf-8"))

    def test_duplicate_key_is_a_blocking_error(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            en, zh = self.clean_catalogs()
            raw_en = (
                '{"message":"first","message":"second",'
                '"sim.elem.DEFAULT_PT_WATR":"Water.",'
                '"sim.elem.DEFAULT_PT_WATR.name":"Water",'
                '"sim.menu.liquids":"Liquids"}'
            )
            self.make_repo(root, en, zh, raw_en=raw_en)
            result = i18n_audit.audit(
                root / "src/lang/en-US.json",
                root / "src/lang/zh-CN.json",
                root,
            )
            self.assertIn("json.duplicate_key", {item.code for item in result.errors})
            self.assertEqual(
                self.run_main(["--source-root", str(root), "--check"]), 1
            )

    def test_flat_string_object_is_required(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            en, zh = self.clean_catalogs()
            en["nested"] = {"not": "flat"}  # type: ignore[assignment]
            self.make_repo(root, en, zh)
            result = i18n_audit.audit(
                root / "src/lang/en-US.json",
                root / "src/lang/zh-CN.json",
                root,
            )
            self.assertIn(
                "json.flat_string_object", {item.code for item in result.errors}
            )

    def test_structural_mismatches_are_blocking(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            en, zh = self.clean_catalogs()
            en["only.en"] = "English only"
            zh["only.zh"] = "仅中文"
            zh["message"] = r"数值 %d\br{a:https://wrong.test|打开}"
            zh["empty"] = ""
            en["empty"] = "Required"
            self.make_repo(root, en, zh)
            result = i18n_audit.audit(
                root / "src/lang/en-US.json",
                root / "src/lang/zh-CN.json",
                root,
            )
            codes = {item.code for item in result.errors}
            self.assertTrue(
                {
                    "keys.missing_zh",
                    "keys.extra_zh",
                    "value.empty_zh",
                    "format.placeholder_mismatch",
                    "control.colour_mismatch",
                    "control.newline_mismatch",
                    "control.link_mismatch",
                    "control.end_mismatch",
                }.issubset(codes)
            )

    def test_mojibake_and_missing_element_names_are_blocking(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            en, zh = self.clean_catalogs()
            del en["sim.elem.DEFAULT_PT_WATR.name"]
            del zh["sim.elem.DEFAULT_PT_WATR.name"]
            zh["message"] = "锟斤拷"
            self.make_repo(root, en, zh)
            result = i18n_audit.audit(
                root / "src/lang/en-US.json",
                root / "src/lang/zh-CN.json",
                root,
            )
            codes = {item.code for item in result.errors}
            self.assertIn("text.mojibake", codes)
            self.assertIn("registration.element_name_en", codes)
            self.assertIn("registration.element_name_zh", codes)

    def test_missing_encyclopedia_enum_keys_are_blocking(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            en, zh = self.clean_catalogs()
            self.make_repo(root, en, zh)
            self.write_encyclopedia_registry(root)
            result = i18n_audit.audit(
                root / "src/lang/en-US.json",
                root / "src/lang/zh-CN.json",
                root,
            )
            codes = {item.code for item in result.errors}
            self.assertIn("registration.encyclopedia_enum_en", codes)
            self.assertIn("registration.encyclopedia_enum_zh", codes)
            rendered = "\n".join(item.render() for item in result.errors)
            self.assertIn("encyclopedia.value.category.RESERVED", rendered)
            self.assertEqual(result.stats["encyclopedia_registry_rows"], 2)
            self.assertEqual(result.stats["encyclopedia_enum_keys"], 10)
            self.assertEqual(result.stats["encyclopedia_enum_missing_en"], 9)
            self.assertEqual(result.stats["encyclopedia_enum_missing_zh"], 9)
            self.assertEqual(
                self.run_main(["--source-root", str(root), "--check"]), 1
            )

    def test_complete_encyclopedia_enum_keys_pass(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            en, zh = self.clean_catalogs()
            self.add_encyclopedia_enum_keys(en, zh)
            self.make_repo(root, en, zh)
            self.write_encyclopedia_registry(root)
            result = i18n_audit.audit(
                root / "src/lang/en-US.json",
                root / "src/lang/zh-CN.json",
                root,
            )
            self.assertFalse(
                any(
                    item.code.startswith("registration.encyclopedia")
                    for item in result.errors
                )
            )
            self.assertEqual(result.stats["encyclopedia_enum_keys"], 10)
            self.assertEqual(result.stats["encyclopedia_enum_missing_en"], 0)
            self.assertEqual(result.stats["encyclopedia_enum_missing_zh"], 0)
            self.assertEqual(
                self.run_main(["--source-root", str(root), "--check"]), 0
            )


if __name__ == "__main__":
    unittest.main()
