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
    scrap = must_element("OMNI_PT_MSCR", 278),
    pathogen = must_element("OMNI_PT_PATH", 292),
    sterilizer = must_element("OMNI_PT_STER", 293),
    coolant = must_element("OMNI_PT_NCLT", 331),
    waste = must_element("OMNI_PT_NWST", 332),
    chlorine = must_element("OMNI_PT_CHLR", 360),
    hydrochloric = must_element("OMNI_PT_HCLA", 462),
    sodium_hydroxide = must_element("OMNI_PT_NAOH", 467),
    helium = must_element("OMNI_PT_HE", 370),
}

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
    assert(acid >= 0 and base >= 0 and sterilizer >= 0 and pathogen >= 0
            and coolant >= 0 and waste >= 0 and scrap >= 0,
        "failed to create enabled module fixtures")
    sim.partProperty(acid, "temp", 300.0)
    sim.partProperty(base, "temp", 300.0)
    sim.partProperty(waste, "temp", 1200.0)
    sim.partProperty(scrap, "ctype", ids.aluminium)
    sim.partProperty(scrap, "temp", 1200.0)
    local stamp = sim.saveStamp(0, 0, sim.XRES - 1, sim.YRES - 1, 1)
    assert(type(stamp) == "string" and stamp:match("^[0-9A-Fa-f]+$") and #stamp == 10,
        "failed to save enabled chemistry OPS fixture")
    local state = assert(io.open(STATE, "wb"))
    state:write(stamp .. "\n")
    state:close()
    return {
        "OMNI_DISABLED_MODULE_PHASE=1",
        "OMNI_DISABLED_MODULES=metallurgy,biology,chemistry,advanced_nuclear",
        "OMNI_DISABLED_MODULE_STAMP=" .. stamp,
        "OMNI_DISABLED_MODULE_FIXTURE_PARTICLES=7",
    }
end

local function phase_two()
    local state = assert(io.open(STATE, "rb"), "missing enabled fixture state")
    local stamp = assert(state:read("*a")):match("^%s*([0-9A-Fa-f]+)%s*$")
    state:close()
    assert(stamp and #stamp == 10, "invalid enabled fixture stamp")

    local disabled_representatives = {
        { "OMNI_PT_MSCR", ids.scrap },
        { "OMNI_PT_STER", ids.sterilizer },
        { "OMNI_PT_CHLR", ids.chlorine },
        { "OMNI_PT_HCLA", ids.hydrochloric },
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
        "always-available periodic selection was blocked with chemistry disabled")
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
    assert(sim.partProperty(acid, "type") == ids.hydrochloric
            and sim.partProperty(base, "type") == ids.sodium_hydroxide,
        "disabled-module OPS load changed or deleted chemistry particles")
    sim.updateUpTo()
    assert(sim.partProperty(acid, "type") == ids.hydrochloric
            and sim.partProperty(base, "type") == ids.sodium_hydroxide,
        "disabled chemistry particles continued reacting after OPS load")
    assert(sim.partProperty(sterilizer, "type") == ids.sterilizer
            and sim.partProperty(pathogen, "type") == ids.pathogen,
        "disabled biology particles continued reacting after OPS load")
    assert(sim.partProperty(coolant, "type") == ids.coolant
            and sim.partProperty(waste, "type") == ids.waste,
        "disabled nuclear particles continued reacting after OPS load")
    assert(sim.partProperty(scrap, "type") == ids.scrap
            and sim.partProperty(scrap, "ctype") == ids.aluminium,
        "disabled metallurgy scrap continued updating after OPS load")
    local metrics = sim.omniEventMetrics()
    assert(tonumber(metrics.total) == 0,
        "disabled chemistry update recorded events: " .. tostring(metrics.total))

    return {
        "OMNI_DISABLED_MODULE_PHASE=2",
        "OMNI_DISABLED_MODULES=metallurgy,biology,chemistry,advanced_nuclear",
        "OMNI_DISABLED_MODULE_METALLURGY=OMNI_PT_MSCR",
        "OMNI_DISABLED_MODULE_BIOLOGY=OMNI_PT_STER",
        "OMNI_DISABLED_MODULE_CORE=OMNI_PT_CHLR",
        "OMNI_DISABLED_MODULE_EXPANSION=OMNI_PT_HCLA",
        "OMNI_DISABLED_MODULE_NUCLEAR=OMNI_PT_NCLT",
        "OMNI_DISABLED_MODULE_LOADED_PARTICLES=7",
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
