local RESULT = "lua-organic-batch2-regression.result"

local function must_element(identifier, short_name)
    local id = elements[identifier]
    assert(type(id) == "number", "missing element constant: " .. identifier)
    assert(elements.getByName(short_name) == id,
        "name/identifier mismatch for " .. identifier)
    return id
end

local ids = {
    water = assert(elements.DEFAULT_PT_WATR),
    carbon_dioxide = assert(elements.DEFAULT_PT_CO2),
    oil = assert(elements.DEFAULT_PT_OIL),
    gas = assert(elements.DEFAULT_PT_GAS),
    smoke = assert(elements.DEFAULT_PT_SMKE),
    chlorine = must_element("OMNI_PT_CHLR", "CHLR"),
    catalyst = must_element("OMNI_PT_CATA", "CATA"),
    ethanol = must_element("OMNI_PT_ETHL", "ETHL"),
    polyethylene = must_element("OMNI_PT_POLY", "POLY"),
    yeast = assert(elements.DEFAULT_PT_YEST),
    molten_wax = assert(elements.DEFAULT_PT_MWAX),
    paste = assert(elements.DEFAULT_PT_PSTE),
    glucose = must_element("OMNI_PT_GLUC", "GLUC"),
    starch = must_element("OMNI_PT_STRC", "STRC"),
    cellulose = must_element("OMNI_PT_CELU", "CELU"),
    propylene = must_element("OMNI_PT_PRPE", "C3H6"),
    butadiene = must_element("OMNI_PT_BDIE", "C4H6"),
    vinyl_chloride = must_element("OMNI_PT_VCHL", "VCM"),
    styrene = must_element("OMNI_PT_STYR", "STYR"),
    tetrafluoroethylene = must_element("OMNI_PT_TFET", "C2F4"),
    adipic_acid = must_element("OMNI_PT_ADIP", "ADIP"),
    diamine = must_element("OMNI_PT_DIAM", "DIAM"),
    epoxy_resin = must_element("OMNI_PT_ERES", "ERES"),
    polypropylene = must_element("OMNI_PT_PPLY", "PPLY"),
    pvc = must_element("OMNI_PT_PVCL", "PVC"),
    polystyrene = must_element("OMNI_PT_PSTY", "PSTY"),
    nylon = must_element("OMNI_PT_NYLN", "NYLN"),
    rubber = must_element("OMNI_PT_RUBR", "RUBR"),
    epoxy = must_element("OMNI_PT_EPXY", "EPXY"),
    ptfe = must_element("OMNI_PT_PTFE", "PTFE"),
    bitumen = must_element("OMNI_PT_BITM", "BITM"),
    ethyl_acetate = must_element("OMNI_PT_EACT", "EACT"),
    acetic_acid = must_element("OMNI_PT_ACTA", "ACTA"),
}

assert(ids.glucose == 602 and ids.ethyl_acetate == 621,
    "organic batch 2 stable range changed: expected 602..621")
assert(ids.polyethylene == 367,
    "canonical polyethylene stable ID changed")
assert(ids.bitumen ~= ids.oil and ids.cellulose ~= assert(elements.DEFAULT_PT_WOOD),
    "precise second-batch materials silently reused generic official elements")

local function configure()
    sim.clearSim()
    sim.paused(true)
    sim.gravityMode(sim.GRAV_OFF)
    sim.airMode(sim.AIR_OFF)
    sim.ambientHeatSim(false)
    sim.heatSim(true)
    sim.ensureDeterminism(true)
    sim.randomSeed(43, 47, 53, 59)
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

local function run_hydrolysis(name, source_type, temperature)
    configure()
    local source = make(source_type, 120, 120, temperature)
    local water = make(ids.water, 121, 120, temperature)
    make(ids.catalyst, 120, 121, temperature)
    sim.updateUpTo()
    assert(sim.partProperty(source, "type") == ids.glucose
            and sim.partProperty(water, "type") == ids.water,
        name .. " did not produce glucose while retaining water")
end

local function run_fermentation()
    configure()
    make(ids.yeast, 120, 120, 300.0)
    local glucose = make(ids.glucose, 121, 120, 300.0)
    local water = make(ids.water, 120, 121, 300.0)
    sim.updateUpTo()
    assert(sim.partProperty(glucose, "type") == ids.ethanol
            and sim.partProperty(water, "type") == ids.carbon_dioxide,
        "official yeast did not prefer glucose fermentation")
end

local function run_pair(name, source_type, product_type, temperature)
    configure()
    local first = make(source_type, 120, 120, temperature)
    local second = make(source_type, 121, 120, temperature)
    make(ids.catalyst, 120, 121, temperature)
    sim.updateUpTo()
    assert(sim.partProperty(first, "type") == product_type
            and sim.partProperty(second, "type") == product_type,
        name .. " did not preserve two bounded polymer particles")
end

local function run_nylon_condensation()
    configure()
    local acid = make(ids.adipic_acid, 120, 120, 500.0)
    local diamine = make(ids.diamine, 121, 120, 500.0)
    make(ids.catalyst, 120, 121, 500.0)
    sim.updateUpTo()
    assert(sim.partProperty(acid, "type") == ids.nylon
            and sim.partProperty(diamine, "type") == ids.water,
        "bounded nylon condensation did not produce nylon and water")
end

