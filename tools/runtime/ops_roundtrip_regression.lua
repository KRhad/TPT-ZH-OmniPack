local PHASE_FILE = "ops-roundtrip.phase"
local STATE_FILE = "ops-roundtrip.state"

local function read_all(path)
    local file = assert(io.open(path, "rb"), "cannot open " .. path)
    local contents = assert(file:read("*a"), "cannot read " .. path)
    file:close()
    return contents
end

local phase = tonumber(read_all(PHASE_FILE):match("%d+"))
assert(phase == 1 or phase == 2 or phase == 3,
    "invalid OPS roundtrip phase: " .. tostring(phase))

local RESULT = "ops-roundtrip-phase" .. phase .. ".result"

local function must_element(identifier, short_name, stable_id)
    local id = elements[identifier]
    assert(type(id) == "number", "missing element constant: " .. identifier)
    assert(elements.getByName(short_name) == id,
        "name/identifier mismatch for " .. identifier)
    assert(id == stable_id,
        identifier .. " stable ID changed: expected " .. stable_id
        .. ", got " .. tostring(id))
    return id
end

local ids = {
    dust = assert(elements.DEFAULT_PT_DUST),
    water = assert(elements.DEFAULT_PT_WATR),
    lava = assert(elements.DEFAULT_PT_LAVA),
    spark = assert(elements.DEFAULT_PT_SPRK),
    conv = assert(elements.DEFAULT_PT_CONV),
    virs = assert(elements.DEFAULT_PT_VIRS),
    alum = must_element("OMNI_PT_ALUM", "ALUM", 256),
    mscr = must_element("OMNI_PT_MSCR", "MSCR", 278),
    nutr = must_element("OMNI_PT_NUTR", "NUTR", 288),
    nful = must_element("OMNI_PT_NFUL", "NFUL", 328),
    chlr = must_element("OMNI_PT_CHLR", "CHLR", 360),
}

assert(ids.alum > 255 and ids.mscr > 255 and ids.nutr > 255
    and ids.nful > 255 and ids.chlr > 255,
    "fixture requires OmniPack element IDs above 255")

local fixtures = {
    {
        name = "official_dust",
        x = 80,
        y = 80,
        particle_type = ids.dust,
        properties = { life = 123 },
    },
    {
        name = "official_water",
        x = 88,
        y = 80,
        particle_type = ids.water,
        properties = { tmp = 17 },
    },
    {
        name = "direct_alum_gt255",
        x = 96,
        y = 80,
        particle_type = ids.alum,
        properties = {},
    },
    {
        name = "direct_nutr_gt255",
        x = 104,
        y = 80,
        particle_type = ids.nutr,
        properties = {},
    },
    {
        name = "direct_nful_gt255",
        x = 112,
        y = 80,
        particle_type = ids.nful,
        properties = {},
    },
    {
        name = "direct_chlr_gt255",
        x = 120,
        y = 80,
        particle_type = ids.chlr,
        properties = {},
    },
    {
        name = "lava_ctype_gt255",
        x = 128,
        y = 80,
        particle_type = ids.lava,
        properties = { ctype = ids.alum },
    },
    {
        name = "spark_ctype_gt255",
        x = 136,
        y = 80,
        create_type = ids.alum,
        particle_type = ids.spark,
        properties = { ctype = ids.nful, life = 4 },
    },
    {
        name = "mscr_ctype_gt255",
        x = 144,
        y = 80,
        particle_type = ids.mscr,
        properties = { ctype = ids.alum },
    },
    {
        name = "conv_tmp_gt255",
        x = 152,
        y = 80,
        particle_type = ids.conv,
        properties = { ctype = ids.nful, tmp = ids.nutr },
    },
    {
        name = "virs_tmp2_gt255",
        x = 160,
        y = 80,
        particle_type = ids.virs,
        properties = { tmp2 = ids.chlr },
    },
}

local function configure_empty_simulation()
    sim.clearSim()
    sim.paused(true)
    sim.gravityMode(sim.GRAV_OFF)
    sim.airMode(sim.AIR_OFF)
    sim.ambientHeatSim(false)
    sim.heatSim(true)
    sim.ensureDeterminism(true)
    sim.randomSeed(1, 2, 3, 4)
