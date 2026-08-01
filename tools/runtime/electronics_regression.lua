local RESULT = "lua-electronics-regression.result"

local function must_element(identifier, short_name)
    local id = elements[identifier]
    assert(type(id) == "number", "missing element constant: " .. identifier)
    assert(elements.getByName(short_name) == id,
        "name/identifier mismatch for " .. identifier)
    return id
end

local ids
local initialise_ok, initialise_error = xpcall(function()
ids = {
    gaas = must_element("OMNI_PT_GAAS", "GAAS"),
    gani = must_element("OMNI_PT_GANI", "GANI"),
    frit = must_element("OMNI_PT_FRIT", "FRIT"),
    pmag = must_element("OMNI_PT_PMAG", "PMAG"),
    smag = must_element("OMNI_PT_SMAG", "SMAG"),
    pzcr = must_element("OMNI_PT_PZCR", "PZCR"),
    telc = must_element("OMNI_PT_TELC", "TELC"),
    supc = must_element("OMNI_PT_SUPC", "SUPC"),
    grph = must_element("OMNI_PT_GRPH", "GRPH"),
    cntb = must_element("OMNI_PT_CNTB", "CNTB"),
    aerg = must_element("OMNI_PT_AERG", "AERG"),
    cfrp = must_element("OMNI_PT_CFRP", "CFRP"),
    lcob = must_element("OMNI_PT_LCOB", "LCOB"),
    gran = must_element("OMNI_PT_GRAN", "GRAN"),
    sele = must_element("OMNI_PT_SELE", "SELE"),
    itox = must_element("OMNI_PT_ITOX", "ITOX"),
    pcmt = must_element("OMNI_PT_PCMT", "PCMT"),
    echr = must_element("OMNI_PT_ECHR", "ECHR"),
    phrs = must_element("OMNI_PT_PHRS", "PHRS"),
    diel = must_element("OMNI_PT_DIEL", "DIEL"),

    catalyst = assert(elements.OMNI_PT_CATA),
    yttrium = assert(elements.OMNI_PT_Y),
    barium = assert(elements.OMNI_PT_BA),
    copper_oxide = assert(elements.OMNI_PT_CUOX),
    lanthanum = assert(elements.OMNI_PT_LA),
    zirconium = assert(elements.OMNI_PT_ZR),
    neodymium = assert(elements.OMNI_PT_ND),
    boron = assert(elements.OMNI_PT_B),
    titanium_dioxide = assert(elements.OMNI_PT_TIOX),
    cobalt = assert(elements.OMNI_PT_COBT),
    indium = assert(elements.OMNI_PT_IN),
    tin = assert(elements.OMNI_PT_TIN),
    germanium = assert(elements.OMNI_PT_GE),
    antimony = assert(elements.OMNI_PT_SB),
    tellurium = assert(elements.OMNI_PT_TE),
    gallium = assert(elements.OMNI_PT_GA),
    arsenic = assert(elements.OMNI_PT_AS),
    nitrogen = assert(elements.OMNI_PT_N),
    iron_oxide = assert(elements.OMNI_PT_FEOX),
    zinc_oxide = assert(elements.OMNI_PT_ZNOX),
    bismuth = assert(elements.OMNI_PT_BI),
    epoxy = assert(elements.OMNI_PT_EPXY),
    epoxy_resin = assert(elements.OMNI_PT_ERES),
    polystyrene = assert(elements.OMNI_PT_PSTY),
    phosphorus = assert(elements.OMNI_PT_P),
    acetone = assert(elements.OMNI_PT_ACET),

    oxygen = assert(elements.DEFAULT_PT_O2),
    lithium = assert(elements.DEFAULT_PT_LITH),
    iron = assert(elements.DEFAULT_PT_IRON),
    lead = assert(elements.OMNI_PT_LEAD),
    tungsten = assert(elements.DEFAULT_PT_TUNG),
    silicon = assert(elements.DEFAULT_PT_SLCN),
    p_silicon = assert(elements.DEFAULT_PT_PSCN),
    n_silicon = assert(elements.DEFAULT_PT_NSCN),
    quartz = assert(elements.DEFAULT_PT_QRTZ),
    water = assert(elements.DEFAULT_PT_WATR),
    coal = assert(elements.DEFAULT_PT_COAL),
    photon = assert(elements.DEFAULT_PT_PHOT),
    electron = assert(elements.DEFAULT_PT_ELEC),
    spark = assert(elements.DEFAULT_PT_SPRK),
    metal = assert(elements.DEFAULT_PT_METL),
    carbon_dioxide = assert(elements.DEFAULT_PT_CO2),
    fire = assert(elements.DEFAULT_PT_FIRE),
    lava = assert(elements.DEFAULT_PT_LAVA),
}

assert(ids.gaas == 622 and ids.diel == 641,
    "electronics stable range changed: expected 622..641")
end, debug.traceback)

