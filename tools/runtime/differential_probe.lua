local CONFIG_FILE = "differential.config"

local function read_config()
    local file = assert(io.open(CONFIG_FILE, "rb"), "cannot open " .. CONFIG_FILE)
    local text = assert(file:read("*a"), "cannot read " .. CONFIG_FILE)
    file:close()
    local values = {}
    for line in text:gmatch("[^\r\n]+") do
        local key, value = line:match("^([A-Za-z0-9_]+)=(.*)$")
        assert(key, "invalid differential config line: " .. line)
        assert(values[key] == nil, "duplicate differential config key: " .. key)
        values[key] = value
    end
    return values
end

local config = read_config()

local function required_integer(name, minimum, maximum)
    local text = assert(config[name], "missing differential config key: " .. name)
    assert(text:match("^%d+$"), "differential config key is not an integer: " .. name)
    local value = assert(tonumber(text), "cannot parse differential config key: " .. name)
    assert(value >= minimum and value <= maximum,
        name .. " must be in [" .. minimum .. ", " .. maximum .. "]")
    return value
end

local schema_version = required_integer("schema_version", 1, 1)
local phase = assert(config.phase, "missing differential phase")
assert(phase == "trace" or phase == "capture", "invalid differential phase: " .. phase)
local scenario_name = assert(config.scenario, "missing differential scenario")
assert(scenario_name == "empty" or scenario_name == "mixed-medium",
    "unknown differential scenario: " .. scenario_name)
local trace_steps = required_integer("trace_steps", 1, 100000)
local capture_step = required_integer("capture_step", 0, trace_steps)
local seed = {
    required_integer("seed_a", 0, 4294967295),
    required_integer("seed_b", 0, 4294967295),
    required_integer("seed_c", 0, 4294967295),
    required_integer("seed_d", 0, 4294967295),
}

assert(type(sim.updateUpTo) == "function", "sim.updateUpTo is unavailable")
assert(type(sim.hash) == "function", "sim.hash is unavailable")
assert(type(sim.parts) == "function", "sim.parts is unavailable")
assert(type(sim.randomSeed) == "function", "sim.randomSeed is unavailable")

local ids = {
    brck = assert(elements.DEFAULT_PT_BRCK),
    btry = assert(elements.DEFAULT_PT_BTRY),
    co2 = assert(elements.DEFAULT_PT_CO2),
    dmnd = assert(elements.DEFAULT_PT_DMND),
    dust = assert(elements.DEFAULT_PT_DUST),
    fire = assert(elements.DEFAULT_PT_FIRE),
    gas = assert(elements.DEFAULT_PT_GAS),
    lava = assert(elements.DEFAULT_PT_LAVA),
    metl = assert(elements.DEFAULT_PT_METL),
    o2 = assert(elements.DEFAULT_PT_O2),
    oil = assert(elements.DEFAULT_PT_OIL),
    plnt = assert(elements.DEFAULT_PT_PLNT),
    salt = assert(elements.DEFAULT_PT_SALT),
    smke = assert(elements.DEFAULT_PT_SMKE),
    sprk = assert(elements.DEFAULT_PT_SPRK),
    watr = assert(elements.DEFAULT_PT_WATR),
    wood = assert(elements.DEFAULT_PT_WOOD),
}

local function create_particle(x, y, element)
    local particle = sim.partCreate(-1, x, y, element)
    assert(particle >= 0,
        "particle creation failed at " .. x .. "," .. y .. " for element " .. element)
    return particle
end

local function fill_rectangle(x1, y1, x2, y2, element, stride_x, stride_y)
    local created = 0
    for y = y1, y2, stride_y or 1 do
        for x = x1, x2, stride_x or 1 do
            create_particle(x, y, element)
            created = created + 1
        end
    end
    return created
end

local function configure_simulation()
    sim.paused(true)
    sim.edgeMode(sim.EDGE_VOID)
    sim.gravityMode(sim.GRAV_VERTICAL)
    sim.newtonianGravity(false)
    sim.airMode(sim.AIR_ON)
    sim.convectionMode(sim.AIRC_LEGACY)
    sim.ambientHeatSim(true)
    sim.heatSim(true)
    sim.waterEqualization(0)
    sim.ambientAirTemp(295.15)
    sim.edgePressure(0.0)
    sim.edgeVelocity(0.0, 0.0)
    sim.ensureDeterminism(true)
    sim.randomSeed(seed[1], seed[2], seed[3], seed[4])
end

local function reset_world()
    -- Match the fixed-step benchmark's pristine Legacy cache contract.
    sim.clearSim()
    configure_simulation()
    sim.updateUpTo()
    sim.clearSim()
    configure_simulation()
end

