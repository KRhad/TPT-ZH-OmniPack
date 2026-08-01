local RESULT = "lua-environment-regression.result"

local function must_element(identifier, short_name, stable_id)
    local id = assert(elements[identifier], "missing element " .. identifier)
    assert(id == stable_id,
        identifier .. " stable ID changed: expected " .. stable_id .. " got " .. id)
    assert(elements.getByName(short_name) == id,
        "name/identifier mismatch for " .. identifier)
    return id
end

local ids
local initialise_ok, initialise_error = xpcall(function()
ids = {
    soil = must_element("OMNI_PT_SOIL", "SOIL", 670),
    wastewater = must_element("OMNI_PT_WWTR", "WWTR", 671),
    pesticide = must_element("OMNI_PT_PEST", "PEST", 672),
    heavy_metal = must_element("OMNI_PT_HMET", "HMET", 673),
    radioactive = must_element("OMNI_PT_RCON", "RCON", 674),
    microplastic = must_element("OMNI_PT_MPLS", "MPLS", 675),
    organic_waste = must_element("OMNI_PT_OWST", "OWST", 676),
    bloom = must_element("OMNI_PT_BLOM", "BLOM", 677),
    mold = must_element("OMNI_PT_MOLD", "MOLD", 678),
    blood = must_element("OMNI_PT_BLOD", "BLOD", 679),
    toxin = must_element("OMNI_PT_TOXN", "TOXN", 680),
    antimicrobial = must_element("OMNI_PT_AMAT", "AMAT", 681),
    sludge = must_element("OMNI_PT_SLUD", "SLUD", 682),
    smog = must_element("OMNI_PT_SMOG", "SMOG", 683),
    acid_rain = must_element("OMNI_PT_ARAN", "ARAN", 684),
    detergent = must_element("OMNI_PT_DETG", "DETG", 685),

    nutrient = assert(elements.OMNI_PT_NUTR),
    algae = assert(elements.OMNI_PT_ALGA),
    mycelium = assert(elements.OMNI_PT_MYCL),
    pathogen = assert(elements.OMNI_PT_PATH),
    humus = assert(elements.OMNI_PT_HUMS),
    biofilm = assert(elements.OMNI_PT_BIOF),
    cellulose = assert(elements.OMNI_PT_CELU),
    calcium_hydroxide = assert(elements.OMNI_PT_CAOH),
    water = assert(elements.DEFAULT_PT_WATR),
    distilled = assert(elements.DEFAULT_PT_DSTW),
    vapour = assert(elements.DEFAULT_PT_WTRV),
    oxygen = assert(elements.DEFAULT_PT_O2),
    carbon_dioxide = assert(elements.DEFAULT_PT_CO2),
    oil = assert(elements.DEFAULT_PT_OIL),
    photon = assert(elements.DEFAULT_PT_PHOT),
}
end, debug.traceback)

if not initialise_ok then
    local report = assert(io.open(RESULT, "w"))
    report:write("OMNI_ENVIRONMENT_ERROR="
        .. tostring(initialise_error):gsub("[\r\n]+", " | ") .. "\n")
    report:write("OMNI_ENVIRONMENT_STATUS=FAIL\n")
    report:close()
    os.exit(1)
end

local function configure()
    sim.clearSim()
    sim.paused(true)
    sim.gravityMode(sim.GRAV_OFF)
    sim.airMode(sim.AIR_OFF)
    sim.ambientHeatSim(false)
    sim.heatSim(true)
    sim.ensureDeterminism(true)
    sim.randomSeed(79, 83, 89, 97)
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

local function expect_type(particle, expected, message)
    assert(sim.partExists(particle), message .. " particle disappeared")
    local actual = sim.partProperty(particle, "type")
    assert(actual == expected,
        message .. " expected=" .. expected .. " actual=" .. tostring(actual))
end

local function run_soil_water_cycle()
    configure()
    local soil = make(ids.soil, 120, 120, 300.0)
    local water = make(ids.water, 121, 120, 300.0)
    sim.updateUpTo()
    expect_type(soil, ids.soil, "soil absorption")
    assert(not sim.partExists(water) and sim.partProperty(soil, "tmp") == 1,
        "soil did not store one fresh-water particle")

    sim.partProperty(soil, "temp", 400.0)
    sim.updateUpTo()
    assert(sim.partProperty(soil, "tmp") == 0 and count_type(ids.vapour) == 1,
        "hot soil did not release its stored water as one vapour particle")