if not initialise_ok then
    local report = assert(io.open(RESULT, "w"))
    local error_text = tostring(initialise_error):gsub("[\r\n]+", " | ")
    report:write("OMNI_ELECTRONICS_ERROR=" .. error_text .. "\n")
    report:write("OMNI_ELECTRONICS_STATUS=FAIL\n")
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
    sim.randomSeed(61, 67, 71, 73)
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

local recipe_coordinates = {
    { 121, 120 }, { 120, 121 }, { 119, 120 }, { 120, 119 },
}

local function run_recipe(recipe)
    configure()
    make(ids.catalyst, 120, 120, recipe.temperature)
    local inputs = {}
    for index, element in ipairs(recipe.inputs) do
        local position = recipe_coordinates[index]
        inputs[index] = make(element, position[1], position[2], recipe.temperature)
    end
    if recipe.pressure then
        sim.airMode(sim.AIR_NOUPDATE)
        sim.pressure(30, 30, recipe.pressure)
    end
    sim.updateUpTo()
    for _, particle in ipairs(inputs) do
        local actual = sim.partProperty(particle, "type")
        local metrics = sim.omniEventMetrics()
        local second_type = inputs[2]
            and sim.partProperty(inputs[2], "type") or -1
        local product_matches = actual == recipe.product
            or (actual == ids.lava
                and sim.partProperty(particle, "ctype") == recipe.product)
        assert(product_matches,
            recipe.name .. " did not produce the registered material: expected="
            .. recipe.product .. " actual=" .. tostring(actual)
            .. " ctype=" .. tostring(sim.partProperty(particle, "ctype"))
            .. " second=" .. tostring(second_type)
            .. " pressure=" .. tostring(sim.pressure(30, 30))
            .. " events=" .. tostring(metrics.total))
    end
end

