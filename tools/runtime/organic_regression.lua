local RESULT = "lua-organic-regression.result"

local function must_element(identifier, short_name)
    local id = elements[identifier]
    assert(type(id) == "number", "missing element constant: " .. identifier)
    assert(elements.getByName(short_name) == id,
        "name/identifier mismatch for " .. identifier)
    return id
end

local ids = {
    water = assert(elements.DEFAULT_PT_WATR),
    steam = assert(elements.DEFAULT_PT_WTRV),
    hydrogen = assert(elements.DEFAULT_PT_H2),
    oxygen = assert(elements.DEFAULT_PT_O2),
    carbon_dioxide = assert(elements.DEFAULT_PT_CO2),
    oil = assert(elements.DEFAULT_PT_OIL),
    gas = assert(elements.DEFAULT_PT_GAS),
    smoke = assert(elements.DEFAULT_PT_SMKE),
    soap = assert(elements.DEFAULT_PT_SOAP),
    caustic = assert(elements.DEFAULT_PT_CAUS),
    nitroglycerin = assert(elements.DEFAULT_PT_NITR),
    catalyst = must_element("OMNI_PT_CATA", "CATA"),
    carbon_monoxide = must_element("OMNI_PT_COMO", "COMO"),
    ammonia = must_element("OMNI_PT_AMON", "AMON"),
    ammonia_water = must_element("OMNI_PT_AMWA", "AMWA"),
    ethanol = must_element("OMNI_PT_ETHL", "ETHL"),
    acetylene = must_element("OMNI_PT_ACTY", "ACTY"),
    nitric_acid = must_element("OMNI_PT_NITA", "NITA"),
    polyethylene = must_element("OMNI_PT_POLY", "POLY"),
    methane = must_element("OMNI_PT_CH4M", "MTHN"),
    ethane = must_element("OMNI_PT_ETHA", "ETHA"),
    propane = must_element("OMNI_PT_PROP", "PROP"),
    butane = must_element("OMNI_PT_BUTA", "BUTA"),
    ethylene = must_element("OMNI_PT_ETHE", "ETHE"),
    methanol = must_element("OMNI_PT_METH", "MEOH"),
    acetone = must_element("OMNI_PT_ACET", "ACET"),
    benzene = must_element("OMNI_PT_BENZ", "BENZ"),
    toluene = must_element("OMNI_PT_TOLU", "TOLU"),
    glycerol = must_element("OMNI_PT_GLYC", "GLYC"),
    acetic_acid = must_element("OMNI_PT_ACTA", "ACTA"),
    urea = must_element("OMNI_PT_UREA", "UREA"),
    fats = must_element("OMNI_PT_FATS", "FATS"),
}

assert(ids.methane == 589 and ids.fats == 601,
    "organic stable range changed: expected 589..601")
assert(ids.polyethylene == 367,
    "canonical POLY stable ID changed instead of being refined in place")
assert(ids.methane ~= ids.gas and ids.fats ~= ids.oil,
    "precise organic materials silently reused generic official GAS/OIL")

local function configure()
    sim.clearSim()
    sim.paused(true)
    sim.gravityMode(sim.GRAV_OFF)
    sim.airMode(sim.AIR_OFF)
    sim.ambientHeatSim(false)
    sim.heatSim(true)
    sim.ensureDeterminism(true)
    sim.randomSeed(31, 37, 41, 43)
    sim.resetOmniEventMetrics()
end

local function make(element, x, y, temperature)
    local particle = sim.partCreate(-1, x, y, element)
    assert(particle >= 0, "failed to create element " .. tostring(element))
    if temperature then sim.partProperty(particle, "temp", temperature) end
    return particle
end

local function count_type(element)
    local count = 0
    for particle in sim.parts() do
        if sim.partProperty(particle, "type") == element then count = count + 1 end
    end
    return count
end

local function run_methane_synthesis()
    configure()
    make(ids.catalyst, 120, 120, 600.0)
    local carbon_dioxide = make(ids.carbon_dioxide, 121, 120, 600.0)
    local hydrogen = make(ids.hydrogen, 120, 121, 600.0)
    sim.updateUpTo()
    assert(sim.partProperty(carbon_dioxide, "type") == ids.methane
            and sim.partProperty(hydrogen, "type") == ids.water,
        "bounded methane synthesis did not produce methane and water")
end

local function run_methane_reforming()
    configure()
    local methane = make(ids.methane, 120, 120, 900.0)
    local steam = make(ids.steam, 121, 120, 900.0)
    make(ids.catalyst, 120, 121, 900.0)
    sim.updateUpTo()
    assert(sim.partProperty(methane, "type") == ids.hydrogen
            and sim.partProperty(steam, "type") == ids.carbon_monoxide,
        "methane steam reforming did not produce hydrogen and carbon monoxide")
