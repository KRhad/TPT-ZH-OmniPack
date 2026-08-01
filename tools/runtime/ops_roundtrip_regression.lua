local PHASE_FILE = "ops-roundtrip.phase"
local SCENARIO_FILE = "ops-roundtrip.scenario"
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

local scenario = read_all(SCENARIO_FILE):match("^%s*([%w_-]+)%s*$")
local valid_scenarios = {
    mixed = true,
    official = true,
    metallurgy = true,
    biology = true,
    chemistry = true,
    nuclear = true,
    periodic = true,
}
assert(valid_scenarios[scenario],
    "invalid OPS roundtrip scenario: " .. tostring(scenario))

local RESULT = "ops-roundtrip-phase" .. phase .. ".result"

local definitions = {
    dust = { "DEFAULT_PT_DUST", "DUST", 1 },
    water = { "DEFAULT_PT_WATR", "WATR", 2 },
    lava = { "DEFAULT_PT_LAVA", "LAVA", 6 },
    spark = { "DEFAULT_PT_SPRK", "SPRK", 15 },
    conv = { "DEFAULT_PT_CONV", "CONV", 85 },
    virs = { "DEFAULT_PT_VIRS", "VIRS", 174 },
    brmt = { "DEFAULT_PT_BRMT", "BRMT", 30 },

    hydrogen = { "DEFAULT_PT_H2", "HYGN", 148 },
    lithium = { "DEFAULT_PT_LITH", "LITH", 191 },
    carbon = { "DEFAULT_PT_DMND", "DMND", 28 },
    oxygen = { "DEFAULT_PT_O2", "OXYG", 61 },
    silicon = { "DEFAULT_PT_SLCN", "SLCN", 187 },
    titanium = { "DEFAULT_PT_TTAN", "TTAN", 144 },
    iron = { "DEFAULT_PT_IRON", "IRON", 76 },
    rubidium = { "DEFAULT_PT_RBDM", "RBDM", 41 },
    tungsten = { "DEFAULT_PT_TUNG", "TUNG", 171 },
    platinum = { "DEFAULT_PT_PTNM", "PTNM", 188 },
    gold = { "DEFAULT_PT_GOLD", "GOLD", 170 },
    mercury = { "DEFAULT_PT_MERC", "MERC", 152 },
    polonium = { "DEFAULT_PT_POLO", "POLO", 182 },

    alum = { "OMNI_PT_ALUM", "ALUM", 256 },
    copr = { "OMNI_PT_COPR", "COPR", 257 },
    lead = { "OMNI_PT_LEAD", "LEAD", 258 },
    tin = { "OMNI_PT_TIN", "TIN", 259 },
    nicl = { "OMNI_PT_NICL", "NICL", 260 },
    magn = { "OMNI_PT_MAGN", "MAGN", 261 },
    chrm = { "OMNI_PT_CHRM", "CHRM", 262 },
    cobt = { "OMNI_PT_COBT", "COBT", 263 },
    moly = { "OMNI_PT_MOLY", "MOLY", 264 },
    zinc = { "OMNI_PT_ZINC", "ZINC", 265 },
    chrc = { "OMNI_PT_CHRC", "CHRC", 266 },
    coke = { "OMNI_PT_COKE", "COKE", 267 },
    stel = { "OMNI_PT_STEL", "STEL", 268 },
    brnz = { "OMNI_PT_BRNZ", "BRNZ", 269 },
    bras = { "OMNI_PT_BRAS", "BRAS", 270 },
    ssil = { "OMNI_PT_SSIL", "SSIL", 271 },
    ncrm = { "OMNI_PT_NCRM", "NCRM", 272 },
    almg = { "OMNI_PT_ALMG", "ALMG", 273 },
    tstl = { "OMNI_PT_TSTL", "TSTL", 274 },
    slag = { "OMNI_PT_SLAG", "SLAG", 275 },
    flux = { "OMNI_PT_FLUX", "FLUX", 276 },
    cruc = { "OMNI_PT_CRUC", "CRUC", 277 },

    nutr = { "OMNI_PT_NUTR", "NUTR", 288 },
    alga = { "OMNI_PT_ALGA", "ALGA", 289 },
    mycl = { "OMNI_PT_MYCL", "MYCL", 290 },
    spor = { "OMNI_PT_SPOR", "SPOR", 291 },
    path = { "OMNI_PT_PATH", "PATH", 292 },
    ster = { "OMNI_PT_STER", "STER", 293 },
    hums = { "OMNI_PT_HUMS", "HUMS", 294 },
    biof = { "OMNI_PT_BIOF", "BIOF", 295 },

    nful = { "OMNI_PT_NFUL", "NFUL", 328 },
    modr = { "OMNI_PT_MODR", "MODR", 329 },
    crod = { "OMNI_PT_CROD", "CROD", 330 },
    nclt = { "OMNI_PT_NCLT", "NCLT", 331 },
    nwst = { "OMNI_PT_NWST", "NWST", 332 },
    ngen = { "OMNI_PT_NGEN", "NGEN", 333 },
    rshd = { "OMNI_PT_RSHD", "RSHD", 334 },

    chlr = { "OMNI_PT_CHLR", "CHLR", 360 },
    amon = { "OMNI_PT_AMON", "AMON", 361 },
    ethl = { "OMNI_PT_ETHL", "ETHL", 362 },
    kero = { "OMNI_PT_KERO", "KERO", 363 },
    gaso = { "OMNI_PT_GASO", "GASO", 364 },
    acty = { "OMNI_PT_ACTY", "ACTY", 365 },
    cata = { "OMNI_PT_CATA", "CATA", 366 },
    poly = { "OMNI_PT_POLY", "POLY", 367 },
    pero = { "OMNI_PT_PERO", "PERO", 368 },
    fert = { "OMNI_PT_FERT", "FERT", 369 },

    he = { "OMNI_PT_HE", "HE", 370 },
    sodium = { "OMNI_PT_NA", "NA", 376 },
    ne = { "OMNI_PT_NE", "NE", 375 },
    ar = { "OMNI_PT_AR", "AR", 379 },
    potassium = { "OMNI_PT_K", "K", 380 },
    kr = { "OMNI_PT_KR", "KR", 390 },
    xe = { "OMNI_PT_XE", "XE", 405 },
    caesium = { "OMNI_PT_CS", "CS", 406 },
    rn = { "OMNI_PT_RN", "RN", 431 },
    francium = { "OMNI_PT_FR", "FR", 432 },
    beryllium = { "OMNI_PT_BE", "BE", 371 },
    calcium = { "OMNI_PT_CA", "CA", 381 },
    strontium = { "OMNI_PT_SR", "SR", 391 },
    barium = { "OMNI_PT_BA", "BA", 407 },
    radium = { "OMNI_PT_RA", "RA", 433 },
    boron = { "OMNI_PT_B", "B", 372 },
    gallium = { "OMNI_PT_GA", "GA", 385 },
    indium = { "OMNI_PT_IN", "IN", 401 },
    thallium = { "OMNI_PT_TL", "TL", 428 },
    nihonium = { "OMNI_PT_NH", "NH", 456 },
    germanium = { "OMNI_PT_GE", "GE", 386 },
    flerovium = { "OMNI_PT_FL", "FL", 457 },
    nitrogen = { "OMNI_PT_N", "N", 373 },
    phosphorus = { "OMNI_PT_P", "P", 377 },
    arsenic = { "OMNI_PT_AS", "AS", 387 },
    antimony = { "OMNI_PT_SB", "SB", 402 },
    bismuth = { "OMNI_PT_BI", "BI", 429 },
    moscovium = { "OMNI_PT_MC", "MC", 458 },
    sulfur = { "OMNI_PT_S", "S", 378 },
    selenium = { "OMNI_PT_SE", "SE", 388 },
    tellurium = { "OMNI_PT_TE", "TE", 403 },
    livermorium = { "OMNI_PT_LV", "LV", 459 },
    fluorine = { "OMNI_PT_F", "F", 374 },
    bromine = { "OMNI_PT_BR", "BR", 389 },
    iodine = { "OMNI_PT_I", "I", 404 },
    astatine = { "OMNI_PT_AT", "AT", 430 },
    tennessine = { "OMNI_PT_TS", "TS", 460 },
    scandium = { "OMNI_PT_SC", "SC", 382 },
    vanadium = { "OMNI_PT_V", "V", 383 },
    manganese = { "OMNI_PT_MN", "MN", 384 },
    yttrium = { "OMNI_PT_Y", "Y", 392 },
    zirconium = { "OMNI_PT_ZR", "ZR", 393 },
    niobium = { "OMNI_PT_NB", "NB", 394 },
    technetium = { "OMNI_PT_TC", "TC", 395 },
    ruthenium = { "OMNI_PT_RU", "RU", 396 },
    rhodium = { "OMNI_PT_RH", "RH", 397 },
    palladium = { "OMNI_PT_PD", "PD", 398 },
    silver = { "OMNI_PT_AG", "AG", 399 },
    cadmium = { "OMNI_PT_CD", "CD", 400 },
    lanthanum = { "OMNI_PT_LA", "LA", 408 },
    cerium = { "OMNI_PT_CE", "CE", 409 },
    praseodymium = { "OMNI_PT_PR", "PR", 410 },
    neodymium = { "OMNI_PT_ND", "ND", 411 },
    promethium = { "OMNI_PT_PM", "PM", 412 },
    samarium = { "OMNI_PT_SM", "SM", 413 },
    europium = { "OMNI_PT_EU", "EU", 414 },
    gadolinium = { "OMNI_PT_GD", "GD", 415 },
    terbium = { "OMNI_PT_TB", "TB", 416 },
    dysprosium = { "OMNI_PT_DY", "DY", 417 },
    holmium = { "OMNI_PT_HO", "HO", 418 },
    erbium = { "OMNI_PT_ER", "ER", 419 },
    thulium = { "OMNI_PT_TM", "TM", 420 },
    ytterbium = { "OMNI_PT_YB", "YB", 421 },
    lutetium = { "OMNI_PT_LU", "LU", 422 },
    actinium = { "OMNI_PT_AC", "AC", 434 },
    thorium = { "OMNI_PT_TH", "TH", 435 },
    protactinium = { "OMNI_PT_PA", "PA", 436 },
    uranium = { "DEFAULT_PT_URAN", "URAN", 32 },
    neptunium = { "OMNI_PT_NP", "NP", 437 },
    plutonium = { "DEFAULT_PT_PLUT", "PLUT", 19 },
    americium = { "OMNI_PT_AM", "AM", 438 },
    curium = { "OMNI_PT_CM", "CM", 439 },
    berkelium = { "OMNI_PT_BK", "BK", 440 },
    californium = { "OMNI_PT_CF", "CF", 441 },
    einsteinium = { "OMNI_PT_ES", "ES", 442 },
    fermium = { "OMNI_PT_FM", "FM", 443 },
    mendelevium = { "OMNI_PT_MD", "MD", 444 },
    nobelium = { "OMNI_PT_NO", "NO", 445 },
    lawrencium = { "OMNI_PT_LR", "LR", 446 },
    rutherfordium = { "OMNI_PT_RF", "RF", 447 },
    dubnium = { "OMNI_PT_DB", "DB", 448 },
    seaborgium = { "OMNI_PT_SG", "SG", 449 },
    bohrium = { "OMNI_PT_BH", "BH", 450 },
    hassium = { "OMNI_PT_HS", "HS", 451 },
    meitnerium = { "OMNI_PT_MT", "MT", 452 },
    darmstadtium = { "OMNI_PT_DS", "DS", 453 },
    roentgenium = { "OMNI_PT_RG", "RG", 454 },
    copernicium = { "OMNI_PT_CN", "CN", 455 },
    hydrochloric = { "OMNI_PT_HCLA", "HCLA", 462 },
    sulfuric = { "OMNI_PT_SULA", "SULA", 463 },
    nitric = { "OMNI_PT_NITA", "NITA", 464 },
    phosphoric = { "OMNI_PT_PHOA", "PHOA", 465 },
    hydrofluoric = { "OMNI_PT_HYFA", "HYFA", 466 },
    sodium_hydroxide = { "OMNI_PT_NAOH", "NAOH", 467 },
    potassium_hydroxide = { "OMNI_PT_KOH", "KOH", 468 },
    calcium_hydroxide = { "OMNI_PT_CAOH", "CAOH", 469 },
    potassium_nitrate = { "OMNI_PT_KNIT", "KNIT", 470 },
    copper_sulfate = { "OMNI_PT_CUSF", "CUSF", 471 },
    calcium_carbonate = { "OMNI_PT_CACO", "CACO", 472 },
    sodium_bicarbonate = { "OMNI_PT_NABC", "NABC", 473 },
    carbon_monoxide = { "OMNI_PT_COMO", "COMO", 474 },
    sulfur_dioxide = { "OMNI_PT_SODI", "SODI", 475 },
    nitrogen_dioxide = { "OMNI_PT_NODI", "NODI", 476 },
    calcium_oxide = { "OMNI_PT_CAOX", "CAOX", 477 },
    carbonic_acid = { "OMNI_PT_CARA", "CARA", 478 },
    hydrogen_sulfide = { "OMNI_PT_H2SG", "H2SG", 479 },
    ammonia_water = { "OMNI_PT_AMWA", "AMWA", 480 },
    barium_hydroxide = { "OMNI_PT_BAOH", "BAOH", 481 },
    potassium_chloride = { "OMNI_PT_KCL", "KCL", 482 },
    calcium_chloride = { "OMNI_PT_CACL", "CACL", 483 },
    iron_chloride = { "OMNI_PT_FECL", "FECL", 484 },
    sodium_sulfate = { "OMNI_PT_NASF", "NASF", 485 },
    ammonium_nitrate = { "OMNI_PT_AMNT", "AMNT", 486 },
    sodium_carbonate = { "OMNI_PT_NACO", "NACO", 487 },
    potassium_permanganate = { "OMNI_PT_KPER", "KPER", 488 },
    aluminium_oxide = { "OMNI_PT_ALOX", "ALOX", 489 },
    magnesium_oxide = { "OMNI_PT_MGOX", "MGOX", 490 },
    iron_oxide = { "OMNI_PT_FEOX", "FEOX", 491 },
    copper_oxide = { "OMNI_PT_CUOX", "CUOX", 492 },
    zinc_oxide = { "OMNI_PT_ZNOX", "ZNOX", 493 },
    sulfur_trioxide = { "OMNI_PT_SUTR", "SUTR", 494 },
    nitric_oxide = { "OMNI_PT_NIMO", "NIMO", 495 },
    titanium_dioxide = { "OMNI_PT_TIOX", "TIOX", 496 },
    uranium_oxide = { "OMNI_PT_UROX", "UROX", 497 },
    calcium_phosphate = { "OMNI_PT_CAPH", "CAPH", 498 },
    iron_sulfide = { "OMNI_PT_FESF", "FESF", 499 },
    sodium_sulfide = { "OMNI_PT_NASD", "NASD", 500 },
    hydrogen_cyanide = { "OMNI_PT_HYCN", "HYCN", 501 },
    calcium_carbide = { "OMNI_PT_CACB", "CACB", 502 },
    silicon_carbide = { "OMNI_PT_SICB", "SICB", 503 },
    boron_nitride = { "OMNI_PT_BORN", "BORN", 504 },
    silicon_nitride = { "OMNI_PT_SINT", "SINT", 505 },
    sodium_hydride = { "OMNI_PT_NAHY", "NAHY", 506 },
    calcium_hydride = { "OMNI_PT_CAHY", "CAHY", 507 },
    aluminium_chloride = { "OMNI_PT_ALCL", "ALCL", 508 },
    magnesium_chloride = { "OMNI_PT_MGCL", "MGCL", 509 },
    copper_chloride = { "OMNI_PT_CUCL", "CUCL", 510 },
    ammonium_chloride = { "OMNI_PT_AMCL", "AMCL", 511 },
    solder = { "OMNI_PT_SOLD", "SOLD", 512 },
    cast_iron = { "OMNI_PT_CSTI", "CSTI", 513 },
    cupronickel = { "OMNI_PT_CUNI", "CUNI", 514 },
    titanium_alloy = { "OMNI_PT_TIAL", "TIAL", 515 },
    nickel_superalloy = { "OMNI_PT_NSAL", "NSAL", 516 },
    tungsten_heavy_alloy = { "OMNI_PT_WALY", "WALY", 517 },
    zirconium_alloy = { "OMNI_PT_ZRAL", "ZRAL", 518 },
    constantan = { "OMNI_PT_CNST", "CNST", 519 },
    nitinol = { "OMNI_PT_NITI", "NITI", 520 },
    hafnium = { "OMNI_PT_HF", "HF", 423 },
    tantalum = { "OMNI_PT_TA", "TA", 424 },
    rhenium = { "OMNI_PT_RE", "RE", 425 },
    osmium = { "OMNI_PT_OS", "OS", 426 },
    iridium = { "OMNI_PT_IR", "IR", 427 },
    og = { "OMNI_PT_OG", "OG", 461 },
}

