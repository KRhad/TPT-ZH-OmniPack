local CONFIG_FILE = "legacy-ledger.config"

local function read_config()
    local file = assert(io.open(CONFIG_FILE, "rb"), "cannot open " .. CONFIG_FILE)
    local text = assert(file:read("*a"), "cannot read " .. CONFIG_FILE)
    file:close()
    local values = {}
    for line in text:gmatch("[^\r\n]+") do
        local key, value = line:match("^([A-Za-z0-9_]+)=(.*)$")
        assert(key, "invalid Legacy ledger config line: " .. line)
        assert(values[key] == nil, "duplicate Legacy ledger config key: " .. key)
        values[key] = value
    end
    return values
end

local config = read_config()

local function required_integer(name, minimum, maximum)
    local text = assert(config[name], "missing Legacy ledger config key: " .. name)
    assert(text:match("^%d+$"), "Legacy ledger config key is not an integer: " .. name)
    local value = assert(tonumber(text), "cannot parse Legacy ledger config key: " .. name)
    assert(value >= minimum and value <= maximum,
        name .. " must be in [" .. minimum .. ", " .. maximum .. "]")
    return value
end

local schema_version = required_integer("schema_version", 1, 1)
local scenario_name = assert(config.scenario, "missing Legacy ledger scenario")
assert(scenario_name == "empty" or scenario_name == "mixed-medium",
    "unknown Legacy ledger scenario: " .. scenario_name)
local total_steps = required_integer("total_steps", 1, 1000000)
local sample_interval = required_integer("sample_interval", 1, total_steps)
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
assert(type(elements.property) == "function", "elements.property is unavailable")
assert(type(bit) == "table" and type(bit.band) == "function", "bit.band is unavailable")

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
    -- Match the fixed-step and first-divergence pristine Legacy cache contract.
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

local ledger_fields = {
    "step", "state_hash_fnv1a32", "particles", "atmosphere_cells",
    "rng_a", "rng_b", "rng_c", "rng_d",
    "property_powder_records", "property_liquid_records", "property_solid_records",
    "property_gas_records", "property_energy_records", "property_unclassified_records",
    "particle_nonfinite_x", "particle_nonfinite_y", "particle_nonfinite_vx",
    "particle_nonfinite_vy", "particle_nonfinite_temp",
    "air_nonfinite_pressure", "air_nonfinite_velocity_x", "air_nonfinite_velocity_y",
    "air_nonfinite_ambient_heat", "particle_position_out_of_bounds",
    "particle_temp_below_min", "particle_temp_above_max", "air_pressure_below_min",
    "air_pressure_above_max", "air_velocity_x_below_min", "air_velocity_x_above_max",
    "air_velocity_y_below_min", "air_velocity_y_above_max",
    "air_ambient_heat_below_min", "air_ambient_heat_above_max",
    "particle_temp_at_min", "particle_temp_at_max", "air_pressure_at_min",
    "air_pressure_at_max", "air_velocity_x_at_min", "air_velocity_x_at_max",
    "air_velocity_y_at_min", "air_velocity_y_at_max", "air_ambient_heat_at_min",
    "air_ambient_heat_at_max", "particle_sum_x", "particle_sum_y",
    "particle_sum_velocity_x", "particle_sum_velocity_y", "particle_sum_speed_squared",
    "particle_sum_temperature", "air_sum_pressure", "air_sum_velocity_x",
    "air_sum_velocity_y", "air_sum_speed_squared", "air_sum_ambient_heat",
    "particle_min_temperature", "particle_max_temperature",
    "particle_max_abs_velocity_x", "particle_max_abs_velocity_y", "air_min_pressure",
    "air_max_pressure", "air_min_ambient_heat", "air_max_ambient_heat",
    "air_max_abs_velocity_x", "air_max_abs_velocity_y",
}

