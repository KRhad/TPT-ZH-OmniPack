# Build, profiler and performance baseline

## Current compiler configuration

The validated build is GCC/UCRT64, Meson `debugoptimized`, `optimization=2`, debug
symbols enabled, `lto=false`, `static=prebuilt`, and `x86_sse=auto` resolving to SSE2.
Its compile commands include:

```text
-O2
-ftree-vectorize
-funsafe-math-optimizations
-ffast-math
-fomit-frame-pointer
-msse2
```

The non-debug Meson path also adds those unsafe/fast math flags; release may use
`-O3`. MSVC uses the analogous fast FP mode. No AVX, `-march=native`, OpenMP or
general parallel particle loop was found. Threads are used in bounded areas such as
rendering, FFT gravity and background/network work.

`debugoptimized` is not a strict-FP configuration.

## Fast-math decision

```text
OMNICORE_FAST_MATH_ALLOWED=false
```

The conservative atmosphere, thermodynamics and chemistry targets must not inherit
global fast-math until strict and fast builds compare conservation, positivity,
NaN/Inf behavior and performance. In particular, finite-value assumptions can make
NaN/Inf checks unreliable under aggressive optimization.

The clean mixed-medium differential now shows that Legacy-fast and Strict first
diverge on update step 1: 4,935 particle IDs and 2,074 Air cells already differ.
This materially confirms sensitivity to the FP contract but does not identify the
correct result. No physical mass, momentum, energy or positivity ledger exists. A
later 1,000-step tool finds no non-finite or Legacy-range violation in 102 sampled
exported states, but does not observe unsampled ticks or physical conserved
quantities. `OMNICORE_FAST_MATH_ALLOWED=false` therefore remains fail-closed.

Required matrix, keeping `-O2`, SSE2 and `lto=false` constant initially:

1. double, strict FP reference;
2. float32, strict FP;
3. scaled float32, fast FP.

GCC strict flags must at least counter global settings with `-fno-fast-math`,
`-fno-unsafe-math-optimizations` and `-ffp-contract=off`; MSVC uses `/fp:strict`.

## Existing profiler limits

Current `FrameTime` has only seven relevant spans: frame, controller/model update,
before/after simulation, free-particle recalculation and Air update. It uses a
high-resolution clock, dynamic vector/map work and a HUD EWMA. It lacks raw export,
percentiles, thread aggregation and dedicated timings for particle updates, ambient
heat, gravity wait, Lua, rendering copy, chemistry, GPU passes and synchronization.
Frame time also includes presentation/wait/limiting and is not pure simulation time.

The replacement profiler should use `steady_clock`, stable numeric span IDs,
allocation-free thread-local events, inclusive/exclusive aggregation and structured
p50/p95/p99/max output.

## Benchmark status

```text
CURRENT_HEAD_THROUGHPUT_BASELINE=true
UNCAPPED_FIXED_STEP_RUNNER=true
SAME_BUILD_DETERMINISTIC_REPLAY=true
FIRST_DIVERGENCE_FIELD_CAPTURE=true
LEGACY_SAMPLED_FINITE_PROXY_LEDGER=true
STRICT_FAST_SAMPLED_PROXY_COMPARISON=true
STRICT_FAST_CONSERVATION_COMPARISON=false
UNSAMPLED_FULL_STATE_FINITE=false
SUBSYSTEM_TIMING_BASELINE=false
CURRENT_PROCESS_RAM_BASELINE=true
CURRENT_PROCESS_VRAM_BASELINE=false
PERFORMANCE_REGRESSION_BUDGET=false
BENCHMARK_FOUNDATION_GATE=GREEN
PERFORMANCE_GATE=RED
```

The existing stress harness is valuable for crashes, hangs, bounded growth, OPS and
scenario invariants. Its historical roughly 60 FPS values are limited by the default
FPS cap and cannot establish throughput or speedup. Its `performance_gate_pass`
currently expresses stability conditions, not a measured regression budget; future
reports should separate stability, performance and numerical gates.

