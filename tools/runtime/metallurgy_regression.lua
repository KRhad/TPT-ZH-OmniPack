local RESULT = "lua-metallurgy-regression.result"

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
    iron = assert(elements.DEFAULT_PT_IRON),
    oxygen = assert(elements.DEFAULT_PT_O2),
    water = assert(elements.DEFAULT_PT_WATR),
    wood = assert(elements.DEFAULT_PT_WOOD),
    coal = assert(elements.DEFAULT_PT_COAL),
    co2 = assert(elements.DEFAULT_PT_CO2),
    brmt = assert(elements.DEFAULT_PT_BRMT),
    dust = assert(elements.DEFAULT_PT_DUST),
    conv = assert(elements.DEFAULT_PT_CONV),
    alum = must_element("OMNI_PT_ALUM", "ALUM"),
    copr = must_element("OMNI_PT_COPR", "COPR"),
    lead = must_element("OMNI_PT_LEAD", "LEAD"),
    tin = must_element("OMNI_PT_TIN", "TIN"),
    nicl = must_element("OMNI_PT_NICL", "NICL"),
    magn = must_element("OMNI_PT_MAGN", "MAGN"),
    chrm = must_element("OMNI_PT_CHRM", "CHRM"),
    cobt = must_element("OMNI_PT_COBT", "COBT"),
    moly = must_element("OMNI_PT_MOLY", "MOLY"),
    zinc = must_element("OMNI_PT_ZINC", "ZINC"),
    chrc = must_element("OMNI_PT_CHRC", "CHRC"),
    coke = must_element("OMNI_PT_COKE", "COKE"),
    stel = must_element("OMNI_PT_STEL", "STEL"),
    brnz = must_element("OMNI_PT_BRNZ", "BRNZ"),
    bras = must_element("OMNI_PT_BRAS", "BRAS"),
    ssil = must_element("OMNI_PT_SSIL", "SSIL"),
    ncrm = must_element("OMNI_PT_NCRM", "NCRM"),
    almg = must_element("OMNI_PT_ALMG", "ALMG"),
    tstl = must_element("OMNI_PT_TSTL", "TSTL"),
    slag = must_element("OMNI_PT_SLAG", "SLAG"),
    flux = must_element("OMNI_PT_FLUX", "FLUX"),
    cruc = must_element("OMNI_PT_CRUC", "CRUC"),
    legacy_mscr = must_element("OMNI_PT_MSCR", "MSCR"),
    rshd = must_element("OMNI_PT_RSHD", "RSHD"),
    sold = must_element("OMNI_PT_SOLD", "SOLD"),
}
assert(ids.sold == 512,
    "SOLD stable high ID changed: expected 512, got " .. tostring(ids.sold))

local RECOVERABLE_SCRAP_MARKER = 0x4F4D5343

local positions = {
    { 0, 0 }, { 1, 0 }, { 0, 1 }, { -1, 0 },
    { 0, -1 }, { 1, 1 }, { -1, 1 }, { 1, -1 }, { -1, -1 },
}

local function configure_simulation()
    sim.clearSim()
    sim.paused(true)
    sim.gravityMode(sim.GRAV_OFF)
    sim.airMode(sim.AIR_OFF)
    sim.ambientHeatSim(false)
    sim.heatSim(true)
    sim.ensureDeterminism(true)
    sim.randomSeed(1, 2, 3, 4)
end

local function create_molten(material, position, temperature)
    local id = sim.partCreate(
        -1, 120 + position[1], 120 + position[2], ids.lava)
    assert(id >= 0, "failed to create molten ingredient")
    sim.partProperty(id, "ctype", material)
    sim.partProperty(id, "temp", temperature)
    return id
end

local function wait_for_ctype(particles, product, maximum_frames)
    for frame = 1, maximum_frames do
        sim.updateUpTo()
        local complete = true
        for _, particle in ipairs(particles) do
            if sim.partProperty(particle, "type") ~= ids.lava
                or sim.partProperty(particle, "ctype") ~= product then
                complete = false
                break
            end
        end
        if complete then
            return frame
        end
    end
    return nil
