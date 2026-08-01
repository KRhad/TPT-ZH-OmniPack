local selectable = {
    { "OMNI_PT_HE", 370, "periodic helium" },
    { "OMNI_PT_NA", 376, "periodic sodium" },
    { "OMNI_PT_CA", 381, "periodic calcium" },
    { "OMNI_PT_B", 372, "periodic boron" },
    { "OMNI_PT_GE", 386, "periodic germanium" },
    { "OMNI_PT_FL", 457, "periodic flerovium" },
    { "OMNI_PT_N", 373, "periodic nitrogen" },
    { "OMNI_PT_MC", 458, "periodic moscovium" },
    { "OMNI_PT_S", 378, "periodic sulfur" },
    { "OMNI_PT_LV", 459, "periodic livermorium" },
    { "OMNI_PT_F", 374, "periodic fluorine" },
    { "OMNI_PT_TS", 460, "periodic tennessine" },
    { "OMNI_PT_SC", 382, "periodic scandium" },
    { "OMNI_PT_Y", 392, "periodic yttrium" },
    { "OMNI_PT_HF", 423, "periodic hafnium" },
    { "OMNI_PT_LA", 408, "periodic lanthanum" },
    { "OMNI_PT_AC", 434, "periodic actinium" },
    { "OMNI_PT_RF", 447, "periodic rutherfordium" },
    { "OMNI_PT_HCLA", 462, "hydrochloric acid" },
    { "OMNI_PT_CARA", 478, "carbonic acid" },
    { "OMNI_PT_AMCL", 511, "ammonium chloride" },
    { "OMNI_PT_NITI", 520, "engineering alloy" },
    { "OMNI_PT_RFBK", 532, "refractory material" },
    { "OMNI_PT_CF52", 588, "californium-252" },
    { "OMNI_PT_EACT", 621, "ethyl acetate" },
    { "OMNI_PT_DIEL", 641, "dielectric ceramic" },
}

for _, entry in ipairs(selectable) do
    local identifier, expected_id, label = entry[1], entry[2], entry[3]
    local element_id = assert(elements[identifier], "missing " .. label)
    assert(element_id == expected_id,
        label .. " stable ID changed: " .. tostring(element_id))
    ui.activeTool(0, identifier)
    assert(ui.activeTool(0) == identifier,
        label .. " was blocked by its enabled module")
end

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
report:write("OMNI_ENGINEERING_HIGH_ID_ACTIVE=OMNI_PT_NITI\n")
report:write("OMNI_MATERIALS_HIGH_ID_ACTIVE=OMNI_PT_RFBK\n")
report:write("OMNI_ISOTOPE_HIGH_ID_ACTIVE=OMNI_PT_CF52\n")
report:write("OMNI_ORGANIC_HIGH_ID_ACTIVE=OMNI_PT_EACT\n")
report:write("OMNI_ELECTRONICS_HIGH_ID_ACTIVE=OMNI_PT_DIEL\n")
report:close()
