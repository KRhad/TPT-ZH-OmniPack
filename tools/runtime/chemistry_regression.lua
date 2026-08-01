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
    hydrochloric = must_element("OMNI_PT_HCLA", "HCLA"),
    sulfuric = must_element("OMNI_PT_SULA", "SULA"),
    nitric = must_element("OMNI_PT_NITA", "NITA"),
    phosphoric = must_element("OMNI_PT_PHOA", "PHOA"),
    hydrofluoric = must_element("OMNI_PT_HYFA", "HYFA"),
    sodium_hydroxide = must_element("OMNI_PT_NAOH", "NAOH"),
    potassium_hydroxide = must_element("OMNI_PT_KOH", "KOH"),
    calcium_hydroxide = must_element("OMNI_PT_CAOH", "CAOH"),
    potassium_nitrate = must_element("OMNI_PT_KNIT", "KNIT"),
    copper_sulfate = must_element("OMNI_PT_CUSF", "CUSF"),
    calcium_carbonate = must_element("OMNI_PT_CACO", "CACO"),
    sodium_bicarbonate = must_element("OMNI_PT_NABC", "NABC"),
    carbon_monoxide = must_element("OMNI_PT_COMO", "COMO"),
    sulfur_dioxide = must_element("OMNI_PT_SODI", "SODI"),
    nitrogen_dioxide = must_element("OMNI_PT_NODI", "NODI"),
    calcium_oxide = must_element("OMNI_PT_CAOX", "CAOX"),
    carbonic_acid = must_element("OMNI_PT_CARA", "CARA"),
    hydrogen_sulfide = must_element("OMNI_PT_H2SG", "H2SG"),
    ammonia_water = must_element("OMNI_PT_AMWA", "AMWA"),
    barium_hydroxide = must_element("OMNI_PT_BAOH", "BAOH"),
    potassium_chloride = must_element("OMNI_PT_KCL", "KCL"),
    calcium_chloride = must_element("OMNI_PT_CACL", "CACL"),
    iron_chloride = must_element("OMNI_PT_FECL", "FECL"),
    sodium_sulfate = must_element("OMNI_PT_NASF", "NASF"),
    ammonium_nitrate = must_element("OMNI_PT_AMNT", "AMNT"),
    sodium_carbonate = must_element("OMNI_PT_NACO", "NACO"),
    potassium_permanganate = must_element("OMNI_PT_KPER", "KPER"),
    aluminium_oxide = must_element("OMNI_PT_ALOX", "ALOX"),
    magnesium_oxide = must_element("OMNI_PT_MGOX", "MGOX"),
    iron_oxide = must_element("OMNI_PT_FEOX", "FEOX"),
    copper_oxide = must_element("OMNI_PT_CUOX", "CUOX"),
    zinc_oxide = must_element("OMNI_PT_ZNOX", "ZNOX"),
    pathogen = must_element("OMNI_PT_PATH", "PATH"),
    humus = must_element("OMNI_PT_HUMS", "HUMS"),
    slag = must_element("OMNI_PT_SLAG", "SLAG"),
    flux = must_element("OMNI_PT_FLUX", "FLUX"),
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
    salt = assert(elements.DEFAULT_PT_SALT),
    salt_water = assert(elements.DEFAULT_PT_SLTW),
    caustic = assert(elements.DEFAULT_PT_CAUS),
    water_vapour = assert(elements.DEFAULT_PT_WTRV),
    glass = assert(elements.DEFAULT_PT_GLAS),
    quartz = assert(elements.DEFAULT_PT_QRTZ),
    iron = assert(elements.DEFAULT_PT_IRON),
    coal = assert(elements.DEFAULT_PT_COAL),
    fire = assert(elements.DEFAULT_PT_FIRE),
    aluminium = must_element("OMNI_PT_ALUM", "ALUM"),
    copper = must_element("OMNI_PT_COPR", "COPR"),
    magnesium = must_element("OMNI_PT_MAGN", "MAGN"),
    zinc = must_element("OMNI_PT_ZINC", "ZINC"),
    calcium = must_element("OMNI_PT_CA", "CA"),
    barium = must_element("OMNI_PT_BA", "BA"),
    manganese = must_element("OMNI_PT_MN", "MN"),
    sulfur = must_element("OMNI_PT_S", "S"),
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
    sim.resetOmniEventMetrics()
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

