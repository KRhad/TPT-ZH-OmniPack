local PHASE_FILE = "element-alias.phase"
local STATE_FILE = "element-alias.state"
local phase_file = assert(io.open(PHASE_FILE, "rb"), "missing alias phase")
local phase = tonumber(assert(phase_file:read("*a")))
phase_file:close()
assert(phase == 1 or phase == 2 or phase == 3, "invalid alias phase")

local RESULT = "element-alias-phase" .. phase .. ".result"
local RECOVERABLE_SCRAP_MARKER = 0x4F4D5343
local ids = {
    alias = assert(elements.OMNI_PT_MSCR),
    brmt = assert(elements.DEFAULT_PT_BRMT),
    iron = assert(elements.DEFAULT_PT_IRON),
    dust = assert(elements.DEFAULT_PT_DUST),
    conv = assert(elements.DEFAULT_PT_CONV),
}
assert(ids.alias == 278, "legacy alias stable ID changed")
assert(ids.brmt == 30, "canonical BRMT stable ID changed")

local function configure()
    sim.clearSim()
    sim.paused(true)
    sim.gravityMode(sim.GRAV_OFF)
    sim.airMode(sim.AIR_OFF)
    sim.ambientHeatSim(false)
    sim.heatSim(true)
    sim.ensureDeterminism(true)
    sim.randomSeed(31, 32, 33, 34)
end

local function save_all()
    local stamp = sim.saveStamp(0, 0, sim.XRES - 1, sim.YRES - 1, 1)
    assert(type(stamp) == "string" and stamp:match("^[0-9A-Fa-f]+$")
            and #stamp == 10,
        "failed to save alias OPS fixture")
    return stamp
end

local function read_state()
    local file = assert(io.open(STATE_FILE, "rb"), "missing alias state")
    local text = assert(file:read("*a"))
    file:close()
    local state = {}
    for key, value in text:gmatch("([%w_]+)=([^\r\n]+)") do
        state[key] = value
    end
    return state
end

local function write_state(stamp1, stamp2)
    local file = assert(io.open(STATE_FILE, "wb"))
    file:write("stamp1=" .. stamp1 .. "\n")
    if stamp2 then file:write("stamp2=" .. stamp2 .. "\n") end
    file:close()
end

local function load_stamp(stamp)
    configure()
    local loaded, load_error = sim.loadStamp(stamp, 0, 0, false, 0, 1)
    assert(loaded == 1, "failed to load alias OPS: " .. tostring(load_error))
end

local function phase_one()
    configure()
    assert(elements.property(ids.alias, "MenuVisible") == 0,
        "legacy alias is visible")
    local direct_ok, direct_error = pcall(
        sim.partCreate, -1, 110, 110, ids.alias)
    assert(not direct_ok and tostring(direct_error):find("unavailable element", 1, true),
        "legacy alias direct creation was not blocked")
    local active_before = ui.activeTool(0)
    ui.activeTool(0, "OMNI_PT_MSCR")
    assert(ui.activeTool(0) == active_before,
        "legacy alias became the active tool")

    local particle = sim.partCreate(-1, 121, 120, ids.dust)
    local converter = sim.partCreate(-1, 120, 120, ids.conv)
    assert(particle >= 0 and converter >= 0,
        "failed to create carried-alias fixture")
    sim.partProperty(converter, "ctype", ids.alias)
    sim.updateUpTo()
    assert(sim.partProperty(particle, "type") == ids.alias,
        "CONV did not create the internal compatibility alias")
    sim.partKill(converter)
    local stamp1 = save_all()
    write_state(stamp1, nil)
    return stamp1, "legacy-alias-saved"
end

local function phase_two()
    local state = read_state()
    assert(state.stamp1 and #state.stamp1 == 10, "missing stamp1")
    load_stamp(state.stamp1)
    local particle = assert(sim.partID(121, 120), "legacy alias particle missing")
    assert(sim.partProperty(particle, "type") == ids.alias,
        "legacy alias changed before the first simulation tick")
    sim.updateUpTo()
    assert(sim.partProperty(particle, "type") == ids.brmt
            and sim.partProperty(particle, "ctype") == ids.iron
            and sim.partProperty(particle, "tmp4") == RECOVERABLE_SCRAP_MARKER,
        "legacy alias did not migrate to canonical recoverable BRMT")
    local stamp2 = save_all()
    assert(stamp2 ~= state.stamp1, "canonical resave reused stamp1")
    write_state(state.stamp1, stamp2)
    return stamp2, "legacy-load-migrate-resave"
end

local function phase_three()
    local state = read_state()
    assert(state.stamp2 and #state.stamp2 == 10, "missing stamp2")
    load_stamp(state.stamp2)
    local particle = assert(sim.partID(121, 120), "canonical BRMT missing")
    assert(sim.partProperty(particle, "type") == ids.brmt
            and sim.partProperty(particle, "ctype") == ids.iron
            and sim.partProperty(particle, "tmp4") == RECOVERABLE_SCRAP_MARKER,
        "canonical BRMT fields changed after OPS restart")
    local alias_count = 0
    for item in sim.parts() do
        if sim.partProperty(item, "type") == ids.alias then
            alias_count = alias_count + 1
        end
    end
    assert(alias_count == 0, "canonical resave still contains alias particles")
    return state.stamp2, "canonical-reload-verified"
end

local ok, stamp, operation = xpcall(function()
    if phase == 1 then return phase_one() end
    if phase == 2 then return phase_two() end
    return phase_three()
end, debug.traceback)

local report = assert(io.open(RESULT, "wb"))
if ok then
    report:write("OMNI_ALIAS_STATUS=PASS\n")
    report:write("OMNI_ALIAS_PHASE=" .. phase .. "\n")
    report:write("OMNI_ALIAS_OPERATION=" .. operation .. "\n")
    report:write("OMNI_ALIAS_STAMP=" .. stamp .. "\n")
    report:write("OMNI_ALIAS_IDENTIFIER=OMNI_PT_MSCR\n")
    report:write("OMNI_ALIAS_STABLE_ID=" .. ids.alias .. "\n")
    report:write("OMNI_ALIAS_CANONICAL=DEFAULT_PT_BRMT\n")
    report:write("OMNI_ALIAS_CANONICAL_ID=" .. ids.brmt .. "\n")
    report:write("OMNI_ALIAS_MARKER=" .. RECOVERABLE_SCRAP_MARKER .. "\n")
else
    report:write("OMNI_ALIAS_STATUS=FAIL\n")
    report:write("OMNI_ALIAS_PHASE=" .. phase .. "\n")
    report:write("OMNI_ALIAS_ERROR=" .. tostring(stamp):gsub("[\r\n]+", " | ") .. "\n")
end
report:close()
os.exit(ok and 0 or 1)
