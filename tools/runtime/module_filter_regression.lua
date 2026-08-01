local helium = assert(elements.OMNI_PT_HE)
assert(helium == 370, "periodic helium stable ID changed: " .. tostring(helium))
ui.activeTool(0, "OMNI_PT_HE")
assert(ui.activeTool(0) == "OMNI_PT_HE",
    "always-available periodic content was blocked by a module gate")
local sodium = assert(elements.OMNI_PT_NA)
assert(sodium == 376, "periodic sodium stable ID changed: " .. tostring(sodium))
ui.activeTool(0, "OMNI_PT_NA")
assert(ui.activeTool(0) == "OMNI_PT_NA",
    "always-available alkali content was blocked by a module gate")
local calcium = assert(elements.OMNI_PT_CA)
assert(calcium == 381, "periodic calcium stable ID changed: " .. tostring(calcium))
ui.activeTool(0, "OMNI_PT_CA")
assert(ui.activeTool(0) == "OMNI_PT_CA",
    "always-available alkaline-earth content was blocked by a module gate")
local boron = assert(elements.OMNI_PT_B)
assert(boron == 372, "periodic boron stable ID changed: " .. tostring(boron))
ui.activeTool(0, "OMNI_PT_B")
assert(ui.activeTool(0) == "OMNI_PT_B",
    "always-available boron-group content was blocked by a module gate")
local germanium = assert(elements.OMNI_PT_GE)
assert(germanium == 386, "periodic germanium stable ID changed: " .. tostring(germanium))
ui.activeTool(0, "OMNI_PT_GE")
assert(ui.activeTool(0) == "OMNI_PT_GE",
    "always-available carbon-group content was blocked by a module gate")
local flerovium = assert(elements.OMNI_PT_FL)
assert(flerovium == 457, "periodic flerovium stable ID changed: " .. tostring(flerovium))
ui.activeTool(0, "OMNI_PT_FL")
assert(ui.activeTool(0) == "OMNI_PT_FL",
    "always-available superheavy carbon-group content was blocked by a module gate")
local nitrogen = assert(elements.OMNI_PT_N)
assert(nitrogen == 373, "periodic nitrogen stable ID changed: " .. tostring(nitrogen))
ui.activeTool(0, "OMNI_PT_N")
assert(ui.activeTool(0) == "OMNI_PT_N",
    "always-available nitrogen-group content was blocked by a module gate")
local moscovium = assert(elements.OMNI_PT_MC)
assert(moscovium == 458, "periodic moscovium stable ID changed: " .. tostring(moscovium))
ui.activeTool(0, "OMNI_PT_MC")
assert(ui.activeTool(0) == "OMNI_PT_MC",
    "always-available superheavy nitrogen-group content was blocked by a module gate")
local sulfur = assert(elements.OMNI_PT_S)
assert(sulfur == 378, "periodic sulfur stable ID changed: " .. tostring(sulfur))
ui.activeTool(0, "OMNI_PT_S")
assert(ui.activeTool(0) == "OMNI_PT_S",
    "always-available oxygen-group content was blocked by a module gate")
local livermorium = assert(elements.OMNI_PT_LV)
assert(livermorium == 459,
    "periodic livermorium stable ID changed: " .. tostring(livermorium))
ui.activeTool(0, "OMNI_PT_LV")
assert(ui.activeTool(0) == "OMNI_PT_LV",
    "always-available superheavy oxygen-group content was blocked by a module gate")
local fluorine = assert(elements.OMNI_PT_F)
assert(fluorine == 374, "periodic fluorine stable ID changed: " .. tostring(fluorine))
ui.activeTool(0, "OMNI_PT_F")
assert(ui.activeTool(0) == "OMNI_PT_F",
    "always-available halogen content was blocked by a module gate")
local tennessine = assert(elements.OMNI_PT_TS)
assert(tennessine == 460,
    "periodic tennessine stable ID changed: " .. tostring(tennessine))
ui.activeTool(0, "OMNI_PT_TS")
assert(ui.activeTool(0) == "OMNI_PT_TS",
    "always-available superheavy halogen content was blocked by a module gate")
local scandium = assert(elements.OMNI_PT_SC)
assert(scandium == 382,
    "periodic scandium stable ID changed: " .. tostring(scandium))
