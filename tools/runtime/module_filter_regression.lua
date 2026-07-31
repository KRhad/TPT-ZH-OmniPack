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
report:close()
