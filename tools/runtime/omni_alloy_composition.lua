local RESULT = "omni-alloy-composition.result"
local function test()
    assert(type(sim.omniAlloyComposition) == "function", "missing alloy composition API")
    local cases = {
        { elem.OMNI_PT_TIAL, { [elem.DEFAULT_PT_TTAN] = 4 / 6, [elem.OMNI_PT_ALUM] = 1 / 6, [elem.OMNI_PT_V] = 1 / 6 } },
        { elem.OMNI_PT_NSAL, { [elem.OMNI_PT_NICL] = 4 / 6, [elem.OMNI_PT_CHRM] = 1 / 6, [elem.OMNI_PT_COBT] = 1 / 6 } },
        { elem.OMNI_PT_WALY, { [elem.DEFAULT_PT_TUNG] = 4 / 6, [elem.OMNI_PT_NICL] = 1 / 6, [elem.DEFAULT_PT_IRON] = 1 / 6 } },
        { elem.OMNI_PT_ZRAL, { [elem.OMNI_PT_ZR] = 4 / 5, [elem.OMNI_PT_TIN] = 1 / 5 } },
        { elem.OMNI_PT_NITI, { [elem.OMNI_PT_NICL] = 1 / 2, [elem.DEFAULT_PT_TTAN] = 1 / 2 } },
    }
    for _, case in ipairs(cases) do
        local definition = assert(sim.omniAlloyComposition(case[1]), "missing alloy definition")
        assert(definition.element_type == case[1] and definition.status == "game_recipe_fraction")
        local total = 0
        for elementType, expected in pairs(case[2]) do
            local actual = assert(definition.composition[elementType], "missing alloy constituent")
            assert(math.abs(actual - expected) < 1.0e-12, "wrong alloy fraction")
            total = total + actual
        end
        assert(math.abs(total - 1.0) < 1.0e-12, "alloy fractions do not sum to one")
    end
    assert(sim.omniAlloyComposition(elem.DEFAULT_PT_WATR) == nil, "non-alloy returned a definition")
    return #cases
end
local ok, count = xpcall(test, debug.traceback)
local report = assert(io.open(RESULT, "w"))
if ok then
    report:write("OMNI_ALLOY_DEFINITIONS=", count, "\n")
    report:write("OMNI_ALLOY_COMPOSITION_STATUS=PASS\n")
else
    report:write("OMNI_ALLOY_ERROR=", tostring(count):gsub("[\r\n]+", " | "), "\n")
    report:write("OMNI_ALLOY_COMPOSITION_STATUS=FAIL\n")
end
report:close()
