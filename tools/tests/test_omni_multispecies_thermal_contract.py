from __future__ import annotations

import importlib.util
from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[2]
TOOL_PATH = ROOT / "tools" / "omni_multispecies_thermal_contract_check.py"
SPEC = importlib.util.spec_from_file_location("omni_multispecies_thermal_contract_check", TOOL_PATH)
assert SPEC is not None and SPEC.loader is not None
tool = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(tool)


class OmniMultispeciesThermalContractTests(unittest.TestCase):
    def test_repository_contract(self) -> None:
        self.assertEqual(tool.check(ROOT), [])


if __name__ == "__main__":
    unittest.main()
