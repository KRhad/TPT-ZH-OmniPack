local RESULT = "lua-chemistry-regression.result"

local function must_element(identifier, short_name)
    local id = elements[identifier]
    assert(type(id) == "number", "missing element constant: " .. identifier)
    assert(elements.getByName(short_name) == id,
        "name/identifier mismatch for " .. identifier)
    return id
end

local ids = {
    acid = assert(elements.DEFAULT_PT_ACID),
    catalyst = must_element("OMNI_PT_CATA", "CATA"),
    chlorine = must_element("OMNI_PT_CHLR", "CHLR"),
    ammonia = must_element("OMNI_PT_AMON", "AMON"),
    ethanol = must_element("OMNI_PT_ETHL", "ETHL"),
    kerosene = must_element("OMNI_PT_KERO", "KERO"),
    gasoline = must_element("OMNI_PT_GASO", "GASO"),
    acetylene = must_element("OMNI_PT_ACTY", "ACTY"),
    polymer = must_element("OMNI_PT_POLY", "POLY"),
    peroxide = must_element("OMNI_PT_PERO", "PERO"),
    fertilizer = must_element("OMNI_PT_FERT", "FERT"),
    dust = assert(elements.DEFAULT_PT_DUST),
    hydrogen = assert(elements.DEFAULT_PT_H2),
    nitrogen = assert(elements.DEFAULT_PT_LNTG),
    oxygen = assert(elements.DEFAULT_PT_O2),
    water = assert(elements.DEFAULT_PT_WATR),
    oil = assert(elements.DEFAULT_PT_OIL),
    plant = assert(elements.DEFAULT_PT_PLNT),
    yeast = assert(elements.DEFAULT_PT_YEST),
    co2 = assert(elements.DEFAULT_PT_CO2),
    spark = assert(elements.DEFAULT_PT_SPRK),
}

local function configure_simulation()
    sim.clearSim()
    sim.paused(true)
    sim.gravityMode(sim.GRAV_OFF)
    sim.airMode(sim.AIR_OFF)
    sim.ambientHeatSim(false)
    sim.heatSim(true)
    sim.ensureDeterminism(true)
    sim.randomSeed(11, 12, 13, 14)
end

local function make(type, x, y, temp)
    local particle = sim.partCreate(-1, x, y, type)
    assert(particle >= 0, "failed to create particle type " .. tostring(type))
    if temp then
        sim.partProperty(particle, "temp", temp)
    end
    return particle
end

local function step(frames)
    for _ = 1, frames or 1 do
        sim.updateUpTo()
    end
end

local function run_fuel_chain()
    configure_simulation()
    local catalyst = make(ids.catalyst, 120, 120, 550.0)
    local oil = make(ids.oil, 121, 120, 550.0)
    step()
    assert(sim.partProperty(oil, "type") == ids.kerosene,
        "catalytic oil cracking did not yield kerosene")
    assert(sim.partProperty(catalyst, "type") == ids.catalyst,
        "catalyst was consumed during oil cracking")

    configure_simulation()
    catalyst = make(ids.catalyst, 120, 120, 700.0)
    local kerosene = make(ids.kerosene, 121, 120, 700.0)
    step()
    assert(sim.partProperty(kerosene, "type") == ids.gasoline,
        "catalytic kerosene cracking did not yield gasoline")

    configure_simulation()
    catalyst = make(ids.catalyst, 120, 120, 1200.0)
    kerosene = make(ids.kerosene, 121, 120, 1200.0)
    step()
    assert(sim.partProperty(kerosene, "type") == ids.acetylene,
        "hot catalytic cracking did not yield acetylene")
end

local function run_polymerisation()
    configure_simulation()
    make(ids.catalyst, 120, 120, 600.0)
    local first = make(ids.acetylene, 121, 120, 600.0)
    local second = make(ids.acetylene, 120, 121, 600.0)
    local diffusion = elements.property(ids.acetylene, "Diffusion")
    elements.property(ids.acetylene, "Diffusion", 0.0)
    step()
    elements.property(ids.acetylene, "Diffusion", diffusion)
    assert(sim.partProperty(first, "type") == ids.polymer
        and sim.partProperty(second, "type") == ids.polymer,
        "moderate catalytic polymerisation did not preserve both polymer particles")
end

local function spark_catalyst(x, y, temperature)
    local particle = make(ids.catalyst, x, y, temperature)
    sim.partProperty(particle, "type", ids.spark)
    sim.partProperty(particle, "ctype", ids.catalyst)
    sim.partProperty(particle, "life", 4)
    return particle
end

