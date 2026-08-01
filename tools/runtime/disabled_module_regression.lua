local PHASE = assert(tonumber(assert(io.open("disabled-module.phase", "rb")):read("*a")),
    "missing disabled-module phase")
local RESULT = "disabled-module-phase" .. PHASE .. ".result"
local STATE = "disabled-module.state"

local function must_element(identifier, expected)
    local id = assert(elements[identifier], "missing element " .. identifier)
    assert(id == expected,
        identifier .. " stable ID changed: expected " .. expected .. ", got " .. id)
    return id
end

local ids = {
    aluminium = must_element("OMNI_PT_ALUM", 256),
    scrap = must_element("DEFAULT_PT_BRMT", 30),
    pathogen = must_element("OMNI_PT_PATH", 292),
    sterilizer = must_element("OMNI_PT_STER", 293),
    coolant = must_element("OMNI_PT_NCLT", 331),
    waste = must_element("OMNI_PT_NWST", 332),
    chlorine = must_element("OMNI_PT_CHLR", 360),
    hydrochloric = must_element("OMNI_PT_HCLA", 462),
    sodium_hydroxide = must_element("OMNI_PT_NAOH", 467),
    carbonic = must_element("OMNI_PT_CARA", 478),
    ammonium_chloride = must_element("OMNI_PT_AMCL", 511),
    engineering = must_element("OMNI_PT_NITI", 520),
    material = must_element("OMNI_PT_RFBK", 532),
    isotope = must_element("OMNI_PT_CF52", 588),
    hydrogen2 = must_element("OMNI_PT_H2IS", 576),
    fats = must_element("OMNI_PT_FATS", 601),
    epoxy_resin = must_element("OMNI_PT_ERES", 612),
    epoxy = must_element("OMNI_PT_EPXY", 618),
    ethyl_acetate = must_element("OMNI_PT_EACT", 621),
    dielectric = must_element("OMNI_PT_DIEL", 641),
    detergent = must_element("OMNI_PT_DETG", 685),
    catalyst = must_element("OMNI_PT_CATA", 366),
    caustic = must_element("DEFAULT_PT_CAUS", 86),
    helium = must_element("OMNI_PT_HE", 370),
    fire = assert(elements.DEFAULT_PT_FIRE),
    metal = assert(elements.DEFAULT_PT_METL),
    oil = assert(elements.DEFAULT_PT_OIL),
    water = assert(elements.DEFAULT_PT_WATR),
}

local RECOVERABLE_SCRAP_MARKER = 0x4F4D5343

local function configure()
    sim.clearSim()
    sim.paused(true)
    sim.gravityMode(sim.GRAV_OFF)
    sim.airMode(sim.AIR_OFF)
    sim.ambientHeatSim(false)
    sim.heatSim(true)
    sim.ensureDeterminism(true)
    sim.randomSeed(21, 22, 23, 24)
    sim.resetOmniEventMetrics()
end

local function write_result(ok, values)
    local report = assert(io.open(RESULT, "wb"))
    report:write("OMNI_DISABLED_MODULE_STATUS=" .. (ok and "PASS" or "FAIL") .. "\n")
    if ok then
        for _, entry in ipairs(values) do
            report:write(entry .. "\n")
        end
    else
        report:write("OMNI_DISABLED_MODULE_ERROR="
            .. tostring(values):gsub("[\r\n]+", " | ") .. "\n")
    end
    report:close()
end

