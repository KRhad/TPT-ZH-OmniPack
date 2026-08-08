from __future__ import annotations

from pathlib import Path
import re
import unittest


ROOT = Path(__file__).resolve().parents[2]
MESON_OPTIONS = ROOT / "meson_options.txt"
MESON_BUILD = ROOT / "meson.build"


class FloatingPointBuildModeContractTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.options = MESON_OPTIONS.read_text(encoding="utf-8")
        cls.build = MESON_BUILD.read_text(encoding="utf-8")

    def test_default_preserves_legacy_optimized_behavior(self) -> None:
        option = re.search(
            r"option\(\s*'fp_mode'.*?\n\)", self.options, re.DOTALL
        )
        self.assertIsNotNone(option)
        option_text = option.group(0)
        self.assertIn("choices: [ 'legacy_fast', 'strict' ]", option_text)
        self.assertIn("value: 'legacy_fast'", option_text)

    def test_strict_mode_explicitly_counters_fast_math(self) -> None:
        self.assertIn("if fp_mode == 'strict'", self.build)
        for flag in (
            "'-fno-fast-math'",
            "'-fno-unsafe-math-optimizations'",
            "'-ffp-contract=off'",
            "'/fp:strict'",
        ):
            self.assertIn(flag, self.build)

    def test_legacy_fast_flags_remain_in_non_debug_branch(self) -> None:
        self.assertIn("elif not is_debug", self.build)
        for flag in (
            "'-ftree-vectorize'",
            "'-fomit-frame-pointer'",
            "'-funsafe-math-optimizations'",
            "'-ffast-math'",
            "'/fp:fast'",
        ):
            self.assertIn(flag, self.build)

    def test_mode_is_visible_in_meson_configuration_output(self) -> None:
        self.assertIn("message('Floating-point mode: ' + fp_mode)", self.build)


if __name__ == "__main__":
    unittest.main()
