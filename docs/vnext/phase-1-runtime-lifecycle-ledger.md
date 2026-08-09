# Phase 1 runtime lifecycle record ledger

## Outcome

```text
REPORT_DATE=2026-08-09
BASE_COMMIT=7d3e66daf
IMPLEMENTATION_COMMIT=4fa0ec2f36448aaf2aefac38f4d9d3d21d46baf6
REPORT_COMMIT=SELF
SOURCE_STATUS_AT_CODE_VALIDATION=clean
RUNTIME_RECORD_LIFECYCLE_OBSERVER=GREEN
OBSERVER_DEFAULT_ENABLED=false
OBSERVER_UNITS=record_units_only
PARTICLE_LAYOUT_CHANGED=false
AIR_STATE_CHANGED=false
SAVE_FORMAT_CHANGED=false
CLASSIC_SIMULATION_SEMANTICS_CHANGED=false
RUNTIME_CORRECTION_OBSERVER=RED
INTERNAL_FULL_STATE_FINITE=RED
PHYSICAL_MASS_CONSERVATION=NOT_EVALUATED
PHYSICAL_MOMENTUM_CONSERVATION=NOT_EVALUATED
PHYSICAL_ENERGY_CONSERVATION=NOT_EVALUATED
PHYSICAL_SOURCE_SINK_ATTRIBUTION=NOT_EVALUATED
G0_UPSTREAM_BASELINE=RED
PRODUCTION_OMNIATMOSPHERE_ALLOWED=false
```

`4fa0ec2f3` adds an optional, disabled-by-default `Simulation` observer that
accounts for **Particle records**, not physical matter. It takes a live-record
histogram at each `BeforeSim` boundary, records known lifecycle deltas, and
compares the resulting per-type expected histogram with the final `AfterSim`
histogram. A mismatch is exposed as an unattributed record delta; it is never
clamped, repaired, or relabelled as mass/energy conservation.

The bounded Lua client regression exercised eight actual `sim.updateUpTo()`
ticks. All eight reconciled with zero unattributed record delta. It also exercised
the central create/kill/type/replacement APIs, the SPRK direct-type fast path, and
the BRMT/TUNG direct preparation path. This proves the observer's scoped record
accounting on that deterministic fixture only. It does not prove coverage of all
Legacy mutation sources, physical conservation, pressure positivity, or numerical
correction accounting.

## Architecture and compatibility boundary

The private observer state holds two `int64_t[PT_NUM]` histograms plus scalar
counters. At the current `PT_NUM=1024`, the arrays alone consume 16,384 bytes per
`Simulation`; scalar overhead was not independently measured. It scans
`parts.active` at the begin/end of a tracked tick, so enabled diagnostic cost is
`O(parts.active + PT_NUM)` per tick. It is disabled by default; no throughput or
memory benchmark claim is made for this diagnostic feature.

No `Particle` field, Air field, element ID, pmap layout, OPS field, save version,
or existing Lua API changed. The additive Lua surface is:

```text
sim.omniLifecycleLedger()
sim.omniLifecycleLedgerEnabled([boolean])
sim.resetOmniLifecycleLedger()
```

The returned table explicitly contains `record_units_only=true`. It exports
begin/end record counts, lifecycle event totals, direct mutation totals,
outside-tick events, last/cumulative unattributed absolute delta, first mismatched
type, invalid type count, and reconciliation status. `PT_NUM` is used only as an
invalid-type mismatch sentinel and is never interpreted as a live element.

The observer handles the source-bound mutation anchors from the feasibility audit:

| Path | Attribution |
|---|---|
| `create_part` allocation | create record |
| `create_part` replacement | replacement type transition |
| `kill_part` | kill record |
| `part_change_type` | central type transition; `PT_NONE` remains a kill |
| SPRK fast path | direct type transition, separately counted |
| BRMT/TUNG transition preparation | direct type transition, separately counted |
| validated-impossible Load fallback | direct type transition, separately counted |

The current `SimulationImpl` dispatcher is serial. The implementation documents
that any future parallel dispatcher needs deterministic per-worker reduction before
writing the per-type observer arrays. This observer is not a CPU-threading design.

## Validation

| Check | Result |
|---|---|
| Dedicated Meson configuration | `build-vnext-lifecycle-ledger`, `debugoptimized`, `legacy_fast`, static prebuilt dependencies |
| Build | PASS; application SHA-256 `3A12B897B2A3A6C0C75FC04D4270F84434331BBF21FA5B13E44FD832CA3CFECC` |
| Meson suite | PASS, `39/39` |
| Python discovery | PASS, `327/327` |
| Observer source contract | PASS, `3/3` |
| Isolated lifecycle Lua client | PASS: `ticks=8`, `reconciliation_failures=0`, `last_unattributed_record_delta_abs=0`, SPRK outside-tick direct transition `1`, BRMT/TUNG preparation `1` |
| Existing upstream Lua regression | PASS: `UPSTREAM_100_1_STATUS=PASS`, `lua_bounds=11` |
| Existing mixed OPS round trip | PASS: two restart loads, 21 field assertions/load, 11 stable identifiers |
| Interactive GUI / visual acceptance | `not_tested` |
| Enabled-observer performance budget | `not_tested` |
| Physical mass, momentum, energy, species, atoms, charge | `not_evaluated` |

The runtime test runs standard implementation paths rather than a test-only
simulation branch. Its high-temperature BRMT with `ctype=TUNG` follows the existing
generic transition logic, producing the direct prep followed by the ordinary
type-change path. The test never uses a fixture-specific observer exception.

## Known limits and risk disposition

- Central lifecycle APIs and the three audited direct paths are attributed; a
  future raw `Particle.type` write is expected to appear as a reconciliation
  mismatch, but this is not a claim of exhaustive source attribution.
- The normal valid-load fallback is structurally instrumented but intentionally was
  not forced with a corrupt save.
- `clear_sim`, load construction, and user/Lua mutations outside a tick are
  reported as outside-tick events when they pass an instrumented anchor; they form
  the next tick's baseline rather than an in-tick conservation claim.
- Actual clamp/floor branch outcomes, Air scratch state, fan/gravity/wall auxiliary
  state, internal subphases, and correction amounts remain unobserved by this
  record-ledger phase. The later scoped audited-Air-cap observer is documented in
  `phase-1-runtime-correction-ledger.md`; it does not change this report's
  record-only or physical-conservation limits.
- This record observer does not alter `Weight`/`HeatCapacity` meanings and does not
  establish a physical scale contract.

## Gate and rollback

```text
RUNTIME_RECORD_LIFECYCLE_OBSERVER=GREEN
RUNTIME_CORRECTION_OBSERVER=RED
INTERNAL_FULL_STATE_FINITE=RED
PHYSICAL_CONSERVATION_LEDGER=RED
SOURCE_SINK_CORRECTION_LEDGER=RED
SUBSYSTEM_PROFILER=RED
PROCESS_VRAM=RED
G0_UPSTREAM_BASELINE=RED
```

Rollback is `git revert 4fa0ec2f3`. It removes only the optional observer, its
additive Lua diagnostics, and local regression tooling; it does not require a save
migration or revert any element data. The next allowed work remains G0 blocker
reduction: branch-outcome correction accounting, internal/full-state finite and
positivity evidence, or profiler/VRAM measurement. OmniAtmosphere implementation
remains blocked.
