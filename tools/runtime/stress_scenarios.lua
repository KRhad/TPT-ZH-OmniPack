local CONFIG_FILE = "stress-scenario.config"
local RESULT_FILE = "stress-lua.result"
local FRAME_SERIES_FILE = "frame-series.csv"
local ATMOSPHERE_SERIES_FILE = "soak-heartbeat.csv"
local ATMOSPHERE_HEARTBEAT_SECONDS = 30.0
local STALL_THRESHOLD_SECONDS = 60.0

local function read_config()
    local file = assert(io.open(CONFIG_FILE, "rb"), "cannot open " .. CONFIG_FILE)
    local text = assert(file:read("*a"), "cannot read " .. CONFIG_FILE)
    file:close()
    local config = {}
    for line in text:gmatch("[^\r\n]+") do
        local key, value = line:match("^([%w_]+)=(.*)$")
        if key then
            config[key] = value
        end
    end
    return config
end

local config = read_config()
local sample_id = assert(config.sample_id, "sample_id is required")
local warmup_seconds = assert(tonumber(config.warmup_seconds), "invalid warmup_seconds")
local sample_seconds = assert(tonumber(config.sample_seconds), "invalid sample_seconds")
local stride = assert(tonumber(config.fixture_stride), "invalid fixture_stride")
local smoke_run = config.smoke_run == "true"
local long_run = config.long_run == "true"
local long_run_checkpoint_target = smoke_run and 1 or 10
assert(warmup_seconds >= 0 and sample_seconds > 0, "invalid duration")
assert(stride >= 3 and stride <= 24, "fixture_stride must be between 3 and 24")
if long_run then
    assert(sample_id == "S20-FULL-CATALOG",
        "long run requires the full catalog scenario")
    assert((smoke_run and sample_seconds >= 2) or sample_seconds >= 7200,
        "formal long run requires at least 7200 sample seconds")
end
assert(socket and type(socket.getTime) == "function", "socket.getTime is unavailable")

local function must_element(identifier, short_name)
    local id = elements[identifier]
    assert(type(id) == "number", "missing element constant: " .. identifier)
    assert(elements.getByName(short_name) == id,
        "name/identifier mismatch for " .. identifier)
    return id
end

local ids = {
    dust = assert(elements.DEFAULT_PT_DUST),
    water = assert(elements.DEFAULT_PT_WATR),
    vapour = assert(elements.DEFAULT_PT_WTRV),
    lava = assert(elements.DEFAULT_PT_LAVA),
    spark = assert(elements.DEFAULT_PT_SPRK),
    conv = assert(elements.DEFAULT_PT_CONV),
    virs = assert(elements.DEFAULT_PT_VIRS),
    iron = assert(elements.DEFAULT_PT_IRON),
    wood = assert(elements.DEFAULT_PT_WOOD),
    coal = assert(elements.DEFAULT_PT_COAL),
    oil = assert(elements.DEFAULT_PT_OIL),
    acid = assert(elements.DEFAULT_PT_ACID),
    oxygen = assert(elements.DEFAULT_PT_O2),
    neutron = assert(elements.DEFAULT_PT_NEUT),
    pscn = assert(elements.DEFAULT_PT_PSCN),
    nscn = assert(elements.DEFAULT_PT_NSCN),
    metl = assert(elements.DEFAULT_PT_METL),
    btry = assert(elements.DEFAULT_PT_BTRY),
    wifi = assert(elements.DEFAULT_PT_WIFI),
    filt = assert(elements.DEFAULT_PT_FILT),
    dlay = assert(elements.DEFAULT_PT_DLAY),
    stor = assert(elements.DEFAULT_PT_STOR),
    ppip = assert(elements.DEFAULT_PT_PPIP),
    cray = assert(elements.DEFAULT_PT_CRAY),
    tsns = assert(elements.DEFAULT_PT_TSNS),
    dtec = assert(elements.DEFAULT_PT_DTEC),
    swch = assert(elements.DEFAULT_PT_SWCH),
    lcry = assert(elements.DEFAULT_PT_LCRY),
    brmt = assert(elements.DEFAULT_PT_BRMT),
    alum = must_element("OMNI_PT_ALUM", "ALUM"),
    magn = must_element("OMNI_PT_MAGN", "MAGN"),
    copr = must_element("OMNI_PT_COPR", "COPR"),
    tin = must_element("OMNI_PT_TIN", "TINN"),
    coke = must_element("OMNI_PT_COKE", "COKE"),
    stel = must_element("OMNI_PT_STEL", "STEL"),
    slag = must_element("OMNI_PT_SLAG", "SLAG"),
    flux = must_element("OMNI_PT_FLUX", "FLUX"),
    cruc = must_element("OMNI_PT_CRUC", "CRUC"),
    nutr = must_element("OMNI_PT_NUTR", "NUTR"),
    alga = must_element("OMNI_PT_ALGA", "ALGA"),
    mycl = must_element("OMNI_PT_MYCL", "MYCL"),
    spor = must_element("OMNI_PT_SPOR", "SPOR"),
    path = must_element("OMNI_PT_PATH", "PATH"),
    ster = must_element("OMNI_PT_STER", "STER"),
    hums = must_element("OMNI_PT_HUMS", "HUMS"),
    biof = must_element("OMNI_PT_BIOF", "BIOF"),
    nful = must_element("OMNI_PT_NFUL", "NFUL"),
    modr = must_element("OMNI_PT_MODR", "MODR"),
    crod = must_element("OMNI_PT_CROD", "CROD"),
    nclt = must_element("OMNI_PT_NCLT", "NCLT"),
    nwst = must_element("OMNI_PT_NWST", "NWST"),
    ngen = must_element("OMNI_PT_NGEN", "NGEN"),
    rshd = must_element("OMNI_PT_RSHD", "RSHD"),
    chlr = must_element("OMNI_PT_CHLR", "CHLR"),
    amon = must_element("OMNI_PT_AMON", "AMON"),
    ethl = must_element("OMNI_PT_ETHL", "ETHL"),
    kero = must_element("OMNI_PT_KERO", "KERO"),
    gaso = must_element("OMNI_PT_GASO", "GASO"),
    acty = must_element("OMNI_PT_ACTY", "ACTY"),
    cata = must_element("OMNI_PT_CATA", "CATA"),
    poly = must_element("OMNI_PT_POLY", "POLY"),
    pero = must_element("OMNI_PT_PERO", "PERO"),
    fert = must_element("OMNI_PT_FERT", "FERT"),
    glucose = must_element("OMNI_PT_GLUC", "GLUC"),
    starch = must_element("OMNI_PT_STRC", "STRC"),
    cellulose = must_element("OMNI_PT_CELU", "CELU"),
    propylene = must_element("OMNI_PT_PRPE", "PRPE"),
    butadiene = must_element("OMNI_PT_BDIE", "BDIE"),
    vinyl_chloride = must_element("OMNI_PT_VCHL", "VCHL"),
    styrene = must_element("OMNI_PT_STYR", "STYR"),
    tetrafluoroethylene = must_element("OMNI_PT_TFET", "TFET"),
    adipic_acid = must_element("OMNI_PT_ADIP", "ADIP"),
    diamine = must_element("OMNI_PT_DIAM", "DIAM"),
    epoxy_resin = must_element("OMNI_PT_ERES", "ERES"),
    polypropylene = must_element("OMNI_PT_PPLY", "PPLY"),
    pvc = must_element("OMNI_PT_PVCL", "PVCL"),
    polystyrene = must_element("OMNI_PT_PSTY", "PSTY"),
    nylon = must_element("OMNI_PT_NYLN", "NYLN"),
    rubber = must_element("OMNI_PT_RUBR", "RUBR"),
    epoxy = must_element("OMNI_PT_EPXY", "EPXY"),
    ptfe = must_element("OMNI_PT_PTFE", "PTFE"),
    bitumen = must_element("OMNI_PT_BITM", "BITM"),
    ethyl_acetate = must_element("OMNI_PT_EACT", "EACT"),
    gaas = must_element("OMNI_PT_GAAS", "GAAS"),
    gani = must_element("OMNI_PT_GANI", "GANI"),
    frit = must_element("OMNI_PT_FRIT", "FRIT"),
    pmag = must_element("OMNI_PT_PMAG", "PMAG"),
    smag = must_element("OMNI_PT_SMAG", "SMAG"),
    pzcr = must_element("OMNI_PT_PZCR", "PZCR"),
    telc = must_element("OMNI_PT_TELC", "TELC"),
    supc = must_element("OMNI_PT_SUPC", "SUPC"),
    grph = must_element("OMNI_PT_GRPH", "GRPH"),
    cntb = must_element("OMNI_PT_CNTB", "CNTB"),
    aerg = must_element("OMNI_PT_AERG", "AERG"),
    cfrp = must_element("OMNI_PT_CFRP", "CFRP"),
    lcob = must_element("OMNI_PT_LCOB", "LCOB"),
    gran = must_element("OMNI_PT_GRAN", "GRAN"),
    sele = must_element("OMNI_PT_SELE", "SELE"),
    itox = must_element("OMNI_PT_ITOX", "ITOX"),
    pcmt = must_element("OMNI_PT_PCMT", "PCMT"),
    echr = must_element("OMNI_PT_ECHR", "ECHR"),
    phrs = must_element("OMNI_PT_PHRS", "PHRS"),
    diel = must_element("OMNI_PT_DIEL", "DIEL"),
    soil = must_element("OMNI_PT_SOIL", "SOIL"),
    wastewater = must_element("OMNI_PT_WWTR", "WWTR"),
    pesticide = must_element("OMNI_PT_PEST", "PEST"),
    heavy_metal = must_element("OMNI_PT_HMET", "HMET"),
    radioactive_contaminant = must_element("OMNI_PT_RCON", "RCON"),
    microplastic = must_element("OMNI_PT_MPLS", "MPLS"),
    organic_waste = must_element("OMNI_PT_OWST", "OWST"),
    bloom = must_element("OMNI_PT_BLOM", "BLOM"),
    mold = must_element("OMNI_PT_MOLD", "MOLD"),
    blood = must_element("OMNI_PT_BLOD", "BLOD"),
    toxin = must_element("OMNI_PT_TOXN", "TOXN"),
    antimicrobial = must_element("OMNI_PT_AMAT", "AMAT"),
    sludge = must_element("OMNI_PT_SLUD", "SLUD"),
    smog = must_element("OMNI_PT_SMOG", "SMOG"),
    acid_rain = must_element("OMNI_PT_ARAN", "ARAN"),
    detergent = must_element("OMNI_PT_DETG", "DETG"),
    calcium_hydroxide = must_element("OMNI_PT_CAOH", "CAOH"),
    fighter = assert(elements.DEFAULT_PT_FIGH),
}

