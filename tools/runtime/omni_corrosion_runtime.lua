local RESULT = "omni-corrosion-runtime.result"

local function test()
    assert(type(sim.omniCorrosion) == "function", "missing OmniCorrosion API")
    sim.clearSim()
    sim.gravityMode(sim.GRAV_OFF)
    sim.edgeMode(sim.EDGE_SOLID)
    sim.omniSimulationMode(sim.OMNI_ENHANCED)
    local iron = sim.partCreate(-1, 120, 120, elem.DEFAULT_PT_IRON)
    local water = sim.partCreate(-1, 121, 120, elem.DEFAULT_PT_WATR)
    assert(iron >= 0 and water >= 0, "could not create corrosion fixture")
    local before = sim.omniCorrosion(iron)
    assert(before.active == true and before.runtime_version == 1,
        "corrosion runtime metadata unavailable")
    sim.updateUpTo()
    local metrics = sim.omniCorrosion(iron)
    assert(metrics.wet_particles >= 1 and metrics.atmosphere_oxidized_particles >= 1,
        "wet iron was not observed by corrosion runtime")
    assert(metrics.particle_progress > 0 and metrics.particle_progress < 1,
        "corrosion progress did not remain a bounded process state")
    assert(metrics.runtime_model == "iron_aqueous_oxygen_process_v1",
        "wrong corrosion runtime model")
    return metrics
end

local ok, metrics = xpcall(test, debug.traceback)
local report = assert(io.open(RESULT, "w"))
if ok then
    report:write("OMNI_CORROSION_PROGRESS=", metrics.particle_progress, "\n")
    report:write("OMNI_CORROSION_PASSIVATION=", metrics.particle_passivation, "\n")
    report:write("OMNI_CORROSION_STATUS=PASS\n")
else
    report:write("OMNI_CORROSION_ERROR=", tostring(metrics):gsub("[\r\n]+", " | "), "\n")
    report:write("OMNI_CORROSION_STATUS=FAIL\n")
end
report:close()
