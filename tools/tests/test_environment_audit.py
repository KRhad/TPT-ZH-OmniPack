from __future__ import annotations

import importlib.util
from pathlib import Path
import sys
import unittest


ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / "tools" / "environment_audit.py"
SPEC = importlib.util.spec_from_file_location("environment_audit", SCRIPT)
assert SPEC is not None and SPEC.loader is not None
environment_audit = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = environment_audit
SPEC.loader.exec_module(environment_audit)


class EnvironmentAuditTests(unittest.TestCase):
    def test_repository_passes(self) -> None:
        self.assertEqual([], environment_audit.audit(ROOT))

    def test_stable_range_is_contiguous(self) -> None:
        self.assertEqual(list(range(670, 686)), list(environment_audit.EXPECTED))

    def test_reaction_set_is_complete(self) -> None:
        self.assertEqual(31, len(environment_audit.REACTIONS))

    def test_codes_are_four_uppercase_characters(self) -> None:
        for code, *_ in environment_audit.EXPECTED.values():
            self.assertEqual(4, len(code))
            self.assertEqual(code.upper(), code)


if __name__ == "__main__":
    unittest.main()
