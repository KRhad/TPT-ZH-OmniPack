from __future__ import annotations

from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[2]
SOURCE = (ROOT / "tools" / "atmospherebench" / "PrecisionMatrix.cpp").read_text(encoding="utf-8")
RUNNER = (ROOT / "tools" / "run_atmosphere_precision_matrix.py").read_text(encoding="utf-8")
MESON = (ROOT / "meson.build").read_text(encoding="utf-8")


class AtmospherePrecisionMatrixContractTests(unittest.TestCase):
    def test_same_template_has_three_real_precision_build_modes(self) -> None:
        self.assertIn("OMNI_PRECISION_DOUBLE_STRICT", SOURCE)
        self.assertIn("OMNI_PRECISION_FLOAT_STRICT", SOURCE)
        self.assertIn("OMNI_PRECISION_FLOAT_FAST", SOURCE)
        self.assertIn("using Real = double", SOURCE)
        self.assertIn("using Real = float", SOURCE)
        self.assertIn("first_order_rusanov_same_template", SOURCE)
        self.assertIn("atmosphere_precision_double_strict", MESON)
        self.assertIn("atmosphere_precision_float_strict", MESON)
        self.assertIn("atmosphere_precision_float_fast", MESON)
        self.assertIn("-ffast-math", MESON)
        self.assertIn("-fno-fast-math", MESON)

    def test_matrix_compares_conservation_positivity_and_signatures(self) -> None:
        for case in ("uniform", "pressure_pulse", "near_vacuum", "sod"):
            self.assertIn(case, RUNNER)
        self.assertIn("mass_drift", RUNNER)
        self.assertIn("energy_drift", RUNNER)
        self.assertIn("maximum_signature_delta_vs_strict_double", RUNNER)
        self.assertIn("fast_math_safety_selected", RUNNER)
        self.assertIn("atmosphere_solver_selection", RUNNER)
        self.assertIn("precision_probe_passed", RUNNER)


if __name__ == "__main__":
    unittest.main()
