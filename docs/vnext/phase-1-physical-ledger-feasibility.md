# Phase 1 Legacy physical-ledger feasibility audit

## Outcome

```text
REPORT_DATE=2026-08-09
BASE_COMMIT=86a2b338634c36ea4f0da9cc49401e7dd4b48eae
STATIC_INVENTORY_TOOL=86a2b3386
CLEAN_SOURCE_REQUIRED=true
SOURCE_FILES_SCANNED=651
PHYSICAL_LEDGER_FEASIBILITY=GREEN
RUNTIME_LIFECYCLE_OBSERVER=NOT_IMPLEMENTED
RUNTIME_CORRECTION_OBSERVER=NOT_IMPLEMENTED
PHYSICAL_MASS_CONSERVATION=NOT_EVALUATED
PHYSICAL_MOMENTUM_CONSERVATION=NOT_EVALUATED
PHYSICAL_ENERGY_CONSERVATION=NOT_EVALUATED
SOURCE_SINK_ATTRIBUTION=NOT_EVALUATED
CORRECTION_EVENT_LEDGER=NOT_EVALUATED
PHYSICAL_PRESSURE_POSITIVITY=NOT_EVALUATED
G0_UPSTREAM_BASELINE=RED
PRODUCTION_OMNIATMOSPHERE_ALLOWED=false
```

The feasibility sub-gate is GREEN only because the current Legacy state and mutation
anchors are now source-bound and reproducibly inventoried. It does not turn a
Particle record, `Weight`, `HeatCapacity`, temperature, velocity or `pv` into a
physical quantity. No runtime observer exists yet, so all physical/source/correction
claims remain `NOT_EVALUATED` and G0 remains RED.

This is the historical feasibility snapshot at `86a2b3386`. Its runtime-lifecycle
status is superseded by the record-only observer report at
`phase-1-runtime-lifecycle-ledger.md`; all physical and correction claims above
remain unchanged.

## Method and reproducibility

`tools/physical_ledger_feasibility.py` scans the 651 C++ header/source files under
`src/simulation` and `src/lua`. It fails on a dirty worktree unless explicitly
overridden for development, verifies the expected three lifecycle definitions and
state fields, binds eight critical source-file SHA-256 values, and emits stable JSON.
`tools/tests/test_physical_ledger_feasibility.py` supplies two contract tests.

The clean inventory is bound to `86a2b338634c36ea4f0da9cc49401e7dd4b48eae` with
`source_dirty=false`. The source hashes include `Particle.h`
`F04F77B2...C0297F`, `Air.h` `4A916222...364AF2`, `Simulation.h`
`024405F8...63D0A8`, `Simulation.cpp` `3B1D7770...CC5070`, `Element.h`
`0C8EFFAE...9D32C0`, `SimulationData.cpp` `B0CD297F...736FA8`,
`LuaScriptInterface.cpp` `4A11EEA0...EBC8FC` and `LuaSimulation.cpp`
`31548AFF...CBD9A3`.

The focused feasibility suite passed `2/2`; the current nested Python discovery
suite passed 324 tests with two declared skips and no failures. This is static
analysis, not a build, GUI, physical or runtime acceptance test.

## What Legacy actually stores

| Domain | Current stored fields | Why it is not a physical ledger |
|---|---|---|
| Particle | `x/y/vx/vy/temp`; all other `Particle` fields are integer payload/state | no parcel mass, density, amount, enthalpy or internal-energy field exists |
| Air current planes | `pv/vx/vy/hv`, plus fan planes `fvx/fvy` | no density, species, gas mass, moles or energy field exists |
| Air scratch planes | `opv/ovx/ovy/ohv` and block maps | transient solver work state is not included in the all-tick Lua export |
| Element metadata | `Weight`, `HeatConduct`, `HeatCapacity`, transport/gameplay coefficients | `Weight` is used as a Legacy movement/comparison parameter; `HeatCapacity` is documented as per-pixel volumetric game state, with no accepted SI scale, parcel volume/depth or J/K conversion |

The audit found no `mass`, `density`, `moles`, `enthalpy` or `energy` field
declaration in the authoritative `Particle.h` or `Air.h` state. This does not mean
such physics cannot be introduced; it proves it cannot be honestly reconstructed
from the current authoritative Legacy fields without a new scale/state contract.