local recipes = {
    { name = "superconductor", inputs = { ids.yttrium, ids.barium, ids.copper_oxide, ids.oxygen }, product = ids.supc, temperature = 1500.0 },
    { name = "solid electrolyte", inputs = { ids.lithium, ids.lanthanum, ids.zirconium, ids.oxygen }, product = ids.sele, temperature = 1600.0 },
    { name = "permanent magnet", inputs = { ids.neodymium, ids.iron, ids.boron }, product = ids.pmag, temperature = 1500.0 },
    { name = "piezoelectric ceramic", inputs = { ids.lead, ids.zirconium, ids.titanium_dioxide }, product = ids.pzcr, temperature = 1500.0 },
    { name = "lithium cobalt oxide", inputs = { ids.lithium, ids.cobalt, ids.oxygen }, product = ids.lcob, temperature = 1200.0 },
    { name = "indium tin oxide", inputs = { ids.indium, ids.tin, ids.oxygen }, product = ids.itox, temperature = 1100.0 },
    { name = "phase-change material", inputs = { ids.germanium, ids.antimony, ids.tellurium }, product = ids.pcmt, temperature = 1100.0 },
    { name = "electrochromic material", inputs = { ids.tungsten, ids.oxygen, ids.quartz }, product = ids.echr, temperature = 1200.0 },
    { name = "gallium arsenide", inputs = { ids.gallium, ids.arsenic }, product = ids.gaas, temperature = 1300.0 },
    { name = "gallium nitride", inputs = { ids.gallium, ids.nitrogen }, product = ids.gani, temperature = 1600.0, pressure = 3.0 },
    { name = "ferrite", inputs = { ids.iron_oxide, ids.zinc_oxide }, product = ids.frit, temperature = 1100.0 },
    { name = "soft magnet", inputs = { ids.iron, ids.silicon }, product = ids.smag, temperature = 1300.0 },
    { name = "thermoelectric material", inputs = { ids.bismuth, ids.tellurium }, product = ids.telc, temperature = 800.0 },
    { name = "carbon nanotube", inputs = { ids.grph, ids.grph }, product = ids.cntb, temperature = 1100.0 },
    { name = "carbon-fiber composite", inputs = { ids.cntb, ids.epoxy }, product = ids.cfrp, temperature = 400.0 },
    { name = "photoresist", inputs = { ids.epoxy_resin, ids.polystyrene }, product = ids.phrs, temperature = 380.0 },
    { name = "dielectric ceramic", inputs = { ids.barium, ids.titanium_dioxide }, product = ids.diel, temperature = 1200.0 },
    { name = "aerogel", inputs = { ids.quartz, ids.water }, product = ids.aerg, temperature = 420.0 },
    { name = "graphite anode", inputs = { ids.coal }, product = ids.gran, temperature = 1800.0 },
    { name = "graphene", inputs = { ids.gran }, product = ids.grph, temperature = 1100.0, pressure = 4.0 },
    { name = "p-type silicon", inputs = { ids.silicon, ids.boron }, product = ids.p_silicon, temperature = 1000.0 },
    { name = "n-type silicon", inputs = { ids.silicon, ids.phosphorus }, product = ids.n_silicon, temperature = 1000.0 },
}

local function spark_particle(particle, source_type, life)
    sim.partProperty(particle, "type", ids.spark)
    sim.partProperty(particle, "ctype", source_type)
    sim.partProperty(particle, "life", life or 4)
    return particle
end

local function run_photoconduction()
    configure()
    local material = make(ids.gaas, 120, 120, 300.0)
    local light = make(ids.photon, 121, 120, 300.0)
    sim.partProperty(light, "vx", 0.0)
    sim.partProperty(light, "vy", 0.0)
    sim.updateUpTo()
    assert(sim.partProperty(material, "type") == ids.spark
            and sim.partProperty(material, "ctype") == ids.gaas,
        "gallium arsenide did not produce a pulse under light")
end

local function run_electroluminescence_and_ferrite()
    configure()
    local led = spark_particle(make(ids.gani, 120, 120, 300.0), ids.gani, 4)
    sim.updateUpTo()
    assert(sim.partExists(led) and count_type(ids.photon) == 1,
        "gallium nitride did not emit one photon")

    configure()
    local ferrite = spark_particle(make(ids.frit, 120, 120, 300.0), ids.frit, 4)
    sim.updateUpTo()
    assert(sim.partProperty(ferrite, "temp") >= 308.0,
        "ferrite did not convert its pulse into heat")
end

local function run_magnetic_and_pressure_behaviour()
    configure()
    make(ids.pmag, 120, 120, 300.0)
    local iron = make(ids.iron, 121, 120, 300.0)
    sim.partProperty(iron, "vx", 0.0)
    sim.partProperty(iron, "vy", 0.0)
    sim.updateUpTo()
    local magnetic_metrics = sim.omniEventMetrics()
    assert(tonumber(magnetic_metrics.total) == 1,
        "permanent magnet did not register an iron-attraction event")

    configure()
    local piezo = make(ids.pzcr, 120, 120, 300.0)
    sim.airMode(sim.AIR_NOUPDATE)
    sim.pressure(30, 30, 6.0)
    sim.updateUpTo()
    assert(sim.partProperty(piezo, "type") == ids.spark
            and sim.partProperty(piezo, "ctype") == ids.pzcr,
        "piezoelectric ceramic did not respond to pressure")

    configure()
    local nanotube = make(ids.cntb, 120, 120, 300.0)
    local aerogel = make(ids.aerg, 121, 120, 300.0)
    sim.airMode(sim.AIR_NOUPDATE)
    sim.pressure(30, 30, 30.0)
    sim.updateUpTo()
    assert(sim.partProperty(nanotube, "type") == ids.grph,
        "carbon nanotube did not collapse into graphene")
    assert(sim.partProperty(aerogel, "type") == ids.quartz,
        "aerogel did not collapse into quartz")
