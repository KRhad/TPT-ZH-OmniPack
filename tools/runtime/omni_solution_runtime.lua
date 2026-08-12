local RESULT = "omni-solution-runtime.result"

local function test()
    assert(type(sim.omniSolution) == "function", "missing OmniSolution API")
    sim.clearSim()
    sim.gravityMode(sim.GRAV_OFF)
    sim.edgeMode(sim.EDGE_SOLID)
    sim.omniSimulationMode(sim.OMNI_ENHANCED)
    local water = sim.partCreate(-1, 200, 180, elem.DEFAULT_PT_WATR)
    local salt = sim.partCreate(-1, 201, 180, elem.DEFAULT_PT_SALT)
    assert(water >= 0 and salt >= 0, "could not create solution fixture")
    sim.updateUpTo()
    local metrics = sim.omniSolution()
    assert(metrics.active == true and metrics.active_tick == false and metrics.runtime_version == 1,
        "OmniSolution metrics unavailable")
    assert(metrics.runtime_model == "aqueous_nacl_mass_fraction_v1", "wrong solution model")
    assert(metrics.dissolution_transactions >= 1 and metrics.dissolved_mass_kg > 0,
        "salt did not dissolve")
    assert(math.abs(metrics.solvent_mass_residual_kg) < 1.0e-10 and
        math.abs(metrics.solute_mass_residual_kg) < 1.0e-10,
        "solution ledgers did not close")
    return metrics
end

local ok, metrics = xpcall(test, debug.traceback)
local report = assert(io.open(RESULT, "w"))
if ok then
    report:write("OMNI_SOLUTION_DISSOLVED_KG=", metrics.dissolved_mass_kg, "\n")
    report:write("OMNI_SOLUTION_SOLVENT_RESIDUAL_KG=", metrics.solvent_mass_residual_kg, "\n")
    report:write("OMNI_SOLUTION_SOLUTE_RESIDUAL_KG=", metrics.solute_mass_residual_kg, "\n")
    report:write("OMNI_SOLUTION_STATUS=PASS\n")
else
    report:write("OMNI_SOLUTION_ERROR=", tostring(metrics):gsub("[\r\n]+", " | "), "\n")
    report:write("OMNI_SOLUTION_STATUS=FAIL\n")
end
report:close()

