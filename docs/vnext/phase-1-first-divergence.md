# Phase 1 first-divergence capture

## Outcome

```text
REPORT_DATE=2026-08-09
BASE_COMMIT=9102d144fd2a9ed556f9f7ee9bfe57769ca051fc
IMPLEMENTATION_COMMIT=c6eecaa77cd7d6025ef997c5dd46e53d112c6e08
FORMAL_RUN_COMMIT=c6eecaa77cd7d6025ef997c5dd46e53d112c6e08
REPORT_COMMIT=SELF
FIRST_DIVERGENCE_RUNNER=GREEN
FIELD_CAPTURE=GREEN
SAME_SOURCE_CPU_FP_DIFFERENTIAL=GREEN
LEGACY_CPU_VS_OMNI_CPU=NOT_IMPLEMENTED
OMNI_CPU_VS_OMNI_GPU=NOT_IMPLEMENTED
CONSERVATION_FINITE_LEDGER=RED
G0_UPSTREAM_BASELINE=RED
PRODUCTION_OMNIATMOSPHERE_ALLOWED=false
```

> Historical status annotation (2026-08-09): `LOAD_BOUNDARY_FIELD_DIFF=RED` in
> this report describes the state at its report commit. The later
> [OPS load-boundary field-attribution report](phase-1-load-boundary.md) closes
> that independent sub-gate at `GREEN` with a clean `9b336fc40` artifact. This
> does not change this report's differential findings or any of its other `RED`
> gates.

The first-divergence and field-capture sub-gate is **GREEN**. A clean-source run
compared the same Legacy simulation in `legacy_fast` and `strict` CPU builds from
one generated mixed state, located the first hash divergence at update step 1,
reproduced that exact step, and exported corresponding differing Particle and Air
fields.

This is not an Omni CPU or GPU comparison. It proves that the two current floating-
point contracts produce different observable state on the first update; it does not
prove which result is more physically or numerically correct. G0 remains **RED**
because conservation, positivity and finite-value ledgers, subsystem profiling,
process VRAM and an accepted performance budget are still absent.

## Goal and implementation

The implementation adds four tooling/test files and changes no production
simulation, Particle, Air, Save, Lua API or persistent data structure:

- `tools/runtime/differential_probe.lua` records a state signature after every
  fixed update and can rerun to capture one exact step;
- `tools/differential_compare.py` validates sequential traces, finds the first
  differing field and compares complete active-particle and Air-cell exports;
- `tools/runtime_first_divergence.ps1` isolates client profiles, binds source,
  build, executable and tool hashes, and retains raw evidence under ignored
  `artifacts/`; and
- `tools/tests/test_first_divergence.py` supplies eight fail-closed comparator,
  probe and wrapper contract tests.

The trace records step, `Snapshot::Hash`, active-particle count and all four RNG
words. When a divergence is found, the wrapper launches fresh capture processes at
that step. The comparator rejects malformed/non-sequential traces and non-finite
captured floats. A differing combined hash with no exported Particle or cell-field
difference sets `unexplained_hash_divergence=true` and fails the wrapper.

## Formal artifact binding

The formal run is bound to clean commit
`c6eecaa77cd7d6025ef997c5dd46e53d112c6e08` and these CPU executables:

| Mode | Executable bytes | SHA-256 |
|---|---:|---|
| Legacy-fast | 321,335,036 | `41FE38FB76C0F4323DA9109B6C3616B40F423FFF12274A0010061F87A232BE6D` |
| Strict | 321,354,807 | `2BD30C0112793EADD5316CD640262873BDA6F999C8935894DE73D1F1D40620BC` |

Both development executables used `C:/msys64/ucrt64/bin` at runtime. Portable
execution remains `not_tested`. The ignored private manifest is:

```text
artifacts/vnext-differential/windows-276049E7945C/c6eecaa77c/
20260808T224612Z-d0a4e2bb/manifest.json
```

