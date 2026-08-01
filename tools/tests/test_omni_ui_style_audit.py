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
        self.assertIsNotNone(audit_module.DISPLAY_CODE.fullmatch("GAAS"))
        self.assertIsNone(audit_module.DISPLAY_CODE.fullmatch("H2O"))

    def test_editorial_wording_is_rejected(self) -> None:
        self.assertIsNotNone(audit_module.FORBIDDEN_STYLE.search("bounded proxy"))
        self.assertIsNotNone(audit_module.FORBIDDEN_STYLE.search("游戏化代理"))
        self.assertIsNotNone(audit_module.FORBIDDEN_STYLE.search("this batch"))
        self.assertIsNotNone(audit_module.FORBIDDEN_STYLE.search("兼容别名"))


if __name__ == "__main__":
    unittest.main()