local function generate_empty()
    return 0
end

local function generate_mixed_medium()
    local created = 0
    created = created + fill_rectangle(20, 50, 588, 53, ids.dmnd)
    created = created + fill_rectangle(20, 54, 23, 343, ids.dmnd)
    created = created + fill_rectangle(585, 54, 588, 343, ids.dmnd)
    created = created + fill_rectangle(24, 340, 584, 343, ids.brck)
    created = created + fill_rectangle(180, 54, 183, 339, ids.dmnd)
    created = created + fill_rectangle(340, 54, 343, 339, ids.dmnd)
    created = created + fill_rectangle(30, 200, 175, 335, ids.dust)
    created = created + fill_rectangle(200, 220, 335, 335, ids.watr)
    created = created + fill_rectangle(205, 215, 235, 215, ids.plnt)
    created = created + fill_rectangle(240, 215, 260, 215, ids.salt)
    created = created + fill_rectangle(285, 205, 305, 210, ids.lava)

    local gas_types = { ids.o2, ids.co2, ids.gas, ids.smke }
    for y = 90, 280, 2 do
        for x = 365, 575, 2 do
            local slot = (math.floor(x / 2) + math.floor(y / 2)) % #gas_types + 1
            create_particle(x, y, gas_types[slot])
            created = created + 1
        end
    end

    created = created + fill_rectangle(365, 305, 575, 307, ids.wood)
    created = created + fill_rectangle(365, 310, 455, 335, ids.oil)
    created = created + fill_rectangle(380, 285, 420, 295, ids.fire, 2, 2)
    created = created + fill_rectangle(365, 70, 575, 73, ids.metl)
    local battery = create_particle(370, 68, ids.btry)
    created = created + 1
    local spark_host = sim.partID(371, 70)
    assert(spark_host and sim.partExists(spark_host), "missing spark host")
    sim.partProperty(spark_host, "type", ids.sprk)
    sim.partProperty(spark_host, "ctype", ids.metl)
    sim.partProperty(spark_host, "life", 4)
    assert(sim.partExists(battery), "battery creation did not persist")

    sim.pressure(88, 15, 40, 45, 6.0)
    sim.velocityX(88, 15, 40, 45, -1.5)
    sim.velocityY(88, 15, 40, 45, 0.75)
    sim.ambientHeat(88, 15, 40, 45, 325.15)
    sim.pressure(5, 15, 30, 45, -4.0)
    return created
end

local generators = {
    ["empty"] = generate_empty,
    ["mixed-medium"] = generate_mixed_medium,
}

local function rng_values()
    local a, b, c, d = sim.randomSeed()
    return a, b, c, d
end

local function integer_text(value)
    return string.format("%.0f", value)
end

local function float_text(value)
    return string.format("%.9g", value)
end

local function write_result(status, values)
    local result = assert(io.open("differential-" .. phase .. ".result", "wb"))
    result:write("OMNI_DIFFERENTIAL_PROBE_STATUS=" .. status .. "\n")
    for _, key in ipairs(values.order) do
        result:write(key .. "=" .. tostring(values[key]) .. "\n")
    end
    result:close()
end

local function base_result()
    return {
        order = {
            "schema_version", "phase", "scenario", "seed", "trace_steps",
            "capture_step", "generated_particles", "state_hash", "particles", "rng",
        },
        schema_version = schema_version,
        phase = phase,
        scenario = scenario_name,
        seed = table.concat(seed, ","),
        trace_steps = trace_steps,
        capture_step = capture_step,
    }
end

