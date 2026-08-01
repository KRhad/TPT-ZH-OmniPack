from __future__ import annotations

import importlib.util
from pathlib import Path
import sys
import unittest


ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / "tools" / "electronics_audit.py"
SPEC = importlib.util.spec_from_file_location("electronics_audit", SCRIPT)
assert SPEC is not None and SPEC.loader is not None
electronics_audit = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = electronics_audit
SPEC.loader.exec_module(electronics_audit)


class ElectronicsAuditTests(unittest.TestCase):
    def test_repository_passes(self) -> None:
        self.assertEqual([], electronics_audit.audit(ROOT))

    def test_stable_range_is_contiguous(self) -> None:
        self.assertEqual(list(range(622, 642)), list(electronics_audit.EXPECTED))

    def test_reaction_set_is_complete(self) -> None:
        self.assertEqual(38, len(electronics_audit.REACTIONS))


if __name__ == "__main__":
    unittest.main()
