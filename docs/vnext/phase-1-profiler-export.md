# 1.0.1 diagnostics and profiler milestone

## Outcome

```text
TARGET_VERSION=1.0.1
BASE_COMMIT=ca3d643c8b24a8588a5e84477e145312707d354b
IMPLEMENTATION_COMMITS=2b6b6e39eaa0647d2371d8f5788dc5dd143322c4,97d2fc2c175818a66636421526e4f562d4d1de01
VALIDATED_HEAD=97d2fc2c175818a66636421526e4f562d4d1de01
VALIDATED_EXECUTABLE_SHA256=EA2C8517771E615D6DFC86B9E3AFD5A77F0D49F8F0FE3F8635E6AF21D02B279D
BUILD=GREEN
CONTRACT_TESTS=GREEN
MESON_SUITE=GREEN
PYTHON_DISCOVERY=GREEN
RUNTIME=GREEN
CONCURRENCY=GREEN
BENCHMARK_RECORDED=GREEN
PROFILER_OVERHEAD_MEASURED=GREEN
INDEPENDENT_PROFILER_REVIEW=not_available
GUI_VISUAL_ACCEPTANCE=not_tested
PROCESS_VRAM=not_tested_no_gpu_backend
PERFORMANCE_REGRESSION_BUDGET=not_evaluated
PHYSICAL_CONSERVATION=RED
V1_0_1_GATE=GREEN
NEXT_VERSION=1.0.2
```

Version 1.0.1 is closed as the diagnostics and stability foundation. It adds no
new atmosphere, materials, thermal, chemistry, SDL3, CUDA or SDL_GPU behavior.
It instruments the existing Legacy CPU path and leaves all uninstrumented future
subsystems explicitly marked rather than emitting fake zero timings.

## Implementation

`2b6b6e39e` introduces the default-off `steady_clock` subsystem profiler,
stable subsystem names, Lua accessors and runtime fixtures. `97d2fc2c1` makes
the isolated profiler fixtures use an empty local profile so the normal UI loop
can tick without importing user state or stopping on the first-run modal.

The exposed Lua surface remains additive:

```lua
sim.omniProfiler()
sim.omniProfilerEnabled([boolean])
sim.resetOmniProfiler()
```

Instrumented Legacy boundaries are `frame`, `simulation`, `particle_update`,
`air`, `ambient_heat`, `gravity_dispatch_wait`, `lua`,
`render_snapshot_copy` and `rendering`. `thermal` and `chemistry` report
`not_instrumented`; `gpu` and `gpu_synchronization` report
`not_tested_no_gpu_backend`. Process VRAM reports the same explicit unavailable
status. `last_nanoseconds` means the last completed span, not a synthetic UI
frame duration.

Renderer worker spans hold a `shared_ptr<FrameTime>` and aggregation is guarded
by a mutex and generation checks. This fixes the raw-pointer lifetime race found
in review without changing renderer semantics.

## Final validation

The final validated executable was rebuilt from clean source at
`97d2fc2c1`; its SHA-256 is
`EA2C8517771E615D6DFC86B9E3AFD5A77F0D49F8F0FE3F8635E6AF21D02B279D`
and its size is `321,906,325` bytes. The PE hash differs from an earlier clean
relink of the same source (`79A03CA4...90BCCD`), so all final runtime and
benchmark evidence below is explicitly bound to the `EA2C...` artifact.

| Check | Result |
|---|---|
| Fresh Meson compile | PASS |
| Meson suite | `40/40 PASS` |
| Python discovery | `342 PASS`, `2 skipped`, `0 failed` |
| Upstream 100.1 Lua regression | PASS (`lua_bounds=11`) |
| OPS save/load round trips | `9/9 PASS`: mixed, official, metallurgy, biology, chemistry, nuclear, periodic, electronics, environment |
| Lifecycle ledger smoke | PASS; 8 ticks, 0 reconciliation failures, 0 unattributed record delta |
| Correction ledger smoke | PASS; 4 base events and overflow `2048 total / 256 retained / 1792 dropped` |
| Profiler Lua smoke | PASS; all Legacy instrumented spans observed; VRAM explicitly unavailable |
| Threaded rendering runtime fixture | PASS; 3 UI ticks, 1 toggle, worker path observed |
| Concurrency probe | PASS; 64 frame, 64 simulation, 4096 rendering calls; disable/reset lifetime race PASS; `sizeof(FrameTime)=576` |

The automation starts an isolated SDL client and exercises the ordinary UI loop;
it is runtime/liveness evidence, not a human visual-layout acceptance result.

## Fixed-step overhead measurement

Six sequential, alternating-order runs used the same final executable, clean
source state, `mixed-medium` generator, 30 warmup steps, 120 measured steps per
pass and five passes. Each run was deterministic internally and all six used
initial FNV-1a state hash `3682553620` and final hash `798083730`.

| Pair | OFF steps/s | ON steps/s | OFF -> ON throughput delta | OFF -> ON latency delta |
|---:|---:|---:|---:|---:|
| 1 | 178.606362 | 170.277337 | +4.663% | +4.892% |
| 2 | 180.460973 | 184.960829 | -2.493% | -2.433% |
| 3 | 178.668217 | 181.692796 | -1.693% | -1.665% |

The median paired throughput delta is `-1.693%` and the observed range is
`-2.493%` through `+4.663%`. It therefore records profiler overhead as measured
but does **not** prove a speedup, establish a noise model, or pass a performance
regression budget. Raw JSON, process/RAM series and logs are private ignored
artifacts under `artifacts/vnext-benchmark/final-1.0.1-rebuilt/`.

## Compatibility and declared limits

- Classic simulation semantics, Element IDs, save schema and existing Lua APIs
  were not intentionally changed by this milestone.
- The profiler is disabled by default and releases its optional frame-time owner
  when neither the legacy frame HUD nor the profiler requires it.
- Visual UI acceptance, portable-runtime validation, per-process VRAM and a
  full external Lua corpus remain `not_tested`.
- The two attempted independent profiler reviewers ended with external service
  `504` failures and generated no auditable review. This is recorded as
  `not_available`, not PASS. A separate correction-observer review remains
  `PASS_WITH_DECLARED_LIMITS` only for that observer's scoped Air-cap evidence.
- This milestone does not add physical mass, energy, species, atom or charge
  conservation. Global G0 and production OmniAtmosphere remain RED.

## Gate and rollback

The defined 1.0.1 gate is GREEN: build, static tests, isolated runtime behavior,
threaded-rendering lifetime evidence, profiler OFF/ON measurement and fixed-step
benchmark provenance all exist. This authorizes the independent 1.0.2 upstream
compatibility stage, not UI/material reorganization or any OmniAtmosphere work.

Rollback is the parent of `2b6b6e39e` (`ca3d643c8`). Reverting the two profiler
commits removes only the diagnostics foundation and its test fixtures.
