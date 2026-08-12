local RESULT = "omni-chemistry-runtime.result"

local function finite(value)
    return value == value and value ~= math.huge and value ~= -math.huge
end

local function test()
    assert(type(sim.omniChemistry) == "function", "missing Omni chemistry API")
    sim.clearSim()
    sim.gravityMode(sim.GRAV_OFF)
    sim.edgeMode(sim.EDGE_SOLID)
    sim.paused(false)
    sim.omniSimulationMode(sim.OMNI_ENHANCED)
    local coal = sim.partCreate(-1, 240, 180, elem.DEFAULT_PT_COAL)
    assert(coal >= 0, "could not create Enhanced COAL")
    sim.partProperty(coal, "temp", 1200.0)
    sim.omniProfilerEnabled(true)
    sim.resetOmniProfiler()
    sim.updateUpTo()
    local metrics = sim.omniChemistry()
    local profile = sim.omniProfiler()
    sim.omniProfilerEnabled(false)
    assert(metrics.active == true and metrics.active_tick == false and metrics.runtime_version == 1,
        "chemistry metrics unavailable")
    assert(metrics.committed_transactions > 0, "Enhanced carbon combustion did not commit")
    assert(metrics.carbon_consumed_kg > 0 and metrics.oxygen_consumed_kg > 0 and
        metrics.carbon_dioxide_produced_kg > 0 and metrics.chemical_energy_released_j > 0,
        "chemistry transaction did not transfer mass and energy")
    assert(finite(metrics.mass_residual_kg) and math.abs(metrics.mass_residual_kg) < 5.0e-12 and
        finite(metrics.carbon_atom_residual_mol) and math.abs(metrics.carbon_atom_residual_mol) < 5.0e-10 and
        finite(metrics.oxygen_atom_residual_mol) and math.abs(metrics.oxygen_atom_residual_mol) < 5.0e-10 and
        finite(metrics.energy_residual_j) and math.abs(metrics.energy_residual_j) < 1.0e-9,
        "chemistry ledger did not close")
    local chemistry = assert(profile.subsystems.chemistry, "missing chemistry profiler subsystem")
    assert(chemistry.instrumented == true and
        chemistry.status == "instrumented_omni_reaction_runtime" and chemistry.calls > 0,
        "chemistry profiler did not observe OmniReactionRuntime")
    return metrics, chemistry
end

local ok, metrics, chemistry = xpcall(test, debug.traceback)
local report = assert(io.open(RESULT, "w"))
if ok then
    report:write("OMNI_CHEMISTRY_TRANSACTIONS=", metrics.committed_transactions, "\n")
    report:write("OMNI_CHEMISTRY_O2_CONSUMED_KG=", metrics.oxygen_consumed_kg, "\n")
    report:write("OMNI_CHEMISTRY_CO2_PRODUCED_KG=", metrics.carbon_dioxide_produced_kg, "\n")
    report:write("OMNI_CHEMISTRY_ENERGY_J=", metrics.chemical_energy_released_j, "\n")
    report:write("OMNI_CHEMISTRY_PROFILE_CALLS=", chemistry.calls, "\n")
    report:write("OMNI_CHEMISTRY_STATUS=PASS\n")
else
    report:write("OMNI_CHEMISTRY_ERROR=", tostring(metrics):gsub("[\r\n]+", " | "), "\n")
    report:write("OMNI_CHEMISTRY_STATUS=FAIL\n")
end
report:close()
