local MODE_FILE = "automation.mode"
local CASE_FILE = "automation.case"
local STAMP_FILE = "automation.stamp"
local CHALLENGE_FILE = "automation.challenge"
local RESULT_FILE = "automation.result"

local function read_line(path, optional)
    local file = io.open(path, "rb")
    if not file then
        if optional then return "" end
        error("cannot open " .. path)
    end
    local value = file:read("*l")
    file:close()
    return (value or ""):match("^%s*(.-)%s*$")
end

local mode = read_line(MODE_FILE)
local case_id = read_line(CASE_FILE)
local challenge_id = read_line(CHALLENGE_FILE, true)
assert(mode == "generate" or mode == "verify",
    "invalid automation mode: " .. tostring(mode))

local ids = {
    none = assert(elements.DEFAULT_PT_NONE),
    acid = assert(elements.DEFAULT_PT_ACID),
    bray = assert(elements.DEFAULT_PT_BRAY),
    brck = assert(elements.DEFAULT_PT_BRCK),
    dmnd = assert(elements.DEFAULT_PT_DMND),
    dust = assert(elements.DEFAULT_PT_DUST),
    filt = assert(elements.DEFAULT_PT_FILT),
    inst = assert(elements.DEFAULT_PT_INST),
    lcry = assert(elements.DEFAULT_PT_LCRY),
    lava = assert(elements.DEFAULT_PT_LAVA),
    metl = assert(elements.DEFAULT_PT_METL),
    nscn = assert(elements.DEFAULT_PT_NSCN),
    oil = assert(elements.DEFAULT_PT_OIL),
    pipe = assert(elements.DEFAULT_PT_PIPE),
    ppip = assert(elements.DEFAULT_PT_PPIP),
    pscn = assert(elements.DEFAULT_PT_PSCN),
    sprk = assert(elements.DEFAULT_PT_SPRK),
    watr = assert(elements.DEFAULT_PT_WATR),
    wtrv = assert(elements.DEFAULT_PT_WTRV),
    wifi = assert(elements.DEFAULT_PT_WIFI),
    swch = assert(elements.DEFAULT_PT_SWCH),
    tsns = assert(elements.DEFAULT_PT_TSNS),
    psns = assert(elements.DEFAULT_PT_PSNS),
    dtec = assert(elements.DEFAULT_PT_DTEC),
    ldtc = assert(elements.DEFAULT_PT_LDTC),
    vsns = assert(elements.DEFAULT_PT_VSNS),
    lsns = assert(elements.DEFAULT_PT_LSNS),
    dlay = assert(elements.DEFAULT_PT_DLAY),
    stor = assert(elements.DEFAULT_PT_STOR),
    cray = assert(elements.DEFAULT_PT_CRAY),
    pstn = assert(elements.DEFAULT_PT_PSTN),

    copr = assert(elements.OMNI_PT_COPR),
    tin = assert(elements.OMNI_PT_TIN),
    brnz = assert(elements.OMNI_PT_BRNZ),
    cruc = assert(elements.OMNI_PT_CRUC),
    ncrm = assert(elements.OMNI_PT_NCRM),
    slag = assert(elements.OMNI_PT_SLAG),
    flux = assert(elements.OMNI_PT_FLUX),
    cata = assert(elements.OMNI_PT_CATA),
    kero = assert(elements.OMNI_PT_KERO),
    gaso = assert(elements.OMNI_PT_GASO),
    hums = assert(elements.OMNI_PT_HUMS),
    fert = assert(elements.OMNI_PT_FERT),
    nutr = assert(elements.OMNI_PT_NUTR),
    path = assert(elements.OMNI_PT_PATH),
    pero = assert(elements.OMNI_PT_PERO),
    nful = assert(elements.OMNI_PT_NFUL),
    nclt = assert(elements.OMNI_PT_NCLT),
    nwst = assert(elements.OMNI_PT_NWST),
    crod = assert(elements.OMNI_PT_CROD),
}