Official `tpt-bench` was inspected at commit
`099b858260421cff42f3553b7e0680b80af3dddf`. It has one dust scenario, mean/stddev
output, hidden preference sensitivity, no structured provenance, and has no LICENSE
file at that commit. Its lowercase `socket.gettime()` call is compatible with this
client because the built-in `compat.lua` explicitly assigns
`socket.gettime = socket.getTime`; the earlier name-mismatch conclusion was wrong.
Runtime execution of upstream `tpt-bench` remains `not_tested`, and it stays
`REFERENCE_ONLY`; its Lua/save content was not copied.

## Fixed-step runner implementation

`tools/runtime_fixed_step_benchmark.ps1` and
`tools/runtime/fixed_step_benchmark.lua` now implement the foundation contract. Each
run records source commit/dirty state, executable hash, exact `Simulation.cpp`
compile command, Meson options, compiler, OS/CPU/GPU/driver/RAM, power mode, backend,
generator/config hashes, seed, fixed-step count and Legacy tick `dt`. Raw evidence is
kept under ignored `artifacts/vnext-benchmark/`.

The timed region is a synchronous loop of `sim.updateUpTo()` calls. It excludes
startup, FPS/draw waits, rendering, scene generation, cache sanitation, warmup, GC,
state hashing and JSON work. It includes one Lua-to-C dispatch per fixed step and is
therefore a client Simulation benchmark, not a pure C++ kernel benchmark.

Same-process replay initially exposed that `clear_sim()` does not clear the derived
Air blocking maps. The runner explicitly records one empty-world sanitation step
outside timing before the actual scene reset. Production behavior was not changed.

Formal clean-source baseline at commit `c4490463f`:

| Scene / mode | Steps | Steps/s | ms/step | Peak working set bytes |
|---|---:|---:|---:|---:|
| empty / Legacy-fast | 7,000 | 1,682.069895 | 0.594506 | 132,689,920 |
| empty / Strict | 7,000 | 1,655.224824 | 0.604148 | 132,849,664 |
| mixed-medium / Legacy-fast | 2,100 | 189.868348 | 5.266807 | 155,041,792 |
| mixed-medium / Strict | 2,100 | 189.808193 | 5.268476 | 155,037,696 |

All four executions were clean-source and deterministic across seven passes within
their own build. Empty produced equal Strict/Legacy final hashes. Mixed began from
the same generated hash but diverged by the end of 60 warmup steps and ended with
different hashes and particle counts. Its strict/fast throughput delta is therefore
not a same-trajectory performance comparison. Full evidence and artifact hashes are
in `phase-1-fixed-step-benchmark.md`. The later non-timed differential runner locates
the first divergence at step 1; see `phase-1-first-divergence.md`.

Required counters/timings include Frame, Simulation, Particle, Air, AmbientHeat,
Gravity dispatch/wait, Thermal, Chemistry, Lua before/after, RenderSnapshotCopy,
Rendering, GPU upload/pass/readback/fence, particles, cells, chunks, reaction
candidates, CFL/substeps/floor hits/corrections, RSS/private bytes, VRAM and the
conservation ledger.

## Characterization set

The self-generated, source-controlled C01-C14 set is complete:

`sand`, `water`, `gas`, `fire`, `explosion`, `vacuum`, `pressure`, `heat`,
`electrical`, `photons`, `PIPE`, `complex-electronics`, `mixed`, and
`maximum-particle`.

Each fixes generator/save SHA-256, provenance/license, seed, world settings, warmup
and measurement steps, expected invariants and Legacy trace/hash. The clean formal
run is `14/14 PASS`; its raw saves and traces stay private under ignored `artifacts/`.
No private user save or unlicensed tpt-bench content is used.

## First-divergence status

The clean `c6eecaa77` same-source CPU comparison traces steps 0-120, reruns the
first differing step and exports every active Particle plus `pv/vx/vy/hv`, wall,
electrical, fan and gravity cells. Step 0 is equal and step 1 differs in Particle
velocity and Air state with `unexplained_hash_divergence=false`. The formal manifest
sets `performance_gate=not_evaluated`; its four short process timings are evidence-
collection diagnostics, not a throughput benchmark.

