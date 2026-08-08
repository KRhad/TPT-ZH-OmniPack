# Phase 1 fixed-step client benchmark

## Outcome

```text
BASE_COMMIT=103a6752d4c0a9addedc6d55229b0db5f8691a9f
IMPLEMENTATION_COMMIT=c4490463f3819695cab734414f827e0e4e4be118
REPORT_COMMIT=SELF
FIXED_STEP_RUNNER=GREEN
CURRENT_HEAD_THROUGHPUT_BASELINE=GREEN
SAME_BUILD_DETERMINISTIC_REPLAY=GREEN
CURRENT_PROCESS_RAM_BASELINE=GREEN
PROCESS_VRAM_BASELINE=RED
SUBSYSTEM_TIMING_BASELINE=RED
PERFORMANCE_REGRESSION_BUDGET=RED
STRICT_FAST_NUMERICAL_COMPARISON=RED
G0_UPSTREAM_BASELINE=RED
```

The client now has an independent fixed-step benchmark foundation. It runs the real
Legacy CPU `Simulation` path synchronously through `sim.updateUpTo()`, without the
FPS limiter or renderer in the timed region. It is not a pure C++ microbenchmark:
each measured step includes one Lua-to-C dispatch plus the complete client
BeforeSim/particle/AfterSim path.

This phase changes no Simulation, Particle, Air, element, Lua API, Save or production
data structure.

## Files and rollback

- `tools/runtime/fixed_step_benchmark.lua`: deterministic `empty` and
  `mixed-medium` generators, warmup, fixed-step timing and Snapshot hash capture.
- `tools/runtime_fixed_step_benchmark.ps1`: isolated client launch, source/build/
  machine provenance, process RAM sampling and structured JSON output.
- `tools/tests/test_fixed_step_benchmark.py`: eight fail-closed source-contract
  checks for timing boundaries, isolation, provenance and result semantics.

The rollback point is `103a6752d`. Reverting `c4490463f` removes only benchmark
tooling and its tests. No save/data migration is involved.

## Timing and state contract

For every pass, the runner fixes four RNG words, settings, generated geometry,
warmup steps and measured steps. Scene generation, cache sanitation, warmup, Lua GC,
particle counting, Snapshot hashing, JSON generation and process startup are outside
the timed region. The timed body is exactly:

```lua
local started = socket.getTime()
for _ = 1, steps_per_pass do
    sim.updateUpTo()
end
local elapsed_seconds = socket.getTime() - started
```

`sim.hash()` is the existing 32-bit FNV-1a hash of Air pressure/velocity/heat,
particles, gravity fields, walls/Air blocking maps, fans, portals, wireless state,
stickmen, frame count and simulation RNG. It is a sensitive state signature, not a
cryptographic content hash and not a conservation ledger.

The isolated profile sets `tpt.fpsCap(2)` (no simulation cap), draw cap 1, and a
900,000 ms Lua watchdog timeout so a synchronous fixed-step pass is not interrupted
by the interactive three-second watchdog. The executable still needs
`C:/msys64/ucrt64/bin` on PATH; portable runtime remains `not_tested`.

## Legacy Air cache finding

An initial mixed-scene smoke run produced equal particle counts but different
generated-state hashes on pass two. Source inspection showed that `clear_sim()`
clears particles and the principal Air fields but not derived
`bmap_blockair/bmap_blockairh` maps. The previous pass therefore contaminated the
next pass before measurement.

The benchmark now performs one empty fixed step followed by a second clear before
generating each scene. This rebuilds those Legacy derived maps to the empty-world
state. The JSON records `reset_sanitization_steps_per_pass=1` and
`reset_sanitization_in_timed_region=false`. Production `clearSim()` semantics were
not changed. Other runtime/characterization tools must not assume that `clearSim()`
alone creates a complete pristine Snapshot state.

## Formal build and machine provenance

All four formal runs used clean source commit `c4490463f`, GCC 16.1.0,
`debugoptimized`, `-O2`, SSE2, `lto=false`, prebuilt static dependencies and the
Balanced Windows power plan on machine ID `windows-276049E7945C`:

```text
CPU=Intel(R) Core(TM) Ultra 9 275HX
PHYSICAL_CORES=24
LOGICAL_CPUS=24
RAM_BYTES=33784102912
GPU=NVIDIA GeForce RTX 5070 Ti Laptop GPU
GPU_DRIVER=591.86
GPU_VRAM_CAPACITY_BYTES=12820938752
```

GPU capacity is machine inventory only. Per-process VRAM was deliberately not polled
because doing so would perturb these short CPU runs; it remains `not_tested`.

| Build | Bytes | Executable SHA-256 |
|---|---:|---|
| Legacy-fast | 321,336,060 | `4644B9C9337FF3A1049C9509C3D6A0A760835E4F4459A617456EC7942E998E1E` |
| Strict | 321,353,783 | `77FDAFCF8C37E31EF7D273FC2D6DFFDA5E4022D725F16BB2A7176B0D8FF593A4` |

