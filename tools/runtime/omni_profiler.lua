local RESULT = "omni-profiler.result"

local function require_metric(metrics, name, expected_calls, require_positive_duration)
    local metric = assert(metrics.subsystems[name], "missing subsystem metric: " .. name)
    assert(metric.instrumented, name .. " is not marked instrumented")
    assert(metric.status == "instrumented", name .. " has unexpected status: " .. tostring(metric.status))
    assert(metric.calls == expected_calls,
        name .. " calls: expected=" .. expected_calls .. ", actual=" .. tostring(metric.calls))
    if require_positive_duration then
        assert(metric.total_nanoseconds > 0,
            name .. " did not accumulate a positive duration")
        assert(metric.maximum_nanoseconds > 0,
            name .. " did not retain a positive maximum duration")
        assert(metric.last_nanoseconds > 0,
            name .. " did not retain a positive last-span duration")
    end
    assert(metric.maximum_nanoseconds <= metric.total_nanoseconds,
        name .. " maximum duration exceeds total duration")
    return metric
end

local function configure()
    sim.clearSim()
    sim.paused(true)
    sim.gravityMode(sim.GRAV_OFF)
    sim.newtonianGravity(false)
    sim.airMode(sim.AIR_ON)
    sim.ambientHeatSim(true)
    sim.heatSim(true)
    sim.convectionMode(sim.AIRC_NONE)
    sim.edgePressure(0.0)
    sim.edgeVelocity(0.0, 0.0)
    sim.vorticityCoeff(0.0)
    sim.resetPressure()
    sim.resetVelocity()
    sim.ensureDeterminism(true)
    sim.randomSeed(109, 113, 127, 131)

    local dust = assert(elements.DEFAULT_PT_DUST)
    local water = assert(elements.DEFAULT_PT_WATR)
    for offset = 0, 7 do
        assert(sim.partCreate(-1, 240 + offset, 180, dust) >= 0,
            "failed to create dust fixture particle")
        assert(sim.partCreate(-1, 240 + offset, 184, water) >= 0,
            "failed to create water fixture particle")
    end
end

local function test_disabled_contract()
    sim.omniProfilerEnabled(false)
    local metrics = sim.omniProfiler()
    assert(not metrics.enabled, "profiler is not disabled by default")
    assert(metrics.clock == "steady_clock_monotonic", "profiler clock contract changed")
    assert(metrics.backend == "legacy_cpu_serial", "unexpected legacy backend label")
    assert(metrics.process_vram_available == false,
        "Legacy CPU profiler incorrectly claims process VRAM availability")
    assert(metrics.process_vram_bytes == "not_tested",
        "Legacy CPU profiler did not make VRAM absence explicit")
    assert(metrics.authoritative_atmosphere_state == false,
        "Legacy profiler incorrectly claims an authoritative atmosphere state")
end

local function test_runtime_metrics()
    configure()
    sim.omniProfilerEnabled(true)
    sim.resetOmniProfiler()

    -- Exercise the real BeforeSim/AfterSim callback path so the Lua timing
    -- category measures client dispatch rather than an empty handler list.
    local callback_work = 0
    local function callback()
        for value = 1, 4096 do
            callback_work = callback_work + value
        end
    end
    event.register(event.BEFORESIM, callback)
    event.register(event.AFTERSIM, callback)

    local ticks = 6
    for _ = 1, ticks do
        sim.updateUpTo()
    end

    local metrics = sim.omniProfiler()
    assert(metrics.enabled, "profiler unexpectedly disabled during tracked updates")
    assert(metrics.live_particle_records > 0, "fixture did not retain live particle records")
    assert(metrics.particle_storage_active_slots >= metrics.live_particle_records,
        "particle storage active range is smaller than live record count")
    assert(metrics.atmosphere_cells == sim.XCELLS * sim.YCELLS,
        "atmosphere cell count does not match public grid dimensions")
    assert(metrics.active_chunks_status == "not_implemented",
        "Legacy profiler incorrectly claims active chunks")
    assert(metrics.reaction_candidates_status == "not_tested_legacy_reaction_callbacks",
        "Legacy reaction-candidate scope changed unexpectedly")

    require_metric(metrics, "frame", ticks, true)
    require_metric(metrics, "simulation", ticks, true)
    require_metric(metrics, "particle_update", ticks, true)
    require_metric(metrics, "air", ticks, true)
    require_metric(metrics, "ambient_heat", ticks, true)
    -- Gravity is still dispatched every tick. With the Null backend there is no
    -- solver work, so a zero duration remains a valid measured outcome.
    require_metric(metrics, "gravity_dispatch_wait", ticks, false)
    require_metric(metrics, "lua", ticks * 2, true)
    assert(callback_work > 0, "Lua callback fixture did not execute")

    event.unregister(event.BEFORESIM, callback)
    event.unregister(event.AFTERSIM, callback)

    for _, name in ipairs({ "thermal", "chemistry", "gpu", "gpu_synchronization" }) do
        local metric = assert(metrics.subsystems[name], "missing non-instrumented metric: " .. name)
        assert(not metric.instrumented, name .. " unexpectedly claims instrumentation")
        assert(metric.calls == 0 and metric.total_nanoseconds == 0,
            name .. " unexpectedly accumulated an unscoped duration")
    end
    assert(metrics.subsystems.thermal.status == "not_instrumented_legacy_per_particle",
        "thermal scope status changed unexpectedly")
    assert(metrics.subsystems.chemistry.status == "not_instrumented_legacy_per_element",
        "chemistry scope status changed unexpectedly")
    assert(metrics.subsystems.gpu.status == "not_tested_no_gpu_backend",
        "GPU scope status changed unexpectedly")
    assert(metrics.subsystems.gpu_synchronization.status == "not_tested_no_gpu_backend",
        "GPU synchronization scope status changed unexpectedly")

    sim.resetOmniProfiler()
    local reset = sim.omniProfiler()
    assert(reset.enabled, "reset unexpectedly disabled the profiler")
    for _, metric in pairs(reset.subsystems) do
        assert(metric.calls == 0 and metric.last_nanoseconds == 0
                and metric.total_nanoseconds == 0 and metric.maximum_nanoseconds == 0,
            "reset retained profiler timing state")
    end

    sim.omniProfilerEnabled(false)
    assert(not sim.omniProfilerEnabled(), "profiler did not disable")
    return metrics
