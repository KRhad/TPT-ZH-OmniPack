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
    assert(metrics.active == true and metrics.active_tick == false and metrics.runtime_version == 2,
        "OmniSolution metrics unavailable")
    assert(metrics.runtime_model == "aqueous_nacl_hcl_naoh_mass_v2", "wrong solution model")
    assert(metrics.dissolution_transactions >= 1 and metrics.dissolved_mass_kg > 0,
        "salt did not dissolve")
    assert(math.abs(metrics.solvent_mass_residual_kg) < 1.0e-10 and
        math.abs(metrics.solute_mass_residual_kg) < 1.0e-10,
        "solution ledgers did not close")
    local dissolved = metrics.dissolved_mass_kg
    sim.clearSim()
    local acid = sim.partCreate(-1, 360, 180, elem.DEFAULT_PT_ACID)
    local base = sim.partCreate(-1, 361, 180, elem.DEFAULT_PT_BASE)
    assert(acid >= 0 and base >= 0, "could not create neutralisation fixture")
    local temperature_before = sim.partProperty(acid, "temp")
    sim.updateUpTo()
    metrics = sim.omniSolution()
    assert(metrics.neutralisation_transactions == 1, "neutralisation must commit exactly once per tick")
    assert(metrics.neutralised_acid_mass_kg > 0 and metrics.neutralised_base_mass_kg > 0,
        "acid/base reactants were not consumed")
    assert(metrics.neutral_salt_produced_kg > 0 and metrics.neutralisation_water_produced_kg > 0,
        "neutralisation products were not generated")
    assert(metrics.neutralisation_energy_released_j > 0 and
        sim.partProperty(acid, "temp") > temperature_before,
        "neutralisation heat was not coupled to the parcels")
    assert(math.abs(metrics.total_solution_mass_residual_kg) < 1.0e-10,
        "neutralisation total mass ledger did not close")
    metrics.dissolved_mass_kg = dissolved
    return metrics
end

local ok, metrics = xpcall(test, debug.traceback)
local report = assert(io.open(RESULT, "w"))
if ok then
    report:write("OMNI_SOLUTION_DISSOLVED_KG=", metrics.dissolved_mass_kg, "\n")
    report:write("OMNI_SOLUTION_SOLVENT_RESIDUAL_KG=", metrics.solvent_mass_residual_kg, "\n")
    report:write("OMNI_SOLUTION_SOLUTE_RESIDUAL_KG=", metrics.solute_mass_residual_kg, "\n")
    report:write("OMNI_SOLUTION_NEUTRALISATION_TRANSACTIONS=", metrics.neutralisation_transactions, "\n")
    report:write("OMNI_SOLUTION_NEUTRAL_SALT_KG=", metrics.neutral_salt_produced_kg, "\n")
    report:write("OMNI_SOLUTION_NEUTRALISATION_WATER_KG=", metrics.neutralisation_water_produced_kg, "\n")
    report:write("OMNI_SOLUTION_NEUTRALISATION_ENERGY_J=", metrics.neutralisation_energy_released_j, "\n")
    report:write("OMNI_SOLUTION_TOTAL_RESIDUAL_KG=", metrics.total_solution_mass_residual_kg, "\n")
    report:write("OMNI_SOLUTION_STATUS=PASS\n")
else
    report:write("OMNI_SOLUTION_ERROR=", tostring(metrics):gsub("[\r\n]+", " | "), "\n")
    report:write("OMNI_SOLUTION_STATUS=FAIL\n")
end
report:close()