local PPIP_PAUSED = 0x02000000
local assertions = 0
local metrics = {
    frames = 0,
    input_events = 0,
    output_events = 0,
    peak_events_per_frame = 0,
    blocked_events = 0,
    pending_events = 0,
    stop_event_delta = 0,
    recovery_assertions = 0,
    omni_events = 0,
    omni_peak_per_frame = 0,
}

local function ok(condition, message)
    assert(condition, message)
    assertions = assertions + 1
end

local function input_event(count)
    count = count or 1
    metrics.input_events = metrics.input_events + count
    metrics.pending_events = metrics.pending_events + count
end

local function output_event(count)
    count = count or 1
    metrics.output_events = metrics.output_events + count
    metrics.pending_events = metrics.pending_events + count
end

local function blocked_event(count)
    count = count or 1
    metrics.blocked_events = metrics.blocked_events + count
end

local function configure()
    sim.clearSim()
    sim.paused(true)
    sim.gravityMode(sim.GRAV_OFF)
    sim.airMode(sim.AIR_OFF)
    sim.ambientHeatSim(false)
    sim.heatSim(true)
    sim.ensureDeterminism(true)
    sim.randomSeed(41, 42, 43, 44)
    sim.resetOmniEventMetrics()
end

local function make(kind, x, y, properties)
    local id = type(kind) == "number" and kind or assert(ids[kind], kind)
    local particle = assert(sim.partCreate(-1, x, y, id),
        "failed to create " .. tostring(kind) .. " at " .. x .. "," .. y)
    for name, value in pairs(properties or {}) do
        sim.partProperty(particle, name, value)
    end
    return particle
end

local function make_molten(material, x, y, temperature)
    return make("lava", x, y, { ctype = ids[material], temp = temperature })
end

local function particle_at(x, y)
    local particle = sim.partID(x, y)
    if type(particle) ~= "number" or particle < 0 then return nil end
    return particle
end

local function type_at(x, y)
    local particle = particle_at(x, y)
    return particle and sim.partProperty(particle, "type") or ids.none
end

local function count_particles()
    local count = 0
    for _ in sim.parts() do count = count + 1 end
    return count
end

local function count_type(element)
    local count = 0
    for particle in sim.parts() do
        if sim.partProperty(particle, "type") == element then count = count + 1 end
    end
    return count
end

local function count_material(element)
    local count = 0
    for particle in sim.parts() do
        local particle_type = sim.partProperty(particle, "type")
        if particle_type == element
            or (particle_type == ids.lava
                and sim.partProperty(particle, "ctype") == element) then
            count = count + 1
        end
    end
    return count
end

local function raw_step(frames)
    for _ = 1, frames or 1 do
        sim.updateUpTo()
    end
end

local function step(frames)
    for _ = 1, frames or 1 do
        sim.updateUpTo()
        metrics.frames = metrics.frames + 1
        if metrics.pending_events > metrics.peak_events_per_frame then
            metrics.peak_events_per_frame = metrics.pending_events
        end
        metrics.pending_events = 0
    end
end

local function spark_particle(particle, sender)
    assert(particle and particle >= 0, "cannot spark missing particle")
    sim.partProperty(particle, "type", ids.sprk)
    sim.partProperty(particle, "ctype", sender)
    sim.partProperty(particle, "life", 4)
    input_event()
end

local function spark_at(x, y, sender)
    spark_particle(particle_at(x, y), sender)
end

local function wait_until(predicate, frames, message)
    for frame = 1, frames do
        step()
        if predicate() then return frame end
    end
    error(message)
end

local function ppip_paused(particle)
    local tmp = sim.partProperty(particle, "tmp")
    return bit.band(tmp, PPIP_PAUSED) ~= 0
end

local function build_thermostatic_furnace()
    make("pscn", 97, 100)
    make("swch", 98, 100, { life = 10 })
    make("nscn", 99, 100)
    make("tsns", 100, 100, { temp = 500.0, tmp = 0, tmp2 = 2 })
    make("dmnd", 101, 100, { temp = 300.0 })
    make("pscn", 100, 99)
    make("lcry", 100, 98)
    make("pscn", 119, 100)
    make("dlay", 120, 100, { temp = 276.15 })
    make("nscn", 121, 100)
    make("cruc", 104, 100, { temp = 900.0 })
    make("ncrm", 105, 100, { temp = 900.0 })
