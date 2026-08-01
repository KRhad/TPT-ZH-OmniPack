local RESULT = "lua-isotope-regression.result"

local function must_element(identifier, short_name)
    local id = elements[identifier]
    assert(type(id) == "number", "missing element constant: " .. identifier)
    assert(elements.getByName(short_name) == id,
        "name/identifier mismatch for " .. identifier)
    return id
end

local ids = {
    deut = assert(elements.DEFAULT_PT_DEUT),
    neutron = assert(elements.DEFAULT_PT_NEUT),
    electron = assert(elements.DEFAULT_PT_ELEC),
    photon = assert(elements.DEFAULT_PT_PHOT),
    fire = assert(elements.DEFAULT_PT_FIRE),
    spark = assert(elements.DEFAULT_PT_SPRK),
    lava = assert(elements.DEFAULT_PT_LAVA),
    cobalt = must_element("OMNI_PT_COBT", "COBT"),
    moderator = must_element("OMNI_PT_MODR", "MODR"),
    control = must_element("OMNI_PT_CROD", "CROD"),
    fuel = must_element("OMNI_PT_NFUL", "NFUL"),
    helium = must_element("OMNI_PT_HE", "HELI"),
    nitrogen = must_element("OMNI_PT_N", "NTRG"),
    nickel = must_element("OMNI_PT_NICL", "NICL"),
    yttrium = must_element("OMNI_PT_Y", "YTTR"),
    xenon = must_element("OMNI_PT_XE", "XENO"),
    barium = must_element("OMNI_PT_BA", "BARI"),
    radium = must_element("OMNI_PT_RA", "RADI"),
    thorium = must_element("OMNI_PT_TH", "THOR"),
    uranium = assert(elements.DEFAULT_PT_URAN),
    neptunium = must_element("OMNI_PT_NP", "NEPT"),
    curium = must_element("OMNI_PT_CM", "CURI"),
    h2 = must_element("OMNI_PT_H2IS", "DTER"),
    h3 = must_element("OMNI_PT_H3IS", "TRIT"),
    c14 = must_element("OMNI_PT_C14I", "CTRC"),
    co60 = must_element("OMNI_PT_CO60", "COGM"),
    sr90 = must_element("OMNI_PT_SR90", "SRBT"),
    i131 = must_element("OMNI_PT_I131", "IODR"),
    cs137 = must_element("OMNI_PT_CS37", "CSGM"),
    th232 = must_element("OMNI_PT_TH32", "THRT"),
    u235 = must_element("OMNI_PT_U235", "UFIS"),
    u238 = must_element("OMNI_PT_U238", "UFRT"),
    pu239 = must_element("OMNI_PT_PU39", "PUTF"),
    am241 = must_element("OMNI_PT_AM41", "AMIS"),
    cf252 = must_element("OMNI_PT_CF52", "CFNS"),
}

assert(ids.h2 == 576 and ids.cf252 == 588,
    "isotope stable range changed: expected 576..588")
assert(ids.deut ~= ids.h2 and elements.getByName("DEUT") == ids.deut,
    "official DEUT heavy water was silently reused as hydrogen-2")

local function configure_simulation()
    sim.clearSim()
    sim.paused(true)
    sim.gravityMode(sim.GRAV_OFF)
    sim.airMode(sim.AIR_OFF)
    sim.ambientHeatSim(false)
    sim.heatSim(true)
    sim.ensureDeterminism(true)
    sim.randomSeed(19, 23, 29, 31)
    sim.resetOmniEventMetrics()
end

local function make(element, x, y, temperature)
    local particle = sim.partCreate(-1, x, y, element)
    assert(particle >= 0, "failed to create element " .. tostring(element))
    if temperature then sim.partProperty(particle, "temp", temperature) end
    return particle
end

local function count_type(element)
    local count = 0
    for particle in sim.parts() do
        if sim.partProperty(particle, "type") == element then count = count + 1 end
    end
    return count
end

local function represents_type(particle, element)
    local actual_type = sim.partProperty(particle, "type")
    if actual_type == element then return true end
    return (actual_type == ids.spark or actual_type == ids.lava)
        and sim.partProperty(particle, "ctype") == element
end

