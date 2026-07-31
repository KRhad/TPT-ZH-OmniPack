local RESULT = "lua-periodic-regression.result"

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
    sodium = must_element("OMNI_PT_NA", "NA", 376),
    potassium = must_element("OMNI_PT_K", "K", 380),
    caesium = must_element("OMNI_PT_CS", "CS", 406),
    francium = must_element("OMNI_PT_FR", "FR", 432),
    beryllium = must_element("OMNI_PT_BE", "BE", 371),
    calcium = must_element("OMNI_PT_CA", "CA", 381),
    strontium = must_element("OMNI_PT_SR", "SR", 391),
    barium = must_element("OMNI_PT_BA", "BA", 407),
    radium = must_element("OMNI_PT_RA", "RA", 433),
    magnesium = must_element("OMNI_PT_MAGN", "MAGN", 261),
    lithium = must_element("DEFAULT_PT_LITH", "LITH", 191),
    rubidium = must_element("DEFAULT_PT_RBDM", "RBDM", 41),
    hydrogen = must_element("DEFAULT_PT_H2", "HYGN", 148),
    water = must_element("DEFAULT_PT_WATR", "WATR", 2),
    acid = must_element("DEFAULT_PT_ACID", "ACID", 21),
    caustic = must_element("DEFAULT_PT_CAUS", "CAUS", 86),
    salt = must_element("DEFAULT_PT_SALT", "SALT", 26),
    oxygen = must_element("DEFAULT_PT_O2", "OXYG", 61),
    lava = must_element("DEFAULT_PT_LAVA", "LAVA", 6),
    fire = must_element("DEFAULT_PT_FIRE", "FIRE", 4),
    metal = assert(elements.DEFAULT_PT_METL),
    electron = assert(elements.DEFAULT_PT_ELEC),
    photon = assert(elements.DEFAULT_PT_PHOT),
    polonium = assert(elements.DEFAULT_PT_POLO),
    scrap = must_element("OMNI_PT_MSCR", "MSCR", 278),
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
    assert(elements.property(ids.sodium, "HighTemperature")
            > elements.property(ids.caesium, "HighTemperature"),
        "sodium and caesium do not retain distinct melting points")
    assert(elements.property(ids.francium, "Properties")
            ~= elements.property(ids.potassium, "Properties"),
        "radioactive francium and stable potassium have identical properties")
    assert(elements.property(ids.beryllium, "Hardness")
            > elements.property(ids.calcium, "Hardness"),
        "beryllium and calcium do not retain distinct hardness")
    assert(elements.property(ids.radium, "Properties")
            ~= elements.property(ids.strontium, "Properties"),
        "radioactive radium and stable strontium have identical properties")
end

local function run_water_reaction(type, seed)
    configure(seed)
    sim.airMode(sim.AIR_NOUPDATE)
    local water = make(ids.water, 121, 120, 293.15)
    -- Update the water first so the post-reaction H2 remains observable for
    -- this frame.  If the metal has the lower particle index, the newly
    -- produced H2 may immediately ignite on the bounded FIRE side product.
    local metal = make(type, 120, 120, 293.15)
    step()
    assert(sim.partExists(metal) and sim.partProperty(metal, "type") == ids.caustic,
        "alkali metal did not become bounded caustic residue")
    assert(sim.partExists(water) and sim.partProperty(water, "type") == ids.hydrogen,
        "alkali water reaction did not produce hydrogen")
    return sim.pressure(30, 30)
end

