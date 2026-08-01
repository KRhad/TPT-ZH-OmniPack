local RESULT = "lua-materials-regression.result"

local function must_element(identifier, short_name)
    local id = elements[identifier]
    assert(type(id) == "number", "missing element constant: " .. identifier)
    assert(elements.getByName(short_name) == id,
        "name/identifier mismatch for " .. identifier)
    return id
end

local ids = {
    lava = assert(elements.DEFAULT_PT_LAVA),
    spark = assert(elements.DEFAULT_PT_SPRK),
    water = assert(elements.DEFAULT_PT_WATR),
    distilled = assert(elements.DEFAULT_PT_DSTW),
    water_vapour = assert(elements.DEFAULT_PT_WTRV),
    carbon_dioxide = assert(elements.DEFAULT_PT_CO2),
    glass = assert(elements.DEFAULT_PT_GLAS),
    broken_glass = assert(elements.DEFAULT_PT_BGLA),
    quartz = assert(elements.DEFAULT_PT_QRTZ),
    brick = assert(elements.DEFAULT_PT_BRCK),
    concrete = assert(elements.DEFAULT_PT_CNCT),
    stone = assert(elements.DEFAULT_PT_STNE),
    boron = must_element("OMNI_PT_B", "B"),
    sodium_hydroxide = must_element("OMNI_PT_NAOH", "NAOH"),
    carbon_monoxide = must_element("OMNI_PT_COMO", "COMO"),
    sulfur_dioxide = must_element("OMNI_PT_SODI", "SODI"),
    peroxide = must_element("OMNI_PT_PERO", "PERO"),
    uranium_oxide = must_element("OMNI_PT_UROX", "UROX"),
    alumina = must_element("OMNI_PT_ALOX", "ALOX"),
    copper = must_element("OMNI_PT_COPR", "COPR"),
    zinc = must_element("OMNI_PT_ZINC", "ZINC"),
    lead = must_element("OMNI_PT_LEAD", "LEAD"),
    coke = must_element("OMNI_PT_COKE", "COKE"),
    flux = must_element("OMNI_PT_FLUX", "FLUX"),
    slag = must_element("OMNI_PT_SLAG", "SLAG"),
    nichrome = must_element("OMNI_PT_NCRM", "NCRM"),
    gypsum = must_element("OMNI_PT_GYPS", "GYPS"),
    bauxite = must_element("OMNI_PT_BAUX", "BAUX"),
    copper_ore = must_element("OMNI_PT_CUOR", "CUOR"),
    zinc_ore = must_element("OMNI_PT_ZNOR", "ZNOR"),
    lead_ore = must_element("OMNI_PT_PBOR", "PBOR"),
    uranium_ore = must_element("OMNI_PT_UORE", "UORE"),
    feldspar = must_element("OMNI_PT_FELD", "FELD"),
    cement = must_element("OMNI_PT_CEMT", "CEMT"),
    alumina_ceramic = must_element("OMNI_PT_ALCR", "ALCR"),
    borosilicate = must_element("OMNI_PT_BSGL", "BSGL"),
    quartz_glass = must_element("OMNI_PT_QGLS", "QGLS"),
    refractory_brick = must_element("OMNI_PT_RFBK", "RFBK"),
}

assert(ids.gypsum == 521 and ids.refractory_brick == 532,
    "materials stable range changed: expected 521..532")

local function configure_simulation()
    sim.clearSim()
    sim.paused(true)
    sim.gravityMode(sim.GRAV_OFF)
    sim.airMode(sim.AIR_OFF)
    sim.ambientHeatSim(false)
    sim.heatSim(true)
    sim.ensureDeterminism(true)
    sim.randomSeed(7, 11, 13, 17)
    sim.resetOmniEventMetrics()
end

local function make(element, x, y, temperature)
    local particle = sim.partCreate(-1, x, y, element)
    assert(particle >= 0, "failed to create element " .. tostring(element))
    if temperature then sim.partProperty(particle, "temp", temperature) end
    return particle
end

local function product_matches(particle, expected)
    local actual = sim.partProperty(particle, "type")
    return actual == expected
        or (actual == ids.lava
            and sim.partProperty(particle, "ctype") == expected)
end

local function count_type(element)
    local count = 0
    for particle in sim.parts() do
        if sim.partProperty(particle, "type") == element then count = count + 1 end
    end
    return count