end

local function run_temperature_and_superconducting_behaviour()
    configure()
    local thermoelectric = make(ids.telc, 120, 120, 300.0)
    make(ids.metal, 121, 120, 550.0)
    sim.updateUpTo()
    assert(sim.partProperty(thermoelectric, "type") == ids.spark
            and sim.partProperty(thermoelectric, "ctype") == ids.telc,
        "thermoelectric material did not respond to the temperature difference")

    configure()
    local superconductor = make(ids.supc, 120, 120, 100.0)
    spark_particle(make(ids.metal, 121, 120, 100.0), ids.metal, 4)
    sim.updateUpTo()
    assert(sim.partProperty(superconductor, "type") == ids.spark
            and sim.partProperty(superconductor, "ctype") == ids.supc,
        "cold superconductor did not accept a pulse")

    configure()
    local graphene = make(ids.grph, 120, 120, 950.0)
    local oxygen = make(ids.oxygen, 121, 120, 950.0)
    sim.updateUpTo()
    assert(sim.partProperty(graphene, "type") == ids.carbon_dioxide
            and not sim.partExists(oxygen),
        "hot graphene did not oxidise to carbon dioxide")
end

local function run_battery_behaviour()
    configure()
    local cathode = spark_particle(make(ids.lcob, 120, 120, 300.0), ids.lcob, 4)
    sim.partProperty(cathode, "tmp", 100)
    make(ids.sele, 121, 120, 300.0)
    local anode = make(ids.gran, 120, 121, 300.0)
    sim.partProperty(anode, "tmp", 0)
    sim.updateUpTo()
    assert(sim.partProperty(cathode, "tmp") == 95
            and sim.partProperty(anode, "tmp") == 5,
        "battery electrodes did not transfer five charge units")

    configure()
    local hot_cathode = make(ids.lcob, 120, 120, 700.0)
    local oxygen = make(ids.oxygen, 121, 120, 700.0)
    sim.updateUpTo()
    assert(sim.partProperty(hot_cathode, "type") == ids.cobalt
            and sim.partProperty(oxygen, "type") == ids.fire,
        "hot lithium cobalt oxide did not enter thermal runaway")
end

local function run_memory_and_optical_behaviour()
    configure()
    local memory = make(ids.pcmt, 120, 120, 900.0)
    sim.updateUpTo()
    assert(sim.partProperty(memory, "tmp") == 0,
        "phase-change material did not enter the amorphous state")
    sim.partProperty(memory, "life", 0)
    sim.partProperty(memory, "temp", 600.0)
    sim.updateUpTo()
    assert(sim.partProperty(memory, "tmp") == 1,
        "phase-change material did not anneal to the crystalline state")

    configure()
    local electrochromic = make(ids.echr, 120, 120, 300.0)
    spark_particle(make(ids.metal, 121, 120, 300.0), ids.metal, 4)
    sim.updateUpTo()
    assert(sim.partProperty(electrochromic, "tmp") == 1,
        "electrochromic material did not change state")

    configure()
    local photoresist = make(ids.phrs, 120, 120, 300.0)
    local light = make(ids.photon, 121, 120, 300.0)
    sim.partProperty(light, "vx", 0.0)
    sim.partProperty(light, "vy", 0.0)
    sim.updateUpTo()
    sim.updateUpTo()
    assert(sim.partProperty(photoresist, "tmp") >= 50,
        "photoresist did not accumulate two exposures")
    if sim.partExists(light) then sim.partKill(light) end
    make(ids.acetone, 121, 120, 300.0)
    sim.updateUpTo()
    assert(not sim.partExists(photoresist),
        "exposed photoresist was not removed by acetone")