local module_keys = {
    metallurgy = {
        "alum", "copr", "lead", "tin", "nicl", "magn", "chrm",
        "cobt", "moly", "zinc", "chrc", "coke", "stel", "brnz",
        "bras", "ssil", "ncrm", "almg", "tstl", "slag", "flux",
        "cruc",
        "solder",
        "cast_iron", "cupronickel", "titanium_alloy",
        "nickel_superalloy", "tungsten_heavy_alloy", "zirconium_alloy",
        "constantan", "nitinol",
    },
    biology = {
        "nutr", "alga", "mycl", "spor", "path", "ster", "hums",
        "biof",
    },
    nuclear = {
        "nful", "modr", "crod", "nclt", "nwst", "ngen", "rshd",
    },
    chemistry = {
        "chlr", "amon", "ethl", "kero", "gaso", "acty", "cata",
        "poly", "pero", "fert", "hydrochloric", "sulfuric", "nitric",
        "phosphoric", "hydrofluoric", "sodium_hydroxide",
        "potassium_hydroxide", "calcium_hydroxide", "potassium_nitrate",
        "copper_sulfate", "calcium_carbonate", "sodium_bicarbonate",
        "carbon_monoxide", "sulfur_dioxide", "nitrogen_dioxide",
        "calcium_oxide", "carbonic_acid", "hydrogen_sulfide",
        "ammonia_water", "barium_hydroxide", "potassium_chloride",
        "calcium_chloride", "iron_chloride", "sodium_sulfate",
        "ammonium_nitrate", "sodium_carbonate", "potassium_permanganate",
        "aluminium_oxide", "magnesium_oxide", "iron_oxide",
        "copper_oxide", "zinc_oxide", "sulfur_trioxide", "nitric_oxide",
        "titanium_dioxide", "uranium_oxide", "calcium_phosphate",
        "iron_sulfide", "sodium_sulfide", "hydrogen_cyanide",
        "calcium_carbide", "silicon_carbide", "boron_nitride",
        "silicon_nitride", "sodium_hydride", "calcium_hydride",
        "aluminium_chloride", "magnesium_chloride", "copper_chloride",
        "ammonium_chloride",
    },
    periodic = {
        "hydrogen", "lithium", "carbon", "oxygen", "silicon", "titanium",
        "iron", "rubidium", "tungsten", "platinum", "gold", "mercury",
        "polonium", "magn", "alum", "chlr", "chrm", "cobt", "nicl",
        "copr", "zinc", "moly", "tin", "lead",
        "he", "ne", "ar", "sodium", "potassium", "kr", "xe",
        "caesium", "rn", "francium", "og", "beryllium", "calcium",
        "strontium", "barium", "radium",
        "boron", "gallium", "indium", "thallium", "nihonium",
        "germanium", "flerovium",
        "nitrogen", "phosphorus", "arsenic", "antimony", "bismuth",
        "moscovium", "sulfur", "selenium", "tellurium", "livermorium",
        "fluorine", "bromine", "iodine", "astatine", "tennessine",
        "scandium", "vanadium", "manganese",
        "yttrium", "zirconium", "niobium", "technetium",
        "ruthenium", "rhodium", "palladium", "silver", "cadmium",
        "lanthanum", "cerium", "praseodymium", "neodymium",
        "promethium", "samarium", "europium", "gadolinium",
        "terbium", "dysprosium", "holmium", "erbium", "thulium",
        "ytterbium", "lutetium",
        "actinium", "thorium", "protactinium", "uranium", "neptunium",
        "plutonium", "americium", "curium", "berkelium", "californium",
        "einsteinium", "fermium", "mendelevium", "nobelium", "lawrencium",
        "rutherfordium", "dubnium", "seaborgium", "bohrium", "hassium",
        "meitnerium", "darmstadtium", "roentgenium", "copernicium",
        "hafnium", "tantalum", "rhenium", "osmium", "iridium",
    },
}

