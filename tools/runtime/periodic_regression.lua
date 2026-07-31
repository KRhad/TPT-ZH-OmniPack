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
    boron = must_element("OMNI_PT_B", "B", 372),
    aluminium = must_element("OMNI_PT_ALUM", "ALUM", 256),
    gallium = must_element("OMNI_PT_GA", "GA", 385),
    indium = must_element("OMNI_PT_IN", "IN", 401),
    thallium = must_element("OMNI_PT_TL", "TL", 428),
    nihonium = must_element("OMNI_PT_NH", "NH", 456),
    diamond = must_element("DEFAULT_PT_DMND", "DMND", 28),
    silicon = must_element("DEFAULT_PT_SLCN", "SLCN", 187),
    germanium = must_element("OMNI_PT_GE", "GE", 386),
    tin = must_element("OMNI_PT_TIN", "TIN", 259),
    lead = must_element("OMNI_PT_LEAD", "LEAD", 258),
    flerovium = must_element("OMNI_PT_FL", "FL", 457),
    nitrogen = must_element("OMNI_PT_N", "N", 373),
    phosphorus = must_element("OMNI_PT_P", "P", 377),
    arsenic = must_element("OMNI_PT_AS", "AS", 387),
    antimony = must_element("OMNI_PT_SB", "SB", 402),
    bismuth = must_element("OMNI_PT_BI", "BI", 429),
    moscovium = must_element("OMNI_PT_MC", "MC", 458),
    sulfur = must_element("OMNI_PT_S", "S", 378),
    selenium = must_element("OMNI_PT_SE", "SE", 388),
    tellurium = must_element("OMNI_PT_TE", "TE", 403),
    livermorium = must_element("OMNI_PT_LV", "LV", 459),
    fluorine = must_element("OMNI_PT_F", "F", 374),
    chlorine = must_element("OMNI_PT_CHLR", "CHLR", 360),
    bromine = must_element("OMNI_PT_BR", "BR", 389),
    iodine = must_element("OMNI_PT_I", "I", 404),
    astatine = must_element("OMNI_PT_AT", "AT", 430),
    tennessine = must_element("OMNI_PT_TS", "TS", 460),
    scandium = must_element("OMNI_PT_SC", "SC", 382),
    titanium = must_element("DEFAULT_PT_TTAN", "TTAN", 144),
    vanadium = must_element("OMNI_PT_V", "V", 383),
    chromium = must_element("OMNI_PT_CHRM", "CHRM", 262),
    manganese = must_element("OMNI_PT_MN", "MN", 384),
    iron = must_element("DEFAULT_PT_IRON", "IRON", 76),
    cobalt = must_element("OMNI_PT_COBT", "COBT", 263),
    nickel = must_element("OMNI_PT_NICL", "NICL", 260),
    copper = must_element("OMNI_PT_COPR", "COPR", 257),
    zinc = must_element("OMNI_PT_ZINC", "ZINC", 265),
    yttrium = must_element("OMNI_PT_Y", "Y", 392),
    zirconium = must_element("OMNI_PT_ZR", "ZR", 393),
    niobium = must_element("OMNI_PT_NB", "NB", 394),
    molybdenum = must_element("OMNI_PT_MOLY", "MOLY", 264),
    technetium = must_element("OMNI_PT_TC", "TC", 395),
    ruthenium = must_element("OMNI_PT_RU", "RU", 396),
    rhodium = must_element("OMNI_PT_RH", "RH", 397),
    palladium = must_element("OMNI_PT_PD", "PD", 398),
    silver = must_element("OMNI_PT_AG", "AG", 399),
    cadmium = must_element("OMNI_PT_CD", "CD", 400),
    lanthanum = must_element("OMNI_PT_LA", "LA", 408),
    cerium = must_element("OMNI_PT_CE", "CE", 409),
    praseodymium = must_element("OMNI_PT_PR", "PR", 410),
    neodymium = must_element("OMNI_PT_ND", "ND", 411),
    promethium = must_element("OMNI_PT_PM", "PM", 412),
    samarium = must_element("OMNI_PT_SM", "SM", 413),
    europium = must_element("OMNI_PT_EU", "EU", 414),
    gadolinium = must_element("OMNI_PT_GD", "GD", 415),
    terbium = must_element("OMNI_PT_TB", "TB", 416),
    dysprosium = must_element("OMNI_PT_DY", "DY", 417),
    holmium = must_element("OMNI_PT_HO", "HO", 418),
    erbium = must_element("OMNI_PT_ER", "ER", 419),
    thulium = must_element("OMNI_PT_TM", "TM", 420),
    ytterbium = must_element("OMNI_PT_YB", "YB", 421),
    lutetium = must_element("OMNI_PT_LU", "LU", 422),
    actinium = must_element("OMNI_PT_AC", "AC", 434),
    thorium = must_element("OMNI_PT_TH", "TH", 435),
    protactinium = must_element("OMNI_PT_PA", "PA", 436),
    uranium = must_element("DEFAULT_PT_URAN", "URAN", 32),
    neptunium = must_element("OMNI_PT_NP", "NP", 437),
    americium = must_element("OMNI_PT_AM", "AM", 438),
    curium = must_element("OMNI_PT_CM", "CM", 439),
    berkelium = must_element("OMNI_PT_BK", "BK", 440),
    californium = must_element("OMNI_PT_CF", "CF", 441),
    einsteinium = must_element("OMNI_PT_ES", "ES", 442),
    fermium = must_element("OMNI_PT_FM", "FM", 443),
    mendelevium = must_element("OMNI_PT_MD", "MD", 444),
    nobelium = must_element("OMNI_PT_NO", "NO", 445),
    lawrencium = must_element("OMNI_PT_LR", "LR", 446),
    hafnium = must_element("OMNI_PT_HF", "HF", 423),
    tantalum = must_element("OMNI_PT_TA", "TA", 424),
    tungsten = must_element("DEFAULT_PT_TUNG", "TUNG", 171),
    rhenium = must_element("OMNI_PT_RE", "RE", 425),
    osmium = must_element("OMNI_PT_OS", "OS", 426),
    iridium = must_element("OMNI_PT_IR", "IR", 427),
    platinum = must_element("DEFAULT_PT_PTNM", "PTNM", 188),
    gold = must_element("DEFAULT_PT_GOLD", "GOLD", 170),
    mercury = must_element("DEFAULT_PT_MERC", "MERC", 152),
    lithium = must_element("DEFAULT_PT_LITH", "LITH", 191),
    rubidium = must_element("DEFAULT_PT_RBDM", "RBDM", 41),
    hydrogen = must_element("DEFAULT_PT_H2", "HYGN", 148),
    water = must_element("DEFAULT_PT_WATR", "WATR", 2),
    water_vapor = must_element("DEFAULT_PT_WTRV", "WTRV", 23),
    acid = must_element("DEFAULT_PT_ACID", "ACID", 21),
    caustic = must_element("DEFAULT_PT_CAUS", "CAUS", 86),
    salt = must_element("DEFAULT_PT_SALT", "SALT", 26),
    dust = must_element("DEFAULT_PT_DUST", "DUST", 1),
    oxygen = must_element("DEFAULT_PT_O2", "OXYG", 61),
    liquid_oxygen = must_element("DEFAULT_PT_LO2", "LOXY", 60),
    neutron = must_element("DEFAULT_PT_NEUT", "NEUT", 18),
    glass = must_element("DEFAULT_PT_GLAS", "GLAS", 45),
    liquid_nitrogen = must_element("DEFAULT_PT_LNTG", "LN2", 37),
    nitrogen_ice = must_element("DEFAULT_PT_NICE", "NICE", 51),
    lava = must_element("DEFAULT_PT_LAVA", "LAVA", 6),
    fire = must_element("DEFAULT_PT_FIRE", "FIRE", 4),
    smoke = must_element("DEFAULT_PT_SMKE", "SMKE", 57),
    plutonium = must_element("DEFAULT_PT_PLUT", "PLUT", 19),
    metal = assert(elements.DEFAULT_PT_METL),
    electron = assert(elements.DEFAULT_PT_ELEC),
    photon = assert(elements.DEFAULT_PT_PHOT),
    polonium = must_element("DEFAULT_PT_POLO", "POLO", 182),
    scrap = must_element("OMNI_PT_MSCR", "MSCR", 278),
    steel = must_element("OMNI_PT_STEL", "STEL", 268),
    tool_steel = must_element("OMNI_PT_TSTL", "TSTL", 274),
    slag = must_element("OMNI_PT_SLAG", "SLAG", 275),
    pathogen = must_element("OMNI_PT_PATH", "PATH", 292),
    peroxide = must_element("OMNI_PT_PERO", "PERO", 368),
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
    assert(elements.property(ids.boron, "Properties")
            ~= elements.property(ids.aluminium, "Properties"),
        "metalloid boron and conductive aluminium have identical properties")
    assert(elements.property(ids.gallium, "HighTemperature")
            < elements.property(ids.indium, "HighTemperature")
            and elements.property(ids.indium, "HighTemperature")
                < elements.property(ids.thallium, "HighTemperature"),
        "gallium indium and thallium do not retain distinct melting points")
    assert(elements.property(ids.nihonium, "Properties")
            ~= elements.property(ids.gallium, "Properties"),
        "radioactive nihonium and stable gallium have identical properties")
    assert(elements.property(ids.diamond, "Meltable") == 0,
        "diamond carbon mapping lost its indestructible non-meltable property")
    assert(elements.property(ids.silicon, "Falldown")
            ~= elements.property(ids.germanium, "Falldown"),
        "powder silicon and solid germanium have identical movement properties")
    assert(elements.property(ids.germanium, "Hardness")
            > elements.property(ids.tin, "Hardness"),
        "brittle germanium is not harder than soft tin")
    assert(elements.property(ids.tin, "HighTemperature")
            < elements.property(ids.lead, "HighTemperature"),
        "tin and lead do not retain distinct melting points")
    assert(elements.property(ids.lead, "Weight")
            > elements.property(ids.tin, "Weight"),
        "dense lead is not heavier than tin")
    assert(elements.property(ids.flerovium, "Properties")
            ~= elements.property(ids.germanium, "Properties"),
        "radioactive flerovium and stable germanium have identical properties")
    assert(elements.property(ids.nitrogen, "Falldown")
            ~= elements.property(ids.phosphorus, "Falldown"),
        "gaseous nitrogen and powdered phosphorus have identical movement properties")
    assert(elements.property(ids.antimony, "HighTemperature")
            > elements.property(ids.bismuth, "HighTemperature"),
        "antimony and bismuth do not retain distinct melting points")
    assert(elements.property(ids.arsenic, "Properties")
            ~= elements.property(ids.bismuth, "Properties"),
        "deadly arsenic and simplified non-deadly bismuth have identical properties")
    assert(elements.property(ids.moscovium, "Properties")
            ~= elements.property(ids.bismuth, "Properties"),
        "radioactive moscovium and stable bismuth have identical properties")
    assert(elements.property(ids.sulfur, "Falldown")
            ~= elements.property(ids.tellurium, "Falldown"),
        "powdered sulfur and fixed tellurium have identical movement properties")
    assert(elements.property(ids.selenium, "HighTemperature")
            < elements.property(ids.tellurium, "HighTemperature"),
        "selenium and tellurium do not retain distinct melting points")
    assert(elements.property(ids.livermorium, "Properties")
            ~= elements.property(ids.selenium, "Properties"),
        "radioactive livermorium and stable selenium have identical properties")
    assert(elements.property(ids.fluorine, "Diffusion")
            > elements.property(ids.chlorine, "Diffusion"),
        "fluorine and chlorine do not retain distinct gas diffusion")
    assert(elements.property(ids.bromine, "Falldown")
            ~= elements.property(ids.iodine, "Falldown"),
        "liquid bromine and solid iodine have identical movement properties")
    assert(elements.property(ids.astatine, "Properties")
            ~= elements.property(ids.iodine, "Properties"),
        "radioactive astatine and stable iodine have identical properties")
    assert(elements.property(ids.tennessine, "Properties")
            ~= elements.property(ids.bromine, "Properties"),
        "superheavy tennessine and liquid bromine have identical properties")
    assert(elements.property(ids.scandium, "Weight")
            < elements.property(ids.titanium, "Weight"),
        "light scandium is not lighter than official titanium")
    assert(elements.property(ids.vanadium, "HighTemperature")
            > elements.property(ids.manganese, "HighTemperature"),
        "vanadium and manganese do not retain distinct melting points")
    assert(elements.property(ids.copper, "HeatConduct")
            > elements.property(ids.vanadium, "HeatConduct"),
        "copper is not more thermally conductive than vanadium")
    assert(elements.property(ids.chromium, "Hardness")
            ~= elements.property(ids.manganese, "Hardness"),
        "chromium and manganese do not retain distinct hardness")
    assert(elements.property(ids.yttrium, "Weight")
            < elements.property(ids.zirconium, "Weight"),
        "yttrium and zirconium do not retain distinct density proxies")
    assert(elements.property(ids.niobium, "HighTemperature")
            < elements.property(ids.molybdenum, "HighTemperature"),
        "niobium and molybdenum do not retain distinct melting points")
    assert(elements.property(ids.technetium, "Properties")
            ~= elements.property(ids.ruthenium, "Properties"),
        "radioactive technetium and stable ruthenium have identical properties")
    assert(elements.property(ids.rhodium, "HeatConduct")
            > elements.property(ids.ruthenium, "HeatConduct"),
        "rhodium is not more thermally conductive than ruthenium")
    assert(elements.property(ids.silver, "HeatConduct")
            > elements.property(ids.palladium, "HeatConduct"),
        "silver is not more thermally conductive than palladium")
    assert(elements.property(ids.cadmium, "HighTemperature")
            < elements.property(ids.silver, "HighTemperature"),
        "cadmium and silver do not retain distinct melting points")
    assert(elements.property(ids.hafnium, "Weight")
            < elements.property(ids.tantalum, "Weight"),
        "hafnium and tantalum do not retain distinct density proxies")
    assert(elements.property(ids.tantalum, "HighTemperature")
            < elements.property(ids.rhenium, "HighTemperature"),
        "tantalum and rhenium do not retain distinct melting points")
    assert(elements.property(ids.tungsten, "HighTemperature")
            > elements.property(ids.rhenium, "HighTemperature"),
        "official tungsten and rhenium do not retain distinct melting points")
    assert(elements.property(ids.osmium, "Weight")
            > elements.property(ids.iridium, "Weight"),
        "osmium is not denser than the iridium gameplay proxy")
    assert(elements.property(ids.iridium, "Hardness")
            > elements.property(ids.platinum, "Hardness"),
        "iridium is not harder than official platinum")
    assert(elements.property(ids.gold, "HeatConduct")
            > elements.property(ids.iridium, "HeatConduct"),
        "official gold is not more thermally conductive than iridium")
    assert(elements.property(ids.mercury, "Falldown")
            ~= elements.property(ids.gold, "Falldown"),
        "liquid mercury and solid gold have identical movement properties")
    assert(elements.property(ids.lanthanum, "HighTemperature")
            > elements.property(ids.cerium, "HighTemperature"),
        "lanthanum and cerium do not retain distinct melting points")
    assert(elements.property(ids.promethium, "Properties")
            ~= elements.property(ids.neodymium, "Properties"),
        "radioactive promethium and stable neodymium have identical properties")
    assert(elements.property(ids.europium, "Hardness")
            < elements.property(ids.gadolinium, "Hardness"),
        "soft europium is not softer than gadolinium")
    assert(elements.property(ids.dysprosium, "HighTemperature")
            < elements.property(ids.holmium, "HighTemperature"),
        "dysprosium and holmium do not retain distinct melting points")
    assert(elements.property(ids.erbium, "HeatConduct")
            > elements.property(ids.thulium, "HeatConduct"),
        "erbium and thulium optical materials have identical conductivity")
    assert(elements.property(ids.ytterbium, "HighTemperature")
            < elements.property(ids.lutetium, "HighTemperature"),
        "low-melting ytterbium and refractory lutetium are not distinct")
    assert(elements.property(ids.lutetium, "Hardness")
            > elements.property(ids.lanthanum, "Hardness"),
        "dense lutetium is not harder than lanthanum")
    assert(elements.property(ids.thorium, "HighTemperature")
            > elements.property(ids.neptunium, "HighTemperature"),
        "thorium and neptunium do not retain distinct melting points")
    assert(elements.property(ids.lawrencium, "Weight")
            > elements.property(ids.actinium, "Weight"),
        "early and late actinides do not retain distinct density proxies")
    assert(elements.property(ids.californium, "HeatConduct")
            > elements.property(ids.nobelium, "HeatConduct"),
        "californium and nobelium do not retain distinct conductivity")
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