end

local function run_cracking(name, source_type, second_product, temperature)
    configure()
    local source = make(source_type, 120, 120, temperature)
    make(ids.catalyst, 121, 120, temperature)
    sim.updateUpTo()
    assert(sim.partProperty(source, "type") == ids.ethylene
            and count_type(second_product) == 1,
        name .. " did not produce ethylene and its bounded coproduct")
end

local function run_polymerisation()
    configure()
    local first = make(ids.ethylene, 120, 120, 500.0)
    local second = make(ids.ethylene, 121, 120, 500.0)
    make(ids.catalyst, 120, 121, 500.0)
    sim.updateUpTo()
    assert(sim.partProperty(first, "type") == ids.polyethylene
            and sim.partProperty(second, "type") == ids.polyethylene,
        "ethylene polymerisation did not preserve two POLY particles")
end

local function run_methanol_oxidation()
    configure()
    local methanol = make(ids.methanol, 120, 120, 500.0)
    local oxygen = make(ids.oxygen, 121, 120, 500.0)
    make(ids.catalyst, 120, 121, 500.0)
    sim.updateUpTo()
    assert(sim.partProperty(methanol, "type") == ids.carbon_monoxide
            and sim.partProperty(oxygen, "type") == ids.water,
        "methanol oxidation did not produce carbon monoxide and water")
end

local function run_ethanol_oxidation()
    configure()
    local ethanol = make(ids.ethanol, 120, 120, 400.0)
    local oxygen = make(ids.oxygen, 121, 120, 400.0)
    make(ids.catalyst, 120, 121, 400.0)
    sim.updateUpTo()
    assert(sim.partProperty(ethanol, "type") == ids.acetic_acid
            and sim.partProperty(oxygen, "type") == ids.water,
        "ethanol oxidation did not produce acetic acid and water")
end

local function run_acetic_ketonisation()
    configure()
    local first = make(ids.acetic_acid, 120, 120, 700.0)
    local second = make(ids.acetic_acid, 121, 120, 700.0)
    make(ids.catalyst, 120, 121, 700.0)
    sim.updateUpTo()
    assert(sim.partProperty(first, "type") == ids.acetone
            and sim.partProperty(second, "type") == ids.carbon_dioxide
            and count_type(ids.water) == 1,
        "acetic-acid ketonisation did not produce acetone CO2 and water")
end

local function run_benzene_synthesis()
    configure()
    make(ids.catalyst, 120, 120, 800.0)
    make(ids.acetylene, 121, 120, 800.0)
    make(ids.acetylene, 120, 121, 800.0)
    make(ids.acetylene, 119, 120, 800.0)
    sim.updateUpTo()
    assert(count_type(ids.benzene) == 1 and count_type(ids.acetylene) == 0,
        "bounded acetylene cyclisation did not produce one benzene particle")
end

local function run_toluene_synthesis()
    configure()
    local benzene = make(ids.benzene, 120, 120, 550.0)
    local methanol = make(ids.methanol, 121, 120, 550.0)
    make(ids.catalyst, 120, 121, 550.0)
    sim.updateUpTo()
    assert(sim.partProperty(benzene, "type") == ids.toluene
            and sim.partProperty(methanol, "type") == ids.water,
        "benzene methylation did not produce toluene and water")
end

local function run_glycerol_proxy()
    configure()
    local glycerol = make(ids.glycerol, 120, 120, 380.0)
    local acid = make(ids.nitric_acid, 121, 120, 380.0)
    sim.updateUpTo()
    assert(sim.partProperty(glycerol, "type") == ids.nitroglycerin
            and sim.partProperty(acid, "type") == ids.water,
        "bounded game-only glycerol proxy did not produce NITR and water")
end

local function run_urea_synthesis()
    configure()
    sim.airMode(sim.AIR_NOUPDATE)
    sim.pressure(30, 30, 3.0)
    make(ids.catalyst, 120, 120, 500.0)
    local first = make(ids.ammonia, 121, 120, 500.0)
    local second = make(ids.ammonia, 120, 121, 500.0)
    make(ids.carbon_dioxide, 119, 120, 500.0)
    sim.updateUpTo()
    assert(sim.partProperty(first, "type") == ids.urea
            and sim.partProperty(second, "type") == ids.water
            and count_type(ids.carbon_dioxide) == 0,
        "pressurised urea synthesis did not produce urea and water")
end

local function run_urea_hydrolysis()
    configure()
    local urea = make(ids.urea, 120, 120, 380.0)
    local water = make(ids.water, 121, 120, 380.0)
    make(ids.catalyst, 120, 121, 380.0)
    sim.updateUpTo()
    assert(sim.partProperty(urea, "type") == ids.ammonia_water
            and sim.partProperty(water, "type") == ids.carbon_dioxide,
        "urea hydrolysis did not produce ammonia water and carbon dioxide")