end

local function run_thermostatic_furnace()
    local switch = assert(particle_at(98, 100))
    local sensor_target = assert(particle_at(101, 100))
    local alarm = assert(particle_at(100, 98))
    step(2)
    ok(type_at(99, 100) == ids.nscn, "cold furnace tripped the temperature sensor")
    sim.partProperty(sensor_target, "temp", 2000.0)
    local trip_frames = wait_until(function()
        return sim.partProperty(switch, "life") < 10
    end, 8, "hot furnace did not open the heater switch")
    output_event(2)
    ok(trip_frames <= 8, "temperature trip exceeded the declared latency")
    ok(sim.partProperty(switch, "life") < 10, "heater switch did not remain off")
    wait_until(function()
        return sim.partProperty(alarm, "tmp2") > 0
    end, 8, "over-temperature alarm did not latch")
    ok(sim.partProperty(alarm, "tmp2") > 0, "alarm remained dark")

    spark_at(119, 100, ids.pscn)
    local delay_frames = wait_until(function()
        return type_at(121, 100) == ids.sprk
    end, 10, "DLAY produced no bounded output pulse")
    output_event()
    ok(delay_frames >= 1 and delay_frames <= 10,
        "DLAY output latency is outside its configured window: " .. delay_frames)

    sim.partProperty(sensor_target, "temp", 300.0)
    spark_at(97, 100, ids.pscn)
    wait_until(function()
        return sim.partProperty(switch, "life") >= 10
    end, 6, "manual reset did not restore the heater switch")
    output_event()
    ok(sim.partProperty(switch, "life") >= 10, "manual reset state was not retained")
end

local function build_automatic_alloy()
    make("pscn", 99, 100)
    make("cray", 100, 100, { ctype = ids.tin, tmp = 1, tmp2 = 0, temp = 2000.0 })
    make_molten("copr", 102, 100, 2000.0)
    make_molten("copr", 101, 101, 2000.0)
    make_molten("copr", 102, 101, 2000.0)
    make("pscn", 119, 100)
    make("dlay", 120, 100, { temp = 277.15 })
    make("nscn", 121, 100)
    make("filt", 129, 100)
    make("lsns", 130, 100, { tmp = 1, tmp2 = 3 })
    make("brck", 132, 100, { life = 7 })
    make("stor", 140, 100, { tmp = ids.tin, temp = 300.0 })
end

local function run_automatic_alloy()
    ok(count_material(ids.copr) == 3, "automatic alloy fixture does not contain three copper inputs")
    ok(count_material(ids.tin) == 0, "automatic alloy fixture was preloaded with tin")
    spark_at(99, 100, ids.pscn)
    local alloy_frames = wait_until(function()
        return count_material(ids.brnz) == 4
    end, 24, "bounded CRAY feed did not produce the bronze batch")
    output_event()
    ok(alloy_frames <= 24, "bronze production exceeded the declared frame bound")
    ok(count_material(ids.brnz) == 4, "bronze batch does not preserve exact 3:1 particle count")
    ok(count_material(ids.tin) == 0, "tin feeder created excess material")

    spark_at(119, 100, ids.pscn)
    local pulse_count = 0
    for _ = 1, 16 do
        step()
        if type_at(121, 100) == ids.sprk then pulse_count = pulse_count + 1 end
    end
    if pulse_count > 0 then output_event(pulse_count) end
    ok(pulse_count > 0 and pulse_count <= 4, "delay counter emitted an invalid pulse width")
    local filter = assert(particle_at(129, 100))
    ok(sim.partProperty(filter, "ctype") == 0x10000007,
        "LSNS did not serialize the bounded life count into FILT")
    ok(sim.partProperty(assert(particle_at(140, 100)), "tmp") == ids.tin,
        "unused reserve feed was not retained in STOR")
end

