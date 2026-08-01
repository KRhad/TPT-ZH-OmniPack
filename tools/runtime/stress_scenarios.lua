local CONFIG_FILE = "stress-scenario.config"
local RESULT_FILE = "stress-lua.result"
local FRAME_SERIES_FILE = "frame-series.csv"

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
assert(warmup_seconds >= 0 and sample_seconds > 0, "invalid duration")
assert(stride >= 3 and stride <= 24, "fixture_stride must be between 3 and 24")
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
}

local RECOVERABLE_SCRAP_MARKER = 0x4F4D5343

local function configure_simulation()
    sim.clearSim()
    sim.paused(true)
    sim.gravityMode(sim.GRAV_OFF)
    sim.airMode(sim.AIR_OFF)
    sim.ambientHeatSim(false)
    sim.heatSim(true)
    sim.ensureDeterminism(true)
    sim.randomSeed(11, 12, 13, 14)
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
    for _, marker in ipairs(recovery_markers) do
        local particle = assert(make(
            marker.particle_type, marker.x, marker.y, marker.properties),
            "failed to create recovery marker: " .. marker.name)
        assert(sim.partID(marker.x, marker.y) == particle,
            "recovery marker is not addressable: " .. marker.name)
    end
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
}

local function particle_count()
    local count = 0
    for _ in sim.parts() do
        count = count + 1
    end
    return count
end

local function timed_save()
    local started = socket.getTime()
    local stamp = sim.saveStamp(0, 0, sim.XRES - 1, sim.YRES - 1, 1)
    local elapsed_ms = (socket.getTime() - started) * 1000.0
    assert(type(stamp) == "string" and stamp:match("^[0-9A-Fa-f]+$") and #stamp == 10,
        "saveStamp did not return a ten-character stamp ID")
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
        "scenario_stop_pass", "scenario_recovery_pass", "stop_event_delta",
        "scenario_recovery_assertions",
    }) do
        result:write(key .. "=" .. tostring(data[key]) .. "\n")
    end
    result:close()
end

local runtime = {
    callback_registered = false,
    series = nil,
}
local tick_callback

local function write_failure(error_text)
    if runtime.series then
        runtime.series:close()
        runtime.series = nil
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

    create_recovery_markers()
    local expected_recovered_particles = final_before_save + #recovery_markers
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
        "official signal metrics must be nonnegative")

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
        signal_stop_pass = tostring(signal_stop_pass),
        scenario_stop_pass = tostring(scenario_stop_pass),
        scenario_recovery_pass = tostring(scenario_recovery_pass),
        stop_event_delta = math.floor(stop_event_delta),
        scenario_recovery_assertions = initial_recovery_assertions,
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
        runtime.sample_frame_times[#runtime.sample_frame_times + 1] = math.max(
            frame_started - runtime.last_frame_started,
            0.000000001)
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
    assert(particle_count() == runtime.initial_particles,
        "first immediate OPS reload changed particle count")
    sim.resetOmniEventMetrics()

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