local function run_decay_case(name, source, product, radiation)
    configure_simulation()
    local isotope = make(source, 120, 120, 300.0)
    sim.partProperty(isotope, "life", 0)
    sim.updateUpTo()
    assert(represents_type(isotope, product),
        name .. " did not decay to its representative product; type="
        .. tostring(sim.partProperty(isotope, "type"))
        .. " ctype=" .. tostring(sim.partProperty(isotope, "ctype"))
        .. " life=" .. tostring(sim.partProperty(isotope, "life"))
        .. " tmp2=" .. tostring(sim.partProperty(isotope, "tmp2")))
    local radiation_survived = count_type(radiation) >= 1
    local radiation_ionised_product =
        (radiation == ids.electron or radiation == ids.photon)
        and sim.partProperty(isotope, "type") == ids.spark
        and sim.partProperty(isotope, "ctype") == product
    assert(radiation_survived or radiation_ionised_product,
        name .. " did not leave bounded radiation or its ionised product; type="
        .. tostring(sim.partProperty(isotope, "type"))
        .. " ctype=" .. tostring(sim.partProperty(isotope, "ctype"))
        .. " radiation_count=" .. tostring(count_type(radiation)))
end

local function run_decay_paths()
    run_decay_case("hydrogen-3", ids.h3, ids.helium, ids.electron)
    run_decay_case("carbon-14", ids.c14, ids.nitrogen, ids.electron)
    run_decay_case("cobalt-60", ids.co60, ids.nickel, ids.photon)
    run_decay_case("strontium-90", ids.sr90, ids.yttrium, ids.electron)
    run_decay_case("iodine-131", ids.i131, ids.xenon, ids.photon)
    run_decay_case("caesium-137", ids.cs137, ids.barium, ids.photon)
    run_decay_case("thorium-232", ids.th232, ids.radium, ids.helium)
    run_decay_case("uranium-235", ids.u235, ids.thorium, ids.helium)
    run_decay_case("uranium-238", ids.u238, ids.thorium, ids.helium)
    run_decay_case("plutonium-239", ids.pu239, ids.uranium, ids.helium)
    run_decay_case("americium-241", ids.am241, ids.neptunium, ids.helium)
    run_decay_case("californium-252", ids.cf252, ids.curium, ids.neutron)
end

local function run_capture_case(name, source, product)
    configure_simulation()
    local isotope = make(source, 120, 120, 300.0)
    local neutron = make(ids.neutron, 121, 120, 300.0)
    sim.updateUpTo()
    assert(sim.partProperty(isotope, "type") == product and not sim.partExists(neutron),
        name .. " did not consume one neutron into the expected product")
end

local function run_fission_case(name, source, needs_moderator)
    configure_simulation()
    local fuel = make(source, 120, 120, 600.0)
    if needs_moderator then make(ids.moderator, 121, 120, 300.0) end
    make(ids.neutron, 120, 121, 600.0)
    sim.updateUpTo()
    assert(represents_type(fuel, ids.sr90)
            or represents_type(fuel, ids.i131)
            or represents_type(fuel, ids.cs137),
        name .. " did not create a registered fission-product isotope")
    assert(count_type(ids.neutron) == 1,
        name .. " did not replace the incoming neutron with one bounded neutron")
end

local function run_neutron_paths()
    run_capture_case("hydrogen-2 capture", ids.h2, ids.h3)

    configure_simulation()
    local cobalt = make(ids.cobalt, 120, 120, 300.0)
    local neutron = make(ids.neutron, 121, 120, 300.0)
    sim.updateUpTo()
    assert(sim.partProperty(cobalt, "type") == ids.co60 and not sim.partExists(neutron),
        "solid cobalt did not activate into cobalt-60")

    run_capture_case("thorium-232 breeding", ids.th232, ids.fuel)
    run_capture_case("uranium-238 breeding", ids.u238, ids.pu239)

    configure_simulation()
    local controlled = make(ids.u235, 120, 120, 600.0)
    make(ids.moderator, 121, 120, 300.0)
    make(ids.control, 119, 120, 300.0)
    local blocked_neutron = make(ids.neutron, 120, 121, 600.0)
    sim.updateUpTo()
    assert(sim.partProperty(controlled, "type") == ids.u235
            and sim.partExists(blocked_neutron),
        "control rod did not suppress uranium-235 fission")

    run_fission_case("uranium-235 fission", ids.u235, true)
    run_fission_case("plutonium-239 fission", ids.pu239, true)
    run_fission_case("californium-252 fission", ids.cf252, false)
