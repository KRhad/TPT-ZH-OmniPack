local RESULT = "omni-correction-ledger.result"

local EXPECTED_KINDS = {
    air_ambient_heat_temperature_cap_high = sim.MAX_TEMP,
    air_ambient_heat_temperature_cap_low = sim.MIN_TEMP,
    air_ambient_heat_velocity_x_cap_high = sim.MAX_PRESSURE,
    air_ambient_heat_velocity_x_cap_low = sim.MIN_PRESSURE,
    air_ambient_heat_velocity_y_cap_high = sim.MAX_PRESSURE,
    air_ambient_heat_velocity_y_cap_low = sim.MIN_PRESSURE,
    air_dynamics_pressure_cap_high = sim.MAX_PRESSURE,
    air_dynamics_pressure_cap_low = sim.MIN_PRESSURE,
    air_dynamics_velocity_x_cap_high = sim.MAX_PRESSURE,
    air_dynamics_velocity_x_cap_low = sim.MIN_PRESSURE,
    air_dynamics_velocity_y_cap_high = sim.MAX_PRESSURE,
    air_dynamics_velocity_y_cap_low = sim.MIN_PRESSURE,
}

local function is_finite(value)
    return value == value and value ~= math.huge and value ~= -math.huge
end

local function assert_close(actual, expected, label)
    assert(math.abs(actual - expected) <= 1.0e-4,
        label .. ": expected=" .. tostring(expected) .. ", actual=" .. tostring(actual))
end

local function configure()
    sim.clearSim()
    sim.paused(true)
    sim.gravityMode(sim.GRAV_OFF)
    sim.newtonianGravity(false)
    sim.airMode(sim.AIR_ON)
    sim.ambientHeatSim(true)
    sim.heatSim(true)
    sim.convectionMode(sim.AIRC_NONE)
    sim.edgePressure(0.0)
    sim.edgeVelocity(0.0, 0.0)
    sim.vorticityCoeff(0.0)
    sim.resetPressure()
    sim.resetVelocity()
    sim.ensureDeterminism(true)
    sim.randomSeed(97, 101, 103, 107)
end

local function test_default_state()
    sim.omniCorrectionLedgerEnabled(false)
    local metrics = sim.omniCorrectionLedger()
    assert(not metrics.enabled, "correction observer is not disabled by default")
    assert(metrics.legacy_field_units_only,
        "disabled correction observer lost its Legacy-field-only contract")
    assert(metrics.audited_air_caps_only,
        "disabled correction observer lost its audited-Air-cap scope")
    assert(metrics.total_events == 0 and metrics.retained_events == 0
            and metrics.dropped_events == 0,
        "disabled correction observer accumulated events")
    assert(metrics.event_capacity == 256, "unexpected correction event capacity")
end

local function trigger_real_air_caps()
    configure()
    sim.omniCorrectionLedgerEnabled(true)

    local fan = assert(sim.walls.DEFAULT_WL_FAN,
        "DEFAULT_WL_FAN is unavailable")
    local cells = {
        { x = 20, y = 20, vx = 10000.0, vy = 10000.0 },
        { x = 40, y = 20, vx = -10000.0, vy = -10000.0 },
    }
    for _, cell in ipairs(cells) do
        sim.wallMap(cell.x, cell.y, fan)
        sim.fanVelocityX(cell.x, cell.y, cell.vx)
        sim.fanVelocityY(cell.x, cell.y, cell.vy)
    end

    sim.updateUpTo()
    return sim.omniCorrectionLedger()
end

