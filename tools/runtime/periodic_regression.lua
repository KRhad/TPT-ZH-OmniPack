local RESULT = "lua-periodic-noble-gas-regression.result"

local function must_element(identifier, short_name, stable_id)
    local id = elements[identifier]
    assert(type(id) == "number", "missing element constant: " .. identifier)
    assert(elements.getByName(short_name) == id,
        "name/identifier mismatch for " .. identifier)
    assert(id == stable_id,
        identifier .. " stable ID changed: expected " .. stable_id
            .. " got " .. tostring(id))
    return id
end

local ids = {
    he = must_element("OMNI_PT_HE", "HE", 370),
    ne = must_element("OMNI_PT_NE", "NE", 375),
    ar = must_element("OMNI_PT_AR", "AR", 379),
    kr = must_element("OMNI_PT_KR", "KR", 390),
    xe = must_element("OMNI_PT_XE", "XE", 405),
    rn = must_element("OMNI_PT_RN", "RN", 431),
    og = must_element("OMNI_PT_OG", "OG", 461),
    hydrogen = must_element("DEFAULT_PT_H2", "HYGN", 148),
    metal = assert(elements.DEFAULT_PT_METL),
    electron = assert(elements.DEFAULT_PT_ELEC),
    photon = assert(elements.DEFAULT_PT_PHOT),
    polonium = assert(elements.DEFAULT_PT_POLO),
}

local function configure(seed)
    sim.clearSim()
    sim.paused(true)
    sim.gravityMode(sim.GRAV_OFF)
    sim.airMode(sim.AIR_OFF)
    sim.ambientHeatSim(false)
    sim.heatSim(false)
    sim.ensureDeterminism(true)
    sim.randomSeed(seed or 71, 72, 73, 74)
    sim.resetOmniEventMetrics()
end

local function make(type, x, y, temperature)
    local particle = sim.partCreate(-1, x, y, type)
    assert(particle >= 0, "failed to create particle type " .. tostring(type))
    if temperature then sim.partProperty(particle, "temp", temperature) end
    return particle
end

local function step(frames)
    for _ = 1, frames or 1 do sim.updateUpTo() end
end

local function count_type(type)
    local count = 0
    for particle in sim.parts() do
        if sim.partProperty(particle, "type") == type then count = count + 1 end
    end
    return count
end

local function run_property_differences()
    assert(elements.property(ids.he, "HeatConduct")
            > elements.property(ids.xe, "HeatConduct"),
        "helium is not more thermally conductive than xenon")
    assert(elements.property(ids.he, "Diffusion")
            > elements.property(ids.rn, "Diffusion"),
        "light and heavy noble gases have identical diffusion")
    assert(elements.property(ids.rn, "Properties")
            ~= elements.property(ids.ne, "Properties"),
        "radioactive and stable noble gases have identical properties")
end

local function run_helium_cryogenics()
    configure(81)
    local old_diffusion = elements.property(ids.he, "Diffusion")
    local old_advection = elements.property(ids.he, "Advection")
    elements.property(ids.he, "Diffusion", 0.0)
    elements.property(ids.he, "Advection", 0.0)
    local helium = make(ids.he, 120, 120, 10.0)
    local metal = make(ids.metal, 121, 120, 300.0)
    step()
    elements.property(ids.he, "Diffusion", old_diffusion)
    elements.property(ids.he, "Advection", old_advection)
    assert(sim.partProperty(helium, "temp") > 10.0,
        "cold helium did not absorb bounded local heat")
    assert(sim.partProperty(metal, "temp") < 300.0,
        "cold helium did not cool its local neighbour")
end

local function run_xenon_discharge()
    configure(91)
    local old_diffusion = elements.property(ids.xe, "Diffusion")
    local old_advection = elements.property(ids.xe, "Advection")
    elements.property(ids.xe, "Diffusion", 0.0)
    elements.property(ids.xe, "Advection", 0.0)
    local xenon = make(ids.xe, 120, 120, 300.0)
    local initial_photons = count_type(ids.photon)
    for _ = 1, 40 do
        local electron = make(ids.electron, 121, 120, 300.0)
        sim.partProperty(electron, "vx", 0.0)
        sim.partProperty(electron, "vy", 0.0)
        step()
        if sim.partExists(electron) then sim.partKill(electron) end
        if count_type(ids.photon) > initial_photons then break end
    end
    elements.property(ids.xe, "Diffusion", old_diffusion)
    elements.property(ids.xe, "Advection", old_advection)
    assert(sim.partExists(xenon) and sim.partProperty(xenon, "life") > 0,
        "xenon did not enter its finite discharge-glow state")
    assert(count_type(ids.photon) > initial_photons,
        "xenon discharge did not emit a photon")
    local metrics = sim.omniEventMetrics()
    assert(tonumber(metrics.peak_per_frame) <= 1024,
        "xenon discharge exceeded the periodic event budget")
end

local function run_radioactive_decays()
    configure(101)
    local radon = make(ids.rn, 120, 120, 300.0)
    sim.partProperty(radon, "tmp", 1)
    step()
    assert(sim.partProperty(radon, "type") == ids.polonium,
        "radon did not decay to polonium")
    assert(count_type(ids.photon) == 1,
        "one radon decay did not emit exactly one finite photon")

    configure(111)
    local oganesson = make(ids.og, 120, 120, 300.0)
    sim.partProperty(oganesson, "tmp", 1)
    step()
    assert(sim.partProperty(oganesson, "type") == ids.rn,
        "oganesson did not enter the compressed radon decay proxy")
    assert(sim.partProperty(oganesson, "tmp") >= 1200,
        "oganesson decay did not initialize the radon lifetime")
    assert(count_type(ids.photon) == 1,
        "one oganesson decay did not emit exactly one finite photon")
end

local function run_decay_budget()
    configure(121)
    local total = 1200
    for index = 0, total - 1 do
        local particle = make(ids.og, 50 + (index % 50), 50 + math.floor(index / 50), 300.0)
        sim.partProperty(particle, "tmp", 1)
    end
    step()
    local metrics = sim.omniEventMetrics()
    local events = assert(tonumber(metrics.total), "missing periodic event total")
    local peak = assert(tonumber(metrics.peak_per_frame), "missing periodic event peak")
    assert(events > 0 and events <= 1024,
        "one-frame decay events were not capped at 1024: " .. tostring(events))
    assert(peak <= 1024,
        "one-frame periodic peak exceeded 1024: " .. tostring(peak))
    assert(count_type(ids.og) >= total - 1024,
        "event-budget exhaustion did not defer remaining oganesson decays")
    assert(count_type(ids.photon) <= 1024,
        "bounded decays emitted too many photons")
    return events
end

local function test()
    run_property_differences()
    run_helium_cryogenics()
    run_xenon_discharge()
    run_radioactive_decays()
    return run_decay_budget()
end

local ok, data = xpcall(test, debug.traceback)
local report = assert(io.open(RESULT, "w"))
if ok then
    report:write("OMNI_PERIODIC_STATUS=PASS\n")
    report:write("OMNI_PERIODIC_NEW_ELEMENTS=7\n")
    report:write("OMNI_PERIODIC_IMPLEMENTED_MAPPINGS=33\n")
    report:write("OMNI_PERIODIC_BUDGET_EVENTS=" .. tostring(data) .. "\n")
else
    local error_text = tostring(data):gsub("[\r\n]+", " | ")
    report:write("OMNI_PERIODIC_STATUS=FAIL\n")
    report:write("OMNI_PERIODIC_ERROR=" .. error_text .. "\n")
end
report:close()
