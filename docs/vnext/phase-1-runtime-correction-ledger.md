# Phase 1 runtime correction observer

## Outcome

```text
REPORT_DATE=2026-08-09
BASE_COMMIT=9274fdb3a7ec0a483a1a1ac2cc36bf8beef9517e
IMPLEMENTATION_COMMIT=4c9f3b909
RUNTIME_CORRECTION_OBSERVER=GREEN
CORRECTION_SCOPE=audited_air_caps_only
CORRECTION_UNITS=legacy_field_units_only
OBSERVER_DEFAULT_ENABLED=false
EVENT_CAPACITY=256
PHYSICAL_SOURCE_SINK_LEDGER=RED
PHYSICAL_MASS_CONSERVATION=NOT_EVALUATED
PHYSICAL_MOMENTUM_CONSERVATION=NOT_EVALUATED
PHYSICAL_ENERGY_CONSERVATION=NOT_EVALUATED
UNSAMPLED_FULL_STATE_FINITE=RED
PROCESS_VRAM=RED
PERFORMANCE_REGRESSION_BUDGET=RED
G0_UPSTREAM_BASELINE=RED
PRODUCTION_OMNIATMOSPHERE_ALLOWED=false
```

This phase adds a disabled-by-default runtime observer for the 12 explicit cap
branches in the current `src/simulation/Air.cpp`: six ambient-heat branches and
six Air pressure/velocity branches. It records an actually executed branch with
cell coordinates, before/after Legacy field values, operation kind and a
monotonic sequence number. It does not infer events from lexical matches or from
values merely being equal to a bound.

The scope is deliberately narrow. `audited_air_caps_only=true` and
`legacy_field_units_only=true` are exported by the Lua API. The observer is not a
physical source/sink ledger, does not interpret `pv` as pressure in pascals,
does not interpret `hv` as energy, and does not establish positivity or any mass,
momentum, energy, species, atom or charge conservation property.

## Implementation

The additive Lua API is:

```text
sim.omniCorrectionLedger()
sim.omniCorrectionLedgerEnabled([boolean])
sim.resetOmniCorrectionLedger()
```

The event buffer has fixed capacity 256. Once full, new events overwrite the
oldest retained record and increment `dropped_events`; total, per-kind,
retained and dropped counts remain explicit. An invalid future enum value is
ignored fail-closed before indexing the count array. Event records are value
initialized, and the current serial Air path is documented as requiring a
deterministic per-worker merge before a future parallel backend can expose the
same observer.

No Particle layout, Air state semantics, save format, element ID, existing Lua
function or default simulation behavior changed when the observer is disabled.
The only production-source modifications are the observer storage/API and calls
at the already existing cap branches.

## Runtime fixture

`tools/runtime/omni_correction_ledger.lua` uses the ordinary Lua simulation API:

1. clear and configure a normal Legacy Air simulation;
2. place two `sim.walls.DEFAULT_WL_FAN` cells;
3. set positive and negative fan velocities with `sim.fanVelocityX/Y`;
4. execute one normal `sim.updateUpTo()` tick;
5. inspect the returned event table and reset it;
6. run a second public-API FAN scene that produces 2,048 events and crosses the
   256-record ring-buffer boundary.

The fixture has no test-name branch or observer exception. It checks default
disabled state, explicit scope, finite before/after values, fixed cap values,
strictly increasing event sequence, per-kind count reconciliation, buffer
capacity and reset behavior. The positive/negative FAN pair observed all four
Air dynamics velocity cap directions. The overflow scene observed `2,048`
events, retained exactly `256`, dropped `1,792`, and exposed the contiguous
retained sequence window `1793..2048`.

## Validation

| Check | Result |
|---|---|
| Full UCRT64 Ninja rebuild | PASS, `564/564` targets/tasks completed |
| Build configuration | `debugoptimized`, `fp_mode=legacy_fast`, `static=prebuilt`, `build_tests=true`, `lto=false`, `x86_sse=auto` |
| Application | 321,399,790 bytes; SHA-256 `A1DF374DF15307BAE3E25E59346143EC84B81B15BC1BC4AB0433030A409A00B8` |
| Correction Lua runtime | PASS; base scene total `4`, retained `4`, dropped `0`, observed kinds `4`; overflow scene total `2,048`, retained `256`, dropped `1,792`, sequence `1793..2048` |
| Lifecycle Lua runtime | PASS; 8 ticks, reconciliation failures `0`, unattributed record delta `0` |
| Upstream 100.1 Lua regression | PASS; `lua_bounds=11` |
| Mixed OPS round trip | PASS; 3 processes, 2 restarts, 2 loads, 21 assertions/load, 11 stable identifiers |
| Dedicated correction Python contract | PASS, `4/4` |
| Lifecycle Python contract | PASS, `3/3` |
| Meson suite | PASS, `39/39` |
| Python discovery | PASS, `331` tests, `329` passed and `2` skipped |
| GUI / visual / portable runtime | `not_tested` |
| Enabled observer performance overhead | `not_tested` |
| Process VRAM | `not_tested` |
| Physical conservation | `not_evaluated` |

The first direct invocation without the UCRT runtime path failed with Windows
loader status `0xC0000135`; the already-passing lifecycle fixture failed the same
way, proving this was an execution-environment issue rather than a correction
implementation result. The wrapper now injects its default
`C:\msys64\ucrt64\bin` runtime directory into the isolated child process, and a
plain wrapper invocation subsequently passed. No system PATH or repository
configuration was changed.

## Limits and risks

- Only the 12 explicit `Air.cpp` cap branches are observed. Other `restrict_flt`,
  `std::clamp`, `std::min`, `std::max`, floor, allocator, particle, thermal,
  chemistry and renderer paths remain outside this observer.
- The ring buffer is diagnostic evidence, not a complete event archive; overflow
  is visible through `dropped_events`. The bounded overflow/wraparound fixture
  proves the sequence-window and count contract, but does not make the buffer a
  lossless archive.
- The runtime fixture observes four dynamics velocity events. The other eight
  branch kinds are source-instrumented and exported but were not claimed as
  observed by this fixture.
- Because Legacy `MIN_PRESSURE` is negative and `pv` is a signed pressure-like
  proxy, these events cannot establish physical pressure positivity.
- The observer is serial-state code. A future CPU-threaded or GPU backend needs a
  deterministic proposal/reduction design before reusing this API.

## Gate and rollback

```text
RUNTIME_CORRECTION_OBSERVER=GREEN
AUDITED_AIR_CAPS_RUNTIME_EVIDENCE=GREEN
SOURCE_SINK_CORRECTION_LEDGER=RED
PHYSICAL_CONSERVATION_LEDGER=RED
UNSAMPLED_FULL_STATE_FINITE=RED
PROCESS_VRAM=RED
PERFORMANCE_REGRESSION_BUDGET=RED
G0_UPSTREAM_BASELINE=RED
```

This phase is independently reversible by reverting its implementation commit
after it is created. Reverting removes only the optional observer, its additive
Lua diagnostics and private local fixture tooling; no save migration or element
ID rollback is required. Production OmniAtmosphere implementation remains
blocked.