local function count_type(type)
    local count = 0
    for particle in sim.parts() do
        if sim.partProperty(particle, "type") == type then count = count + 1 end
    end
    return count
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
    assert(sim.partProperty(chlorine, "type") == ids.hydrochloric
        and sim.partProperty(hydrogen, "type") == ids.hydrochloric,
        "heated chlorine and hydrogen did not form hydrochloric acid")

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

local function run_biology_treatment()
    configure_simulation()
    local peroxide = make(ids.peroxide, 120, 120, 300.0)
    local pathogen = make(ids.pathogen, 121, 120, 300.0)
    step()
    assert(sim.partProperty(peroxide, "type") == ids.water
        and sim.partProperty(pathogen, "type") == ids.humus,
        "peroxide did not treat the adjacent pathogen")

    configure_simulation()
    peroxide = make(ids.peroxide, 120, 120, 280.0)
    pathogen = make(ids.pathogen, 121, 120, 280.0)
    step(3)
    assert(sim.partProperty(peroxide, "type") == ids.peroxide
        and sim.partProperty(pathogen, "type") == ids.pathogen,
        "cold peroxide treated a pathogen outside its window")
end

local function run_slag_recovery()
    configure_simulation()
    local catalyst = make(ids.catalyst, 120, 120, 320.0)
    local slag = make(ids.slag, 121, 120, 320.0)
    local acid = make(ids.acid, 120, 121, 320.0)
    step()
    assert(sim.partProperty(catalyst, "type") == ids.catalyst
        and sim.partProperty(slag, "type") == ids.flux
        and sim.partProperty(acid, "type") == ids.water,
        "mild catalytic acid leaching did not recover slag into flux and water")

    configure_simulation()
    catalyst = make(ids.catalyst, 120, 120, 350.0)
    slag = make(ids.slag, 121, 120, 350.0)
    acid = make(ids.acid, 120, 121, 350.0)
    step(3)
    assert(sim.partProperty(slag, "type") == ids.slag
        and sim.partProperty(acid, "type") == ids.acid,
        "hot slag leaching ran outside its registered temperature window")
end

local function run_inorganic_acids()
    configure_simulation()
    local acid = make(ids.hydrochloric, 120, 120, 300.0)
    local base = make(ids.sodium_hydroxide, 121, 120, 300.0)
    step()
    assert(sim.partProperty(acid, "type") == ids.water
        and sim.partProperty(base, "type") == ids.salt,
        "hydrochloric acid and sodium hydroxide did not neutralise")

    configure_simulation()
    acid = make(ids.sulfuric, 120, 120, 500.0)
    local copper = make(ids.copper, 121, 120, 500.0)
    step()
    assert(sim.partProperty(acid, "type") == ids.copper_sulfate
        and sim.partProperty(copper, "type") == ids.sulfur_dioxide,
        "hot sulfuric acid and copper did not form sulfate and sulfur dioxide")

    configure_simulation()
    acid = make(ids.nitric, 120, 120, 320.0)
    base = make(ids.potassium_hydroxide, 121, 120, 320.0)
    step()
    assert(sim.partProperty(acid, "type") == ids.water
        and sim.partProperty(base, "type") == ids.potassium_nitrate,
        "nitric acid and potassium hydroxide did not form potassium nitrate")

    configure_simulation()
    acid = make(ids.phosphoric, 120, 120, 300.0)
    local ammonia = make(ids.ammonia, 121, 120, 300.0)
    local ammonia_diffusion = elements.property(ids.ammonia, "Diffusion")
    elements.property(ids.ammonia, "Diffusion", 0.0)
    step()
    elements.property(ids.ammonia, "Diffusion", ammonia_diffusion)
    assert(sim.partProperty(acid, "type") == ids.fertilizer
        and sim.partProperty(ammonia, "type") == ids.water,
        "phosphoric acid and ammonia did not form fertilizer and water")

    configure_simulation()
    acid = make(ids.hydrofluoric, 120, 120, 300.0)
    local glass = make(ids.glass, 121, 120, 300.0)
    step()
    assert(sim.partProperty(acid, "type") == ids.caustic
        and sim.partProperty(glass, "type") == ids.dust,
        "hydrofluoric acid did not attack glass into bounded proxies")

    configure_simulation()
    acid = make(ids.hydrochloric, 120, 120, 300.0)
    local carbonate = make(ids.calcium_carbonate, 121, 120, 300.0)
    step()
    assert(sim.partProperty(acid, "type") == ids.water
        and sim.partProperty(carbonate, "type") == ids.salt
        and count_type(ids.co2) == 1,
        "acid carbonate route did not yield water salt and carbon dioxide")
