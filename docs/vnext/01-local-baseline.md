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
| Legacy ledger implementation commit | `8bd640c3e2aca501a17987aa462b2489901e6694` |
| Byte-stable ledger replay fix | `b3aa56cf3914ab18da60d1ac9ff1e492377f6e88` |
| Load-boundary implementation commit | `971687a24` |
| Load-boundary artifact-closure commit | `a09c6d716` |
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
Windows Git first on PATH. The final Legacy ledger run at clean commit `b3aa56cf3`
used Ninja to build both targets immediately before probing, then re-hashed the
executables and build provenance after execution.

The final OPS load-boundary run used a new clean detached source worktree at
`9b336fc40`, its own `builds/ledger-9b336-legacy` Legacy-fast directory and
Windows PowerShell 5. It built `786/786` targets plus the expected always-stale
generated `9/9` relink, then re-hashed the executable and Meson provenance after
the C01-C14 run. Its executable is 321,491,196 bytes with SHA-256
`AD4BE609CB25A58943F858FE39EACE2D2481DD242F4DB5C6E1B0A8321152126B`.

| Mode | Executable bytes | SHA-256 |
|---|---:|---|
| Legacy-fast | 321,484,028 | `8828592F758563B9D025064CA25F441A6BD267EF627ABFD920EEBEB18037DC85` |
| Strict | 321,504,311 | `319C0AB8D0CCFEAB3727BF9FA827E82B5F1493AC8EE948B3272F24706C819145` |

Both development executables require `C:/msys64/ucrt64/bin` on PATH. VCS tag
generation is disabled in these development directories, so the executable hash,
clean source commit, Meson introspection, 754 matched compile commands per mode and
exact execution-tool hashes form the recorded local binding. The executable does
not cryptographically embed the commit; portable/reproducible output is not claimed.

GCC emitted a `-Wmaybe-uninitialized` warning in the `ByteString`/`optional` path in
`PowderToy.cpp`; this is recorded as `needs_triage`, not as a proven runtime defect.

## Test evidence

| Check | Result | Scope |
|---|---:|---|
| Meson registered tests | formal Legacy `39/39 PASS`; historical Strict `39/39 PASS` | explicit FP builds |
| Nested Python unit tests | `321/321 OK` | formal clean load-boundary source with UCRT64 compiler on PATH |
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
| Legacy sampled proxy ledger | `PASS` | clean 1,000-step Legacy-fast versus Strict, 102 samples each |
| Sampled exported finite/range | `true / true` both modes | Particle `x/y/vx/vy/temp` plus all Air `pv/vx/vy/hv` at scheduled samples |
| Ledger nonfinite/range/bound observations | `0 / 0 / 0` both modes | sampled observations; not internal correction events |
| Ledger artifact integrity | `18/18 PASS` | 19 files/0 directories, manifest lengths/SHA-256, 44/44 DLLs, 754/754 compile commands |
| Frozen ledger comparison replay | byte-identical `PASS` | 13,848 bytes, SHA-256 `54D3B2BB...FBC1` |
| OPS load-boundary field attribution | `14/14 PASS` | before-save versus loaded-A fields; loaded-A/B captures are byte-identical |
| Load-boundary artifact integrity | `369/369 PASS` | plus manifest, 15 directories, frozen inputs and result/capture references |
| Frozen load-boundary comparator replay | byte-identical `14/14 PASS` | no artifact writes or `__pycache__` |
| Physical mass/momentum/energy conservation | `not_evaluated` | proxy sums are not physical units |
| Source/sink and correction attribution | `not_evaluated` | no event ledger exists |
| Unsampled/full-state finite and pressure positivity | `not_tested` | sampled Legacy range is not physical positivity |
| Legacy CPU vs Omni CPU | `not_implemented` | no Omni solver exists |
| Omni CPU vs Omni GPU | `not_implemented` | no GPU compute backend exists |

Formal throughput is 1,682.069895/1,655.224824 steps/s for empty and
189.868348/189.808193 steps/s for mixed Legacy/Strict. Mixed starts from the same
generated hash but diverges by 60 warmup steps, so this is a throughput baseline and
numerical-risk signal, not proof of FP equivalence. See
`phase-1-fixed-step-benchmark.md`. The later field runner locates the first mixed
divergence at step 1; see `phase-1-first-divergence.md`.

The final Legacy ledger keeps that step-1 result and extends the trajectory to 1,000
steps. RNG and type histograms first differ at sampled step 40, particle count first
differs at sampled step 90, and the final counts are 50,471 versus 50,477. Both modes
have zero sampled nonfinite/range/bound observations. See
`phase-1-legacy-ledger.md`; none of its proxy drift is a conservation result.

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
LEGACY_SAMPLED_FINITE_PROXY_LEDGER=GREEN
STRICT_FAST_SAMPLED_PROXY_COMPARISON=GREEN
UNSAMPLED_FULL_STATE_FINITE=RED
PHYSICAL_CONSERVATION_LEDGER=RED
SOURCE_SINK_CORRECTION_LEDGER=RED
LOAD_BOUNDARY_FIELD_DIFF=GREEN
CONSERVATION_POSITIVITY_FULL_STATE_FINITE_LEDGER=RED
LEGACY_CPU_VS_OMNI_CPU=RED
OMNI_CPU_VS_OMNI_GPU=RED
G0_UPSTREAM_BASELINE=RED
```

The gate is fail-closed: benchmark, characterization and same-source CPU
first-divergence, sampled proxy-ledger and load-boundary field-attribution
foundations are GREEN, but they do not substitute for physical conservation/source/
correction accounting, unsampled/full-state finite/positivity evidence, subsystem
profiling, VRAM measurement or Omni CPU/GPU differential validation. See
`phase-1-legacy-characterization.md`, `phase-1-first-divergence.md` and
`phase-1-legacy-ledger.md`; load-boundary scope and evidence are in
`phase-1-load-boundary.md`.