local function run_alkali_water_series()
    local sodium_pressure = run_water_reaction(ids.sodium, 131)
    local potassium_pressure = run_water_reaction(ids.potassium, 141)
    local caesium_pressure = run_water_reaction(ids.caesium, 151)
    local francium_pressure = run_water_reaction(ids.francium, 161)
    assert(sodium_pressure < potassium_pressure
            and potassium_pressure < caesium_pressure
            and caesium_pressure < francium_pressure,
        "alkali water-reaction severity does not increase down the family")

    configure(171)
    local lithium = make(ids.lithium, 120, 120, 293.15)
    local water = make(ids.water, 121, 120, 293.15)
    step()
    assert(sim.partProperty(lithium, "type") == ids.lithium
            and sim.partProperty(water, "type") == ids.hydrogen,
        "reused official lithium no longer performs its water-to-hydrogen reaction")

    configure(181)
    local old_advection = elements.property(ids.water, "Advection")
    local old_gravity = elements.property(ids.water, "Gravity")
    local old_falldown = elements.property(ids.water, "Falldown")
    elements.property(ids.water, "Advection", 0.0)
    elements.property(ids.water, "Gravity", 0.0)
    elements.property(ids.water, "Falldown", 0)
    make(ids.rubidium, 120, 120, 293.15)
    water = make(ids.water, 121, 120, 300.0)
    for _ = 1, 600 do
        step()
        if sim.partProperty(water, "type") == ids.fire then break end
    end
    elements.property(ids.water, "Advection", old_advection)
    elements.property(ids.water, "Gravity", old_gravity)
    elements.property(ids.water, "Falldown", old_falldown)
    assert(sim.partProperty(water, "type") == ids.fire,
        "reused official rubidium no longer ignites adjacent warm water")
end

local function run_alkali_acid_oxygen_and_phase()
    configure(191)
    local acid = make(ids.acid, 121, 120, 293.15)
    local sodium = make(ids.sodium, 120, 120, 293.15)
    step()
    assert(sim.partProperty(sodium, "type") == ids.salt
            and sim.partProperty(acid, "type") == ids.hydrogen,
        "sodium acid reaction did not produce generic salt and hydrogen")

    configure(201)
    local molten = make(ids.lava, 120, 120, 700.0)
    sim.partProperty(molten, "ctype", ids.potassium)
    local oxygen = make(ids.oxygen, 121, 120, 700.0)
    step()
    assert(sim.partProperty(molten, "type") == ids.salt
            and sim.partProperty(oxygen, "type") == ids.fire,
        "molten potassium did not use the bounded hot-oxygen route")

    configure(211)
    local water = make(ids.water, 121, 120, 293.15)
    molten = make(ids.lava, 120, 120, 600.0)
    sim.partProperty(molten, "ctype", ids.caesium)
    step()
    assert(sim.partProperty(molten, "type") == ids.caustic
            and sim.partProperty(water, "type") == ids.hydrogen,
        "molten caesium ctype did not retain its water reaction")

    configure(221)
    molten = make(ids.lava, 120, 120, 1200.0)
    sim.partProperty(molten, "ctype", ids.sodium)
    step()
    assert(sim.partProperty(molten, "type") == ids.fire
            and sim.partProperty(molten, "ctype") == ids.sodium
            and sim.partProperty(molten, "life") == 60,
        "hot molten sodium did not enter its finite vaporisation proxy")
end

local function run_alkaline_earth_water_reaction(type, seed)
    configure(seed)
    sim.airMode(sim.AIR_NOUPDATE)
    local water = make(ids.water, 121, 120, 293.15)
    local metal = make(type, 120, 120, 293.15)
    step()
    assert(sim.partExists(metal) and sim.partProperty(metal, "type") == ids.caustic,
        "alkaline-earth metal did not become bounded caustic residue")
    assert(sim.partExists(water) and sim.partProperty(water, "type") == ids.hydrogen,
        "alkaline-earth water reaction did not produce hydrogen")
    return sim.pressure(30, 30)
end