end

local function run_gypsum_cycle()
    configure_simulation()
    local gypsum = make(ids.gypsum, 120, 120, 500.0)
    sim.updateUpTo()
    assert(sim.partProperty(gypsum, "tmp") == 1
            and count_type(ids.water_vapour) == 1,
        "hot gypsum did not release one bounded steam particle")
    sim.partProperty(gypsum, "temp", 330.0)
    local water = make(ids.water, 121, 120, 300.0)
    sim.updateUpTo()
    assert(sim.partProperty(gypsum, "tmp") == 0 and not sim.partExists(water),
        "cooled dehydrated gypsum did not consume water and rehydrate")
end

local function run_ore_case(name, ore_type, reducer_type, product, byproduct, temp)
    configure_simulation()
    local ore = make(ore_type, 120, 120, temp)
    local reducer = make(reducer_type, 121, 120, temp)
    sim.updateUpTo()
    assert(product_matches(ore, product), name .. " did not produce its metal or oxide")
    assert(product_matches(reducer, byproduct), name .. " did not produce its bounded byproduct")
end

local function powered_nichrome(x, y, temperature)
    local heater = make(ids.nichrome, x, y, temperature)
    sim.partProperty(heater, "type", ids.spark)
    sim.partProperty(heater, "ctype", ids.nichrome)
    sim.partProperty(heater, "life", 4)
    sim.partProperty(heater, "temp", temperature)
    return heater
end

local function run_processing_paths()
    run_ore_case("bauxite", ids.bauxite, ids.sodium_hydroxide,
        ids.alumina, ids.slag, 700.0)
    run_ore_case("copper ore", ids.copper_ore, ids.carbon_monoxide,
        ids.copper, ids.carbon_dioxide, 1100.0)
    run_ore_case("zinc ore", ids.zinc_ore, ids.carbon_monoxide,
        ids.zinc, ids.carbon_dioxide, 1250.0)
    run_ore_case("lead ore", ids.lead_ore, ids.coke,
        ids.lead, ids.sulfur_dioxide, 1150.0)
    run_ore_case("uranium ore", ids.uranium_ore, ids.peroxide,
        ids.uranium_oxide, ids.water, 500.0)

    configure_simulation()
    local feldspar = make(ids.feldspar, 120, 120, 1500.0)
    local flux = make(ids.flux, 121, 120, 1500.0)
    sim.updateUpTo()
    assert(sim.partProperty(feldspar, "type") == ids.lava
            and sim.partProperty(feldspar, "ctype") == ids.glass
            and product_matches(flux, ids.slag),
        "feldspar and flux did not produce typed molten glass and slag")

    configure_simulation()
    local cement = make(ids.cement, 120, 120, 300.0)
    local water = make(ids.water, 121, 120, 300.0)
    sim.updateUpTo()
    assert(sim.partProperty(cement, "tmp") == 1 and not sim.partExists(water),
        "cement did not enter its bounded hydration timer")
    for _ = 1, 100 do sim.updateUpTo() end
    assert(sim.partProperty(cement, "type") == ids.concrete,
        "hydrated cement did not cure into official concrete")

    configure_simulation()
    powered_nichrome(120, 120, 1800.0)
    local alumina = make(ids.alumina, 121, 120, 1800.0)
    sim.updateUpTo()
    assert(sim.partProperty(alumina, "type") == ids.alumina_ceramic,
        "powered nichrome did not sinter hot alumina")

    configure_simulation()
    powered_nichrome(120, 120, 1450.0)
    local brick = make(ids.brick, 121, 120, 1450.0)
    alumina = make(ids.alumina, 120, 121, 1450.0)
    sim.updateUpTo()
    assert(sim.partProperty(brick, "type") == ids.refractory_brick
            and sim.partProperty(alumina, "type") == ids.refractory_brick,
        "brick and alumina did not fire into two refractory bricks")

    configure_simulation()
    powered_nichrome(120, 120, 2000.0)
    local molten_glass = make(ids.lava, 121, 120, 2000.0)
    sim.partProperty(molten_glass, "ctype", ids.glass)
    local boron = make(ids.boron, 120, 121, 2000.0)
    sim.updateUpTo()
    assert(sim.partProperty(molten_glass, "type") == ids.lava
            and sim.partProperty(molten_glass, "ctype") == ids.borosilicate
            and sim.partProperty(boron, "type") == ids.lava
            and sim.partProperty(boron, "ctype") == ids.borosilicate,
        "molten glass and boron did not form typed borosilicate melt")