end

local function create_fixture()
    configure_empty_simulation()
    for _, fixture in ipairs(fixtures) do
        local particle = sim.partCreate(
            -1,
            fixture.x,
            fixture.y,
            fixture.create_type or fixture.particle_type)
        assert(particle >= 0,
            "failed to create fixture particle: " .. fixture.name)
        if fixture.create_type then
            sim.partProperty(particle, "type", fixture.particle_type)
        end
        for property, value in pairs(fixture.properties) do
            sim.partProperty(particle, property, value)
        end
        assert(sim.partID(fixture.x, fixture.y) == particle,
            "fixture particle is not addressable: " .. fixture.name)
    end
end

local function verify_fixture()
    local assertions = 0
    for _, fixture in ipairs(fixtures) do
        local particle = sim.partID(fixture.x, fixture.y)
        assert(type(particle) == "number",
            "missing loaded fixture particle: " .. fixture.name)
        local actual_type = sim.partProperty(particle, "type")
        assert(actual_type == fixture.particle_type,
            fixture.name .. " type changed: expected "
            .. fixture.particle_type .. ", got " .. tostring(actual_type))
        assertions = assertions + 1
        for property, expected in pairs(fixture.properties) do
            local actual = sim.partProperty(particle, property)
            assert(actual == expected,
                fixture.name .. "." .. property .. " changed: expected "
                .. expected .. ", got " .. tostring(actual))
            assertions = assertions + 1
        end
    end

    local particle_count = 0
    for _ in sim.parts() do
        particle_count = particle_count + 1
    end
    assert(particle_count == #fixtures,
        "loaded particle count changed: expected " .. #fixtures
        .. ", got " .. particle_count)
    return assertions, particle_count
end

local function read_state()
    local state = {}
    for line in read_all(STATE_FILE):gmatch("[^\r\n]+") do
        local key, value = line:match("^([%w_]+)=(.+)$")
        if key then
            state[key] = value
        end
    end
    return state
end

local function write_state(stamp1, stamp2)
    local state = assert(io.open(STATE_FILE, "wb"))
    state:write("stamp1=" .. stamp1 .. "\n")
    if stamp2 then
        state:write("stamp2=" .. stamp2 .. "\n")
    end
    state:close()
end

local function save_entire_simulation()
    local stamp = sim.saveStamp(0, 0, sim.XRES - 1, sim.YRES - 1, 1)
    assert(type(stamp) == "string" and stamp:match("^[0-9A-Fa-f]+$")
        and #stamp == 10,
        "client did not return a ten-character stamp ID: " .. tostring(stamp))
    return stamp
end

local required_palette_identifiers = {
    "DEFAULT_PT_DUST",
    "DEFAULT_PT_WATR",
    "DEFAULT_PT_LAVA",
    "DEFAULT_PT_SPRK",
    "DEFAULT_PT_CONV",
    "DEFAULT_PT_VIRS",
    "OMNI_PT_ALUM",
    "OMNI_PT_MSCR",
    "OMNI_PT_NUTR",
    "OMNI_PT_NFUL",
    "OMNI_PT_CHLR",
}

