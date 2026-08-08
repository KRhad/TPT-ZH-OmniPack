from __future__ import annotations

import importlib.util
from pathlib import Path
import sys
import unittest


ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / "tools" / "omni_ui_style_audit.py"
SPEC = importlib.util.spec_from_file_location("omni_ui_style_audit", SCRIPT)
assert SPEC is not None and SPEC.loader is not None
audit_module = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = audit_module
SPEC.loader.exec_module(audit_module)


class OmniUiStyleAuditTests(unittest.TestCase):
    def test_repository_passes(self) -> None:
        self.assertEqual([], audit_module.audit(ROOT))

    def test_display_code_contract_is_exact(self) -> None:
        self.assertIsNotNone(audit_module.DISPLAY_CODE.fullmatch("GAS"))
        self.assertIsNotNone(audit_module.DISPLAY_CODE.fullmatch("GAAS"))
        self.assertIsNone(audit_module.DISPLAY_CODE.fullmatch("H2O"))
        self.assertIsNone(audit_module.DISPLAY_CODE.fullmatch("AB"))
        self.assertIsNone(audit_module.DISPLAY_CODE.fullmatch("ABCDE"))

    def test_editorial_wording_is_rejected(self) -> None:
        self.assertIsNotNone(audit_module.FORBIDDEN_STYLE.search("bounded proxy"))
        self.assertIsNotNone(audit_module.FORBIDDEN_STYLE.search("游戏化代理"))
        self.assertIsNotNone(audit_module.FORBIDDEN_STYLE.search("this batch"))
        self.assertIsNotNone(audit_module.FORBIDDEN_STYLE.search("兼容别名"))
        self.assertIsNotNone(audit_module.FORBIDDEN_STYLE.search("no unlocks"))
        self.assertIsNotNone(audit_module.FORBIDDEN_STYLE.search("direct selection"))
        self.assertIsNotNone(audit_module.FORBIDDEN_STYLE.search("placed directly"))
        self.assertIsNotNone(audit_module.FORBIDDEN_STYLE.search("direct placement"))
        self.assertIsNotNone(audit_module.FORBIDDEN_STYLE.search("direct sandbox placement"))
        self.assertIsNotNone(audit_module.FORBIDDEN_STYLE.search("无需解锁"))
        self.assertIsNotNone(audit_module.FORBIDDEN_STYLE.search("直接选择"))
        self.assertIsNotNone(audit_module.FORBIDDEN_STYLE.search("可直接放置"))

    def test_periodic_grid_long_press_is_rejected(self) -> None:
        target = ROOT / "src" / "gui" / "periodictable" / "PeriodicTableActivity.cpp"
        original = audit_module.read_text
        try:
            audit_module.read_text = lambda path, errors: (
                original(path, errors) + "\nSetLongPressCallback\n"
                if path == target
                else original(path, errors)
            )
            errors = audit_module.audit(ROOT)
        finally:
            audit_module.read_text = original
        self.assertTrue(any("outer periodic grid" in error for error in errors))

    def test_missing_long_press_hint_is_rejected(self) -> None:
        target = ROOT / "src" / "gui" / "elementsearch" / "ElementInfo.cpp"
        original = audit_module.read_text
        try:
            audit_module.read_text = lambda path, errors: (
                original(path, errors).replace(
                    'Tr("element.long_press_hint")', 'Tr("removed.long_press_hint")'
                )
                if path == target
                else original(path, errors)
            )
            errors = audit_module.audit(ROOT)
        finally:
            audit_module.read_text = original
        self.assertTrue(any("description hint marker" in error for error in errors))

    def test_periodic_picker_duplicate_description_is_rejected(self) -> None:
        target = (
            ROOT
            / "src"
            / "gui"
            / "periodictable"
            / "PeriodicElementDetailActivity.cpp"
        )
        original = audit_module.read_text
        try:
            audit_module.read_text = lambda path, errors: (
                original(path, errors) + '\nTr("encyclopedia.description");\n'
                if path == target
                else original(path, errors)
            )
            errors = audit_module.audit(ROOT)
        finally:
            audit_module.read_text = original
        self.assertTrue(any("periodic picker repeats" in error for error in errors))

    def test_periodic_picker_missing_material_count_is_rejected(self) -> None:
        target = (
            ROOT
            / "src"
            / "gui"
            / "periodictable"
            / "PeriodicElementDetailActivity.cpp"
        )
        original = audit_module.read_text
        try:
            audit_module.read_text = lambda path, errors: (
                original(path, errors).replace(
                    'Tr("periodic.detail.material_count")',
                    'Tr("removed.material_count")',
                )
                if path == target
                else original(path, errors)
            )
            errors = audit_module.audit(ROOT)
        finally:
            audit_module.read_text = original
        self.assertTrue(any("related-material count" in error for error in errors))

    def test_unbounded_tooltip_alignment_is_rejected(self) -> None:
        target = ROOT / "src" / "gui" / "game" / "GameView.cpp"
        original = audit_module.read_text
        try:
            audit_module.read_text = lambda path, errors: (
                original(path, errors)
                + "\nSize.X - 27 - (Graphics::TextSize(toolTip).X - 1);\n"
                if path == target
                else original(path, errors)
            )
            errors = audit_module.audit(ROOT)
        finally:
            audit_module.read_text = original
        self.assertTrue(any("unbounded right alignment" in error for error in errors))

    def test_missing_transient_description_timeout_is_rejected(self) -> None:
        target = ROOT / "src" / "gui" / "game" / "GameView.cpp"
        original = audit_module.read_text
        try:
            audit_module.read_text = lambda path, errors: (
                original(path, errors).replace(
                    "constexpr unsigned long ElementDescriptionMaximumMs = 2500;",
                    "constexpr unsigned long RemovedDescriptionMaximumMs = 2500;",
                )
                if path == target
                else original(path, errors)
            )
            errors = audit_module.audit(ROOT)
        finally:
            audit_module.read_text = original
        self.assertTrue(any("transient description marker" in error for error in errors))

    def test_missing_stale_android_hover_guard_is_rejected(self) -> None:
        target = ROOT / "src" / "gui" / "game" / "GameView.cpp"
        original = audit_module.read_text
        try:
            audit_module.read_text = lambda path, errors: (
                original(path, errors).replace(
                    "if (suppressToolTipsUntilMouseMove)\n\t\treturn;",
                    "if (suppressToolTipsUntilMouseMove)\n\t\tisToolTipFadingIn = true;",
                    1,
                )
                if path == target
                else original(path, errors)
            )
            errors = audit_module.audit(ROOT)
        finally:
            audit_module.read_text = original
        self.assertTrue(any("stale Android hover" in error for error in errors))


if __name__ == "__main__":
    unittest.main()