local function validate_metrics(metrics)
    assert(metrics.enabled, "correction observer became disabled")
    assert(metrics.legacy_field_units_only,
        "correction observer must not claim physical correction units")
    assert(metrics.audited_air_caps_only,
        "correction observer must declare its audited-Air-cap-only scope")
    assert(metrics.total_events > 0, "no executed Air cap branch was observed")
    assert(metrics.retained_events > 0,
        "observer reported events but retained no event records")
    assert(metrics.retained_events <= metrics.event_capacity,
        "retained event count exceeds fixed capacity")
    assert(metrics.dropped_events == 0,
        "bounded fixture unexpectedly overflowed the event buffer")
    assert(metrics.total_events == metrics.retained_events + metrics.dropped_events,
        "event total does not reconcile with retained and dropped records")

    local counted = 0
    local observed_kinds = 0
    for kind, count in pairs(metrics.counts) do
        assert(EXPECTED_KINDS[kind] ~= nil, "unknown correction kind: " .. tostring(kind))
        assert(count >= 0 and count == math.floor(count),
            "invalid count for " .. kind)
        counted = counted + count
        if count > 0 then observed_kinds = observed_kinds + 1 end
    end
    assert(counted == metrics.total_events,
        "per-kind counts do not sum to total events")

    local previous_sequence = 0
    local event_kind_counts = {}
    for index, event in ipairs(metrics.events) do
        assert(index <= metrics.retained_events,
            "event table exceeds retained event count")
        assert(event.sequence > previous_sequence,
            "retained event sequence is not strictly increasing")
        previous_sequence = event.sequence
        assert(event.cell_x >= 0 and event.cell_x < sim.XCELLS,
            "event cell_x is out of range")
        assert(event.cell_y >= 0 and event.cell_y < sim.YCELLS,
            "event cell_y is out of range")
        assert(is_finite(event.before) and is_finite(event.after),
            "event before/after value is not finite")

        local expected_after = assert(EXPECTED_KINDS[event.kind],
            "event exported an unknown kind")
        assert_close(event.after, expected_after, event.kind .. " after")
        if event.kind:match("_high$") then
            assert(event.before > event.after,
                event.kind .. " did not reduce an above-cap value")
        else
            assert(event.before < event.after,
                event.kind .. " did not raise a below-cap value")
        end
        event_kind_counts[event.kind] = (event_kind_counts[event.kind] or 0) + 1
    end
    assert(#metrics.events == metrics.retained_events,
        "Lua event table length does not match retained_events")
    for kind, count in pairs(metrics.counts) do
        assert((event_kind_counts[kind] or 0) == count,
            "retained event records do not match count for " .. kind)
    end

    assert(metrics.counts.air_dynamics_velocity_x_cap_high > 0,
        "positive fan did not execute the Air X high-cap branch")
    assert(metrics.counts.air_dynamics_velocity_x_cap_low > 0,
        "negative fan did not execute the Air X low-cap branch")
    assert(metrics.counts.air_dynamics_velocity_y_cap_high > 0,
        "positive fan did not execute the Air Y high-cap branch")
    assert(metrics.counts.air_dynamics_velocity_y_cap_low > 0,
        "negative fan did not execute the Air Y low-cap branch")

    return observed_kinds
end