local function run_boron_group_reactions()
    configure(381)
    local boron = make(ids.boron, 120, 120, 293.15)
    local neutron = make(ids.neutron, 121, 120, 293.15)
    sim.partProperty(neutron, "vx", 0.0)
    sim.partProperty(neutron, "vy", 0.0)
    step()
    assert(sim.partProperty(boron, "type") == ids.lithium,
        "boron neutron capture did not produce the lithium proxy")
    assert(sim.partProperty(neutron, "type") == ids.he,
        "boron neutron capture did not produce the helium proxy")

    configure(391)
    local acid = make(ids.acid, 121, 120, 340.0)
    local indium = make(ids.indium, 120, 120, 340.0)
    step()
    assert(sim.partProperty(indium, "type") == ids.salt
            and sim.partProperty(acid, "type") == ids.hydrogen,
        "indium acid reaction did not produce generic salt and hydrogen")

    configure(401)
    local caustic = make(ids.caustic, 121, 120, 400.0)
    local aluminium = make(ids.aluminium, 120, 120, 400.0)
    step()
    assert(sim.partProperty(aluminium, "type") == ids.salt
            and sim.partProperty(caustic, "type") == ids.hydrogen,
        "aluminium caustic route did not produce generic salt and hydrogen")

    configure(411)
    local gallium = make(ids.gallium, 120, 120, 310.0)
    aluminium = make(ids.aluminium, 121, 120, 293.15)
    step()
    assert(sim.partProperty(aluminium, "type") == ids.scrap
            and sim.partProperty(aluminium, "ctype") == ids.aluminium,
        "liquid gallium did not embrittle aluminium into typed scrap")
    assert(sim.partExists(gallium),
        "gallium catalyst was incorrectly consumed by aluminium embrittlement")

    configure(421)
    boron = make(ids.boron, 120, 120, 1100.0)
    local oxygen = make(ids.oxygen, 121, 120, 1100.0)
    step()
    assert(sim.partProperty(boron, "type") == ids.glass
            and sim.partProperty(oxygen, "type") == ids.fire,
        "hot boron oxidation did not produce glassy oxide and finite fire proxies: boron="
            .. tostring(sim.partProperty(boron, "type"))
            .. " oxygen=" .. tostring(sim.partProperty(oxygen, "type")))

    configure(431)
    local molten = make(ids.lava, 120, 120, 2500.0)
    sim.partProperty(molten, "ctype", ids.indium)
    step()
    assert(sim.partProperty(molten, "type") == ids.fire
            and sim.partProperty(molten, "ctype") == ids.indium
            and sim.partProperty(molten, "life") == 60,
        "hot molten indium did not enter its finite vaporisation proxy")
end

local function run_carbon_group_reactions()
    configure(451)
    local old_diffusion = elements.property(ids.oxygen, "Diffusion")
    local old_advection = elements.property(ids.oxygen, "Advection")
    elements.property(ids.oxygen, "Diffusion", 0.0)
    elements.property(ids.oxygen, "Advection", 0.0)
    local diamond = make(ids.diamond, 120, 120, 1100.0)
    local oxygen = make(ids.oxygen, 121, 120, 1100.0)
    step()
    assert(sim.partProperty(diamond, "type") == ids.diamond,
        "diamond carbon mapping was made chemically reactive")
    assert(sim.partProperty(oxygen, "type") == ids.oxygen,
        "inert diamond unexpectedly consumed local oxygen")

    configure(461)
    local silicon = make(ids.silicon, 120, 120, 1000.0)
    oxygen = make(ids.oxygen, 121, 120, 1000.0)
    step()
    assert(sim.partProperty(silicon, "type") == ids.glass
            and sim.partProperty(oxygen, "type") == ids.fire,
        "hot silicon oxidation did not produce glass and finite fire proxies")
    elements.property(ids.oxygen, "Diffusion", old_diffusion)
    elements.property(ids.oxygen, "Advection", old_advection)

    configure(471)
    local germanium = make(ids.germanium, 120, 120, 300.0)
    for _ = 1, 80 do
        local electron = make(ids.electron, 121, 120, 300.0)
        sim.partProperty(electron, "vx", 0.0)
        sim.partProperty(electron, "vy", 0.0)
        step()
        if sim.partExists(electron) then sim.partKill(electron) end
        if sim.partProperty(germanium, "life") > 0 then break end
    end
    assert(sim.partExists(germanium)
            and sim.partProperty(germanium, "life") > 0,
        "germanium did not enter its finite local-discharge glow state")

    configure(481)
    germanium = make(ids.germanium, 120, 120, 360.0)
    local acid = make(ids.acid, 121, 120, 360.0)
    step()
    assert(sim.partProperty(germanium, "type") == ids.salt
            and sim.partProperty(acid, "type") == ids.hydrogen,
        "warm germanium acid route did not produce generic salt and hydrogen")

    configure(491)
    local tin = make(ids.tin, 120, 120, 250.0)
    step(120)
    assert(sim.partProperty(tin, "type") == ids.scrap
            and sim.partProperty(tin, "ctype") == ids.tin,
        "prolonged cold tin did not become recoverable typed brittle scrap")

    configure(501)
    local lead = make(ids.lead, 124, 120, 293.15)
    local neutron = make(ids.neutron, 120, 120, 293.15)
    sim.partProperty(neutron, "vx", 4.0)
    sim.partProperty(neutron, "vy", 0.0)
    for _ = 1, 8 do
        step()
        if not sim.partExists(neutron) then break end
    end
    assert(sim.partExists(lead) and not sim.partExists(neutron),
        "lead did not absorb an incident neutron through PROP_NEUTABSORB")

    configure(511)
    lead = make(ids.lead, 120, 120, 360.0)
    acid = make(ids.acid, 121, 120, 360.0)
    step()
    assert(sim.partProperty(lead, "type") == ids.salt
            and sim.partProperty(acid, "type") == ids.hydrogen,
        "warm lead acid route did not produce generic salt and hydrogen")

    configure(521)
    lead = make(ids.lead, 120, 120, 700.0)
    oxygen = make(ids.oxygen, 121, 120, 700.0)
    step()
    assert(sim.partProperty(lead, "type") == ids.salt
            and sim.partProperty(oxygen, "type") == ids.fire,
        "hot lead oxidation did not produce generic salt and finite fire")

    configure(531)
    local molten = make(ids.lava, 120, 120, 3000.0)
    sim.partProperty(molten, "ctype", ids.tin)
    step()
    assert(sim.partProperty(molten, "type") == ids.fire
            and sim.partProperty(molten, "ctype") == ids.tin
            and sim.partProperty(molten, "life") == 60,
        "hot molten tin did not enter its finite vaporisation proxy")
