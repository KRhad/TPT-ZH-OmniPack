from __future__ import annotations

import math
import unittest


U64_MAX = (1 << 64) - 1


def reference_arm(start: int, old_start: int, fps: float) -> tuple[int, int]:
    if not math.isfinite(fps) or fps <= 0:
        return start, 0
    duration = min(max(int(1_000_000_000 / fps), 1), 1_000_000_000)
    block = old_start // duration
    next_block = min(block + 1, U64_MAX)
    next_start = min(next_block * duration, U64_MAX)
    new_start = max(start, next_start)
    return new_start, new_start - start


class FrameScheduleDifferentialTest(unittest.TestCase):
    def test_reference_edges(self) -> None:
        for fps in (0.0, -1.0, float("nan"), float("inf")):
            self.assertEqual(reference_arm(123, 99, fps), (123, 0))
        for fps in (1.0, 60.0, 1_000_000_000.0):
            start, delay = reference_arm(0, 0, fps)
            self.assertGreaterEqual(start, 1)
            self.assertEqual(delay, start)

    def test_reference_is_monotonic_and_saturating(self) -> None:
        start = U64_MAX - 2
        old_start = U64_MAX - 3
        next_start, delay = reference_arm(start, old_start, 60.0)
        self.assertGreaterEqual(next_start, start)
        self.assertEqual(next_start, U64_MAX)
        self.assertEqual(delay, 2)

    def test_header_contains_same_guards(self) -> None:
        from pathlib import Path

        source = (Path(__file__).resolve().parents[2] / "src" / "FrameSchedule.h").read_text()
        self.assertIn("std::isfinite(fps)", source)
        self.assertIn("fps <= 0.0f", source)
        self.assertIn("numeric_limits<uint64_t>::max()", source)
        self.assertIn("std::max(startNs, nextStartNs)", source)


if __name__ == "__main__":
    unittest.main()