end

local function run_inorganic_lime_and_bases()
    configure_simulation()
    local base = make(ids.sodium_hydroxide, 120, 120, 300.0)
    local carbon_dioxide = make(ids.co2, 121, 120, 300.0)
    local co2_diffusion = elements.property(ids.co2, "Diffusion")
    elements.property(ids.co2, "Diffusion", 0.0)
    step()
    elements.property(ids.co2, "Diffusion", co2_diffusion)
    assert(sim.partProperty(base, "type") == ids.sodium_bicarbonate
        and sim.partProperty(carbon_dioxide, "type") == ids.water,
        "sodium hydroxide carbonation did not form bicarbonate and water")

    configure_simulation()
    base = make(ids.calcium_hydroxide, 120, 120, 300.0)
    carbon_dioxide = make(ids.co2, 121, 120, 300.0)
    elements.property(ids.co2, "Diffusion", 0.0)
    step()
    elements.property(ids.co2, "Diffusion", co2_diffusion)
    assert(sim.partProperty(base, "type") == ids.calcium_carbonate
        and sim.partProperty(carbon_dioxide, "type") == ids.water,
        "calcium hydroxide carbonation did not form carbonate and water")

    configure_simulation()
    local lime = make(ids.calcium_oxide, 120, 120, 300.0)
    local water = make(ids.water, 121, 120, 300.0)
    step()
    assert(sim.partProperty(lime, "type") == ids.calcium_hydroxide
        and sim.partProperty(water, "type") == ids.calcium_hydroxide
        and sim.partProperty(lime, "temp") > 350.0,
        "calcium oxide hydration did not make two hot hydroxide particles")

    configure_simulation()
    local hydroxide = make(ids.calcium_hydroxide, 120, 120, 800.0)
    step()
    assert(sim.partProperty(hydroxide, "type") == ids.calcium_oxide
        and count_type(ids.water_vapour) == 1,
        "hot calcium hydroxide did not dehydrate into lime and steam")

    configure_simulation()
    local limestone = make(ids.calcium_carbonate, 120, 120, 1200.0)
    step()
    assert(sim.partProperty(limestone, "type") == ids.calcium_oxide
        and count_type(ids.co2) == 1,
        "hot calcium carbonate did not calcine into lime and carbon dioxide")

    configure_simulation()
    local bicarbonate = make(ids.sodium_bicarbonate, 120, 120, 450.0)
    step()
    assert(sim.partProperty(bicarbonate, "type") == ids.sodium_carbonate
        and count_type(ids.co2) == 1,
        "hot sodium bicarbonate did not produce sodium carbonate and carbon dioxide")

    configure_simulation()
    base = make(ids.sodium_hydroxide, 120, 120, 340.0)
    local aluminium = make(ids.aluminium, 121, 120, 340.0)
    step()
    assert(sim.partProperty(base, "type") == ids.salt
        and sim.partProperty(aluminium, "type") == ids.hydrogen,
        "warm sodium hydroxide did not attack aluminium and release hydrogen")
end