local function inspect_ops_file(stamp)
    local raw = read_all("stamps/" .. stamp .. ".stm")
    assert(#raw > 15, "saved OPS file is too short")
    assert(raw:sub(1, 4) == "OPS1", "saved file is not OPS1")
    assert(raw:sub(13, 15) == "BZh", "OPS payload is not bzip2")
    local b1, b2, b3, b4 = raw:byte(9, 12)
    local declared_size = b1 + b2 * 256 + b3 * 65536 + b4 * 16777216
    assert(declared_size > 0 and declared_size <= 209715200,
        "OPS declared payload size is invalid: " .. declared_size)
    local payload, error_code, error_text = bz2.decompress(
        raw:sub(13), declared_size)
    assert(payload,
        "cannot decompress OPS payload: " .. tostring(error_code)
        .. ": " .. tostring(error_text))
    assert(#payload == declared_size,
        "OPS payload size changed: expected " .. declared_size
        .. ", got " .. #payload)
    for _, identifier in ipairs(required_palette_identifiers) do
        assert(payload:find(identifier, 1, true),
            "OPS palette is missing identifier: " .. identifier)
    end
    return #raw, declared_size, #required_palette_identifiers
end

local function load_stamp(stamp)
    configure_empty_simulation()
    local loaded, load_error = sim.loadStamp(stamp, 0, 0, false, 0, 1)
    assert(loaded == 1,
        "client failed to load stamp " .. tostring(stamp)
        .. ": " .. tostring(load_error))
    return verify_fixture()
end

local function run_phase()
    if phase == 1 then
        create_fixture()
        local assertions, particle_count = verify_fixture()
        local stamp1 = save_entire_simulation()
        local file_size, payload_size, palette_identifiers =
            inspect_ops_file(stamp1)
        write_state(stamp1, nil)
        return {
            stamp = stamp1,
            assertions = assertions,
            particle_count = particle_count,
            file_size = file_size,
            payload_size = payload_size,
            palette_identifiers = palette_identifiers,
            operation = "create-save-exit",
        }
    end

    local state = read_state()
    assert(state.stamp1 and state.stamp1:match("^[0-9A-Fa-f]+$")
        and #state.stamp1 == 10, "state does not contain stamp1")

    if phase == 2 then
        local assertions, particle_count = load_stamp(state.stamp1)
        local stamp2 = save_entire_simulation()
        assert(stamp2 ~= state.stamp1,
            "second save unexpectedly reused the first stamp ID")
        local file_size, payload_size, palette_identifiers =
            inspect_ops_file(stamp2)
        write_state(state.stamp1, stamp2)
        return {
            stamp = stamp2,
            source_stamp = state.stamp1,
            assertions = assertions,
            particle_count = particle_count,
            file_size = file_size,
            payload_size = payload_size,
            palette_identifiers = palette_identifiers,
            operation = "restart-load-resave-exit",
        }
    end

    assert(state.stamp2 and state.stamp2:match("^[0-9A-Fa-f]+$")
        and #state.stamp2 == 10, "state does not contain stamp2")
    local assertions, particle_count = load_stamp(state.stamp2)
    return {
        stamp = state.stamp2,
        source_stamp = state.stamp2,
        assertions = assertions,
        particle_count = particle_count,
        operation = "restart-reload-verify-exit",
    }
end

local ok, data = xpcall(run_phase, debug.traceback)
local report = assert(io.open(RESULT, "wb"))
if ok then
    report:write("OMNI_OPS_STATUS=PASS\n")
    report:write("OMNI_OPS_PHASE=" .. phase .. "\n")
    report:write("OMNI_OPS_OPERATION=" .. data.operation .. "\n")
    report:write("OMNI_OPS_STAMP=" .. data.stamp .. "\n")
    if data.source_stamp then
        report:write("OMNI_OPS_SOURCE_STAMP=" .. data.source_stamp .. "\n")
    end
    report:write("OMNI_OPS_PARTICLES=" .. data.particle_count .. "\n")
    report:write("OMNI_OPS_FIELD_ASSERTIONS=" .. data.assertions .. "\n")
    if data.file_size then
        report:write("OMNI_OPS_FILE_SIZE=" .. data.file_size .. "\n")
        report:write("OMNI_OPS_PAYLOAD_SIZE=" .. data.payload_size .. "\n")
        report:write(
            "OMNI_OPS_PALETTE_IDENTIFIERS=" .. data.palette_identifiers .. "\n")
    end
    report:write("OMNI_OPS_ALUM_ID=" .. ids.alum .. "\n")
    report:write("OMNI_OPS_MSCR_ID=" .. ids.mscr .. "\n")
    report:write("OMNI_OPS_NUTR_ID=" .. ids.nutr .. "\n")
    report:write("OMNI_OPS_NFUL_ID=" .. ids.nful .. "\n")
    report:write("OMNI_OPS_CHLR_ID=" .. ids.chlr .. "\n")
else
    local error_text = tostring(data):gsub("[\r\n]+", " | ")
    report:write("OMNI_OPS_STATUS=FAIL\n")
    report:write("OMNI_OPS_PHASE=" .. phase .. "\n")
    report:write("OMNI_OPS_ERROR=" .. error_text .. "\n")
end
report:close()

os.exit(ok and 0 or 1)
