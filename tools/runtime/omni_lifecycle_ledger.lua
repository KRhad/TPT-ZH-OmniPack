local RESULT = "omni-lifecycle-ledger.result"

local ids = {
    dust = assert(elements.DEFAULT_PT_DUST),
    water = assert(elements.DEFAULT_PT_WATR),
    oil = assert(elements.DEFAULT_PT_OIL),
    metal = assert(elements.DEFAULT_PT_METL),
    spark = assert(elements.DEFAULT_PT_SPRK),
    brmt = assert(elements.DEFAULT_PT_BRMT),
    tung = assert(elements.DEFAULT_PT_TUNG),
}

local function make(element, x, y, temperature)
    local particle = sim.partCreate(-1, x, y, element)
    assert(particle >= 0, "failed to create element " .. tostring(element))
    if temperature then sim.partProperty(particle, "temp", temperature) end
    return particle
end

local function configure()
    sim.clearSim()
    sim.paused(true)
    sim.gravityMode(sim.GRAV_OFF)
    sim.airMode(sim.AIR_OFF)
    sim.ambientHeatSim(false)
    sim.heatSim(true)
    sim.ensureDeterminism(true)
    sim.randomSeed(73, 79, 83, 89)
end

local function expect_reconciled(metrics, completed_ticks)
    assert(metrics.enabled, "lifecycle ledger was disabled during a tick")
    assert(metrics.record_units_only,
        "lifecycle ledger must not claim physical units")
    assert(not metrics.active_tick, "lifecycle ledger left a tick open")
    assert(metrics.ticks_started == completed_ticks,
        "unexpected tick starts: " .. tostring(metrics.ticks_started))
    assert(metrics.ticks_completed == completed_ticks,
        "unexpected tick completions: " .. tostring(metrics.ticks_completed))
    assert(metrics.last_frame_reconciled,
        "last tick had an unattributed record delta")
    assert(metrics.reconciliation_failures == 0,
        "a previous tick had an unattributed record delta")
    assert(metrics.last_unattributed_record_delta_abs == 0,
        "last unattributed record delta was nonzero")
    assert(metrics.total_unattributed_record_delta_abs == 0,
        "cumulative unattributed record delta was nonzero")
    assert(metrics.last_first_mismatched_type == -1,
        "unexpected mismatched type: " .. tostring(metrics.last_first_mismatched_type))
end

local function test_default_state()
    sim.omniLifecycleLedgerEnabled(false)
    local metrics = sim.omniLifecycleLedger()
    assert(not metrics.enabled, "lifecycle ledger is not disabled by default")
    assert(metrics.record_units_only,
        "disabled lifecycle ledger lost its explicit record-only contract")
    assert(metrics.ticks_started == 0 and metrics.ticks_completed == 0,
        "disabled lifecycle ledger accumulated tick state")
end

local function test_outside_tick_categories()
    configure()
    sim.omniLifecycleLedgerEnabled(true)

    local particle = make(ids.dust, 120, 120, 300.0)
    sim.partChangeType(particle, ids.water)
    local replacement = sim.partCreate(particle, 120, 120, ids.oil)
    assert(replacement == particle, "record replacement allocated a new particle")
    sim.partKill(particle)

    local conductor = make(ids.metal, 140, 120, 300.0)
    local spark = sim.partCreate(conductor, 140, 120, ids.spark)
    assert(spark == conductor, "SPRK fast path replaced its record")

    local metrics = sim.omniLifecycleLedger()
    assert(metrics.creates == 2, "outside-tick creates were not fully attributed")
    assert(metrics.kills == 1, "outside-tick kill was not attributed")
    assert(metrics.replacements == 1, "outside-tick replacement was not attributed")
    assert(metrics.spark_fast_path_transitions == 1,
        "SPRK fast path was not explicitly attributed")
    assert(metrics.direct_type_transitions == 1,
        "direct type transition total did not include SPRK")
    assert(metrics.outside_tick_events == 6,
        "outside-tick event total changed unexpectedly: " .. tostring(metrics.outside_tick_events))
    assert(metrics.outside_tick_direct_transitions == 1,
        "outside-tick direct transition total did not include SPRK")
    return metrics
end

local function test_per_tick_reconciliation()
    configure()
    sim.omniLifecycleLedgerEnabled(true)

    make(ids.water, 180, 120, 600.0)
    local brmt = make(ids.brmt, 240, 120, 5000.0)
    sim.partProperty(brmt, "ctype", ids.tung)

    -- Initial construction is intentionally outside the accounting boundary.
    -- The first tracked tick captures the complete live-record baseline.
    sim.resetOmniLifecycleLedger()
    for tick = 1, 8 do
        sim.updateUpTo()
        expect_reconciled(sim.omniLifecycleLedger(), tick)
    end

    local metrics = sim.omniLifecycleLedger()
    assert(metrics.type_transitions >= 3,
        "expected normal transition accounting was not observed")
    assert(metrics.brmt_tung_preparation_transitions == 1,
        "BRMT/TUNG preparation bypass was not explicitly attributed")
    return metrics
end

local function test()
    test_default_state()
    local outside_tick_metrics = test_outside_tick_categories()
    local tick_metrics = test_per_tick_reconciliation()
    return {
        outside_tick_metrics = outside_tick_metrics,
        tick_metrics = tick_metrics,
    }
end

local ok, metrics_or_error = xpcall(test, debug.traceback)
local report = assert(io.open(RESULT, "w"))
if ok then
    local metrics = metrics_or_error.tick_metrics
    local outside_tick_metrics = metrics_or_error.outside_tick_metrics
    report:write("OMNI_LIFECYCLE_LEDGER_RECORD_UNITS_ONLY=",
        tostring(metrics.record_units_only), "\n")
    report:write("OMNI_LIFECYCLE_LEDGER_TICKS=", metrics.ticks_completed, "\n")
    report:write("OMNI_LIFECYCLE_LEDGER_RECONCILIATION_FAILURES=",
        metrics.reconciliation_failures, "\n")
    report:write("OMNI_LIFECYCLE_LEDGER_LAST_FRAME_RECONCILED=",
        tostring(metrics.last_frame_reconciled), "\n")
    report:write("OMNI_LIFECYCLE_LEDGER_LAST_UNATTRIBUTED_RECORD_DELTA_ABS=",
        metrics.last_unattributed_record_delta_abs, "\n")
    report:write("OMNI_LIFECYCLE_LEDGER_OUTSIDE_DIRECT_TRANSITIONS=",
        outside_tick_metrics.outside_tick_direct_transitions, "\n")
    report:write("OMNI_LIFECYCLE_LEDGER_BRMT_TUNG_PREPARATION_TRANSITIONS=",
        metrics.brmt_tung_preparation_transitions, "\n")
    report:write("OMNI_LIFECYCLE_LEDGER_STATUS=PASS\n")
else
    report:write("OMNI_LIFECYCLE_LEDGER_ERROR=",
        tostring(metrics_or_error):gsub("[\r\n]+", " | "), "\n")
    report:write("OMNI_LIFECYCLE_LEDGER_STATUS=FAIL\n")
end
report:close()