end

local function run_ignition_case(name, source)
    configure_simulation()
    local isotope = make(source, 120, 120, 300.0)
    make(ids.fire, 121, 120, 900.0)
    sim.updateUpTo()
    assert(sim.partProperty(isotope, "type") == ids.fire
            and sim.partProperty(isotope, "life") >= 180
            and sim.partProperty(isotope, "life") <= 259,
        name .. " did not enter the bounded hydrogen fire path")
end

local function run_ignition_paths()
    run_ignition_case("hydrogen-2 ignition", ids.h2)
    run_ignition_case("hydrogen-3 ignition", ids.h3)
end

local function run_phase_path()
    configure_simulation()
    local iodine = make(ids.i131, 120, 120, 500.0)
    local original_life = sim.partProperty(iodine, "life")
    sim.updateUpTo()
    assert(sim.partProperty(iodine, "type") == ids.lava
            and sim.partProperty(iodine, "ctype") == ids.i131,
        "iodine-131 did not enter typed isotope lava")
    local molten_timer = sim.partProperty(iodine, "tmp3")
    assert(molten_timer > 0 and molten_timer <= original_life
            and original_life - molten_timer <= 1,
        "iodine-131 melting reset its decay timer; original="
        .. tostring(original_life) .. " molten=" .. tostring(molten_timer))
    sim.partProperty(iodine, "temp", 300.0)
    sim.updateUpTo()
    assert(sim.partProperty(iodine, "type") == ids.i131
            and sim.partProperty(iodine, "life") > 0
            and sim.partProperty(iodine, "life") <= molten_timer
            and molten_timer - sim.partProperty(iodine, "life") <= 1,
        "iodine-131 did not freeze with its bounded decay timer")
end

local function run_budget_case()
    configure_simulation()
    local particles = {}
    for row = 0, 14 do
        for column = 0, 39 do
            local particle = make(ids.i131, 20 + column * 3, 20 + row * 3, 300.0)
            sim.partProperty(particle, "life", 0)
            particles[#particles + 1] = particle
        end
    end
    sim.updateUpTo()
    local decayed = 0
    for _, particle in ipairs(particles) do
        if represents_type(particle, ids.xenon) then decayed = decayed + 1 end
    end
    local metrics = sim.omniEventMetrics()
    local peak = assert(tonumber(metrics.peak_per_frame), "missing event peak")
    assert(decayed == 512 and peak == 512,
        "shared nuclear event budget drifted: decayed=" .. decayed
        .. " peak=" .. tostring(peak))
    return peak
end

local function test()
    run_decay_paths()
    run_neutron_paths()
    run_ignition_paths()
    run_phase_path()
    return run_budget_case()
end

local ok, data = xpcall(test, debug.traceback)
local report = assert(io.open(RESULT, "w"))
if ok then
    report:write("OMNI_ISOTOPE_ELEMENTS=13\n")
    report:write("OMNI_ISOTOPE_IDS=" .. ids.h2 .. "-" .. ids.cf252 .. "\n")
    report:write("OMNI_ISOTOPE_DECAY_PATHS=12\n")
    report:write("OMNI_ISOTOPE_NEUTRON_PATHS=8\n")
    report:write("OMNI_ISOTOPE_IGNITION_PATHS=2\n")
    report:write("OMNI_ISOTOPE_PHASE_PATHS=1\n")
    report:write("OMNI_ISOTOPE_TIMER_CONTINUITY=true\n")
    report:write("OMNI_ISOTOPE_EVENT_PEAK=" .. data .. "\n")
    report:write("OMNI_ISOTOPE_DEUT_SEPARATE=" .. ids.deut .. "\n")
    report:write("OMNI_ISOTOPE_STATUS=PASS\n")
else
    local error_text = tostring(data):gsub("[\r\n]+", " | ")
    report:write("OMNI_ISOTOPE_ERROR=" .. error_text .. "\n")
    report:write("OMNI_ISOTOPE_STATUS=FAIL\n")
end
report:close()
