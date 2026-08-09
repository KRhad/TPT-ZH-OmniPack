local CONFIG_FILE = "fixed-step-benchmark.config"
local RESULT_FILE = "fixed-step-lua.result"

local function read_config()
    local file = assert(io.open(CONFIG_FILE, "rb"), "cannot open " .. CONFIG_FILE)
    local text = assert(file:read("*a"), "cannot read " .. CONFIG_FILE)
    file:close()
    local values = {}
    for line in text:gmatch("[^\r\n]+") do
        local key, value = line:match("^([A-Za-z0-9_]+)=(.*)$")
        assert(key, "invalid benchmark config line: " .. line)
        assert(values[key] == nil, "duplicate benchmark config key: " .. key)
        values[key] = value
    end
    return values
end

local config = read_config()

local function required_integer(name, minimum, maximum)
    local text = assert(config[name], "missing benchmark config key: " .. name)
    assert(text:match("^%d+$"), "benchmark config key is not an integer: " .. name)
    local value = assert(tonumber(text), "cannot parse benchmark config key: " .. name)
    assert(value >= minimum and value <= maximum,
        name .. " must be in [" .. minimum .. ", " .. maximum .. "]")
    return value
end

local schema_version = required_integer("schema_version", 1, 1)
local scenario_name = assert(config.scenario, "missing benchmark config key: scenario")
assert(scenario_name == "empty" or scenario_name == "mixed-medium",
    "unknown benchmark scenario: " .. scenario_name)
local warmup_steps = required_integer("warmup_steps", 0, 100000)
local steps_per_pass = required_integer("steps_per_pass", 1, 1000000)
local pass_count = required_integer("passes", 1, 50)
local omni_profiler_enabled = required_integer("omni_profiler_enabled", 0, 1) == 1
local seed = {
    required_integer("seed_a", 0, 4294967295),
    required_integer("seed_b", 0, 4294967295),
    required_integer("seed_c", 0, 4294967295),
    required_integer("seed_d", 0, 4294967295),
}

assert(socket and type(socket.getTime) == "function", "socket.getTime is unavailable")
assert(type(sim.updateUpTo) == "function", "sim.updateUpTo is unavailable")
assert(type(sim.hash) == "function", "sim.hash is unavailable")
assert(type(sim.ensureDeterminism) == "function", "sim.ensureDeterminism is unavailable")
assert(type(sim.randomSeed) == "function", "sim.randomSeed is unavailable")
assert(type(sim.omniProfilerEnabled) == "function", "sim.omniProfilerEnabled is unavailable")
assert(type(sim.omniProfiler) == "function", "sim.omniProfiler is unavailable")
assert(type(sim.resetOmniProfiler) == "function", "sim.resetOmniProfiler is unavailable")

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
    -- Legacy clearSim does not clear the derived Air blocking maps. One empty
    -- fixed step rebuilds those maps without particles, then the second clear
    -- restores frame/RNG/Air state before the actual scene is generated.
    sim.clearSim()
    configure_simulation()
    sim.omniProfilerEnabled(omni_profiler_enabled)
    sim.updateUpTo()
    sim.clearSim()
    configure_simulation()
    sim.resetOmniProfiler()
end

local function generate_empty()
    return 0
end