ui.activeTool(0, "OMNI_PT_SC")
assert(ui.activeTool(0) == "OMNI_PT_SC",
    "always-available first-transition content was blocked by a module gate")
local yttrium = assert(elements.OMNI_PT_Y)
assert(yttrium == 392,
    "periodic yttrium stable ID changed: " .. tostring(yttrium))
ui.activeTool(0, "OMNI_PT_Y")
assert(ui.activeTool(0) == "OMNI_PT_Y",
    "always-available second-transition content was blocked by a module gate")
local hafnium = assert(elements.OMNI_PT_HF)
assert(hafnium == 423,
    "periodic hafnium stable ID changed: " .. tostring(hafnium))
ui.activeTool(0, "OMNI_PT_HF")
assert(ui.activeTool(0) == "OMNI_PT_HF",
    "always-available third-transition content was blocked by a module gate")
local lanthanum = assert(elements.OMNI_PT_LA)
assert(lanthanum == 408,
    "periodic lanthanum stable ID changed: " .. tostring(lanthanum))
ui.activeTool(0, "OMNI_PT_LA")
assert(ui.activeTool(0) == "OMNI_PT_LA",
    "always-available lanthanide content was blocked by a module gate")
local actinium = assert(elements.OMNI_PT_AC)
assert(actinium == 434,
    "periodic actinium stable ID changed: " .. tostring(actinium))
ui.activeTool(0, "OMNI_PT_AC")
assert(ui.activeTool(0) == "OMNI_PT_AC",
    "always-available actinide content was blocked by a module gate")
local rutherfordium = assert(elements.OMNI_PT_RF)
assert(rutherfordium == 447,
    "periodic rutherfordium stable ID changed: " .. tostring(rutherfordium))
ui.activeTool(0, "OMNI_PT_RF")
assert(ui.activeTool(0) == "OMNI_PT_RF",
    "always-available superheavy content was blocked by a module gate")
local hydrochloric = assert(elements.OMNI_PT_HCLA)
assert(hydrochloric == 462,
    "inorganic hydrochloric acid stable ID changed: " .. tostring(hydrochloric))
ui.activeTool(0, "OMNI_PT_HCLA")
assert(ui.activeTool(0) == "OMNI_PT_HCLA",
    "enabled inorganic chemistry content was blocked by its module gate")
local carbonic = assert(elements.OMNI_PT_CARA)
assert(carbonic == 478,
    "second inorganic batch stable ID changed: " .. tostring(carbonic))
ui.activeTool(0, "OMNI_PT_CARA")
assert(ui.activeTool(0) == "OMNI_PT_CARA",
    "enabled second-batch inorganic content was blocked by its module gate")
local ammonium_chloride = assert(elements.OMNI_PT_AMCL)
assert(ammonium_chloride == 511,
    "third inorganic batch stable ID changed: " .. tostring(ammonium_chloride))
ui.activeTool(0, "OMNI_PT_AMCL")
assert(ui.activeTool(0) == "OMNI_PT_AMCL",
    "enabled third-batch inorganic content was blocked by its module gate")
local solder = assert(elements.OMNI_PT_SOLD)
assert(solder == 512,
    "engineering solder stable ID changed: " .. tostring(solder))
ui.activeTool(0, "OMNI_PT_SOLD")
assert(ui.activeTool(0) == "OMNI_PT_SOLD",
    "enabled high-ID engineering content was blocked by its metallurgy gate")

local id = elements.allocate("OMNITEST", "LUA1")
assert(id == 255, "expected first runtime Lua element in reserved slot 255, got " .. tostring(id))

elements.property(id, "Name", "LUAT")
elements.property(id, "Description", "OmniPack module-filter regression element")
elements.property(id, "MenuVisible", 1)
elements.property(id, "MenuSection", 8)
ui.activeTool(0, "OMNITEST_PT_LUA1")

local active = ui.activeTool(0)
assert(active == "OMNITEST_PT_LUA1", "module gate blocked runtime Lua element: " .. tostring(active))

local last_preferred = id
for index = 2, 60 do
    local runtime_id = string.format("L%03d", index)
    last_preferred = elements.allocate("OMNITEST", runtime_id)
    assert(last_preferred == 256 - index,
        "Lua one-byte buffer allocation drifted at index " .. index
            .. ": got " .. tostring(last_preferred))
