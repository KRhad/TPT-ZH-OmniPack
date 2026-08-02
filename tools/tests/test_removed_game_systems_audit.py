from __future__ import annotations

import importlib.util
from pathlib import Path
import sys
import unittest


ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / "tools" / "removed_game_systems_audit.py"
SPEC = importlib.util.spec_from_file_location("removed_game_systems_audit", SCRIPT)
assert SPEC is not None and SPEC.loader is not None
audit_module = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = audit_module
SPEC.loader.exec_module(audit_module)


class RemovedGameSystemsAuditTests(unittest.TestCase):
    def test_repository_passes(self) -> None:
        self.assertEqual(audit_module.audit(ROOT), [])

    def test_reintroduced_progress_marker_is_rejected(self) -> None:
        target = ROOT / "src" / "client" / "GameSave.cpp"
        original = audit_module.read_text
        try:
            audit_module.read_text = lambda path, errors: (
                original(path, errors) + "\nOmniAlchemy\n" if path == target else original(path, errors)
            )
            errors = audit_module.audit(ROOT)
        finally:
            audit_module.read_text = original
        self.assertTrue(any("retired marker" in error for error in errors))

    def test_missing_element_description_label_is_rejected(self) -> None:
        target = ROOT / "src" / "gui" / "elementsearch" / "ElementInfo.cpp"
        original = audit_module.read_text
        try:
            audit_module.read_text = lambda path, errors: (
                original(path, errors).replace(
                    'Tr("encyclopedia.description")', 'Tr("removed.description")'
                )
                if path == target
                else original(path, errors)
            )
            errors = audit_module.audit(ROOT)
        finally:
            audit_module.read_text = original
        self.assertTrue(any("element description label" in error for error in errors))


if __name__ == "__main__":
    unittest.main()