local function generate_mixed_medium()
    local created = 0

    -- Stable diamond shell and dividers keep four independently busy regions.
    created = created + fill_rectangle(20, 50, 588, 53, ids.dmnd)
    created = created + fill_rectangle(20, 54, 23, 343, ids.dmnd)
    created = created + fill_rectangle(585, 54, 588, 343, ids.dmnd)
    created = created + fill_rectangle(24, 340, 584, 343, ids.brck)
    created = created + fill_rectangle(180, 54, 183, 339, ids.dmnd)
    created = created + fill_rectangle(340, 54, 343, 339, ids.dmnd)

    -- Dense powder and liquid beds exercise movement, pmap and heat transfer.
    created = created + fill_rectangle(30, 200, 175, 335, ids.dust)
    created = created + fill_rectangle(200, 220, 335, 335, ids.watr)
    created = created + fill_rectangle(205, 215, 235, 215, ids.plnt)
    created = created + fill_rectangle(240, 215, 260, 215, ids.salt)
    created = created + fill_rectangle(285, 205, 305, 210, ids.lava)

    -- Alternating gas species create a deterministic advection/mixing workload.
    local gas_types = { ids.o2, ids.co2, ids.gas, ids.smke }
    for y = 90, 280, 2 do
        for x = 365, 575, 2 do
            local slot = (math.floor(x / 2) + math.floor(y / 2)) % #gas_types + 1
            create_particle(x, y, gas_types[slot])
            created = created + 1
        end
    end

    -- A combustible lower bed and a small ignition front exercise reactions.
    created = created + fill_rectangle(365, 305, 575, 307, ids.wood)
    created = created + fill_rectangle(365, 310, 455, 335, ids.oil)
    created = created + fill_rectangle(380, 285, 420, 295, ids.fire, 2, 2)

    -- A bounded conductor and battery add the official electrical update path.
    created = created + fill_rectangle(365, 70, 575, 73, ids.metl)
    local battery = create_particle(370, 68, ids.btry)
    created = created + 1
    local spark_host = sim.partID(371, 70)
    assert(spark_host and sim.partExists(spark_host), "missing spark host")
    sim.partProperty(spark_host, "type", ids.sprk)
    sim.partProperty(spark_host, "ctype", ids.metl)
    sim.partProperty(spark_host, "life", 4)
    assert(sim.partExists(battery), "battery creation did not persist")

    -- Seed nonuniform Legacy Air fields without including this setup in timing.
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

local function write_result(status, values)
    local result = assert(io.open(RESULT_FILE, "wb"))
    result:write("OMNI_FIXED_STEP_STATUS=" .. status .. "\n")
    for _, key in ipairs(values.order) do
        result:write(key .. "=" .. tostring(values[key]) .. "\n")
    end
    result:close()
end