end

local function test()
    test_disabled_contract()
    return test_runtime_metrics()
end

local ok, metrics_or_error = xpcall(test, debug.traceback)
local report = assert(io.open(RESULT, "w"))
if ok then
    local metrics = metrics_or_error
    report:write("OMNI_PROFILER_BACKEND=", metrics.backend, "\n")
    report:write("OMNI_PROFILER_CLOCK=", metrics.clock, "\n")
    report:write("OMNI_PROFILER_LIVE_PARTICLES=", metrics.live_particle_records, "\n")
    report:write("OMNI_PROFILER_ATMOSPHERE_CELLS=", metrics.atmosphere_cells, "\n")
    report:write("OMNI_PROFILER_FRAME_CALLS=", metrics.subsystems.frame.calls, "\n")
    report:write("OMNI_PROFILER_SIMULATION_CALLS=", metrics.subsystems.simulation.calls, "\n")
    report:write("OMNI_PROFILER_PARTICLE_CALLS=", metrics.subsystems.particle_update.calls, "\n")
    report:write("OMNI_PROFILER_AIR_CALLS=", metrics.subsystems.air.calls, "\n")
    report:write("OMNI_PROFILER_AMBIENT_HEAT_CALLS=", metrics.subsystems.ambient_heat.calls, "\n")
    report:write("OMNI_PROFILER_GRAVITY_CALLS=", metrics.subsystems.gravity_dispatch_wait.calls, "\n")
    report:write("OMNI_PROFILER_LUA_CALLS=", metrics.subsystems.lua.calls, "\n")
    report:write("OMNI_PROFILER_FRAME_TOTAL_NS=", metrics.subsystems.frame.total_nanoseconds, "\n")
    report:write("OMNI_PROFILER_SIMULATION_TOTAL_NS=", metrics.subsystems.simulation.total_nanoseconds, "\n")
    report:write("OMNI_PROFILER_PARTICLE_TOTAL_NS=", metrics.subsystems.particle_update.total_nanoseconds, "\n")
    report:write("OMNI_PROFILER_AIR_TOTAL_NS=", metrics.subsystems.air.total_nanoseconds, "\n")
    report:write("OMNI_PROFILER_AMBIENT_HEAT_TOTAL_NS=", metrics.subsystems.ambient_heat.total_nanoseconds, "\n")
    report:write("OMNI_PROFILER_GRAVITY_TOTAL_NS=", metrics.subsystems.gravity_dispatch_wait.total_nanoseconds, "\n")
    report:write("OMNI_PROFILER_LUA_TOTAL_NS=", metrics.subsystems.lua.total_nanoseconds, "\n")
    report:write("OMNI_PROFILER_PROCESS_VRAM_AVAILABLE=", tostring(metrics.process_vram_available), "\n")
    report:write("OMNI_PROFILER_PROCESS_VRAM_STATUS=", metrics.process_vram_status, "\n")
    report:write("OMNI_PROFILER_STATUS=PASS\n")
else
    report:write("OMNI_PROFILER_ERROR=", tostring(metrics_or_error):gsub("[\r\n]+", " | "), "\n")
    report:write("OMNI_PROFILER_STATUS=FAIL\n")
end
report:close()