local function phase_one()
    configure()
    local acid = sim.partCreate(-1, 120, 120, ids.hydrochloric)
    local base = sim.partCreate(-1, 121, 120, ids.sodium_hydroxide)
    local sterilizer = sim.partCreate(-1, 140, 120, ids.sterilizer)
    local pathogen = sim.partCreate(-1, 141, 120, ids.pathogen)
    local coolant = sim.partCreate(-1, 160, 120, ids.coolant)
    local waste = sim.partCreate(-1, 161, 120, ids.waste)
    local scrap = sim.partCreate(-1, 180, 120, ids.scrap)
    local carbonic = sim.partCreate(-1, 200, 120, ids.carbonic)
    local ammonium_chloride = sim.partCreate(-1, 220, 120, ids.ammonium_chloride)
    local engineering = sim.partCreate(-1, 240, 120, ids.engineering)
    local material = sim.partCreate(-1, 260, 120, ids.material)
    local isotope = sim.partCreate(-1, 280, 120, ids.isotope)
    local hydrogen2 = sim.partCreate(-1, 300, 120, ids.hydrogen2)
    local fire = sim.partCreate(-1, 301, 120, ids.fire)
    local fats = sim.partCreate(-1, 320, 120, ids.fats)
    local caustic = sim.partCreate(-1, 321, 120, ids.caustic)
    local ethyl_acetate = sim.partCreate(-1, 340, 120, ids.ethyl_acetate)
    local resin_first = sim.partCreate(-1, 360, 120, ids.epoxy_resin)
    local resin_second = sim.partCreate(-1, 361, 120, ids.epoxy_resin)
    local catalyst = sim.partCreate(-1, 360, 121, ids.catalyst)
    local dielectric = sim.partCreate(-1, 380, 120, ids.dielectric)
    local metal = sim.partCreate(-1, 381, 120, ids.metal)
    local detergent = sim.partCreate(-1, 400, 120, ids.detergent)
    local oil = sim.partCreate(-1, 401, 120, ids.oil)
    local water = sim.partCreate(-1, 400, 121, ids.water)
    assert(acid >= 0 and base >= 0 and sterilizer >= 0 and pathogen >= 0
            and coolant >= 0 and waste >= 0 and scrap >= 0 and carbonic >= 0
            and ammonium_chloride >= 0 and engineering >= 0 and material >= 0
            and isotope >= 0 and hydrogen2 >= 0 and fire >= 0
            and fats >= 0 and caustic >= 0 and ethyl_acetate >= 0
            and resin_first >= 0 and resin_second >= 0 and catalyst >= 0
            and dielectric >= 0 and metal >= 0 and detergent >= 0
            and oil >= 0 and water >= 0,
        "failed to create enabled module fixtures")
    sim.partProperty(acid, "temp", 300.0)
    sim.partProperty(base, "temp", 300.0)
    sim.partProperty(waste, "temp", 1200.0)
    sim.partProperty(scrap, "ctype", ids.aluminium)
    sim.partProperty(scrap, "tmp4", RECOVERABLE_SCRAP_MARKER)
    sim.partProperty(scrap, "temp", 1200.0)
    sim.partProperty(ammonium_chloride, "temp", 550.0)
    sim.partProperty(fats, "temp", 380.0)
    sim.partProperty(caustic, "temp", 380.0)
    sim.partProperty(ethyl_acetate, "temp", 340.0)
    sim.partProperty(resin_first, "temp", 380.0)
    sim.partProperty(resin_second, "temp", 380.0)
    sim.partProperty(catalyst, "temp", 380.0)
    sim.partProperty(dielectric, "tmp", 8)
    local stamp = sim.saveStamp(0, 0, sim.XRES - 1, sim.YRES - 1, 1)
    assert(type(stamp) == "string" and stamp:match("^[0-9A-Fa-f]+$") and #stamp == 10,
        "failed to save enabled chemistry OPS fixture")
    local state = assert(io.open(STATE, "wb"))
    state:write(stamp .. "\n")
    state:close()
    return {
        "OMNI_DISABLED_MODULE_PHASE=1",
        "OMNI_DISABLED_MODULES=metallurgy,biology,chemistry,advanced_nuclear,electronics",
        "OMNI_DISABLED_MODULE_STAMP=" .. stamp,
        "OMNI_DISABLED_MODULE_FIXTURE_PARTICLES=25",
    }
end