local function run_benchmark()
    tpt.fpsCap(2)
    tpt.drawCap(1)

    local result = {
        order = {
            "schema_version", "scenario", "warmup_steps", "steps_per_pass",
            "passes", "seed", "atmosphere_cells", "simulation_dt_value",
            "simulation_dt_unit", "reset_sanitization_steps", "timing_scope", "omni_profiler_enabled", "generated_particles",
            "initial_particles", "final_particles", "initial_state_hash",
            "final_state_hash", "deterministic_replay",
        },
        schema_version = schema_version,
        scenario = scenario_name,
        warmup_steps = warmup_steps,
        steps_per_pass = steps_per_pass,
        passes = pass_count,
        seed = table.concat(seed, ","),
        atmosphere_cells = sim.XCELLS * sim.YCELLS,
        simulation_dt_value = 1,
        simulation_dt_unit = "legacy_tick",
        reset_sanitization_steps = 1,
        timing_scope = "synchronous_sim_updateUpTo_loop",
        omni_profiler_enabled = tostring(omni_profiler_enabled),
    }

    local reference_generated_hash
    local reference_initial_hash
    local reference_final_hash
    local reference_initial_particles
    local reference_final_particles
    local reference_created
    local deterministic = true
    local determinism_mismatches = {}

    for pass = 1, pass_count do
        reset_world()
        sim.resetOmniEventMetrics()
        local created = generators[scenario_name]()
        local generated_hash = sim.hash()

        for _ = 1, warmup_steps do
            sim.updateUpTo()
        end
        collectgarbage("collect")

        -- This reset is outside the timed region. When enabled, the following
        -- metrics therefore describe exactly the synchronous fixed-step loop.
        sim.resetOmniProfiler()

        local initial_particles = sim.partCount()
        local initial_hash = sim.hash()
        local started = socket.getTime()
        for _ = 1, steps_per_pass do
            sim.updateUpTo()
        end
        local elapsed_seconds = socket.getTime() - started
        local final_particles = sim.partCount()
        local final_hash = sim.hash()
        local profiler = sim.omniProfiler()

        assert(profiler.enabled == omni_profiler_enabled,
            "profiler enable state does not match benchmark configuration")
        local expected_profiler_calls = omni_profiler_enabled and steps_per_pass or 0
        for _, subsystem in ipairs({
            "frame", "simulation", "particle_update", "air", "ambient_heat",
            "gravity_dispatch_wait",
        }) do
            assert(profiler.subsystems[subsystem].calls == expected_profiler_calls,
                "unexpected profiler calls for " .. subsystem)
        end

        assert(elapsed_seconds > 0.0, "benchmark pass elapsed time is not positive")

        if pass == 1 then
            reference_created = created
            reference_generated_hash = generated_hash
            reference_initial_hash = initial_hash
            reference_final_hash = final_hash
            reference_initial_particles = initial_particles
            reference_final_particles = final_particles
        else
            for field, pair in pairs({
                generated_particles = { created, reference_created },
                generated_hash = { generated_hash, reference_generated_hash },
                initial_hash = { initial_hash, reference_initial_hash },
                final_hash = { final_hash, reference_final_hash },
                initial_particles = { initial_particles, reference_initial_particles },
                final_particles = { final_particles, reference_final_particles },
            }) do
                if pair[1] ~= pair[2] then
                    deterministic = false
                    determinism_mismatches[#determinism_mismatches + 1] =
                        "pass" .. pass .. ":" .. field .. ":" .. pair[1] .. "!=" .. pair[2]
                end
            end
        end

        for _, key in ipairs({
            "elapsed_seconds", "generated_hash", "initial_hash", "final_hash",
            "initial_particles", "final_particles",
            "profiler_frame_calls", "profiler_simulation_calls",
        }) do
            result.order[#result.order + 1] = "pass_" .. pass .. "_" .. key
        end
        result["pass_" .. pass .. "_elapsed_seconds"] = string.format("%.9f", elapsed_seconds)
        result["pass_" .. pass .. "_generated_hash"] = generated_hash
        result["pass_" .. pass .. "_initial_hash"] = initial_hash
        result["pass_" .. pass .. "_final_hash"] = final_hash
        result["pass_" .. pass .. "_initial_particles"] = initial_particles
        result["pass_" .. pass .. "_final_particles"] = final_particles
        result["pass_" .. pass .. "_profiler_frame_calls"] = profiler.subsystems.frame.calls
        result["pass_" .. pass .. "_profiler_simulation_calls"] = profiler.subsystems.simulation.calls
    end

    result.generated_particles = reference_created
    result.initial_particles = reference_initial_particles
    result.final_particles = reference_final_particles
    result.initial_state_hash = reference_initial_hash
    result.final_state_hash = reference_final_hash
    result.deterministic_replay = tostring(deterministic)
    if not deterministic then
        result.order[#result.order + 1] = "error"
        result.order[#result.order + 1] = "determinism_mismatches"
        result.error = "same-process deterministic replay diverged between passes"
        result.determinism_mismatches = table.concat(determinism_mismatches, ";")
        write_result("FAIL", result)
        return false
    end

    write_result("PASS", result)
    return true
end

local ok, completed_or_error = xpcall(run_benchmark, debug.traceback)
if not ok then
    write_result("FAIL", {
        order = { "schema_version", "scenario", "error" },
        schema_version = schema_version,
        scenario = scenario_name,
        error = tostring(completed_or_error):gsub("[\r\n]+", " | "),
    })
    os.exit(1)
end

if not completed_or_error then
    os.exit(1)
end

os.exit(0)
