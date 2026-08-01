from __future__ import annotations

import importlib.util
from pathlib import Path
import sys
import tempfile
import unittest


SCRIPT = Path(__file__).resolve().parents[1] / "element_usage_audit.py"
SPEC = importlib.util.spec_from_file_location("element_usage_audit", SCRIPT)
assert SPEC is not None and SPEC.loader is not None
element_usage_audit = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = element_usage_audit
SPEC.loader.exec_module(element_usage_audit)

ROOT = Path(__file__).resolve().parents[2]


class ElementUsageAuditTests(unittest.TestCase):
    def make_root(self) -> tuple[tempfile.TemporaryDirectory[str], Path]:
        temp = tempfile.TemporaryDirectory()
        root = Path(temp.name)
        docs = root / "docs"
        docs.mkdir()
        for name in ("ELEMENT_REGISTRY.csv", "ELEMENT_USAGE_MATRIX.csv"):
            (docs / name).write_bytes((ROOT / "docs" / name).read_bytes())
        return temp, root

    def test_repository_passes(self) -> None:
        self.assertEqual(element_usage_audit.audit(ROOT), [])

    def test_missing_element_is_rejected(self) -> None:
        temp, root = self.make_root()
        with temp:
            matrix = root / "docs" / "ELEMENT_USAGE_MATRIX.csv"
            lines = matrix.read_text(encoding="utf-8").splitlines()
            matrix.write_text(
                "\n".join(line for line in lines if not line.startswith("OMNI_PT_ALUM,")) + "\n",
                encoding="utf-8",
                newline="",
            )
            errors = element_usage_audit.audit(root)
        self.assertTrue(any("missing implemented OmniPack elements" in error for error in errors))

    def test_identity_drift_is_rejected(self) -> None:
        temp, root = self.make_root()
        with temp:
            matrix = root / "docs" / "ELEMENT_USAGE_MATRIX.csv"
            text = matrix.read_text(encoding="utf-8")
            matrix.write_text(
                text.replace("OMNI_PT_ALUM,256,ALUM,", "OMNI_PT_ALUM,999,ALUM,", 1),
                encoding="utf-8",
                newline="",
            )
            errors = element_usage_audit.audit(root)
        self.assertTrue(any("stable_id mismatch" in error for error in errors))

    def test_direct_only_without_gap_is_rejected(self) -> None:
        temp, root = self.make_root()
        with temp:
            matrix = root / "docs" / "ELEMENT_USAGE_MATRIX.csv"
            text = matrix.read_text(encoding="utf-8")
            matrix.write_text(
                text.replace(
                    "missing_production;missing_cross_module_link,link_in_0.2",
                    "missing_cross_module_link,link_in_0.2",
                    1,
                ),
                encoding="utf-8",
                newline="",
            )
            errors = element_usage_audit.audit(root)
        self.assertTrue(
            any("direct_only lacks a production disposition" in error for error in errors)
        )

    def test_intentional_direct_only_material_is_accepted(self) -> None:
        self.assertFalse(
            any(
                "OMNI_PT_HYCN" in error
                for error in element_usage_audit.audit(ROOT)
            )
        )


if __name__ == "__main__":
    unittest.main()