end

local function run_nitrogen_group_reactions()
    configure(551)
    sim.heatSim(true)
    local old_n_conduct = elements.property(ids.nitrogen, "HeatConduct")
    local old_ln2_conduct = elements.property(ids.liquid_nitrogen, "HeatConduct")
    local old_nice_conduct = elements.property(ids.nitrogen_ice, "HeatConduct")
    elements.property(ids.nitrogen, "HeatConduct", 250)
    elements.property(ids.liquid_nitrogen, "HeatConduct", 250)
    elements.property(ids.nitrogen_ice, "HeatConduct", 250)
    local nitrogen = make(ids.nitrogen, 120, 120, 76.0)
    step()
    assert(sim.partProperty(nitrogen, "type") == ids.liquid_nitrogen,
        "cold nitrogen gas did not condense into liquid nitrogen")
    sim.partProperty(nitrogen, "temp", 62.0)
    step()
    assert(sim.partProperty(nitrogen, "type") == ids.nitrogen_ice,
        "liquid nitrogen did not freeze into nitrogen ice")
    sim.partProperty(nitrogen, "temp", 64.0)
    step()
    assert(sim.partProperty(nitrogen, "type") == ids.liquid_nitrogen,
        "nitrogen ice did not thaw into liquid nitrogen")
    sim.partProperty(nitrogen, "temp", 78.0)
    step()
    assert(sim.partProperty(nitrogen, "type") == ids.nitrogen,
        "liquid nitrogen did not warm back into periodic nitrogen gas")
    elements.property(ids.nitrogen, "HeatConduct", old_n_conduct)
    elements.property(ids.liquid_nitrogen, "HeatConduct", old_ln2_conduct)
    elements.property(ids.nitrogen_ice, "HeatConduct", old_nice_conduct)

    configure(561)
    local old_n_diffusion = elements.property(ids.nitrogen, "Diffusion")
    local old_n_advection = elements.property(ids.nitrogen, "Advection")
    elements.property(ids.nitrogen, "Diffusion", 0.0)
    elements.property(ids.nitrogen, "Advection", 0.0)
    nitrogen = make(ids.nitrogen, 120, 120, 300.0)
    for _ = 1, 80 do
        local electron = make(ids.electron, 121, 120, 300.0)
        sim.partProperty(electron, "vx", 0.0)
        sim.partProperty(electron, "vy", 0.0)
        step()
        if sim.partExists(electron) then sim.partKill(electron) end
        if sim.partExists(nitrogen)
                and sim.partProperty(nitrogen, "life") > 0 then break end
    end
    elements.property(ids.nitrogen, "Diffusion", old_n_diffusion)
    elements.property(ids.nitrogen, "Advection", old_n_advection)
    assert(sim.partExists(nitrogen)
            and sim.partProperty(nitrogen, "life") > 0,
        "nitrogen did not enter its finite local-discharge glow state")

    configure(571)
    local old_o_diffusion = elements.property(ids.oxygen, "Diffusion")
    local old_o_advection = elements.property(ids.oxygen, "Advection")
    elements.property(ids.oxygen, "Diffusion", 0.0)
    elements.property(ids.oxygen, "Advection", 0.0)
    local phosphorus = make(ids.phosphorus, 120, 120, 330.0)
    local oxygen = make(ids.oxygen, 121, 120, 330.0)
    step(2)
    assert(sim.partProperty(phosphorus, "type") == ids.dust
            and sim.partProperty(oxygen, "type") == ids.fire,
        "warm phosphorus oxygen route did not produce dust and finite fire")
    elements.property(ids.oxygen, "Diffusion", old_o_diffusion)
    elements.property(ids.oxygen, "Advection", old_o_advection)

    configure(581)
    local arsenic = make(ids.arsenic, 120, 120, 380.0)
    local acid = make(ids.acid, 121, 120, 380.0)
    step()
    assert(sim.partProperty(arsenic, "type") == ids.salt
            and sim.partProperty(acid, "type") == ids.hydrogen,
        "warm arsenic acid route did not produce generic salt and hydrogen")

    configure(591)
    arsenic = make(ids.arsenic, 120, 120, 887.0)
    step()
    assert(sim.partProperty(arsenic, "type") == ids.fire
            and sim.partProperty(arsenic, "ctype") == ids.arsenic
            and sim.partProperty(arsenic, "life") == 60,
        "hot arsenic did not enter its finite sublimation proxy")

    configure(601)
    local antimony = make(ids.antimony, 120, 120, 350.0)
    acid = make(ids.acid, 121, 120, 350.0)
    step()
    assert(sim.partProperty(antimony, "type") == ids.salt
            and sim.partProperty(acid, "type") == ids.hydrogen,
        "warm antimony acid route did not produce generic salt and hydrogen")

    configure(611)
    old_o_diffusion = elements.property(ids.oxygen, "Diffusion")
    old_o_advection = elements.property(ids.oxygen, "Advection")
    elements.property(ids.oxygen, "Diffusion", 0.0)
    elements.property(ids.oxygen, "Advection", 0.0)
    local bismuth = make(ids.bismuth, 120, 120, 720.0)
    oxygen = make(ids.oxygen, 121, 120, 720.0)
    step()
    assert(sim.partProperty(bismuth, "type") == ids.salt
            and sim.partProperty(oxygen, "type") == ids.fire,
        "hot bismuth oxygen route did not produce generic salt and finite fire")
    elements.property(ids.oxygen, "Diffusion", old_o_diffusion)
    elements.property(ids.oxygen, "Advection", old_o_advection)

    configure(621)
    local molten = make(ids.lava, 120, 120, 2000.0)
    sim.partProperty(molten, "ctype", ids.antimony)
    step()
    assert(sim.partProperty(molten, "type") == ids.fire
            and sim.partProperty(molten, "ctype") == ids.antimony
            and sim.partProperty(molten, "life") == 60,
        "hot molten antimony did not enter its finite vaporisation proxy")

    configure(631)
    molten = make(ids.lava, 120, 120, 1900.0)
    sim.partProperty(molten, "ctype", ids.bismuth)
    step()
    assert(sim.partProperty(molten, "type") == ids.fire
            and sim.partProperty(molten, "ctype") == ids.bismuth
            and sim.partProperty(molten, "life") == 60,
        "hot molten bismuth did not enter its finite vaporisation proxy")
end

local function run_oxygen_group_reactions()
    configure(651)
    sim.heatSim(true)
    local old_o_conduct = elements.property(ids.oxygen, "HeatConduct")
    local old_lo2_conduct = elements.property(ids.liquid_oxygen, "HeatConduct")
    elements.property(ids.oxygen, "HeatConduct", 250)
    elements.property(ids.liquid_oxygen, "HeatConduct", 250)
    local oxygen = make(ids.oxygen, 120, 120, 89.0)
    step()
    assert(sim.partProperty(oxygen, "type") == ids.liquid_oxygen,
        "cold official oxygen did not condense into liquid oxygen")
    sim.partProperty(oxygen, "temp", 91.0)
    step()
    assert(sim.partProperty(oxygen, "type") == ids.oxygen,
        "liquid oxygen did not warm back into official oxygen gas")
    elements.property(ids.oxygen, "HeatConduct", old_o_conduct)
    elements.property(ids.liquid_oxygen, "HeatConduct", old_lo2_conduct)

    configure(661)
    local polonium = make(ids.polonium, 120, 120, 400.0)
    sim.partProperty(polonium, "tmp2", 10)
    step()
    assert(sim.partProperty(polonium, "type") == ids.plutonium,
        "official polonium tmp2 route did not produce plutonium")

    configure(671)
    local old_o_diffusion = elements.property(ids.oxygen, "Diffusion")
    local old_o_advection = elements.property(ids.oxygen, "Advection")
    elements.property(ids.oxygen, "Diffusion", 0.0)
    elements.property(ids.oxygen, "Advection", 0.0)
    local sulfur = make(ids.sulfur, 120, 120, 400.0)
    oxygen = make(ids.oxygen, 121, 120, 400.0)
    step(2)
    assert(sim.partProperty(sulfur, "type") == ids.smoke
            and sim.partProperty(sulfur, "life") > 0
            and sim.partProperty(oxygen, "type") == ids.fire,
        "warm sulfur oxygen route did not produce finite smoke and fire")
    elements.property(ids.oxygen, "Diffusion", old_o_diffusion)
    elements.property(ids.oxygen, "Advection", old_o_advection)

    configure(681)
    sim.heatSim(true)
    local old_s_conduct = elements.property(ids.sulfur, "HeatConduct")
    elements.property(ids.sulfur, "HeatConduct", 250)
    sulfur = make(ids.sulfur, 120, 120, 389.0)
    step()
    assert(sim.partProperty(sulfur, "type") == ids.lava
            and sim.partProperty(sulfur, "ctype") == ids.sulfur,
        "sulfur did not melt into typed LAVA")
    elements.property(ids.sulfur, "HeatConduct", old_s_conduct)
    sim.partProperty(sulfur, "temp", 720.0)
    step()
    assert(sim.partProperty(sulfur, "type") == ids.fire
            and sim.partProperty(sulfur, "ctype") == ids.sulfur
            and sim.partProperty(sulfur, "life") == 60,
        "hot molten sulfur did not enter its finite vaporisation proxy")

    configure(691)
    local old_se_diffusion = elements.property(ids.selenium, "Diffusion")
    local old_se_advection = elements.property(ids.selenium, "Advection")
    elements.property(ids.selenium, "Diffusion", 0.0)
    elements.property(ids.selenium, "Advection", 0.0)
    local selenium = make(ids.selenium, 120, 120, 300.0)
    for _ = 1, 80 do
        local photon = make(ids.photon, 121, 120, 300.0)
        sim.partProperty(photon, "vx", 0.0)
        sim.partProperty(photon, "vy", 0.0)
        step()
        if sim.partExists(photon) then sim.partKill(photon) end
        if sim.partExists(selenium)
                and sim.partProperty(selenium, "life") > 0 then break end
    end
    elements.property(ids.selenium, "Diffusion", old_se_diffusion)
    elements.property(ids.selenium, "Advection", old_se_advection)
    assert(sim.partExists(selenium)
            and sim.partProperty(selenium, "life") > 0,
        "selenium did not enter its finite local-photon glow state")

    configure(701)
    old_o_diffusion = elements.property(ids.oxygen, "Diffusion")
    old_o_advection = elements.property(ids.oxygen, "Advection")
    elements.property(ids.oxygen, "Diffusion", 0.0)
    elements.property(ids.oxygen, "Advection", 0.0)
    selenium = make(ids.selenium, 120, 120, 630.0)
    oxygen = make(ids.oxygen, 121, 120, 630.0)
    step(2)
    assert(sim.partProperty(selenium, "type") == ids.dust
            and sim.partProperty(oxygen, "type") == ids.fire,
        "hot selenium oxygen route did not produce dust and finite fire")
    elements.property(ids.oxygen, "Diffusion", old_o_diffusion)
    elements.property(ids.oxygen, "Advection", old_o_advection)

    configure(711)
    old_o_diffusion = elements.property(ids.oxygen, "Diffusion")
    old_o_advection = elements.property(ids.oxygen, "Advection")
    elements.property(ids.oxygen, "Diffusion", 0.0)
    elements.property(ids.oxygen, "Advection", 0.0)
    local tellurium = make(ids.tellurium, 120, 120, 790.0)
    oxygen = make(ids.oxygen, 121, 120, 790.0)
    step(2)
    assert(sim.partProperty(tellurium, "type") == ids.glass
            and sim.partProperty(oxygen, "type") == ids.fire,
        "hot tellurium oxygen route did not produce glass and finite fire")
    elements.property(ids.oxygen, "Diffusion", old_o_diffusion)
    elements.property(ids.oxygen, "Advection", old_o_advection)

    configure(721)
    local molten = make(ids.lava, 120, 120, 960.0)
    sim.partProperty(molten, "ctype", ids.selenium)
    step()
    assert(sim.partProperty(molten, "type") == ids.fire
            and sim.partProperty(molten, "ctype") == ids.selenium
            and sim.partProperty(molten, "life") == 60,
        "hot molten selenium did not enter its finite vaporisation proxy")

    configure(731)
    molten = make(ids.lava, 120, 120, 1270.0)
    sim.partProperty(molten, "ctype", ids.tellurium)
    step()
    assert(sim.partProperty(molten, "type") == ids.fire
            and sim.partProperty(molten, "ctype") == ids.tellurium
            and sim.partProperty(molten, "life") == 60,
        "hot molten tellurium did not enter its finite vaporisation proxy")