end

local function run_alloy(name, inputs, product, temperature)
    configure_simulation()
    local particles = {}
    for index, material in ipairs(inputs) do
        particles[index] = create_molten(
            material, positions[index], temperature)
    end
    local frames = wait_for_ctype(particles, product, 4)
    assert(frames, name .. " did not produce its declared molten alloy")

    for _, particle in ipairs(particles) do
        sim.partProperty(particle, "temp", 300.0)
    end
    local solid = false
    for _ = 1, 120 do
        sim.updateUpTo()
        solid = true
        for _, particle in ipairs(particles) do
            if sim.partProperty(particle, "type") ~= product then
                solid = false
                break
            end
        end
        if solid then
            break
        end
    end
    assert(solid, name .. " did not cool within 120 simulation frames")
    for _, particle in ipairs(particles) do
        local final_type = sim.partProperty(particle, "type")
        local final_ctype = sim.partProperty(particle, "ctype")
        local final_temp = sim.partProperty(particle, "temp")
        assert(final_type == product,
            name .. " did not solidify to its stable product; type="
            .. tostring(final_type) .. " ctype=" .. tostring(final_ctype)
            .. " temp=" .. tostring(final_temp)
            .. " melt=" .. tostring(elements.property(
                product, "HighTemperature"))
            .. " transition=" .. tostring(elements.property(
                product, "HighTemperatureTransition"))
            .. " expected=" .. tostring(product))
        assert(final_ctype == 0,
            name .. " retained stale ctype after solidification; ctype="
            .. tostring(final_ctype))
    end
    return frames
end

local function run_steel()
    configure_simulation()
    local iron = {}
    for index = 1, 4 do
        iron[index] = create_molten(
            ids.iron, positions[index], 2600.0)
    end
    local coke = sim.partCreate(
        -1, 120 + positions[5][1], 120 + positions[5][2], ids.coke)
    local flux = sim.partCreate(
        -1, 120 + positions[6][1], 120 + positions[6][2], ids.flux)
    assert(coke >= 0 and flux >= 0, "failed to create steel additives")
    sim.partProperty(coke, "temp", 1200.0)
    sim.partProperty(flux, "temp", 1200.0)

    local frames = wait_for_ctype(iron, ids.stel, 4)
    assert(frames, "steel recipe did not convert four molten iron particles")
    assert(sim.partProperty(coke, "type") == ids.co2,
        "steel recipe did not preserve the carbon input as CO2")
    local flux_type = sim.partProperty(flux, "type")
    local flux_ctype = sim.partProperty(flux, "ctype")
    assert(flux_type == ids.slag
        or (flux_type == ids.lava and flux_ctype == ids.slag),
        "steel recipe did not convert direct or molten flux to slag")
    return frames
end

local function run_carbonization()
    configure_simulation()
    local wood = sim.partCreate(-1, 120, 120, ids.wood)
    local wood_crucible = sim.partCreate(-1, 121, 120, ids.cruc)
    assert(wood >= 0 and wood_crucible >= 0, "wood retort setup failed")
    sim.partProperty(wood, "temp", 700.0)
    sim.partProperty(wood_crucible, "temp", 700.0)
    for _ = 1, 65 do
        sim.updateUpTo()
    end
    assert(sim.partProperty(wood, "type") == ids.chrc,
        "oxygen-starved wood did not become charcoal after its hold time")

    configure_simulation()
    local coal = sim.partCreate(-1, 120, 120, ids.coal)
    local coal_crucible = sim.partCreate(-1, 121, 120, ids.cruc)
    assert(coal >= 0 and coal_crucible >= 0, "coal retort setup failed")
    sim.partProperty(coal, "temp", 1100.0)
    sim.partProperty(coal_crucible, "temp", 1100.0)
    for _ = 1, 95 do
        sim.updateUpTo()
    end
    assert(sim.partProperty(coal, "type") == ids.coke,
        "oxygen-starved coal did not become coke after its hold time")