local function phase_two()
    local state = assert(io.open(STATE, "rb"), "missing enabled fixture state")
    local stamp = assert(state:read("*a")):match("^%s*([0-9A-Fa-f]+)%s*$")
    state:close()
    assert(stamp and #stamp == 10, "invalid enabled fixture stamp")

    local disabled_representatives = {
        { "OMNI_PT_ALUM", ids.aluminium },
        { "OMNI_PT_NITI", ids.engineering },
        { "OMNI_PT_RFBK", ids.material },
        { "OMNI_PT_CF52", ids.isotope },
        { "OMNI_PT_FATS", ids.fats },
        { "OMNI_PT_EACT", ids.ethyl_acetate },
        { "OMNI_PT_DIEL", ids.dielectric },
        { "OMNI_PT_DETG", ids.detergent },
        { "OMNI_PT_STER", ids.sterilizer },
        { "OMNI_PT_CHLR", ids.chlorine },
        { "OMNI_PT_HCLA", ids.hydrochloric },
        { "OMNI_PT_CARA", ids.carbonic },
        { "OMNI_PT_AMCL", ids.ammonium_chloride },
        { "OMNI_PT_NCLT", ids.coolant },
    }
    local active_before = ui.activeTool(0)
    for index, representative in ipairs(disabled_representatives) do
        ui.activeTool(0, representative[1])
        assert(ui.activeTool(0) == active_before,
            "disabled module element became the active tool: " .. representative[1])
        local created, creation_error = pcall(function()
            sim.partCreate(-1, 99 + index, 100, representative[2])
        end)
        assert(not created and tostring(creation_error):find("disabled module", 1, true),
            "disabled creation was not rejected with a module reason: "
                .. representative[1])
    end

    ui.activeTool(0, "OMNI_PT_HE")
    assert(ui.activeTool(0) == "OMNI_PT_HE",
        "periodic helium selection was blocked with content modules disabled")
    local helium = sim.partCreate(-1, 102, 100, ids.helium)
    assert(helium >= 0, "periodic creation was blocked with chemistry disabled")

    configure()
    local loaded, load_error = sim.loadStamp(stamp, 0, 0, false, 0, 1)
    assert(loaded == 1, "disabled-module OPS load failed: " .. tostring(load_error))
    local acid = assert(sim.partID(120, 120), "loaded hydrochloric acid is missing")
    local base = assert(sim.partID(121, 120), "loaded sodium hydroxide is missing")
    local sterilizer = assert(sim.partID(140, 120), "loaded sterilizer is missing")
    local pathogen = assert(sim.partID(141, 120), "loaded pathogen is missing")
    local coolant = assert(sim.partID(160, 120), "loaded coolant is missing")
    local waste = assert(sim.partID(161, 120), "loaded nuclear waste is missing")
    local scrap = assert(sim.partID(180, 120), "loaded metal scrap is missing")
    local carbonic = assert(sim.partID(200, 120), "loaded carbonic acid is missing")
    local ammonium_chloride = assert(
        sim.partID(220, 120), "loaded ammonium chloride is missing")
    local engineering = assert(
        sim.partID(240, 120), "loaded engineering alloy is missing")
    local material = assert(
        sim.partID(260, 120), "loaded refractory material is missing")
    local isotope = assert(
        sim.partID(280, 120), "loaded isotope material is missing")
    local hydrogen2 = assert(
        sim.partID(300, 120), "loaded hydrogen-2 isotope is missing")
    local fats = assert(sim.partID(320, 120), "loaded fats particle is missing")
    local caustic = assert(
        sim.partID(321, 120), "loaded caustic particle is missing")
    local ethyl_acetate = assert(
        sim.partID(340, 120), "loaded ethyl acetate particle is missing")
    local resin_first = assert(
        sim.partID(360, 120), "loaded first epoxy resin particle is missing")
    local resin_second = assert(
        sim.partID(361, 120), "loaded second epoxy resin particle is missing")
    local catalyst = assert(
        sim.partID(360, 121), "loaded organic catalyst particle is missing")
    local dielectric = assert(
        sim.partID(380, 120), "loaded dielectric ceramic is missing")
    local metal = assert(sim.partID(381, 120), "loaded dielectric conductor is missing")
    local detergent = assert(sim.partID(400, 120), "loaded detergent is missing")
    local oil = assert(sim.partID(401, 120), "loaded oil fixture is missing")
    local water = assert(sim.partID(400, 121), "loaded wash-water fixture is missing")
    assert(sim.partProperty(acid, "type") == ids.hydrochloric
            and sim.partProperty(base, "type") == ids.sodium_hydroxide
            and sim.partProperty(carbonic, "type") == ids.carbonic
            and sim.partProperty(ammonium_chloride, "type") == ids.ammonium_chloride,
        "disabled-module OPS load changed or deleted chemistry particles")
    assert(sim.partProperty(engineering, "type") == ids.engineering,
        "disabled-module OPS load changed or deleted high-ID engineering alloy")
    assert(sim.partProperty(material, "type") == ids.material,
        "disabled-module OPS load changed or deleted high-ID material")
    assert(sim.partProperty(isotope, "type") == ids.isotope,
        "disabled-module OPS load changed or deleted high-ID isotope")
    assert(sim.partProperty(ethyl_acetate, "type") == ids.ethyl_acetate
            and sim.partProperty(resin_first, "type") == ids.epoxy_resin
            and sim.partProperty(resin_second, "type") == ids.epoxy_resin
            and sim.partProperty(catalyst, "type") == ids.catalyst,
        "disabled-module OPS load changed or deleted organic batch 2 particles")
    assert(sim.partProperty(dielectric, "type") == ids.dielectric
            and sim.partProperty(dielectric, "tmp") == 8
            and sim.partProperty(metal, "type") == ids.metal,
        "disabled-module OPS load changed the electronics fixture")
    assert(sim.partProperty(detergent, "type") == ids.detergent
            and sim.partProperty(oil, "type") == ids.oil
            and sim.partProperty(water, "type") == ids.water,
        "disabled-module OPS load changed the environmental fixture")
    sim.updateUpTo()
    assert(sim.partProperty(acid, "type") == ids.hydrochloric
            and sim.partProperty(base, "type") == ids.sodium_hydroxide
            and sim.partProperty(carbonic, "type") == ids.carbonic
            and sim.partProperty(ammonium_chloride, "type") == ids.ammonium_chloride,
        "disabled chemistry particles continued reacting after OPS load")
    assert(sim.partProperty(engineering, "type") == ids.engineering,
        "disabled metallurgy high-ID particle continued updating after OPS load")
    assert(sim.partProperty(material, "type") == ids.material,
        "disabled metallurgy material continued updating after OPS load")
    assert(sim.partProperty(isotope, "type") == ids.isotope,
        "disabled nuclear isotope continued updating after OPS load")
    assert(sim.partProperty(hydrogen2, "type") == ids.hydrogen2,
        "disabled hydrogen-2 isotope reacted with fire after OPS load")
    assert(sim.partProperty(fats, "type") == ids.fats
            and sim.partProperty(caustic, "type") == ids.caustic,
        "disabled organic saponification continued after OPS load")
    assert(sim.partProperty(ethyl_acetate, "type") == ids.ethyl_acetate
            and sim.partProperty(resin_first, "type") == ids.epoxy_resin
            and sim.partProperty(resin_second, "type") == ids.epoxy_resin
            and sim.partProperty(catalyst, "type") == ids.catalyst,
        "disabled organic batch 2 curing or phase behavior continued after OPS load")
    assert(sim.partProperty(dielectric, "type") == ids.dielectric
            and sim.partProperty(dielectric, "tmp") == 8
            and sim.partProperty(metal, "type") == ids.metal,
        "disabled dielectric ceramic discharged after OPS load")
    assert(sim.partProperty(detergent, "type") == ids.detergent
            and sim.partProperty(oil, "type") == ids.oil
            and sim.partProperty(water, "type") == ids.water,
        "disabled environmental materials continued reacting after OPS load")
    assert(sim.partProperty(sterilizer, "type") == ids.sterilizer
            and sim.partProperty(pathogen, "type") == ids.pathogen,
        "disabled biology particles continued reacting after OPS load")
    assert(sim.partProperty(coolant, "type") == ids.coolant
            and sim.partProperty(waste, "type") == ids.waste,
        "disabled nuclear particles continued reacting after OPS load")
    assert(sim.partProperty(scrap, "type") == ids.scrap
            and sim.partProperty(scrap, "ctype") == ids.aluminium
            and sim.partProperty(scrap, "tmp4") == RECOVERABLE_SCRAP_MARKER,
        "disabled metallurgy scrap continued updating after OPS load")
    local metrics = sim.omniEventMetrics()
    assert(tonumber(metrics.total) == 0,
        "disabled chemistry update recorded events: " .. tostring(metrics.total))

    return {
        "OMNI_DISABLED_MODULE_PHASE=2",
        "OMNI_DISABLED_MODULES=metallurgy,biology,chemistry,advanced_nuclear,electronics",
        "OMNI_DISABLED_MODULE_METALLURGY=OMNI_PT_ALUM",
        "OMNI_DISABLED_MODULE_ENGINEERING=OMNI_PT_NITI",
        "OMNI_DISABLED_MODULE_MATERIAL=OMNI_PT_RFBK",
        "OMNI_DISABLED_MODULE_SCRAP=DEFAULT_PT_BRMT",
        "OMNI_DISABLED_MODULE_BIOLOGY=OMNI_PT_STER",
        "OMNI_DISABLED_MODULE_CORE=OMNI_PT_CHLR",
        "OMNI_DISABLED_MODULE_EXPANSION=OMNI_PT_HCLA",
        "OMNI_DISABLED_MODULE_BATCH2=OMNI_PT_CARA",
        "OMNI_DISABLED_MODULE_BATCH3=OMNI_PT_AMCL",
        "OMNI_DISABLED_MODULE_NUCLEAR=OMNI_PT_NCLT",
        "OMNI_DISABLED_MODULE_ISOTOPE=OMNI_PT_CF52",
        "OMNI_DISABLED_MODULE_ORGANIC=OMNI_PT_EACT",
        "OMNI_DISABLED_MODULE_ELECTRONICS=OMNI_PT_DIEL",
        "OMNI_DISABLED_MODULE_ENVIRONMENT=OMNI_PT_DETG",
        "OMNI_DISABLED_MODULE_LOADED_PARTICLES=25",
        "OMNI_DISABLED_MODULE_UPDATE_EVENTS=0",
        "OMNI_DISABLED_MODULE_PERIODIC_ACTIVE=OMNI_PT_HE",
        "OMNI_DISABLED_MODULE_OPS_FORMAT=OPS1",
    }
end

local ok, data = xpcall(function()
    assert(PHASE == 1 or PHASE == 2, "invalid disabled-module phase")
    return PHASE == 1 and phase_one() or phase_two()
end, debug.traceback)
write_result(ok, data)
os.exit(ok and 0 or 1)
