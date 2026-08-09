from __future__ import annotations

from pathlib import Path
import re
import unittest


ROOT = Path(__file__).resolve().parents[2]
SIMULATION_HEADER = ROOT / "src" / "simulation" / "Simulation.h"
SIMULATION_CPP = ROOT / "src" / "simulation" / "Simulation.cpp"
AIR_CPP = ROOT / "src" / "simulation" / "Air.cpp"
LUA_SIMULATION = ROOT / "src" / "lua" / "LuaSimulation.cpp"
RUNTIME = ROOT / "tools" / "runtime" / "omni_correction_ledger.lua"
WRAPPER = ROOT / "tools" / "runtime_lua_correction_ledger_test.ps1"


KINDS = (
    "AirAmbientHeatTemperatureCapHigh",
    "AirAmbientHeatTemperatureCapLow",
    "AirAmbientHeatVelocityXCapHigh",
    "AirAmbientHeatVelocityXCapLow",
    "AirAmbientHeatVelocityYCapHigh",
    "AirAmbientHeatVelocityYCapLow",
    "AirDynamicsPressureCapHigh",
    "AirDynamicsPressureCapLow",
    "AirDynamicsVelocityXCapHigh",
    "AirDynamicsVelocityXCapLow",
    "AirDynamicsVelocityYCapHigh",
    "AirDynamicsVelocityYCapLow",
)


class OmniCorrectionLedgerContractTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.header = SIMULATION_HEADER.read_text(encoding="utf-8")
        cls.cpp = SIMULATION_CPP.read_text(encoding="utf-8")
        cls.air = AIR_CPP.read_text(encoding="utf-8")
        cls.lua_simulation = LUA_SIMULATION.read_text(encoding="utf-8")
        cls.runtime = RUNTIME.read_text(encoding="utf-8")
        cls.wrapper = WRAPPER.read_text(encoding="utf-8")

    def test_observer_is_bounded_disabled_and_legacy_field_only(self) -> None:
        self.assertIn("struct OmniCorrectionLedgerMetrics", self.header)
        self.assertIn("bool legacyFieldUnitsOnly;", self.header)
        self.assertIn("bool auditedAirCapsOnly;", self.header)
        self.assertIn("bool omniCorrectionLedgerEnabled = false;", self.header)
        self.assertIn("OmniCorrectionLedgerEventCapacity = 256", self.header)
        self.assertIn("metrics.legacyFieldUnitsOnly = true;", self.cpp)
        self.assertIn("metrics.auditedAirCapsOnly = true;", self.cpp)
        self.assertIn("omniCorrectionLedgerDroppedEvents++", self.cpp)
        self.assertIn("omniCorrectionLedgerRetainedEvents <", self.cpp)
        self.assertIn("kindIndex >= OmniCorrectionKindCount", self.cpp)
        self.assertIn("merge them deterministically", self.header)

    def test_each_audited_air_cap_branch_records_its_executed_values(self) -> None:
        for kind in KINDS:
            self.assertEqual(
                self.air.count(f"Simulation::OmniCorrectionKind::{kind}"),
                1,
                kind,
            )
            self.assertIn(f"case Simulation::OmniCorrectionKind::{kind}:", self.lua_simulation)
        self.assertEqual(self.air.count("sim.RecordOmniCorrection("), len(KINDS))
        self.assertEqual(self.header.count("OmniCorrectionKind::Count"), 1)
        self.assertRegex(
            self.air,
            r"auto before = dp;\s*dp = MAX_PRESSURE;\s*"
            r"sim\.RecordOmniCorrection\(\s*"
            r"Simulation::OmniCorrectionKind::AirDynamicsPressureCapHigh,\s*"
            r"x, y, before, dp",
        )

    def test_lua_surface_and_real_runtime_fixture_fail_closed(self) -> None:
        for name in (
            "omniCorrectionLedger",
            "omniCorrectionLedgerEnabled",
            "resetOmniCorrectionLedger",
        ):
            self.assertIn(f"LFUNC({name})", self.lua_simulation)
        for field in (
            "legacy_field_units_only",
            "audited_air_caps_only",
            "total_events",
            "retained_events",
            "dropped_events",
            "event_capacity",
            "counts",
            "events",
        ):
            self.assertIn(field, self.lua_simulation)
            self.assertIn(field, self.runtime)
        self.assertIn("sim.walls.DEFAULT_WL_FAN", self.runtime)
        self.assertIn("sim.fanVelocityX", self.runtime)
        self.assertIn("sim.fanVelocityY", self.runtime)
        self.assertIn("sim.updateUpTo()", self.runtime)
        self.assertIn("trigger_overflow", self.runtime)
        self.assertIn("metrics.total_events > metrics.event_capacity", self.runtime)
        self.assertIn("metrics.events[#metrics.events].sequence", self.runtime)
        self.assertNotRegex(self.runtime, r"if\s+test\s*==|correction.*fixture.*branch")
        for secret_name in ("GITHUB_PAT_TOKEN", "GITHUB_TOKEN", "GH_TOKEN"):
            self.assertIn(secret_name, self.wrapper)
        self.assertIn('[string] $RuntimeDirectory = "C:\\msys64\\ucrt64\\bin"', self.wrapper)
        self.assertIn('$startInfo.Environment["PATH"]', self.wrapper)
        self.assertIn('$startInfo.ArgumentList.Add("ddir")', self.wrapper)
        self.assertIn("Stop-Process -Id $process.Id -Force", self.wrapper)
        self.assertIn("if ($process) { $process.Dispose() }", self.wrapper)
        self.assertIn("OMNI_CORRECTION_LEDGER_STATUS=PASS", self.wrapper)
        self.assertIn("OVERFLOW_RETAINED_EVENTS=256", self.wrapper)

    def test_nested_lua_event_fields_are_written_to_the_event_table(self) -> None:
        event_loop = re.search(
            r"for \(size_t index = 0; index < metrics\.retainedEvents; index\+\+\)"
            r"(?P<body>.*?)lua_rawseti\(L, -2,",
            self.lua_simulation,
            re.DOTALL,
        )
        self.assertIsNotNone(event_loop)
        body = event_loop.group("body")
        for field in ("sequence", "cell_x", "cell_y", "before", "after", "kind"):
            self.assertIn(f'lua_setfield(L, -2, "{field}")', body)


if __name__ == "__main__":
    unittest.main()
