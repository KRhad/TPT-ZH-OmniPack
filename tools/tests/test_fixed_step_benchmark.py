from __future__ import annotations

from pathlib import Path
import re
import unittest


ROOT = Path(__file__).resolve().parents[2]
LUA = ROOT / "tools" / "runtime" / "fixed_step_benchmark.lua"
POWERSHELL = ROOT / "tools" / "runtime_fixed_step_benchmark.ps1"
GITIGNORE = ROOT / ".gitignore"
LUA_SIMULATION = ROOT / "src" / "lua" / "LuaSimulation.cpp"
SNAPSHOT = ROOT / "src" / "simulation" / "Snapshot.cpp"


class FixedStepBenchmarkContractTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.lua = LUA.read_text(encoding="utf-8")
        cls.powershell = POWERSHELL.read_text(encoding="utf-8")
        cls.gitignore = GITIGNORE.read_text(encoding="utf-8")
        cls.lua_simulation = LUA_SIMULATION.read_text(encoding="utf-8")
        cls.snapshot = SNAPSHOT.read_text(encoding="utf-8")

    def test_runner_has_two_deterministic_generated_scenarios(self) -> None:
        for scenario in ("empty", "mixed-medium"):
            self.assertIn(f'["{scenario}"]', self.lua)
            self.assertIn(f'"{scenario}"', self.powershell)
        self.assertIn("sim.clearSim()", self.lua)
        self.assertIn("local function reset_world()", self.lua)
        self.assertIn("Legacy clearSim does not clear", self.lua)
        self.assertIn("sim.ensureDeterminism(true)", self.lua)
        self.assertIn(
            "sim.randomSeed(seed[1], seed[2], seed[3], seed[4])", self.lua
        )
        self.assertIn("same-process deterministic replay diverged", self.lua)

    def test_timed_region_contains_only_fixed_step_dispatch(self) -> None:
        timed = re.search(
            r"local started = socket\.getTime\(\)\s*"
            r"for _ = 1, steps_per_pass do\s*"
            r"sim\.updateUpTo\(\)\s*"
            r"end\s*"
            r"local elapsed_seconds = socket\.getTime\(\) - started",
            self.lua,
        )
        self.assertIsNotNone(timed)
        self.assertIn('collectgarbage("collect")', self.lua[: timed.start()])
        self.assertNotIn("event.register", self.lua)
        self.assertNotIn("sim.hash()", timed.group(0))

    def test_fixed_step_and_snapshot_hash_apis_are_real_client_apis(self) -> None:
        self.assertIn("static int updateUpTo(lua_State *L)", self.lua_simulation)
        self.assertIn("lsi->gameModel->SetQueuedFrames(1);", self.lua_simulation)
        self.assertIn("lsi->gameModel->UpdateUpTo(upTo + 1);", self.lua_simulation)
        self.assertIn("static int hash(lua_State *L)", self.lua_simulation)
        self.assertIn("lsi->sim->CreateSnapshot()->Hash()", self.lua_simulation)
        for field in (
            "AirPressure",
            "AirVelocityX",
            "AirVelocityY",
            "AmbientHeat",
            "Particles",
            "FrameCount",
            "RngState",
        ):
            self.assertIn(field, self.snapshot)

    def test_wrapper_isolates_profile_and_runtime_dependencies(self) -> None:
        self.assertIn('$startInfo.ArgumentList.Add("ddir")', self.powershell)
        self.assertIn('$startInfo.ArgumentList.Add($testRoot)', self.powershell)
        self.assertIn('(Join-Path $testRoot "powder.pref")', self.powershell)
        self.assertIn('"LuaHookTimeout"', self.powershell)
        self.assertIn('$startInfo.Environment["PATH"]', self.powershell)
        self.assertIn("runtime_path_injected", self.powershell)
        self.assertIn("portable_runtime_tested = $false", self.powershell)
        for secret in ("GITHUB_PAT_TOKEN", "GITHUB_TOKEN", "GH_TOKEN"):
            self.assertIn(secret, self.powershell)

    def test_wrapper_records_source_binary_build_and_machine_provenance(self) -> None:
        for field in (
            "worktree_state_sha256",
            "harness_powershell_sha256",
            "lua_generator_sha256",
            "run_config_sha256",
            "simulation_compile_command",
            "fp_mode",
            "compiler",
            "exe",
            "power_mode",
            "physical_memory_bytes",
            "gpu_nvidia_smi",
        ):
            self.assertIn(field, self.powershell)
        self.assertIn("intro-buildoptions.json", self.powershell)
        self.assertIn("intro-compilers.json", self.powershell)
        self.assertIn("compile_commands.json", self.powershell)
        self.assertIn("src/simulation/Simulation", self.powershell)

    def test_result_separates_execution_from_performance_gate(self) -> None:
        self.assertIn('benchmark_execution_status = "PASS"', self.powershell)
        self.assertIn('performance_gate = "not_evaluated"', self.powershell)
        self.assertIn('subsystem_timings_available = $false', self.powershell)
        self.assertIn('peak_process_vram_bytes = "not_tested"', self.powershell)
        self.assertIn('benchmark_kind = "client_fixed_step_simulation"', self.powershell)
        self.assertIn("aggregate_steps_per_second", self.powershell)
        self.assertIn("passes_detail", self.powershell)

    def test_timing_boundaries_are_explicit_in_json(self) -> None:
        for field in (
            "scene_generation_in_timed_region = $false",
            "reset_sanitization_in_timed_region = $false",
            "warmup_in_timed_region = $false",
            "state_signature_in_timed_region = $false",
            "rendering_in_timed_region = $false",
            "fps_wait_in_timed_region = $false",
            "lua_dispatch_in_timed_region = $true",
        ):
            self.assertIn(field, self.powershell)
        self.assertIn("not a pure C++ microbenchmark", self.powershell)

    def test_raw_artifacts_are_ignored_and_cleanup_is_bounded(self) -> None:
        self.assertIn('[string] $OutputDirectory = "artifacts/vnext-benchmark"', self.powershell)
        self.assertIn("/artifacts/", self.gitignore)
        self.assertIn("function Remove-IsolatedRoot", self.powershell)
        self.assertIn("$attempt -le 20", self.powershell)
        self.assertIn("$process.Dispose()", self.powershell)
        self.assertIn("Refusing to remove a benchmark directory", self.powershell)


if __name__ == "__main__":
    unittest.main()