local RECOVERABLE_SCRAP_MARKER = 0x4F4D5343
local LEGACY_ALIAS_ID = 278
local FIGHTER_SAVE_LIMIT = 50
local fixture_type_count = 0
local fixture_created_type_count = 0
local fixture_visible_type_count = 0

local function configure_simulation()
    sim.clearSim()
    sim.paused(true)
    sim.gravityMode(sim.GRAV_OFF)
    sim.airMode(sim.AIR_OFF)
    sim.ambientHeatSim(false)
    sim.heatSim(true)
    sim.ensureDeterminism(true)
    sim.randomSeed(11, 12, 13, 14)
    if long_run then
        sim.edgeMode(sim.EDGE_SOLID)
        sim.omniSimulationMode(sim.OMNI_ENHANCED)
        local atmosphere = sim.omniAtmosphere()
        assert(atmosphere.active and atmosphere.available,
            "formal long run did not activate OmniAtmosphere")
    end
end

local function make(type, x, y, properties)
    local particle = sim.partCreate(-1, x, y, type)
    if particle < 0 then
        return nil
    end
    for property, value in pairs(properties or {}) do
        sim.partProperty(particle, property, value)
    end
    return particle
end

local function molten(ctype, x, y, temperature)
    return make(ids.lava, x, y, { ctype = ctype, temp = temperature or 1800.0 })
end

local function spark_generator(x, y, temperature)
    local particle = make(ids.ngen, x, y, { temp = temperature or 400.0 })
    if particle then
        sim.partProperty(particle, "type", ids.spark)
        sim.partProperty(particle, "ctype", ids.ngen)
        sim.partProperty(particle, "life", 4)
    end
    return particle
end

local function spark_nichrome(x, y, temperature)
    local particle = make(ids.ncrm, x, y, { temp = temperature or 800.0 })
    if particle then
        sim.partProperty(particle, "type", ids.spark)
        sim.partProperty(particle, "ctype", ids.ncrm)
        sim.partProperty(particle, "life", 4)
    end
    return particle
end

local function grid(bounds, callback)
    local ordinal = 0
    for y = bounds.y1, bounds.y2, stride do
        for x = bounds.x1, bounds.x2, stride do
            ordinal = ordinal + 1
            callback(x, y, ordinal)
        end
    end
end

local full = { x1 = 48, y1 = 48, x2 = sim.XRES - 49, y2 = sim.YRES - 49 }

local periodic_types = {
    148, 370, 191, 371, 372, 28, 373, 61, 374, 375, 376, 261,
    256, 187, 377, 378, 360, 379, 380, 381, 382, 144, 383, 262,
    384, 76, 263, 260, 257, 265, 385, 386, 387, 388, 389, 390,
    41, 391, 392, 393, 394, 264, 395, 396, 397, 398, 399, 400,
    401, 259, 402, 403, 404, 405, 406, 407, 408, 409, 410, 411,
    412, 413, 414, 415, 416, 417, 418, 419, 420, 421, 422, 423,
    424, 171, 425, 426, 427, 188, 170, 152, 428, 258, 429, 182,
    430, 431, 432, 433, 434, 435, 436, 32, 437, 19, 438, 439,
    440, 441, 442, 443, 444, 445, 446, 447, 448, 449, 450, 451,
    452, 453, 454, 455, 456, 457, 458, 459, 460, 461,
}