local function run_second_batch_acids_and_bases()
    configure_simulation()
    spark_catalyst(120, 120, 300.0)
    local carbon_dioxide = make(ids.co2, 121, 120, 300.0)
    local water = make(ids.water, 120, 121, 300.0)
    local co2_diffusion = elements.property(ids.co2, "Diffusion")
    elements.property(ids.co2, "Diffusion", 0.0)
    step()
    elements.property(ids.co2, "Diffusion", co2_diffusion)
    assert(sim.partProperty(carbon_dioxide, "type") == ids.carbonic_acid
        and sim.partProperty(water, "type") == ids.carbonic_acid,
        "electrified catalyst did not hydrate carbon dioxide into carbonic acid")

    configure_simulation()
    local carbonic = make(ids.carbonic_acid, 120, 120, 340.0)
    step()
    assert(sim.partProperty(carbonic, "type") == ids.water
        and count_type(ids.co2) == 1,
        "warm carbonic acid did not release water and carbon dioxide")

    configure_simulation()
    local ammonia = make(ids.ammonia, 120, 120, 300.0)
    water = make(ids.water, 121, 120, 300.0)
    local ammonia_diffusion = elements.property(ids.ammonia, "Diffusion")
    elements.property(ids.ammonia, "Diffusion", 0.0)
    step()
    elements.property(ids.ammonia, "Diffusion", ammonia_diffusion)
    assert(sim.partProperty(ammonia, "type") == ids.ammonia_water
        and sim.partProperty(water, "type") == ids.ammonia_water,
        "cool ammonia and water did not form ammonia water")

    configure_simulation()
    local ammonia_water = make(ids.ammonia_water, 120, 120, 380.0)
    step()
    assert(sim.partProperty(ammonia_water, "type") == ids.ammonia
        and count_type(ids.water_vapour) == 1,
        "warm ammonia water did not release ammonia and steam")

    local neutralisations = {
        { ids.hydrochloric, ids.potassium_hydroxide, ids.potassium_chloride,
            "hydrochloric acid and potassium hydroxide" },
        { ids.hydrochloric, ids.calcium_hydroxide, ids.calcium_chloride,
            "hydrochloric acid and calcium hydroxide" },
        { ids.sulfuric, ids.sodium_hydroxide, ids.sodium_sulfate,
            "sulfuric acid and sodium hydroxide" },
        { ids.nitric, ids.ammonia_water, ids.ammonium_nitrate,
            "nitric acid and ammonia water" },
        { ids.carbonic_acid, ids.sodium_hydroxide, ids.sodium_carbonate,
            "carbonic acid and sodium hydroxide" },
        { ids.hydrochloric, ids.barium_hydroxide, ids.salt,
            "hydrochloric acid and barium hydroxide" },
    }
    for _, case in ipairs(neutralisations) do
        configure_simulation()
        local acid = make(case[1], 120, 120, 300.0)
        local base = make(case[2], 121, 120, 300.0)
        step()
        assert(sim.partProperty(acid, "type") == ids.water
            and sim.partProperty(base, "type") == case[3],
            case[4] .. " did not yield water and the expected salt")
    end

    configure_simulation()
    local acid = make(ids.hydrochloric, 120, 120, 310.0)
    local iron = make(ids.iron, 121, 120, 310.0)
    step()
    assert(sim.partProperty(acid, "type") == ids.hydrogen
        and sim.partProperty(iron, "type") == ids.iron_chloride,
        "hydrochloric acid and iron did not form hydrogen and iron chloride")
end

local function run_second_batch_salts()
    configure_simulation()
    local chloride = make(ids.potassium_chloride, 120, 120, 300.0)
    local water = make(ids.water, 121, 120, 300.0)
    step()
    assert(sim.partProperty(chloride, "type") == ids.salt_water
        and sim.partProperty(water, "type") == ids.water,
        "potassium chloride did not dissolve into the salt-water proxy")

    configure_simulation()
    chloride = make(ids.calcium_chloride, 120, 120, 300.0)
    water = make(ids.water, 121, 120, 300.0)
    step()
    assert(sim.partProperty(chloride, "type") == ids.salt_water
        and sim.partProperty(water, "temp") > 350.0,
        "calcium chloride dissolution did not produce warm salt water")

    configure_simulation()
    local iron_chloride = make(ids.iron_chloride, 120, 120, 300.0)
    water = make(ids.water, 121, 120, 300.0)
    step()
    assert(sim.partProperty(iron_chloride, "type") == ids.iron_oxide
        and sim.partProperty(water, "type") == ids.hydrochloric,
        "iron chloride did not hydrolyse into iron oxide and hydrochloric acid")

    configure_simulation()
    local ammonium_nitrate = make(ids.ammonium_nitrate, 120, 120, 550.0)
    step()
    assert(sim.partProperty(ammonium_nitrate, "type") == ids.nitrogen_dioxide
        and count_type(ids.water_vapour) == 1,
        "hot ammonium nitrate did not produce bounded nitrogen dioxide and steam")

    configure_simulation()
    local permanganate = make(ids.potassium_permanganate, 120, 120, 300.0)
    local sulfide = make(ids.hydrogen_sulfide, 121, 120, 300.0)
    local sulfide_diffusion = elements.property(ids.hydrogen_sulfide, "Diffusion")
    elements.property(ids.hydrogen_sulfide, "Diffusion", 0.0)
    step()
    elements.property(ids.hydrogen_sulfide, "Diffusion", sulfide_diffusion)
    assert(sim.partProperty(permanganate, "type") == ids.manganese
        and sim.partProperty(sulfide, "type") == ids.sulfuric,
        "permanganate did not oxidise hydrogen sulfide into sulfuric acid")

    configure_simulation()
    permanganate = make(ids.potassium_permanganate, 120, 120, 500.0)
    local coal = make(ids.coal, 121, 120, 500.0)
    step()
    assert(sim.partProperty(permanganate, "type") == ids.manganese
        and sim.partProperty(coal, "type") == ids.fire
        and sim.partProperty(coal, "life") > 0,
        "hot permanganate did not produce bounded carbonaceous oxidation")