end

local function run_halogen_group_reactions()
    configure(751)
    local chlorine = make(ids.chlorine, 120, 120, 450.0)
    local hydrogen = make(ids.hydrogen, 121, 120, 450.0)
    step()
    assert(sim.partProperty(chlorine, "type") == ids.acid
            and sim.partProperty(hydrogen, "type") == ids.acid,
        "reused chlorine chemistry no longer converts hot hydrogen into acid")

    configure(761)
    local fluorine = make(ids.fluorine, 120, 120, 293.15)
    local water = make(ids.water, 121, 120, 293.15)
    step()
    assert(sim.partProperty(fluorine, "type") == ids.acid
            and sim.partProperty(water, "type") == ids.acid,
        "fluorine water route did not produce two acid proxy particles")

    configure(771)
    fluorine = make(ids.fluorine, 120, 120, 293.15)
    local metal = make(ids.metal, 121, 120, 293.15)
    step()
    assert(sim.partProperty(fluorine, "type") == ids.salt
            and sim.partProperty(metal, "type") == ids.salt,
        "fluorine metal route did not produce two halide salt proxies")

    configure(781)
    chlorine = make(ids.chlorine, 120, 120, 293.15)
    local pathogen = make(ids.pathogen, 121, 120, 293.15)
    step()
    assert(sim.partProperty(chlorine, "type") == ids.salt
            and sim.partProperty(pathogen, "type") == ids.dust,
        "chlorine disinfection did not produce salt and inert dust")

    configure(791)
    local bromine = make(ids.bromine, 120, 120, 380.0)
    hydrogen = make(ids.hydrogen, 121, 120, 380.0)
    step()
    assert(sim.partProperty(bromine, "type") == ids.acid
            and sim.partProperty(hydrogen, "type") == ids.acid,
        "warm bromine hydrogen route did not produce acid")

    configure(801)
    bromine = make(ids.bromine, 120, 120, 333.0)
    step()
    assert(sim.partProperty(bromine, "type") == ids.smoke
            and sim.partProperty(bromine, "ctype") == ids.bromine
            and sim.partProperty(bromine, "life") == 60,
        "warm bromine did not enter its finite coloured vapour proxy")

    configure(811)
    sim.heatSim(true)
    local old_i_conduct = elements.property(ids.iodine, "HeatConduct")
    elements.property(ids.iodine, "HeatConduct", 250)
    local iodine = make(ids.iodine, 120, 120, 387.0)
    step()
    assert(sim.partProperty(iodine, "type") == ids.lava
            and sim.partProperty(iodine, "ctype") == ids.iodine,
        "iodine did not melt into typed LAVA")
    elements.property(ids.iodine, "HeatConduct", old_i_conduct)
    sim.partProperty(iodine, "temp", 458.0)
    step()
    assert(sim.partProperty(iodine, "type") == ids.smoke
            and sim.partProperty(iodine, "ctype") == ids.iodine
            and sim.partProperty(iodine, "life") == 60,
        "hot molten iodine did not enter its finite purple vapour proxy")

    configure(821)
    local astatine = make(ids.astatine, 120, 120, 293.15)
    assert(sim.partProperty(astatine, "tmp") >= 240
            and sim.partProperty(astatine, "tmp") <= 480,
        "astatine creation did not initialize its bounded lifetime")
    sim.partProperty(astatine, "tmp", 1)
    step()
    assert(sim.partProperty(astatine, "type") == ids.polonium,
        "astatine did not decay to polonium")
    assert(count_type(ids.photon) == 1,
        "one astatine decay did not emit exactly one finite photon")

    configure(831)
    local molten = make(ids.lava, 120, 120, 600.0)
    sim.partProperty(molten, "ctype", ids.astatine)
    sim.partProperty(molten, "tmp", 1)
    step()
    assert(sim.partProperty(molten, "type") == ids.polonium,
        "molten astatine ctype did not retain radioactive decay")
    assert(count_type(ids.photon) == 1,
        "one molten astatine decay did not emit exactly one finite photon")

    configure(841)
    local tennessine = make(ids.tennessine, 120, 120, 293.15)
    assert(sim.partProperty(tennessine, "tmp") >= 35
            and sim.partProperty(tennessine, "tmp") <= 75,
        "tennessine creation did not initialize its bounded lifetime")
    sim.partProperty(tennessine, "tmp", 1)
    step()
    assert(sim.partProperty(tennessine, "type") == ids.moscovium,
        "tennessine did not enter the compressed moscovium decay proxy")
    assert(count_type(ids.photon) == 1,
        "first tennessine decay stage did not emit exactly one finite photon")
    sim.partProperty(tennessine, "tmp", 1)
    step()
    assert(sim.partProperty(tennessine, "type") == ids.nihonium,
        "moscovium produced by tennessine did not decay to nihonium")
    sim.partProperty(tennessine, "tmp", 1)
    step()
    assert(sim.partProperty(tennessine, "type") == ids.polonium,
        "nihonium produced by tennessine did not decay to polonium")
    assert(count_type(ids.photon) == 3,
        "three-stage tennessine decay did not emit exactly three finite photons")
end

local function run_first_transition_reactions()
    configure(871)
    local scandium = make(ids.scandium, 120, 120, 330.0)
    local acid = make(ids.acid, 121, 120, 330.0)
    step()
    assert(sim.partProperty(scandium, "type") == ids.salt
            and sim.partProperty(acid, "type") == ids.hydrogen,
        "warm scandium acid route did not produce generic salt and hydrogen")

    configure(881)
    scandium = make(ids.scandium, 120, 120, 300.0)
    for _ = 1, 80 do
        local electron = make(ids.electron, 121, 120, 300.0)
        sim.partProperty(electron, "vx", 0.0)
        sim.partProperty(electron, "vy", 0.0)
        step()
        if sim.partExists(electron) then sim.partKill(electron) end
        if sim.partExists(scandium)
                and sim.partProperty(scandium, "life") > 0 then break end
    end
    assert(sim.partExists(scandium)
            and sim.partProperty(scandium, "life") > 0,
        "scandium did not enter its finite discharge-lamp glow state")
    assert(count_type(ids.photon) >= 1,
        "scandium discharge did not emit a finite photon")

    configure(891)
    local old_o_diffusion = elements.property(ids.oxygen, "Diffusion")
    local old_o_advection = elements.property(ids.oxygen, "Advection")
    elements.property(ids.oxygen, "Diffusion", 0.0)
    elements.property(ids.oxygen, "Advection", 0.0)
    scandium = make(ids.scandium, 120, 120, 760.0)
    local oxygen = make(ids.oxygen, 121, 120, 760.0)
    step()
    assert(sim.partProperty(scandium, "type") == ids.scrap
            and sim.partProperty(scandium, "ctype") == ids.scandium
            and sim.partProperty(oxygen, "type") == ids.fire,
        "hot scandium oxygen route did not produce typed scrap and finite fire")
    elements.property(ids.oxygen, "Diffusion", old_o_diffusion)
    elements.property(ids.oxygen, "Advection", old_o_advection)

    configure(901)
    local vanadium = make(ids.vanadium, 120, 120, 1900.0)
    local steel = make(ids.lava, 121, 120, 1900.0)
    sim.partProperty(steel, "ctype", ids.steel)
    step()
    assert(sim.partProperty(vanadium, "type") == ids.lava
            and sim.partProperty(vanadium, "ctype") == ids.tool_steel
            and sim.partProperty(steel, "type") == ids.lava
            and sim.partProperty(steel, "ctype") == ids.tool_steel,
        "vanadium and molten steel did not form two typed tool-steel proxies")

    configure(911)
    local manganese = make(ids.manganese, 120, 120, 1300.0)
    local iron = make(ids.lava, 121, 120, 1700.0)
    sim.partProperty(iron, "ctype", ids.iron)
    oxygen = make(ids.oxygen, 120, 121, 1300.0)
    step()
    assert(sim.partProperty(manganese, "type") == ids.lava
            and sim.partProperty(manganese, "ctype") == ids.steel
            and sim.partProperty(iron, "type") == ids.lava
            and sim.partProperty(iron, "ctype") == ids.steel
            and sim.partProperty(oxygen, "type") == ids.slag,
        "manganese did not deoxidize molten iron into steel and slag proxies")

    configure(921)
    local molten = make(ids.lava, 120, 120, 2340.0)
    sim.partProperty(molten, "ctype", ids.manganese)
    step()
    assert(sim.partProperty(molten, "type") == ids.fire
            and sim.partProperty(molten, "ctype") == ids.manganese
            and sim.partProperty(molten, "life") == 60,
        "hot molten manganese did not enter its finite vaporisation proxy")
end

