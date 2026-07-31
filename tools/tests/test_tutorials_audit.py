from __future__ import annotations

import copy
import importlib.util
from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / "tools" / "tutorials_audit.py"
SPEC = importlib.util.spec_from_file_location("tutorials_audit", SCRIPT)
assert SPEC is not None and SPEC.loader is not None
module = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(module)


class TutorialsAuditTests(unittest.TestCase):
    def setUp(self) -> None:
        self.spec = module.read_json(module.SPEC_PATH)
        self.tutorials = module.read_json(module.TUTORIAL_PATH)
        self.example_ids = {row["id"] for row in self.spec["examples"]}

    def test_repository_contract_passes_without_generated_files(self) -> None:
        self.assertEqual(module.audit_spec(self.spec), [])
        self.assertEqual(module.audit_tutorials(self.tutorials, self.example_ids), [])

    def test_example_count_is_fail_closed(self) -> None:
        broken = copy.deepcopy(self.spec)
        broken["examples"].pop()
        self.assertTrue(any("exactly 7" in error for error in module.audit_spec(broken)))

    def test_tutorial_count_is_fail_closed(self) -> None:
        broken = copy.deepcopy(self.tutorials)
        broken["tutorials"].pop()
        self.assertTrue(any("exactly 8" in error for error in module.audit_tutorials(broken, self.example_ids)))

    def test_missing_bilingual_field_is_rejected(self) -> None:
        broken = copy.deepcopy(self.tutorials)
        broken["tutorials"][0]["title_en"] = ""
        errors = module.audit_tutorials(broken, self.example_ids)
        self.assertTrue(any("title_en" in error for error in errors))

    def test_unknown_example_reference_is_rejected(self) -> None:
        broken = copy.deepcopy(self.tutorials)
        broken["tutorials"][0]["example_id"] = "missing"
        errors = module.audit_tutorials(broken, self.example_ids)
        self.assertTrue(any("missing example" in error for error in errors))


if __name__ == "__main__":
    unittest.main()
