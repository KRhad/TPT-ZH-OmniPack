from __future__ import annotations

from pathlib import Path
import re
import unittest


ROOT = Path(__file__).resolve().parents[2]
SIMULATION_HEADER = ROOT / "src" / "simulation" / "Simulation.h"
SIMULATION_CPP = ROOT / "src" / "simulation" / "Simulation.cpp"
LUA_SIMULATION = ROOT / "src" / "lua" / "LuaSimulation.cpp"
RUNTIME = ROOT / "tools" / "runtime" / "omni_lifecycle_ledger.lua"
WRAPPER = ROOT / "tools" / "runtime_lua_lifecycle_ledger_test.ps1"


class OmniLifecycleLedgerContractTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.header = SIMULATION_HEADER.read_text(encoding="utf-8")
        cls.cpp = SIMULATION_CPP.read_text(encoding="utf-8")
        cls.lua_simulation = LUA_SIMULATION.read_text(encoding="utf-8")
        cls.runtime = RUNTIME.read_text(encoding="utf-8")
        cls.wrapper = WRAPPER.read_text(encoding="utf-8")

    def test_observer_is_explicitly_record_only_and_disabled_by_default(self) -> None:
        self.assertIn("struct OmniLifecycleLedgerMetrics", self.header)
        self.assertIn("bool recordUnitsOnly;", self.header)
        self.assertIn("bool omniLifecycleLedgerEnabled = false;", self.header)
        self.assertIn("Audit-only record accounting", self.header)
        self.assertIn("true,\n\t\tomniLifecycleLedgerLastFrameReconciled", self.cpp)

    def test_tick_boundaries_and_all_known_direct_type_bypasses_are_instrumented(self) -> None:
        self.assertIn("BeginOmniLifecycleLedgerTick();", self.cpp)
        self.assertIn("EndOmniLifecycleLedgerTick();", self.cpp)
        for kind in (
            "OmniLifecycleMutationKind::Create",
            "OmniLifecycleMutationKind::Kill",
            "OmniLifecycleMutationKind::TypeChange",
            "OmniLifecycleMutationKind::Replacement",
            "OmniLifecycleMutationKind::SparkFastPath",
            "OmniLifecycleMutationKind::BrmtTungPreparation",
            "OmniLifecycleMutationKind::LoadFallback",
        ):
            self.assertIn(kind, self.cpp)
        self.assertRegex(
            self.cpp,
            r"parts\[index\]\.type = PT_SPRK;\s*"
            r"RecordOmniLifecycleMutation\(type, PT_SPRK, OmniLifecycleMutationKind::SparkFastPath\);",
        )
        self.assertRegex(
            self.cpp,
            r"parts\[i\]\.type = PT_TUNG;\s*"
            r"RecordOmniLifecycleMutation\(oldType, PT_TUNG, OmniLifecycleMutationKind::BrmtTungPreparation\);",
        )
        self.assertRegex(
            self.cpp,
            r"parts\[i\]\.type = 0;\s*"
            r"RecordOmniLifecycleMutation\(oldType, PT_NONE, OmniLifecycleMutationKind::LoadFallback\);",
        )

    def test_lua_surface_is_additive_and_runtime_checks_each_tick(self) -> None:
        for name in (
            "omniLifecycleLedger",
            "omniLifecycleLedgerEnabled",
            "resetOmniLifecycleLedger",
        ):
            self.assertIn(f"LFUNC({name})", self.lua_simulation)
        for field in (
            "record_units_only",
            "last_frame_reconciled",
            "last_unattributed_record_delta_abs",
            "brmt_tung_preparation_transitions",
        ):
            self.assertIn(field, self.lua_simulation)
            self.assertIn(field, self.runtime)
        self.assertIn("for tick = 1, 8 do", self.runtime)
        self.assertIn("expect_reconciled(sim.omniLifecycleLedger(), tick)", self.runtime)
        self.assertIn("sim.omniLifecycleLedgerEnabled(false)", self.runtime)
        self.assertIn("sim.omniLifecycleLedgerEnabled(true)", self.runtime)
        for secret_name in ("GITHUB_PAT_TOKEN", "GITHUB_TOKEN", "GH_TOKEN"):
            self.assertIn(secret_name, self.wrapper)
        self.assertIn('$startInfo.ArgumentList.Add("ddir")', self.wrapper)
        self.assertIn("Stop-Process -Id $process.Id -Force", self.wrapper)
        self.assertIn("if ($process) { $process.Dispose() }", self.wrapper)
        self.assertIn("OMNI_LIFECYCLE_LEDGER_STATUS=PASS", self.wrapper)


if __name__ == "__main__":
    unittest.main()