end

local function run_dielectric_behaviour()
    configure()
    local dielectric = make(ids.diel, 120, 120, 300.0)
    spark_particle(make(ids.metal, 121, 120, 300.0), ids.metal, 4)
    sim.updateUpTo()
    assert(sim.partProperty(dielectric, "tmp") == 1,
        "dielectric ceramic did not accept a charge pulse")

    configure()
    dielectric = make(ids.diel, 120, 120, 300.0)
    sim.partProperty(dielectric, "tmp", 8)
    local conductor = make(ids.metal, 121, 120, 300.0)
    sim.updateUpTo()
    assert(sim.partProperty(dielectric, "tmp") == 0
            and sim.partProperty(conductor, "type") == ids.spark
            and sim.partProperty(conductor, "ctype") == ids.metal,
        "charged dielectric ceramic did not discharge into the conductor")
end

local function run_material_differences()
    assert(elements.property(ids.aerg, "Weight") < elements.property(ids.cntb, "Weight")
            and elements.property(ids.cntb, "Weight") < elements.property(ids.diel, "Weight"),
        "electronic material density ordering collapsed")
    assert(elements.property(ids.aerg, "HeatConduct")
            < elements.property(ids.grph, "HeatConduct"),
        "aerogel and graphene no longer differ in thermal conduction")
    assert(elements.property(ids.cfrp, "Hardness")
            > elements.property(ids.grph, "Hardness"),
        "carbon-fiber composite no longer exceeds graphene hardness")
end

local function run_budget_case()
    configure()
    local sources = {}
    for row = 0, 21 do
        for column = 0, 49 do
            local x = 10 + column * 4
            local y = 10 + row * 4
            make(ids.catalyst, x, y, 1800.0)
            sources[#sources + 1] = make(ids.coal, x + 1, y, 1800.0)
        end
    end
    sim.updateUpTo()
    local reacted = 0
    for _, source in ipairs(sources) do
        if sim.partProperty(source, "type") == ids.gran then reacted = reacted + 1 end
    end
    local metrics = sim.omniEventMetrics()
    local peak = assert(tonumber(metrics.peak_per_frame), "missing event peak")
    assert(reacted == 1024 and peak == 1024,
        "electronics event limit changed: reacted=" .. reacted
        .. " peak=" .. tostring(peak))
    return reacted, peak
end

local function test()
    for _, recipe in ipairs(recipes) do run_recipe(recipe) end
    run_photoconduction()
    run_electroluminescence_and_ferrite()
    run_magnetic_and_pressure_behaviour()
    run_temperature_and_superconducting_behaviour()
    run_battery_behaviour()
    run_memory_and_optical_behaviour()
    run_dielectric_behaviour()
    run_material_differences()
    return run_budget_case()
end

local ok, reacted, peak = xpcall(test, debug.traceback)
local report = assert(io.open(RESULT, "w"))
if ok then
    report:write("OMNI_ELECTRONICS_ELEMENTS=20\n")
    report:write("OMNI_ELECTRONICS_IDS=622-641\n")
    report:write("OMNI_ELECTRONICS_SYNTHESIS_PATHS=22\n")
    report:write("OMNI_ELECTRONICS_BEHAVIOURS=16\n")
    report:write("OMNI_ELECTRONICS_EVENT_REACTED=", reacted, "\n")
    report:write("OMNI_ELECTRONICS_EVENT_PEAK=", peak, "\n")
    report:write("OMNI_ELECTRONICS_STATUS=PASS\n")
else
    local error_text = tostring(reacted):gsub("[\r\n]+", " | ")
    report:write("OMNI_ELECTRONICS_ERROR=" .. error_text .. "\n")
    report:write("OMNI_ELECTRONICS_STATUS=FAIL\n")
end
report:close()
