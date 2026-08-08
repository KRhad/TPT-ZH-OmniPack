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

local function expect_velocity(x, y, expected_x, expected_y, label)
    assert(
        sim.velocityX(x, y) == expected_x,
        label .. " returned the wrong X velocity"
    )
    assert(
        sim.velocityY(x, y) == expected_y,
        label .. " returned the wrong Y velocity"
    )
end

local function set_velocity(x, y)
    sim.velocityX(x, y, 12.5)
    sim.velocityY(x, y, -7.25)
end

local function check_reset_velocity_bounds()
    local last_x = sim.XCELLS - 1
    local last_y = sim.YCELLS - 1
    local middle_x = math.floor(sim.XCELLS / 2)
    local middle_y = math.floor(sim.YCELLS / 2)

    set_velocity(0, 0)
    set_velocity(last_x, 0)
    set_velocity(0, last_y)
    set_velocity(last_x, last_y)
    sim.resetVelocity()
    expect_velocity(0, 0, 0, 0, "resetVelocity full grid first cell")
    expect_velocity(last_x, 0, 0, 0, "resetVelocity full grid last column")
    expect_velocity(0, last_y, 0, 0, "resetVelocity full grid last row")
    expect_velocity(last_x, last_y, 0, 0, "resetVelocity full grid last cell")

    set_velocity(last_x, middle_y)
    sim.resetVelocity(last_x, 0, 1, sim.YCELLS)
    expect_velocity(last_x, middle_y, 0, 0, "resetVelocity explicit last column")

    set_velocity(middle_x, last_y)
    sim.resetVelocity(0, last_y, sim.XCELLS, 1)
    expect_velocity(middle_x, last_y, 0, 0, "resetVelocity explicit last row")

    set_velocity(last_x, last_y)
    sim.resetVelocity(last_x, last_y, 1, 1)
    expect_velocity(last_x, last_y, 0, 0, "resetVelocity explicit last cell")
end

local function run()
    expect_ok("resetPressure INT_MIN", function()
        sim.resetPressure(-2147483648, -2147483648, 1, 1)
    end)
    expect_ok("resetVelocity INT_MIN", function()
        sim.resetVelocity(-2147483648, -2147483648, 1, 1)
    end)
    expect_ok("resetVelocity full boundary coverage", check_reset_velocity_bounds)
    expect_error("neighbors invalid position", "Invalid position", function()
        return sim.neighbors(-2147483648, 0, 1, 1)()
    end)
    expect_error("neighbors invalid radius", "Invalid radius", function()
        return sim.neighbors(100, 100, 1000000, 1)()
    end)

    write_result("PASS", "lua_bounds=11")
end

local ok, detail = pcall(run)
if not ok then
    write_result("FAIL", detail)
end
