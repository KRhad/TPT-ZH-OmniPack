from __future__ import annotations

import importlib.util
from pathlib import Path
import sys
import unittest


SCRIPT = Path(__file__).resolve().parents[1] / "materials_audit.py"
SPEC = importlib.util.spec_from_file_location("materials_audit", SCRIPT)
assert SPEC is not None and SPEC.loader is not None
materials_audit = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = materials_audit
SPEC.loader.exec_module(materials_audit)

ROOT = Path(__file__).resolve().parents[2]


class MaterialsAuditTests(unittest.TestCase):
    def test_repository_passes(self) -> None:
        self.assertEqual(materials_audit.audit(ROOT), [])

    def test_global_scan_is_rejected(self) -> None:
        source = (ROOT / "src" / "simulation" / "OmniMaterials.cpp").read_text(
            encoding="utf-8"
        )
        errors: list[str] = []
        original = materials_audit.read_text
        try:
            materials_audit.read_text = lambda path, _errors: (
                source + "\nfor (int index = 0; index < NPART; ++index) {}\n"
                if path.name == "OmniMaterials.cpp"
                else original(path, _errors)
            )
            materials_audit.check_engine(ROOT, errors)
        finally:
            materials_audit.read_text = original
        self.assertTrue(any("global particle" in error for error in errors))


if __name__ == "__main__":
    unittest.main()
