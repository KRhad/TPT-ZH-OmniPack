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

local id = elements.allocate("OMNITEST", "LUA1")
assert(id == 255, "expected first runtime Lua element in reserved slot 255, got " .. tostring(id))

elements.property(id, "Name", "LUAT")
elements.property(id, "Description", "OmniPack module-filter regression element")
elements.property(id, "MenuVisible", 1)
elements.property(id, "MenuSection", 8)
ui.activeTool(0, "OMNITEST_PT_LUA1")

local active = ui.activeTool(0)
assert(active == "OMNITEST_PT_LUA1", "module gate blocked runtime Lua element: " .. tostring(active))

local report = assert(io.open("lua-module-regression.result", "w"))
report:write("OMNI_LUA_ALLOC_ID=" .. id .. "\n")
report:write("OMNI_LUA_ACTIVE=" .. active .. "\n")
report:write("OMNI_PERIODIC_ACTIVE=OMNI_PT_HE\n")
report:write("OMNI_PERIODIC_ALKALI_ACTIVE=OMNI_PT_NA\n")
report:write("OMNI_PERIODIC_ALKALINE_EARTH_ACTIVE=OMNI_PT_CA\n")
report:write("OMNI_PERIODIC_BORON_GROUP_ACTIVE=OMNI_PT_B\n")
report:close()