local function validate_type_list(types, expected_count, label, require_visible)
    assert(#types == expected_count,
        label .. " fixture count changed: " .. tostring(#types))
    local seen = {}
    for _, type in ipairs(types) do
        assert(type >= 0 and type < 1024, label .. " fixture contains invalid ID")
        assert(not seen[type], label .. " fixture repeats ID " .. tostring(type))
        seen[type] = true
        assert(elements.property(type, "Enabled") == 1,
            label .. " fixture contains disabled ID " .. tostring(type))
        if require_visible ~= false then
            assert(elements.property(type, "MenuVisible") == 1,
                label .. " fixture contains hidden ID " .. tostring(type))
        end
    end
    return types
end

local function enabled_range(first, last, expected_count, label)
    local types = {}
    for type = first, last do
        if elements.property(type, "Enabled") == 1
            and elements.property(type, "MenuVisible") == 1 then
            types[#types + 1] = type
        end
    end
    return validate_type_list(types, expected_count, label)
end

local function playable_catalog()
    local types = {}
    for type = 0, 1023 do
        local valid, enabled = pcall(elements.property, type, "Enabled")
        if valid and enabled == 1 and type ~= LEGACY_ALIAS_ID then
            types[#types + 1] = type
        end
    end
    return validate_type_list(types, 487, "full catalog", false)
end

local function catalog_fixture(bounds, types, temperature, require_all_created)
    local created = {}
    local created_instances = {}
    local function place(type, x, y)
        -- Official FIGH accepts more live instances than OPS restores. Keep the
        -- fixture at the official save-compatible fighter limit so an immediate
        -- roundtrip tests content preservation instead of deliberate FIGH
        -- normalization from an invalid over-cap population.
        if type == ids.fighter
            and (created_instances[type] or 0) >= FIGHTER_SAVE_LIMIT then
            return
        end
        if make(type, x, y, { temp = temperature }) then
            created[type] = true
            created_instances[type] = (created_instances[type] or 0) + 1
        end
    end
    grid(bounds, function(x, y, n)
        local first = types[((n - 1) % #types) + 1]
        local second = types[((n + 36) % #types) + 1]
        place(first, x, y)
        place(second, x + 1, y)
    end)
    local created_count = 0
    local visible_count = 0
    local missing = {}
    for _, type in ipairs(types) do
        if created[type] then
            created_count = created_count + 1
        else
            missing[#missing + 1] = elements.property(type, "Identifier")
        end
        if type ~= 0 and elements.property(type, "MenuVisible") == 1 then
            visible_count = visible_count + 1
        end
    end
    fixture_type_count = #types
    fixture_created_type_count = created_count
    fixture_visible_type_count = visible_count
    if require_all_created then
        assert(created_count == #types,
            "catalog fixture could not create: " .. table.concat(missing, ","))
    end
end

local recovery_markers = {
    {
        name = "brmt_recoverable_ctype",
        x = 8,
        y = 8,
        particle_type = ids.brmt,
        properties = {
            ctype = ids.alum,
            tmp4 = RECOVERABLE_SCRAP_MARKER,
        },
    },
    {
        name = "conv_ctype_tmp",
        x = 12,
        y = 8,
        particle_type = ids.conv,
        properties = { ctype = ids.nful, tmp = ids.chlr },
    },
    {
        name = "virs_tmp2",
        x = 16,
        y = 8,
        particle_type = ids.virs,
        properties = { tmp2 = ids.alum },
    },
    {
        name = "electronics_conv_fields",
        x = 20,
        y = 8,
        particle_type = ids.conv,
        properties = { ctype = ids.diel, tmp = ids.pcmt },
    },
    {
        name = "environment_conv_fields",
        x = 24,
        y = 8,
        particle_type = ids.conv,
        properties = { ctype = ids.detergent, tmp = ids.radioactive_contaminant },
    },
}

local function create_recovery_markers()
    -- Dense/long-running scenarios can drift an unrelated particle onto a
    -- marker coordinate. Clear only the five exact test pixels, then bind the
    -- expected round-trip count to the post-clear baseline.
    for _, marker in ipairs(recovery_markers) do
        local occupant = sim.partID(marker.x, marker.y)
        while type(occupant) == "number" do
            sim.partKill(occupant)
            occupant = sim.partID(marker.x, marker.y)
        end
    end
    local cleared_particle_count = 0
    for _ in sim.parts() do
        cleared_particle_count = cleared_particle_count + 1
    end
    for _, marker in ipairs(recovery_markers) do
        local particle = assert(make(
            marker.particle_type, marker.x, marker.y, marker.properties),
            "failed to create recovery marker: " .. marker.name)
        assert(sim.partID(marker.x, marker.y) == particle,
            "recovery marker is not addressable: " .. marker.name)
    end
    return cleared_particle_count
end

local function verify_recovery_markers()
    local assertions = 0
    for _, marker in ipairs(recovery_markers) do
        local particle = sim.partID(marker.x, marker.y)
        assert(type(particle) == "number",
            "missing recovered marker: " .. marker.name)
        assert(sim.partProperty(particle, "type") == marker.particle_type,
            "recovered marker type changed: " .. marker.name)
        assertions = assertions + 1
        for property, expected in pairs(marker.properties) do
            assert(sim.partProperty(particle, property) == expected,
                "recovered marker field changed: " .. marker.name .. "." .. property)
            assertions = assertions + 1
        end
    end
    return assertions
end

local function metallurgy(bounds)
    grid(bounds, function(x, y, n)
        local recipe = n % 4
        if recipe == 0 then
            molten(ids.alum, x, y, 1000.0)
            molten(ids.alum, x + 1, y, 1000.0)
            molten(ids.magn, x, y + 1, 1000.0)
        elseif recipe == 1 then
            molten(ids.copr, x, y, 1500.0)
            molten(ids.copr, x + 1, y, 1500.0)
            molten(ids.tin, x, y + 1, 1500.0)
        elseif recipe == 2 then
            molten(ids.iron, x, y, 2600.0)
            make(ids.coke, x + 1, y, { temp = 1200.0 })
            make(ids.flux, x, y + 1, { temp = 1200.0 })
        else
            make(ids.brmt, x, y, {
                ctype = ids.stel,
                tmp4 = RECOVERABLE_SCRAP_MARKER,
                temp = 900.0,
            })
            make(ids.flux, x + 1, y, { temp = 900.0 })
            make(ids.slag, x, y + 1, { temp = 900.0 })
        end
    end)
end

local function furnaces(bounds)
    grid(bounds, function(x, y, n)
        make(ids.cruc, x, y, { temp = 1300.0 })
        make((n % 2 == 0) and ids.wood or ids.coal, x + 1, y,
            { temp = 1300.0 })
        make(ids.oxygen, x, y + 1, { temp = 900.0 })
    end)
end

local function ecology(bounds)
    local types = { ids.nutr, ids.alga, ids.mycl, ids.spor, ids.hums, ids.water }
    grid(bounds, function(x, y, n)
        make(types[(n % #types) + 1], x, y, { temp = 298.15 })
        make(types[((n + 2) % #types) + 1], x + 1, y, { temp = 298.15 })
    end)
end

local function pathogen(bounds)
    local types = { ids.path, ids.water, ids.ster, ids.biof, ids.nutr }
    grid(bounds, function(x, y, n)
        make(types[(n % #types) + 1], x, y, { temp = 310.0 })
        make(types[((n + 1) % #types) + 1], x + 1, y, { temp = 310.0 })
        -- Keep a sparse chemistry-to-biology treatment and recovery loop in
        -- the pathogen fixture.  The extra particles stay inside this
        -- stride cell, so the stress sample remains a bounded local test.
        if n % 6 == 0 then
            make(ids.path, x + 2, y, { temp = 300.0 })
            make(ids.pero, x + 2, y + 1, { temp = 300.0 })
        elseif n % 6 == 1 then
            make(ids.hums, x + 2, y, { temp = 300.0 })
            make(ids.fert, x + 2, y + 1, { temp = 300.0 })
            make(ids.water, x + 1, y + 1, { temp = 300.0 })
        end
    end)
end

local function ecology_chemistry_loop(bounds)
    grid(bounds, function(x, y, n)
        if n % 2 == 0 then
            make(ids.path, x + 2, y + 1, { temp = 300.0 })
            make(ids.pero, x + 2, y + 2, { temp = 300.0 })
        else
            make(ids.hums, x + 2, y + 1, { temp = 300.0 })
            make(ids.fert, x + 2, y + 2, { temp = 300.0 })
            make(ids.water, x + 1, y + 2, { temp = 300.0 })
        end
    end)
end

local function chemistry(bounds)
    local types = {
        ids.chlr, ids.amon, ids.ethl, ids.kero, ids.gaso,
        ids.acty, ids.cata, ids.poly, ids.pero, ids.fert,
        ids.oil, ids.water, ids.glucose, ids.starch, ids.cellulose,
        ids.propylene, ids.butadiene, ids.vinyl_chloride, ids.styrene,
        ids.tetrafluoroethylene, ids.adipic_acid, ids.diamine,
        ids.epoxy_resin, ids.polypropylene, ids.pvc, ids.polystyrene,
        ids.nylon, ids.rubber, ids.epoxy, ids.ptfe, ids.bitumen,
        ids.ethyl_acetate,
    }
    grid(bounds, function(x, y, n)
        make(types[(n % #types) + 1], x, y, { temp = 430.0 })
        make(types[((n + 5) % #types) + 1], x + 1, y, { temp = 430.0 })
        if n % 8 == 0 then
            make(ids.cata, x + 2, y + 1, { temp = 320.0 })
            make(ids.slag, x + 2, y + 2, { temp = 320.0 })
            make(ids.acid, x + 1, y + 2, { temp = 320.0 })
        elseif n % 8 == 4 then
            make(ids.nwst, x + 1, y + 1, { temp = 550.0 })
            make(ids.slag, x + 2, y + 1, { temp = 550.0 })
            make(ids.hums, x + 2, y + 2, { temp = 550.0 })
            make(ids.poly, x + 1, y + 2, { temp = 550.0 })
            make(ids.cata, x, y + 2, { temp = 550.0 })
            make(ids.water, x, y + 1, { temp = 550.0 })
        end
    end)
end

local function electronics(bounds)
    sim.airMode(sim.AIR_NOUPDATE)
    local passive = {
        ids.gaas, ids.gani, ids.frit, ids.pmag, ids.smag, ids.pzcr,
        ids.telc, ids.supc, ids.grph, ids.cntb, ids.aerg, ids.cfrp,
        ids.lcob, ids.gran, ids.sele, ids.itox, ids.pcmt, ids.echr,
        ids.phrs, ids.diel,
    }
    grid(bounds, function(x, y, n)
        local mode = n % 6
        if mode == 0 then
            make(ids.cata, x, y, { temp = 1800.0 })
            make(ids.coal, x + 1, y, { temp = 1800.0 })
        elseif mode == 1 then
            make(ids.cntb, x, y, { temp = 300.0 })
            sim.pressure(math.floor(x / 4), math.floor(y / 4), 30.0)
        elseif mode == 2 then
            make(ids.aerg, x, y, { temp = 300.0 })
            sim.pressure(math.floor(x / 4), math.floor(y / 4), 12.0)
        elseif mode == 3 then
            make(ids.grph, x, y, { temp = 950.0 })
            make(ids.oxygen, x + 1, y, { temp = 950.0 })
        elseif mode == 4 then
            make(ids.lcob, x, y, { temp = 700.0 })
            make(ids.oxygen, x + 1, y, { temp = 700.0 })
        else
            make(passive[(n % #passive) + 1], x, y, { temp = 300.0 })
            make(passive[((n + 7) % #passive) + 1], x + 1, y,
                { temp = 300.0 })
        end
    end)
end

local function environment(bounds)
    grid(bounds, function(x, y, n)
        local mode = n % 12
        if mode == 0 then
            make(ids.pesticide, x, y, { temp = 300.0 })
            make(ids.path, x + 1, y, { temp = 300.0 })
        elseif mode == 1 then
            make(ids.heavy_metal, x, y, { temp = 300.0 })
            make(ids.water, x + 1, y, { temp = 300.0 })
        elseif mode == 2 then
            make(ids.wastewater, x, y, { temp = 300.0 })
            make(ids.alga, x + 1, y, { temp = 300.0 })
            make(ids.nutr, x, y + 1, { temp = 300.0 })
        elseif mode == 3 then
            make(ids.microplastic, x, y, { temp = 300.0 })
            make(ids.biof, x + 1, y, { temp = 300.0 })
        elseif mode == 4 then
            make(ids.organic_waste, x, y, { temp = 300.0 })
            make(ids.mycl, x + 1, y, { temp = 300.0 })
            make(ids.water, x, y + 1, { temp = 300.0 })
        elseif mode == 5 then
            make(ids.bloom, x, y, { temp = 300.0, tmp = 3 })
            make(ids.oxygen, x + 1, y, { temp = 300.0 })
        elseif mode == 6 then
            make(ids.smog, x, y, { temp = 300.0 })
            make(ids.vapour, x + 1, y, { temp = 300.0 })
        elseif mode == 7 then
            make(ids.detergent, x, y, { temp = 300.0 })
            make(ids.oil, x + 1, y, { temp = 300.0 })
            make(ids.water, x, y + 1, { temp = 300.0 })
        elseif mode == 8 then
            make(ids.radioactive_contaminant, x, y, { temp = 300.0, life = 33 })
            make(ids.water, x + 1, y, { temp = 300.0 })
        elseif mode == 9 then
            make(ids.mold, x, y, { temp = 300.0 })
            make(ids.organic_waste, x + 1, y, { temp = 300.0 })
            make(ids.water, x, y + 1, { temp = 300.0 })
        elseif mode == 10 then
            make(ids.blood, x, y, { temp = 300.0 })
            make(ids.path, x + 1, y, { temp = 300.0 })
        else
            local variant = math.floor(n / 12) % 4
            if variant == 0 then
                make(ids.antimicrobial, x, y, { temp = 300.0 })
                make(ids.path, x + 1, y, { temp = 300.0 })
            elseif variant == 1 then
                make(ids.soil, x, y, { temp = 300.0, tmp = 1 })
                make(ids.toxin, x + 1, y, { temp = 300.0 })
            elseif variant == 2 then
                make(ids.sludge, x, y, { temp = 400.0 })
            else
                make(ids.acid_rain, x, y, { temp = 300.0 })
                make(ids.calcium_hydroxide, x + 1, y, { temp = 300.0 })
            end
        end
    end)
end

local function generators(bounds)
    grid(bounds, function(x, y, n)
        spark_generator(x, y, 450.0)
        if n % 3 ~= 0 then
            make(ids.nful, x + 1, y, { temp = 450.0 })
        end
        if n % 4 ~= 0 then
            make(ids.modr, x, y + 1, { temp = 450.0 })
        end
        if n % 5 == 0 then
            make(ids.crod, x + 1, y + 1, { temp = 450.0 })
        end
    end)
end

local function stable_reactor(bounds)
    grid(bounds, function(x, y, n)
        spark_generator(x, y, 450.0)
        make(ids.nful, x + 1, y, { temp = 450.0 })
        make(ids.modr, x, y + 1, { temp = 450.0 })
        if n % 2 == 0 then
            make(ids.crod, x + 1, y + 1, { temp = 450.0 })
        else
            make(ids.nclt, x + 1, y + 1, { temp = 450.0 })
        end
        make(ids.rshd, x + 2, y, { temp = 450.0 })
        if n % 6 == 0 then
            make(ids.ssil, x + 2, y + 1, { temp = 800.0 })
            molten(ids.lead, x + 2, y + 2, 800.0)
            spark_nichrome(x + 1, y + 2, 800.0)
        end
    end)
end

local function loca(bounds)
    grid(bounds, function(x, y, n)
        if n % 2 == 0 then
            make(ids.nwst, x, y, { temp = 1700.0 })
            make(ids.rshd, x + 1, y, { temp = 500.0 })
        else
            spark_generator(x, y, 800.0)
            make(ids.nful, x + 1, y, { temp = 800.0 })
            make(ids.modr, x, y + 1, { temp = 800.0 })
        end
    end)
end

local function carriers(bounds)
    local omni = { ids.alum, ids.nutr, ids.nful, ids.chlr, ids.cruc }
    grid(bounds, function(x, y, n)
        local target = omni[(n % #omni) + 1]
        local kind = n % 5
        if kind == 0 then
            make(ids.lava, x, y, { ctype = target, temp = 1700.0 })
        elseif kind == 1 then
            local spark = make(target, x, y, { temp = 400.0 })
            if spark then
                sim.partProperty(spark, "type", ids.spark)
                sim.partProperty(spark, "ctype", target)
                sim.partProperty(spark, "life", 4)
            end
        elseif kind == 2 then
            make(ids.brmt, x, y, {
                ctype = target,
                tmp4 = RECOVERABLE_SCRAP_MARKER,
            })
        elseif kind == 3 then
            make(ids.conv, x, y, { ctype = target, tmp = omni[((n + 1) % #omni) + 1] })
        else
            make(ids.virs, x, y, { tmp2 = target })
        end
    end)
end

local function automation_factory(bounds)
    grid(bounds, function(x, y, n)
        local kind = n % 4
        local target
        if kind == 0 then
            molten(ids.copr, x, y, 2000.0)
            molten(ids.copr, x + 1, y, 2000.0)
            molten(ids.copr, x, y + 1, 2000.0)
            molten(ids.tin, x + 1, y + 1, 2000.0)
            target = ids.lava
        elseif kind == 1 then
            make(ids.path, x, y, { temp = 300.0 })
            make(ids.pero, x + 1, y, { temp = 300.0 })
            target = ids.path
        elseif kind == 2 then
            make(ids.nclt, x, y, { temp = 900.0 })
            make(ids.nwst, x + 1, y, { temp = 1200.0 })
            target = ids.nwst
        else
            make(ids.oil, x, y, { temp = 550.0 })
            make(ids.cata, x + 1, y, { temp = 550.0 })
            target = ids.cata
        end
        if n % 2 == 0 then
            make(ids.tsns, x + 2, y, { temp = 450.0, tmp = 0, tmp2 = 2 })
        else
            make(ids.dtec, x + 2, y, { ctype = target, tmp2 = 2 })
        end
        make(ids.pscn, x + 2, y + 1)
    end)
end

local function automation_signal_loop(bounds)
    grid(bounds, function(x, y, n)
        local channel_temperature = 173.15 + ((n % 8) * 100.0)
        make(ids.wifi, x, y, { temp = channel_temperature })
        make(ids.pscn, x + 1, y)
        -- Official BTRY repeatedly excites adjacent conductors after their
        -- cooldown, so the formal sample measures a sustained official SPRK
        -- population instead of a one-shot pulse consumed during warmup.
        make(ids.btry, x + 2, y)
        make(ids.dlay, x, y + 1, { temp = 275.15 })
        make(ids.pscn, x + 1, y + 1)
        make(ids.nscn, x + 2, y + 1)
        make(ids.swch, x, y + 2, { life = 10 })
        make(ids.ppip, x + 1, y + 2, { life = 0 })
        make(ids.lcry, x + 2, y + 2)
    end)
end

local scenarios = {
    ["S01-METALLURGY-LARGE"] = function() metallurgy(full) end,
    ["S02-FURNACES-PARALLEL"] = function() furnaces(full) end,
    ["S03-ECOLOGY-AREA"] = function() ecology(full) end,
    ["S04-PATHOGEN-CONTROL"] = function()
        pathogen(full)
    end,
    ["S05-CHEMISTRY-DENSE"] = function() chemistry(full) end,
    ["S06-NEUTRON-GENERATORS"] = function() generators(full) end,
    ["S07-REACTOR-STABLE"] = function() stable_reactor(full) end,
    ["S08-REACTOR-LOCA"] = function() loca(full) end,
    ["S09-ALL-MODULES"] = function()
        metallurgy({ x1 = 48, y1 = 48, x2 = sim.XRES / 2 - 12, y2 = sim.YRES / 2 - 12 })
        ecology({ x1 = sim.XRES / 2 + 12, y1 = 48, x2 = sim.XRES - 49, y2 = sim.YRES / 2 - 12 })
        ecology_chemistry_loop({ x1 = sim.XRES / 2 + 12, y1 = 48, x2 = sim.XRES - 49, y2 = sim.YRES / 2 - 12 })
        chemistry({ x1 = 48, y1 = sim.YRES / 2 + 12, x2 = sim.XRES / 2 - 12, y2 = sim.YRES - 49 })
        stable_reactor({ x1 = sim.XRES / 2 + 12, y1 = sim.YRES / 2 + 12, x2 = sim.XRES - 49, y2 = sim.YRES - 49 })
    end,
    ["S10-CARRIERS-ROUNDTRIP"] = function() carriers(full) end,
    ["S11-AUTOMATION-FACTORY"] = function() automation_factory(full) end,
    ["S12-AUTOMATION-SIGNAL-LOOP"] = function() automation_signal_loop(full) end,
    ["S13-ELECTRONICS-DENSE"] = function() electronics(full) end,
    ["S14-ENVIRONMENT-DENSE"] = function() environment(full) end,
    ["S15-PERIODIC-ALL"] = function()
        catalog_fixture(
            full,
            validate_type_list(periodic_types, 118, "periodic table"),
            300.0,
            true)
    end,
    ["S16-INORGANIC-DENSE"] = function()
        catalog_fixture(full, enabled_range(462, 511, 50, "inorganic"), 430.0, true)
    end,
    ["S17-MATERIALS-DENSE"] = function()
        catalog_fixture(full, enabled_range(512, 532, 21, "materials"), 900.0, true)
    end,
    ["S18-ISOTOPES-DENSE"] = function()
        catalog_fixture(full, enabled_range(576, 588, 13, "isotopes"), 450.0, true)
    end,
    ["S19-ORGANICS-DENSE"] = function()
        catalog_fixture(full, enabled_range(589, 621, 33, "organics"), 700.0, true)
    end,
    ["S20-FULL-CATALOG"] = function()
        catalog_fixture(full, playable_catalog(), 300.0, false)
    end,
}

local function particle_count()
    local count = 0
    for _ in sim.parts() do
        count = count + 1
    end
    return count
end

local function timed_save()
    -- Observe the authoritative state before saveStamp reaches the explicit
    -- serialization-boundary canonicalizer.  A checkpoint must never turn an
    -- invalid post-step state into an apparently healthy later heartbeat.
    local atmosphere = sim.omniAtmosphere()
    if atmosphere.available and atmosphere.active then
        assert(atmosphere.non_finite_cells == 0
            and atmosphere.state_non_finite_cells == 0,
            "OmniAtmosphere state was invalid before checkpoint save")
    end
    local started = socket.getTime()
    local stamp = sim.saveStamp(0, 0, sim.XRES - 1, sim.YRES - 1, 1)
    local elapsed_ms = (socket.getTime() - started) * 1000.0
    assert(type(stamp) == "string" and stamp:match("^[0-9A-Fa-f]+$") and #stamp == 10,
        "saveStamp did not return a ten-character stamp ID: value="
        .. tostring(stamp) .. ",particles=" .. tostring(particle_count())
        .. ",omni_mode=" .. tostring(sim.omniSimulationMode()))
    return stamp, elapsed_ms
end

local function timed_load(stamp)
    sim.clearSim()
    local started = socket.getTime()
    local loaded, load_error = sim.loadStamp(stamp, 0, 0, false, 0, 1)
    local elapsed_ms = (socket.getTime() - started) * 1000.0
    assert(loaded == 1, "loadStamp failed: " .. tostring(load_error))
    return elapsed_ms
end

local function write_success(data)
    local result = assert(io.open(RESULT_FILE, "wb"))
    result:write("OMNI_STRESS_LUA_STATUS=PASS\n")
    for _, key in ipairs({
        "sample_id", "initial_particles", "peak_particles", "final_particles",
        "warmup_frames", "sample_frames", "actual_warmup_seconds",
        "actual_sample_seconds", "average_fps", "one_percent_low_fps",
        "minimum_fps", "first_stamp", "second_stamp", "save_time_first_ms",
        "load_time_first_ms", "save_time_second_ms", "load_time_second_ms",
        "roundtrip_pass", "event_count_total", "event_count_peak_per_frame",
        "signal_count_total", "signal_count_peak_per_frame", "signal_stop_pass",
        "fixture_type_count", "fixture_created_type_count",
        "fixture_visible_type_count",
        "scenario_stop_pass", "scenario_recovery_pass", "stop_event_delta",
        "scenario_recovery_assertions",
        "long_run", "long_run_save_load_cycles", "long_run_language_switches",
        "long_run_module_toggle_cycles", "long_run_settings_recovery_pass",
        "long_run_checkpoint_save_ms_total", "long_run_checkpoint_load_ms_total",
        "simulation_steps", "heartbeat_count", "heartbeat_interval_seconds",
        "maximum_heartbeat_gap_seconds", "stalls", "nan_count", "inf_count",
        "omni_atmosphere_active", "atmosphere_mass_initial_kg",
        "atmosphere_mass_final_kg", "atmosphere_mass_min_kg",
        "atmosphere_mass_max_kg", "atmosphere_mass_residual_abs_max_kg",
        "species_mass_residual_abs_max_kg", "minimum_density_kg_m3",
        "maximum_density_kg_m3", "minimum_pressure_pa", "maximum_pressure_pa",
        "minimum_temperature_k", "maximum_temperature_k",
    }) do
        result:write(key .. "=" .. tostring(data[key]) .. "\n")
    end
    result:close()
end

local runtime = {
    callback_registered = false,
    series = nil,
    atmosphere_series = nil,
    long_run_save_load_cycles = 0,
    long_run_language_switches = 0,
    long_run_module_toggle_cycles = 0,
    long_run_checkpoint_save_ms_total = 0.0,
    long_run_checkpoint_load_ms_total = 0.0,
    language_probes = {},
    heartbeat_count = 0,
    maximum_heartbeat_gap_seconds = 0.0,
    last_heartbeat_elapsed = nil,
    stalls = 0,
    nan_count = 0,
    inf_count = 0,
    omni_atmosphere_active = false,
    atmosphere_mass_initial_kg = 0.0,
    atmosphere_mass_final_kg = 0.0,
    atmosphere_mass_min_kg = 0.0,
    atmosphere_mass_max_kg = 0.0,
    atmosphere_mass_residual_abs_max_kg = 0.0,
    species_mass_residual_abs_max_kg = 0.0,
    minimum_density_kg_m3 = 0.0,
    maximum_density_kg_m3 = 0.0,
    minimum_pressure_pa = 0.0,
    maximum_pressure_pa = 0.0,
    minimum_temperature_k = 0.0,
    maximum_temperature_k = 0.0,
}
local tick_callback

local atmosphere_species = { "N2", "O2", "Ar", "CO2", "H2O" }

local function checked_finite(value, label)
    assert(type(value) == "number", label .. " is not numeric")
    if value ~= value then
        runtime.nan_count = runtime.nan_count + 1
        error(label .. " is NaN")
    end
    if value == math.huge or value == -math.huge then
        runtime.inf_count = runtime.inf_count + 1
        error(label .. " is infinite")
    end
    return value
end

local function record_atmosphere_heartbeat(now, force)
    if not long_run then
        return
    end
    local elapsed = math.max(0.0, now - runtime.phase_started)
    if not force and now < runtime.next_atmosphere_sample then
        return
    end
    if force and runtime.last_heartbeat_elapsed
        and elapsed - runtime.last_heartbeat_elapsed < 0.001 then
        return
    end
    local atmosphere = sim.omniAtmosphere()
    assert(atmosphere.active and atmosphere.available,
        "OmniAtmosphere became inactive during formal long run")
    runtime.omni_atmosphere_active = true
    local mass = checked_finite(atmosphere.mass_kg, "atmosphere mass")
    local condensed = checked_finite(
        atmosphere.condensed_water_mass_kg, "condensed water mass")
    local mass_residual = checked_finite(
        atmosphere.mass_residual_kg, "atmosphere mass residual")
    local minimum_density = checked_finite(
        atmosphere.minimum_density_kg_m3, "minimum atmosphere density")
    local maximum_density = checked_finite(
        atmosphere.maximum_density_kg_m3, "maximum atmosphere density")
    local minimum_pressure = checked_finite(
        atmosphere.minimum_pressure_pa, "minimum atmosphere pressure")
    local maximum_pressure = checked_finite(
        atmosphere.maximum_pressure_pa, "maximum atmosphere pressure")
    local minimum_temperature = checked_finite(
        atmosphere.minimum_temperature_k, "minimum atmosphere temperature")
    local maximum_temperature = checked_finite(
        atmosphere.maximum_temperature_k, "maximum atmosphere temperature")
    assert(mass >= 0.0 and condensed >= 0.0,
        "atmosphere mass became negative")
    assert(minimum_density > 0.0 and maximum_density >= minimum_density,
        "atmosphere density range is invalid")
    assert(minimum_pressure > 0.0 and maximum_pressure >= minimum_pressure,
        "atmosphere pressure range is invalid")
    assert(minimum_temperature > 0.0 and maximum_temperature >= minimum_temperature,
        "atmosphere temperature range is invalid")
    assert(atmosphere.non_finite_cells == 0
        and atmosphere.state_non_finite_cells == 0,
        "OmniAtmosphere reported non-finite state cells")

    local species = {}
    local species_total = 0.0
    local species_residual_abs_max = 0.0
    for _, name in ipairs(atmosphere_species) do
        local value = checked_finite(
            atmosphere.species_mass_kg[name], "species mass " .. name)
        local residual = checked_finite(
            atmosphere.species_mass_residual_kg[name],
            "species mass residual " .. name)
        assert(value >= 0.0, "species mass became negative: " .. name)
        species[name] = value
        species_total = species_total + value
        species_residual_abs_max = math.max(
            species_residual_abs_max, math.abs(residual))
    end
    assert(math.abs((species_total + condensed) - mass) <= 1.0e-8,
        "atmosphere species masses no longer close to total mass")
    assert(math.abs(mass_residual) <= 1.0e-8,
        "atmosphere mass residual exceeded long-run tolerance")
    assert(species_residual_abs_max <= 1.0e-8,
        "atmosphere species residual exceeded long-run tolerance")

    runtime.heartbeat_count = runtime.heartbeat_count + 1
    if runtime.last_heartbeat_elapsed then
        runtime.maximum_heartbeat_gap_seconds = math.max(
            runtime.maximum_heartbeat_gap_seconds,
            elapsed - runtime.last_heartbeat_elapsed)
    end
    runtime.last_heartbeat_elapsed = elapsed
    runtime.atmosphere_mass_final_kg = mass
    if runtime.heartbeat_count == 1 then
        runtime.atmosphere_mass_initial_kg = mass
        runtime.atmosphere_mass_min_kg = mass
        runtime.atmosphere_mass_max_kg = mass
        runtime.minimum_density_kg_m3 = minimum_density
        runtime.maximum_density_kg_m3 = maximum_density
        runtime.minimum_pressure_pa = minimum_pressure
        runtime.maximum_pressure_pa = maximum_pressure
        runtime.minimum_temperature_k = minimum_temperature
        runtime.maximum_temperature_k = maximum_temperature
    else
        runtime.atmosphere_mass_min_kg = math.min(runtime.atmosphere_mass_min_kg, mass)
        runtime.atmosphere_mass_max_kg = math.max(runtime.atmosphere_mass_max_kg, mass)
        runtime.minimum_density_kg_m3 = math.min(
            runtime.minimum_density_kg_m3, minimum_density)
        runtime.maximum_density_kg_m3 = math.max(
            runtime.maximum_density_kg_m3, maximum_density)
        runtime.minimum_pressure_pa = math.min(
            runtime.minimum_pressure_pa, minimum_pressure)
        runtime.maximum_pressure_pa = math.max(
            runtime.maximum_pressure_pa, maximum_pressure)
        runtime.minimum_temperature_k = math.min(
            runtime.minimum_temperature_k, minimum_temperature)
        runtime.maximum_temperature_k = math.max(
            runtime.maximum_temperature_k, maximum_temperature)
    end
    runtime.atmosphere_mass_residual_abs_max_kg = math.max(
        runtime.atmosphere_mass_residual_abs_max_kg, math.abs(mass_residual))
    runtime.species_mass_residual_abs_max_kg = math.max(
        runtime.species_mass_residual_abs_max_kg, species_residual_abs_max)
    runtime.atmosphere_series:write(string.format(
        "%.6f,%d,%d,%.17g,%.17g,%.17g,%.17g,%.17g,%.17g,%.17g,%.17g,%.17g,%d,%d,%.17g,%.17g,%.17g,%.17g,%.17g,%.17g,%d,%d,%d\n",
        elapsed, runtime.sample_frames, particle_count(), mass, mass_residual,
        species_residual_abs_max, species.N2, species.O2, species.Ar,
        species.CO2, species.H2O, condensed, atmosphere.non_finite_cells,
        atmosphere.state_non_finite_cells, minimum_density, maximum_density,
        minimum_pressure, maximum_pressure, minimum_temperature,
        maximum_temperature, runtime.nan_count, runtime.inf_count,
        runtime.stalls))
    runtime.atmosphere_series:flush()
    runtime.next_atmosphere_sample = now + ATMOSPHERE_HEARTBEAT_SECONDS
end

local long_run_modules = {
    "metallurgy",
    "biology",
    "chemistry",
    "advanced_nuclear",
    "electronics",
}

local function run_long_run_checkpoint()
    local particles_before = particle_count()
    local checkpoint_stamp, save_ms = timed_save()
    local load_ms = timed_load(checkpoint_stamp)
    local particles_after = particle_count()
    assert(particles_after == particles_before,
        "long-run OPS cycle changed particle count: "
        .. tostring(particles_before) .. ">" .. tostring(particles_after))
    sim.deleteStamp(checkpoint_stamp)
    runtime.long_run_save_load_cycles = runtime.long_run_save_load_cycles + 1
    runtime.long_run_checkpoint_save_ms_total =
        runtime.long_run_checkpoint_save_ms_total + save_ms
    runtime.long_run_checkpoint_load_ms_total =
        runtime.long_run_checkpoint_load_ms_total + load_ms

    local target_language = runtime.long_run_language_switches % 2 == 0 and 0 or 1
    local actual_language, probe = sim.omniLanguage(target_language)
    assert(actual_language == target_language,
        "language switch did not apply in the running process")
    assert(type(probe) == "string" and #probe > 0,
        "language switch returned an empty translation probe")
    if runtime.language_probes[target_language] then
        assert(runtime.language_probes[target_language] == probe,
            "language translation probe changed during long run")
    else
        runtime.language_probes[target_language] = probe
    end
    if runtime.language_probes[0] and runtime.language_probes[1] then
        assert(runtime.language_probes[0] ~= runtime.language_probes[1],
            "English and Chinese translation probes are identical")
    end
    runtime.long_run_language_switches = runtime.long_run_language_switches + 1

    for _, module_name in ipairs(long_run_modules) do
        assert(sim.omniModuleEnabled(module_name, false) == false,
            "module did not disable: " .. module_name)
    end
    sim.updateUpTo()
    for _, module_name in ipairs(long_run_modules) do
        assert(sim.omniModuleEnabled(module_name, true) == true,
            "module did not re-enable: " .. module_name)
    end
    runtime.long_run_module_toggle_cycles =
        runtime.long_run_module_toggle_cycles + 1
end

local function write_failure(error_text)
    if runtime.series then
        runtime.series:close()
        runtime.series = nil
    end
    if runtime.atmosphere_series then
        runtime.atmosphere_series:close()
        runtime.atmosphere_series = nil
    end
    if runtime.callback_registered and tick_callback then
        event.unregister(event.tick, tick_callback)
        runtime.callback_registered = false
    end
    local result = assert(io.open(RESULT_FILE, "wb"))
    result:write("OMNI_STRESS_LUA_STATUS=FAIL\n")
    result:write("sample_id=" .. tostring(sample_id) .. "\n")
    result:write("error=" .. tostring(error_text):gsub("[\r\n]+", " | ") .. "\n")
    result:close()
end

local function begin_sample(now)
    runtime.phase = "sample"
    runtime.phase_started = now
    runtime.next_particle_sample = now + 1.0
    runtime.sample_frames = 0
    runtime.sample_frame_times = {}
    runtime.last_frame_started = nil
    runtime.sample_peak_particles = particle_count()
    runtime.signal_count_total = 0
    runtime.signal_count_peak_per_frame = 0
    if long_run then
        runtime.long_run_checkpoint_interval =
            sample_seconds / long_run_checkpoint_target
        runtime.next_long_run_checkpoint =
            now + runtime.long_run_checkpoint_interval
        runtime.next_atmosphere_sample = now
        runtime.atmosphere_series = assert(io.open(ATMOSPHERE_SERIES_FILE, "wb"))
        runtime.atmosphere_series:write(
            "elapsed_seconds,simulation_steps,particle_count,atmosphere_mass_kg,atmosphere_mass_residual_kg,species_mass_residual_abs_max_kg,species_n2_mass_kg,species_o2_mass_kg,species_ar_mass_kg,species_co2_mass_kg,species_h2o_mass_kg,condensed_water_mass_kg,non_finite_cells,state_non_finite_cells,minimum_density_kg_m3,maximum_density_kg_m3,minimum_pressure_pa,maximum_pressure_pa,minimum_temperature_k,maximum_temperature_k,nan_count,inf_count,stalls\n")
        runtime.atmosphere_series:flush()
        record_atmosphere_heartbeat(now, true)
    end
    runtime.series = assert(io.open(FRAME_SERIES_FILE, "wb"))
    runtime.series:write("elapsed_seconds,frames,particles\n")
    runtime.series:flush()
end

local function finish_sample(now)
    local sample_elapsed = now - runtime.phase_started
    local final_before_save = particle_count()
    runtime.sample_peak_particles = math.max(
        runtime.sample_peak_particles,
        final_before_save)
    runtime.series:write(string.format(
        "%.6f,%d,%d\n",
        sample_elapsed,
        runtime.sample_frames,
        final_before_save))
    runtime.series:close()
    runtime.series = nil
    if long_run then
        record_atmosphere_heartbeat(now, true)
        runtime.atmosphere_series:close()
        runtime.atmosphere_series = nil
    end

    assert(runtime.sample_frames > 0, "sample completed without simulation frames")
    if #runtime.sample_frame_times == 0 then
        runtime.sample_frame_times[1] = sample_elapsed / runtime.sample_frames
    end
    table.sort(runtime.sample_frame_times)
    local slow_index = math.max(
        1,
        math.ceil(#runtime.sample_frame_times * 0.99))
    local maximum_frame_time = runtime.sample_frame_times[
        #runtime.sample_frame_times]

    local long_run_settings_recovery_pass = true
    if long_run then
        assert(runtime.long_run_save_load_cycles == long_run_checkpoint_target,
            "long run did not complete the required OPS cycles")
        assert(runtime.long_run_language_switches == long_run_checkpoint_target,
            "long run did not complete the required language switches")
        assert(runtime.long_run_module_toggle_cycles == long_run_checkpoint_target,
            "long run did not complete the required module toggle cycles")
        local restored_language = sim.omniLanguage(runtime.initial_language)
        local final_language = sim.omniLanguage()
        long_run_settings_recovery_pass =
            restored_language == runtime.initial_language
            and final_language == runtime.initial_language
        for _, module_name in ipairs(long_run_modules) do
            long_run_settings_recovery_pass = long_run_settings_recovery_pass
                and sim.omniModuleEnabled(module_name) == true
        end
        assert(long_run_settings_recovery_pass,
            "long-run language or module settings were not restored")
        assert(runtime.heartbeat_count >= math.floor(sample_elapsed / 60.0) + 1,
            "formal long run did not persist heartbeats every 30-60 seconds")
        assert(runtime.stalls == 0,
            "formal long run observed a simulation stall")
        assert(runtime.nan_count == 0 and runtime.inf_count == 0,
            "formal long run observed non-finite diagnostics")
    end

    local recovery_marker_baseline = create_recovery_markers()
    local expected_recovered_particles = recovery_marker_baseline + #recovery_markers
    local second_stamp, save_time_second_ms = timed_save()
    local load_time_second_ms = timed_load(second_stamp)
    local after_second_load = particle_count()
    local roundtrip_pass = expected_recovered_particles == after_second_load
    assert(roundtrip_pass, "second immediate OPS reload changed particle count")
    local initial_recovery_assertions = verify_recovery_markers()

    event.unregister(event.tick, tick_callback)
    runtime.callback_registered = false
    local event_metrics = sim.omniEventMetrics()
    local event_count_total = assert(tonumber(event_metrics.total),
        "omni event total is unavailable")
    local event_count_peak_per_frame = assert(
        tonumber(event_metrics.peak_per_frame),
        "omni event peak is unavailable")
    assert(event_count_total >= 0 and event_count_peak_per_frame >= 0,
        "omni event metrics must be nonnegative")
    assert(runtime.signal_count_total >= 0
        and runtime.signal_count_peak_per_frame >= 0,
        "official signal metrics must be nonnegative: total="
        .. tostring(runtime.signal_count_total) .. ",peak="
        .. tostring(runtime.signal_count_peak_per_frame))

    sim.clearSim()
    for _ = 1, 4 do
        sim.updateUpTo()
    end
    local stopped_particles = particle_count()
    local stopped_signals = sim.elementCount(ids.spark)
    local stopped_metrics = sim.omniEventMetrics()
    local stop_event_delta = assert(tonumber(stopped_metrics.total),
        "omni event total disappeared after stop") - event_count_total
    local scenario_stop_pass = stopped_particles == 0 and stop_event_delta == 0
    local signal_stop_pass = stopped_signals == 0
    assert(scenario_stop_pass,
        "cleared stress scenario retained particles or produced omni events")
    assert(signal_stop_pass, "cleared stress scenario retained official signals")

    timed_load(second_stamp)
    local recovered_particles = particle_count()
    local scenario_recovery_pass = recovered_particles == after_second_load
        and verify_recovery_markers() == initial_recovery_assertions
    assert(scenario_recovery_pass,
        "stress scenario did not recover saved marker state after stop")

    write_success({
        sample_id = sample_id,
        initial_particles = runtime.initial_particles,
        peak_particles = math.max(
            runtime.warmup_peak_particles,
            runtime.sample_peak_particles),
        final_particles = final_before_save,
        warmup_frames = runtime.warmup_frames,
        sample_frames = runtime.sample_frames,
        actual_warmup_seconds = string.format(
            "%.6f", runtime.actual_warmup_seconds),
        actual_sample_seconds = string.format("%.6f", sample_elapsed),
        average_fps = string.format(
            "%.6f", runtime.sample_frames / sample_elapsed),
        one_percent_low_fps = string.format(
            "%.6f", 1.0 / runtime.sample_frame_times[slow_index]),
        minimum_fps = string.format("%.6f", 1.0 / maximum_frame_time),
        first_stamp = runtime.first_stamp,
        second_stamp = second_stamp,
        save_time_first_ms = string.format(
            "%.6f", runtime.save_time_first_ms),
        load_time_first_ms = string.format(
            "%.6f", runtime.load_time_first_ms),
        save_time_second_ms = string.format("%.6f", save_time_second_ms),
        load_time_second_ms = string.format("%.6f", load_time_second_ms),
        roundtrip_pass = "true",
        event_count_total = math.floor(event_count_total),
        event_count_peak_per_frame = math.floor(event_count_peak_per_frame),
        signal_count_total = math.floor(runtime.signal_count_total),
        signal_count_peak_per_frame = math.floor(runtime.signal_count_peak_per_frame),
        fixture_type_count = fixture_type_count,
        fixture_created_type_count = fixture_created_type_count,
        fixture_visible_type_count = fixture_visible_type_count,
        signal_stop_pass = tostring(signal_stop_pass),
        scenario_stop_pass = tostring(scenario_stop_pass),
        scenario_recovery_pass = tostring(scenario_recovery_pass),
        stop_event_delta = math.floor(stop_event_delta),
        scenario_recovery_assertions = initial_recovery_assertions,
        long_run = tostring(long_run),
        long_run_save_load_cycles = runtime.long_run_save_load_cycles,
        long_run_language_switches = runtime.long_run_language_switches,
        long_run_module_toggle_cycles = runtime.long_run_module_toggle_cycles,
        long_run_settings_recovery_pass =
            tostring(long_run_settings_recovery_pass),
        long_run_checkpoint_save_ms_total = string.format(
            "%.6f", runtime.long_run_checkpoint_save_ms_total),
        long_run_checkpoint_load_ms_total = string.format(
            "%.6f", runtime.long_run_checkpoint_load_ms_total),
        simulation_steps = runtime.sample_frames,
        heartbeat_count = runtime.heartbeat_count,
        heartbeat_interval_seconds = string.format(
            "%.6f", ATMOSPHERE_HEARTBEAT_SECONDS),
        maximum_heartbeat_gap_seconds = string.format(
            "%.6f", runtime.maximum_heartbeat_gap_seconds),
        stalls = runtime.stalls,
        nan_count = runtime.nan_count,
        inf_count = runtime.inf_count,
        omni_atmosphere_active = tostring(runtime.omni_atmosphere_active),
        atmosphere_mass_initial_kg = string.format(
            "%.17g", runtime.atmosphere_mass_initial_kg),
        atmosphere_mass_final_kg = string.format(
            "%.17g", runtime.atmosphere_mass_final_kg),
        atmosphere_mass_min_kg = string.format(
            "%.17g", runtime.atmosphere_mass_min_kg),
        atmosphere_mass_max_kg = string.format(
            "%.17g", runtime.atmosphere_mass_max_kg),
        atmosphere_mass_residual_abs_max_kg = string.format(
            "%.17g", runtime.atmosphere_mass_residual_abs_max_kg),
        species_mass_residual_abs_max_kg = string.format(
            "%.17g", runtime.species_mass_residual_abs_max_kg),
        minimum_density_kg_m3 = string.format(
            "%.17g", runtime.minimum_density_kg_m3),
        maximum_density_kg_m3 = string.format(
            "%.17g", runtime.maximum_density_kg_m3),
        minimum_pressure_pa = string.format(
            "%.17g", runtime.minimum_pressure_pa),
        maximum_pressure_pa = string.format(
            "%.17g", runtime.maximum_pressure_pa),
        minimum_temperature_k = string.format(
            "%.17g", runtime.minimum_temperature_k),
        maximum_temperature_k = string.format(
            "%.17g", runtime.maximum_temperature_k),
    })
    os.exit(0)
end

local function tick_once()
    local frame_started = socket.getTime()
    if runtime.phase == "warmup"
        and frame_started - runtime.phase_started >= warmup_seconds then
        local particles = particle_count()
        runtime.warmup_peak_particles = math.max(
            runtime.warmup_peak_particles,
            particles)
        runtime.actual_warmup_seconds = frame_started - runtime.phase_started
        begin_sample(frame_started)
    end

    if runtime.phase == "sample" and runtime.last_frame_started then
        local frame_gap = math.max(
            frame_started - runtime.last_frame_started, 0.000000001)
        runtime.sample_frame_times[#runtime.sample_frame_times + 1] = frame_gap
        if long_run and frame_gap > STALL_THRESHOLD_SECONDS then
            runtime.stalls = runtime.stalls + 1
        end
    end
    if runtime.phase == "sample" then
        runtime.last_frame_started = frame_started
    end

    sim.updateUpTo()
    if runtime.phase == "sample" then
        local active_signals = sim.elementCount(ids.spark)
        runtime.signal_count_total = runtime.signal_count_total + active_signals
        runtime.signal_count_peak_per_frame = math.max(
            runtime.signal_count_peak_per_frame,
            active_signals)
    end
    if runtime.phase == "warmup" then
        runtime.warmup_frames = runtime.warmup_frames + 1
    else
        runtime.sample_frames = runtime.sample_frames + 1
    end

    local now = socket.getTime()
    if runtime.phase == "sample" and long_run then
        while runtime.long_run_save_load_cycles < long_run_checkpoint_target
            and now >= runtime.next_long_run_checkpoint do
            run_long_run_checkpoint()
            runtime.next_long_run_checkpoint = runtime.next_long_run_checkpoint
                + runtime.long_run_checkpoint_interval
            now = socket.getTime()
        end
        record_atmosphere_heartbeat(now, false)
    end
    if now >= runtime.next_particle_sample then
        local particles = particle_count()
        if runtime.phase == "warmup" then
            runtime.warmup_peak_particles = math.max(
                runtime.warmup_peak_particles,
                particles)
        else
            runtime.sample_peak_particles = math.max(
                runtime.sample_peak_particles,
                particles)
            runtime.series:write(string.format(
                "%.6f,%d,%d\n",
                now - runtime.phase_started,
                runtime.sample_frames,
                particles))
            runtime.series:flush()
        end
        runtime.next_particle_sample = now + 1.0
    end

    if runtime.phase == "sample"
        and now - runtime.phase_started >= sample_seconds then
        finish_sample(now)
    end
end

tick_callback = function()
    local ok, error_text = xpcall(tick_once, debug.traceback)
    if not ok then
        write_failure(error_text)
        os.exit(1)
    end
end

local function start()
    local scenario = assert(scenarios[sample_id], "unknown sample_id: " .. sample_id)
    configure_simulation()
    scenario()
    runtime.initial_particles = particle_count()
    assert(runtime.initial_particles > 0, "scenario created no particles")

    runtime.first_stamp, runtime.save_time_first_ms = timed_save()
    runtime.load_time_first_ms = timed_load(runtime.first_stamp)
    local reloaded_particles = particle_count()
    assert(reloaded_particles == runtime.initial_particles,
        "first immediate OPS reload changed particle count: "
        .. tostring(runtime.initial_particles) .. ">" .. tostring(reloaded_particles))
    sim.resetOmniEventMetrics()

    runtime.initial_language, runtime.language_probes[1] = sim.omniLanguage()
    if long_run then
        assert(runtime.initial_language == 1,
            "isolated long-run profile did not start in Simplified Chinese")
        for _, module_name in ipairs(long_run_modules) do
            assert(sim.omniModuleEnabled(module_name) == true,
                "isolated long-run module did not start enabled: " .. module_name)
        end
    end

    local now = socket.getTime()
    runtime.warmup_frames = 0
    runtime.warmup_peak_particles = runtime.initial_particles
    runtime.actual_warmup_seconds = 0.0
    if warmup_seconds == 0 then
        begin_sample(now)
    else
        runtime.phase = "warmup"
        runtime.phase_started = now
        runtime.next_particle_sample = now + 1.0
    end
    event.register(event.tick, tick_callback)
    runtime.callback_registered = true
end

local ok, error_text = xpcall(start, debug.traceback)
if not ok then
    write_failure(error_text)
    os.exit(1)
end