end

local function run_second_batch_oxides()
    local oxidation_cases = {
        { ids.aluminium, ids.aluminium_oxide, "aluminium" },
        { ids.magnesium, ids.magnesium_oxide, "magnesium" },
        { ids.iron, ids.iron_oxide, "iron" },
        { ids.copper, ids.copper_oxide, "copper" },
        { ids.zinc, ids.zinc_oxide, "zinc" },
        { ids.barium, ids.barium_hydroxide, "barium" },
    }
    for _, case in ipairs(oxidation_cases) do
        configure_simulation()
        local peroxide = make(ids.peroxide, 120, 120, 320.0)
        local metal = make(case[1], 121, 120, 320.0)
        step()
        assert(sim.partProperty(peroxide, "type") == ids.water
            and sim.partProperty(metal, "type") == case[2],
            "peroxide did not make the expected " .. case[3] .. " product")
    end

    local reduction_cases = {
        { ids.copper_oxide, ids.carbon_monoxide, ids.copper, ids.co2, 700.0,
            "copper oxide" },
        { ids.iron_oxide, ids.hydrogen, ids.iron, ids.water, 900.0,
            "iron oxide" },
        { ids.zinc_oxide, ids.carbon_monoxide, ids.zinc, ids.co2, 1000.0,
            "zinc oxide" },
    }
    for _, case in ipairs(reduction_cases) do
        configure_simulation()
        local oxide = make(case[1], 120, 120, case[5])
        local reducer = make(case[2], 121, 120, case[5])
        local diffusion = elements.property(case[2], "Diffusion")
        elements.property(case[2], "Diffusion", 0.0)
        step()
        elements.property(case[2], "Diffusion", diffusion)
        assert(sim.partProperty(oxide, "type") == case[3]
            and sim.partProperty(reducer, "type") == case[4],
            "hot reducing gas did not reduce " .. case[6])
    end

    configure_simulation()
    local sulfuric = make(ids.sulfuric, 120, 120, 300.0)
    local copper_oxide = make(ids.copper_oxide, 121, 120, 300.0)
    step()
    assert(sim.partProperty(sulfuric, "type") == ids.water
        and sim.partProperty(copper_oxide, "type") == ids.copper_sulfate,
        "sulfuric acid and copper oxide did not form copper sulfate")

    configure_simulation()
    local cold_oxide = make(ids.zinc_oxide, 120, 120, 900.0)
    local cold_reducer = make(ids.carbon_monoxide, 121, 120, 900.0)
    local co_diffusion = elements.property(ids.carbon_monoxide, "Diffusion")
    elements.property(ids.carbon_monoxide, "Diffusion", 0.0)
    step(3)
    elements.property(ids.carbon_monoxide, "Diffusion", co_diffusion)
    assert(sim.partProperty(cold_oxide, "type") == ids.zinc_oxide
        and sim.partProperty(cold_reducer, "type") == ids.carbon_monoxide,
        "zinc oxide reduced below its registered temperature threshold")
end