local function run_second_transition_reactions()
    configure(941)
    local yttrium = make(ids.yttrium, 120, 120, 300.0)
    for _ = 1, 200 do
        local electron = make(ids.electron, 121, 120, 300.0)
        sim.partProperty(electron, "vx", 0.0)
        sim.partProperty(electron, "vy", 0.0)
        step()
        if sim.partExists(electron) then sim.partKill(electron) end
        if count_type(ids.photon) >= 1 then break end
    end
    assert(sim.partExists(yttrium)
            and sim.partProperty(yttrium, "life") > 0,
        "yttrium did not enter its finite green discharge-glow state")
    assert(count_type(ids.photon) >= 1,
        "yttrium discharge did not emit a finite photon")

    configure(951)
    local old_vapor_advection = elements.property(ids.water_vapor, "Advection")
    local old_vapor_diffusion = elements.property(ids.water_vapor, "Diffusion")
    local old_vapor_gravity = elements.property(ids.water_vapor, "Gravity")
    elements.property(ids.water_vapor, "Advection", 0.0)
    elements.property(ids.water_vapor, "Diffusion", 0.0)
    elements.property(ids.water_vapor, "Gravity", 0.0)
    local water = make(ids.water_vapor, 121, 120, 1100.0)
    local zirconium = make(ids.zirconium, 120, 120, 1100.0)
    step()
    elements.property(ids.water_vapor, "Advection", old_vapor_advection)
    elements.property(ids.water_vapor, "Diffusion", old_vapor_diffusion)
    elements.property(ids.water_vapor, "Gravity", old_vapor_gravity)
    assert(sim.partProperty(zirconium, "type") == ids.scrap
            and sim.partProperty(zirconium, "ctype") == ids.zirconium
            and sim.partProperty(water, "type") == ids.hydrogen,
        "hot zirconium and steam did not produce typed scrap and hydrogen: zr="
            .. tostring(sim.partProperty(zirconium, "type"))
            .. "/" .. tostring(sim.partProperty(zirconium, "ctype"))
            .. " steam=" .. tostring(sim.partProperty(water, "type")))
    assert(count_type(ids.fire) >= 1,
        "zirconium steam oxidation did not emit bounded bright fire")

    configure(961)
    local old_acid_advection = elements.property(ids.acid, "Advection")
    local old_acid_gravity = elements.property(ids.acid, "Gravity")
    local old_acid_falldown = elements.property(ids.acid, "Falldown")
    elements.property(ids.acid, "Advection", 0.0)
    elements.property(ids.acid, "Gravity", 0.0)
    elements.property(ids.acid, "Falldown", 0)
    local niobium = make(ids.niobium, 120, 120, 560.0)
    local acid = make(ids.acid, 121, 120, 560.0)
    step()
    elements.property(ids.acid, "Advection", old_acid_advection)
    elements.property(ids.acid, "Gravity", old_acid_gravity)
    elements.property(ids.acid, "Falldown", old_acid_falldown)
    assert(sim.partProperty(niobium, "type") == ids.salt
            and sim.partProperty(acid, "type") == ids.hydrogen,
        "hot niobium acid route did not produce generic salt and hydrogen")

    configure(971)
    local technetium = make(ids.technetium, 120, 120, 300.0)
    assert(sim.partProperty(technetium, "tmp") >= 600
            and sim.partProperty(technetium, "tmp") <= 1200,
        "technetium creation did not initialize its bounded lifetime")
    sim.partProperty(technetium, "tmp", 1)
    step()
    assert(sim.partProperty(technetium, "type") == ids.ruthenium,
        "technetium did not decay to ruthenium")
    assert(count_type(ids.photon) == 1,
        "one technetium decay did not emit exactly one finite photon")

    local function run_platinum_catalyst(catalyst_type, temperature, seed)
        configure(seed)
        local old_h_diffusion = elements.property(ids.hydrogen, "Diffusion")
        local old_h_advection = elements.property(ids.hydrogen, "Advection")
        local old_o_diffusion = elements.property(ids.oxygen, "Diffusion")
        local old_o_advection = elements.property(ids.oxygen, "Advection")
        elements.property(ids.hydrogen, "Diffusion", 0.0)
        elements.property(ids.hydrogen, "Advection", 0.0)
        elements.property(ids.oxygen, "Diffusion", 0.0)
        elements.property(ids.oxygen, "Advection", 0.0)
        local catalyst = make(catalyst_type, 120, 120, temperature)
        local hydrogen = make(ids.hydrogen, 121, 120, temperature)
        local oxygen = make(ids.oxygen, 120, 121, temperature)
        for _ = 1, 80 do
            step()
            if sim.partProperty(hydrogen, "type") == ids.water
                    and sim.partProperty(oxygen, "type") == ids.water then break end
        end
        elements.property(ids.hydrogen, "Diffusion", old_h_diffusion)
        elements.property(ids.hydrogen, "Advection", old_h_advection)
        elements.property(ids.oxygen, "Diffusion", old_o_diffusion)
        elements.property(ids.oxygen, "Advection", old_o_advection)
        assert(sim.partProperty(catalyst, "type") == catalyst_type
                and sim.partProperty(hydrogen, "type") == ids.water
                and sim.partProperty(oxygen, "type") == ids.water,
            "platinum-group catalyst did not convert hydrogen and oxygen into water")
    end
    run_platinum_catalyst(ids.ruthenium, 460.0, 981)
    run_platinum_catalyst(ids.rhodium, 350.0, 991)

    configure(1001)
    local old_h_diffusion = elements.property(ids.hydrogen, "Diffusion")
    local old_h_advection = elements.property(ids.hydrogen, "Advection")
    elements.property(ids.hydrogen, "Diffusion", 0.0)
    elements.property(ids.hydrogen, "Advection", 0.0)
    local palladium = make(ids.palladium, 120, 120, 300.0)
    make(ids.hydrogen, 121, 120, 300.0)
    make(ids.hydrogen, 119, 120, 300.0)
    make(ids.hydrogen, 120, 121, 300.0)
    make(ids.hydrogen, 120, 119, 300.0)
    step(8)
    assert(sim.partProperty(palladium, "tmp") == 4
            and count_type(ids.hydrogen) == 0,
        "cool palladium did not absorb four bounded local hydrogen particles")
    sim.partProperty(palladium, "temp", 550.0)
    step(4)
    assert(sim.partProperty(palladium, "tmp") == 0
            and count_type(ids.hydrogen) == 4,
        "hot palladium did not release four stored hydrogen particles")
    elements.property(ids.hydrogen, "Diffusion", old_h_diffusion)
    elements.property(ids.hydrogen, "Advection", old_h_advection)

    configure(1011)
    local old_sulfur_gravity = elements.property(ids.sulfur, "Gravity")
    local old_sulfur_falldown = elements.property(ids.sulfur, "Falldown")
    elements.property(ids.sulfur, "Gravity", 0.0)
    elements.property(ids.sulfur, "Falldown", 0)
    local silver = make(ids.silver, 120, 120, 360.0)
    local sulfur = make(ids.sulfur, 121, 120, 360.0)
    step()
    elements.property(ids.sulfur, "Gravity", old_sulfur_gravity)
    elements.property(ids.sulfur, "Falldown", old_sulfur_falldown)
    assert(sim.partProperty(silver, "type") == ids.scrap
            and sim.partProperty(silver, "ctype") == ids.silver
            and sim.partProperty(sulfur, "type") == ids.dust,
        "warm silver and sulfur did not form typed dark tarnish proxies")

    configure(1021)
    local old_o_diffusion = elements.property(ids.oxygen, "Diffusion")
    local old_o_advection = elements.property(ids.oxygen, "Advection")
    elements.property(ids.oxygen, "Diffusion", 0.0)
    elements.property(ids.oxygen, "Advection", 0.0)
    local cadmium = make(ids.cadmium, 120, 120, 560.0)
    local oxygen = make(ids.oxygen, 121, 120, 560.0)
    step()
    elements.property(ids.oxygen, "Diffusion", old_o_diffusion)
    elements.property(ids.oxygen, "Advection", old_o_advection)
    assert(sim.partProperty(cadmium, "type") == ids.scrap
            and sim.partProperty(cadmium, "ctype") == ids.cadmium
            and sim.partProperty(oxygen, "type") == ids.fire,
        "hot cadmium oxygen route did not produce typed scrap and finite fire")

    configure(1031)
    local molten = make(ids.lava, 120, 120, 1050.0)
    sim.partProperty(molten, "ctype", ids.cadmium)
    step()
    assert(sim.partProperty(molten, "type") == ids.fire
            and sim.partProperty(molten, "ctype") == ids.cadmium
            and sim.partProperty(molten, "life") == 60,
        "hot molten cadmium did not enter its finite vaporisation proxy")
end

local function run_third_transition_reactions()
    configure(1051)
    local hafnium = make(ids.hafnium, 120, 120, 300.0)
    local neutron = make(ids.neutron, 121, 120, 300.0)
    sim.partProperty(neutron, "vx", 0.0)
    sim.partProperty(neutron, "vy", 0.0)
    step()
    assert(sim.partExists(hafnium)
            and sim.partProperty(hafnium, "type") == ids.hafnium
            and sim.partProperty(hafnium, "tmp") == 1
            and sim.partProperty(hafnium, "temp") > 300.0
            and not sim.partExists(neutron),
        "hafnium did not absorb one local neutron with bounded heating")

    configure(1061)
    local old_acid_advection = elements.property(ids.acid, "Advection")
    local old_acid_gravity = elements.property(ids.acid, "Gravity")
    local old_acid_falldown = elements.property(ids.acid, "Falldown")
    elements.property(ids.acid, "Advection", 0.0)
    elements.property(ids.acid, "Gravity", 0.0)
    elements.property(ids.acid, "Falldown", 0)
    hafnium = make(ids.hafnium, 120, 120, 710.0)
    local acid = make(ids.acid, 121, 120, 710.0)
    step()
    elements.property(ids.acid, "Advection", old_acid_advection)
    elements.property(ids.acid, "Gravity", old_acid_gravity)
    elements.property(ids.acid, "Falldown", old_acid_falldown)
    assert(sim.partProperty(hafnium, "type") == ids.salt
            and sim.partProperty(acid, "type") == ids.hydrogen,
        "hot hafnium acid route did not produce generic salt and hydrogen")

    configure(1071)
    local old_o_diffusion = elements.property(ids.oxygen, "Diffusion")
    local old_o_advection = elements.property(ids.oxygen, "Advection")
    elements.property(ids.oxygen, "Diffusion", 0.0)
    elements.property(ids.oxygen, "Advection", 0.0)
    local tantalum = make(ids.tantalum, 120, 120, 800.0)
    local oxygen = make(ids.oxygen, 121, 120, 800.0)
    step()
    elements.property(ids.oxygen, "Diffusion", old_o_diffusion)
    elements.property(ids.oxygen, "Advection", old_o_advection)
    assert(sim.partProperty(tantalum, "type") == ids.tantalum
            and sim.partProperty(tantalum, "life") > 0
            and sim.partProperty(oxygen, "type") == ids.glass,
        "hot tantalum did not form its temporary glassy passivation layer")
    old_acid_advection = elements.property(ids.acid, "Advection")
    old_acid_gravity = elements.property(ids.acid, "Gravity")
    old_acid_falldown = elements.property(ids.acid, "Falldown")
    elements.property(ids.acid, "Advection", 0.0)
    elements.property(ids.acid, "Gravity", 0.0)
    elements.property(ids.acid, "Falldown", 0)
    acid = make(ids.acid, 119, 120, 1300.0)
    step()
    assert(sim.partProperty(tantalum, "type") == ids.tantalum
            and sim.partProperty(acid, "type") == ids.acid,
        "passive tantalum did not temporarily resist hot acid")
    sim.partProperty(tantalum, "life", 0)
    step()
    elements.property(ids.acid, "Advection", old_acid_advection)
    elements.property(ids.acid, "Gravity", old_acid_gravity)
    elements.property(ids.acid, "Falldown", old_acid_falldown)
    assert(sim.partProperty(tantalum, "type") == ids.salt
            and sim.partProperty(acid, "type") == ids.hydrogen,
        "unpassivated hot tantalum did not re-enter the acid route")

    configure(1081)
    local rhenium = make(ids.rhenium, 120, 120, 2100.0)
    local nickel = make(ids.lava, 121, 120, 2100.0)
    sim.partProperty(nickel, "ctype", ids.nickel)
    step()
    assert(sim.partProperty(rhenium, "type") == ids.lava
            and sim.partProperty(rhenium, "ctype") == ids.tool_steel
            and sim.partProperty(nickel, "type") == ids.lava
            and sim.partProperty(nickel, "ctype") == ids.tool_steel,
        "hot rhenium and nickel did not form two superalloy proxies")

    configure(1091)
    old_o_diffusion = elements.property(ids.oxygen, "Diffusion")
    old_o_advection = elements.property(ids.oxygen, "Advection")
    elements.property(ids.oxygen, "Diffusion", 0.0)
    elements.property(ids.oxygen, "Advection", 0.0)
    local osmium = make(ids.osmium, 120, 120, 500.0)
    oxygen = make(ids.oxygen, 121, 120, 500.0)
    step()
    elements.property(ids.oxygen, "Diffusion", old_o_diffusion)
    elements.property(ids.oxygen, "Advection", old_o_advection)
    assert(sim.partProperty(osmium, "type") == ids.smoke
            and sim.partProperty(osmium, "ctype") == ids.osmium
            and sim.partProperty(osmium, "life") == 80
            and sim.partProperty(oxygen, "type") == ids.caustic,
        "warm osmium oxidation did not create finite typed vapour and caustic proxy")

    configure(1101)
    local old_peroxide_advection = elements.property(ids.peroxide, "Advection")
    local old_peroxide_gravity = elements.property(ids.peroxide, "Gravity")
    local old_peroxide_falldown = elements.property(ids.peroxide, "Falldown")
    elements.property(ids.peroxide, "Advection", 0.0)
    elements.property(ids.peroxide, "Gravity", 0.0)
    elements.property(ids.peroxide, "Falldown", 0)
    local iridium = make(ids.iridium, 120, 120, 360.0)
    local first_peroxide = make(ids.peroxide, 121, 120, 360.0)
    local second_peroxide = make(ids.peroxide, 120, 121, 360.0)
    step()
    elements.property(ids.peroxide, "Advection", old_peroxide_advection)
    elements.property(ids.peroxide, "Gravity", old_peroxide_gravity)
    elements.property(ids.peroxide, "Falldown", old_peroxide_falldown)
    assert(sim.partProperty(iridium, "type") == ids.iridium
            and sim.partProperty(first_peroxide, "type") == ids.water
            and sim.partProperty(second_peroxide, "type") == ids.water
            and count_type(ids.oxygen) == 1,
        "iridium did not catalyse two peroxide particles into water and oxygen")

    configure(1111)
    old_o_diffusion = elements.property(ids.oxygen, "Diffusion")
    old_o_advection = elements.property(ids.oxygen, "Advection")
    elements.property(ids.oxygen, "Diffusion", 0.0)
    elements.property(ids.oxygen, "Advection", 0.0)
    rhenium = make(ids.rhenium, 120, 120, 1100.0)
    oxygen = make(ids.oxygen, 121, 120, 1100.0)
    step()
    elements.property(ids.oxygen, "Diffusion", old_o_diffusion)
    elements.property(ids.oxygen, "Advection", old_o_advection)
    assert(sim.partProperty(rhenium, "type") == ids.scrap
            and sim.partProperty(rhenium, "ctype") == ids.rhenium
            and sim.partProperty(oxygen, "type") == ids.fire,
        "hot rhenium oxygen route did not produce typed scrap and finite fire")

    configure(1121)
    local molten = make(ids.lava, 120, 120, 4710.0)
    sim.partProperty(molten, "ctype", ids.iridium)
    step()
    assert(sim.partProperty(molten, "type") == ids.fire
            and sim.partProperty(molten, "ctype") == ids.iridium
            and sim.partProperty(molten, "life") == 60,
        "hot molten iridium did not enter its finite vaporisation proxy")