local count_fields = {
    "property_powder_records", "property_liquid_records", "property_solid_records",
    "property_gas_records", "property_energy_records", "property_unclassified_records",
    "particle_nonfinite_x", "particle_nonfinite_y", "particle_nonfinite_vx",
    "particle_nonfinite_vy", "particle_nonfinite_temp", "air_nonfinite_pressure",
    "air_nonfinite_velocity_x", "air_nonfinite_velocity_y", "air_nonfinite_ambient_heat",
    "particle_position_out_of_bounds", "particle_temp_below_min",
    "particle_temp_above_max", "air_pressure_below_min", "air_pressure_above_max",
    "air_velocity_x_below_min", "air_velocity_x_above_max",
    "air_velocity_y_below_min", "air_velocity_y_above_max",
    "air_ambient_heat_below_min", "air_ambient_heat_above_max", "particle_temp_at_min",
    "particle_temp_at_max", "air_pressure_at_min", "air_pressure_at_max",
    "air_velocity_x_at_min", "air_velocity_x_at_max", "air_velocity_y_at_min",
    "air_velocity_y_at_max", "air_ambient_heat_at_min", "air_ambient_heat_at_max",
}

local nonfinite_fields = {
    "particle_nonfinite_x", "particle_nonfinite_y", "particle_nonfinite_vx",
    "particle_nonfinite_vy", "particle_nonfinite_temp", "air_nonfinite_pressure",
    "air_nonfinite_velocity_x", "air_nonfinite_velocity_y", "air_nonfinite_ambient_heat",
}

local range_fields = {
    "particle_position_out_of_bounds", "particle_temp_below_min",
    "particle_temp_above_max", "air_pressure_below_min", "air_pressure_above_max",
    "air_velocity_x_below_min", "air_velocity_x_above_max",
    "air_velocity_y_below_min", "air_velocity_y_above_max",
    "air_ambient_heat_below_min", "air_ambient_heat_above_max",
}

local bound_fields = {
    "particle_temp_at_min", "particle_temp_at_max", "air_pressure_at_min",
    "air_pressure_at_max", "air_velocity_x_at_min", "air_velocity_x_at_max",
    "air_velocity_y_at_min", "air_velocity_y_at_max", "air_ambient_heat_at_min",
    "air_ambient_heat_at_max",
}

local function is_finite(value)
    return value == value and value ~= math.huge and value ~= -math.huge
end

local function new_accumulator()
    return { sum = 0.0, correction = 0.0 }
end

local function compensated_add(accumulator, value)
    local adjusted = value - accumulator.correction
    local total = accumulator.sum + adjusted
    accumulator.correction = (total - accumulator.sum) - adjusted
    accumulator.sum = total
end

local function integer_text(value)
    return string.format("%.0f", value)
end

local function float_text(value)
    assert(is_finite(value), "attempted to serialize a non-finite Legacy ledger aggregate")
    return string.format("%.17g", value)
end

local function build_identifier_map()
    local identifiers = { [0] = "DEFAULT_PT_NONE" }
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

local identifiers = build_identifier_map()
local properties_cache = {}

local function element_properties(type_id)
    local properties = properties_cache[type_id]
    if properties == nil then
        properties = elements.property(type_id, "Properties")
        properties_cache[type_id] = properties
    end
    return properties
end

local function initialize_metrics(step)
    local metrics = { step = step }
    for _, field in ipairs(count_fields) do
        metrics[field] = 0
    end
    metrics.particle_min_temperature = nil
    metrics.particle_max_temperature = nil
    metrics.particle_max_abs_velocity_x = 0.0
    metrics.particle_max_abs_velocity_y = 0.0
    metrics.air_min_pressure = nil
    metrics.air_max_pressure = nil
    metrics.air_min_ambient_heat = nil
    metrics.air_max_ambient_heat = nil
    metrics.air_max_abs_velocity_x = 0.0
    metrics.air_max_abs_velocity_y = 0.0
    return metrics
end