local function run_trace(created)
    local trace = assert(io.open("trace.csv", "wb"))
    trace:write("step,state_hash_fnv1a32,particles,rng_a,rng_b,rng_c,rng_d\n")
    local function write_row(step)
        local a, b, c, d = rng_values()
        trace:write(step .. "," .. sim.hash() .. "," .. sim.partCount() .. ","
            .. integer_text(a) .. "," .. integer_text(b) .. ","
            .. integer_text(c) .. "," .. integer_text(d) .. "\n")
    end
    write_row(0)
    for step = 1, trace_steps do
        sim.updateUpTo()
        write_row(step)
    end
    trace:close()

    local a, b, c, d = rng_values()
    local result = base_result()
    result.generated_particles = created
    result.state_hash = sim.hash()
    result.particles = sim.partCount()
    result.rng = table.concat({ integer_text(a), integer_text(b), integer_text(c), integer_text(d) }, ",")
    result.order[#result.order + 1] = "trace_file"
    result.trace_file = "trace.csv"
    write_result("PASS", result)
end

local function build_identifier_map()
    local identifiers = {}
    for name, value in pairs(elements) do
        if type(name) == "string" and type(value) == "number"
                and name:match("^[A-Z0-9_]+_PT_[A-Z0-9_]+$") then
            if identifiers[value] == nil or name < identifiers[value] then
                identifiers[value] = name
            end
        end
    end
    return identifiers
end

local particle_fields = {
    "type", "life", "ctype", "x", "y", "vx", "vy", "temp", "flags",
    "tmp", "tmp2", "tmp3", "tmp4", "dcolour",
}

local function dump_particles()
    local identifiers = build_identifier_map()
    local file = assert(io.open("particles.csv", "wb"))
    file:write("id,type,type_identifier,life,ctype,x,y,vx,vy,temp,flags,tmp,tmp2,tmp3,tmp4,dcolour\n")
    local count = 0
    for id in sim.parts() do
        local values = {}
        for _, field in ipairs(particle_fields) do
            values[field] = sim.partProperty(id, field)
        end
        file:write(integer_text(id) .. "," .. integer_text(values.type) .. ","
            .. (identifiers[values.type] or "UNKNOWN") .. ","
            .. integer_text(values.life) .. "," .. integer_text(values.ctype) .. ","
            .. float_text(values.x) .. "," .. float_text(values.y) .. ","
            .. float_text(values.vx) .. "," .. float_text(values.vy) .. ","
            .. float_text(values.temp) .. "," .. integer_text(values.flags) .. ","
            .. integer_text(values.tmp) .. "," .. integer_text(values.tmp2) .. ","
            .. integer_text(values.tmp3) .. "," .. integer_text(values.tmp4) .. ","
            .. integer_text(values.dcolour) .. "\n")
        count = count + 1
    end
    file:close()
    return count
end

local function dump_cells()
    local file = assert(io.open("cells.csv", "wb"))
    file:write("cx,cy,pressure,velocity_x,velocity_y,ambient_heat,wall_map,elec_map,"
        .. "fan_velocity_x,fan_velocity_y,gravity_mass,gravity_mask,gravity_force_x,gravity_force_y\n")
    local count = 0
    for cy = 0, sim.YCELLS - 1 do
        for cx = 0, sim.XCELLS - 1 do
            local gravity_x, gravity_y = sim.gravityField(cx, cy)
            file:write(cx .. "," .. cy .. "," .. float_text(sim.pressure(cx, cy)) .. ","
                .. float_text(sim.velocityX(cx, cy)) .. ","
                .. float_text(sim.velocityY(cx, cy)) .. ","
                .. float_text(sim.ambientHeat(cx, cy)) .. ","
                .. integer_text(sim.wallMap(cx, cy)) .. ","
                .. integer_text(sim.elecMap(cx, cy)) .. ","
                .. float_text(sim.fanVelocityX(cx, cy)) .. ","
                .. float_text(sim.fanVelocityY(cx, cy)) .. ","
                .. float_text(sim.gravityMass(cx, cy)) .. ","
                .. integer_text(sim.gravityMask(cx, cy)) .. ","
                .. float_text(gravity_x) .. "," .. float_text(gravity_y) .. "\n")
            count = count + 1
        end
    end
    file:close()
    return count
end

local function run_capture(created)
    for _ = 1, capture_step do
        sim.updateUpTo()
    end
    local particle_records = dump_particles()
    local cell_records = dump_cells()
    assert(particle_records == sim.partCount(), "particle dump count mismatch")
    assert(cell_records == sim.NCELL, "cell dump count mismatch")

    local a, b, c, d = rng_values()
    local result = base_result()
    result.generated_particles = created
    result.state_hash = sim.hash()
    result.particles = sim.partCount()
    result.rng = table.concat({ integer_text(a), integer_text(b), integer_text(c), integer_text(d) }, ",")
    for _, key in ipairs({ "particle_file", "particle_records", "cell_file", "cell_records" }) do
        result.order[#result.order + 1] = key
    end
    result.particle_file = "particles.csv"
    result.particle_records = particle_records
    result.cell_file = "cells.csv"
    result.cell_records = cell_records
    write_result("PASS", result)
end

local function run()
    tpt.fpsCap(2)
    tpt.drawCap(1)
    reset_world()
    local created = generators[scenario_name]()
    assert(created == sim.partCount(), "generated particle count mismatch")
    if phase == "trace" then
        run_trace(created)
    else
        run_capture(created)
    end
end

local ok, error_text = xpcall(run, debug.traceback)
if not ok then
    write_result("FAIL", {
        order = { "schema_version", "phase", "scenario", "error" },
        schema_version = schema_version,
        phase = phase,
        scenario = scenario_name,
        error = tostring(error_text):gsub("[\r\n]+", " | "),
    })
    os.exit(1)
end
os.exit(0)