local function run_hydrogen_sulfide_cycle()
    configure_simulation()
    spark_catalyst(120, 120, 600.0)
    local sulfur = make(ids.sulfur, 121, 120, 600.0)
    local hydrogen = make(ids.hydrogen, 120, 121, 600.0)
    local hydrogen_diffusion = elements.property(ids.hydrogen, "Diffusion")
    elements.property(ids.hydrogen, "Diffusion", 0.0)
    step()
    elements.property(ids.hydrogen, "Diffusion", hydrogen_diffusion)
    assert(sim.partProperty(sulfur, "type") == ids.hydrogen_sulfide
        and sim.partProperty(hydrogen, "type") == ids.hydrogen_sulfide,
        "electrified catalyst did not form bounded hydrogen sulfide")

    configure_simulation()
    local sulfide = make(ids.hydrogen_sulfide, 120, 120, 500.0)
    local oxygen = make(ids.oxygen, 121, 120, 500.0)
    local sulfide_diffusion = elements.property(ids.hydrogen_sulfide, "Diffusion")
    local oxygen_diffusion = elements.property(ids.oxygen, "Diffusion")
    elements.property(ids.hydrogen_sulfide, "Diffusion", 0.0)
    elements.property(ids.oxygen, "Diffusion", 0.0)
    step()
    elements.property(ids.hydrogen_sulfide, "Diffusion", sulfide_diffusion)
    elements.property(ids.oxygen, "Diffusion", oxygen_diffusion)
    assert(sim.partProperty(sulfide, "type") == ids.sulfur_dioxide
        and sim.partProperty(oxygen, "type") == ids.water,
        "hot oxygen did not convert hydrogen sulfide into sulfur dioxide and water")

    configure_simulation()
    local cold_sulfide = make(ids.hydrogen_sulfide, 120, 120, 400.0)
    local cold_oxygen = make(ids.oxygen, 121, 120, 400.0)
    elements.property(ids.hydrogen_sulfide, "Diffusion", 0.0)
    elements.property(ids.oxygen, "Diffusion", 0.0)
    step(3)
    elements.property(ids.hydrogen_sulfide, "Diffusion", sulfide_diffusion)
    elements.property(ids.oxygen, "Diffusion", oxygen_diffusion)
    assert(sim.partProperty(cold_sulfide, "type") == ids.hydrogen_sulfide
        and sim.partProperty(cold_oxygen, "type") == ids.oxygen,
        "hydrogen sulfide oxidised below its registered threshold")

    configure_simulation()
    spark_catalyst(120, 120, 280.0)
    local cold_co2 = make(ids.co2, 121, 120, 280.0)
    local cold_water = make(ids.water, 120, 121, 280.0)
    local co2_diffusion = elements.property(ids.co2, "Diffusion")
    elements.property(ids.co2, "Diffusion", 0.0)
    step(3)
    elements.property(ids.co2, "Diffusion", co2_diffusion)
    assert(sim.partProperty(cold_co2, "type") == ids.co2
        and count_type(ids.carbonic_acid) == 0,
        "cold electrified catalyst hydrated carbon dioxide outside its window")
end

