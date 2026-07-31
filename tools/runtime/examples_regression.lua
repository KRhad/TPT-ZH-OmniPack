local MODE_FILE = "examples.mode"
local CASE_FILE = "examples.case"
local STAMP_FILE = "examples.stamp"
local CHALLENGE_FILE = "examples.challenge"
local RESULT_FILE = "examples.result"

local function read_line(path)
    local file = assert(io.open(path, "rb"), "cannot open " .. path)
    local value = file:read("*l")
    file:close()
    return (value or ""):match("^%s*(.-)%s*$")
end

local mode = read_line(MODE_FILE)
local case_id = read_line(CASE_FILE)
assert(mode == "generate" or mode == "verify",
    "invalid examples mode: " .. tostring(mode))

local ids = {
    pero = assert(elements.OMNI_PT_PERO),
    path = assert(elements.OMNI_PT_PATH),
    hums = assert(elements.OMNI_PT_HUMS),
    fert = assert(elements.OMNI_PT_FERT),
    nutr = assert(elements.OMNI_PT_NUTR),
    dust = assert(elements.DEFAULT_PT_DUST),
    watr = assert(elements.DEFAULT_PT_WATR),
    cata = assert(elements.OMNI_PT_CATA),
    slag = assert(elements.OMNI_PT_SLAG),
    acid = assert(elements.DEFAULT_PT_ACID),
    flux = assert(elements.OMNI_PT_FLUX),
    ssil = assert(elements.OMNI_PT_SSIL),
    lead = assert(elements.OMNI_PT_LEAD),
    lava = assert(elements.DEFAULT_PT_LAVA),
    ncrm = assert(elements.OMNI_PT_NCRM),
    sprk = assert(elements.DEFAULT_PT_SPRK),
    nwst = assert(elements.OMNI_PT_NWST),
    poly = assert(elements.OMNI_PT_POLY),
    rshd = assert(elements.OMNI_PT_RSHD),
}

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

local function make(key, x, y, temperature, ctype, life)
    local particle = assert(sim.partCreate(-1, x, y, ids[key]),
        "failed to create " .. key)
    sim.partProperty(particle, "temp", temperature or 300.0)
    if ctype then
        sim.partProperty(particle, "ctype", ids[ctype])
    end
    if life then
        sim.partProperty(particle, "life", life)
    end
    return particle
end

local examples = {
    ["peroxide-pathogen"] = function()
        make("pero", 120, 120, 300.0)
        make("path", 121, 120, 300.0)
    end,
    ["humus-fertilizer"] = function()
        make("hums", 120, 120, 300.0)
        make("fert", 121, 120, 300.0)
        make("watr", 120, 121, 300.0)
    end,
    ["slag-acid"] = function()
        make("cata", 120, 120, 320.0)
        make("slag", 121, 120, 320.0)
        make("acid", 120, 121, 320.0)
    end,
    ["shield-assembly"] = function()
        make("ssil", 120, 120, 800.0)
        make("lava", 121, 120, 800.0, "lead")
        make("ncrm", 120, 121, 800.0)
    end,
    ["waste-stabilization"] = function()
        make("nwst", 120, 120, 550.0)
        make("slag", 121, 120, 550.0)
        make("hums", 120, 121, 550.0)
        make("poly", 119, 120, 550.0)
        make("watr", 120, 119, 550.0)
        make("cata", 121, 121, 550.0)
    end,
    ["waste-missing-catalyst"] = function()
        make("nwst", 120, 120, 550.0)
        make("slag", 121, 120, 550.0)
        make("hums", 120, 121, 550.0)
        make("poly", 119, 120, 550.0)
        make("watr", 120, 119, 550.0)
    end,
    ["integrated-recovery"] = function()
        make("pero", 96, 96, 300.0)
        make("path", 97, 96, 300.0)
        make("hums", 112, 96, 300.0)
        make("fert", 113, 96, 300.0)
        make("watr", 112, 97, 300.0)
        make("cata", 128, 96, 320.0)
        make("slag", 129, 96, 320.0)
        make("acid", 128, 97, 320.0)
        make("ssil", 96, 128, 800.0)
        make("lava", 97, 128, 800.0, "lead")
        make("ncrm", 96, 129, 800.0)
        make("nwst", 128, 128, 550.0)
        make("slag", 129, 128, 550.0)
        make("hums", 128, 129, 550.0)
        make("poly", 127, 128, 550.0)
        make("watr", 128, 127, 550.0)
        make("cata", 129, 129, 550.0)
    end,
}

local function particle_at(x, y)
    local particle = sim.partID(x, y)
    assert(type(particle) == "number" and particle >= 0,
        "missing particle at " .. x .. "," .. y)
    return particle
end

