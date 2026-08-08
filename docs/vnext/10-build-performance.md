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
CURRENT_HEAD_THROUGHPUT_BASELINE=false
UNCAPPED_FIXED_STEP_RUNNER=false
SUBSYSTEM_TIMING_BASELINE=false
CURRENT_RAM_VRAM_BASELINE=false
BENCHMARK_GATE=RED
```

The existing stress harness is valuable for crashes, hangs, bounded growth, OPS and
scenario invariants. Its historical roughly 60 FPS values are limited by the default
FPS cap and cannot establish throughput or speedup. Its `performance_gate_pass`
currently expresses stability conditions, not a measured regression budget; future
reports should separate stability, performance and numerical gates.

Official `tpt-bench` was inspected at commit
`099b858260421cff42f3553b7e0680b80af3dddf`. It has one dust scenario, mean/stddev
output, hidden preference sensitivity, no structured provenance, calls
`socket.gettime()` while this client exposes `socket.getTime()`, and has no LICENSE
file at that commit. It is `REFERENCE_ONLY`; its Lua/save content must not be copied.

## Fixed-step runner contract

Each run must record source commit/dirty state, executable hash, complete flags,
compiler, OS/CPU/GPU/driver/RAM, power mode, backend, save/generator hash, seed,
fixed step count and simulation `dt`. It must disable draw/FPS limiting and isolate
the kernel from rendering while retaining a separate end-to-end profile.

Required counters/timings include Frame, Simulation, Particle, Air, AmbientHeat,
Gravity dispatch/wait, Thermal, Chemistry, Lua before/after, RenderSnapshotCopy,
Rendering, GPU upload/pass/readback/fence, particles, cells, chunks, reaction
candidates, CFL/substeps/floor hits/corrections, RSS/private bytes, VRAM and the
conservation ledger.

## Characterization set

Create self-generated, source-controlled manifests for C01-C14:

`sand`, `water`, `gas`, `fire`, `explosion`, `vacuum`, `pressure`, `heat`,
`electrical`, `photons`, `PIPE`, `complex-electronics`, `mixed`, and
`maximum-particle`.

Each fixes generator/save SHA-256, provenance/license, seed, world settings, warmup
and measurement steps, expected invariants and Legacy trace/hash. Do not use private
user saves or the unlicensed tpt-bench save.

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

Build/test capability is GREEN. Profiler, strict-FP and current benchmark foundations
are RED and are immediate G0 blockers.
