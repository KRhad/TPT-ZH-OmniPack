local RESULT = "lua-nuclear-regression.result"

local function must_element(identifier, short_name)
    local id = elements[identifier]
    assert(type(id) == "number", "missing element constant: " .. identifier)
    assert(elements.getByName(short_name) == id,
        "name/identifier mismatch for " .. identifier)
    return id
end

local ids = {
    fuel = must_element("OMNI_PT_NFUL", "NFUL"),
    moderator = must_element("OMNI_PT_MODR", "MODR"),
    control = must_element("OMNI_PT_CROD", "CROD"),
    coolant = must_element("OMNI_PT_NCLT", "NCLT"),
    waste = must_element("OMNI_PT_NWST", "NWST"),
    generator = must_element("OMNI_PT_NGEN", "NGEN"),
    shield = must_element("OMNI_PT_RSHD", "RSHD"),
    neutron = assert(elements.DEFAULT_PT_NEUT),
    spark = assert(elements.DEFAULT_PT_SPRK),
    steam = assert(elements.DEFAULT_PT_WTRV),
}

local function configure_simulation()
    sim.clearSim()
    sim.paused(true)
    sim.gravityMode(sim.GRAV_OFF)
    sim.airMode(sim.AIR_OFF)
    sim.ambientHeatSim(false)
    sim.heatSim(true)
    sim.ensureDeterminism(true)
    sim.randomSeed(31, 32, 33, 34)
end

local function make(type, x, y, temp)
    local particle = sim.partCreate(-1, x, y, type)
    assert(particle >= 0, "failed to create particle type " .. tostring(type))
    if temp then
        sim.partProperty(particle, "temp", temp)
    end
    return particle
end

local function step(frames)
    for _ = 1, frames or 1 do
        sim.updateUpTo()
    end
end

local function count_local_type(x, y, type)
    local count = 0
    for dy = -1, 1 do
        for dx = -1, 1 do
            local particle = sim.partID(x + dx, y + dy)
            if particle and particle >= 0
                and sim.partProperty(particle, "type") == type then
                count = count + 1
            end
        end
    end
    return count
end

local function spark_generator(x, y, temp)
    local generator = make(ids.generator, x, y, temp)
    sim.partProperty(generator, "type", ids.spark)
    sim.partProperty(generator, "ctype", ids.generator)
    sim.partProperty(generator, "life", 4)
    return generator
end

local function run_controlled_fission()
    configure_simulation()
    sim.heatSim(false)
    spark_generator(120, 120, 400.0)
    local fuel = make(ids.fuel, 121, 120, 400.0)
    make(ids.moderator, 120, 121, 400.0)
    step(2)
    assert(sim.partProperty(fuel, "type") == ids.waste,
        "sparked generator and moderator did not convert fuel to waste")
    assert(sim.partProperty(fuel, "temp") >= 1600.0,
        "controlled fission did not create the documented waste heat: "
            .. tostring(sim.partProperty(fuel, "temp")))

    configure_simulation()
    spark_generator(120, 120, 400.0)
    local controlled_fuel = make(ids.fuel, 121, 120, 400.0)
    make(ids.moderator, 120, 121, 400.0)
    make(ids.control, 121, 121, 400.0)
    local diffusion = elements.property(ids.neutron, "Diffusion")
    elements.property(ids.neutron, "Diffusion", 0.0)
    step(4)
    elements.property(ids.neutron, "Diffusion", diffusion)
    assert(sim.partProperty(controlled_fuel, "type") == ids.fuel,
        "control rod did not suppress adjacent custom fuel conversion")
    assert(count_local_type(120, 120, ids.neutron) == 1,
        "one sparked generator emitted more than one neutron")

    configure_simulation()
    spark_generator(120, 120, 400.0)
    step(2)
    assert(count_local_type(120, 120, ids.neutron) == 0,
        "generator without local fuel emitted a neutron")
end

local function run_cooling_and_shielding()
    configure_simulation()
    local coolant = make(ids.coolant, 120, 120, 900.0)
    local waste = make(ids.waste, 121, 120, 1200.0)
    step()
    assert(sim.partProperty(coolant, "type") == ids.steam,
        "coolant beside hot waste did not become steam")
    assert(sim.partProperty(waste, "temp") == 900.0,
        "coolant did not reduce waste heat to the bounded target")

    configure_simulation()
    local shield = make(ids.shield, 120, 120, 300.0)
    local neutron = make(ids.neutron, 121, 120, 300.0)
    sim.partProperty(neutron, "vx", 0.0)
    sim.partProperty(neutron, "vy", 0.0)
    step()
    assert(not sim.partExists(neutron),
        "radiation shield did not absorb its adjacent neutron")
    assert(sim.partProperty(shield, "temp") > 300.0,
        "radiation shield did not convert absorbed neutron energy into heat")
end

local function test()
    run_controlled_fission()
    run_cooling_and_shielding()
end

local ok, data = xpcall(test, debug.traceback)
local report = assert(io.open(RESULT, "w"))
if ok then
    report:write("OMNI_NUCLEAR_STATUS=PASS\n")
    report:write("OMNI_NUCLEAR_PATHS=4\n")
    report:write("OMNI_NUCLEAR_IDS=" .. ids.fuel .. "-" .. ids.shield .. "\n")
else
    local error_text = tostring(data):gsub("[\r\n]+", " | ")
    report:write("OMNI_NUCLEAR_STATUS=FAIL\n")
    report:write("OMNI_NUCLEAR_ERROR=" .. error_text .. "\n")
end
report:close()