local function run_inorganic_salts_and_gases()
    configure_simulation()
    local sulfate = make(ids.copper_sulfate, 120, 120, 300.0)
    local iron = make(ids.iron, 121, 120, 300.0)
    step()
    assert(sim.partProperty(sulfate, "type") == ids.copper
        and sim.partProperty(iron, "type") == ids.salt,
        "iron did not displace copper from copper sulfate")

    configure_simulation()
    local nitrate = make(ids.potassium_nitrate, 120, 120, 600.0)
    local coal = make(ids.coal, 121, 120, 600.0)
    step()
    local nitrate_type = sim.partProperty(nitrate, "type")
    local coal_type = sim.partProperty(coal, "type")
    local coal_life = tonumber(sim.partProperty(coal, "life")) or -1
    assert(nitrate_type == ids.co2
        and coal_type == ids.fire
        and coal_life > 0,
        "hot potassium nitrate did not produce bounded oxidiser combustion: type="
            .. tostring(nitrate_type) .. " coal=" .. tostring(coal_type)
            .. " life=" .. tostring(coal_life))

    configure_simulation()
    local monoxide = make(ids.carbon_monoxide, 120, 120, 700.0)
    local oxygen = make(ids.oxygen, 121, 120, 700.0)
    local oxygen_diffusion = elements.property(ids.oxygen, "Diffusion")
    elements.property(ids.oxygen, "Diffusion", 0.0)
    step()
    elements.property(ids.oxygen, "Diffusion", oxygen_diffusion)
    assert(sim.partProperty(monoxide, "type") == ids.co2
        and sim.partProperty(oxygen, "type") == ids.fire
        and sim.partProperty(oxygen, "life") > 0,
        "hot carbon monoxide oxidation did not yield carbon dioxide and finite fire")

    configure_simulation()
    local sulfur_dioxide = make(ids.sulfur_dioxide, 120, 120, 320.0)
    local peroxide = make(ids.peroxide, 121, 120, 320.0)
    step()
    assert(sim.partProperty(sulfur_dioxide, "type") == ids.sulfuric
        and sim.partProperty(peroxide, "type") == ids.water,
        "peroxide did not oxidise sulfur dioxide into sulfuric acid")

    configure_simulation()
    local nitrogen_dioxide = make(ids.nitrogen_dioxide, 120, 120, 320.0)
    peroxide = make(ids.peroxide, 121, 120, 320.0)
    step()
    assert(sim.partProperty(nitrogen_dioxide, "type") == ids.nitric
        and sim.partProperty(peroxide, "type") == ids.water,
        "peroxide did not convert nitrogen dioxide into nitric acid")

    configure_simulation()
    local cold_acid = make(ids.sulfuric, 120, 120, 400.0)
    local cold_copper = make(ids.copper, 121, 120, 400.0)
    step(3)
    assert(sim.partProperty(cold_acid, "type") == ids.sulfuric
        and sim.partProperty(cold_copper, "type") == ids.copper,
        "cold sulfuric acid attacked copper below its registered threshold")

    configure_simulation()
    local cold_monoxide = make(ids.carbon_monoxide, 120, 120, 500.0)
    local cold_oxygen = make(ids.oxygen, 121, 120, 500.0)
    elements.property(ids.oxygen, "Diffusion", 0.0)
    step(3)
    elements.property(ids.oxygen, "Diffusion", oxygen_diffusion)
    assert(sim.partProperty(cold_monoxide, "type") == ids.carbon_monoxide
        and sim.partProperty(cold_oxygen, "type") == ids.oxygen,
        "cold carbon monoxide oxidised below its registered threshold")
end

local function run_inorganic_budget()
    configure_simulation()
    local total = 1800
    for index = 0, total - 1 do
        local x = 50 + (index % 100) * 5
        local y = 50 + math.floor(index / 100) * 5
        make(ids.calcium_carbonate, x, y, 1200.0)
    end
    step()
    local metrics = sim.omniEventMetrics()
    local events = assert(tonumber(metrics.total), "missing inorganic event metrics")
    assert(events > 0 and events <= 1536,
        "one-frame inorganic events were not capped at 1536: " .. tostring(events))
    assert(count_type(ids.calcium_carbonate) >= total - 1536,
        "budget exhaustion did not defer remaining carbonate calcination")
    return events
end

local function test()
    run_fuel_chain()
    run_polymerisation()
    run_electrochemistry()
    run_reaction_network()
    run_negative_control()
    run_biology_treatment()
    run_slag_recovery()
    run_inorganic_acids()
    run_inorganic_lime_and_bases()
    run_inorganic_salts_and_gases()
    run_second_batch_acids_and_bases()
    run_second_batch_salts()
    run_second_batch_oxides()
    run_hydrogen_sulfide_cycle()
    return run_inorganic_budget()
end

local ok, data = xpcall(test, debug.traceback)
local report = assert(io.open(RESULT, "w"))
if ok then
    report:write("OMNI_CHEMISTRY_STATUS=PASS\n")
    report:write("OMNI_CHEMISTRY_PATHS=58\n")
    report:write("OMNI_CHEMISTRY_ELEMENTS=42\n")
    report:write("OMNI_INORGANIC_ELEMENTS=32\n")
    report:write("OMNI_CHEMISTRY_BUDGET_EVENTS=" .. tostring(data) .. "\n")
    report:write("OMNI_CHEMISTRY_IDS=" .. ids.chlorine .. "-" .. ids.fertilizer .. "\n")
    report:write("OMNI_INORGANIC_IDS=" .. ids.hydrochloric .. "-" .. ids.calcium_oxide .. "\n")
    report:write("OMNI_INORGANIC_BATCH2_IDS=" .. ids.carbonic_acid .. "-" .. ids.zinc_oxide .. "\n")
else
    local error_text = tostring(data):gsub("[\r\n]+", " | ")
    report:write("OMNI_CHEMISTRY_STATUS=FAIL\n")
    report:write("OMNI_CHEMISTRY_ERROR=" .. error_text .. "\n")
end
report:close()
