local RESULT = "lua-biology-regression.result"

local function must_element(identifier, short_name)
    local id = elements[identifier]
    assert(type(id) == "number", "missing element constant: " .. identifier)
    assert(elements.getByName(short_name) == id,
        "name/identifier mismatch for " .. identifier)
    return id
end

local ids = {
    nutrient = must_element("OMNI_PT_NUTR", "NUTR"),
    algae = must_element("OMNI_PT_ALGA", "ALGA"),
    mycelium = must_element("OMNI_PT_MYCL", "MYCL"),
    spore = must_element("OMNI_PT_SPOR", "SPOR"),
    pathogen = must_element("OMNI_PT_PATH", "PATH"),
    sterilant = must_element("OMNI_PT_STER", "STER"),
    humus = must_element("OMNI_PT_HUMS", "HUMS"),
    biofilm = must_element("OMNI_PT_BIOF", "BIOF"),
    peroxide = must_element("OMNI_PT_PERO", "PERO"),
    fertilizer = must_element("OMNI_PT_FERT", "FERT"),
    catalyst = must_element("OMNI_PT_CATA", "CATA"),
    water = assert(elements.DEFAULT_PT_WATR),
    wood = assert(elements.DEFAULT_PT_WOOD),
    co2 = assert(elements.DEFAULT_PT_CO2),
    oxygen = assert(elements.DEFAULT_PT_O2),
    dust = assert(elements.DEFAULT_PT_DUST),
}

local mode_file = assert(io.open("biology-mode.txt", "r"))
local simplified = mode_file:read("*l") == "simplified"
mode_file:close()

local function configure_simulation()
    sim.clearSim()
    sim.paused(true)
    sim.gravityMode(sim.GRAV_OFF)
    sim.airMode(sim.AIR_OFF)
    sim.ambientHeatSim(false)
    sim.heatSim(true)
    sim.ensureDeterminism(true)
    sim.randomSeed(21, 22, 23, 24)
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

local function no_diffusion(type, action)
    local original = elements.property(type, "Diffusion")
    elements.property(type, "Diffusion", 0.0)
    local ok, data = xpcall(action, debug.traceback)
    elements.property(type, "Diffusion", original)
    assert(ok, data)
end

local function run_photosynthesis()
    configure_simulation()
    local algae = make(ids.algae, 120, 120, 300.0)
    local nutrient = make(ids.nutrient, 121, 120, 300.0)
    make(ids.water, 120, 121, 300.0)
    local carbon_dioxide = make(ids.co2, 121, 121, 300.0)
    no_diffusion(ids.co2, function()
        step()
    end)
    assert(sim.partProperty(carbon_dioxide, "type") == ids.oxygen,
        "algae did not convert local carbon dioxide into oxygen")
    assert(sim.partProperty(nutrient, "type") == (simplified and ids.humus or ids.algae),
        "algae did not apply the selected biomass propagation mode")
    assert(sim.partProperty(algae, "type") == ids.algae,
        "algae was consumed during its local photosynthesis path")

    configure_simulation()
    make(ids.algae, 120, 120, 280.0)
    local cold_nutrient = make(ids.nutrient, 121, 120, 280.0)
    make(ids.water, 120, 121, 280.0)
    make(ids.co2, 121, 121, 280.0)
    step(3)
    assert(sim.partProperty(cold_nutrient, "type") == ids.nutrient,
        "cold algae propagated outside its temperature window")
end

local function run_decomposition_and_germination()
    configure_simulation()
    make(ids.mycelium, 120, 120, 300.0)
    local wood = make(ids.wood, 121, 120, 300.0)
    local water = make(ids.water, 120, 121, 300.0)
    step()
    assert(sim.partProperty(wood, "type") == ids.humus
        and sim.partProperty(water, "type") == ids.nutrient,
        "wet mycelium did not decompose wood into humus and nutrient")

    configure_simulation()
    local spore = make(ids.spore, 120, 120, 300.0)
    local nutrient = make(ids.nutrient, 121, 120, 300.0)
    make(ids.water, 120, 121, 300.0)
    step()
    assert(sim.partProperty(spore, "type") == ids.mycelium,
        "spore did not germinate into mycelium")
    assert(sim.partProperty(nutrient, "type") == (simplified and ids.humus or ids.mycelium),
        "spore germination did not apply the selected biomass propagation mode")
end