end

local function run_negative_control()
    configure_simulation()
    local copper = sim.partCreate(-1, 120, 120, ids.copr)
    local tin = sim.partCreate(-1, 121, 120, ids.tin)
    assert(copper >= 0 and tin >= 0, "negative-control setup failed")
    for _ = 1, 5 do
        sim.updateUpTo()
    end
    assert(sim.partProperty(copper, "type") == ids.copr
        and sim.partProperty(tin, "type") == ids.tin,
        "cold solid metals alloyed without melting")
end

local function run_material_behaviors()
    configure_simulation()
    local heater = sim.partCreate(-1, 120, 120, ids.ncrm)
    assert(heater >= 0, "nichrome heater setup failed")
    sim.partProperty(heater, "type", ids.spark)
    sim.partProperty(heater, "ctype", ids.ncrm)
    sim.partProperty(heater, "life", 4)
    sim.partProperty(heater, "temp", 300.0)
    sim.updateUpTo()
    assert(sim.partProperty(heater, "temp") > 300.0,
        "nichrome spark did not produce resistive heat")

    configure_simulation()
    local fusible = sim.partCreate(-1, 120, 120, ids.sold)
    assert(fusible >= 0, "solder fusible-link setup failed")
    sim.partProperty(fusible, "type", ids.spark)
    sim.partProperty(fusible, "ctype", ids.sold)
    sim.partProperty(fusible, "life", 4)
    sim.partProperty(fusible, "temp", 300.0)
    for _ = 1, 10 do
        sim.updateUpTo()
        if sim.partProperty(fusible, "type") == ids.lava then
            break
        end
    end
    assert(sim.partProperty(fusible, "type") == ids.lava
            and sim.partProperty(fusible, "ctype") == ids.sold,
        "repeated solder spark did not melt into typed high-ID LAVA")

    configure_simulation()
    local soft = sim.partCreate(-1, 120, 120, ids.alum)
    local strong = sim.partCreate(-1, 124, 120, ids.tstl)
    assert(soft >= 0 and strong >= 0, "pressure test setup failed")
    sim.airMode(sim.AIR_NOUPDATE)
    sim.pressure(30, 30, 256.0)
    sim.pressure(31, 30, 80.0)
    sim.updateUpTo()
    local soft_type = sim.partProperty(soft, "type")
    local soft_ctype = sim.partProperty(soft, "ctype")
    assert(sim.partProperty(soft, "type") == ids.brmt
        and sim.partProperty(soft, "ctype") == ids.alum
        and sim.partProperty(soft, "tmp4") == RECOVERABLE_SCRAP_MARKER,
        "soft aluminium did not preserve its type as pressure scrap; type="
        .. tostring(soft_type) .. " ctype=" .. tostring(soft_ctype)
        .. " pressure=" .. tostring(sim.pressure(30, 30)))
    assert(sim.partProperty(strong, "type") == ids.tstl,
        "tool steel failed the differentiated pressure threshold")

    configure_simulation()
    local solder_scrap = sim.partCreate(-1, 120, 120, ids.sold)
    assert(solder_scrap >= 0, "high-ID solder pressure setup failed")
    sim.airMode(sim.AIR_NOUPDATE)
    sim.pressure(30, 30, 256.0)
    sim.updateUpTo()
    assert(sim.partProperty(solder_scrap, "type") == ids.brmt
            and sim.partProperty(solder_scrap, "ctype") == ids.sold
            and sim.partProperty(solder_scrap, "tmp4") == RECOVERABLE_SCRAP_MARKER,
        "SOLD=512 did not survive pressure conversion in BRMT ctype")

    configure_simulation()
    local scrap = sim.partCreate(-1, 120, 120, ids.brmt)
    assert(scrap >= 0, "scrap recycling setup failed")
    sim.partProperty(scrap, "ctype", ids.copr)
    sim.partProperty(scrap, "tmp4", RECOVERABLE_SCRAP_MARKER)
    sim.partProperty(scrap, "temp", 1600.0)
    sim.updateUpTo()
    assert(sim.partProperty(scrap, "type") == ids.lava
        and sim.partProperty(scrap, "ctype") == ids.copr,
        "enhanced BRMT did not restore its recorded metal as molten ctype")

    local alias_ok = pcall(
        sim.partCreate, -1, 124, 120, ids.legacy_mscr)
    assert(not alias_ok,
        "legacy MSCR compatibility alias remained directly creatable")
    assert(elements.property(ids.legacy_mscr, "MenuVisible") == 0,
        "legacy MSCR compatibility alias remained visible")

    configure_simulation()
    local converted = sim.partCreate(-1, 121, 120, ids.dust)
    local converter = sim.partCreate(-1, 120, 120, ids.conv)
    assert(converted >= 0 and converter >= 0,
        "legacy carried-type migration setup failed")
    sim.partProperty(converter, "ctype", ids.legacy_mscr)
    sim.updateUpTo()
    assert(sim.partProperty(converted, "type") == ids.legacy_mscr,
        "official CONV did not resolve the legacy carried type internally")
    sim.partKill(converter)
    sim.updateUpTo()
    assert(sim.partProperty(converted, "type") == ids.brmt
            and sim.partProperty(converted, "ctype") == ids.iron
            and sim.partProperty(converted, "tmp4") == RECOVERABLE_SCRAP_MARKER,
        "legacy carried MSCR did not migrate to canonical recoverable BRMT")

    configure_simulation()
    local oxygen_diffusion = elements.property(ids.oxygen, "Diffusion")
    elements.property(ids.oxygen, "Diffusion", 0.0)
    local magnesium = sim.partCreate(-1, 120, 120, ids.magn)
    local oxygen = sim.partCreate(-1, 121, 120, ids.oxygen)
    assert(magnesium >= 0 and oxygen >= 0, "magnesium burn setup failed")
    sim.partProperty(magnesium, "temp", 850.0)
    sim.partProperty(oxygen, "temp", 850.0)
    for _ = 1, 120 do
        sim.updateUpTo()
        if sim.partProperty(magnesium, "type") == ids.brmt then
            break
        end
    end
    elements.property(ids.oxygen, "Diffusion", oxygen_diffusion)
    assert(sim.partProperty(magnesium, "type") == ids.brmt
        and sim.partProperty(magnesium, "ctype") == ids.magn
        and sim.partProperty(magnesium, "tmp4") == RECOVERABLE_SCRAP_MARKER,
        "hot magnesium did not burn into recoverable magnesium scrap")

    configure_simulation()
    local zinc = sim.partCreate(-1, 120, 120, ids.zinc)
    local protected_iron = sim.partCreate(-1, 121, 120, ids.iron)
    local water = sim.partCreate(-1, 120, 121, ids.water)
    assert(zinc >= 0 and protected_iron >= 0 and water >= 0,
        "zinc protection setup failed")
    sim.updateUpTo()
    assert(sim.partProperty(protected_iron, "life") > 0,
        "zinc did not pause corrosion on adjacent wet iron")