end

local function run_saponification()
    configure()
    local fats = make(ids.fats, 120, 120, 380.0)
    local caustic = make(ids.caustic, 121, 120, 380.0)
    sim.updateUpTo()
    assert(sim.partProperty(fats, "type") == ids.soap
            and sim.partProperty(caustic, "type") == ids.glycerol,
        "fat saponification did not produce official soap and glycerol")
end

local function run_phase_path(source_type, product_type, temperature, pressure)
    configure()
    local source = make(source_type, 120, 120, temperature or 300.0)
    if pressure then
        sim.airMode(sim.AIR_NOUPDATE)
        sim.pressure(30, 30, pressure)
    end
    -- Temperature transitions are sampled through HeatConduct in the engine,
    -- so exercise a bounded number of frames while holding the test condition.
    for _ = 1, 64 do
        sim.partProperty(source, "temp", temperature or 300.0)
        if pressure then sim.pressure(30, 30, pressure) end
        sim.updateUpTo()
        if sim.partProperty(source, "type") == product_type then break end
    end
    assert(sim.partProperty(source, "type") == product_type,
        "organic phase path failed for source " .. tostring(source_type)
        .. " expected " .. tostring(product_type)
        .. " got " .. tostring(sim.partProperty(source, "type"))
        .. " temp=" .. tostring(sim.partProperty(source, "temp"))
        .. " high_temp=" .. tostring(elements.property(source_type, "HighTemperature"))
        .. " high_transition=" .. tostring(elements.property(
            source_type, "HighTemperatureTransition")))
end

local function run_phase_paths()
    run_phase_path(ids.propane, ids.oil, 300.0, 10.0)
    run_phase_path(ids.butane, ids.oil, 300.0, 7.0)
    -- Leave enough margin for the engine's first-frame heat exchange while the
    -- registered 329 K threshold itself remains covered by the static audit.
    run_phase_path(ids.acetone, ids.gas, 400.0, nil)
    run_phase_path(ids.urea, ids.ammonia, 420.0, nil)
    run_phase_path(ids.fats, ids.smoke, 650.0, nil)
end

local function run_budget_case()
    configure()
    local sources = {}
    for row = 0, 31 do
        for column = 0, 49 do
            local x = 10 + column * 4
            local y = 10 + row * 4
            sources[#sources + 1] = make(ids.ethanol, x, y, 400.0)
            make(ids.oxygen, x + 1, y, 400.0)
            make(ids.catalyst, x, y + 1, 400.0)
        end
    end
    sim.updateUpTo()
    local reacted = 0
    for _, source in ipairs(sources) do
        if sim.partProperty(source, "type") == ids.acetic_acid then
            reacted = reacted + 1
        end
    end
    local metrics = sim.omniEventMetrics()
    local peak = assert(tonumber(metrics.peak_per_frame), "missing event peak")
    assert(reacted == 1536 and peak == 1536,
        "shared chemistry budget drifted: reacted=" .. reacted
        .. " peak=" .. tostring(peak))
    return peak
end

local function test()
    run_methane_synthesis()
    run_methane_reforming()
    run_cracking("ethane cracking", ids.ethane, ids.hydrogen, 720.0)
    run_cracking("propane cracking", ids.propane, ids.methane, 770.0)
    run_cracking("butane cracking", ids.butane, ids.ethane, 820.0)
    run_polymerisation()
    run_methanol_oxidation()
    run_ethanol_oxidation()
    run_acetic_ketonisation()
    run_benzene_synthesis()
    run_toluene_synthesis()
    run_glycerol_proxy()
    run_urea_synthesis()
    run_urea_hydrolysis()
    run_saponification()
    run_phase_paths()
    return run_budget_case()
end

local ok, data = xpcall(test, debug.traceback)
local report = assert(io.open(RESULT, "w"))
if ok then
    report:write("OMNI_ORGANIC_ELEMENTS=13\n")
    report:write("OMNI_ORGANIC_IDS=" .. ids.methane .. "-" .. ids.fats .. "\n")
    report:write("OMNI_ORGANIC_POLY_REUSED=" .. ids.polyethylene .. "\n")
    report:write("OMNI_ORGANIC_REACTION_PATHS=15\n")
    report:write("OMNI_ORGANIC_PHASE_PATHS=5\n")
    report:write("OMNI_ORGANIC_EVENT_PEAK=", data, "\n")
    report:write("OMNI_ORGANIC_STATUS=PASS\n")
else
    local error_text = tostring(data):gsub("[\r\n]+", " | ")
    report:write("OMNI_ORGANIC_ERROR=" .. error_text .. "\n")
    report:write("OMNI_ORGANIC_STATUS=FAIL\n")
end
report:close()