Manifest SHA-256:
`9B0A48ECAD09291D4974D612BAFBDF1428547679FEB9C9B392146B307DB5DAB3`.
It contains 25 files: the manifest plus 24 declared files whose lengths and
SHA-256 values independently passed `24/24`. The source binding also records:

| Input | SHA-256 |
|---|---|
| Lua probe | `328D3B5C766243F07FA9941BA8424CD56FED0BE89C26E8D15C734F1B5BF45780` |
| Python comparator | `CD7066D232BDD2A57D5071B6FF8EC4A13138D4B968439BC87A985EF553804A49` |
| PowerShell wrapper | `938C2D229D0EC9244D39C2E7A8272AC01EC36EF6EED25B21D5C521C2B1FB940C` |

Two independent recomputations from the formal raw CSV files produced byte-identical
comparison JSON:

| Comparison | Bytes | SHA-256 |
|---|---:|---|
| Trace | 670 | `B0B67B565BC0D2B4101BAA8DDB492B06DB0A560776B38E09B4BD23B73E9B8C2D` |
| Field capture | 36,710 | `A4515630E56897A73082CEE7075E7ABF19FD700CEE23BA020C3FBD21D5B00F77` |

This recomputation verifies deterministic comparison of the retained CSV evidence;
it is not a second generation of those CSV files by the two clients.

## First divergence

The mixed-medium scene used seed `(101, 202, 303, 404)`. Both builds began with
59,084 particles and identical step-0 state and RNG. The trace contains 121 rows
covering steps 0 through 120. Fresh capture processes reproduced the step-1 hashes,
particle counts and RNG words before exporting fields.

| Observation | Legacy-fast | Strict |
|---|---:|---:|
| Step-0 state hash | 3,129,219,535 | 3,129,219,535 |
| Step-1 state hash | 2,089,865,305 | 1,195,260,457 |
| Step-1 particle count | 59,084 | 59,084 |
| Step-1 RNG | `(827760370, 872114107, 2602751517, 2893986612)` | identical |

At step 1, 4,935 particle IDs and 2,074 of 14,688 Air cells differ. Particle
differences are limited to `vx/vy/x/y` in this capture; Air differences are limited
to `ambient_heat/velocity_x/velocity_y`. The first particle difference in ID order
is:

| Field | Legacy-fast | Strict |
|---|---:|---:|
| Particle | ID `13906`, `DEFAULT_PT_DUST` | same |
| `vx` | `-0.497469544` | `-0.497469574` |
| `x`, `y` | `135.502533`, `232.059891` | identical |
| `vy`, `temp` | `0.0598977022`, `295.149994` | identical |

The first Air-cell difference in `(cy, cx)` sort order is `(cx, cy) = (86, 12)`, where
`ambient_heat` is `295.149994` versus `295.150024`. The differing particle lies in
cell `(34, 58)`; that cell's `velocity_x` is `-0.385244876` versus
`-0.385244936`. Its pressure, vertical velocity and ambient heat are identical at
this step. The capture also contains both sides of the local particle neighborhood.

The field comparison reports `unexplained_hash_divergence=false`: this particular
hash difference is observable in exported domains. "First" here means comparator
sort order, not the causal origin of numerical propagation. No special-case
experimental logic or tolerance was added to the simulator.

## Coverage boundary

The current capture exports:

- every active Particle ID and all persisted Particle fields;
- `pv`, `vx`, `vy` and `hv` for every Air cell;
- wall, electrical and fan maps; and
- gravity input/mask and output force cells.

It does not yet export `blockAir/blockAirH`, portal storage, wireless channels, or
stickman/fighter auxiliary state. Because this run already has exported differences,
`unexplained_hash_divergence=false` does not prove those unexported domains are
equal. Pre-save versus post-load field capture is also still unexecuted; the 14/14
known OPS Snapshot mismatch remains explicit rather than being normalized away.

`sim.hash()` is the existing 32-bit FNV state signature. It is useful for locating
a divergence, but it is not a cryptographic content digest, a conservation metric or
a correctness proof. Likewise, rejecting non-finite values while parsing exported
capture CSV is not a whole-state, every-tick NaN/Inf ledger.