local function build_fuel_control()
    make("pscn", 99, 100)
    make("cray", 100, 100, { ctype = ids.oil, tmp = 1, temp = 550.0 })
    make("cata", 102, 100, { temp = 550.0 })
    make("ppip", 118, 100, { life = 0 })
    make("nscn", 119, 100)
    make("tsns", 120, 100, { temp = 650.0, tmp = 0, tmp2 = 2 })
    make("dmnd", 121, 100, { temp = 300.0 })
    make("filt", 139, 100)
    make("dtec", 140, 100, { ctype = ids.cata, tmp2 = 4 })
    make("bray", 142, 100, { ctype = 0x12345, life = 20 })
    make("cata", 143, 100, { temp = 550.0 })
end

local function run_fuel_control()
    spark_at(99, 100, ids.pscn)
    local fuel_frames = wait_until(function()
        return count_type(ids.kero) >= 1
    end, 24, "bounded oil feed did not produce kerosene")
    output_event()
    ok(fuel_frames <= 24, "fuel production exceeded the declared frame bound")
    ok(count_type(ids.kero) == 1, "fuel feeder produced an unexpected kerosene count")

    local valve = assert(particle_at(118, 100))
    local hot = assert(particle_at(121, 100))
    ok(not ppip_paused(valve), "fuel valve began in the fail state")
    sim.partProperty(hot, "temp", 2000.0)
    wait_until(function() return ppip_paused(valve) end, 10,
        "over-temperature sensor did not pause the fuel valve")
    output_event()
    ok(ppip_paused(valve), "fuel valve did not retain the paused state")
    ok(sim.partProperty(assert(particle_at(139, 100)), "ctype") == 0x12345,
        "DTEC/FILT catalyst filter did not record the observed process state")
end

local function build_nutrient_dosing()
    make("stor", 97, 100, { tmp = ids.fert, temp = 300.0 })
    make("pscn", 99, 100)
    make("dtec", 100, 100, { ctype = ids.hums, tmp2 = 5 })
    make("hums", 96, 101, { temp = 300.0 })
    make("watr", 96, 102, { temp = 300.0 })
    make("hums", 120, 100, { temp = 300.0 })
    make("fert", 121, 100, { temp = 300.0 })
    make("filt", 130, 100)
    make("lsns", 131, 100, { tmp = 1, tmp2 = 3 })
    make("inst", 134, 100)
    make("swch", 135, 100, { life = 10 })
end

local function run_nutrient_dosing()
    local storage = assert(particle_at(97, 100))
    ok(sim.partProperty(storage, "tmp") == ids.fert, "nutrient storage was not primed")
    wait_until(function()
        return type_at(96, 101) == ids.nutr
    end, 16, "wet biomass interlock did not release and consume one fertilizer dose")
    output_event(2)
    ok(type_at(96, 101) == ids.nutr, "wet humus was not recovered to nutrient")
    ok(sim.partProperty(storage, "tmp") == 0, "nutrient storage did not release exactly one dose")
    ok(count_type(ids.dust) >= 1, "fertilizer dose was not consumed into dust")

    step(3)
    ok(type_at(120, 100) == ids.hums and type_at(121, 100) == ids.fert,
        "dry biomass bypassed the water interlock")
    blocked_event()
end

local function build_pathogen_disinfection()
    make("path", 96, 100, { temp = 300.0 })
    make("cray", 98, 100, { ctype = ids.pero, tmp = 1, temp = 300.0 })
    make("pscn", 99, 100)
    make("dtec", 100, 100, { ctype = ids.path, tmp2 = 5 })
    make("filt", 101, 100)
    make("pero", 120, 100, { temp = 300.0 })
    make("path", 121, 100, { temp = 300.0 })
    make("cata", 121, 101, { temp = 1000.0 })
    make("cray", 140, 100, { ctype = ids.pero, tmp = 1, temp = 300.0 })
    make("pscn", 141, 100)
    make("dtec", 142, 100, { ctype = ids.path, tmp2 = 4 })
end