end

local function run_quench_and_pressure_paths()
    configure_simulation()
    sim.heatSim(false)
    local quenched = false
    local quartz_melt = -1
    for _ = 1, 96 do
        quartz_melt = make(ids.lava, 120, 120, 2800.0)
        sim.partProperty(quartz_melt, "ctype", ids.quartz)
        local water = make(ids.water, 121, 120, 300.0)
        sim.updateUpTo()
        if sim.partProperty(quartz_melt, "ctype") == ids.quartz_glass then
            quenched = sim.partExists(water)
                and sim.partProperty(water, "type") == ids.water_vapour
            break
        end
        if sim.partExists(water) then sim.partKill(water) end
        if sim.partExists(quartz_melt) then sim.partKill(quartz_melt) end
    end
    local quench_metrics = sim.omniEventMetrics()
    assert(quenched,
        "molten quartz did not complete its bounded water quench; type="
        .. tostring(sim.partProperty(quartz_melt, "type"))
        .. " ctype=" .. tostring(sim.partProperty(quartz_melt, "ctype"))
        .. " temp=" .. tostring(sim.partProperty(quartz_melt, "temp"))
        .. " quartz_melt=" .. tostring(elements.property(
            ids.quartz, "HighTemperature"))
        .. " events=" .. tostring(quench_metrics.total))

    configure_simulation()
    local pane = make(ids.borosilicate, 120, 120, 300.0)
    sim.airMode(sim.AIR_NOUPDATE)
    sim.updateUpTo()
    sim.pressure(30, 30, 10.0)
    sim.updateUpTo()
    assert(sim.partProperty(pane, "type") == ids.broken_glass,
        "borosilicate glass did not break under a large pressure delta")

    configure_simulation()
    local refractory = make(ids.refractory_brick, 120, 120, 1600.0)
    local cracked = false
    for _ = 1, 160 do
        sim.partProperty(refractory, "temp", 1600.0)
        local water = make(ids.water, 121, 120, 300.0)
        sim.updateUpTo()
        if sim.partProperty(refractory, "type") == ids.stone then
            cracked = sim.partExists(water)
                and sim.partProperty(water, "type") == ids.water_vapour
            break
        end
        if sim.partExists(water) then sim.partKill(water) end
    end
    assert(cracked, "extreme refractory-brick quench did not create stone and steam")
end

local function run_budget_case()
    configure_simulation()
    local particles = {}
    for row = 0, 39 do
        for column = 0, 39 do
            local particle = make(
                ids.gypsum, 20 + column * 3, 20 + row * 3, 500.0)
            particles[#particles + 1] = particle
        end
    end
    sim.updateUpTo()
    local dehydrated = 0
    for _, particle in ipairs(particles) do
        if sim.partProperty(particle, "tmp") == 1 then dehydrated = dehydrated + 1 end
    end
    local metrics = sim.omniEventMetrics()
    local peak = assert(tonumber(metrics.peak_per_frame), "missing event peak")
    assert(dehydrated == 1536 and peak == 1536,
        "materials event budget drifted: dehydrated=" .. dehydrated
        .. " peak=" .. tostring(peak))
    return peak
end

local function test()
    run_gypsum_cycle()
    run_processing_paths()
    run_quench_and_pressure_paths()
    return run_budget_case()
end

local ok, data = xpcall(test, debug.traceback)
local report = assert(io.open(RESULT, "w"))
if ok then
    report:write("OMNI_MATERIALS_STATUS=PASS\n")
    report:write("OMNI_MATERIALS_ELEMENTS=12\n")
    report:write("OMNI_MATERIALS_IDS=" .. ids.gypsum .. "-"
        .. ids.refractory_brick .. "\n")
    report:write("OMNI_MATERIALS_PATHS=12\n")
    report:write("OMNI_MATERIALS_EVENT_PEAK=" .. data .. "\n")
else
    local error_text = tostring(data):gsub("[\r\n]+", " | ")
    report:write("OMNI_MATERIALS_STATUS=FAIL\n")
    report:write("OMNI_MATERIALS_ERROR=" .. error_text .. "\n")
end
report:close()