end

local function run_lanthanide_reactions()
    configure(1141)
    local old_h_diffusion = elements.property(ids.hydrogen, "Diffusion")
    local old_h_advection = elements.property(ids.hydrogen, "Advection")
    elements.property(ids.hydrogen, "Diffusion", 0.0)
    elements.property(ids.hydrogen, "Advection", 0.0)
    local lanthanum = make(ids.lanthanum, 120, 120, 500.0)
    make(ids.hydrogen, 121, 120, 500.0)
    make(ids.hydrogen, 119, 120, 500.0)
    make(ids.hydrogen, 120, 121, 500.0)
    make(ids.hydrogen, 120, 119, 500.0)
    step(8)
    assert(sim.partProperty(lanthanum, "tmp") == 4
            and count_type(ids.hydrogen) == 0,
        "warm lanthanum did not absorb four bounded local hydrogen particles")
    sim.partProperty(lanthanum, "temp", 950.0)
    step(4)
    assert(sim.partProperty(lanthanum, "tmp") == 0
            and count_type(ids.hydrogen) == 4,
        "hot lanthanum did not release four stored hydrogen particles")
    elements.property(ids.hydrogen, "Diffusion", old_h_diffusion)
    elements.property(ids.hydrogen, "Advection", old_h_advection)

    configure(1151)
    local old_o_diffusion = elements.property(ids.oxygen, "Diffusion")
    local old_o_advection = elements.property(ids.oxygen, "Advection")
    elements.property(ids.oxygen, "Diffusion", 0.0)
    elements.property(ids.oxygen, "Advection", 0.0)
    local cerium = make(ids.cerium, 120, 120, 500.0)
    make(ids.oxygen, 121, 120, 500.0)
    make(ids.oxygen, 119, 120, 500.0)
    make(ids.oxygen, 120, 121, 500.0)
    make(ids.oxygen, 120, 119, 500.0)
    step(8)
    assert(sim.partProperty(cerium, "tmp") == 4
            and count_type(ids.oxygen) == 0,
        "warm cerium did not absorb four bounded local oxygen particles")
    sim.partProperty(cerium, "temp", 1100.0)
    step(4)
    assert(sim.partProperty(cerium, "tmp") == 0
            and count_type(ids.oxygen) == 4,
        "hot cerium did not release four stored oxygen particles")
    elements.property(ids.oxygen, "Diffusion", old_o_diffusion)
    elements.property(ids.oxygen, "Advection", old_o_advection)

    local function run_magnetic_response(type, expected_life, seed)
        configure(seed)
        local material = make(type, 120, 120, 300.0)
        local electron = make(ids.electron, 121, 120, 300.0)
        sim.partProperty(electron, "vx", 0.0)
        sim.partProperty(electron, "vy", 0.0)
        step()
        assert(sim.partProperty(material, "life") >= expected_life - 1
                and sim.partProperty(material, "tmp2") == 1,
            "lanthanide magnetic response did not enter its bounded glow state")
    end
    run_magnetic_response(ids.praseodymium, 55, 1161)
    run_magnetic_response(ids.neodymium, 70, 1171)
    run_magnetic_response(ids.dysprosium, 80, 1181)
    run_magnetic_response(ids.holmium, 75, 1191)

    configure(1201)
    local promethium = make(ids.promethium, 120, 120, 300.0)
    assert(sim.partProperty(promethium, "life") >= 180
            and sim.partProperty(promethium, "life") <= 360,
        "promethium creation did not initialize its bounded lifetime")
    sim.partProperty(promethium, "life", 0)
    step()
    assert(sim.partProperty(promethium, "type") == ids.samarium,
        "promethium did not decay to samarium")
    assert(count_type(ids.photon) == 1,
        "one promethium decay did not emit exactly one finite photon")

    configure(1211)
    local molten_promethium = make(ids.lava, 120, 120, 1500.0)
    sim.partProperty(molten_promethium, "ctype", ids.promethium)
    sim.partProperty(molten_promethium, "life", 0)
    step()
    assert(sim.partProperty(molten_promethium, "type") == ids.lava
            and sim.partProperty(molten_promethium, "ctype") == ids.samarium,
        "typed molten promethium did not decay to typed molten samarium")

    local function run_neutron_capture(type, seed)
        configure(seed)
        local material = make(type, 120, 120, 300.0)
        local neutron = make(ids.neutron, 121, 120, 300.0)
        sim.partProperty(neutron, "vx", 0.0)
        sim.partProperty(neutron, "vy", 0.0)
        step()
        assert(sim.partProperty(material, "tmp") == 1
                and not sim.partExists(neutron),
            "lanthanide neutron absorber did not consume one local neutron")
        return sim.partProperty(material, "temp")
    end
    local samarium_temp = run_neutron_capture(ids.samarium, 1221)
    local gadolinium_temp = run_neutron_capture(ids.gadolinium, 1231)
    local lutetium_temp = run_neutron_capture(ids.lutetium, 1241)
    assert(gadolinium_temp > samarium_temp and samarium_temp > lutetium_temp,
        "lanthanide neutron-capture heating is not element-specific")

    local function run_fluorescence(type, expected_mask, seed)
        configure(seed)
        local material = make(type, 120, 120, 300.0)
        local photon = make(ids.photon, 121, 120, 300.0)
        sim.partProperty(photon, "vx", 0.0)
        sim.partProperty(photon, "vy", 0.0)
        step()
        assert(sim.partProperty(material, "life") > 0
                and sim.partExists(photon)
                and sim.partProperty(photon, "ctype") == expected_mask,
            "lanthanide phosphor did not convert the photon band")
    end
    run_fluorescence(ids.europium, 0x000FF000, 1251)
    run_fluorescence(ids.terbium, 0x00003FF0, 1261)

    local function run_photon_amplifier(type, expected_mask, seed)
        configure(seed)
        local material = make(type, 120, 120, 300.0)
        local seed_photon = make(ids.photon, 121, 120, 300.0)
        sim.partProperty(seed_photon, "vx", 0.0)
        sim.partProperty(seed_photon, "vy", 0.0)
        local before = count_type(ids.photon)
        step()
        assert(sim.partProperty(material, "life") > 0
                and count_type(ids.photon) == before + 1,
            "lanthanide optical material did not emit one bounded photon")
        local found_band = false
        for particle in sim.parts() do
            if sim.partProperty(particle, "type") == ids.photon
                    and sim.partProperty(particle, "ctype") == expected_mask then
                found_band = true
            end
        end
        assert(found_band, "lanthanide optical material emitted the wrong photon band")
        sim.resetOmniEventMetrics()
        step()
        local metrics = sim.omniEventMetrics()
        assert(tonumber(metrics.total) == 0,
            "lanthanide photon amplifier ignored its finite cooldown")
    end
    run_photon_amplifier(ids.erbium, 0x00003FF0, 1271)
    run_photon_amplifier(ids.thulium, 0x03F00000, 1281)

    configure(1291)
    local old_water_advection = elements.property(ids.water, "Advection")
    local old_water_gravity = elements.property(ids.water, "Gravity")
    local old_water_falldown = elements.property(ids.water, "Falldown")
    elements.property(ids.water, "Advection", 0.0)
    elements.property(ids.water, "Gravity", 0.0)
    elements.property(ids.water, "Falldown", 0)
    local water = make(ids.water, 121, 120, 340.0)
    local ytterbium = make(ids.ytterbium, 120, 120, 340.0)
    step()
    elements.property(ids.water, "Advection", old_water_advection)
    elements.property(ids.water, "Gravity", old_water_gravity)
    elements.property(ids.water, "Falldown", old_water_falldown)
    assert(sim.partProperty(ytterbium, "type") == ids.salt
            and sim.partProperty(water, "type") == ids.hydrogen,
        "warm ytterbium and water did not produce generic salt and hydrogen")

    configure(1301)
    local old_acid_advection = elements.property(ids.acid, "Advection")
    local old_acid_gravity = elements.property(ids.acid, "Gravity")
    local old_acid_falldown = elements.property(ids.acid, "Falldown")
    elements.property(ids.acid, "Advection", 0.0)
    elements.property(ids.acid, "Gravity", 0.0)
    elements.property(ids.acid, "Falldown", 0)
    local lutetium = make(ids.lutetium, 120, 120, 510.0)
    local acid = make(ids.acid, 121, 120, 510.0)
    step()
    elements.property(ids.acid, "Advection", old_acid_advection)
    elements.property(ids.acid, "Gravity", old_acid_gravity)
    elements.property(ids.acid, "Falldown", old_acid_falldown)
    assert(sim.partProperty(lutetium, "type") == ids.salt
            and sim.partProperty(acid, "type") == ids.hydrogen,
        "hot lutetium acid route did not produce generic salt and hydrogen")

    configure(1311)
    old_o_diffusion = elements.property(ids.oxygen, "Diffusion")
    old_o_advection = elements.property(ids.oxygen, "Advection")
    elements.property(ids.oxygen, "Diffusion", 0.0)
    elements.property(ids.oxygen, "Advection", 0.0)
    local terbium = make(ids.terbium, 120, 120, 730.0)
    local oxygen = make(ids.oxygen, 121, 120, 730.0)
    step()
    elements.property(ids.oxygen, "Diffusion", old_o_diffusion)
    elements.property(ids.oxygen, "Advection", old_o_advection)
    assert(sim.partProperty(terbium, "type") == ids.scrap
            and sim.partProperty(terbium, "ctype") == ids.terbium
            and sim.partProperty(oxygen, "type") == ids.fire,
        "hot terbium oxygen route did not produce typed scrap and finite fire")

    configure(1321)
    local molten = make(ids.lava, 120, 120, 2230.0)
    sim.partProperty(molten, "ctype", ids.thulium)
    step()
    assert(sim.partProperty(molten, "type") == ids.fire
            and sim.partProperty(molten, "ctype") == ids.thulium
            and sim.partProperty(molten, "life") == 60,
        "hot molten thulium did not enter its finite vaporisation proxy")
