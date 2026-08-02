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


if __name__ == "__main__":
    unittest.main()