end

local function run_water_treatment_and_bloom()
    configure()
    local wastewater = make(ids.wastewater, 120, 120, 300.0)
    local biofilm = make(ids.biofilm, 121, 120, 300.0)
    sim.updateUpTo()
    expect_type(wastewater, ids.distilled, "wastewater filtration")
    assert(sim.partProperty(biofilm, "tmp") == 1,
        "biofilm did not record one filtration load")

    configure()
    make(ids.wastewater, 120, 120, 300.0)
    make(ids.algae, 121, 120, 300.0)
    make(ids.nutrient, 120, 121, 300.0)
    sim.updateUpTo()
    assert(count_type(ids.bloom) == 2 and count_type(ids.humus) == 1,
        "wastewater eutrophication did not produce two blooms and humus")
end

local function run_pollutants()
    configure()
    local pesticide = make(ids.pesticide, 120, 120, 300.0)
    local pathogen = make(ids.pathogen, 121, 120, 300.0)
    sim.updateUpTo()
    expect_type(pesticide, ids.toxin, "pesticide residue")
    expect_type(pathogen, ids.humus, "pesticide treatment")

    configure()
    make(ids.heavy_metal, 120, 120, 300.0)
    local water = make(ids.water, 121, 120, 300.0)
    sim.updateUpTo()
    expect_type(water, ids.wastewater, "heavy-metal contamination")

    configure()
    local radioactive = make(ids.radioactive, 120, 120, 300.0)
    sim.partProperty(radioactive, "life", 1)
    sim.updateUpTo()
    expect_type(radioactive, ids.heavy_metal, "radioactive decay")
    assert(count_type(ids.photon) == 1,
        "radioactive contaminant did not emit one bounded photon")

    configure()
    local microplastic = make(ids.microplastic, 120, 120, 300.0)
    make(ids.biofilm, 121, 120, 300.0)
    sim.updateUpTo()
    expect_type(microplastic, ids.sludge, "microplastic capture")
end

local function run_biological_materials()
    configure()
    local waste = make(ids.organic_waste, 120, 120, 300.0)
    make(ids.mycelium, 121, 120, 300.0)
    local water = make(ids.water, 120, 121, 300.0)
    sim.updateUpTo()
    expect_type(waste, ids.humus, "organic-waste composting")
    expect_type(water, ids.nutrient, "organic-waste nutrient recovery")

    configure()
    local bloom = make(ids.bloom, 120, 120, 300.0)
    sim.partProperty(bloom, "tmp", 3)
    local oxygen = make(ids.oxygen, 121, 120, 300.0)
    sim.updateUpTo()
    expect_type(bloom, ids.sludge, "bloom collapse")
    expect_type(oxygen, ids.carbon_dioxide, "bloom oxygen depletion")

    configure()
    make(ids.mold, 120, 120, 300.0)
    local feed = make(ids.organic_waste, 121, 120, 300.0)
    make(ids.water, 120, 121, 300.0)
    sim.updateUpTo()
    expect_type(feed, ids.mold, "feed-limited mould growth")
    assert(count_type(ids.mold) == 2, "mould growth changed particle count incorrectly")

    configure()
    local blood = make(ids.blood, 120, 120, 300.0)
    make(ids.pathogen, 121, 120, 300.0)
    sim.updateUpTo()
    expect_type(blood, ids.toxin, "pathogen-exposed blood")

    configure()
    local toxin = make(ids.toxin, 120, 120, 300.0)
    local algae = make(ids.algae, 121, 120, 300.0)
    sim.updateUpTo()
    expect_type(toxin, ids.wastewater, "spent toxin")
    expect_type(algae, ids.humus, "toxin biological damage")

    configure()
    local material = make(ids.antimicrobial, 120, 120, 300.0)
    local pathogen = make(ids.pathogen, 121, 120, 300.0)
    sim.updateUpTo()
    expect_type(material, ids.antimicrobial, "antimicrobial surface")
    expect_type(pathogen, ids.humus, "antimicrobial treatment")
    assert(sim.partProperty(material, "tmp") == 11,
        "antimicrobial surface did not consume exactly one charge")
