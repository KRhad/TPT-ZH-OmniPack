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
report:close()
