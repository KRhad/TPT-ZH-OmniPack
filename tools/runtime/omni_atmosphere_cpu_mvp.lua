local RESULT = "omni-atmosphere-cpu-mvp.result"

local function finite(value)
    return value == value and value ~= math.huge and value ~= -math.huge
end

local function test()
    assert(type(sim.omniSimulationMode) == "function", "missing Omni mode API")
    assert(type(sim.omniAtmosphere) == "function", "missing Omni atmosphere API")
    assert(sim.omniSimulationMode() == sim.OMNI_CLASSIC, "new runtime did not default to Classic")

    sim.clearSim()
    sim.edgeMode(sim.EDGE_VOID)
    sim.gravityMode(sim.GRAV_OFF)
    sim.newtonianGravity(false)
    sim.airMode(sim.AIR_ON)
    sim.paused(false)
    sim.omniSimulationMode(sim.OMNI_ENHANCED)

    local initial = sim.omniAtmosphere()
    assert(initial.active and initial.available, "Enhanced atmosphere is not active")
    assert(initial.state_version == 3, "Enhanced atmosphere state version is not v3")
    assert(initial.state_serialization_supported == true,
        "Enhanced atmosphere does not advertise state serialization support")
    assert(initial.state_serialized == false and initial.serialization_status == "fresh_preset",
        "fresh Enhanced atmosphere has an invalid persistence status")
    assert(initial.migration_degraded == false, "fresh Enhanced atmosphere is marked degraded")
    assert(initial.width == sim.XCELLS and initial.height == sim.YCELLS,
        "Enhanced atmosphere dimensions do not match Legacy cells")
    assert(initial.mass_kg > 0 and initial.energy_j > 0, "Enhanced atmosphere has no initial state")
    local expectedSpecies = { "N2", "O2", "Ar", "CO2", "H2O" }
    assert(type(initial.species_registry) == "table" and #initial.species_registry == #expectedSpecies,
        "Enhanced atmosphere species registry has the wrong size")
    local speciesMassSum = 0.0
    for index, species in ipairs(expectedSpecies) do
        assert(initial.species_registry[index] == species, "Enhanced atmosphere species registry order changed")
        assert(finite(initial.species_mass_kg[species]) and initial.species_mass_kg[species] >= 0,
            "Enhanced atmosphere species mass is invalid")
        speciesMassSum = speciesMassSum + initial.species_mass_kg[species]
    end
    assert(math.abs(speciesMassSum - initial.mass_kg) < 1.0e-10,
        "Enhanced atmosphere species masses do not sum to total gas mass")

    local selected = sim.omniAtmosphere(10, 10).selected_cell
    assert(selected.x == 10 and selected.y == 10 and finite(selected.density_kg_m3) and
        finite(selected.pressure_pa) and finite(selected.temperature_k) and
        finite(selected.relative_humidity), "selected-cell atmosphere diagnostics are invalid")
    local massFractionSum = 0.0
    local partialPressureSum = 0.0
    for _, species in ipairs(expectedSpecies) do
        massFractionSum = massFractionSum + selected.mass_fraction[species]
        partialPressureSum = partialPressureSum + selected.partial_pressure_pa[species]
    end
    assert(math.abs(massFractionSum - 1.0) < 1.0e-12,
        "selected-cell species mass fractions do not sum to one")
    assert(math.abs(partialPressureSum - selected.pressure_pa) < 1.0e-8,
        "selected-cell partial pressures do not sum to mixture pressure")

    -- Activating Enhanced synchronizes the already-selected Legacy edge mode
    -- into the new atmosphere.  A real topology change is intentionally routed
    -- through one bounded acoustic tick, even when the freshly initialized
    -- state is uniform.  The following quiet tick must return to the one-step
    -- low-Mach bulk path.
    sim.updateUpTo()
    local activation = sim.omniAtmosphere()
    assert(activation.active and activation.acoustic_route,
        "Enhanced activation did not route its boundary synchronization event")
    assert(activation.timestep_limited and activation.advanced_timestep_s < activation.requested_timestep_s,
        "Enhanced activation hid its bounded acoustic timestep")

    sim.updateUpTo()
    local after = sim.omniAtmosphere()
    assert(after.active and after.substeps == 1, "quiet Open Enhanced tick did not use the low-Mach bulk path")
    assert(after.timestep_limited == false, "quiet Open Enhanced tick was incorrectly CFL-limited")
    assert(after.acoustic_route == false, "matching Open reference state incorrectly activated the acoustic route")
    assert(math.abs(after.requested_timestep_s - 1.0 / 60.0) < 1.0e-12,
        "Enhanced fixed timestep contract drifted")
    assert(math.abs(after.advanced_timestep_s - after.requested_timestep_s) < 1.0e-12,
        "quiet Enhanced tick did not advance the full fixed timestep")
    assert(after.non_finite_cells == 0, "Enhanced tick produced non-finite cells")
    assert(finite(after.mass_residual_kg) and math.abs(after.mass_residual_kg) < 1.0e-10,
        "Enhanced mass ledger did not close")
    assert(finite(after.momentum_x_residual) and math.abs(after.momentum_x_residual) < 1.0e-10,
        "Enhanced X momentum ledger did not close")
    assert(finite(after.momentum_y_residual) and math.abs(after.momentum_y_residual) < 1.0e-10,
        "Enhanced Y momentum ledger did not close")
    assert(finite(after.energy_residual_j) and math.abs(after.energy_residual_j) < 1.0e-5,
        "Enhanced energy ledger did not close")

    sim.edgeMode(sim.EDGE_SOLID)

    sim.updateUpTo()
    local sealedBoundary = sim.omniAtmosphere()
    assert(sealedBoundary.acoustic_route and sealedBoundary.timestep_limited,
        "runtime sealed-boundary topology change did not use the acoustic route")
    assert(finite(sealedBoundary.mass_residual_kg) and math.abs(sealedBoundary.mass_residual_kg) < 1.0e-10,
        "runtime sealed-boundary mass ledger did not close")
    assert(finite(sealedBoundary.momentum_x_residual) and math.abs(sealedBoundary.momentum_x_residual) < 1.0e-10,
        "runtime sealed-boundary X momentum ledger did not close")
    assert(finite(sealedBoundary.momentum_y_residual) and math.abs(sealedBoundary.momentum_y_residual) < 1.0e-10,
        "runtime sealed-boundary Y momentum ledger did not close")
    assert(finite(sealedBoundary.energy_residual_j) and math.abs(sealedBoundary.energy_residual_j) < 1.0e-5,
        "runtime sealed-boundary energy ledger did not close")

    assert(socket and type(socket.getTime) == "function", "socket.getTime is unavailable")
    local benchmarkTicks = 32
    local benchmarkStart = socket.getTime()
    for _ = 1, benchmarkTicks do
        sim.updateUpTo()
    end
    local benchmarkMsPerTick = (socket.getTime() - benchmarkStart) * 1000.0 / benchmarkTicks
    assert(finite(benchmarkMsPerTick) and benchmarkMsPerTick > 0.0,
        "Enhanced runtime benchmark did not produce a finite duration")
    local benchmarkState = sim.omniAtmosphere()

    sim.omniProfilerEnabled(true)
    sim.resetOmniProfiler()
    local profileTicks = 16
    for _ = 1, profileTicks do
        sim.updateUpTo()
    end
    local profile = sim.omniProfiler()
    sim.omniProfilerEnabled(false)

    local pressureBeforeSource = sim.pressure(10, 10)
    sim.pressure(10, 10, pressureBeforeSource + 5.0)
    sim.updateUpTo()
    local event = sim.omniAtmosphere()
    assert(event.active and event.non_finite_cells == 0, "Legacy pressure source destabilized Enhanced state")
    assert(event.acoustic_route and event.timestep_limited and event.advanced_timestep_s < event.requested_timestep_s,
        "compressible event did not expose bounded physical-time slowdown")

    sim.updateUpTo()
    local persistedEvent = sim.omniAtmosphere()
    assert(persistedEvent.acoustic_route and persistedEvent.timestep_limited and
        persistedEvent.advanced_timestep_s < persistedEvent.requested_timestep_s,
        "compressible event fell off the acoustic route after its source frame")
    assert(persistedEvent.non_finite_cells == 0 and
        finite(persistedEvent.mass_residual_kg) and math.abs(persistedEvent.mass_residual_kg) < 1.0e-10 and
        finite(persistedEvent.energy_residual_j) and math.abs(persistedEvent.energy_residual_j) < 1.0e-5,
        "persisted compressible event did not close its runtime ledger")

    sim.omniSimulationMode(sim.OMNI_CLASSIC)
    local classic = sim.omniAtmosphere()
    assert(not classic.active and classic.mode == sim.OMNI_CLASSIC,
        "Classic opt-out did not restore Legacy mode")
    return after, event, persistedEvent, benchmarkMsPerTick, benchmarkState, profile, profileTicks