local function run_esterification()
    configure()
    local acid = make(ids.acetic_acid, 120, 120, 380.0)
    local ethanol = make(ids.ethanol, 121, 120, 380.0)
    make(ids.catalyst, 120, 121, 380.0)
    sim.updateUpTo()
    assert(sim.partProperty(acid, "type") == ids.ethyl_acetate
            and sim.partProperty(ethanol, "type") == ids.water,
        "bounded esterification did not produce ethyl acetate and water")
end

local function run_bitumen_residue()
    configure()
    make(ids.catalyst, 120, 120, 700.0)
    local first = make(ids.oil, 121, 120, 700.0)
    local second = make(ids.oil, 120, 121, 700.0)
    sim.updateUpTo()
    assert(sim.partProperty(first, "type") == ids.bitumen
            and sim.partProperty(second, "type") == ids.gas,
        "two-oil bounded residue route did not produce bitumen and gas")
end

local function run_pvc_decomposition()
    configure()
    local pvc = make(ids.pvc, 120, 120, 700.0)
    sim.updateUpTo()
    assert(sim.partProperty(pvc, "type") == ids.smoke
            and count_type(ids.chlorine) == 1,
        "hot PVC did not produce one bounded chlorine particle and smoke")
end

local function run_material_differences()
    assert(elements.property(ids.polypropylene, "HighTemperature")
            < elements.property(ids.polyethylene, "HighTemperature"),
        "polypropylene and polyethylene softening profiles collapsed")
    assert(elements.property(ids.polystyrene, "Hardness")
            < elements.property(ids.nylon, "Hardness")
            and elements.property(ids.nylon, "Hardness")
                < elements.property(ids.epoxy, "Hardness"),
        "polymer hardness ordering collapsed")
    assert(elements.property(ids.ptfe, "Flammable") == 0
            and elements.property(ids.ptfe, "HighTemperature")
                > elements.property(ids.epoxy, "HighTemperature"),
        "PTFE high-temperature inert profile drifted")
    assert(elements.property(ids.bitumen, "HighTemperatureTransition") == ids.oil,
        "bitumen no longer softens into canonical official OIL")
    assert(elements.property(ids.rubber, "HighPressureTransition") == ids.paste,
        "rubber pressure-deformation proxy drifted")
end

local function run_budget_case()
    configure()
    local sources = {}
    for row = 0, 31 do
        for column = 0, 49 do
            local x = 10 + column * 4
            local y = 10 + row * 4
            sources[#sources + 1] = make(ids.propylene, x, y, 500.0)
            sources[#sources + 1] = make(ids.propylene, x + 1, y, 500.0)
            make(ids.catalyst, x, y + 1, 500.0)
        end
    end
    sim.updateUpTo()
    local reacted = 0
    for _, source in ipairs(sources) do
        if sim.partProperty(source, "type") == ids.polypropylene then
            reacted = reacted + 1
        end
    end
    local metrics = sim.omniEventMetrics()
    local peak = assert(tonumber(metrics.peak_per_frame), "missing event peak")
    assert(reacted == 3072 and peak == 1536,
        "shared chemistry budget drifted: reacted=" .. reacted
        .. " peak=" .. tostring(peak))
    return reacted, peak
end

local function test()
    run_hydrolysis("starch hydrolysis", ids.starch, 360.0)
    run_hydrolysis("cellulose hydrolysis", ids.cellulose, 400.0)
    run_fermentation()
    run_pair("propylene polymerisation", ids.propylene, ids.polypropylene, 500.0)
    run_pair("butadiene polymerisation", ids.butadiene, ids.rubber, 500.0)
    run_pair("vinyl chloride polymerisation", ids.vinyl_chloride, ids.pvc, 500.0)
    run_pair("styrene polymerisation", ids.styrene, ids.polystyrene, 390.0)
    run_pair("TFE polymerisation", ids.tetrafluoroethylene, ids.ptfe, 500.0)
    run_pair("epoxy curing", ids.epoxy_resin, ids.epoxy, 380.0)
    run_nylon_condensation()
    run_esterification()
    run_bitumen_residue()
    run_pvc_decomposition()
    run_material_differences()
    return run_budget_case()
end

local ok, reacted, peak = xpcall(test, debug.traceback)
local report = assert(io.open(RESULT, "w"))
if ok then
    report:write("OMNI_ORGANIC_BATCH2_ELEMENTS=20\n")
    report:write("OMNI_ORGANIC_BATCH2_IDS=602-621\n")
    report:write("OMNI_ORGANIC_BATCH2_REACTION_PATHS=13\n")
    report:write("OMNI_ORGANIC_BATCH2_POLYMER_PATHS=6\n")
    report:write("OMNI_ORGANIC_BATCH2_OFFICIAL_MERGES=6\n")
    report:write("OMNI_ORGANIC_BATCH2_EVENT_REACTED=", reacted, "\n")
    report:write("OMNI_ORGANIC_BATCH2_EVENT_PEAK=", peak, "\n")
    report:write("OMNI_ORGANIC_BATCH2_STATUS=PASS\n")
else
    local error_text = tostring(reacted):gsub("[\r\n]+", " | ")
    report:write("OMNI_ORGANIC_BATCH2_ERROR=" .. error_text .. "\n")
    report:write("OMNI_ORGANIC_BATCH2_STATUS=FAIL\n")
end
report:close()
