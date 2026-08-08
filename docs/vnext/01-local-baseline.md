# Local baseline

## Repository identity

| Field | Value |
|---|---|
| Workspace | `D:/CodexWork/OmniPack/repos/TPT-ZH-OmniPack` |
| User junction | `C:/Users/KR/TPT-ZH-OmniPack` |
| Pre-vNext OmniPack | `fb72d5e8f` |
| Original common base | `bff38ce6959e1c1a7a4d17d0d5d44d127a0dfcbd` |
| Benchmark implementation commit | `c4490463f3819695cab734414f827e0e4e4be118` |
| Characterization implementation commit | `1b8586877e6e7d3703ecd1dbab1eb89b3e4eb7a2` |
| Differential implementation commit | `c6eecaa77cd7d6025ef997c5dd46e53d112c6e08` |
| Report commit | `SELF` |
| Stable/master upstream | `d768aeb89acad986bd252d7e904bf44bb374545f` |
| Particle layout | 56-byte AoS, `NPART=235008` |
| Resolution | 612 x 384 particles; 153 x 96 Air cells; `CELL=4` |
| Graphics/input dependency | SDL2 `2.30.9-tpt-libs` |

## Current build artifacts

The original `build-vnext-g0-clean` build completed `787/787` actions. Current
formal fixed-step, characterization and differential evidence uses
`build-vnext-fp-mode-legacy` and `build-vnext-fp-mode-strict`. The formal Legacy
characterization executable was relinked `9/9` at clean commit `1b8586877` with
Windows Git first on PATH. The tooling-only first-divergence commit reused the bound
executables below and recorded its own clean source/tool hashes.

| Mode | Executable bytes | SHA-256 |
|---|---:|---|
| Legacy-fast | 321,335,036 | `41FE38FB76C0F4323DA9109B6C3616B40F423FFF12274A0010061F87A232BE6D` |
| Strict | 321,354,807 | `2BD30C0112793EADD5316CD640262873BDA6F999C8935894DE73D1F1D40620BC` |

Both development executables require `C:/msys64/ucrt64/bin` on PATH. VCS tag
generation is disabled in these development directories, so the executable hash,
clean source commit, Meson introspection and exact compile command form the recorded
local binding; portable/reproducible output is not claimed.

GCC emitted a `-Wmaybe-uninitialized` warning in the `ByteString`/`optional` path in
`PowderToy.cpp`; this is recorded as `needs_triage`, not as a proven runtime defect.

## Test evidence

| Check | Result | Scope |
|---|---:|---|
| Meson registered tests | Legacy `39/39`; Strict `39/39 PASS` | current explicit FP builds |
| Nested Python unit tests | 281 run: 281 passed, 0 skipped, 0 failed | current HEAD with UCRT64 compiler on PATH; includes 8 differential tests |
| Lua module regression | `PASS` | isolated client process |
| Lua 100.1 boundary regression | `11/11 PASS` | reset pressure/velocity/neighbors boundaries |
| OPS scenario cases | `8 PASS` | fixed scenario set |
| OPS processes | `24` | three processes per scenario |
| OPS restarts/load verifications | `16 / 16` | two restart loads per scenario |
| OPS particles | `368` total | scenario aggregate |
| OPS field assertions | `436` per load | stable identifier and carried fields |
| Element inventory check | `PASS` | 488 elements, 431 bindings, 164 roots |
| GUI visual acceptance | `not_tested` | no visible interactive review |
| PSv/fuC fixtures | `not_tested` | current fixtures are OPS1 |
| Portable clean-machine runtime | `not_tested` | compile is not portability proof |
| Broad third-party Lua corpus | `not_tested` | only bounded local regressions |
| Current uncapped throughput | `4/4 PASS` | empty/mixed, Legacy/Strict, seven passes each |
| Same-build deterministic replay | `4/4 PASS` | generated/initial/final Snapshot signatures repeat |
| Process RAM | `4/4 measured` | peak working set/private bytes |
| Process VRAM | `not_tested` | GPU capacity inventory is not process usage |
| Conservation/numerical drift | `not_tested` | no OmniCore solver exists |
| C01-C14 characterization | `14/14 PASS` | clean source, 42 processes, 28 restart loads |
| Characterization restart traces | `14/14 equal` | loaded baseline, fixed seed and frame count |
| Characterization process RAM | `42/42 measured` | 133,210,112-186,638,336 peak working-set bytes |
| First-divergence trace/capture | `PASS` | same Legacy source, Legacy-fast CPU versus Strict CPU |
| First divergent step | `1` | equal step-0 hash/RNG; Particle and Air fields differ after first update |
| Differential artifact integrity | `24/24 PASS` | manifest lengths/SHA-256 plus byte-identical trace/capture recomputation |
| Legacy CPU vs Omni CPU | `not_implemented` | no Omni solver exists |
| Omni CPU vs Omni GPU | `not_implemented` | no GPU compute backend exists |

Formal throughput is 1,682.069895/1,655.224824 steps/s for empty and
189.868348/189.808193 steps/s for mixed Legacy/Strict. Mixed starts from the same
generated hash but diverges by 60 warmup steps, so this is a throughput baseline and
numerical-risk signal, not proof of FP equivalence. See
`phase-1-fixed-step-benchmark.md`. The later field runner locates the first mixed
divergence at step 1; see `phase-1-first-divergence.md`.

## G0 gate

```text
BUILD=GREEN
STATIC_TESTS=GREEN
LUA_BASIC=GREEN
LUA_100_1_BOUNDARIES=GREEN
OPS_ROUNDTRIP=GREEN
LATEST_STABLE_CODE_PRESENT=GREEN
CRITICAL_UPSTREAM_FIXES_PRESENT=GREEN
GUI_VISUAL=NOT_TESTED
PSV_FUC=NOT_TESTED
PORTABLE_RUNTIME=NOT_TESTED
FIXED_STEP_RUNNER=GREEN
CURRENT_UNCAPPED_BENCHMARK=GREEN
CURRENT_PROCESS_RAM_BASELINE=GREEN
CURRENT_PROCESS_VRAM_BASELINE=RED
PERFORMANCE_REGRESSION_BUDGET=RED
CHARACTERIZATION_SAVES=GREEN
STRICT_FP_NUMERICAL_BASELINE=RED
FIRST_DIVERGENCE_RUNNER=GREEN
SAME_SOURCE_CPU_FP_DIFFERENTIAL=GREEN
LOAD_BOUNDARY_FIELD_DIFF=RED
CONSERVATION_FINITE_LEDGER=RED
LEGACY_CPU_VS_OMNI_CPU=RED
OMNI_CPU_VS_OMNI_GPU=RED
G0_UPSTREAM_BASELINE=RED
```

The gate is fail-closed: benchmark, characterization and same-source CPU
first-divergence foundations are GREEN, but they do not substitute for conservation
and finite-value ledgers, load-boundary field evidence, subsystem profiling, VRAM
measurement or Omni CPU/GPU differential validation. See
`phase-1-legacy-characterization.md` and `phase-1-first-divergence.md`.
