from __future__ import annotations

from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[2]
FRAME_TIME_H = ROOT / "src" / "simulation" / "FrameTime.h"
FRAME_TIME_CPP = ROOT / "src" / "simulation" / "FrameTime.cpp"
SIMULATION_CPP = ROOT / "src" / "simulation" / "Simulation.cpp"
GAME_MODEL_CPP = ROOT / "src" / "gui" / "game" / "GameModel.cpp"
GAME_MODEL_H = ROOT / "src" / "gui" / "game" / "GameModel.h"
GAME_CONTROLLER_CPP = ROOT / "src" / "gui" / "game" / "GameController.cpp"
GAME_CONTROLLER_H = ROOT / "src" / "gui" / "game" / "GameController.h"
GAME_VIEW_CPP = ROOT / "src" / "gui" / "game" / "GameView.cpp"
LUA_SIMULATION = ROOT / "src" / "lua" / "LuaSimulation.cpp"
RUNTIME_LUA = ROOT / "tools" / "runtime" / "omni_profiler.lua"
RUNTIME_WRAPPER = ROOT / "tools" / "runtime_lua_profiler_test.ps1"
RENDERING_RUNTIME_LUA = ROOT / "tools" / "runtime" / "omni_profiler_rendering.lua"
RENDERING_RUNTIME_WRAPPER = ROOT / "tools" / "runtime_lua_profiler_rendering_test.ps1"
THREAD_PROBE = ROOT / "tools" / "omni_profiler_thread_probe.cpp"
MESON = ROOT / "meson.build"


class OmniProfilerContractTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.frame_time_h = FRAME_TIME_H.read_text(encoding="utf-8")
        cls.frame_time_cpp = FRAME_TIME_CPP.read_text(encoding="utf-8")
        cls.simulation_cpp = SIMULATION_CPP.read_text(encoding="utf-8")
        cls.game_model_cpp = GAME_MODEL_CPP.read_text(encoding="utf-8")
        cls.game_model_h = GAME_MODEL_H.read_text(encoding="utf-8")
        cls.game_controller_cpp = GAME_CONTROLLER_CPP.read_text(encoding="utf-8")
        cls.game_controller_h = GAME_CONTROLLER_H.read_text(encoding="utf-8")
        cls.game_view_cpp = GAME_VIEW_CPP.read_text(encoding="utf-8")
        cls.lua_simulation = LUA_SIMULATION.read_text(encoding="utf-8")
        cls.runtime_lua = RUNTIME_LUA.read_text(encoding="utf-8")
        cls.runtime_wrapper = RUNTIME_WRAPPER.read_text(encoding="utf-8")
        cls.rendering_runtime_lua = RENDERING_RUNTIME_LUA.read_text(encoding="utf-8")
        cls.rendering_runtime_wrapper = RENDERING_RUNTIME_WRAPPER.read_text(encoding="utf-8")
        cls.thread_probe = THREAD_PROBE.read_text(encoding="utf-8")
        cls.meson = MESON.read_text(encoding="utf-8")

    def test_profiler_uses_monotonic_clock_and_is_optional(self) -> None:
        self.assertIn("std::chrono::steady_clock", self.frame_time_h)
        self.assertIn("bool subsystemProfilerEnabled = false", self.frame_time_h)
        self.assertIn("subsystemProfilerGeneration", self.frame_time_h)
        self.assertIn("SetSubsystemProfilerEnabled", self.frame_time_h)
        self.assertIn("ResetSubsystemProfiler", self.frame_time_h)
        self.assertIn("OptionalFrame", self.frame_time_h)
        self.assertIn("HasActiveFrame", self.frame_time_h)
        self.assertIn("generation != subsystemProfilerGeneration", self.frame_time_cpp)

    def test_subsystem_ids_are_stable_and_scope_is_explicit(self) -> None:
        expected = (
            "Frame",
            "Simulation",
            "ParticleUpdate",
            "Air",
            "AmbientHeat",
            "Gravity",
            "Thermal",
            "Chemistry",
            "Lua",
            "RenderSnapshotCopy",
            "Rendering",
            "Gpu",
            "GpuSynchronization",
        )
        for subsystem in expected:
            self.assertIn(subsystem, self.frame_time_h)
        for label in (
            'return "frame"',
            'return "particle_update"',
            'return "gravity_dispatch_wait"',
            'return "gpu_synchronization"',
        ):
            self.assertIn(label, self.frame_time_cpp)

    def test_real_legacy_boundaries_are_instrumented_without_solver_changes(self) -> None:
        self.assertIn("FrameTime::Subsystem::ParticleUpdate", self.game_model_cpp)
        self.assertIn("FrameTime::Subsystem::Lua", self.game_model_cpp)
        self.assertIn("FrameTime::Subsystem::Air", self.simulation_cpp)
        self.assertIn("FrameTime::Subsystem::AmbientHeat", self.simulation_cpp)
        self.assertIn("FrameTime::Subsystem::Gravity", self.simulation_cpp)
        self.assertIn("FrameTime::Subsystem::Rendering", self.game_view_cpp)
        self.assertIn("FrameTime::Subsystem::RenderSnapshotCopy", self.game_view_cpp)
        self.assertNotIn("OmniAtmosphere", self.game_model_cpp)

    def test_lua_api_exports_fail_closed_capability_fields(self) -> None:
        for function in (
            "static int omniProfiler(lua_State *L)",
            "static int omniProfilerEnabled(lua_State *L)",
            "static int resetOmniProfiler(lua_State *L)",
        ):
            self.assertIn(function, self.lua_simulation)
        for api in (
            "LFUNC(omniProfiler)",
            "LFUNC(omniProfilerEnabled)",
            "LFUNC(resetOmniProfiler)",
        ):
            self.assertIn(api, self.lua_simulation)
        for status in (
            '"not_instrumented_legacy_per_particle"',
            '"not_instrumented_legacy_per_element"',
            '"not_tested_no_gpu_backend"',
            '"not_tested"',
        ):
            self.assertIn(status, self.lua_simulation)
        self.assertIn('setBoolean("authoritative_atmosphere_state", false)', self.lua_simulation)

    def test_runtime_fixture_uses_public_api_and_checks_reset(self) -> None:
        self.assertIn("sim.omniProfilerEnabled", self.runtime_lua)
        self.assertIn("sim.resetOmniProfiler()", self.runtime_lua)
        self.assertIn("sim.updateUpTo()", self.runtime_lua)
        self.assertIn("event.register(event.BEFORESIM, callback)", self.runtime_lua)
        self.assertIn("event.unregister(event.BEFORESIM, callback)", self.runtime_lua)
        self.assertNotIn("RecordSubsystem", self.runtime_lua)
        self.assertIn("OMNI_PROFILER_STATUS=PASS", self.runtime_lua)

    def test_wrapper_isolated_and_vram_status_is_explicit(self) -> None:
        self.assertIn('$startInfo.ArgumentList.Add("ddir")', self.runtime_wrapper)
        self.assertIn('$startInfo.Environment["PATH"]', self.runtime_wrapper)
        self.assertIn('(Join-Path $resolvedTestRoot "powder.pref")', self.runtime_wrapper)
        self.assertIn('"{}" + [Environment]::NewLine', self.runtime_wrapper)
        self.assertIn("Refusing to create profiler test outside", self.runtime_wrapper)
        self.assertIn("OMNI_PROFILER_PROCESS_VRAM_AVAILABLE=false", self.runtime_wrapper)
        self.assertIn("OMNI_PROFILER_PROCESS_VRAM_STATUS=not_tested_no_gpu_backend", self.runtime_wrapper)
        for secret in ("GITHUB_PAT_TOKEN", "GITHUB_TOKEN", "GH_TOKEN"):
            self.assertIn(secret, self.runtime_wrapper)

    def test_thread_probe_exercises_concurrent_render_aggregation(self) -> None:
        self.assertIn("std::thread renderingThread", self.thread_probe)
        self.assertIn("FrameTime::Subsystem::Rendering", self.thread_probe)
        self.assertIn("RenderingSpanCount = 4096", self.thread_probe)
        self.assertIn("rendering_calls=", self.thread_probe)

        self.assertIn("std::weak_ptr<FrameTime>", self.thread_probe)
        self.assertIn("lifetimeFrameTime->ResetSubsystemProfiler()", self.thread_probe)
        self.assertIn("lifetimeFrameTime->SetSubsystemProfilerEnabled(false)", self.thread_probe)
        self.assertIn("lifetime_disable_reset_race=PASS", self.thread_probe)
        self.assertIn("frame_time_bytes=", self.thread_probe)
        self.assertIn("omni_profiler_thread_probe", self.meson)
        self.assertIn("omni-profiler-thread-probe", self.meson)

    def test_rendering_fixture_uses_the_normal_ui_loop(self) -> None:
        self.assertIn("event.register(event.TICK, callback)", self.rendering_runtime_lua)
        self.assertIn("sim.paused(false)", self.rendering_runtime_lua)
        self.assertNotIn("sim.paused(true)", self.rendering_runtime_lua)
        self.assertIn("metrics.subsystems.rendering", self.rendering_runtime_lua)
        self.assertIn("metrics.subsystems.render_snapshot_copy", self.rendering_runtime_lua)
        self.assertIn("metrics.threaded_rendering_observed", self.rendering_runtime_lua)
        self.assertIn("sim.omniProfilerEnabled(false)", self.rendering_runtime_lua)
        self.assertIn("sim.omniProfilerEnabled(true)", self.rendering_runtime_lua)
        self.assertNotIn("RenderSimulation(", self.rendering_runtime_lua)
        self.assertIn('report:write("OMNI_PROFILER_RENDERING_STATUS=", status', self.rendering_runtime_lua)
        self.assertIn('$startInfo.Environment["PATH"]', self.rendering_runtime_wrapper)
        self.assertIn('(Join-Path $resolvedTestRoot "powder.pref")', self.rendering_runtime_wrapper)
        self.assertIn('"{}" + [Environment]::NewLine', self.rendering_runtime_wrapper)
        self.assertIn("OMNI_PROFILER_RENDERING_COPY_CALLS", self.rendering_runtime_wrapper)
        self.assertIn("OMNI_PROFILER_THREADED_RENDERING_OBSERVED=true", self.rendering_runtime_wrapper)

    def test_renderer_spans_hold_a_safe_profiler_owner(self) -> None:
        self.assertIn("std::shared_ptr<FrameTime> frameTime", self.game_model_h)
        self.assertIn("std::mutex frameTimeMutex", self.game_model_h)
        self.assertIn("std::shared_ptr<FrameTime> GetFrameTime() const", self.game_controller_h)
        self.assertIn("return gameModel->GetFrameTime()", self.game_controller_cpp)
        self.assertIn("std::shared_ptr<FrameTime> owner", self.frame_time_h)
        self.assertIn("FrameTime::SubsystemSpan renderingSpan(std::move(frameTime)", self.game_view_cpp)
        self.assertIn("RecordThreadedRenderingObserved", self.game_view_cpp)

    def test_async_last_duration_is_not_misattributed_to_a_ui_frame(self) -> None:
        self.assertIn("This is the last completed span", self.frame_time_cpp)
        self.assertIn("metric.lastNanoseconds = value", self.frame_time_cpp)
        self.assertNotIn("currentSubsystemNanoseconds", self.frame_time_cpp)


if __name__ == "__main__":
    unittest.main()
