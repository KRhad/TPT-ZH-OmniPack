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


SCRIPT = Path(__file__).resolve().parents[1] / "sync_element_localization.py"
SPEC = importlib.util.spec_from_file_location("sync_element_localization", SCRIPT)
assert SPEC is not None and SPEC.loader is not None
sync_element_localization = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = sync_element_localization
SPEC.loader.exec_module(sync_element_localization)


REGISTRY_COLUMNS = (
    "identifier",
    "english_name",
    "chinese_name",
    "stable_id",
    "element_state",
    "implementation_status",
    "source_file",
)


class SyncElementLocalizationTests(unittest.TestCase):
    def make_repo(
        self,
        root: Path,
        *,
        en_name: str | None = None,
        zh_name: str | None = None,
        include_description: bool = True,
        extra_rows: list[dict[str, str]] | None = None,
        english_name: str = "Water",
        chinese_name: str = "水",
    ) -> tuple[Path, Path]:
        lang_dir = root / "src" / "lang"
        element_dir = root / "src" / "simulation" / "elements"
        docs_dir = root / "docs"
        lang_dir.mkdir(parents=True)
        element_dir.mkdir(parents=True)
        docs_dir.mkdir(parents=True)

        (element_dir / "WATR.cpp").write_text(
            """
void Element::Element_WATR()
{
    Identifier = "DEFAULT_PT_WATR";
    Name = "WATR";
}
""".strip(),
            encoding="utf-8",
        )

        en: dict[str, str] = {"before": "Before"}
        zh: dict[str, str] = {"before": "之前"}
        if include_description:
            en["sim.elem.DEFAULT_PT_WATR"] = "Water description."
            zh["sim.elem.DEFAULT_PT_WATR"] = "水的说明。"
        en["after"] = "After"
        zh["after"] = "之后"
        if en_name is not None:
            en["sim.elem.DEFAULT_PT_WATR.name"] = en_name
        if zh_name is not None:
            zh["sim.elem.DEFAULT_PT_WATR.name"] = zh_name

        en_path = lang_dir / "en-US.json"
        zh_path = lang_dir / "zh-CN.json"
        en_path.write_text(
            json.dumps(en, ensure_ascii=False, indent=4) + "\n",
            encoding="utf-8",
        )
        zh_path.write_text(
            json.dumps(zh, ensure_ascii=False, indent=4) + "\n",
            encoding="utf-8",
        )

        rows = [
            {
                "identifier": "DEFAULT_PT_WATR",
                "english_name": english_name,
                "chinese_name": chinese_name,
                "stable_id": "2",
                "element_state": "liquid",
                "implementation_status": "implemented",
                "source_file": "src/simulation/elements/WATR.cpp",
            },
            {
                "identifier": "RESERVED_PT_146",
                "english_name": "Reserved Slot 146",
                "chinese_name": "保留槽位 146",
                "stable_id": "146",
                "element_state": "reserved",
                "implementation_status": "reserved",
                "source_file": "",
            },
        ]
        if extra_rows:
            rows.extend(extra_rows)
        with (docs_dir / "ELEMENT_REGISTRY.csv").open(
            "w", encoding="utf-8", newline=""
        ) as registry:
            writer = csv.DictWriter(registry, fieldnames=REGISTRY_COLUMNS)
            writer.writeheader()
            writer.writerows(rows)
        return en_path, zh_path

    def run_main(self, root: Path, *, write: bool = False) -> int:
        argv = ["--source-root", str(root)]
        if write:
            argv.append("--write")
        with contextlib.redirect_stdout(io.StringIO()):
            with contextlib.redirect_stderr(io.StringIO()):
                return sync_element_localization.main(argv)

    def test_default_check_is_read_only_and_reports_required_sync(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            en_path, zh_path = self.make_repo(root)
            before_en = en_path.read_bytes()
            before_zh = zh_path.read_bytes()
            self.assertEqual(self.run_main(root), 1)
            self.assertEqual(en_path.read_bytes(), before_en)
            self.assertEqual(zh_path.read_bytes(), before_zh)

    def test_write_inserts_names_after_descriptions_with_stable_json(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            en_path, zh_path = self.make_repo(root)
            self.assertEqual(self.run_main(root, write=True), 0)

            en_text = en_path.read_text(encoding="utf-8")
            zh_text = zh_path.read_text(encoding="utf-8")
            self.assertTrue(en_text.endswith("\n"))
            self.assertIn('\n  "before":', en_text)
            self.assertNotIn('\n    "before":', en_text)
            self.assertIn('"水"', zh_text)
            self.assertNotIn("\\u6c34", zh_text.lower())

            en = json.loads(en_text, object_pairs_hook=dict)
            zh = json.loads(zh_text, object_pairs_hook=dict)
            en_keys = list(en)
            zh_keys = list(zh)
            description = "sim.elem.DEFAULT_PT_WATR"
            name = f"{description}.name"
            self.assertEqual(en_keys.index(name), en_keys.index(description) + 1)
            self.assertEqual(zh_keys.index(name), zh_keys.index(description) + 1)
            self.assertEqual(en[name], "Water")
            self.assertEqual(zh[name], "水")
            self.assertNotIn("sim.elem.RESERVED_PT_146.name", en)
            self.assertNotIn("sim.elem.RESERVED_PT_146.name", zh)
            self.assertEqual(self.run_main(root), 0)

    def test_write_updates_and_moves_existing_names(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            en_path, zh_path = self.make_repo(
                root, en_name="Wrong", zh_name="错误"
            )
            self.assertEqual(self.run_main(root, write=True), 0)
            en = json.loads(en_path.read_text(encoding="utf-8"))
            zh = json.loads(zh_path.read_text(encoding="utf-8"))
            self.assertEqual(en["sim.elem.DEFAULT_PT_WATR.name"], "Water")
            self.assertEqual(zh["sim.elem.DEFAULT_PT_WATR.name"], "水")
            en_keys = list(en)
            self.assertEqual(
                en_keys.index("sim.elem.DEFAULT_PT_WATR.name"),
                en_keys.index("sim.elem.DEFAULT_PT_WATR") + 1,
            )

    def test_duplicate_identifier_blocks_write(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            duplicate = {
                "identifier": "DEFAULT_PT_WATR",
                "english_name": "Duplicate",
                "chinese_name": "重复",
                "stable_id": "3",
                "element_state": "liquid",
                "implementation_status": "implemented",
                "source_file": "src/simulation/elements/WATR.cpp",
            }
            en_path, zh_path = self.make_repo(root, extra_rows=[duplicate])
            before_en = en_path.read_bytes()
            before_zh = zh_path.read_bytes()
            result = sync_element_localization.synchronize(
                root,
                root / "docs/ELEMENT_REGISTRY.csv",
                en_path,
                zh_path,
                write=True,
            )
            self.assertTrue(
                any("identifier 重复" in error for error in result.errors)
            )
            self.assertEqual(en_path.read_bytes(), before_en)
            self.assertEqual(zh_path.read_bytes(), before_zh)

    def test_duplicate_stable_id_blocks_write(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            duplicate = {
                "identifier": "DEFAULT_PT_COPY",
                "english_name": "Duplicate",
                "chinese_name": "重复",
                "stable_id": "2",
                "element_state": "liquid",
                "implementation_status": "implemented",
                "source_file": "src/simulation/elements/WATR.cpp",
            }
            en_path, zh_path = self.make_repo(root, extra_rows=[duplicate])
            before_en = en_path.read_bytes()
            before_zh = zh_path.read_bytes()
            result = sync_element_localization.synchronize(
                root,
                root / "docs/ELEMENT_REGISTRY.csv",
                en_path,
                zh_path,
                write=True,
            )
            self.assertTrue(
                any("stable_id 重复" in error for error in result.errors)
            )
            self.assertEqual(en_path.read_bytes(), before_en)
            self.assertEqual(zh_path.read_bytes(), before_zh)

    def test_empty_name_and_missing_description_block_write(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            en_path, zh_path = self.make_repo(
                root,
                include_description=False,
                english_name="",
                chinese_name="",
            )
            before_en = en_path.read_bytes()
            before_zh = zh_path.read_bytes()
            result = sync_element_localization.synchronize(
                root,
                root / "docs/ELEMENT_REGISTRY.csv",
                en_path,
                zh_path,
                write=True,
            )
            self.assertTrue(
                any("英文名称为空" in error for error in result.errors)
            )
            self.assertTrue(
                any("中文名称为空" in error for error in result.errors)
            )
            self.assertEqual(en_path.read_bytes(), before_en)
            self.assertEqual(zh_path.read_bytes(), before_zh)

    def test_source_element_missing_from_registry_is_an_error(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            self.make_repo(root)
            (root / "src/simulation/elements/FIRE.cpp").write_text(
                """
void Element::Element_FIRE()
{
    Identifier = "DEFAULT_PT_FIRE";
    Name = "FIRE";
}
""".strip(),
                encoding="utf-8",
            )
            result = sync_element_localization.synchronize(
                root,
                root / "docs/ELEMENT_REGISTRY.csv",
                root / "src/lang/en-US.json",
                root / "src/lang/zh-CN.json",
                write=False,
            )
            self.assertTrue(
                any("DEFAULT_PT_FIRE" in error for error in result.errors)
            )


if __name__ == "__main__":
    unittest.main()