end

local ok, result, event, persistedEvent, benchmarkMsPerTick, benchmarkState, profile, profileTicks =
    xpcall(test, debug.traceback)
local report = assert(io.open(RESULT, "w"))
if ok then
    report:write("OMNI_ATMOSPHERE_MODE=", result.mode, "\n")
    report:write("OMNI_ATMOSPHERE_CELLS=", result.width * result.height, "\n")
    report:write("OMNI_ATMOSPHERE_SUBSTEPS=", result.substeps, "\n")
    report:write("OMNI_ATMOSPHERE_TIMESTEP_LIMITED=", tostring(result.timestep_limited), "\n")
    report:write("OMNI_ATMOSPHERE_MASS_RESIDUAL_KG=", result.mass_residual_kg, "\n")
    report:write("OMNI_ATMOSPHERE_ENERGY_RESIDUAL_J=", result.energy_residual_j, "\n")
    report:write("OMNI_ATMOSPHERE_EVENT_TIMESTEP_LIMITED=", tostring(event.timestep_limited), "\n")
    report:write("OMNI_ATMOSPHERE_EVENT_PERSISTED_ACOUSTIC=", tostring(persistedEvent.acoustic_route), "\n")
    report:write("OMNI_ATMOSPHERE_EVENT_SECOND_ADVANCED_S=", persistedEvent.advanced_timestep_s, "\n")
    report:write("OMNI_ATMOSPHERE_RUNTIME_MS_PER_TICK=", benchmarkMsPerTick, "\n")
    report:write("OMNI_ATMOSPHERE_BENCHMARK_ACOUSTIC_ROUTE=", tostring(benchmarkState.acoustic_route), "\n")
    report:write("OMNI_ATMOSPHERE_BENCHMARK_SUBSTEPS=", benchmarkState.substeps, "\n")
    for _, name in ipairs({ "frame", "simulation", "particle_update", "air", "ambient_heat", "gravity_dispatch_wait", "lua" }) do
        local metric = assert(profile.subsystems[name], "missing profiler subsystem " .. name)
        report:write("OMNI_ATMOSPHERE_PROFILE_", name:upper(), "_MS_PER_TICK=",
            metric.total_nanoseconds / 1000000.0 / profileTicks, "\n")
    end
    report:write("OMNI_ATMOSPHERE_STATUS=PASS\n")
else
    report:write("OMNI_ATMOSPHERE_ERROR=", tostring(result):gsub("[\r\n]+", " | "), "\n")
    report:write("OMNI_ATMOSPHERE_STATUS=FAIL\n")
end
report:close()