`SimulationData.cpp:129` compares `Element.Weight` during movement, and
`STKM.cpp:531` uses it in a gameplay collision adjustment. Those are concrete local
uses, not evidence that it is kg or a parcel mass. `HeatCapacity` can support a
Legacy thermal proxy but cannot make temperature sums into J without Phase 2's
pixel/depth/parcel/time contract and a declared energy reference.

## Lifecycle source/sink anchors

The central APIs are defined once each in `Simulation.cpp`:

| API | Definition | Lexical occurrences in `src/simulation` + `src/lua` |
|---|---:|---:|
| `kill_part` | 1763 | 173 |
| `part_change_type` | 1806 | 337 |
| `create_part` | 1851 | 241 |

These are the correct first observer anchors, but they are not the entire state
mutation surface. The audit reports five direct `Particle.type` assignment
candidates:

| Line | Context | Observer implication |
|---:|---|---|
| `Simulation.cpp:242` | impossible-after-validation load fallback | must be an explicit load/failure category, not silently ignored |
| `Simulation.cpp:1833` | central `part_change_type` commit | naturally covered by an API observer |
| `Simulation.cpp:1876` | SPRK fast path | bypasses `part_change_type`; requires a distinct transition event |
| `Simulation.cpp:1952` | central `create_part` commit | naturally covered by an API observer |
| `Simulation.cpp:2588` | BRMT/TUNG transition preparation | requires explicit paired-transition handling to avoid misattribution |

Lua type writes route through `part_change_type` at the currently audited bindings,
but that is not permission to assume future Lua or Element code will do so. A
runtime observer must reconcile its per-type event deltas against a begin/end live
type histogram and fail closed on unexplained mutation.

## Correction and positivity boundary

The source inventory has 102 `restrict_flt` lexical candidates, 27 `std::clamp`,
189 `std::min`, 106 `std::max`, 63 `MIN_TEMP`, 218 `MAX_TEMP`, 36 `MIN_PRESSURE`
and 38 `MAX_PRESSURE` references. These are triage counts, not event counts: many
are ordinary gameplay calculations or bounds checks and may not actually clamp.

`Air.cpp` alone has 12 explicit cap candidates: temperature at lines 186-187,
ambient-heat-driven velocity at 243-246, and pressure/velocity at 459-464. This
demonstrates why a future correction ledger must instrument actual branch outcomes,
not grep totals or bound occupancy. It also confirms that signed Legacy `pv` has a
negative `MIN_PRESSURE`; range compliance can never be reported as physical pressure
positivity.

## Safe next implementation

The recommended next bounded change is an optional, disabled-by-default runtime
observer with no physical-unit claim:

1. At a tick boundary, capture a per-element live-record histogram.
2. In the three lifecycle APIs, record create, kill, replacement and type-change
   events in record units, including old/new type and a coarse origin enum.
3. Instrument the three bypass categories above explicitly rather than claiming the
   central APIs cover all mutations.
4. At the final boundary, reconcile event deltas with the final histogram. Any
   mismatch is an `unattributed_record_delta`, not a silently corrected result.
5. Add correction events only where a branch demonstrably clamps or floors a value;
   record the field, before/after value and operation class. Do not label ordinary
   `std::min`/`std::max` use as a numerical correction by lexical match alone.

That observer would characterize record creation/destruction and corrections. It
would still not establish physical mass/momentum/energy until Phase 2 introduces
the Physical Scale Contract and OmniAtmosphere introduces authoritative conserved
state.

## Gate and rollback

```text
PHYSICAL_LEDGER_FEASIBILITY=GREEN
ALL_TICK_POST_UPDATE_EXPORTED_FLOATS=GREEN
RUNTIME_LIFECYCLE_OBSERVER=RED
RUNTIME_CORRECTION_OBSERVER=RED
PHYSICAL_MASS_MOMENTUM_ENERGY_LEDGER=RED
SOURCE_SINK_ATTRIBUTION=RED
CORRECTION_EVENT_LEDGER=RED
INTERNAL_FULL_STATE_FINITE=RED
PHYSICAL_PRESSURE_POSITIVITY=RED
SUBSYSTEM_PROFILER=RED
PROCESS_VRAM=RED
G0_UPSTREAM_BASELINE=RED
```

Rollback is `git revert 86a2b3386`. It removes only the static audit tool and tests;
no production state, save, Lua behavior, source/sink or correction logic changed.
Production OmniAtmosphere remains blocked.