local function run_pathogen_and_treatment()
    configure_simulation()
    make(ids.pathogen, 120, 120, 300.0)
    local host = make(ids.algae, 121, 120, 300.0)
    step()
    assert(sim.partProperty(host, "type") == (simplified and ids.humus or ids.pathogen),
        "pathogen infection did not apply the selected host conversion mode")

    configure_simulation()
    local sterilant = make(ids.sterilant, 120, 120, 300.0)
    local pathogen = make(ids.pathogen, 121, 120, 300.0)
    step()
    assert(sim.partProperty(sterilant, "type") == ids.humus
        and sim.partProperty(pathogen, "type") == ids.humus,
        "sterilant did not neutralize the adjacent pathogen")

    configure_simulation()
    local biofilm = make(ids.biofilm, 120, 120, 300.0)
    pathogen = make(ids.pathogen, 121, 120, 300.0)
    make(ids.water, 120, 121, 300.0)
    step()
    assert(sim.partProperty(biofilm, "type") == ids.biofilm
        and sim.partProperty(pathogen, "type") == ids.humus,
        "wet biofilm did not filter the adjacent pathogen")
end

local function run_biology_chemistry_recovery()
    configure_simulation()
    local peroxide = make(ids.peroxide, 120, 120, 300.0)
    local pathogen = make(ids.pathogen, 121, 120, 300.0)
    step()
    assert(sim.partProperty(peroxide, "type") == ids.water
        and sim.partProperty(pathogen, "type") == ids.humus,
        "peroxide did not convert pathogen into recoverable humus and water")

    configure_simulation()
    peroxide = make(ids.peroxide, 120, 120, 280.0)
    pathogen = make(ids.pathogen, 121, 120, 280.0)
    step(3)
    assert(sim.partProperty(peroxide, "type") == ids.peroxide
        and sim.partProperty(pathogen, "type") == ids.pathogen,
        "cold peroxide treated pathogen outside the registered temperature window")

    configure_simulation()
    peroxide = make(ids.peroxide, 120, 120, 300.0)
    pathogen = make(ids.pathogen, 121, 120, 300.0)
    local catalyst = make(ids.catalyst, 121, 121, 700.0)
    local hot_catalyst_trace = {}
    local catalyst_stayed_hot = true
    for frame = 1, 3 do
        step()
        catalyst_stayed_hot = catalyst_stayed_hot
            and sim.partProperty(catalyst, "temp") >= 350.0
        hot_catalyst_trace[#hot_catalyst_trace + 1] = string.format(
            "f%d:pero=%d@%.3f,path=%d@%.3f,cata=%d@%.3f",
            frame,
            sim.partProperty(peroxide, "type"), sim.partProperty(peroxide, "temp"),
            sim.partProperty(pathogen, "type"), sim.partProperty(pathogen, "temp"),
            sim.partProperty(catalyst, "type"), sim.partProperty(catalyst, "temp"))
    end
    assert(catalyst_stayed_hot
        and sim.partProperty(peroxide, "type") == ids.peroxide
        and sim.partProperty(pathogen, "type") == ids.pathogen,
        "hot catalyst did not preempt peroxide pathogen treatment; trace="
            .. table.concat(hot_catalyst_trace, ";"))

    configure_simulation()
    local humus = make(ids.humus, 120, 120, 300.0)
    local fertilizer = make(ids.fertilizer, 121, 120, 300.0)
    step(3)
    assert(sim.partProperty(humus, "type") == ids.humus
        and sim.partProperty(fertilizer, "type") == ids.fertilizer,
        "dry humus recovery ran without its water condition")
    local water = make(ids.water, 120, 121, 300.0)
    step()
    assert(sim.partProperty(humus, "type") == ids.nutrient
        and sim.partProperty(fertilizer, "type") == ids.dust
        and sim.partProperty(water, "type") == ids.water,
        "wet humus and fertilizer did not recover nutrient while retaining water")

    configure_simulation()
    humus = make(ids.humus, 120, 120, 280.0)
    fertilizer = make(ids.fertilizer, 121, 120, 280.0)
    water = make(ids.water, 120, 121, 280.0)
    step(3)
    assert(sim.partProperty(humus, "type") == ids.humus
        and sim.partProperty(fertilizer, "type") == ids.fertilizer
        and sim.partProperty(water, "type") == ids.water,
        "cold humus recovery ran outside the composting temperature window")
end

local function test()
    run_photosynthesis()
    run_decomposition_and_germination()
    run_pathogen_and_treatment()
    run_biology_chemistry_recovery()
end

local ok, data = xpcall(test, debug.traceback)
local report = assert(io.open(RESULT, "w"))
if ok then
    report:write("OMNI_BIOLOGY_STATUS=PASS\n")
    report:write("OMNI_BIOLOGY_PATHS=8\n")
    report:write("OMNI_BIOLOGY_MODE=" .. (simplified and "simplified" or "full") .. "\n")
    report:write("OMNI_BIOLOGY_IDS=" .. ids.nutrient .. "-" .. ids.biofilm .. "\n")
else
    local error_text = tostring(data):gsub("[\r\n]+", " | ")
    report:write("OMNI_BIOLOGY_STATUS=FAIL\n")
    report:write("OMNI_BIOLOGY_ERROR=" .. error_text .. "\n")
end
report:close()
