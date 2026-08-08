local result_path = "lua-upstream-100-1-regression.result"

local function write_result(status, detail)
    local result = assert(io.open(result_path, "w"))
    result:write("UPSTREAM_100_1_STATUS=" .. status .. "\n")
    result:write("UPSTREAM_100_1_DETAIL=" .. tostring(detail) .. "\n")
    result:close()
end

local function expect_ok(label, callback)
    local ok, detail = pcall(callback)
    assert(ok, label .. " failed: " .. tostring(detail))
end

local function expect_error(label, expected, callback)
    local ok, detail = pcall(callback)
    assert(not ok, label .. " unexpectedly succeeded")
    assert(
        tostring(detail):find(expected, 1, true),
        label .. " returned the wrong error: " .. tostring(detail)
    )
end

local function run()
    expect_ok("resetPressure INT_MIN", function()
        sim.resetPressure(-2147483648, -2147483648, 1, 1)
    end)
    expect_ok("resetVelocity INT_MIN", function()
        sim.resetVelocity(-2147483648, -2147483648, 1, 1)
    end)
    expect_error("neighbors invalid position", "Invalid position", function()
        return sim.neighbors(-2147483648, 0, 1, 1)()
    end)
    expect_error("neighbors invalid radius", "Invalid radius", function()
        return sim.neighbors(100, 100, 1000000, 1)()
    end)

    write_result("PASS", "lua_bounds=4")
end

local ok, detail = pcall(run)
if not ok then
    write_result("FAIL", detail)
end
