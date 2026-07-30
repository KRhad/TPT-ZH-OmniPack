from __future__ import annotations

from pathlib import Path
import re
import unittest


ROOT = Path(__file__).resolve().parents[2]
LUA = ROOT / "tools" / "runtime" / "stress_scenarios.lua"
POWERSHELL = ROOT / "tools" / "runtime_stress_test.ps1"

SAMPLES = {
    "S01-METALLURGY-LARGE",
    "S02-FURNACES-PARALLEL",
    "S03-ECOLOGY-AREA",
    "S04-PATHOGEN-CONTROL",
    "S05-CHEMISTRY-DENSE",
    "S06-NEUTRON-GENERATORS",
    "S07-REACTOR-STABLE",
    "S08-REACTOR-LOCA",
    "S09-ALL-MODULES",
    "S10-CARRIERS-ROUNDTRIP",
}


class StressHarnessContractTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.lua = LUA.read_text(encoding="utf-8")
        cls.powershell = POWERSHELL.read_text(encoding="utf-8")

    def test_every_fixed_sample_is_implemented_and_selectable(self) -> None:
        for sample in SAMPLES:
            self.assertIn(f'["{sample}"]', self.lua)
            self.assertIn(f'"{sample}"', self.powershell)

    def test_gate_defaults_are_ten_minutes_after_one_minute_warmup(self) -> None:
        self.assertRegex(self.powershell, r"\[int\] \$WarmupSeconds = 60")
        self.assertRegex(self.powershell, r"\[int\] \$SampleSeconds = 600")
        self.assertIn("$SampleSeconds = 2", self.powershell)
        self.assertIn('gate_result = if ($Smoke) { "not_tested" }', self.powershell)

    def test_runtime_isolated_and_secrets_removed(self) -> None:
        self.assertIn('$startInfo.ArgumentList.Add("ddir")', self.powershell)
        self.assertIn('$startInfo.ArgumentList.Add($testRoot)', self.powershell)
        for name in ("GITHUB_PAT_TOKEN", "GITHUB_TOKEN", "GH_TOKEN"):
            self.assertIn(name, self.powershell)
        self.assertIn('(Join-Path $testRoot "powder.pref")', self.powershell)
        self.assertIn('"{}" + [Environment]::NewLine', self.powershell)
        self.assertNotIn("AppData", self.powershell)

    def test_long_run_returns_control_to_the_ui_event_loop(self) -> None:
        self.assertIn("event.register(event.tick, tick_callback)", self.lua)
        self.assertIn("event.unregister(event.tick, tick_callback)", self.lua)
        self.assertNotIn("while socket.getTime() - started < seconds", self.lua)

    def test_required_raw_series_and_ops_evidence_are_persisted(self) -> None:
        for filename in (
            "frame-series.csv",
            "process-series.csv",
            "input-first.stm",
            "output-second.stm",
            "result.json",
        ):
            self.assertIn(filename, self.powershell)
        self.assertIn('GetString($bytes, 0, 4) -ne "OPS1"', self.powershell)

    def test_unavailable_evidence_is_not_invented(self) -> None:
        for field in (
            "display_resolution",
            "dpi_percent",
            "language",
            "event_count_total",
            "event_count_peak_per_frame",
            "unbounded_growth",
        ):
            self.assertRegex(self.powershell, rf"{re.escape(field)} = \"not_tested\"")


if __name__ == "__main__":
    unittest.main()