end
assert(last_preferred == 196,
    "Lua one-byte compatibility buffer did not end at stable slot 196")

local high_id = elements.allocate("OMNITEST", "HIGH")
assert(high_id == 1023,
    "Lua allocator consumed an official hole or wrong high slot: "
        .. tostring(high_id))
elements.property(high_id, "Name", "HIGH")
elements.property(high_id, "Description", "High-ID Lua capacity regression element")
elements.property(high_id, "MenuVisible", 1)
elements.property(high_id, "MenuSection", 8)
ui.activeTool(0, "OMNITEST_PT_HIGH")
assert(ui.activeTool(0) == "OMNITEST_PT_HIGH",
    "module gate blocked a catalog-independent high-ID Lua element")
local high_particle = sim.partCreate(-1, 100, 100, high_id)
assert(high_particle >= 0 and sim.partProperty(high_particle, "type") == high_id,
    "high-ID Lua element could not be directly created")

local report = assert(io.open("lua-module-regression.result", "w"))
report:write("OMNI_LUA_ALLOC_ID=" .. id .. "\n")
report:write("OMNI_LUA_ACTIVE=" .. active .. "\n")
report:write("OMNI_LUA_PREFERRED_LAST=" .. last_preferred .. "\n")
report:write("OMNI_LUA_HIGH_ID=" .. high_id .. "\n")
report:write("OMNI_LUA_OFFICIAL_HOLE_PRESERVED=146\n")
report:write("OMNI_PERIODIC_ACTIVE=OMNI_PT_HE\n")
report:write("OMNI_PERIODIC_ALKALI_ACTIVE=OMNI_PT_NA\n")
report:write("OMNI_PERIODIC_ALKALINE_EARTH_ACTIVE=OMNI_PT_CA\n")
report:write("OMNI_PERIODIC_BORON_GROUP_ACTIVE=OMNI_PT_B\n")
report:write("OMNI_PERIODIC_CARBON_GROUP_ACTIVE=OMNI_PT_GE\n")
report:write("OMNI_PERIODIC_CARBON_SUPERHEAVY_ACTIVE=OMNI_PT_FL\n")
report:write("OMNI_PERIODIC_NITROGEN_GROUP_ACTIVE=OMNI_PT_N\n")
report:write("OMNI_PERIODIC_NITROGEN_SUPERHEAVY_ACTIVE=OMNI_PT_MC\n")
report:write("OMNI_PERIODIC_OXYGEN_GROUP_ACTIVE=OMNI_PT_S\n")
report:write("OMNI_PERIODIC_OXYGEN_SUPERHEAVY_ACTIVE=OMNI_PT_LV\n")
report:write("OMNI_PERIODIC_HALOGEN_ACTIVE=OMNI_PT_F\n")
report:write("OMNI_PERIODIC_HALOGEN_SUPERHEAVY_ACTIVE=OMNI_PT_TS\n")
report:write("OMNI_PERIODIC_FIRST_TRANSITION_ACTIVE=OMNI_PT_SC\n")
report:write("OMNI_PERIODIC_SECOND_TRANSITION_ACTIVE=OMNI_PT_Y\n")
report:write("OMNI_PERIODIC_THIRD_TRANSITION_ACTIVE=OMNI_PT_HF\n")
report:write("OMNI_PERIODIC_LANTHANIDE_ACTIVE=OMNI_PT_LA\n")
report:write("OMNI_PERIODIC_ACTINIDE_ACTIVE=OMNI_PT_AC\n")
report:write("OMNI_PERIODIC_SUPERHEAVY_ACTIVE=OMNI_PT_RF\n")
report:write("OMNI_INORGANIC_ACTIVE=OMNI_PT_HCLA\n")
report:write("OMNI_INORGANIC_BATCH2_ACTIVE=OMNI_PT_CARA\n")
report:write("OMNI_INORGANIC_BATCH3_ACTIVE=OMNI_PT_AMCL\n")
report:write("OMNI_ENGINEERING_HIGH_ID_ACTIVE=OMNI_PT_SOLD\n")
report:close()
