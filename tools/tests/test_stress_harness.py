from __future__ import annotations

from pathlib import Path
import re
import unittest


ROOT = Path(__file__).resolve().parents[2]
LUA = ROOT / "tools" / "runtime" / "stress_scenarios.lua"
POWERSHELL = ROOT / "tools" / "runtime_stress_test.ps1"
SIMULATION_HEADER = ROOT / "src" / "simulation" / "Simulation.h"
SIMULATION_CPP = ROOT / "src" / "simulation" / "Simulation.cpp"
LUA_SIMULATION = ROOT / "src" / "lua" / "LuaSimulation.cpp"
EVENT_MODULES = (
    ROOT / "src" / "simulation" / "OmniMetallurgy.cpp",
    ROOT / "src" / "simulation" / "OmniBiology.cpp",
    ROOT / "src" / "simulation" / "OmniChemistry.cpp",
    ROOT / "src" / "simulation" / "OmniNuclear.cpp",
)

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
    "S11-AUTOMATION-FACTORY",
    "S12-AUTOMATION-SIGNAL-LOOP",
}


class StressHarnessContractTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.lua = LUA.read_text(encoding="utf-8")
        cls.powershell = POWERSHELL.read_text(encoding="utf-8")
        cls.simulation_header = SIMULATION_HEADER.read_text(encoding="utf-8")
        cls.simulation_cpp = SIMULATION_CPP.read_text(encoding="utf-8")
        cls.lua_simulation = LUA_SIMULATION.read_text(encoding="utf-8")
        cls.event_modules = [path.read_text(encoding="utf-8") for path in EVENT_MODULES]

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

    def test_formal_runs_bind_the_package_manifest_and_executable(self) -> None:
        self.assertIn("PackageZip is required for formal stress runs", self.powershell)
        self.assertIn("TEST-MANIFEST.txt", self.powershell)
        self.assertIn("kind=(public-test|local-dev)", self.powershell)
        self.assertIn('[ValidateSet("0.1.0-test", "0.2.0-dev", "0.3.0-dev")]', self.powershell)
        self.assertIn("Package manifest version does not match", self.powershell)
        self.assertIn("revision=([0-9a-f]{40})", self.powershell)
        self.assertIn("member=tpt-zh-omnipack", self.powershell)
        self.assertIn("Package executable size does not match", self.powershell)
        self.assertIn("Package executable hash does not match", self.powershell)
        self.assertIn("$sourceCommit = $packageProvenance.Revision", self.powershell)
        self.assertIn("harness_commit = $harnessCommit", self.powershell)
        self.assertIn("elseif (-not $Smoke)", self.powershell)
        self.assertEqual(self.powershell.count("rev-parse HEAD"), 1)

    def test_event_and_stop_evidence_are_measured_not_invented(self) -> None:
        for field in (
            "display_resolution",
            "dpi_percent",
            "language",
            "unbounded_growth",
        ):
            self.assertRegex(self.powershell, rf"{re.escape(field)} = \"not_tested\"")
        self.assertIn("event_count_total = [int64]$lua.event_count_total", self.powershell)
        self.assertIn(
            "event_count_peak_per_frame = [int64]$lua.event_count_peak_per_frame",
            self.powershell,
        )
        self.assertIn("signal_count_total = [int64]$lua.signal_count_total", self.powershell)
        self.assertIn(
            "signal_count_peak_per_frame = [int64]$lua.signal_count_peak_per_frame",
            self.powershell,
        )
        self.assertIn("signal_stop_pass = [System.Convert]::ToBoolean", self.powershell)
        self.assertIn("scenario_stop_pass = [System.Convert]::ToBoolean", self.powershell)
        self.assertIn("scenario_recovery_pass = [System.Convert]::ToBoolean", self.powershell)
        self.assertIn("scenario_recovery_assertions = [int64]$lua.scenario_recovery_assertions", self.powershell)
        self.assertIn("sim.resetOmniEventMetrics()", self.lua)
        self.assertIn("sim.omniEventMetrics()", self.lua)
        self.assertIn("scenario_stop_pass", self.lua)
        self.assertIn("scenario_recovery_pass", self.lua)
        self.assertIn("scenario_recovery_assertions", self.lua)
        self.assertIn("void ResetOmniEventMetrics();", self.simulation_header)
        self.assertIn("void RecordOmniEvent();", self.simulation_header)
        self.assertIn("void Simulation::RecordOmniEvent()", self.simulation_cpp)
        self.assertIn("omniEventCountCurrentFrame.store(0", self.simulation_cpp)
        self.assertIn("LFUNC(omniEventMetrics)", self.lua_simulation)
        self.assertIn("LFUNC(resetOmniEventMetrics)", self.lua_simulation)
        for module in self.event_modules:
            self.assertIn("sim->RecordOmniEvent();", module)

    def test_automation_stress_measures_official_signal_population(self) -> None:
        self.assertIn("local function automation_factory(bounds)", self.lua)
        self.assertIn("local function automation_signal_loop(bounds)", self.lua)
        self.assertIn("btry = assert(elements.DEFAULT_PT_BTRY)", self.lua)
        self.assertIn("make(ids.btry, x + 2, y)", self.lua)
        self.assertIn("sim.elementCount(ids.spark)", self.lua)
        self.assertIn("runtime.signal_count_total", self.lua)
        self.assertIn("runtime.signal_count_peak_per_frame", self.lua)
        self.assertIn('S11-AUTOMATION-FACTORY', self.lua)
        self.assertIn('S12-AUTOMATION-SIGNAL-LOOP', self.lua)

    def test_biology_chemistry_fixture_is_present_in_targeted_stress_samples(self) -> None:
        self.assertIn("local function ecology_chemistry_loop(bounds)", self.lua)
        self.assertIn('make(ids.path, x + 2, y + 1, { temp = 300.0 })', self.lua)
        self.assertIn('ids.pero, x + 2, y + 2', self.lua)
        self.assertIn('ids.hums, x + 2, y + 1', self.lua)
        self.assertIn('ids.fert, x + 2, y + 2', self.lua)
        self.assertIn('S04-PATHOGEN-CONTROL', self.lua)
        self.assertIn('S09-ALL-MODULES', self.lua)

    def test_slag_recovery_fixture_is_present_in_chemistry_stress(self) -> None:
        self.assertIn('acid = assert(elements.DEFAULT_PT_ACID)', self.lua)
        self.assertIn('make(ids.cata, x + 2, y + 1, { temp = 320.0 })', self.lua)
        self.assertIn('make(ids.slag, x + 2, y + 2, { temp = 320.0 })', self.lua)
        self.assertIn('make(ids.acid, x + 1, y + 2, { temp = 320.0 })', self.lua)

    def test_radiation_shield_assembly_fixture_is_present_in_reactor_stress(self) -> None:
        self.assertIn("local function spark_nichrome", self.lua)
        self.assertIn('make(ids.ssil, x + 2, y + 1, { temp = 800.0 })', self.lua)
        self.assertIn('molten(ids.lead, x + 2, y + 2, 800.0)', self.lua)
        self.assertIn('spark_nichrome(x + 1, y + 2, 800.0)', self.lua)

    def test_four_module_waste_fixture_is_present_in_all_module_stress(self) -> None:
        for marker in (
            'make(ids.nwst, x + 1, y + 1, { temp = 550.0 })',
            'make(ids.slag, x + 2, y + 1, { temp = 550.0 })',
            'make(ids.hums, x + 2, y + 2, { temp = 550.0 })',
            'make(ids.poly, x + 1, y + 2, { temp = 550.0 })',
            'make(ids.cata, x, y + 2, { temp = 550.0 })',
            'make(ids.water, x, y + 1, { temp = 550.0 })',
        ):
            self.assertIn(marker, self.lua)


if __name__ == "__main__":
    unittest.main()