local function type_at(x, y)
    return sim.partProperty(particle_at(x, y), "type")
end

local function temp_at(x, y)
    return sim.partProperty(particle_at(x, y), "temp")
end

local function count_particles()
    local count = 0
    for _ in sim.parts() do
        count = count + 1
    end
    return count
end

local function step(frames)
    for _ = 1, frames or 1 do
        sim.updateUpTo()
    end
end

local function load_example(stamp)
    configure()
    local loaded, load_error = sim.loadStamp(stamp, 0, 0, false, 0, 1)
    assert(loaded == 1, "could not load example " .. stamp .. ": "
        .. tostring(load_error))
end

local function assert_type(x, y, expected, label)
    assert(type_at(x, y) == ids[expected], label .. " type changed")
end

local function run_challenge(challenge)
    local initial_count = count_particles()
    local assertions = 0
    local function ok(condition, message)
        assert(condition, message)
        assertions = assertions + 1
    end

    if challenge == "T01-PERO-PATH" then
        ok(type_at(120, 120) == ids.pero, "T01 missing PERO")
        ok(type_at(121, 120) == ids.path, "T01 missing PATH")
        step()
        ok(type_at(120, 120) == ids.watr, "T01 PERO was not recovered")
        ok(type_at(121, 120) == ids.hums, "T01 PATH was not treated")
    elseif challenge == "T02-PERO-CONTROL" then
        local catalyst = make("cata", 121, 121, 1000.0)
        ok(catalyst >= 0, "T02 catalyst setup failed")
        for _ = 1, 3 do
            step()
            ok(temp_at(121, 121) >= 350.0, "T02 catalyst lost hot interlock")
        end
        ok(type_at(120, 120) == ids.pero, "T02 PERO was consumed")
        ok(type_at(121, 120) == ids.path, "T02 PATH was consumed")
    elseif challenge == "T03-HUMS-FERT" then
        local water = particle_at(120, 121)
        sim.partKill(water)
        step(2)
        ok(type_at(120, 120) == ids.hums, "T03 dry HUMS changed")
        ok(type_at(121, 120) == ids.fert, "T03 dry FERT changed")
        make("watr", 120, 121, 300.0)
        step()
        ok(type_at(120, 120) == ids.nutr, "T03 HUMS was not recovered")
        ok(type_at(121, 120) == ids.dust, "T03 FERT was not consumed")
        ok(type_at(120, 121) == ids.watr, "T03 water was not retained")
    elseif challenge == "T04-SLAG-ACID" then
        step()
        ok(type_at(121, 120) == ids.flux, "T04 SLAG was not recovered")
        ok(type_at(120, 121) == ids.watr, "T04 ACID was not neutralized")
        ok(type_at(120, 120) == ids.cata, "T04 catalyst was consumed")
    elseif challenge == "T05-SHIELD" then
        local trigger = particle_at(120, 121)
        ok(sim.partProperty(trigger, "type") == ids.ncrm,
            "T05 missing NCRM trigger")
        sim.partProperty(trigger, "type", ids.sprk)
        sim.partProperty(trigger, "ctype", ids.ncrm)
        sim.partProperty(trigger, "life", 4)
        step()
        ok(type_at(120, 120) == ids.rshd, "T05 steel did not become RSHD")
        ok(type_at(121, 120) == ids.rshd, "T05 lead did not become RSHD")
        ok(count_particles() == initial_count,
            "T05 shield assembly did not retain its trigger particle")
    elseif challenge == "T06-WASTE-CATALYST" then
        sim.heatSim(false)
        step(2)
        ok(type_at(120, 120) == ids.nwst, "T06 waste changed without CATA")
        sim.heatSim(true)
        make("cata", 121, 121, 550.0)
        step(2)
        local waste_type = sim.partProperty(particle_at(120, 120), "type")
        ok(waste_type == ids.rshd,
            "T06 waste was not stabilized (type=" .. tostring(waste_type) .. ")")
        ok(type_at(119, 120) == ids.rshd, "T06 polymer was not shielded")
        ok(type_at(121, 120) == ids.flux, "T06 slag was not recovered")
        ok(type_at(120, 121) == ids.nutr, "T06 humus was not recovered")
        ok(type_at(120, 119) == ids.watr, "T06 water was not retained")
    elseif challenge == "T07-WASTE-FULL" then
        step()
        ok(type_at(120, 120) == ids.rshd, "T07 waste was not stabilized")
        ok(type_at(119, 120) == ids.rshd, "T07 polymer was not shielded")
        ok(type_at(121, 120) == ids.flux, "T07 slag was not recovered")
        ok(type_at(120, 121) == ids.nutr, "T07 humus was not recovered")
        ok(type_at(120, 119) == ids.watr, "T07 water was not retained")
        ok(type_at(121, 121) == ids.cata, "T07 catalyst was not retained")
        ok(count_particles() == initial_count,
            "T07 stabilization increased particle count")
    elseif challenge == "T08-INTEGRATED" then
        local trigger = particle_at(96, 129)
        ok(sim.partProperty(trigger, "type") == ids.ncrm,
            "T08 missing NCRM trigger")
        sim.partProperty(trigger, "type", ids.sprk)
        sim.partProperty(trigger, "ctype", ids.ncrm)
        sim.partProperty(trigger, "life", 4)
        step()
        ok(type_at(96, 96) == ids.watr, "T08 peroxide chain missing")
        ok(type_at(97, 96) == ids.hums, "T08 pathogen chain missing")
        ok(type_at(112, 96) == ids.nutr, "T08 nutrient chain missing")
        ok(type_at(113, 96) == ids.dust, "T08 fertilizer chain missing")
        ok(type_at(129, 96) == ids.flux, "T08 slag chain missing")
        ok(type_at(128, 97) == ids.watr, "T08 acid chain missing")
        ok(type_at(96, 128) == ids.rshd, "T08 shield chain missing")
        ok(type_at(97, 128) == ids.rshd, "T08 lead chain missing")
        ok(type_at(128, 128) == ids.rshd, "T08 waste chain missing")
        ok(type_at(127, 128) == ids.rshd, "T08 polymer chain missing")
        ok(type_at(129, 128) == ids.flux, "T08 waste slag missing")
        ok(type_at(128, 129) == ids.nutr, "T08 waste humus missing")
        ok(type_at(128, 127) == ids.watr, "T08 waste water missing")
        ok(type_at(129, 129) == ids.cata, "T08 waste catalyst missing")
        ok(count_particles() <= initial_count,
            "T08 integrated recovery increased particle count")
    else
        error("unknown challenge: " .. tostring(challenge))
    end
    return initial_count, count_particles(), assertions