end

local function run_radiation_shield_assembly()
    configure_simulation()
    local steel = sim.partCreate(-1, 120, 120, ids.ssil)
    local lead = create_molten(ids.lead, { 1, 0 }, 800.0)
    local heater = sim.partCreate(-1, 120, 121, ids.ncrm)
    assert(steel >= 0 and lead >= 0 and heater >= 0,
        "radiation shield assembly setup failed")
    sim.partProperty(steel, "temp", 800.0)
    sim.partProperty(heater, "type", ids.spark)
    sim.partProperty(heater, "ctype", ids.ncrm)
    sim.partProperty(heater, "life", 4)
    sim.partProperty(heater, "temp", 800.0)
    sim.updateUpTo()
    assert(sim.partProperty(steel, "type") == ids.rshd
        and sim.partProperty(lead, "type") == ids.rshd,
        "stainless steel and molten lead did not assemble into two shields")

    configure_simulation()
    steel = sim.partCreate(-1, 120, 120, ids.ssil)
    lead = create_molten(ids.lead, { 1, 0 }, 800.0)
    assert(steel >= 0 and lead >= 0, "shield negative-control setup failed")
    sim.partProperty(steel, "temp", 800.0)
    for _ = 1, 3 do
        sim.updateUpTo()
    end
    assert(sim.partProperty(steel, "type") == ids.ssil
        and sim.partProperty(lead, "type") == ids.lava
        and sim.partProperty(lead, "ctype") == ids.lead,
        "shield assembly ran without its active nichrome spark")