local function test_reset(metrics)
    sim.resetOmniCorrectionLedger()
    local reset = sim.omniCorrectionLedger()
    assert(reset.enabled, "reset unexpectedly disabled the observer")
    assert(reset.total_events == 0 and reset.retained_events == 0
            and reset.dropped_events == 0,
        "reset did not clear correction metrics")
    assert(#reset.events == 0, "reset retained correction event records")
    for kind, count in pairs(reset.counts) do
        assert(EXPECTED_KINDS[kind] ~= nil, "reset exported unknown kind")
        assert(count == 0, "reset retained a count for " .. kind)
    end
    return metrics
end

local function trigger_overflow()
    configure()
    sim.omniCorrectionLedgerEnabled(true)
    local fan = assert(sim.walls.DEFAULT_WL_FAN,
        "DEFAULT_WL_FAN is unavailable for overflow fixture")
    local x, y, width, height = 8, 8, 32, 32
    sim.wallMap(x, y, width, height, fan)
    sim.fanVelocityX(x, y, width, height, 10000.0)
    sim.fanVelocityY(x, y, width, height, 10000.0)
    sim.updateUpTo()
    return sim.omniCorrectionLedger()
end

local function validate_overflow(metrics)
    assert(metrics.enabled and metrics.audited_air_caps_only,
        "overflow fixture lost correction observer scope")
    assert(metrics.total_events > metrics.event_capacity,
        "overflow fixture did not exceed the fixed event capacity")
    assert(metrics.retained_events == metrics.event_capacity,
        "overflow fixture did not saturate retained event capacity")
    assert(metrics.dropped_events == metrics.total_events - metrics.retained_events,
        "overflow fixture dropped-event accounting did not reconcile")
    assert(#metrics.events == metrics.event_capacity,
        "overflow fixture returned an unexpected retained event length")

    local expected_first = metrics.total_events - metrics.retained_events + 1
    local previous_sequence = expected_first - 1
    for _, event in ipairs(metrics.events) do
        assert(event.sequence == previous_sequence + 1,
            "overflow event sequence window is not contiguous")
        previous_sequence = event.sequence
        assert(is_finite(event.before) and is_finite(event.after),
            "overflow event contains a non-finite value")
    end
    assert(metrics.events[1].sequence == expected_first,
        "overflow event window does not begin at the expected sequence")
    assert(metrics.events[#metrics.events].sequence == metrics.total_events,
        "overflow event window does not end at the latest sequence")

    local counted = 0
    for kind, count in pairs(metrics.counts) do
        assert(EXPECTED_KINDS[kind] ~= nil, "overflow exported unknown kind")
        counted = counted + count
    end
    assert(counted == metrics.total_events,
        "overflow cumulative per-kind counts do not reconcile")
    return expected_first
end

local function test()
    test_default_state()
    local metrics = trigger_real_air_caps()
    local observed_kinds = validate_metrics(metrics)
    test_reset(metrics)
    local overflow = trigger_overflow()
    local overflow_first_sequence = validate_overflow(overflow)
    return metrics, observed_kinds, overflow, overflow_first_sequence
end

local ok, metrics_or_error, observed_kinds, overflow, overflow_first_sequence =
    xpcall(test, debug.traceback)
local report = assert(io.open(RESULT, "w"))
if ok then
    report:write("OMNI_CORRECTION_LEDGER_LEGACY_FIELD_UNITS_ONLY=",
        tostring(metrics_or_error.legacy_field_units_only), "\n")
    report:write("OMNI_CORRECTION_LEDGER_AUDITED_AIR_CAPS_ONLY=",
        tostring(metrics_or_error.audited_air_caps_only), "\n")
    report:write("OMNI_CORRECTION_LEDGER_TOTAL_EVENTS=",
        metrics_or_error.total_events, "\n")
    report:write("OMNI_CORRECTION_LEDGER_RETAINED_EVENTS=",
        metrics_or_error.retained_events, "\n")
    report:write("OMNI_CORRECTION_LEDGER_DROPPED_EVENTS=",
        metrics_or_error.dropped_events, "\n")
    report:write("OMNI_CORRECTION_LEDGER_OBSERVED_KINDS=",
        observed_kinds, "\n")
    report:write("OMNI_CORRECTION_LEDGER_X_HIGH=",
        metrics_or_error.counts.air_dynamics_velocity_x_cap_high, "\n")
    report:write("OMNI_CORRECTION_LEDGER_X_LOW=",
        metrics_or_error.counts.air_dynamics_velocity_x_cap_low, "\n")
    report:write("OMNI_CORRECTION_LEDGER_Y_HIGH=",
        metrics_or_error.counts.air_dynamics_velocity_y_cap_high, "\n")
    report:write("OMNI_CORRECTION_LEDGER_Y_LOW=",
        metrics_or_error.counts.air_dynamics_velocity_y_cap_low, "\n")
    report:write("OMNI_CORRECTION_LEDGER_OVERFLOW_TOTAL_EVENTS=",
        overflow.total_events, "\n")
    report:write("OMNI_CORRECTION_LEDGER_OVERFLOW_RETAINED_EVENTS=",
        overflow.retained_events, "\n")
    report:write("OMNI_CORRECTION_LEDGER_OVERFLOW_DROPPED_EVENTS=",
        overflow.dropped_events, "\n")
    report:write("OMNI_CORRECTION_LEDGER_OVERFLOW_FIRST_SEQUENCE=",
        overflow_first_sequence, "\n")
    report:write("OMNI_CORRECTION_LEDGER_OVERFLOW_LAST_SEQUENCE=",
        overflow.events[#overflow.events].sequence, "\n")
    report:write("OMNI_CORRECTION_LEDGER_STATUS=PASS\n")
else
    report:write("OMNI_CORRECTION_LEDGER_ERROR=",
        tostring(metrics_or_error):gsub("[\r\n]+", " | "), "\n")
    report:write("OMNI_CORRECTION_LEDGER_STATUS=FAIL\n")
end
report:close()