The JSON contains the exact `Simulation.cpp` compile command, all Meson options,
compiler identity, executable hash, source status, Lua source hash, run-config hash,
OS/CPU/GPU/RAM and power scheme. These hashes identify local artifacts; they do not
claim reproducible PE output.

## Throughput baseline

Each mode used the same configuration hash for a given scene. Seven same-process
passes independently rebuilt the scene; every pass within a build produced the same
generated, initial and final Snapshot signatures.

| Scene / mode | Warmup | Measured steps | Total seconds | Steps/s | ms/step | Pass p50 / p95 s |
|---|---:|---:|---:|---:|---:|---:|
| empty / Legacy-fast | 120/pass | 7,000 | 4.161539317 | 1,682.069895 | 0.594506 | 0.595177174 / 0.600468302 |
| empty / Strict | 120/pass | 7,000 | 4.229032756 | 1,655.224824 | 0.604148 | 0.601495028 / 0.618735934 |
| mixed-medium / Legacy-fast | 60/pass | 2,100 | 11.060295344 | 189.868348 | 5.266807 | 1.564662695 / 1.658913064 |
| mixed-medium / Strict | 60/pass | 2,100 | 11.063800573 | 189.808193 | 5.268476 | 1.581354141 / 1.616711902 |

Strict throughput relative to Legacy-fast was `-1.595955%` for empty and
`-0.031682%` for mixed-medium in this single alternating-order snapshot. There is no
accepted noise model, thermal control or regression budget, and the mixed state
trajectories differ, so these deltas are observations rather than speed claims.

## Strict versus Legacy state evidence

| Scene | Generated hash | State after warmup | State after measured steps | Final particles |
|---|---:|---|---|---|
| empty | both `1134739233` | both `214105246` | both `2786562981` | both 0 |
| mixed-medium | both `3129219535` | Legacy `2028052342`; Strict `898704571` | Legacy `2075078834`; Strict `1815522718` | Legacy 51,608; Strict 51,263 |

The mixed scene starts from an identical generated Snapshot and has 58,868 particles
after 60 warmup steps in both modes, but its state hash has already diverged. After
300 measured steps its final particle counts also differ. This does not identify a
bug or quantify physical error in a chaotic Legacy particle scene. It does prove that
build viability is not numerical equivalence. First-divergence state capture,
conservation/positivity/NaN metrics and an OmniCore ledger remain required.

## Process memory evidence

| Scene / mode | Peak working set bytes | Peak private bytes sampled | Process VRAM |
|---|---:|---:|---|
| empty / Legacy-fast | 132,689,920 | 132,968,448 | `not_tested` |
| empty / Strict | 132,849,664 | 135,102,464 | `not_tested` |
| mixed-medium / Legacy-fast | 155,041,792 | 143,667,200 | `not_tested` |
| mixed-medium / Strict | 155,037,696 | 143,417,344 | `not_tested` |

Sampling interval was 50 ms. These are whole-process development-client baselines,
not bytes-per-particle or bytes-per-cell ownership measurements.

## Private raw evidence

Raw configs, Lua results, process series, client logs and JSON are under ignored
`artifacts/vnext-benchmark/`; none was committed, pushed or uploaded. Result identity:

| Run | Result JSON SHA-256 |
|---|---|
| `20260808T205402Z-df748b8a` empty Legacy | `C90B54693E946ADD3EDACE6E7F5BCD6F5F43DDA2304B026727932C241F366E76` |
| `20260808T205426Z-7fc9d26e` empty Strict | `5C78DCD0CECEC3D7BAB176B7641862E922FA4D722F5D39E2E3880CBFE512A45F` |
| `20260808T205448Z-707260ce` mixed Strict | `5E9D7B9A22A9A1245A01ED1605683DAFCBBC2621C1BBB8CCE655EC54F57A3810` |
| `20260808T205537Z-b9a32a0d` mixed Legacy | `A1A75F3ACD1979CBEF6DCCD87D56388080EEB18C5B38A70C8BA14D1FE08EB3F0` |

The common Lua source SHA-256 is
`A24BEAFAA12AC471EF8D65D492832262A9CCFE8A3BB2F1F32A72360B5EFA2656`.
Formal-result validation found `4/4 PASS`, `4/4 clean`, `4/4 deterministic`, `4/4`
with nonzero RAM metrics and `0/4` with measured process VRAM.

## Tests, compatibility and gate

- PowerShell parser: `PASS`.
- Fixed-step source-contract tests: `8/8 PASS`.
- Full Python suite: 256 run, 254 passed, 2 skipped, 0 failed.
- Legacy-fast Meson suite: `39/39 PASS`.
- Strict Meson suite: `39/39 PASS`.
- Legacy/Strict mixed and empty client executions: `4/4 PASS`.
- GUI visual, portable runtime, subsystem profiler and VRAM: `not_tested`.

The fixed-step runner and current two-scene throughput/RAM baseline are GREEN. The
performance gate remains RED because it has no accepted budgets or controlled repeat
matrix. Strict/fast numerical comparison remains RED. C01-C14 characterization,
first-divergence/ledger tooling and profiler export remain G0 blockers, so production
OmniAtmosphere work stays prohibited.