local function run_electrochemistry()
    configure_simulation()
    spark_catalyst(120, 120, 400.0)
    local first = make(ids.water, 121, 120, 400.0)
    local second = make(ids.water, 120, 121, 400.0)
    local oxygen = make(ids.oxygen, 121, 121, 400.0)
    local diffusion = elements.property(ids.oxygen, "Diffusion")
    elements.property(ids.oxygen, "Diffusion", 0.0)
    step()
    elements.property(ids.oxygen, "Diffusion", diffusion)
    assert(sim.partProperty(first, "type") == ids.peroxide
        and sim.partProperty(second, "type") == ids.peroxide,
        "sparked catalyst and oxygen did not yield peroxide")
    assert(not sim.partExists(oxygen),
        "peroxide route did not consume its oxygen input")

    configure_simulation()
    make(ids.catalyst, 120, 120, 400.0)
    first = make(ids.peroxide, 121, 120, 400.0)
    second = make(ids.peroxide, 120, 121, 400.0)
    diffusion = elements.property(ids.oxygen, "Diffusion")
    elements.property(ids.oxygen, "Diffusion", 0.0)
    step()
    elements.property(ids.oxygen, "Diffusion", diffusion)
    assert(sim.partProperty(first, "type") == ids.water
        and sim.partProperty(second, "type") == ids.water,
        "hot catalyst did not decompose peroxide into water")
    local oxygen_found = false
    for dy = -1, 1 do
        for dx = -1, 1 do
            local particle = sim.partID(120 + dx, 120 + dy)
            if particle and particle >= 0 and sim.partProperty(particle, "type") == ids.oxygen then
                oxygen_found = true
            end
        end
    end
    assert(oxygen_found, "peroxide decomposition did not release oxygen")

    configure_simulation()
    local catalyst_spark = spark_catalyst(120, 120, 700.0)
    local nitrogen = make(ids.nitrogen, 121, 120, 700.0)
    local h1 = make(ids.hydrogen, 120, 121, 700.0)
    local h2 = make(ids.hydrogen, 119, 120, 700.0)
    local h3 = make(ids.hydrogen, 120, 119, 700.0)
    local hdiff = elements.property(ids.hydrogen, "Diffusion")
    elements.property(ids.hydrogen, "Diffusion", 0.0)
    sim.airMode(sim.AIR_NOUPDATE)
    sim.pressure(30, 30, 3.0)
    step()
    elements.property(ids.hydrogen, "Diffusion", hdiff)
    assert(sim.partProperty(nitrogen, "type") == ids.ammonia
        and sim.partProperty(h1, "type") == ids.ammonia
        and sim.partProperty(h2, "type") == ids.ammonia
        and sim.partProperty(h3, "type") == ids.ammonia,
        "pressurized sparked catalyst did not synthesize ammonia")
end

local function run_reaction_network()
    configure_simulation()
    local chlorine = make(ids.chlorine, 120, 120, 500.0)
    local hydrogen = make(ids.hydrogen, 121, 120, 500.0)
    local hdiff = elements.property(ids.hydrogen, "Diffusion")
    elements.property(ids.hydrogen, "Diffusion", 0.0)
    step()
    elements.property(ids.hydrogen, "Diffusion", hdiff)
    assert(sim.partProperty(chlorine, "type") == ids.acid
        and sim.partProperty(hydrogen, "type") == ids.acid,
        "heated chlorine and hydrogen did not form acid")

    configure_simulation()
    local ammonia = make(ids.ammonia, 120, 120, 300.0)
    local acid = make(ids.acid, 121, 120, 300.0)
    step()
    assert(sim.partProperty(ammonia, "type") == ids.fertilizer
        and sim.partProperty(acid, "type") == ids.fertilizer,
        "ammonia acid neutralisation did not make fertilizer")

    configure_simulation()
    local fertilizer = make(ids.fertilizer, 120, 120, 300.0)
    local plant = make(ids.plant, 121, 120, 300.0)
    make(ids.water, 120, 121, 300.0)
    step()
    assert(sim.partProperty(fertilizer, "type") == ids.dust,
        "wet plant did not consume fertilizer")
    local fertilizer_oxygen = false
    for dy = -1, 1 do
        for dx = -1, 1 do
            local particle = sim.partID(121 + dx, 120 + dy)
            if particle and particle >= 0 and sim.partProperty(particle, "type") == ids.oxygen then
                fertilizer_oxygen = true
            end
        end
    end
    assert(fertilizer_oxygen,
        "fertilizer did not trigger the plant oxygen-production path")

    configure_simulation()
    make(ids.yeast, 120, 120, 300.0)
    local plant_feed = make(ids.plant, 121, 120, 300.0)
    local water = make(ids.water, 120, 121, 300.0)
    step()
    assert(sim.partProperty(plant_feed, "type") == ids.ethanol
        and sim.partProperty(water, "type") == ids.co2,
        "warm yeast fermentation did not produce ethanol and carbon dioxide")
end

local function run_negative_control()
    configure_simulation()
    make(ids.catalyst, 120, 120, 300.0)
    local oil = make(ids.oil, 121, 120, 300.0)
    step(3)
    assert(sim.partProperty(oil, "type") == ids.oil,
        "cold catalyst cracked oil without its temperature condition")
end

local function test()
    run_fuel_chain()
    run_polymerisation()
    run_electrochemistry()
    run_reaction_network()
    run_negative_control()
end

local ok, data = xpcall(test, debug.traceback)
local report = assert(io.open(RESULT, "w"))
if ok then
    report:write("OMNI_CHEMISTRY_STATUS=PASS\n")
    report:write("OMNI_CHEMISTRY_PATHS=9\n")
    report:write("OMNI_CHEMISTRY_IDS=" .. ids.chlorine .. "-" .. ids.fertilizer .. "\n")
else
    local error_text = tostring(data):gsub("[\r\n]+", " | ")
    report:write("OMNI_CHEMISTRY_STATUS=FAIL\n")
    report:write("OMNI_CHEMISTRY_ERROR=" .. error_text .. "\n")
end
report:close()
