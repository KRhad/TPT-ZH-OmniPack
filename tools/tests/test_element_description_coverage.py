from __future__ import annotations

import copy
import importlib.util
from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / "tools" / "element_description_coverage_audit.py"
SPEC = importlib.util.spec_from_file_location("element_description_coverage_audit", SCRIPT)
assert SPEC is not None and SPEC.loader is not None
module = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(module)


class ElementDescriptionCoverageTests(unittest.TestCase):
    def test_repository_has_complete_description_routes(self) -> None:
        errors, stats = module.audit(ROOT)
        self.assertEqual(errors, [])
        self.assertEqual(stats["canonical_materials"], 487)
        self.assertEqual(stats["official_wiki_full"], 195)
        self.assertEqual(stats["official_runtime_detail"], 0)
        self.assertEqual(stats["omnipack_structured_detail"], 292)

    def test_missing_structured_content_is_rejected(self) -> None:
        original = module.load_csv

        def altered(path: Path, errors: list[str]):
            rows = copy.deepcopy(original(path, errors))
            if path.name == "ELEMENT_CONTENT.csv":
                target = next(row for row in rows if row["identifier"] == "OMNI_PT_ETHL")
                target["use_zh"] = ""
            return rows

        module.load_csv = altered
        try:
            errors, _ = module.audit(ROOT)
        finally:
            module.load_csv = original
        self.assertTrue(any("OMNI_PT_ETHL" in error and "use_zh" in error for error in errors))

    def test_known_legacy_description_defects_are_repaired(self) -> None:
        errors: list[str] = []
        rows = {
            row["identifier"]: row
            for row in module.load_csv(
                ROOT / "docs" / "OFFICIAL_ELEMENT_DESCRIPTIONS.csv", errors
            )
        }
        self.assertEqual(errors, [])
        self.assertEqual(len(rows), 195)
        expected_chinese = {
            "DEFAULT_PT_DYST": ("死酵母", "不是菌丝"),
            "DEFAULT_PT_PSTS": ("固态浆糊", "PSTE"),
            "DEFAULT_PT_GOO": ("黏胶", "不是黏土粉"),
            "DEFAULT_PT_THDR": ("球状闪电", "压力脉冲"),
            "DEFAULT_PT_TESC": ("特斯拉线圈", "LIGH"),
            "DEFAULT_PT_BOYL": ("波义耳", "不可燃气体"),
            "DEFAULT_PT_INVIS": ("压力阈值", "VOID"),
            "DEFAULT_PT_DMG": ("冲击粒子", "不是引力炸弹"),
            "DEFAULT_PT_DRAY": ("复制射线", "目标侧"),
            "DEFAULT_PT_O2": ("氧气", "助燃"),
        }
        for identifier, markers in expected_chinese.items():
            description = rows[identifier]["chinese_description"]
            for marker in markers:
                self.assertIn(marker, description, identifier)
        for identifier, row in rows.items():
            self.assertGreaterEqual(len(row["english_description"]), 80, identifier)
            self.assertGreaterEqual(len(row["chinese_description"]), 80, identifier)
            self.assertTrue(
                row["wiki_url"].startswith("https://powdertoy.co.uk/Wiki/W/"),
                identifier,
            )
            self.assertRegex(row["wiki_snapshot"], r"^[0-9]{14}$", identifier)


if __name__ == "__main__":
    unittest.main()
