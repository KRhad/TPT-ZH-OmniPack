local RESULT = "lua-alchemy-gate.result"

local function read_mode()
    local file = assert(io.open("alchemy-mode.txt", "r"))
    local value = assert(file:read("*l"))
    file:close()
    assert(value == "locked" or value == "free" or value == "stamp-source",
        "invalid alchemy test mode")
    return value
end

local ids = {
    none = assert(elements.DEFAULT_PT_NONE),
    dust = assert(elements.DEFAULT_PT_DUST),
    fire = assert(elements.DEFAULT_PT_FIRE),
    water = assert(elements.DEFAULT_PT_WATR),
}

local function assert_rejected_without_mutation(label, action)
    local before = sim.partCount()
    local ok, message = pcall(action)
    assert(not ok, label .. " unexpectedly succeeded")
    assert(tostring(message):find("locked by alchemy progress", 1, true),
        label .. " returned the wrong rejection: " .. tostring(message))
    assert(sim.partCount() == before, label .. " mutated particle count before rejection")
end

local function run_locked()
    sim.clearSim()
    local progress = sim.omniAlchemyProgress()
    assert(progress.schema_version == 1
        and progress.completed_stage_count == 0
        and progress.stage_count == 10
        and progress.mastered == false
        and progress.current_stage_id == "A01-STEAM-CYCLE",
        "unexpected initial progress contract")
    assert(sim.omniAlchemyUnlocked(ids.fire), "FIRE is not initially unlocked")
    assert(sim.omniAlchemyUnlocked(ids.water), "WATR is not initially unlocked")
    assert(not sim.omniAlchemyUnlocked(ids.dust), "DUST is initially unlocked")

    assert_rejected_without_mutation("partCreate", function()
        sim.partCreate(-1, 20, 20, ids.dust)
    end)
    assert_rejected_without_mutation("createParts", function()
        sim.createParts(30, 30, 1, 1, ids.dust)
    end)
    assert_rejected_without_mutation("createLine", function()
        sim.createLine(40, 40, 44, 40, 0, 0, ids.dust)
    end)
    assert_rejected_without_mutation("createBox", function()
        sim.createBox(50, 50, 54, 54, ids.dust)
    end)
    assert_rejected_without_mutation("floodParts", function()
        sim.floodParts(60, 60, ids.dust)
    end)
    local dust_tool = assert(tools.index["DEFAULT_PT_DUST"])
    assert_rejected_without_mutation("toolBrush", function()
        sim.toolBrush(80, 80, 1, 1, dust_tool, 0, 1.0)
    end)
    assert_rejected_without_mutation("toolLine", function()
        sim.toolLine(82, 82, 86, 82, 0, 0, dust_tool, 0, 1.0)
    end)
    assert_rejected_without_mutation("toolBox", function()
        sim.toolBox(88, 88, 92, 92, dust_tool, 1.0, 0, 0, 0)
    end)

    local particle = assert(sim.partCreate(-1, 70, 70, ids.fire))
    assert(particle >= 0 and sim.partProperty(particle, "type") == ids.fire,
        "initial FIRE creation failed")
    assert_rejected_without_mutation("partChangeType", function()
        sim.partChangeType(particle, ids.dust)
    end)
    assert(sim.partProperty(particle, "type") == ids.fire,
        "partChangeType changed type before rejection")
    assert_rejected_without_mutation("partProperty(type)", function()
        sim.partProperty(particle, "type", ids.dust)
    end)
    assert(sim.partProperty(particle, "type") == ids.fire,
        "partProperty changed type before rejection")

    local stamp_file = io.open("locked-stamp.txt", "r")
    if stamp_file then
        local stamp_path = assert(stamp_file:read("*l"))
        stamp_file:close()
        assert_rejected_without_mutation("loadStamp", function()
            sim.loadStamp(stamp_path, 100, 100)
        end)
    else
        error("locked stamp fixture is missing")
    end

    ui.activeTool(0, "DEFAULT_PT_DUST")
    assert(ui.activeTool(0) ~= "DEFAULT_PT_DUST", "active-tool gate selected locked DUST")
    ui.activeTool(0, "DEFAULT_PT_FIRE")
    assert(ui.activeTool(0) == "DEFAULT_PT_FIRE", "active-tool gate rejected initial FIRE")
    return 11
end

local function run_free()
    sim.clearSim()
    assert(not sim.omniAlchemyUnlocked(ids.dust),
        "raw discovery state should remain initial when restrictions are off")
    local particle = assert(sim.partCreate(-1, 20, 20, ids.dust))
    assert(particle >= 0 and sim.partProperty(particle, "type") == ids.dust,
        "normal sandbox partCreate was restricted")
    sim.partChangeType(particle, ids.fire)
    assert(sim.partProperty(particle, "type") == ids.fire,
        "normal sandbox partChangeType was restricted")
    sim.partProperty(particle, "type", ids.dust)
    assert(sim.partProperty(particle, "type") == ids.dust,
        "normal sandbox partProperty(type) was restricted")
    assert(sim.createParts(30, 30, 1, 1, ids.dust) ~= nil,
        "normal sandbox createParts was restricted")
    sim.createLine(40, 40, 44, 40, 0, 0, ids.dust)
    sim.createBox(50, 50, 54, 54, ids.dust)
    local dust_tool = assert(tools.index["DEFAULT_PT_DUST"])
    sim.toolBrush(80, 80, 1, 1, dust_tool, 0, 1.0)
    sim.toolLine(82, 82, 86, 82, 0, 0, dust_tool, 0, 1.0)
    sim.toolBox(88, 88, 92, 92, dust_tool, 1.0, 0, 0, 0)
    ui.activeTool(0, "DEFAULT_PT_DUST")
    assert(ui.activeTool(0) == "DEFAULT_PT_DUST",
        "normal sandbox active-tool selection was restricted")
    return 10
end

local function run_stamp_source()
    sim.clearSim()
    assert(sim.partCreate(-1, 20, 20, ids.dust) >= 0,
        "could not create locked-element stamp fixture")
    local stamp_id = assert(sim.saveStamp(16, 16, 16, 16, 1))
    assert(#stamp_id > 0, "saveStamp returned an empty fixture ID")
    return stamp_id
end

local mode = read_mode()
local ok, result = xpcall(function()
    if mode == "locked" then return run_locked() end
    if mode == "free" then return run_free() end
    return run_stamp_source()
end, debug.traceback)
local report = assert(io.open(RESULT, "w"))
if ok then
    report:write("OMNI_ALCHEMY_GATE_STATUS=PASS\n")
    report:write("OMNI_ALCHEMY_GATE_MODE=" .. mode .. "\n")
    if mode == "stamp-source" then
        report:write("OMNI_ALCHEMY_STAMP_ID=" .. tostring(result) .. "\n")
        report:write("OMNI_ALCHEMY_GATE_ASSERTIONS=1\n")
    else
        report:write("OMNI_ALCHEMY_GATE_ASSERTIONS=" .. tostring(result) .. "\n")
    end
else
    report:write("OMNI_ALCHEMY_GATE_STATUS=FAIL\n")
    report:write("OMNI_ALCHEMY_GATE_MODE=" .. mode .. "\n")
    report:write("OMNI_ALCHEMY_GATE_ERROR=" .. tostring(result):gsub("[\r\n]+", " | ") .. "\n")
end
report:close()