local function run_pathogen_disinfection()
    wait_until(function()
        return type_at(96, 100) == ids.hums
    end, 16, "pathogen detector did not trigger the peroxide feeder")
    output_event(2)
    ok(type_at(96, 100) == ids.hums, "detected pathogen was not treated")
    ok(type_at(97, 100) == ids.watr, "peroxide dose was not recovered to water")
    ok(count_type(ids.pero) == 1, "targeted feeder created an excess peroxide dose")

    step(4)
    ok(type_at(140, 100) == ids.cray and particle_at(139, 100) == nil,
        "no-target disinfection cell created a dose")
    blocked_event()
    ok(type_at(120, 100) == ids.pero and type_at(121, 100) == ids.path,
        "hot catalyst interlock failed to block peroxide treatment")
    blocked_event()
end

local function build_reactor_cooling()
    make("stor", 97, 100, { tmp = ids.nclt, temp = 900.0 })
    make("pscn", 99, 100)
    make("tsns", 100, 100, { temp = 1000.0, tmp = 0, tmp2 = 5 })
    make("nwst", 96, 102, { temp = 1200.0 })
    make("ppip", 118, 100, { life = 0, ctype = ids.nful })
    make("nscn", 119, 100)
    make("tsns", 120, 100, { temp = 1000.0, tmp = 0, tmp2 = 2 })
    make("dmnd", 121, 100, { temp = 2000.0 })
    make("metl", 139, 100)
    make("wifi", 140, 100, { temp = 373.15 })
    make("wifi", 150, 100, { temp = 373.15 })
    make("pscn", 151, 100)
    make("lcry", 152, 100)
end

local function run_reactor_cooling()
    local waste = assert(particle_at(96, 102))
    wait_until(function()
        return sim.partProperty(waste, "temp") <= 900.1
    end, 20, "reactor temperature trip did not release coolant")
    output_event(2)
    ok(sim.partProperty(waste, "temp") <= 900.1, "reactor waste was not cooled to the bounded target")
    ok(count_type(ids.wtrv) >= 1, "released coolant did not become steam")
    ok(sim.partProperty(assert(particle_at(97, 100)), "tmp") == 0,
        "reactor coolant storage retained its dose after trip")

    local fuel_valve = assert(particle_at(118, 100))
    wait_until(function() return ppip_paused(fuel_valve) end, 12,
        "reactor over-temperature interlock did not close fuel feed")
    output_event()
    ok(ppip_paused(fuel_valve), "reactor fuel feed did not remain closed")

    spark_at(139, 100, ids.metl)
    wait_until(function()
        return sim.partProperty(assert(particle_at(152, 100)), "tmp2") > 0
    end, 16, "wireless reactor alarm did not reach LCRY")
    output_event(2)
    ok(sim.partProperty(assert(particle_at(152, 100)), "tmp2") > 0,
        "reactor alarm remained dark")
end

local function build_emergency_stop()
    make("nscn", 99, 100)
    make("ppip", 100, 100, { life = 0 })
    make("pscn", 101, 100)
    make("nscn", 104, 100)
    make("swch", 105, 100, { life = 10 })
    make("pscn", 119, 100)
    make("dlay", 120, 100, { temp = 278.15 })
    make("nscn", 121, 100)
    make("wifi", 140, 100, { temp = 473.15 })
    make("lcry", 142, 100)
end

local function run_emergency_stop()
    local valve = assert(particle_at(100, 100))
    local switch = assert(particle_at(105, 100))
    ok(not ppip_paused(valve) and sim.partProperty(switch, "life") >= 10,
        "emergency-stop fixture did not begin in the running state")
    spark_at(99, 100, ids.nscn)
    spark_at(104, 100, ids.nscn)
    wait_until(function()
        return ppip_paused(valve) and sim.partProperty(switch, "life") < 10
    end, 10, "emergency stop did not deenergize the series switch and valve")
    output_event(2)
    ok(ppip_paused(valve), "emergency stop did not latch the valve pause")
    ok(sim.partProperty(switch, "life") < 10, "emergency stop did not isolate start input")

    local outputs_before = metrics.output_events
    step(4)
    ok(ppip_paused(valve) and metrics.output_events == outputs_before,
        "early reset window produced a post-stop event")
    blocked_event()

    spark_at(119, 100, ids.pscn)
    wait_until(function() return type_at(121, 100) == ids.sprk end, 12,
        "delayed reset window never opened")
    output_event()
    spark_at(101, 100, ids.pscn)
    wait_until(function() return not ppip_paused(valve) end, 10,
        "manual reset did not reopen the stopped valve")
    output_event()
    ok(not ppip_paused(valve), "delayed manual reset was not retained")
