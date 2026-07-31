local RESULT = "lua-alchemy-progression.result"

local mode_file = assert(io.open("alchemy-progression-mode.txt", "r"))
local progression_mode = assert(mode_file:read("*l"))
mode_file:close()
assert(progression_mode == "modules-on" or progression_mode == "modules-off",
    "invalid progression mode")
local modules_off = progression_mode == "modules-off"

local ids = {
    fire = assert(elements.DEFAULT_PT_FIRE),
    water = assert(elements.DEFAULT_PT_WATR),
    stone = assert(elements.DEFAULT_PT_STNE),
    oxygen = assert(elements.DEFAULT_PT_O2),
    vapor = assert(elements.DEFAULT_PT_WTRV),
    lava = assert(elements.DEFAULT_PT_LAVA),
    sand = assert(elements.DEFAULT_PT_SAND),
    brick = assert(elements.DEFAULT_PT_BRCK),
    metal = assert(elements.DEFAULT_PT_METL),
    battery = assert(elements.DEFAULT_PT_BTRY),
    pump = assert(elements.DEFAULT_PT_PUMP),
    spark = assert(elements.DEFAULT_PT_SPRK),
    pscn = assert(elements.DEFAULT_PT_PSCN),
    nscn = assert(elements.DEFAULT_PT_NSCN),
    hydrogen = assert(elements.DEFAULT_PT_H2),
    acid = assert(elements.DEFAULT_PT_ACID),
    filt = assert(elements.DEFAULT_PT_FILT),
    pipe = assert(elements.DEFAULT_PT_PIPE),
    glass = assert(elements.DEFAULT_PT_GLAS),
    ceramic = assert(elements.DEFAULT_PT_CRMC),
    coal = assert(elements.DEFAULT_PT_COAL),
    salt = assert(elements.DEFAULT_PT_SALT),
    inst = assert(elements.DEFAULT_PT_INST),
    conv = assert(elements.DEFAULT_PT_CONV),
    diamond = assert(elements.DEFAULT_PT_DMND),
    dust = assert(elements.DEFAULT_PT_DUST),
    aluminium = assert(elements.OMNI_PT_ALUM),
    copper = assert(elements.OMNI_PT_COPR),
    tin = assert(elements.OMNI_PT_TIN),
    nickel = assert(elements.OMNI_PT_NICL),
    fertilizer = assert(elements.OMNI_PT_FERT),
}

local total_frames = 0
local stage_frames = {}

local function configure()
    sim.clearSim()
    sim.paused(true)
    sim.gravityMode(sim.GRAV_OFF)
    sim.airMode(sim.AIR_OFF)
    sim.ambientHeatSim(false)
    sim.heatSim(true)
    sim.ensureDeterminism(true)
    sim.randomSeed(41, 42, 43, 44)
end

local function clear_particles()
    sim.clearRect(0, 0, sim.XRES, sim.YRES)
    sim.resetPressure()
    sim.airMode(sim.AIR_OFF)
end

local function make(type, x, y, temp)
    local particle = sim.partCreate(-1, x, y, type)
    assert(particle and particle >= 0,
        "failed to create unlocked type " .. tostring(type))
    if temp then sim.partProperty(particle, "temp", temp) end
    sim.partProperty(particle, "vx", 0)
    sim.partProperty(particle, "vy", 0)
    return particle
end

local function keep(particle, x, y, temp, life)
    assert(sim.partExists(particle), "tracked particle disappeared: " .. tostring(particle))
    if x then sim.partProperty(particle, "x", x) end
    if y then sim.partProperty(particle, "y", y) end
    if temp then sim.partProperty(particle, "temp", temp) end
    if life then sim.partProperty(particle, "life", life) end
    sim.partProperty(particle, "vx", 0)
    sim.partProperty(particle, "vy", 0)
end

local function make_spark(x, y)
    local particle = make(ids.metal, x, y, 300)
    sim.partChangeType(particle, ids.spark)
    assert(sim.partProperty(particle, "type") == ids.spark,
        "could not form an unlocked persistent spark")
    sim.partProperty(particle, "life", 1000)
    return particle
end

local function set_pressure_area(value)
    for y = 28, 32 do
        for x = 28, 32 do sim.pressure(x, y, value) end
    end
end

