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
    lithium = must_element("DEFAULT_PT_LITH", "LITH", 191),
    rubidium = must_element("DEFAULT_PT_RBDM", "RBDM", 41),
    hydrogen = must_element("DEFAULT_PT_H2", "HYGN", 148),
    water = must_element("DEFAULT_PT_WATR", "WATR", 2),
    acid = must_element("DEFAULT_PT_ACID", "ACID", 21),
    caustic = must_element("DEFAULT_PT_CAUS", "CAUS", 86),
    salt = must_element("DEFAULT_PT_SALT", "SALT", 26),
    dust = must_element("DEFAULT_PT_DUST", "DUST", 1),
    oxygen = must_element("DEFAULT_PT_O2", "OXYG", 61),
    neutron = must_element("DEFAULT_PT_NEUT", "NEUT", 18),
    glass = must_element("DEFAULT_PT_GLAS", "GLAS", 45),
    liquid_nitrogen = must_element("DEFAULT_PT_LNTG", "LN2", 37),
    nitrogen_ice = must_element("DEFAULT_PT_NICE", "NICE", 51),
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

local function test()
    run_property_differences()
    run_alkali_water_series()
    run_alkali_acid_oxygen_and_phase()
    run_alkaline_earth_water_series()
    run_alkaline_earth_oxygen_and_phase()
    run_boron_group_reactions()
    run_carbon_group_reactions()
    run_nitrogen_group_reactions()
    run_helium_cryogenics()
    run_xenon_discharge()
    run_radioactive_decays()
    local noble_budget = run_decay_budget()
    local francium_budget = run_francium_budget()
    local radium_budget = run_radium_budget()
    local nihonium_budget = run_nihonium_budget()
    local flerovium_budget = run_flerovium_budget()
    local moscovium_budget = run_moscovium_budget()
    return math.max(noble_budget, francium_budget, radium_budget,
        nihonium_budget, flerovium_budget, moscovium_budget)
end

local ok, data = xpcall(test, debug.traceback)
local report = assert(io.open(RESULT, "w"))
if ok then
    report:write("OMNI_PERIODIC_STATUS=PASS\n")
    report:write("OMNI_PERIODIC_NEW_ELEMENTS=29\n")
    report:write("OMNI_PERIODIC_IMPLEMENTED_MAPPINGS=55\n")
    report:write("OMNI_PERIODIC_ALKALI_FAMILY=6\n")
    report:write("OMNI_PERIODIC_ALKALINE_EARTH_FAMILY=6\n")
    report:write("OMNI_PERIODIC_BORON_GROUP=6\n")
    report:write("OMNI_PERIODIC_CARBON_GROUP=6\n")
    report:write("OMNI_PERIODIC_NITROGEN_GROUP=6\n")
    report:write("OMNI_PERIODIC_BUDGET_EVENTS=" .. tostring(data) .. "\n")
else
    local error_text = tostring(data):gsub("[\r\n]+", " | ")
    report:write("OMNI_PERIODIC_STATUS=FAIL\n")
    report:write("OMNI_PERIODIC_ERROR=" .. error_text .. "\n")
end
report:close()
