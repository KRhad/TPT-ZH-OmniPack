local RESULT = "omni-profiler-rendering.result"

local function write_result(status, fields)
    local report = assert(io.open(RESULT, "w"))
    for _, key in ipairs(fields.order) do
        report:write(key, "=", tostring(fields[key]), "\n")
    end
    report:write("OMNI_PROFILER_RENDERING_STATUS=", status, "\n")
    report:close()
end

local function run()
    -- Keep simulation live: threaded rendering is intentionally unavailable
    -- while paused, so this fixture must take the real renderer-worker path.
    sim.paused(false)
    sim.omniProfilerEnabled(true)
    sim.resetOmniProfiler()

    local ticks = 0
    local phase = "warmup"
    local toggle_cycles = 0
    local callback
    callback = function()
        ticks = ticks + 1
        local metrics = sim.omniProfiler()
        if phase == "warmup" then
            if metrics.threaded_rendering_observed then
                -- The next controller update drops the disabled model owner
                -- while a previous renderer job may still hold its span.
                sim.omniProfilerEnabled(false)
                phase = "disabled"
                return
            end
            assert(ticks < 60, "renderer worker was not observed during warmup")
            return
        end

        if phase == "disabled" then
            assert(not metrics.enabled, "profiler did not disable during live renderer test")
            sim.omniProfilerEnabled(true)
            sim.resetOmniProfiler()
            toggle_cycles = toggle_cycles + 1
            phase = "rewarm"
            return
        end

        if metrics.threaded_rendering_observed then
            local rendering = metrics.subsystems.rendering
            local copy = metrics.subsystems.render_snapshot_copy
            assert(metrics.enabled, "profiler was disabled before rendering coverage")
            assert(rendering.instrumented and copy.instrumented,
                "rendering subsystems are not marked instrumented")
            assert(rendering.calls > 0,
                "renderer worker did not record a rendering span")
            assert(copy.calls > 0,
                "normal GameView presentation copy did not record a span")
            assert(rendering.total_nanoseconds > 0,
                "rendering timing did not accumulate a positive duration")
            assert(copy.total_nanoseconds > 0,
                "presentation-copy timing did not accumulate a positive duration")

            event.unregister(event.TICK, callback)
            write_result("PASS", {
                order = {
                    "OMNI_PROFILER_RENDERING_TICKS",
                    "OMNI_PROFILER_RENDERING_TOGGLE_CYCLES",
                    "OMNI_PROFILER_THREADED_RENDERING_OBSERVED",
                    "OMNI_PROFILER_RENDERING_CALLS",
                    "OMNI_PROFILER_RENDERING_COPY_CALLS",
                    "OMNI_PROFILER_RENDERING_TOTAL_NS",
                    "OMNI_PROFILER_RENDERING_COPY_TOTAL_NS",
                },
                OMNI_PROFILER_RENDERING_TICKS = ticks,
                OMNI_PROFILER_RENDERING_TOGGLE_CYCLES = toggle_cycles,
                OMNI_PROFILER_THREADED_RENDERING_OBSERVED = metrics.threaded_rendering_observed,
                OMNI_PROFILER_RENDERING_CALLS = rendering.calls,
                OMNI_PROFILER_RENDERING_COPY_CALLS = copy.calls,
                OMNI_PROFILER_RENDERING_TOTAL_NS = rendering.total_nanoseconds,
                OMNI_PROFILER_RENDERING_COPY_TOTAL_NS = copy.total_nanoseconds,
            })
            return
        end
        assert(ticks < 120, "renderer worker was not observed after profiler re-enable")
    end
    event.register(event.TICK, callback)
end

local ok, error_text = xpcall(run, debug.traceback)
if not ok then
    write_result("FAIL", {
        order = { "OMNI_PROFILER_RENDERING_ERROR" },
        OMNI_PROFILER_RENDERING_ERROR = tostring(error_text):gsub("[\r\n]+", " | "),
    })
end