local function wait_stage(stage, maximum_frames, maintain)
    local before = sim.omniAlchemyProgress().completed_stage_count
    assert(before == stage - 1,
        "stage order drift before " .. stage .. ": " .. tostring(before))
    for frame = 1, maximum_frames do
        maintain(frame)
        sim.updateUpTo()
        total_frames = total_frames + 1
        local progress = sim.omniAlchemyProgress()
        if progress.completed_stage_count == stage then
            stage_frames[stage] = frame
            return
        end
        assert(progress.completed_stage_count == stage - 1,
            "progress skipped a stage: " .. tostring(progress.completed_stage_count))
    end
    local progress = sim.omniAlchemyProgress()
    local types = {}
    for particle in sim.parts() do
        types[#types + 1] = tostring(sim.partProperty(particle, "type"))
    end
    error("stage " .. stage .. " did not complete within " .. maximum_frames
        .. " frames; dwell=" .. tostring(progress.current_dwell_frames)
        .. "; cooling=" .. tostring(progress.current_cooling_armed)
        .. "; pressure30=" .. tostring(sim.pressure(30, 30))
        .. "; types=" .. table.concat(types, ","))
end

local function unlocked(...)
    for index = 1, select("#", ...) do
        local type = select(index, ...)
        assert(sim.omniAlchemyUnlocked(type), "expected unlocked type " .. tostring(type))
    end
end

local function stage_1()
    clear_particles()
    local fire = make(ids.fire, 120, 120, 1200)
    local cold = make(ids.water, 121, 122, 295)
    local hot = make(ids.water, 123, 120, 500)
    wait_stage(1, 180, function()
        keep(fire, 120, 120, 1200, 1000)
        keep(cold, 121, 122, 295)
        keep(hot, 123, 120, 500)
    end)
    unlocked(ids.vapor)
end

local function stage_2()
    clear_particles()
    local fire = make(ids.fire, 120, 120, 1200)
    local cold_stone = make(ids.stone, 122, 122, 300)
    local hot_stone = make(ids.stone, 124, 120, 1500)
    local vapor = make(ids.vapor, 126, 120, 500)
    wait_stage(2, 120, function()
        keep(fire, 120, 120, 1200, 1000)
        keep(cold_stone, 122, 122, 300)
        keep(hot_stone, 124, 120, 1500)
        keep(vapor, 126, 120, 500)
    end)
    unlocked(ids.lava, ids.sand, ids.brick)
end

local function stage_3()
    clear_particles()
    local vapor = make(ids.vapor, 120, 120, 500)
    local brick = make(ids.brick, 124, 120, 300)
    sim.airMode(sim.AIR_NOUPDATE)
    wait_stage(3, 240, function()
        keep(vapor, 120, 120, 500)
        keep(brick, 124, 120, 300)
        set_pressure_area(2.5)
    end)
    unlocked(ids.metal, ids.battery, ids.pump)
end

local function stage_4()
    clear_particles()
    local battery = make(ids.battery, 120, 120, 300)
    local metals = {}
    for index = 1, 6 do metals[index] = make(ids.metal, 120 + index, 120, 300) end
    wait_stage(4, 240, function()
        keep(battery, 120, 120, 300)
        for _, particle in ipairs(metals) do
            if sim.partExists(particle) then
                local type = sim.partProperty(particle, "type")
                if type == ids.spark then sim.partProperty(particle, "life", 1000) end
                sim.partProperty(particle, "vx", 0)
                sim.partProperty(particle, "vy", 0)
            end
        end
    end)
    unlocked(ids.spark, ids.pscn, ids.nscn)
end

local function stage_5()
    clear_particles()
    local water = make(ids.water, 120, 120, 295)
    local oxygen = make(ids.oxygen, 132, 120, 300)
    local spark = make_spark(124, 120)
    local metals = {}
    for index = 1, 4 do metals[index] = make(ids.metal, 126 + index, 120, 300) end
    wait_stage(5, 300, function()
        keep(water, 120, 120, 295)
        keep(oxygen, 132, 120, 300)
        keep(spark, 124, 120, 300, 1000)
        for index, particle in ipairs(metals) do keep(particle, 126 + index, 120, 300) end
    end)
    unlocked(ids.hydrogen, ids.acid)
end

local function stage_6()
    clear_particles()
    local particles = {}
    for index = 1, 4 do
        particles[#particles + 1] = { make(ids.water, 120 + index, 120, 295), 120 + index, 120, 295 }
        particles[#particles + 1] = { make(ids.sand, 120 + index, 124, 300), 120 + index, 124, 300 }
    end
    local brick = make(ids.brick, 124, 128, 300)
    wait_stage(6, 360, function()
        for _, item in ipairs(particles) do keep(item[1], item[2], item[3], item[4]) end
        keep(brick, 124, 128, 300)
    end)
    unlocked(ids.filt, ids.pipe, ids.glass)
end

local function stage_7()
    clear_particles()
    local lava = make(ids.lava, 120, 120, 1500)
    local pipe = make(ids.pipe, 124, 120, 300)
    local metal = make(ids.metal, 128, 120, 300)
    local fires = {}
    for index = 1, 4 do fires[index] = make(ids.fire, 140 + index * 2, 120, 1200) end
    sim.airMode(sim.AIR_NOUPDATE)
    wait_stage(7, 90, function()
        keep(lava, 120, 120, 1500)
        keep(pipe, 124, 120, 300)
        keep(metal, 128, 120, 300)
        for index, particle in ipairs(fires) do keep(particle, 140 + index * 2, 120, 1200, 1000) end
        set_pressure_area(2.5)
    end)
    unlocked(ids.ceramic, ids.coal, ids.aluminium, ids.copper, ids.tin, ids.nickel)
    ui.activeTool(0, "OMNI_PT_ALUM")
    if modules_off then
        assert(ui.activeTool(0) ~= "OMNI_PT_ALUM",
            "disabled metallurgy module exposed an unlocked element")
    else
        assert(ui.activeTool(0) == "OMNI_PT_ALUM",
            "enabled metallurgy module hid an unlocked element")
    end
    ui.activeTool(0, "DEFAULT_PT_FIRE")
end

local function stage_8()
    clear_particles()
    local acid1 = make(ids.acid, 126, 120, 340)
    local acid2 = make(ids.acid, 126, 122, 340)
    local water = make(ids.water, 130, 120, 290)
    local filt1 = make(ids.filt, 120, 120, 300)
    local filt2 = make(ids.filt, 120, 122, 300)
    local sand1 = make(ids.sand, 136, 120, 300)
    local sand2 = make(ids.sand, 136, 122, 300)
    wait_stage(8, 330, function()
        keep(acid1, 126, 120, 340)
        keep(acid2, 126, 122, 340)
        keep(water, 130, 120, 290)
        keep(filt1, 120, 120, 300)
        keep(filt2, 120, 122, 300)
        keep(sand1, 136, 120, 300)
        keep(sand2, 136, 122, 300)
    end)
    unlocked(ids.salt, ids.fertilizer)
    ui.activeTool(0, "OMNI_PT_FERT")
    if modules_off then
        assert(ui.activeTool(0) ~= "OMNI_PT_FERT",
            "disabled chemistry module exposed an unlocked element")
    else
        assert(ui.activeTool(0) == "OMNI_PT_FERT",
            "enabled chemistry module hid an unlocked element")
    end
    ui.activeTool(0, "DEFAULT_PT_FIRE")
end

local function stage_9()
    clear_particles()
    local pump = make(ids.pump, 120, 120, 300)
    local filt = make(ids.filt, 124, 120, 300)
    local pipe = make(ids.pipe, 128, 120, 300)
    local spark = make_spark(132, 120)
    sim.airMode(sim.AIR_NOUPDATE)
    wait_stage(9, 420, function()
        keep(pump, 120, 120, 300)
        keep(filt, 124, 120, 300)
        keep(pipe, 128, 120, 300)
        keep(spark, 132, 120, 300, 1000)
        set_pressure_area(2.5)
    end)
    unlocked(ids.inst, ids.conv)
end

local function stage_10()
    clear_particles()
    local hydrogen1 = make(ids.hydrogen, 120, 120, 300)
    local hydrogen2 = make(ids.hydrogen, 121, 120, 300)
    local oxygen = make(ids.oxygen, 136, 120, 300)
    local ceramics = {}
    for index = 1, 4 do ceramics[index] = make(ids.ceramic, 126 + index, 126, 300) end
    local inst = make(ids.inst, 132, 120, 300)
    local filt1 = make(ids.filt, 120, 124, 300)
    local filt2 = make(ids.filt, 122, 124, 300)
    local fire = make(ids.fire, 200, 200, 1200)
    local spark = make_spark(210, 200)
    sim.airMode(sim.AIR_NOUPDATE)
    wait_stage(10, 690, function()
        keep(hydrogen1, 120, 120, 300)
        keep(hydrogen2, 121, 120, 300)
        keep(oxygen, 136, 120, 300)
        for index, particle in ipairs(ceramics) do keep(particle, 126 + index, 126, 300) end
        keep(inst, 132, 120, 300)
        keep(filt1, 120, 124, 300)
        keep(filt2, 122, 124, 300)
        keep(fire, 200, 200, 1200, 1000)
        keep(spark, 210, 200, 300, 1000)
        set_pressure_area(3.5)
    end)
    unlocked(ids.diamond, ids.dust)
    local progress = sim.omniAlchemyProgress()
    assert(progress.mastered and progress.completed_stage_count == 10
        and progress.current_stage_id == "", "mastery state is incomplete")
end

local function run()
    configure()
    stage_1()
    stage_2()
    stage_3()
    stage_4()
    stage_5()
    stage_6()
    stage_7()
    stage_8()
    stage_9()
    stage_10()
end

local ok, failure = xpcall(run, debug.traceback)
local report = assert(io.open(RESULT, "w"))
if ok then
    report:write("OMNI_ALCHEMY_PROGRESSION_STATUS=PASS\n")
    report:write("OMNI_ALCHEMY_STAGES=10\n")
    report:write("OMNI_ALCHEMY_MODULE_MODE=" .. progression_mode .. "\n")
    report:write("OMNI_ALCHEMY_TOTAL_FRAMES=" .. total_frames .. "\n")
    report:write("OMNI_ALCHEMY_STAGE_FRAMES=" .. table.concat(stage_frames, ",") .. "\n")
    report:write("OMNI_ALCHEMY_MASTERY=true\n")
else
    report:write("OMNI_ALCHEMY_PROGRESSION_STATUS=FAIL\n")
    report:write("OMNI_ALCHEMY_MODULE_MODE=" .. progression_mode .. "\n")
    report:write("OMNI_ALCHEMY_TOTAL_FRAMES=" .. total_frames .. "\n")
    report:write("OMNI_ALCHEMY_ERROR=" .. tostring(failure):gsub("[\r\n]+", " | ") .. "\n")
end
report:close()