end

local function build_waste_transfer()
    make("pscn", 98, 100)
    make("stor", 100, 100, { ctype = ids.slag })
    make("slag", 101, 100, { temp = 400.0 })
    make("stor", 120, 100, { ctype = ids.slag })
    make("dust", 121, 100)
    make("ppip", 140, 100, { life = 0, tmp = PPIP_PAUSED, ctype = ids.nwst })
    make("filt", 159, 100)
    make("dtec", 160, 100, { ctype = ids.slag, tmp2 = 3 })
    make("bray", 162, 100, { ctype = 0x15555, life = 20 })
    make("pipe", 180, 100, { life = 0 })
    make("lsns", 182, 100, { tmp = 1, tmp2 = 3 })
end

local function run_waste_transfer()
    local known = assert(particle_at(100, 100))
    local unknown = assert(particle_at(120, 100))
    step(2)
    ok(sim.partProperty(known, "tmp") == ids.slag, "registered waste was not captured by STOR")
    ok(sim.partProperty(unknown, "tmp") == 0 and type_at(121, 100) == ids.dust,
        "unknown waste bypassed the STOR type filter")
    blocked_event()

    spark_at(98, 100, ids.pscn)
    wait_until(function() return sim.partProperty(known, "tmp") == 0 end, 8,
        "registered waste did not leave storage")
    output_event()
    ok(count_type(ids.slag) == 1, "registered waste release changed the exact item count")

    local pipe_valve = assert(particle_at(140, 100))
    step(8)
    ok(ppip_paused(pipe_valve) and sim.partProperty(pipe_valve, "ctype") == ids.nwst,
        "paused waste valve lost its retained payload")
    ok(sim.partProperty(assert(particle_at(159, 100)), "ctype") == 0x15555,
        "waste detector did not copy its bounded filter payload")
end

local function build_integrated_factory()
    make_molten("copr", 80, 80, 2000.0)
    make_molten("copr", 81, 80, 2000.0)
    make_molten("copr", 80, 81, 2000.0)
    make_molten("tin", 81, 81, 2000.0)
    make("oil", 110, 80, { temp = 550.0 })
    make("cata", 111, 80, { temp = 550.0 })
    make("hums", 140, 80, { temp = 300.0 })
    make("fert", 141, 80, { temp = 300.0 })
    make("watr", 140, 81, { temp = 300.0 })
    make("nclt", 170, 80, { temp = 900.0 })
    make("nwst", 171, 80, { temp = 1200.0 })
    make("pero", 200, 80, { temp = 300.0 })
    make("path", 201, 80, { temp = 300.0 })
    make("cata", 201, 81, { temp = 1000.0 })

    make("tsns", 240, 80, { temp = 500.0, tmp2 = 4 })
    make("psns", 244, 80, { temp = 277.15 })
    make("dtec", 248, 80, { ctype = ids.nwst, tmp2 = 4 })
    make("filt", 252, 80)
    make("dlay", 256, 80, { temp = 277.15 })
    make("lsns", 260, 80, { tmp = 1, tmp2 = 4 })
    make("stor", 264, 80, { tmp = ids.slag })
    make("ppip", 268, 80, { life = 0 })
    make("cray", 272, 80, { ctype = ids.dust, tmp = 1 })
    make("pipe", 276, 80, { life = 0 })
    make("swch", 280, 80, { life = 10 })
    make("wifi", 284, 80, { temp = 373.15 })
    make("inst", 288, 80)
end