local function update_min_max(metrics, minimum_field, maximum_field, value)
    if metrics[minimum_field] == nil or value < metrics[minimum_field] then
        metrics[minimum_field] = value
    end
    if metrics[maximum_field] == nil or value > metrics[maximum_field] then
        metrics[maximum_field] = value
    end
end

local function sample_state(step, ledger_file, type_file)
    local metrics = initialize_metrics(step)
    local particle_sums = {
        x = new_accumulator(),
        y = new_accumulator(),
        vx = new_accumulator(),
        vy = new_accumulator(),
        speed2 = new_accumulator(),
        temp = new_accumulator(),
    }
    local air_sums = {
        pressure = new_accumulator(),
        vx = new_accumulator(),
        vy = new_accumulator(),
        speed2 = new_accumulator(),
        heat = new_accumulator(),
    }
    local type_counts = {}
    local particle_records = 0

    for id in sim.parts() do
        local type_id = sim.partProperty(id, "type")
        local x = sim.partProperty(id, "x")
        local y = sim.partProperty(id, "y")
        local vx = sim.partProperty(id, "vx")
        local vy = sim.partProperty(id, "vy")
        local temp = sim.partProperty(id, "temp")
        particle_records = particle_records + 1
        type_counts[type_id] = (type_counts[type_id] or 0) + 1

        local properties = element_properties(type_id)
        local classified = false
        if bit.band(properties, elements.TYPE_PART) ~= 0 then
            metrics.property_powder_records = metrics.property_powder_records + 1
            classified = true
        end
        if bit.band(properties, elements.TYPE_LIQUID) ~= 0 then
            metrics.property_liquid_records = metrics.property_liquid_records + 1
            classified = true
        end
        if bit.band(properties, elements.TYPE_SOLID) ~= 0 then
            metrics.property_solid_records = metrics.property_solid_records + 1
            classified = true
        end
        if bit.band(properties, elements.TYPE_GAS) ~= 0 then
            metrics.property_gas_records = metrics.property_gas_records + 1
            classified = true
        end
        if bit.band(properties, elements.TYPE_ENERGY) ~= 0 then
            metrics.property_energy_records = metrics.property_energy_records + 1
            classified = true
        end
        if not classified then
            metrics.property_unclassified_records =
                metrics.property_unclassified_records + 1
        end

        if is_finite(x) then
            compensated_add(particle_sums.x, x)
        else
            metrics.particle_nonfinite_x = metrics.particle_nonfinite_x + 1
        end
        if is_finite(y) then
            compensated_add(particle_sums.y, y)
        else
            metrics.particle_nonfinite_y = metrics.particle_nonfinite_y + 1
        end
        if is_finite(x) and is_finite(y)
                and (x < 0 or x >= sim.XRES or y < 0 or y >= sim.YRES) then
            metrics.particle_position_out_of_bounds =
                metrics.particle_position_out_of_bounds + 1
        end

        if is_finite(vx) then
            compensated_add(particle_sums.vx, vx)
            metrics.particle_max_abs_velocity_x =
                math.max(metrics.particle_max_abs_velocity_x, math.abs(vx))
        else
            metrics.particle_nonfinite_vx = metrics.particle_nonfinite_vx + 1
        end
        if is_finite(vy) then
            compensated_add(particle_sums.vy, vy)
            metrics.particle_max_abs_velocity_y =
                math.max(metrics.particle_max_abs_velocity_y, math.abs(vy))
        else
            metrics.particle_nonfinite_vy = metrics.particle_nonfinite_vy + 1
        end
        if is_finite(vx) and is_finite(vy) then
            compensated_add(particle_sums.speed2, vx * vx + vy * vy)
        end

        if is_finite(temp) then
            compensated_add(particle_sums.temp, temp)
            update_min_max(metrics, "particle_min_temperature", "particle_max_temperature", temp)
            if temp < sim.MIN_TEMP then
                metrics.particle_temp_below_min = metrics.particle_temp_below_min + 1
            elseif temp > sim.MAX_TEMP then
                metrics.particle_temp_above_max = metrics.particle_temp_above_max + 1
            end
            if temp == sim.MIN_TEMP then
                metrics.particle_temp_at_min = metrics.particle_temp_at_min + 1
            elseif temp == sim.MAX_TEMP then
                metrics.particle_temp_at_max = metrics.particle_temp_at_max + 1
            end
        else
            metrics.particle_nonfinite_temp = metrics.particle_nonfinite_temp + 1
        end
    end

    assert(particle_records == sim.partCount(), "Legacy ledger particle count mismatch")

    for cy = 0, sim.YCELLS - 1 do
        for cx = 0, sim.XCELLS - 1 do
            local pressure = sim.pressure(cx, cy)
            local vx = sim.velocityX(cx, cy)
            local vy = sim.velocityY(cx, cy)
            local heat = sim.ambientHeat(cx, cy)

            if is_finite(pressure) then
                compensated_add(air_sums.pressure, pressure)
                update_min_max(metrics, "air_min_pressure", "air_max_pressure", pressure)
                if pressure < sim.MIN_PRESSURE then
                    metrics.air_pressure_below_min = metrics.air_pressure_below_min + 1
                elseif pressure > sim.MAX_PRESSURE then
                    metrics.air_pressure_above_max = metrics.air_pressure_above_max + 1
                end
                if pressure == sim.MIN_PRESSURE then
                    metrics.air_pressure_at_min = metrics.air_pressure_at_min + 1
                elseif pressure == sim.MAX_PRESSURE then
                    metrics.air_pressure_at_max = metrics.air_pressure_at_max + 1
                end
            else
                metrics.air_nonfinite_pressure = metrics.air_nonfinite_pressure + 1
            end

            if is_finite(vx) then
                compensated_add(air_sums.vx, vx)
                metrics.air_max_abs_velocity_x =
                    math.max(metrics.air_max_abs_velocity_x, math.abs(vx))
                if vx < sim.MIN_PRESSURE then
                    metrics.air_velocity_x_below_min = metrics.air_velocity_x_below_min + 1
                elseif vx > sim.MAX_PRESSURE then
                    metrics.air_velocity_x_above_max = metrics.air_velocity_x_above_max + 1
                end
                if vx == sim.MIN_PRESSURE then
                    metrics.air_velocity_x_at_min = metrics.air_velocity_x_at_min + 1
                elseif vx == sim.MAX_PRESSURE then
                    metrics.air_velocity_x_at_max = metrics.air_velocity_x_at_max + 1
                end
            else
                metrics.air_nonfinite_velocity_x = metrics.air_nonfinite_velocity_x + 1
            end

            if is_finite(vy) then
                compensated_add(air_sums.vy, vy)
                metrics.air_max_abs_velocity_y =
                    math.max(metrics.air_max_abs_velocity_y, math.abs(vy))
                if vy < sim.MIN_PRESSURE then
                    metrics.air_velocity_y_below_min = metrics.air_velocity_y_below_min + 1
                elseif vy > sim.MAX_PRESSURE then
                    metrics.air_velocity_y_above_max = metrics.air_velocity_y_above_max + 1
                end
                if vy == sim.MIN_PRESSURE then
                    metrics.air_velocity_y_at_min = metrics.air_velocity_y_at_min + 1
                elseif vy == sim.MAX_PRESSURE then
                    metrics.air_velocity_y_at_max = metrics.air_velocity_y_at_max + 1
                end
            else
                metrics.air_nonfinite_velocity_y = metrics.air_nonfinite_velocity_y + 1
            end

            if is_finite(vx) and is_finite(vy) then
                compensated_add(air_sums.speed2, vx * vx + vy * vy)
            end

            if is_finite(heat) then
                compensated_add(air_sums.heat, heat)
                update_min_max(metrics, "air_min_ambient_heat", "air_max_ambient_heat", heat)
                if heat < sim.MIN_TEMP then
                    metrics.air_ambient_heat_below_min = metrics.air_ambient_heat_below_min + 1
                elseif heat > sim.MAX_TEMP then
                    metrics.air_ambient_heat_above_max = metrics.air_ambient_heat_above_max + 1
                end
                if heat == sim.MIN_TEMP then
                    metrics.air_ambient_heat_at_min = metrics.air_ambient_heat_at_min + 1
                elseif heat == sim.MAX_TEMP then
                    metrics.air_ambient_heat_at_max = metrics.air_ambient_heat_at_max + 1
                end
            else
                metrics.air_nonfinite_ambient_heat = metrics.air_nonfinite_ambient_heat + 1
            end
        end
    end

    local rng_a, rng_b, rng_c, rng_d = sim.randomSeed()
    metrics.state_hash_fnv1a32 = sim.hash()
    metrics.particles = particle_records
    metrics.atmosphere_cells = sim.NCELL
    metrics.rng_a = rng_a
    metrics.rng_b = rng_b
    metrics.rng_c = rng_c
    metrics.rng_d = rng_d
    metrics.particle_sum_x = particle_sums.x.sum
    metrics.particle_sum_y = particle_sums.y.sum
    metrics.particle_sum_velocity_x = particle_sums.vx.sum
    metrics.particle_sum_velocity_y = particle_sums.vy.sum
    metrics.particle_sum_speed_squared = particle_sums.speed2.sum
    metrics.particle_sum_temperature = particle_sums.temp.sum
    metrics.air_sum_pressure = air_sums.pressure.sum
    metrics.air_sum_velocity_x = air_sums.vx.sum
    metrics.air_sum_velocity_y = air_sums.vy.sum
    metrics.air_sum_speed_squared = air_sums.speed2.sum
    metrics.air_sum_ambient_heat = air_sums.heat.sum
    metrics.particle_min_temperature = metrics.particle_min_temperature or 0.0
    metrics.particle_max_temperature = metrics.particle_max_temperature or 0.0
    metrics.air_min_pressure = metrics.air_min_pressure or 0.0
    metrics.air_max_pressure = metrics.air_max_pressure or 0.0
    metrics.air_min_ambient_heat = metrics.air_min_ambient_heat or 0.0
    metrics.air_max_ambient_heat = metrics.air_max_ambient_heat or 0.0

    local values = {}
    for _, field in ipairs(ledger_fields) do
        local value = assert(metrics[field] ~= nil and metrics[field],
            "missing Legacy ledger metric: " .. field)
        if field == "step" or field == "state_hash_fnv1a32" or field == "particles"
                or field == "atmosphere_cells"
                or field:match("^rng_") or field:match("_records$")
                or field:match("^particle_nonfinite_") or field:match("^air_nonfinite_")
                or field:match("_out_of_bounds$") or field:match("_below_min$")
                or field:match("_above_max$") or field:match("_at_min$")
                or field:match("_at_max$") then
            values[#values + 1] = integer_text(value)
        else
            values[#values + 1] = float_text(value)
        end
    end
    ledger_file:write(table.concat(values, ",") .. "\n")

    type_file:write(step .. ",0,DEFAULT_PT_NONE,0\n")
    local active_types = {}
    for type_id in pairs(type_counts) do
        active_types[#active_types + 1] = type_id
    end
    table.sort(active_types)
    for _, type_id in ipairs(active_types) do
        local identifier = identifiers[type_id] or ("UNKNOWN_TYPE_" .. type_id)
        type_file:write(step .. "," .. integer_text(type_id) .. "," .. identifier .. ","
            .. integer_text(type_counts[type_id]) .. "\n")
    end

    local totals = { nonfinite = 0, range = 0, bound = 0 }
    for _, field in ipairs(nonfinite_fields) do
        totals.nonfinite = totals.nonfinite + metrics[field]
    end
    for _, field in ipairs(range_fields) do
        totals.range = totals.range + metrics[field]
    end
    for _, field in ipairs(bound_fields) do
        totals.bound = totals.bound + metrics[field]
    end
    return metrics, totals
end

local function should_sample(step)
    return step == 0 or step == 1 or step == total_steps or step % sample_interval == 0
end

local function write_result(status, values)
    local result = assert(io.open("legacy-ledger.result", "wb"))
    result:write("OMNI_LEGACY_LEDGER_STATUS=" .. status .. "\n")
    for _, key in ipairs(values.order) do
        result:write(key .. "=" .. tostring(values[key]) .. "\n")
    end
    result:close()
end

local function run()
    tpt.fpsCap(2)
    tpt.drawCap(1)
    reset_world()
    local created = generators[scenario_name]()
    assert(created == sim.partCount(), "generated particle count mismatch")

    local ledger_file = assert(io.open("ledger.csv", "wb"))
    local type_file = assert(io.open("type-counts.csv", "wb"))
    ledger_file:write(table.concat(ledger_fields, ",") .. "\n")
    type_file:write("step,type,type_identifier,count\n")

    local samples = 0
    local nonfinite_observations = 0
    local range_violation_observations = 0
    local bound_occupancy_observations = 0
    local initial_hash
    local initial_particles
    local final_hash
    local final_particles

    local function collect(step)
        local metrics, totals = sample_state(step, ledger_file, type_file)
        samples = samples + 1
        nonfinite_observations = nonfinite_observations + totals.nonfinite
        range_violation_observations = range_violation_observations + totals.range
        bound_occupancy_observations = bound_occupancy_observations + totals.bound
        if step == 0 then
            initial_hash = metrics.state_hash_fnv1a32
            initial_particles = metrics.particles
        end
        if step == total_steps then
            final_hash = metrics.state_hash_fnv1a32
            final_particles = metrics.particles
        end
    end

    collect(0)
    for step = 1, total_steps do
        sim.updateUpTo()
        if should_sample(step) then
            collect(step)
        end
    end
    ledger_file:close()
    type_file:close()

    write_result("PASS", {
        order = {
            "schema_version", "scenario", "seed", "total_steps", "sample_interval",
            "samples", "generated_particles", "initial_particles", "final_particles",
            "initial_state_hash", "final_state_hash", "atmosphere_cells",
            "nonfinite_observations", "range_violation_observations",
            "bound_occupancy_observations", "ledger_file", "type_counts_file",
            "proxy_contract", "physical_mass_conservation_evaluated",
            "physical_energy_conservation_evaluated",
            "physical_momentum_conservation_evaluated",
            "source_sink_attribution_evaluated", "correction_events_evaluated",
        },
        schema_version = schema_version,
        scenario = scenario_name,
        seed = table.concat(seed, ","),
        total_steps = total_steps,
        sample_interval = sample_interval,
        samples = samples,
        generated_particles = created,
        initial_particles = initial_particles,
        final_particles = final_particles,
        initial_state_hash = initial_hash,
        final_state_hash = final_hash,
        atmosphere_cells = sim.NCELL,
        nonfinite_observations = nonfinite_observations,
        range_violation_observations = range_violation_observations,
        bound_occupancy_observations = bound_occupancy_observations,
        ledger_file = "ledger.csv",
        type_counts_file = "type-counts.csv",
        proxy_contract = "legacy_state_proxies_not_physical_units",
        physical_mass_conservation_evaluated = "false",
        physical_energy_conservation_evaluated = "false",
        physical_momentum_conservation_evaluated = "false",
        source_sink_attribution_evaluated = "false",
        correction_events_evaluated = "false",
    })
end

local ok, error_text = xpcall(run, debug.traceback)
if not ok then
    write_result("FAIL", {
        order = { "schema_version", "scenario", "error" },
        schema_version = schema_version,
        scenario = scenario_name,
        error = tostring(error_text):gsub("[\r\n]+", " | "),
    })
    os.exit(1)
end
os.exit(0)
