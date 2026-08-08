local CONFIG_FILE = "characterization.config"

local function read_config()
    local file = assert(io.open(CONFIG_FILE, "rb"), "cannot open " .. CONFIG_FILE)
    local text = assert(file:read("*a"), "cannot read " .. CONFIG_FILE)
    file:close()
    local values = {}
    for line in text:gmatch("[^\r\n]+") do
        local key, value = line:match("^([A-Za-z0-9_]+)=(.*)$")
        assert(key, "invalid characterization config line: " .. line)
        assert(values[key] == nil, "duplicate characterization config key: " .. key)
        values[key] = value
    end
    return values
end

local config = read_config()

local function required_integer(name, minimum, maximum)
    local text = assert(config[name], "missing characterization config key: " .. name)
    assert(text:match("^%d+$"), "characterization config key is not an integer: " .. name)
    local value = assert(tonumber(text), "cannot parse characterization config key: " .. name)
    assert(value >= minimum and value <= maximum,
        name .. " must be in [" .. minimum .. ", " .. maximum .. "]")
    return value
end

local schema_version = required_integer("schema_version", 1, 1)
local phase = assert(config.phase, "missing characterization phase")
assert(phase == "generate" or phase == "verify-a" or phase == "verify-b",
    "invalid characterization phase: " .. phase)
local case_id = assert(config.case_id, "missing characterization case_id")
assert(case_id:match("^C%d%d$"), "invalid characterization case_id: " .. case_id)
local slug = assert(config.slug, "missing characterization slug")
assert(slug:match("^[a-z0-9-]+$"), "invalid characterization slug: " .. slug)
local profile_name = assert(config.settings_profile, "missing settings profile")
local warmup_steps = required_integer("warmup_steps", 0, 100000)
local trace_steps = required_integer("trace_steps", 1, 100000)
local expected_min_particles = required_integer("expected_min_particles", 0, sim.MAX_PARTS)
local expected_max_particles = required_integer("expected_max_particles", 0, sim.MAX_PARTS)
assert(expected_min_particles <= expected_max_particles, "invalid expected particle range")
local seed = {
    required_integer("seed_a", 0, 4294967295),
    required_integer("seed_b", 0, 4294967295),
    required_integer("seed_c", 0, 4294967295),
    required_integer("seed_d", 0, 4294967295),
}