end

local function run_actinide_reactions()
    configure(1341)
    local thorium = make(ids.thorium, 120, 120, 293.15)
    local lawrencium = make(ids.lawrencium, 122, 120, 293.15)
    assert(sim.partProperty(thorium, "life") >= 1200
            and sim.partProperty(thorium, "life") <= 2000,
        "thorium creation did not initialize the longest bounded family timer")
    assert(sim.partProperty(lawrencium, "life") >= 120
            and sim.partProperty(lawrencium, "life") <= 240,
        "lawrencium creation did not initialize the shortest bounded family timer")

    local function run_decay(type, product, minimum_life, maximum_life, radiation, seed)
        configure(seed)
        local material = make(type, 120, 120, 300.0)
        sim.partProperty(material, "life", 0)
        step()
        assert(sim.partProperty(material, "type") == product,
            "actinide representative decay produced the wrong descendant")
        if minimum_life then
            local life = sim.partProperty(material, "life")
            assert(life >= minimum_life and life <= maximum_life,
                "actinide decay did not initialize the descendant lifetime")
        end
        assert(count_type(radiation) == 1,
            "one actinide decay did not emit exactly one bounded radiation particle")
    end
    run_decay(ids.actinium, ids.francium, nil, nil, ids.photon, 1351)
    run_decay(ids.thorium, ids.radium, nil, nil, ids.photon, 1361)
    run_decay(ids.protactinium, ids.uranium, nil, nil, ids.photon, 1371)
    run_decay(ids.neptunium, ids.protactinium, 900, 1500, ids.photon, 1381)
    run_decay(ids.californium, ids.curium, 480, 840, ids.neutron, 1391)
    run_decay(ids.lawrencium, ids.mendelevium, 180, 360, ids.photon, 1401)

    configure(1411)
    local molten = make(ids.lava, 120, 120, 1500.0)
    sim.partProperty(molten, "ctype", ids.americium)
    sim.partProperty(molten, "life", 0)
    step()
    assert(sim.partProperty(molten, "type") == ids.lava
            and sim.partProperty(molten, "ctype") == ids.neptunium
            and sim.partProperty(molten, "life") >= 720,
        "typed molten americium did not decay to typed molten neptunium")

    local function run_capture(type, product, temperature, seed)
        configure(seed)
        local material = make(type, 120, 120, temperature or 300.0)
        local neutron = make(ids.neutron, 121, 120, temperature or 300.0)
        sim.partProperty(neutron, "vx", 0.0)
        sim.partProperty(neutron, "vy", 0.0)
        step()
        assert(sim.partProperty(material, "type") == product,
            "actinide neutron transmutation produced the wrong product")
        assert(not sim.partExists(neutron),
            "actinide neutron transmutation did not consume the incoming neutron")
        return material
    end
    run_capture(ids.actinium, ids.thorium, 300.0, 1421)
    run_capture(ids.thorium, ids.protactinium, 300.0, 1431)
    run_capture(ids.protactinium, ids.uranium, 300.0, 1441)
    run_capture(ids.neptunium, ids.plutonium, 300.0, 1451)
    run_capture(ids.americium, ids.curium, 300.0, 1461)
    run_capture(ids.berkelium, ids.californium, 300.0, 1471)
    run_capture(ids.californium, ids.einsteinium, 300.0, 1481)
    run_capture(ids.nobelium, ids.lawrencium, 300.0, 1491)

    configure(1501)
    local hot_californium = make(ids.californium, 120, 120, 950.0)
    local incoming = make(ids.neutron, 121, 120, 950.0)
    sim.partProperty(incoming, "vx", 0.0)
    sim.partProperty(incoming, "vy", 0.0)
    step()
    assert(sim.partProperty(hot_californium, "type") == ids.curium,
        "hot californium did not enter the bounded curium fission proxy")
    assert(count_type(ids.neutron) == 1,
        "californium fission did not replace exactly one consumed neutron")

    configure(1511)
    local terminal = make(ids.lawrencium, 120, 120, 300.0)
    local terminal_neutron = make(ids.neutron, 121, 120, 300.0)
    sim.partProperty(terminal_neutron, "vx", 0.0)
    sim.partProperty(terminal_neutron, "vy", 0.0)
    step()
    assert(sim.partProperty(terminal, "type") == ids.lawrencium
            and sim.partProperty(terminal, "tmp") == 1
            and sim.partProperty(terminal, "temp") > 700.0,
        "terminal lawrencium did not retain bounded neutron-capture heat")

    configure(1521)
    local old_acid_advection = elements.property(ids.acid, "Advection")
    local old_acid_gravity = elements.property(ids.acid, "Gravity")
    local old_acid_falldown = elements.property(ids.acid, "Falldown")
    elements.property(ids.acid, "Advection", 0.0)
    elements.property(ids.acid, "Gravity", 0.0)
    elements.property(ids.acid, "Falldown", 0)
    local actinium = make(ids.actinium, 120, 120, 340.0)
    local acid = make(ids.acid, 121, 120, 340.0)
    step()
    elements.property(ids.acid, "Advection", old_acid_advection)
    elements.property(ids.acid, "Gravity", old_acid_gravity)
    elements.property(ids.acid, "Falldown", old_acid_falldown)
    assert(sim.partProperty(actinium, "type") == ids.scrap
            and sim.partProperty(actinium, "ctype") == ids.actinium
            and sim.partProperty(acid, "type") == ids.hydrogen,
        "actinide acid route did not retain typed radioactive waste")

    configure(1531)
    local old_o_diffusion = elements.property(ids.oxygen, "Diffusion")
    local old_o_advection = elements.property(ids.oxygen, "Advection")
    elements.property(ids.oxygen, "Diffusion", 0.0)
    elements.property(ids.oxygen, "Advection", 0.0)
    local americium = make(ids.americium, 120, 120, 610.0)
    local oxygen = make(ids.oxygen, 121, 120, 610.0)
    step()
    elements.property(ids.oxygen, "Diffusion", old_o_diffusion)
    elements.property(ids.oxygen, "Advection", old_o_advection)
    assert(sim.partProperty(americium, "type") == ids.scrap
            and sim.partProperty(americium, "ctype") == ids.americium
            and sim.partProperty(oxygen, "type") == ids.fire,
        "hot actinide oxygen route did not produce typed waste and finite fire")

    configure(1541)
    local vapour = make(ids.lava, 120, 120, 1300.0)
    sim.partProperty(vapour, "ctype", ids.einsteinium)
    sim.partProperty(vapour, "life", 200)
    step()
    assert(sim.partProperty(vapour, "type") == ids.fire
            and sim.partProperty(vapour, "ctype") == ids.einsteinium
            and sim.partProperty(vapour, "life") == 45,
        "hot molten einsteinium did not enter its finite vapour proxy")
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

    configure(118)
    local nihonium = make(ids.nihonium, 120, 120, 293.15)
    sim.partProperty(nihonium, "tmp", 1)
    step()
    assert(sim.partProperty(nihonium, "type") == ids.polonium,
        "nihonium did not enter its compressed polonium decay proxy")
    assert(count_type(ids.photon) == 1,
        "one nihonium decay did not emit exactly one finite photon")

    configure(119)
    local flerovium = make(ids.flerovium, 120, 120, 293.15)
    sim.partProperty(flerovium, "tmp", 1)
    step()
    assert(sim.partProperty(flerovium, "type") == ids.polonium,
        "flerovium did not enter its compressed polonium decay proxy")
    assert(count_type(ids.photon) == 1,
        "one flerovium decay did not emit exactly one finite photon")

    configure(120)
    local moscovium = make(ids.moscovium, 120, 120, 293.15)
    sim.partProperty(moscovium, "tmp", 1)
    step()
    assert(sim.partProperty(moscovium, "type") == ids.nihonium,
        "moscovium did not enter the compressed nihonium decay proxy")
    assert(sim.partProperty(moscovium, "tmp") >= 90
            and sim.partProperty(moscovium, "tmp") <= 180,
        "moscovium decay did not initialize the nihonium lifetime")
    assert(count_type(ids.photon) == 1,
        "first moscovium decay stage did not emit exactly one finite photon")
    sim.partProperty(moscovium, "tmp", 1)
    step()
    assert(sim.partProperty(moscovium, "type") == ids.polonium,
        "nihonium produced by moscovium did not decay to polonium")
    assert(count_type(ids.photon) == 2,
        "two-stage moscovium decay did not emit exactly two finite photons")

    configure(130)
    local livermorium = make(ids.livermorium, 120, 120, 293.15)
    sim.partProperty(livermorium, "tmp", 1)
    step()
    assert(sim.partProperty(livermorium, "type") == ids.flerovium,
        "livermorium did not enter the compressed flerovium decay proxy")
    assert(sim.partProperty(livermorium, "tmp") >= 60
            and sim.partProperty(livermorium, "tmp") <= 130,
        "livermorium decay did not initialize the flerovium lifetime")
    assert(count_type(ids.photon) == 1,
        "first livermorium decay stage did not emit exactly one finite photon")
    sim.partProperty(livermorium, "tmp", 1)
    step()
    assert(sim.partProperty(livermorium, "type") == ids.polonium,
        "flerovium produced by livermorium did not decay to polonium")
    assert(count_type(ids.photon) == 2,
        "two-stage livermorium decay did not emit exactly two finite photons")
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

local function run_nihonium_budget()
    configure(441)
    local total = 1200
    for index = 0, total - 1 do
        local particle = make(ids.nihonium,
            50 + (index % 100) * 5,
            50 + math.floor(index / 100) * 5,
            293.15)
        sim.partProperty(particle, "tmp", 1)
    end
    step()
    local metrics = sim.omniEventMetrics()
    local events = assert(tonumber(metrics.total), "missing nihonium event total")
    assert(events > 0 and events <= 1024,
        "one-frame nihonium events were not capped at 1024: " .. tostring(events))
    assert(count_type(ids.nihonium) >= total - 1024,
        "event-budget exhaustion did not defer remaining nihonium decays")
    assert(count_type(ids.photon) <= 1024,
        "bounded nihonium decays emitted too many photons")
    return events
end