end

local function generate()
    assert(examples[case_id], "unknown example: " .. tostring(case_id))
    configure()
    examples[case_id]()
    local initial_count = count_particles()
    assert(initial_count > 0, "example is empty")
    local stamp = assert(sim.saveStamp(0, 0, sim.XRES - 1, sim.YRES - 1, 1),
        "saveStamp returned no stamp")
    assert(stamp:match("^[0-9A-Fa-f]+$") and #stamp == 10,
        "invalid generated stamp: " .. tostring(stamp))
    return {
        stamp = stamp,
        initial_count = initial_count,
        final_count = initial_count,
        assertions = 1,
        operation = "generate-save",
    }
end

local function verify()
    local stamp = read_line(STAMP_FILE)
    local challenge = read_line(CHALLENGE_FILE)
    assert(stamp:match("^[0-9A-Fa-f]+$") and #stamp == 10,
        "invalid verification stamp")
    load_example(stamp)
    assert(examples[case_id], "unknown verification example: " .. tostring(case_id))
    local initial_count, final_count, assertions = run_challenge(challenge)
    return {
        stamp = stamp,
        initial_count = initial_count,
        final_count = final_count,
        assertions = assertions,
        challenge = challenge,
        operation = "load-solve-verify",
    }
end

local ok, data = xpcall(mode == "generate" and generate or verify,
    debug.traceback)
local report = assert(io.open(RESULT_FILE, "wb"))
if ok then
    report:write("OMNI_EXAMPLE_STATUS=PASS\n")
    report:write("OMNI_EXAMPLE_CASE=" .. case_id .. "\n")
    report:write("OMNI_EXAMPLE_MODE=" .. mode .. "\n")
    report:write("OMNI_EXAMPLE_OPERATION=" .. data.operation .. "\n")
    report:write("OMNI_EXAMPLE_STAMP=" .. data.stamp .. "\n")
    report:write("OMNI_EXAMPLE_INITIAL_PARTICLES=" .. data.initial_count .. "\n")
    report:write("OMNI_EXAMPLE_FINAL_PARTICLES=" .. data.final_count .. "\n")
    report:write("OMNI_EXAMPLE_ASSERTIONS=" .. data.assertions .. "\n")
    if data.challenge then
        report:write("OMNI_CHALLENGE_ID=" .. data.challenge .. "\n")
    end
else
    report:write("OMNI_EXAMPLE_STATUS=FAIL\n")
    report:write("OMNI_EXAMPLE_CASE=" .. tostring(case_id) .. "\n")
    report:write("OMNI_EXAMPLE_MODE=" .. tostring(mode) .. "\n")
    report:write("OMNI_EXAMPLE_ERROR=" .. tostring(data):gsub("[\r\n]+", " | ") .. "\n")
end
report:close()
os.exit(ok and 0 or 1)