end

local function run_recovery_and_air_pollution()
    configure()
    local sludge = make(ids.sludge, 120, 120, 400.0)
    sim.updateUpTo()
    expect_type(sludge, ids.soil, "sludge drying")
    assert(count_type(ids.vapour) == 1,
        "sludge drying did not release one water-vapour particle")

    configure()
    local smog = make(ids.smog, 120, 120, 300.0)
    local vapour = make(ids.vapour, 121, 120, 300.0)
    sim.updateUpTo()
    expect_type(smog, ids.acid_rain, "smog condensation")
    expect_type(vapour, ids.acid_rain, "vapour condensation")

    configure()
    local acid_rain = make(ids.acid_rain, 120, 120, 300.0)
    local lime = make(ids.calcium_hydroxide, 121, 120, 300.0)
    sim.updateUpTo()
    expect_type(acid_rain, ids.distilled, "acid-rain neutralisation")
    expect_type(lime, ids.sludge, "acid-rain neutralisation residue")

    configure()
    local detergent = make(ids.detergent, 120, 120, 300.0)
    local oil = make(ids.oil, 121, 120, 300.0)
    local water = make(ids.water, 120, 121, 300.0)
    sim.updateUpTo()
    expect_type(detergent, ids.wastewater, "detergent consumption")
    expect_type(oil, ids.wastewater, "oil washing")
    expect_type(water, ids.wastewater, "wash-water pollution")
end

local function run_material_differences()
    assert(elements.property(ids.microplastic, "Weight")
            < elements.property(ids.soil, "Weight"),
        "microplastic and soil density ordering collapsed")
    assert(elements.property(ids.smog, "Properties") ~= 0
            and elements.property(ids.smog, "Diffusion") > 0,
        "smog no longer behaves as a diffusing material")
    assert(elements.property(ids.antimicrobial, "Hardness")
            > elements.property(ids.sludge, "Hardness"),
        "solid antimicrobial material no longer differs from sludge")
end

local function run_budget_case()
    configure()
    local pesticides = {}
    for row = 0, 21 do
        for column = 0, 49 do
            local x = 10 + column * 5
            local y = 10 + row * 5
            pesticides[#pesticides + 1] = make(ids.pesticide, x, y, 300.0)
            make(ids.pathogen, x + 1, y, 300.0)
        end
    end
    sim.updateUpTo()
    local reacted = 0
    for _, particle in ipairs(pesticides) do
        if sim.partProperty(particle, "type") == ids.toxin then reacted = reacted + 1 end
    end
    local metrics = sim.omniEventMetrics()
    local peak = assert(tonumber(metrics.peak_per_frame), "missing event peak")
    assert(reacted == 1024 and peak == 1024,
        "shared ecology event limit changed: reacted=" .. reacted
        .. " peak=" .. tostring(peak))
    return reacted, peak
end

local function test()
    run_soil_water_cycle()
    run_water_treatment_and_bloom()
    run_pollutants()
    run_biological_materials()
    run_recovery_and_air_pollution()
    run_material_differences()
    return run_budget_case()
end

local ok, reacted, peak = xpcall(test, debug.traceback)
local report = assert(io.open(RESULT, "w"))
if ok then
    report:write("OMNI_ENVIRONMENT_ELEMENTS=16\n")
    report:write("OMNI_ENVIRONMENT_IDS=670-685\n")
    report:write("OMNI_ENVIRONMENT_BEHAVIOURS=17\n")
    report:write("OMNI_ENVIRONMENT_EVENT_REACTED=", reacted, "\n")
    report:write("OMNI_ENVIRONMENT_EVENT_PEAK=", peak, "\n")
    report:write("OMNI_ENVIRONMENT_STATUS=PASS\n")
else
    report:write("OMNI_ENVIRONMENT_ERROR="
        .. tostring(reacted):gsub("[\r\n]+", " | ") .. "\n")
    report:write("OMNI_ENVIRONMENT_STATUS=FAIL\n")
end
report:close()