local function run_alkaline_earth_water_series()
    local calcium_pressure = run_alkaline_earth_water_reaction(ids.calcium, 241)
    local strontium_pressure = run_alkaline_earth_water_reaction(ids.strontium, 251)
    local barium_pressure = run_alkaline_earth_water_reaction(ids.barium, 261)
    local radium_pressure = run_alkaline_earth_water_reaction(ids.radium, 271)
    assert(calcium_pressure < strontium_pressure
            and strontium_pressure < barium_pressure
            and barium_pressure < radium_pressure,
        "alkaline-earth water-reaction severity does not increase down the family")

    configure(281)
    local water = make(ids.water, 121, 120, 293.15)
    local beryllium = make(ids.beryllium, 120, 120, 293.15)
    step()
    assert(sim.partProperty(beryllium, "type") == ids.beryllium
            and sim.partProperty(water, "type") == ids.water,
        "beryllium incorrectly reacts with cool water")

    configure(291)
    local acid = make(ids.acid, 121, 120, 340.0)
    beryllium = make(ids.beryllium, 120, 120, 293.15)
    step()
    assert(sim.partProperty(beryllium, "type") == ids.salt
            and sim.partProperty(acid, "type") == ids.hydrogen,
        "warm acid did not overcome beryllium passivation")

    configure(301)
    acid = make(ids.acid, 121, 120, 293.15)
    local magnesium = make(ids.magnesium, 120, 120, 293.15)
    step()
    assert(sim.partProperty(magnesium, "type") == ids.salt
            and sim.partProperty(acid, "type") == ids.hydrogen,
        "reused magnesium did not enter the shared acid route")
end

local function run_flame_signature(type, temperature, colour, seed)
    configure(seed)
    local oxygen = make(ids.oxygen, 121, 120, temperature)
    local metal = make(type, 120, 120, temperature)
    step()
    assert(sim.partProperty(metal, "type") == ids.salt,
        "hot alkaline-earth metal did not oxidize to generic salt")
    assert(sim.partProperty(oxygen, "type") == ids.fire,
        "hot alkaline-earth oxidation did not create finite fire")
    assert(sim.partProperty(oxygen, "dcolour") == colour,
        "alkaline-earth flame signature colour changed")
end

local function run_alkaline_earth_oxygen_and_phase()
    run_flame_signature(ids.calcium, 700.0, 0xFFFF8A35, 311)
    run_flame_signature(ids.strontium, 700.0, 0xFFFF3030, 321)
    run_flame_signature(ids.barium, 700.0, 0xFF66FF66, 331)

    configure(341)
    local old_diffusion = elements.property(ids.oxygen, "Diffusion")
    local old_advection = elements.property(ids.oxygen, "Advection")
    elements.property(ids.oxygen, "Diffusion", 0.0)
    elements.property(ids.oxygen, "Advection", 0.0)
    local oxygen = make(ids.oxygen, 121, 120, 900.0)
    local magnesium = make(ids.magnesium, 120, 120, 900.0)
    for _ = 1, 120 do
        step()
        if sim.partProperty(magnesium, "type") == ids.scrap then break end
    end
    elements.property(ids.oxygen, "Diffusion", old_diffusion)
    elements.property(ids.oxygen, "Advection", old_advection)
    assert(sim.partProperty(magnesium, "type") == ids.scrap,
        "reused magnesium no longer burns to typed recoverable scrap")
    assert(not sim.partExists(oxygen)
            or sim.partProperty(oxygen, "type") ~= ids.oxygen,
        "magnesium combustion did not consume local oxygen")
    local white_flame = false
    for particle in sim.parts() do
        if sim.partProperty(particle, "type") == ids.fire
                and sim.partProperty(particle, "dcolour") == 0xFFFFFFFF then
            white_flame = true
            break
        end
    end
    assert(white_flame, "magnesium combustion lost its bright white flame signature")

    configure(351)
    local water = make(ids.water, 121, 120, 293.15)
    local molten = make(ids.lava, 120, 120, 1200.0)
    sim.partProperty(molten, "ctype", ids.calcium)
    step()
    assert(sim.partProperty(molten, "type") == ids.caustic
            and sim.partProperty(water, "type") == ids.hydrogen,
        "molten calcium ctype did not retain its water reaction")

    configure(361)
    molten = make(ids.lava, 120, 120, 1600.0)
    sim.partProperty(molten, "ctype", ids.barium)
    step()
    assert(sim.partProperty(molten, "type") == ids.fire
            and sim.partProperty(molten, "ctype") == ids.barium
            and sim.partProperty(molten, "life") == 60,
        "hot molten barium did not enter its finite vaporisation proxy")
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

    configure(116)
    local francium = make(ids.francium, 120, 120, 293.15)
    sim.partProperty(francium, "tmp", 1)
    step()
    assert(sim.partProperty(francium, "type") == ids.polonium,
        "francium did not enter its compressed polonium decay proxy")
    assert(count_type(ids.photon) == 1,
        "one francium decay did not emit exactly one finite photon")

    configure(117)
    local radium = make(ids.radium, 120, 120, 293.15)
    sim.partProperty(radium, "tmp", 1)
    step()
    assert(sim.partProperty(radium, "type") == ids.rn,
        "radium did not enter its compressed radon decay proxy")
    assert(sim.partProperty(radium, "tmp") >= 1200,
        "radium decay did not initialize the radon lifetime")
    assert(count_type(ids.photon) == 1,
        "one radium decay did not emit exactly one finite photon")
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