local function run_flerovium_budget()
    configure(541)
    local total = 1200
    for index = 0, total - 1 do
        local particle = make(ids.flerovium,
            50 + (index % 100) * 5,
            50 + math.floor(index / 100) * 5,
            293.15)
        sim.partProperty(particle, "tmp", 1)
    end
    step()
    local metrics = sim.omniEventMetrics()
    local events = assert(tonumber(metrics.total), "missing flerovium event total")
    assert(events > 0 and events <= 1024,
        "one-frame flerovium events were not capped at 1024: " .. tostring(events))
    assert(count_type(ids.flerovium) >= total - 1024,
        "event-budget exhaustion did not defer remaining flerovium decays")
    assert(count_type(ids.photon) <= 1024,
        "bounded flerovium decays emitted too many photons")
    return events
end

local function run_moscovium_budget()
    configure(641)
    local total = 1200
    for index = 0, total - 1 do
        local particle = make(ids.moscovium,
            50 + (index % 100) * 5,
            50 + math.floor(index / 100) * 5,
            293.15)
        sim.partProperty(particle, "tmp", 1)
    end
    step()
    local metrics = sim.omniEventMetrics()
    local events = assert(tonumber(metrics.total), "missing moscovium event total")
    assert(events > 0 and events <= 1024,
        "one-frame moscovium events were not capped at 1024: " .. tostring(events))
    assert(count_type(ids.moscovium) >= total - 1024,
        "event-budget exhaustion did not defer remaining moscovium decays")
    assert(count_type(ids.photon) <= 1024,
        "bounded moscovium decays emitted too many photons")
    return events
end

local function run_livermorium_budget()
    configure(741)
    local total = 1200
    for index = 0, total - 1 do
        local particle = make(ids.livermorium,
            50 + (index % 100) * 5,
            50 + math.floor(index / 100) * 5,
            293.15)
        sim.partProperty(particle, "tmp", 1)
    end
    step()
    local metrics = sim.omniEventMetrics()
    local events = assert(tonumber(metrics.total), "missing livermorium event total")
    assert(events > 0 and events <= 1024,
        "one-frame livermorium events were not capped at 1024: " .. tostring(events))
    assert(count_type(ids.livermorium) >= total - 1024,
        "event-budget exhaustion did not defer remaining livermorium decays")
    assert(count_type(ids.photon) <= 1024,
        "bounded livermorium decays emitted too many photons")
    return events
end

local function run_astatine_budget()
    configure(851)
    local total = 1200
    for index = 0, total - 1 do
        local particle = make(ids.astatine,
            50 + (index % 100) * 5,
            50 + math.floor(index / 100) * 5,
            293.15)
        sim.partProperty(particle, "tmp", 1)
    end
    step()
    local metrics = sim.omniEventMetrics()
    local events = assert(tonumber(metrics.total), "missing astatine event total")
    assert(events > 0 and events <= 1024,
        "one-frame astatine events were not capped at 1024: " .. tostring(events))
    assert(count_type(ids.astatine) >= total - 1024,
        "event-budget exhaustion did not defer remaining astatine decays")
    assert(count_type(ids.photon) <= 1024,
        "bounded astatine decays emitted too many photons")
    return events
end

local function run_tennessine_budget()
    configure(861)
    local total = 1200
    for index = 0, total - 1 do
        local particle = make(ids.tennessine,
            50 + (index % 100) * 5,
            50 + math.floor(index / 100) * 5,
            293.15)
        sim.partProperty(particle, "tmp", 1)
    end
    step()
    local metrics = sim.omniEventMetrics()
    local events = assert(tonumber(metrics.total), "missing tennessine event total")
    assert(events > 0 and events <= 1024,
        "one-frame tennessine events were not capped at 1024: " .. tostring(events))
    assert(count_type(ids.tennessine) >= total - 1024,
        "event-budget exhaustion did not defer remaining tennessine decays")
    assert(count_type(ids.photon) <= 1024,
        "bounded tennessine decays emitted too many photons")
    return events
end

local function run_first_transition_budget()
    configure(931)
    local total = 1200
    for index = 0, total - 1 do
        local x = 50 + (index % 100) * 5
        local y = 50 + math.floor(index / 100) * 5
        make(ids.scandium, x, y, 760.0)
        make(ids.oxygen, x + 1, y, 760.0)
    end
    step()
    local metrics = sim.omniEventMetrics()
    local events = assert(tonumber(metrics.total),
        "missing first-transition event total")
    assert(events > 0 and events <= 1024,
        "one-frame first-transition events were not capped at 1024: "
            .. tostring(events))
    assert(count_type(ids.scandium) >= total - 1024,
        "event-budget exhaustion did not defer remaining scandium oxidation")
    return events
end

local function run_second_transition_budget()
    configure(1041)
    local total = 1200
    for index = 0, total - 1 do
        local particle = make(ids.technetium,
            50 + (index % 100) * 5,
            50 + math.floor(index / 100) * 5,
            293.15)
        sim.partProperty(particle, "tmp", 1)
    end
    step()
    local metrics = sim.omniEventMetrics()
    local events = assert(tonumber(metrics.total),
        "missing second-transition event total")
    assert(events > 0 and events <= 1024,
        "one-frame second-transition events were not capped at 1024: "
            .. tostring(events))
    assert(count_type(ids.technetium) >= total - 1024,
        "event-budget exhaustion did not defer remaining technetium decays")
    assert(count_type(ids.photon) <= 1024,
        "bounded technetium decays emitted too many photons")
    return events
end

local function run_third_transition_budget()
    configure(1131)
    local old_o_diffusion = elements.property(ids.oxygen, "Diffusion")
    local old_o_advection = elements.property(ids.oxygen, "Advection")
    elements.property(ids.oxygen, "Diffusion", 0.0)
    elements.property(ids.oxygen, "Advection", 0.0)
    local total = 1200
    for index = 0, total - 1 do
        local x = 50 + (index % 100) * 5
        local y = 50 + math.floor(index / 100) * 5
        make(ids.osmium, x, y, 500.0)
        make(ids.oxygen, x + 1, y, 500.0)
    end
    step()
    elements.property(ids.oxygen, "Diffusion", old_o_diffusion)
    elements.property(ids.oxygen, "Advection", old_o_advection)
    local metrics = sim.omniEventMetrics()
    local events = assert(tonumber(metrics.total),
        "missing third-transition event total")
    assert(events > 0 and events <= 1024,
        "one-frame third-transition events were not capped at 1024: "
            .. tostring(events))
    assert(count_type(ids.osmium) >= total - 1024,
        "event-budget exhaustion did not defer remaining osmium oxidation")
    assert(count_type(ids.smoke) <= 1024,
        "bounded osmium oxidation emitted too many smoke proxies")
    return events
end

local function run_lanthanide_budget()
    configure(1331)
    local total = 1200
    for index = 0, total - 1 do
        local x = 50 + (index % 100) * 5
        local y = 50 + math.floor(index / 100) * 5
        local promethium = make(ids.promethium, x, y, 300.0)
        sim.partProperty(promethium, "life", 0)
    end
    step()
    local metrics = sim.omniEventMetrics()
    local events = assert(tonumber(metrics.total),
        "missing lanthanide event total")
    assert(events > 0 and events <= 1024,
        "one-frame lanthanide events were not capped at 1024: "
            .. tostring(events))
    assert(count_type(ids.promethium) >= total - 1024,
        "event-budget exhaustion did not defer remaining promethium decays")
    assert(count_type(ids.photon) <= 1024,
        "bounded promethium decays emitted too many photons")
    return events
end

local function run_actinide_budget()
    configure(1551)
    local total = 1200
    for index = 0, total - 1 do
        local x = 50 + (index % 100) * 5
        local y = 50 + math.floor(index / 100) * 5
        local lawrencium = make(ids.lawrencium, x, y, 300.0)
        sim.partProperty(lawrencium, "life", 0)
    end
    step()
    local metrics = sim.omniEventMetrics()
    local events = assert(tonumber(metrics.total),
        "missing actinide event total")
    assert(events > 0 and events <= 1024,
        "one-frame actinide events were not capped at 1024: "
            .. tostring(events))
    assert(count_type(ids.lawrencium) >= total - 1024,
        "event-budget exhaustion did not defer remaining lawrencium decays")
    assert(count_type(ids.photon) <= 1024,
        "bounded actinide decays emitted too many photons")
    return events
end

local function test()
    run_property_differences()
    run_alkali_water_series()
    run_alkali_acid_oxygen_and_phase()
    run_alkaline_earth_water_series()
    run_alkaline_earth_oxygen_and_phase()
    run_boron_group_reactions()
    run_carbon_group_reactions()
    run_nitrogen_group_reactions()
    run_oxygen_group_reactions()
    run_halogen_group_reactions()
    run_first_transition_reactions()
    run_second_transition_reactions()
    run_third_transition_reactions()
    run_lanthanide_reactions()
    run_actinide_reactions()
    run_helium_cryogenics()
    run_xenon_discharge()
    run_radioactive_decays()
    local noble_budget = run_decay_budget()
    local francium_budget = run_francium_budget()
    local radium_budget = run_radium_budget()
    local nihonium_budget = run_nihonium_budget()
    local flerovium_budget = run_flerovium_budget()
    local moscovium_budget = run_moscovium_budget()
    local livermorium_budget = run_livermorium_budget()
    local astatine_budget = run_astatine_budget()
    local tennessine_budget = run_tennessine_budget()
    local first_transition_budget = run_first_transition_budget()
    local second_transition_budget = run_second_transition_budget()
    local third_transition_budget = run_third_transition_budget()
    local lanthanide_budget = run_lanthanide_budget()
    local actinide_budget = run_actinide_budget()
    return math.max(noble_budget, francium_budget, radium_budget,
        nihonium_budget, flerovium_budget, moscovium_budget,
        livermorium_budget, astatine_budget, tennessine_budget,
        first_transition_budget, second_transition_budget,
        third_transition_budget, lanthanide_budget, actinide_budget)
end

local ok, data = xpcall(test, debug.traceback)
local report = assert(io.open(RESULT, "w"))
if ok then
    report:write("OMNI_PERIODIC_STATUS=PASS\n")
    report:write("OMNI_PERIODIC_NEW_ELEMENTS=83\n")
    report:write("OMNI_PERIODIC_IMPLEMENTED_MAPPINGS=109\n")
    report:write("OMNI_PERIODIC_ALKALI_FAMILY=6\n")
    report:write("OMNI_PERIODIC_ALKALINE_EARTH_FAMILY=6\n")
    report:write("OMNI_PERIODIC_BORON_GROUP=6\n")
    report:write("OMNI_PERIODIC_CARBON_GROUP=6\n")
    report:write("OMNI_PERIODIC_NITROGEN_GROUP=6\n")
    report:write("OMNI_PERIODIC_OXYGEN_GROUP=6\n")
    report:write("OMNI_PERIODIC_HALOGEN_GROUP=6\n")
    report:write("OMNI_PERIODIC_FIRST_TRANSITION_SERIES=10\n")
    report:write("OMNI_PERIODIC_SECOND_TRANSITION_SERIES=10\n")
    report:write("OMNI_PERIODIC_THIRD_TRANSITION_SERIES=9\n")
    report:write("OMNI_PERIODIC_LANTHANIDE_SERIES=15\n")
    report:write("OMNI_PERIODIC_ACTINIDE_SERIES=15\n")
    report:write("OMNI_PERIODIC_BUDGET_EVENTS=" .. tostring(data) .. "\n")
else
    local error_text = tostring(data):gsub("[\r\n]+", " | ")
    report:write("OMNI_PERIODIC_STATUS=FAIL\n")
    report:write("OMNI_PERIODIC_ERROR=" .. error_text .. "\n")
end
report:close()