local required_identifiers = {}
for identifier in assert(config.required_identifiers,
    "missing required_identifiers"):gmatch("[^,]+") do
    assert(identifier:match("^[A-Z0-9_-]+$"),
        "invalid required element identifier: " .. identifier)
    required_identifiers[#required_identifiers + 1] = identifier
end
assert(#required_identifiers > 0, "required_identifiers is empty")

assert(type(sim.updateUpTo) == "function", "sim.updateUpTo is unavailable")
assert(type(sim.hash) == "function", "sim.hash is unavailable")
assert(type(sim.ensureDeterminism) == "function", "sim.ensureDeterminism is unavailable")
assert(type(sim.randomSeed) == "function", "sim.randomSeed is unavailable")

local ids = {
    aray = assert(elements.DEFAULT_PT_ARAY),
    brck = assert(elements.DEFAULT_PT_BRCK),
    btry = assert(elements.DEFAULT_PT_BTRY),
    co2 = assert(elements.DEFAULT_PT_CO2),
    dlay = assert(elements.DEFAULT_PT_DLAY),
    dmnd = assert(elements.DEFAULT_PT_DMND),
    dust = assert(elements.DEFAULT_PT_DUST),
    filt = assert(elements.DEFAULT_PT_FILT),
    fire = assert(elements.DEFAULT_PT_FIRE),
    gas = assert(elements.DEFAULT_PT_GAS),
    glas = assert(elements.DEFAULT_PT_GLAS),
    ice = assert(elements.DEFAULT_PT_ICEI),
    insl = assert(elements.DEFAULT_PT_INSL),
    inwr = assert(elements.DEFAULT_PT_INWR),
    lava = assert(elements.DEFAULT_PT_LAVA),
    metl = assert(elements.DEFAULT_PT_METL),
    nitr = assert(elements.DEFAULT_PT_NITR),
    nscn = assert(elements.DEFAULT_PT_NSCN),
    o2 = assert(elements.DEFAULT_PT_O2),
    oil = assert(elements.DEFAULT_PT_OIL),
    phot = assert(elements.DEFAULT_PT_PHOT),
    pipe = assert(elements.DEFAULT_PT_PIPE),
    plex = assert(elements.DEFAULT_PT_PLEX),
    ppip = assert(elements.DEFAULT_PT_PPIP),
    pscn = assert(elements.DEFAULT_PT_PSCN),
    sand = assert(elements.DEFAULT_PT_SAND),
    salt = assert(elements.DEFAULT_PT_SALT),
    smke = assert(elements.DEFAULT_PT_SMKE),
    sprk = assert(elements.DEFAULT_PT_SPRK),
    watr = assert(elements.DEFAULT_PT_WATR),
    wifi = assert(elements.DEFAULT_PT_WIFI),
    wood = assert(elements.DEFAULT_PT_WOOD),
}

local profiles = {
    standard = {
        edge = sim.EDGE_VOID,
        gravity = sim.GRAV_VERTICAL,
        air = sim.AIR_ON,
        ambient_heat = true,
        heat = true,
    },
    sealed = {
        edge = sim.EDGE_SOLID,
        gravity = sim.GRAV_VERTICAL,
        air = sim.AIR_ON,
        ambient_heat = true,
        heat = true,
    },
    field = {
        edge = sim.EDGE_SOLID,
        gravity = sim.GRAV_OFF,
        air = sim.AIR_ON,
        ambient_heat = true,
        heat = true,
    },
    electronics = {
        edge = sim.EDGE_VOID,
        gravity = sim.GRAV_OFF,
        air = sim.AIR_ON,
        ambient_heat = true,
        heat = true,
    },
    ["max-inert"] = {
        edge = sim.EDGE_VOID,
        gravity = sim.GRAV_OFF,
        air = sim.AIR_OFF,
        ambient_heat = false,
        heat = false,
    },
}
local selected_profile = assert(profiles[profile_name],
    "unknown settings profile: " .. profile_name)

local function apply_profile()
    sim.paused(true)
    sim.edgeMode(selected_profile.edge)
    sim.gravityMode(selected_profile.gravity)
    sim.newtonianGravity(false)
    sim.airMode(selected_profile.air)
    sim.convectionMode(sim.AIRC_LEGACY)
    sim.ambientHeatSim(selected_profile.ambient_heat)
    sim.heatSim(selected_profile.heat)
    sim.waterEqualization(0)
    sim.ambientAirTemp(295.15)
    sim.edgePressure(0.0)
    sim.edgeVelocity(0.0, 0.0)
    sim.ensureDeterminism(true)
    sim.randomSeed(seed[1], seed[2], seed[3], seed[4])
end

local function reset_world()
    -- clearSim leaves derived Air blocking maps. Rebuild them with one empty
    -- frame, then clear again so characterization starts from a pristine cache.
    sim.clearSim()
    apply_profile()
    sim.updateUpTo()
    sim.clearSim()
    apply_profile()
end

local function create_particle(x, y, element, properties)
    local particle = sim.partCreate(-1, x, y, element)
    assert(particle >= 0,
        "particle creation failed at " .. x .. "," .. y .. " for element " .. element)
    if properties then
        for property, value in pairs(properties) do
            sim.partProperty(particle, property, value)
        end
    end
    return particle
end

local function fill_rectangle(x1, y1, x2, y2, element, stride_x, stride_y, properties)
    local created = 0
    for y = y1, y2, stride_y or 1 do
        for x = x1, x2, stride_x or 1 do
            create_particle(x, y, element, properties)
            created = created + 1
        end
    end
    return created
end

local function create_box()
    local created = 0
    created = created + fill_rectangle(20, 40, 591, 43, ids.dmnd)
    created = created + fill_rectangle(20, 44, 23, 343, ids.dmnd)
    created = created + fill_rectangle(588, 44, 591, 343, ids.dmnd)
    created = created + fill_rectangle(24, 340, 587, 343, ids.dmnd)
    return created
end

local function spark_particle(particle, conductor)
    sim.partProperty(particle, "type", ids.sprk)
    sim.partProperty(particle, "ctype", conductor)
    sim.partProperty(particle, "life", 4)
end

local function generate_c01()
    return create_box() + fill_rectangle(60, 100, 230, 320, ids.sand)
end

local function generate_c02()
    return create_box() + fill_rectangle(80, 160, 320, 335, ids.watr)
end

local function generate_c03()
    local created = create_box()
    local gas_types = { ids.o2, ids.co2, ids.gas, ids.smke }
    for y = 70, 325, 3 do
        for x = 50, 560, 3 do
            local slot = (math.floor(x / 3) + math.floor(y / 3)) % #gas_types + 1
            create_particle(x, y, gas_types[slot])
            created = created + 1
        end
    end
    sim.pressure(20, 10, 50, 60, 4.0)
    sim.velocityX(20, 10, 50, 60, 1.25)
    return created
end

local function generate_c04()
    local created = create_box()
    created = created + fill_rectangle(100, 280, 250, 335, ids.wood)
    created = created + fill_rectangle(300, 300, 450, 335, ids.oil)
    created = created + fill_rectangle(60, 70, 550, 260, ids.o2, 4, 4)
    created = created + fill_rectangle(180, 268, 220, 276, ids.fire, 2, 2)
    return created
end

local function generate_c05()
    local created = create_box()
    created = created + fill_rectangle(120, 240, 240, 330, ids.nitr)
    created = created + fill_rectangle(320, 270, 440, 330, ids.plex)
    created = created + fill_rectangle(175, 225, 205, 235, ids.fire, 2, 2)
    sim.pressure(40, 40, 20, 20, 8.0)
    return created
end

local function generate_c06()
    local created = create_box()
    for y = 80, 320, 3 do
        for x = 60, 550, 3 do
            create_particle(x, y, ((x + y) % 2 == 0) and ids.o2 or ids.gas)
            created = created + 1
        end
    end
    sim.pressure(20, 15, 110, 65, -80.0)
    sim.velocityX(20, 15, 110, 65, 0.0)
    sim.velocityY(20, 15, 110, 65, 0.0)
    return created
end

local function generate_c07()
    local created = create_box()
    created = created + fill_rectangle(210, 160, 390, 300, ids.dust, 2, 2)
    sim.pressure(5, 10, 65, 75, 24.0)
    sim.pressure(80, 10, 65, 75, -20.0)
    sim.velocityX(5, 10, 65, 75, 3.0)
    sim.velocityX(80, 10, 65, 75, -3.0)
    return created
end

local function generate_c08()
    local created = create_box()
    created = created + fill_rectangle(60, 120, 180, 260, ids.metl, 2, 2, { temp = 173.15 })
    created = created + fill_rectangle(220, 180, 360, 320, ids.watr, 2, 1, { temp = 295.15 })
    created = created + fill_rectangle(400, 220, 500, 320, ids.lava, 2, 2, { temp = 1800.0 })
    created = created + fill_rectangle(400, 100, 500, 180, ids.ice, 2, 2, { temp = 240.0 })
    sim.ambientHeat(10, 10, 55, 70, 220.0)
    sim.ambientHeat(85, 10, 55, 70, 1200.0)
    return created
end

local function generate_c09()
    local created = create_box()
    for lane = 0, 9 do
        local y = 70 + lane * 24
        created = created + fill_rectangle(70, y, 299, y + 1, ids.metl)
        created = created + fill_rectangle(306, y, 540, y + 1, ids.metl)
        create_particle(68, y, ids.btry)
        created = created + 1
        local host = sim.partID(70, y)
        assert(host, "missing electrical spark host")
        spark_particle(host, ids.metl)
    end
    created = created + fill_rectangle(300, 60, 305, 320, ids.insl)
    return created
end

local function generate_c10()
    local created = create_box()
    created = created + fill_rectangle(200, 70, 203, 320, ids.filt)
    created = created + fill_rectangle(350, 70, 353, 320, ids.glas)
    for y = 70, 315, 4 do
        for x = 60, 120, 4 do
            local photon = create_particle(x, y, ids.phot, {
                vx = 3.0,
                vy = ((x + y) % 8 == 0) and 0.25 or -0.25,
                ctype = 0x3FFFFFFF,
            })
            assert(sim.partExists(photon), "photon did not persist")
            created = created + 1
        end
    end
    return created
end

local function pipe_properties(index)
    local colors = { 0x00040000, 0x00080000, 0x000C0000 }
    return {
        life = 0,
        tmp = colors[index % #colors + 1],
        ctype = ids.watr,
        temp = 295.15,
    }
end

local function generate_c11()
    local created = create_box()
    local index = 0
    for y = 100, 103 do
        for x = 80, 530 do
            local element = (x % 17 == 0) and ids.ppip or ids.pipe
            create_particle(x, y, element, pipe_properties(index))
            index = index + 1
            created = created + 1
        end
    end
    for y = 104, 300 do
        for x = 80, 83 do
            local element = (y % 19 == 0) and ids.ppip or ids.pipe
            create_particle(x, y, element, pipe_properties(index))
            index = index + 1
            created = created + 1
        end
        for x = 527, 530 do
            local element = (y % 23 == 0) and ids.ppip or ids.pipe
            create_particle(x, y, element, pipe_properties(index))
            index = index + 1
            created = created + 1
        end
    end
    created = created + fill_rectangle(84, 297, 526, 300, ids.pipe, 1, 1,
        { life = 0, tmp = 0x00040000, ctype = ids.watr })
    return created
end

local function generate_c12()
    local created = create_box()
    for lane = 0, 7 do
        local y = 70 + lane * 32
        created = created + fill_rectangle(70, y, 304, y + 1, ids.inwr)
        created = created + fill_rectangle(309, y, 540, y + 1, ids.inwr)
        create_particle(68, y, ids.btry)
        create_particle(90, y + 6, ids.aray)
        created = created + 2
        created = created + fill_rectangle(110, y + 6, 250, y + 7, ids.filt)
        created = created + fill_rectangle(280, y + 6, 300, y + 7, ids.dlay,
            1, 1, { temp = 300.0 + lane * 25.0 })
        create_particle(340, y + 6, ids.wifi, { temp = 373.15 + lane * 100.0 })
        create_particle(470, y + 6, ids.wifi, { temp = 373.15 + lane * 100.0 })
        created = created + 2
        local host = sim.partID(70, y)
        assert(host, "missing complex-electronics spark host")
        spark_particle(host, ids.inwr)
    end
    created = created + fill_rectangle(305, 60, 308, 325, ids.insl)
    created = created + fill_rectangle(40, 320, 200, 323, ids.pscn)
    created = created + fill_rectangle(410, 320, 570, 323, ids.nscn)
    return created
end

local function generate_c13()
    local created = create_box()
    created = created + fill_rectangle(40, 210, 175, 335, ids.sand)
    created = created + fill_rectangle(200, 230, 335, 335, ids.watr)
    local gas_types = { ids.o2, ids.co2, ids.gas, ids.smke }
    for y = 80, 275, 3 do
        for x = 365, 560, 3 do
            local slot = (math.floor(x / 3) + math.floor(y / 3)) % #gas_types + 1
            create_particle(x, y, gas_types[slot])
            created = created + 1
        end
    end
    created = created + fill_rectangle(365, 305, 560, 307, ids.wood)
    created = created + fill_rectangle(365, 312, 455, 335, ids.oil)
    created = created + fill_rectangle(390, 288, 420, 296, ids.fire, 2, 2)
    created = created + fill_rectangle(365, 60, 560, 63, ids.metl)
    create_particle(360, 60, ids.btry)
    created = created + 1
    local host = assert(sim.partID(365, 60), "missing mixed spark host")
    spark_particle(host, ids.metl)
    sim.pressure(90, 15, 40, 50, 6.0)
    sim.velocityX(90, 15, 40, 50, -1.0)
    sim.ambientHeat(90, 15, 40, 50, 350.0)
    return created
end

local function generate_c14()
    local created = 0
    for y = 0, sim.YRES - 1 do
        for x = 0, sim.XRES - 1 do
            create_particle(x, y, ids.dmnd)
            created = created + 1
        end
    end
    assert(created == sim.MAX_PARTS,
        "maximum-particle generator did not fill NPART: " .. created)
    return created
end

local generators = {
    C01 = generate_c01,
    C02 = generate_c02,
    C03 = generate_c03,
    C04 = generate_c04,
    C05 = generate_c05,
    C06 = generate_c06,
    C07 = generate_c07,
    C08 = generate_c08,
    C09 = generate_c09,
    C10 = generate_c10,
    C11 = generate_c11,
    C12 = generate_c12,
    C13 = generate_c13,
    C14 = generate_c14,
}
assert(generators[case_id], "no generator for characterization case: " .. case_id)

local function required_counts(require_present)
    local counts = {}
    for _, identifier in ipairs(required_identifiers) do
        local element = elements[identifier]
        assert(type(element) == "number", "missing required element: " .. identifier)
        local count = sim.elementCount(element)
        if require_present then
            assert(count > 0,
                "required element is absent from state: " .. identifier)
        end
        counts[#counts + 1] = identifier .. ":" .. count
    end
    return table.concat(counts, "|")
end

local function assert_particle_range(count)
    assert(count >= expected_min_particles and count <= expected_max_particles,
        "particle count outside expected range: " .. count .. " not in ["
        .. expected_min_particles .. "," .. expected_max_particles .. "]")
end

local function rng_text()
    local a, b, c, d = sim.randomSeed()
    return table.concat({ a, b, c, d }, ",")
end

local function write_result(status, values)
    local path = "characterization-" .. phase .. ".result"
    local result = assert(io.open(path, "wb"))
    result:write("OMNI_CHARACTERIZATION_STATUS=" .. status .. "\n")
    for _, key in ipairs(values.order) do
        result:write(key .. "=" .. tostring(values[key]) .. "\n")
    end
    result:close()
end

local function base_result_order()
    return {
        "schema_version", "phase", "case_id", "slug", "settings_profile",
        "seed", "warmup_steps", "trace_steps", "expected_min_particles",
        "expected_max_particles", "required_identifiers",
    }
end

local function base_result()
    return {
        order = base_result_order(),
        schema_version = schema_version,
        phase = phase,
        case_id = case_id,
        slug = slug,
        settings_profile = profile_name,
        seed = table.concat(seed, ","),
        warmup_steps = warmup_steps,
        trace_steps = trace_steps,
        expected_min_particles = expected_min_particles,
        expected_max_particles = expected_max_particles,
        required_identifiers = table.concat(required_identifiers, ","),
    }
end

local function append_fields(result, fields)
    for _, field in ipairs(fields) do
        result.order[#result.order + 1] = field
    end
end

local function run_generate()
    tpt.fpsCap(2)
    tpt.drawCap(1)
    reset_world()
    local generated_particles = generators[case_id]()
    assert(generated_particles == sim.partCount(),
        "generator count differs from Simulation count")
    local generated_hash = sim.hash()
    for _ = 1, warmup_steps do
        sim.updateUpTo()
    end
    local particles_before_save = sim.partCount()
    assert_particle_range(particles_before_save)
    local counts_before_save = required_counts(true)
    local hash_before_save = sim.hash()
    local rng_before_save = rng_text()
    local first_stamp = sim.saveStamp(0, 0, sim.XRES - 1, sim.YRES - 1, 1)
    local second_stamp = sim.saveStamp(0, 0, sim.XRES - 1, sim.YRES - 1, 1)
    assert(type(first_stamp) == "string" and first_stamp:match("^[0-9A-Fa-f]+$")
            and #first_stamp == 10, "first saveStamp returned an invalid ID")
    assert(type(second_stamp) == "string" and second_stamp:match("^[0-9A-Fa-f]+$")
            and #second_stamp == 10, "second saveStamp returned an invalid ID")
    assert(first_stamp ~= second_stamp, "duplicate saveStamp IDs")
    assert(sim.hash() == hash_before_save, "saveStamp changed Simulation state")
    assert(rng_text() == rng_before_save, "saveStamp changed Simulation RNG")

    local result = base_result()
    append_fields(result, {
        "generated_particles", "generated_hash", "particles_before_save",
        "hash_before_save", "rng_before_save", "required_counts_before_save",
        "first_stamp", "second_stamp", "reset_sanitization_steps",
    })
    result.generated_particles = generated_particles
    result.generated_hash = generated_hash
    result.particles_before_save = particles_before_save
    result.hash_before_save = hash_before_save
    result.rng_before_save = rng_before_save
    result.required_counts_before_save = counts_before_save
    result.first_stamp = first_stamp
    result.second_stamp = second_stamp
    result.reset_sanitization_steps = 1
    write_result("PASS", result)
end

local function verify_loaded_state()
    local callback
    callback = function()
        event.unregister(event.tick, callback)
        local ok, error_text = xpcall(function()
            assert(sim.paused(), "loaded characterization save is not paused")
            assert(sim.ensureDeterminism(),
                "loaded characterization save lost ensureDeterminism")
            assert(sim.edgeMode() == selected_profile.edge, "loaded edge mode changed")
            assert(sim.gravityMode() == selected_profile.gravity, "loaded gravity mode changed")
            assert(sim.airMode() == selected_profile.air, "loaded Air mode changed")
            assert(sim.ambientHeatSim() == selected_profile.ambient_heat,
                "loaded ambient heat setting changed")
            assert(sim.heatSim() == selected_profile.heat,
                "loaded heat setting changed")

            local loaded_particles = sim.partCount()
            assert_particle_range(loaded_particles)
            local loaded_counts = required_counts(true)
            local loaded_hash = sim.hash()
            local loaded_rng = rng_text()
            local trace_path = "characterization-" .. phase .. ".csv"
            local trace = assert(io.open(trace_path, "wb"))
            trace:write("frame_offset,state_hash_fnv1a32,particles,required_counts\n")
            trace:write("0," .. loaded_hash .. "," .. loaded_particles .. ","
                .. loaded_counts .. "\n")
            for step = 1, trace_steps do
                sim.updateUpTo()
                trace:write(step .. "," .. sim.hash() .. "," .. sim.partCount()
                    .. "," .. required_counts(false) .. "\n")
            end
            trace:close()

            local final_particles = sim.partCount()
            local final_hash = sim.hash()
            local final_rng = rng_text()
            local final_counts = required_counts(false)
            local result = base_result()
            append_fields(result, {
                "loaded_particles", "loaded_hash", "loaded_rng",
                "loaded_required_counts", "final_particles", "final_hash",
                "final_rng", "final_required_counts", "trace_file",
            })
            result.loaded_particles = loaded_particles
            result.loaded_hash = loaded_hash
            result.loaded_rng = loaded_rng
            result.loaded_required_counts = loaded_counts
            result.final_particles = final_particles
            result.final_hash = final_hash
            result.final_rng = final_rng
            result.final_required_counts = final_counts
            result.trace_file = trace_path
            write_result("PASS", result)
        end, debug.traceback)
        if not ok then
            write_result("FAIL", {
                order = { "schema_version", "phase", "case_id", "slug", "error" },
                schema_version = schema_version,
                phase = phase,
                case_id = case_id,
                slug = slug,
                error = tostring(error_text):gsub("[\r\n]+", " | "),
            })
            os.exit(1)
        end
        os.exit(0)
    end

    tpt.fpsCap(2)
    tpt.drawCap(1)
    sim.paused(true)
    event.register(event.tick, callback)
end

if phase == "generate" then
    local ok, error_text = xpcall(run_generate, debug.traceback)
    if not ok then
        write_result("FAIL", {
            order = { "schema_version", "phase", "case_id", "slug", "error" },
            schema_version = schema_version,
            phase = phase,
            case_id = case_id,
            slug = slug,
            error = tostring(error_text):gsub("[\r\n]+", " | "),
        })
        os.exit(1)
    end
    os.exit(0)
else
    local ok, error_text = xpcall(verify_loaded_state, debug.traceback)
    if not ok then
        write_result("FAIL", {
            order = { "schema_version", "phase", "case_id", "slug", "error" },
            schema_version = schema_version,
            phase = phase,
            case_id = case_id,
            slug = slug,
            error = tostring(error_text):gsub("[\r\n]+", " | "),
        })
        os.exit(1)
    end
end