The implemented comparison topology is therefore:

```text
same Legacy source: legacy_fast CPU <-> strict CPU    GREEN
Legacy CPU <-> future Omni CPU                        NOT_IMPLEMENTED
future Omni CPU <-> future Omni GPU                   NOT_IMPLEMENTED
```

## Tests and review

- First-divergence unit/contracts: `8/8 PASS`.
- Empty five-step development smoke comparison: PASS with
  `divergence_found=false`; it is not the formal clean-source artifact.
- Mixed smoke and formal 120-step run: both locate step 1 and the same primary
  Particle/Air fields.
- Legacy-fast Meson suite: `39/39 PASS`.
- Current-HEAD Strict Meson suite: `39/39 PASS`.
- Current-HEAD Python suite with UCRT64 compiler on PATH: 281 run, 281 passed,
  0 skipped, 0 failed. The earlier two conditional skips execute and pass when
  `c++.exe` is discoverable.
- Formal artifact static validation: `24/24 PASS`, with no missing or extra declared
  evidence file.
- Independent trace and field JSON recomputation: byte-identical PASS.

These automated results do not constitute visible GUI, visual-equivalence,
portable-runtime, long-run, performance, release or physical-correctness evidence.

## Benchmark, memory and compatibility

The formal wrapper sampled four short client phases. Wall times were 1.823017 and
1.769083 seconds for the two traces, then 0.416713 and 0.387138 seconds for capture.
Peak working set ranged from 133,001,216 to 136,970,240 bytes; maximum sampled
private bytes were 137,379,840. These are diagnostic process measurements with one
run per path and different trajectories, not a benchmark or speed comparison.
The manifest records `performance_gate=not_evaluated`; process VRAM remains
`not_tested`.

No production hot loop or persistent state changed, so the previous fixed-step
throughput baseline remains the before measurement and an after benchmark is not
applicable to simulator behavior. Stable element IDs, Particle layout, Lua property
indices, save formats and Classic updates are unchanged by the tooling commit.

## Interpretation and remaining risks

The result strengthens the fast-math risk from "diverges by warmup 60" to "diverges
on the first update in both Particle and Air state." It does not establish a
reference truth: Strict may avoid transformations that hide invalid values, but
neither build currently reports condensed/gas mass, momentum, energy, positivity or
NaN/Inf drift. `OMNICORE_FAST_MATH_ALLOWED=false` therefore remains unchanged.

The formal scope is one generated mixed scene, GCC/UCRT64 and two CPU FP modes. It
does not validate another compiler, architecture, Classic/Omni boundary or GPU.

The next numerical tool must add a visible Legacy conservation/finite-value ledger
and compare controlled strict/fast trajectories without silently clamping errors.
The other independent G0 path is subsystem profiler export plus process VRAM. A
future Omni backend must reuse this runner and add explicit intentional-deviation or
tolerance policies rather than treating screenshot similarity as equivalence.

## Gate and rollback

```text
TRACE_PROVENANCE=GREEN
FIRST_DIVERGENCE_LOCATION=GREEN
PARTICLE_AIR_FIELD_CAPTURE=GREEN
UNEXPLAINED_HASH_FAIL_CLOSED=GREEN
SAME_SOURCE_CPU_FP_DIFFERENTIAL=GREEN
LOAD_BOUNDARY_FIELD_DIFF=RED
CONSERVATION_POSITIVITY_FINITE_LEDGER=RED
SUBSYSTEM_PROFILER=RED
PROCESS_VRAM=RED
PERFORMANCE_REGRESSION_BUDGET=RED
LEGACY_CPU_VS_OMNI_CPU=RED
OMNI_CPU_VS_OMNI_GPU=RED
G0_UPSTREAM_BASELINE=RED
```

The implementation rollback point is
`9102d144fd2a9ed556f9f7ee9bfe57769ca051fc`. Reverting `c6eecaa77` removes only
the four private differential tools/tests; no data or save migration is required.
No rollback is recommended.