end

local function test()
    local frames = {}
    frames.brnz = run_alloy(
        "bronze",
        { ids.copr, ids.copr, ids.copr, ids.tin },
        ids.brnz,
        2000.0)
    frames.bras = run_alloy(
        "brass",
        { ids.copr, ids.copr, ids.copr, ids.zinc },
        ids.bras,
        2000.0)
    frames.ncrm = run_alloy(
        "nichrome",
        { ids.nicl, ids.nicl, ids.nicl, ids.nicl, ids.chrm },
        ids.ncrm,
        2700.0)
    frames.almg = run_alloy(
        "aluminium-magnesium",
        { ids.alum, ids.alum, ids.alum, ids.alum, ids.magn },
        ids.almg,
        1400.0)
    frames.ssil = run_alloy(
        "stainless steel",
        { ids.stel, ids.stel, ids.stel, ids.stel, ids.chrm, ids.nicl },
        ids.ssil,
        2700.0)
    frames.tstl = run_alloy(
        "tool steel",
        { ids.stel, ids.stel, ids.stel, ids.stel, ids.cobt, ids.moly },
        ids.tstl,
        3500.0)
    frames.sold = run_alloy(
        "tin-lead solder",
        { ids.tin, ids.tin, ids.tin, ids.lead, ids.lead },
        ids.sold,
        900.0)
    frames.stel = run_steel()
    run_carbonization()
    run_negative_control()
    run_material_behaviors()
    run_radiation_shield_assembly()
    return frames
end

local ok, data = xpcall(test, debug.traceback)
local report = assert(io.open(RESULT, "w"))
if ok then
    report:write("OMNI_METALLURGY_STATUS=PASS\n")
    report:write("OMNI_METALLURGY_RECIPES=9\n")
    report:write("OMNI_METALLURGY_BEHAVIORS=7\n")
    report:write(
        "OMNI_METALLURGY_IDS=" .. ids.alum .. "-" .. ids.cruc .. "\n")
    report:write("OMNI_METALLURGY_CANONICAL_SCRAP=" .. ids.brmt .. "\n")
    report:write("OMNI_METALLURGY_LEGACY_ALIAS=" .. ids.legacy_mscr .. "\n")
    report:write("OMNI_METALLURGY_HIGH_ID=" .. ids.sold .. "\n")
    report:write(
        "OMNI_METALLURGY_FRAMES="
        .. data.brnz .. "," .. data.bras .. "," .. data.ncrm .. ","
        .. data.almg .. "," .. data.ssil .. "," .. data.tstl .. ","
        .. data.stel .. "," .. data.sold .. "\n")
else
    local error_text = tostring(data):gsub("[\r\n]+", " | ")
    report:write("OMNI_METALLURGY_STATUS=FAIL\n")
    report:write("OMNI_METALLURGY_ERROR=" .. error_text .. "\n")
end
report:close()