local function run_integrated_factory()
    wait_until(function()
        return count_material(ids.brnz) == 4
            and count_type(ids.kero) >= 1
            and type_at(140, 80) == ids.nutr
            and sim.partProperty(assert(particle_at(171, 80)), "temp") <= 900.1
    end, 24, "integrated four-module cells did not reach their bounded outputs")
    output_event(4)
    ok(count_material(ids.brnz) == 4, "integrated metallurgy cell failed")
    ok(count_type(ids.kero) == 1, "integrated chemistry cell failed")
    ok(type_at(140, 80) == ids.nutr, "integrated biology cell failed")
    ok(sim.partProperty(assert(particle_at(171, 80)), "temp") <= 900.1,
        "integrated nuclear cooling cell failed")
    ok(type_at(200, 80) == ids.pero and type_at(201, 80) == ids.path,
        "hot disinfection fault escaped its isolated cell")
    blocked_event()
    ok(count_particles() < 4096, "integrated factory exceeded the declared particle budget")
end

local builders = {
    ["thermostatic-furnace"] = build_thermostatic_furnace,
    ["automatic-alloy"] = build_automatic_alloy,
    ["fuel-control"] = build_fuel_control,
    ["nutrient-dosing"] = build_nutrient_dosing,
    ["pathogen-disinfection"] = build_pathogen_disinfection,
    ["reactor-cooling"] = build_reactor_cooling,
    ["emergency-stop"] = build_emergency_stop,
    ["waste-transfer"] = build_waste_transfer,
    ["integrated-factory"] = build_integrated_factory,
}

local runners = {
    ["thermostatic-furnace"] = run_thermostatic_furnace,
    ["automatic-alloy"] = run_automatic_alloy,
    ["fuel-control"] = run_fuel_control,
    ["nutrient-dosing"] = run_nutrient_dosing,
    ["pathogen-disinfection"] = run_pathogen_disinfection,
    ["reactor-cooling"] = run_reactor_cooling,
    ["emergency-stop"] = run_emergency_stop,
    ["waste-transfer"] = run_waste_transfer,
    ["integrated-factory"] = run_integrated_factory,
}

local expected_challenge = {
    ["thermostatic-furnace"] = "A01-TEMPERATURE",
    ["automatic-alloy"] = "A02-FEEDING",
    ["waste-transfer"] = "A03-SORTING",
    ["pathogen-disinfection"] = "A04-DISINFECTION",
    ["emergency-stop"] = "A05-EMERGENCY-STOP",
    ["integrated-factory"] = "A06-MULTI-MODULE",
}

local function generate()
    assert(builders[case_id], "unknown automation scenario: " .. tostring(case_id))
    configure()
    builders[case_id]()
    local initial_count = count_particles()
    assert(initial_count > 0 and initial_count <= 4096, "invalid generated particle count")
    local stamp = assert(sim.saveStamp(0, 0, sim.XRES - 1, sim.YRES - 1, 1),
        "saveStamp returned no automation stamp")
    assert(stamp:match("^[0-9A-Fa-f]+$") and #stamp == 10,
        "invalid generated stamp: " .. tostring(stamp))
    return {
        stamp = stamp,
        initial_count = initial_count,
        final_count = initial_count,
        operation = "generate-save",
    }
end

local function finish_runtime_roundtrip()
    local event_metrics = sim.omniEventMetrics()
    metrics.omni_events = assert(tonumber(event_metrics.total), "missing Omni event total")
    metrics.omni_peak_per_frame = assert(tonumber(event_metrics.peak_per_frame), "missing Omni event peak")
    if metrics.omni_peak_per_frame > metrics.peak_events_per_frame then
        metrics.peak_events_per_frame = metrics.omni_peak_per_frame
    end
    local before_save = count_particles()
    local runtime_stamp = assert(sim.saveStamp(0, 0, sim.XRES - 1, sim.YRES - 1, 1),
        "runtime saveStamp failed")
    sim.clearSim()
    raw_step(32)
    local stopped = sim.omniEventMetrics()
    metrics.stop_event_delta = assert(tonumber(stopped.total)) - metrics.omni_events
    ok(count_particles() == 0, "automation stop left particles behind")
    ok(metrics.stop_event_delta == 0, "automation stop produced post-stop Omni events")
    metrics.recovery_assertions = metrics.recovery_assertions + 2
    local loaded, load_error = sim.loadStamp(runtime_stamp, 0, 0, false, 0, 1)
    assert(loaded == 1, "runtime recovery load failed: " .. tostring(load_error))
    ok(count_particles() == before_save, "automation recovery changed particle count")
    metrics.recovery_assertions = metrics.recovery_assertions + 1
    return count_particles()
