import subprocess
import sys
import unittest
from pathlib import Path


class OmniSolutionContractTest(unittest.TestCase):
    def test_contract(self):
        root = Path(__file__).resolve().parents[2]
        result = subprocess.run(
            [sys.executable, str(root / "tools/omni_solution_contract_check.py")],
            cwd=root,
            text=True,
            capture_output=True,
            check=False,
        )
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn("omni_solution_contract_pass=true", result.stdout)


if __name__ == "__main__":
    unittest.main()
