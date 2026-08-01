from __future__ import annotations

import importlib.util
from pathlib import Path
import sys
import unittest


ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / "tools" / "organic_batch2_audit.py"
SPEC = importlib.util.spec_from_file_location("organic_batch2_audit", SCRIPT)
assert SPEC is not None and SPEC.loader is not None
organic_batch2_audit = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = organic_batch2_audit
SPEC.loader.exec_module(organic_batch2_audit)


class OrganicBatch2AuditTest(unittest.TestCase):
    def test_repository_passes(self) -> None:
        self.assertEqual([], organic_batch2_audit.audit(ROOT))

    def test_stable_range_is_contiguous(self) -> None:
        self.assertEqual(list(range(602, 622)), list(organic_batch2_audit.EXPECTED))

    def test_reaction_set_is_complete(self) -> None:
        self.assertEqual(13, len(organic_batch2_audit.REACTIONS))

    def test_duplicate_candidates_are_canonical_merges(self) -> None:
        self.assertEqual(6, len(organic_batch2_audit.CANONICAL_MERGES))


if __name__ == "__main__":
    unittest.main()