local official_keys = {
    "dust", "water", "lava", "spark", "conv", "virs", "brmt",
}
local mixed_keys = { "alum", "nutr", "nful", "chlr" }
local RECOVERABLE_SCRAP_MARKER = 0x4F4D5343

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

local ids = {}
local validated_keys = {}

local function validate_key(key)
    if ids[key] then
        return
    end
    local definition = assert(definitions[key], "missing definition for " .. key)
    ids[key] = must_element(definition[1], definition[2], definition[3])
    validated_keys[#validated_keys + 1] = key
end

for _, key in ipairs(official_keys) do
    validate_key(key)
end
if scenario == "mixed" then
    for _, key in ipairs(mixed_keys) do
        validate_key(key)
    end
elseif scenario ~= "official" then
    for _, key in ipairs(module_keys[scenario]) do
        validate_key(key)
    end
end

local fixtures = {}
local required_palette = {}
local direct_gt255 = 0
local direct_elements = 0

local function require_palette(key)
    local definition = assert(definitions[key], "missing definition for " .. key)
    required_palette[definition[1]] = true
end

local function add_fixture(name, particle_key, property_specs, create_key)
    local fixture_index = #fixtures
    local fixture = {
        name = name,
        x = 64 + (fixture_index % 60) * 8,
        y = 64 + math.floor(fixture_index / 60) * 8,
        particle_type = assert(ids[particle_key],
            "unvalidated fixture particle type: " .. particle_key),
        properties = {},
    }
    require_palette(particle_key)
    if create_key then
        fixture.create_type = assert(ids[create_key],
            "unvalidated fixture create type: " .. create_key)
    end
    for _, property_spec in ipairs(property_specs or {}) do
        local value = property_spec.value
        if property_spec.element then
            value = assert(ids[property_spec.element],
                "unvalidated carried element: " .. property_spec.element)
            require_palette(property_spec.element)
        end
        fixture.properties[property_spec.name] = value
    end
    fixtures[#fixtures + 1] = fixture
end

local function add_direct_module_fixture(key)
    local is_gt255 = ids[key] > 255
    add_fixture("direct_" .. key .. (is_gt255 and "_gt255" or "_reused"), key)
    direct_elements = direct_elements + 1
    if is_gt255 then direct_gt255 = direct_gt255 + 1 end
end

add_fixture("official_dust", "dust", {
    { name = "life", value = 123 },
})
add_fixture("official_water", "water", {
    { name = "tmp", value = 17 },
})

if scenario == "mixed" then
    add_direct_module_fixture("alum")
    add_direct_module_fixture("nutr")
    add_direct_module_fixture("nful")
    add_direct_module_fixture("chlr")
else
    for _, key in ipairs(module_keys[scenario] or {}) do
        add_direct_module_fixture(key)
    end
end

local carrier_targets = {
    official = {
        lava = "dust",
        spark = "water",
        conv_ctype = "dust",
        conv_tmp = "water",
        virs_tmp2 = "dust",
    },
    metallurgy = {
        lava = "nitinol",
        spark = "nitinol",
        scrap = "nitinol",
        conv_ctype = "nitinol",
        conv_tmp = "nitinol",
        virs_tmp2 = "nitinol",
    },
    biology = {
        lava = "hums",
        spark = "biof",
        conv_ctype = "nutr",
        conv_tmp = "alga",
        virs_tmp2 = "path",
    },
    chemistry = {
        lava = "ammonium_chloride",
        spark = "copper_chloride",
        conv_ctype = "sulfur_trioxide",
        conv_tmp = "hydrogen_cyanide",
        virs_tmp2 = "sodium_hydride",
    },
    nuclear = {
        lava = "nful",
        spark = "ngen",
        conv_ctype = "crod",
        conv_tmp = "nwst",
        virs_tmp2 = "rshd",
    },
    periodic = {
        lava = "copernicium",
        spark = "roentgenium",
        scrap = "rutherfordium",
        conv_ctype = "dubnium",
        conv_tmp = "hassium",
        virs_tmp2 = "darmstadtium",
    },
    mixed = {
        lava = "alum",
        spark = "nful",
        scrap = "alum",
        conv_ctype = "nful",
        conv_tmp = "nutr",
        virs_tmp2 = "chlr",
    },
}

local targets = carrier_targets[scenario]
add_fixture("lava_ctype_gt255", "lava", {
    { name = "ctype", element = targets.lava },
})
add_fixture("spark_ctype_gt255", "spark", {
    { name = "ctype", element = targets.spark },
    { name = "life", value = 4 },
}, targets.spark)
if targets.scrap then
    add_fixture("brmt_recoverable_ctype_gt255", "brmt", {
        { name = "ctype", element = targets.scrap },
        { name = "tmp4", value = RECOVERABLE_SCRAP_MARKER },
    })
end
add_fixture("conv_ctype_tmp_gt255", "conv", {
    { name = "ctype", element = targets.conv_ctype },
    { name = "tmp", element = targets.conv_tmp },
})
add_fixture("virs_tmp2_gt255", "virs", {
    { name = "tmp2", element = targets.virs_tmp2 },
})

local required_palette_identifiers = {}
for identifier in pairs(required_palette) do
    required_palette_identifiers[#required_palette_identifiers + 1] = identifier
end
table.sort(required_palette_identifiers)

table.sort(validated_keys, function(left, right)
    return definitions[left][1] < definitions[right][1]
end)
local stable_identifier_entries = {}
for _, key in ipairs(validated_keys) do
    stable_identifier_entries[#stable_identifier_entries + 1] =
        definitions[key][1] .. ":" .. ids[key]
end

local ctype_carriers = targets.scrap and "LAVA,SPRK,BRMT,CONV"
    or "LAVA,SPRK,CONV"
local tmp_carriers = "CONV"
local tmp2_carriers = "VIRS"

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
    report:write("OMNI_OPS_SCENARIO=" .. scenario .. "\n")
    report:write("OMNI_OPS_PHASE=" .. phase .. "\n")
    report:write("OMNI_OPS_OPERATION=" .. data.operation .. "\n")
    report:write("OMNI_OPS_STAMP=" .. data.stamp .. "\n")
    if data.source_stamp then
        report:write("OMNI_OPS_SOURCE_STAMP=" .. data.source_stamp .. "\n")
    end
    report:write("OMNI_OPS_PARTICLES=" .. data.particle_count .. "\n")
    report:write("OMNI_OPS_FIELD_ASSERTIONS=" .. data.assertions .. "\n")
    report:write("OMNI_OPS_DIRECT_GT255=" .. direct_gt255 .. "\n")
    report:write("OMNI_OPS_DIRECT_ELEMENTS=" .. direct_elements .. "\n")
    report:write("OMNI_OPS_CTYPE_CARRIERS=" .. ctype_carriers .. "\n")
    report:write("OMNI_OPS_TMP_CARRIERS=" .. tmp_carriers .. "\n")
    report:write("OMNI_OPS_TMP2_CARRIERS=" .. tmp2_carriers .. "\n")
    report:write("OMNI_OPS_STABLE_IDENTIFIER_COUNT="
        .. #stable_identifier_entries .. "\n")
    report:write("OMNI_OPS_STABLE_IDENTIFIERS="
        .. table.concat(stable_identifier_entries, ";") .. "\n")
    if data.file_size then
        report:write("OMNI_OPS_FILE_SIZE=" .. data.file_size .. "\n")
        report:write("OMNI_OPS_PAYLOAD_SIZE=" .. data.payload_size .. "\n")
        report:write(
            "OMNI_OPS_PALETTE_IDENTIFIERS=" .. data.palette_identifiers .. "\n")
    end
else
    local error_text = tostring(data):gsub("[\r\n]+", " | ")
    report:write("OMNI_OPS_STATUS=FAIL\n")
    report:write("OMNI_OPS_SCENARIO=" .. scenario .. "\n")
    report:write("OMNI_OPS_PHASE=" .. phase .. "\n")
    report:write("OMNI_OPS_ERROR=" .. error_text .. "\n")
end
report:close()

os.exit(ok and 0 or 1)