local function run_francium_budget()
    configure(231)
    local total = 1200
    for index = 0, total - 1 do
        local particle = make(ids.francium,
            50 + (index % 100) * 5,
            50 + math.floor(index / 100) * 5,
            293.15)
        sim.partProperty(particle, "tmp", 1)
    end
    step()
    local metrics = sim.omniEventMetrics()
    local events = assert(tonumber(metrics.total), "missing francium event total")
    assert(events > 0 and events <= 1024,
        "one-frame francium events were not capped at 1024: " .. tostring(events))
    assert(count_type(ids.francium) >= total - 1024,
        "event-budget exhaustion did not defer remaining francium decays")
    assert(count_type(ids.photon) <= 1024,
        "bounded francium decays emitted too many photons")
    return events
end

local function run_radium_budget()
    configure(371)
    local total = 1200
    for index = 0, total - 1 do
        local particle = make(ids.radium,
            50 + (index % 100) * 5,
            50 + math.floor(index / 100) * 5,
            293.15)
        sim.partProperty(particle, "tmp", 1)
    end
    step()
    local metrics = sim.omniEventMetrics()
    local events = assert(tonumber(metrics.total), "missing radium event total")
    assert(events > 0 and events <= 1024,
        "one-frame radium events were not capped at 1024: " .. tostring(events))
    assert(count_type(ids.radium) >= total - 1024,
        "event-budget exhaustion did not defer remaining radium decays")
    assert(count_type(ids.photon) <= 1024,
        "bounded radium decays emitted too many photons")
    return events
end

local function test()
    run_property_differences()
    run_alkali_water_series()
    run_alkali_acid_oxygen_and_phase()
    run_alkaline_earth_water_series()
    run_alkaline_earth_oxygen_and_phase()
    run_helium_cryogenics()
    run_xenon_discharge()
    run_radioactive_decays()
    local noble_budget = run_decay_budget()
    local francium_budget = run_francium_budget()
    local radium_budget = run_radium_budget()
    return math.max(noble_budget, francium_budget, radium_budget)
end

local ok, data = xpcall(test, debug.traceback)
local report = assert(io.open(RESULT, "w"))
if ok then
    report:write("OMNI_PERIODIC_STATUS=PASS\n")
    report:write("OMNI_PERIODIC_NEW_ELEMENTS=16\n")
    report:write("OMNI_PERIODIC_IMPLEMENTED_MAPPINGS=42\n")
    report:write("OMNI_PERIODIC_ALKALI_FAMILY=6\n")
    report:write("OMNI_PERIODIC_ALKALINE_EARTH_FAMILY=6\n")
    report:write("OMNI_PERIODIC_BUDGET_EVENTS=" .. tostring(data) .. "\n")
else
    local error_text = tostring(data):gsub("[\r\n]+", " | ")
    report:write("OMNI_PERIODIC_STATUS=FAIL\n")
    report:write("OMNI_PERIODIC_ERROR=" .. error_text .. "\n")
end
report:close()