Legacy CPU versus future Omni CPU and Omni CPU versus future Omni GPU remain
unimplemented. The OPS load-boundary and all-tick exported-float sub-gates are
GREEN; physical conservation accounting and internal/full-state finite evidence
remain RED.

## OPS load-boundary field attribution

The clean `9b336fc40` formal run exports Particle payloads, Air/wall/fan/gravity
cells and simulation settings immediately before saving and immediately after two
fresh loads. It completed C01-C14, including photons, PIPE and full allocator
saturation. The 369-file private artifact is hash-closed, loaded-A/B captures are
byte-identical and the frozen comparator replays all 14 comparison JSON files
byte-identically. This is a save-compatibility characterization only: pre-save and
loaded Snapshot hashes remain unequal, runtime Particle IDs are diagnostic only,
and mass/momentum/energy/source-sink claims are explicitly false. See
`phase-1-load-boundary.md`.

The CSV export and comparison are intentionally not included in the fixed-step
throughput baseline. Their process timings and C14 memory are evidence-collection
costs, not a simulation speed claim.

## All-tick exported-float ledger

The clean `632665650` run uses `SampleInterval=1`, so it scans step 0 and every
post-update state through step 1,000: 1,001 records per Legacy-fast/Strict mode.
All active Particle `x/y/vx/vy/temp` and every Air `pv/vx/vy/hv` record is finite
and within the Legacy range contract; the scope-aware comparator explicitly emits
`all_tick_post_update_exported_fields=true` and `sampled_states_only=false`.

This is deliberately narrower than full state: private Air scratch planes,
fan/gravity/wall auxiliary state, internal subphases and correction events are not
observed, while signed Legacy `pv` is not physical pressure positivity. The two
probe wall times (47.501361/47.919276 seconds) and sampled process working sets
(139,460,608/138,895,360 bytes) are evidence-collection costs, not a benchmark.
See `phase-1-all-tick-ledger.md`.

## Legacy sampled proxy-ledger status

The clean `b3aa56cf3` ledger compares Legacy-fast and Strict over 1,000 mixed-scene
updates at steps `0`, `1`, every tenth step and `1000`. Both sides report zero
sampled non-finite, range-violation and exact-bound observations across active
Particle `x/y/vx/vy/temp` and every Air cell's `pv/vx/vy/hv`. Frozen CSV replay is
byte-identical to the stored 13,848-byte comparison JSON.

This extends diagnosis without changing the fast-math decision. State/proxy metrics
split at step 1, RNG and type histograms at sampled step 40, and total particle count
at sampled step 90. Final counts are 50,471 and 50,477. The tool deliberately marks
physical mass, momentum, energy, source/sink attribution and correction events as
`not_evaluated`; unsampled ticks and physical pressure positivity are also absent.
See `phase-1-legacy-ledger.md`.

## Memory baseline

- fixed particle array: `235008 * 56 = 13,160,448` bytes (`12.551 MiB`);
- pmap plus photons: `1,880,064` bytes (`1.793 MiB`);
- Legacy Air current/scratch/block/fan planes: `616,896` bytes (`0.588 MiB`);
- a renderer snapshot's particle/maps/current Air/walls lower bound is about
  `14.596 MiB` before gravity, signs and other state.

At 60 copies/second that lower bound is roughly 876 MiB/s, so future species planes
must not be blindly copied into renderer snapshots. Debug views should copy only the
selected derived plane and profile `RenderSnapshotCopy`.

## Gate

Build/test capability, the explicit Strict build, fixed-step runner, current
two-scene throughput/process-RAM baseline, C01-C14 characterization, scoped
first-divergence capture, OPS field attribution and all-tick exported-float ledger
are GREEN. Subsystem profiling, process VRAM, accepted performance budgets, physical
conservation/source/correction accounting and internal/full-state finite/positivity
comparison remain RED. The next G0 work is the physical Legacy ledger or subsystem
profiler/VRAM export;
production OmniAtmosphere remains blocked.