end

local function verify()
    local stamp = read_line(STAMP_FILE)
    assert(stamp:match("^[0-9A-Fa-f]+$") and #stamp == 10,
        "invalid automation verification stamp")
    configure()
    local loaded, load_error = sim.loadStamp(stamp, 0, 0, false, 0, 1)
    assert(loaded == 1, "could not load automation scenario: " .. tostring(load_error))
    local initial_count = count_particles()
    assert(builders[case_id] and runners[case_id], "unknown automation verification case")
    if expected_challenge[case_id] then
        assert(challenge_id == expected_challenge[case_id],
            "challenge mismatch for " .. case_id .. ": " .. tostring(challenge_id))
    else
        assert(challenge_id == "", "unexpected challenge for scenario " .. case_id)
    end
    runners[case_id]()
    local final_count = finish_runtime_roundtrip()
    ok(metrics.frames <= 240, "automation scenario exceeded the 240-frame contract")
    ok(metrics.peak_events_per_frame <= 128,
        "automation scenario exceeded the 128-event frame budget")
    return {
        stamp = stamp,
        initial_count = initial_count,
        final_count = final_count,
        operation = "load-run-stop-recover",
    }
end

local succeeded, data = xpcall(mode == "generate" and generate or verify,
    debug.traceback)
local report = assert(io.open(RESULT_FILE, "wb"))
if succeeded then
    report:write("OMNI_AUTOMATION_STATUS=PASS\n")
    report:write("OMNI_AUTOMATION_CASE=" .. case_id .. "\n")
    report:write("OMNI_AUTOMATION_MODE=" .. mode .. "\n")
    report:write("OMNI_AUTOMATION_OPERATION=" .. data.operation .. "\n")
    report:write("OMNI_AUTOMATION_STAMP=" .. data.stamp .. "\n")
    report:write("OMNI_AUTOMATION_INITIAL_PARTICLES=" .. data.initial_count .. "\n")
    report:write("OMNI_AUTOMATION_FINAL_PARTICLES=" .. data.final_count .. "\n")
    report:write("OMNI_AUTOMATION_ASSERTIONS=" .. assertions .. "\n")
    report:write("OMNI_AUTOMATION_FRAMES=" .. metrics.frames .. "\n")
    report:write("OMNI_AUTOMATION_INPUT_EVENTS=" .. metrics.input_events .. "\n")
    report:write("OMNI_AUTOMATION_OUTPUT_EVENTS=" .. metrics.output_events .. "\n")
    report:write("OMNI_AUTOMATION_PEAK_EVENTS_PER_FRAME=" .. metrics.peak_events_per_frame .. "\n")
    report:write("OMNI_AUTOMATION_BLOCKED_EVENTS=" .. metrics.blocked_events .. "\n")
    report:write("OMNI_AUTOMATION_STOP_EVENT_DELTA=" .. metrics.stop_event_delta .. "\n")
    report:write("OMNI_AUTOMATION_RECOVERY_ASSERTIONS=" .. metrics.recovery_assertions .. "\n")
    report:write("OMNI_AUTOMATION_OMNI_EVENTS=" .. metrics.omni_events .. "\n")
    if challenge_id ~= "" then
        report:write("OMNI_AUTOMATION_CHALLENGE=" .. challenge_id .. "\n")
    end
else
    report:write("OMNI_AUTOMATION_STATUS=FAIL\n")
    report:write("OMNI_AUTOMATION_CASE=" .. tostring(case_id) .. "\n")
    report:write("OMNI_AUTOMATION_MODE=" .. tostring(mode) .. "\n")
    report:write("OMNI_AUTOMATION_ERROR=" .. tostring(data):gsub("[\r\n]+", " | ") .. "\n")
end
report:close()
os.exit(succeeded and 0 or 1)
