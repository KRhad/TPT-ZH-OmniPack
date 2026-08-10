# OmniCore vNext orchestrator state

## Outcome

```text
REPORT_DATE=2026-08-10
CURRENT_BRANCH=integration/omnicore-vnext
CURRENT_VERSION=1.0.4
CURRENT_VERSION_GATE=GREEN
NEXT_VERSION=1.0.5
NEXT_PHASE=refresh upstream, then define PhysicalScale and isolated AtmosphereBench candidates only
PROFILER_IMPLEMENTATION_HEAD=97d2fc2c175818a66636421526e4f562d4d1de01
PROFILER_VALIDATED_EXECUTABLE_SHA256=EA2C8517771E615D6DFC86B9E3AFD5A77F0D49F8F0FE3F8635E6AF21D02B279D
PROFILER_RUNTIME_AND_CONCURRENCY=GREEN
PROFILER_OVERHEAD_MEASURED=GREEN
INDEPENDENT_PROFILER_REVIEW=not_available
UPSTREAM_REFRESH_DATE=2026-08-10
UPSTREAM_REFRESH=GREEN_NO_NEW_DELTA
UPSTREAM_TAG_FETCH=YELLOW_LOCAL_V99_5_394_COLLISION_NOT_OVERWRITTEN
UI_ROUTE_CONTRACT=GREEN_488_487_1
I18N_STATIC=GREEN_1839_1839_0_ERRORS_38_WARNINGS
UI_STATIC_BUILD=GREEN_40_40
UI_PYTHON_DISCOVERY=GREEN_343_PASS_2_SKIPPED
UI_PERIODIC_RUNTIME=GREEN_118_MAPPINGS
UI_MATERIAL_RUNTIME=GREEN_IDS_521_532
UI_ZH_EN_CURRENT_SYSTEM_DPI_200=GREEN
UI_DPI_100_125_150=NOT_TESTED
V1_0_3_GATE=YELLOW
V1_0_3_DISPOSITION=USER_ACCEPTED_YELLOW
V1_0_3_DPI_MATRIX=DPI_DEFERRED
V1_0_4_GATE=GREEN
V1_0_4_IMPLEMENTATION_HEAD=9cc2b11c51c2a63bec174486bfb9efada25737d9
V1_0_4_ROLLBACK=477372373cb1be30c3c04bca4309a7d6fd9fa799
V1_0_4_CLEAN_VALIDATION=GREEN_BUILD_80_STATIC_41_PYTHON_385_PLUS_2_SKIPS_LUA_OPS_PACKAGE_BENCHMARK
BENCHMARK_IMPLEMENTATION_HEAD=c4490463f3819695cab734414f827e0e4e4be118
CHARACTERIZATION_IMPLEMENTATION_HEAD=1b8586877e6e7d3703ecd1dbab1eb89b3e4eb7a2
DIFFERENTIAL_IMPLEMENTATION_HEAD=c6eecaa77cd7d6025ef997c5dd46e53d112c6e08
LEGACY_LEDGER_IMPLEMENTATION_HEAD=b3aa56cf3914ab18da60d1ac9ff1e492377f6e88
ALL_TICK_LEDGER_IMPLEMENTATION_HEAD=632665650
PHYSICAL_LEDGER_FEASIBILITY_HEAD=86a2b3386
RUNTIME_RECORD_LIFECYCLE_LEDGER_HEAD=4fa0ec2f3
RUNTIME_CORRECTION_OBSERVER_HEAD=4c9f3b909
LOAD_BOUNDARY_IMPLEMENTATION_HEAD=971687a24
LOAD_BOUNDARY_CLOSURE_HEAD=a09c6d716
REPORT_COMMIT=SELF
WORKTREE_DIRTY_AT_REPORT_START=false
UPSTREAM_STABLE_VERSION=100.1 build 400
UPSTREAM_MASTER_COMMIT=d768aeb89acad986bd252d7e904bf44bb374545f
UPSTREAM_PHASE_RECHECK=GREEN
LOCAL_HISTORICAL_TAG_COLLISION=YELLOW
FIXED_STEP_BENCHMARK=GREEN
CHARACTERIZATION_SAVES=GREEN
FIRST_DIVERGENCE_FIELD_CAPTURE=GREEN
LEGACY_SAMPLED_FINITE_PROXY_LEDGER=GREEN
ALL_TICK_POST_UPDATE_EXPORTED_FLOATS=GREEN
UNSAMPLED_FULL_STATE_FINITE=RED
PHYSICAL_LEDGER_FEASIBILITY=GREEN
RUNTIME_RECORD_LIFECYCLE_OBSERVER=GREEN
RUNTIME_CORRECTION_OBSERVER=GREEN
AUDITED_AIR_CAPS_RUNTIME_EVIDENCE=GREEN
LOAD_BOUNDARY_FIELD_DIFF=GREEN
PHYSICAL_CONSERVATION_LEDGER=RED
SOURCE_SINK_CORRECTION_LEDGER=RED
G0_UPSTREAM_BASELINE=RED
PRODUCTION_OMNIATMOSPHERE_ALLOWED=false
```

The 1.0.1 diagnostics gate is **GREEN**. The final clean build at `97d2fc2c1`
passes the 40-test Meson suite, 342 Python tests with two declared skips, the
upstream Lua smoke, nine OPS round-trip scenarios, lifecycle/correction ledger
smokes, ordinary and real threaded-render profiler fixtures, and the explicit
lifetime race probe. The final rebuilt executable is bound to SHA-256
`EA2C8517771E615D6DFC86B9E3AFD5A77F0D49F8F0FE3F8635E6AF21D02B279D`.

The global G0 gate is still **RED** because physical conservation/source/correction
accounting, unsampled **full-state** finite/positivity evidence, per-process VRAM
and an accepted performance regression budget do not exist. The profiler itself is
no longer a missing G0 foundation: its spans, concurrency safety and measured
OFF/ON behavior are recorded in `phase-1-profiler-export.md`. The red gate blocks
production OmniAtmosphere work; it does not prevent beginning the isolated 1.0.2
upstream compatibility audit.

The 1.0.2 refresh is also **GREEN**: the official download page, GitHub latest
release, annotated `v100.1.400` tag and official `master` all resolve to the same
`d768aeb89` commit. A branch-only official fetch passed and zero upstream commits
exist after the local 100.1 merge. The rejected tag-inclusive fetch was limited to
the unrelated historical `v99.5.394` collision and did not overwrite local data.

The 1.0.3 UI/material checkpoint is **YELLOW**, not a simulation phase. The
pre-existing data-driven material organization passes a new route-policy contract,
the current localization report is synchronized, and an isolated static client
passes 40/40 Meson tests, 343 Python tests, periodic/material runtime checks, and
visible Chinese/English UI paths at the host's 200% system DPI. No production UI
route, ID, Lua identifier, save format, material behavior, or simulation code was
changed. The required 100%, 125%, and 150% system-DPI matrix remains untested, so
the version did not independently satisfy its GREEN gate. After this limitation was
reported, the user explicitly directed the orchestrator to enter the next version.
That scoped exception is recorded as `USER_ACCEPTED_YELLOW / DPI_DEFERRED`; it does
not manufacture the missing evidence or change the 1.0.3 gate to GREEN. Details are in
[`phase-3-ui-material-organization.md`](phase-3-ui-material-organization.md).

Version 1.0.4 is **GREEN** at `9cc2b11c5`. It adds an offline-only OmniMaterials
data foundation: machine-readable Material/Species/Reaction contracts, canonical
units, property-level provenance, a 488-entry Legacy identity map and fail-closed
validation. Clean build/static/Python/Lua/OPS/source-package/fixed-step checks are
recorded in `phase-4-material-data-foundation.md`. No production simulation,
physical behavior, Element ID, Lua identifier, save-format change, or runtime data
consumer was introduced. Its rollback point is `477372373`.

The Legacy ledger exports finite/range observations and diagnostic proxies. Its
all-tick post-update exported-float sub-gate is GREEN; the source-bound physical
ledger feasibility audit, record-only runtime lifecycle observer, audited-Air-cap
correction observer and OPS load-boundary field-attribution sub-gates are also
GREEN. The lifecycle observer does not give records physical units, and the
correction observer is explicitly limited to 12 Legacy Air cap branches. Physical
mass, momentum, energy, complete runtime source/sink attribution, complete
correction accounting, full state and pressure positivity remain RED and still
block G0.

## Capability audit

| Capability | Available | Evidence boundary |
|---|---|---|
| Internet access | `true` | Official site, Git refs, GitHub releases and license files queried on 2026-08-09 |
| Git | `true` | Windows Git at `E:/Git/cmd/git.exe`; branch, log, remotes and worktrees inspected |
| Sub-agents | `true` | Worker slots were available; three requested read-only ledger audits exhausted service retries with 429 and contributed no evidence. Main Orchestrator independently reran clients, tests, artifact hashes and replay |
| Multiple shells/tool calls | `true` | Independent PowerShell commands can run concurrently |
| Git worktree | `true` | upstream/element worktrees remain isolated; formal load-boundary and all-tick-ledger runs used clean detached worktrees |
| Compile project | `true` | final clean 1.0.1 rebuild passed; final app SHA-256 is `EA2C8517...2B279D` |
| Run automated tests | `true` | final Meson suite `40/40`; Python discovery `342` passed with `2` declared skips |
| GPU hardware | `true` | NVIDIA GeForce RTX 5070 Ti Laptop GPU, driver 591.86, reported 12,227 MiB, compute capability 12.0 |
| SDL application process | `true` | isolated Lua/runtime clients execute; Chinese/English periodic, detail, scroll, long press, and search are visibly checked at 200% system DPI; 100/125/150% remains `not_tested` |
| SDL3 / SDL_GPU runtime | `false` | repository is SDL2; no SDL3 build or GPU compute pipeline exists |
| Shader toolchain | `false` | `dxc`, `glslc`, `spirv-val`, `shadercross`, and `sdl3-config` unavailable on PATH |
| Collect process metrics | `true` | fixed-step runner records process CPU/RAM and final profiler off/on pairs; per-process VRAM is `not_tested_no_gpu_backend` |
| Accepted throughput benchmark | `true` | two generated scenes, two FP modes, clean commit/hash/settings/machine provenance |

The Intel graphics adapter reports an error through WMI and two virtual adapters are
present. No GPU correctness or performance conclusion is inferred from hardware
enumeration.

## Git safety state

The repository began vNext at clean commit `fb72d5e8f`. No `reset --hard`,
`clean -fd`, automatic stash, force push, PR, release, or upload was performed. Existing
public remotes were preserved:

| Role | Remote | URL |
|---|---|---|
| Official upstream | `official` | `https://github.com/The-Powder-Toy/The-Powder-Toy.git` |
| OmniPack publication remote | `omnipack` | `https://github.com/KRhad/TPT-ZH-OmniPack.git` |
| Existing fork origin | `origin` | `https://github.com/Dragonrster/The-Powder-Toy-Chinese.git` |

Additional historical mod remotes remain unchanged. The MSYS2 Git executable must
not precede Windows Git on this CRLF checkout: doing so transiently reports roughly
1,116 false dirty paths and contaminates the VCS tag with `+`. The validated command
PATH begins with `E:/Git/cmd`, then UCRT64, then MSYS2 `usr/bin`.

The Phase 1 recheck read the official download page and official Git refs:
`v100.1.400` and `master` remain `d768aeb89`. `git fetch official --tags --prune`
safely rejected replacement of the local historical `v99.5.394` tag (local object
`7fa5ccf6...`, official tag object `f40a5862...`). It was not overwritten and has
no effect on the current 100.1 baseline; resolve that unrelated tag collision only
with an explicit maintenance decision.

## Integrated phase commits

| Commit | Purpose | Production behavior |
|---|---|---|
| `635eb9f92` | order generated `VcsTag.h` correctly in clean parallel builds | build-only |
| `ba30cd2e6` | merge official 100.1 build 400 | upstream adaptation |
| `729f72cba` | preserve complex author BSON and add 100.1 regressions | Save correctness |
| `9c8767120` | make `sim.resetVelocity()` include final air row/column | Lua correctness |
| `f1320b48d` | deterministic element update inventory | tooling/docs only |
| `abeca81bd` | explicit `legacy_fast` and `strict` FP modes | build-only |
| `103a6752d` | verified FP build gate report | docs only |
| `c4490463f` | fixed-step client benchmark and contract tests | tooling/tests only |
| `b9ae20bf0` | formal fixed-step benchmark baseline | docs only |
| `908878f74` | restore deterministic continuation state after full save load | Save correctness |
| `ce8087d07` | synchronize loaded edge mode across model and simulation | Save/UI correctness |
| `1b8586877` | deterministic C01-C14 characterization suite | tooling/tests only |
| `9102d144f` | formal Legacy characterization report and gate update | docs only |
| `c6eecaa77` | first-divergence trace and field capture | tooling/tests only |
| `4765fc123` | formal first-divergence report and gate update | docs only |
| `8bd640c3e` | sampled Legacy numerical proxy ledger | tooling/tests only |
| `b3aa56cf3` | platform-independent byte-stable ledger JSON replay | tooling/tests only |
| `632665650` | make all-tick ledger scope explicit | tooling/tests only |
| `86a2b3386` | source-bound physical-ledger feasibility audit | tooling/tests only |
| `971687a24` | OPS load-boundary field capture and attribution | tooling/tests only |
| `a09c6d716` | close frozen-input and recursive artifact manifest | tooling/tests only |
| `4fa0ec2f3` | optional record-level lifecycle reconciliation observer | additive diagnostics; disabled by default |
| `4c9f3b909` | bounded runtime observer for 12 audited Legacy Air cap branches, Lua fixture, and overflow replay | additive diagnostics; disabled by default |
| `2b6b6e39e` | default-off steady-clock profiler, Lua export, runtime fixtures and race probe | additive diagnostics; disabled by default |
| `97d2fc2c1` | isolated normal-UI profiler fixture and threaded-render heartbeat stabilization | test/runtime wrapper only |

## Phase report

- Goal: establish latest-upstream, compatibility, research, and inventory evidence
  before any state-model replacement.
- Base commit: `fb72d5e8f` (pre-vNext OmniPack).
- Architecture changes: no OmniCore physical state model; upstream integration,
  two bounded correctness fixes, isolated benchmark/characterization/differential/
  ledger tooling, a disabled-by-default record-only lifecycle diagnostic, and a
  disabled-by-default audited-Air-cap correction diagnostic are integrated. Version
  1.0.1 additionally adds a default-off `steady_clock` profiler with mutex/generation
  protected aggregation and a shared renderer-lifetime owner.
- Numerical verification: identical generated mixed state first diverges between FP
  modes at update step 1 in Particle velocity and Air state. A clean 1,000-step
  ledger then found `0` non-finite/range/bound observations across 102 sampled
  exported states and 1,001 all-tick post-update exported states on each side. This
  does not select a correct build or evaluate physical conservation, pressure
  positivity, full state, CFL, near vacuum or species.
- Benchmark: clean `c4490463f` fixed-step baseline records 1,682.07/1,655.22 steps/s
  for empty and 189.87/189.81 for mixed Legacy/Strict. No speed winner is claimed.
- Memory: the observer adds two `int64_t[PT_NUM]` arrays (16,384-byte array floor)
  plus scalar state per `Simulation`; the final profiler off/on pairs sampled
  155,156,480-156,524,544 byte working sets. Per-process VRAM remains
  `not_tested_no_gpu_backend`.
- Characterization: clean-source C01-C14 is `14/14 PASS`; 28 independent restart
  loads have identical traces, RNG, particle count and required-element counts.
- Differential: clean-source Legacy-fast/Strict CPU trace is `PASS`; step 1 contains
  4,935 differing particle IDs and 2,074 differing Air cells, with
  `unexplained_hash_divergence=false`. Omni CPU/GPU comparisons do not yet exist.
- Ledger: clean-source Legacy-fast/Strict all-tick proxy comparison is `PASS`; the
  `632665650` artifact records 1,001 post-update scans per mode, 18 declared files
  plus the manifest and byte-identical frozen replay. The OPS load-boundary formal
  manifest is `F47F4119...8DC6C`, with 369 declared files plus the manifest, 15
  directories and 14/14 byte-identical comparator replays. The physical-ledger
  audit inventories Lifecycle/correction anchors and proves the current lack of
  authoritative physical fields. `4fa0ec2f3` adds a record-only runtime observer;
  its isolated eight-tick client reports zero reconciliation failures and zero
  unattributed delta, with direct SPRK and BRMT/TUNG paths observed. `4c9f3b909`
  adds the scoped Air-cap observer: the base fixture reports 4 events and the
  overflow fixture reports 2,048 total, 256 retained, 1,792 dropped, with a
  contiguous retained sequence window. Physical conservation claims remain
  explicitly false.
- Compatibility: automated build/Lua/OPS evidence passes; GUI, PSv/fuC, portable
  runtime, broad external Lua corpus, and visual behavior remain `not_tested`.
- Known deviation: `resetVelocity` final-edge fix is ahead of official master.
- Gate: fixed-step, characterization, first-divergence, sampled/all-tick
  proxy-ledger, physical-ledger feasibility, record-only runtime lifecycle ledger,
  audited-Air-cap correction observer and load-boundary field-attribution
  foundations are GREEN. The 1.0.1 profiler/export gate is also GREEN: final build,
  `40/40` Meson, `342` Python pass plus `2` skips, runtime and race probe pass, and
  sequential OFF/ON measurement is recorded. Runtime physical conservation,
  complete source/sink and correction ledgers, VRAM and an accepted performance
  budget keep global G0 RED.
- Rollback point: `fb72d5e8f` for all vNext integration, or the parent of each bounded
  commit for phase-local rollback. No rollback is currently recommended.

## Next permitted work

Version 1.0.4 is closed. Version 1.0.5 may begin only with a fresh upstream
stable/master impact check, then may:

1. define and validate the PhysicalScale/units contract without a runtime solver;
2. build isolated, deterministic AtmosphereBench candidates and test fixtures;
3. record conservation, positivity, memory and benchmark criteria needed for solver
   selection.

Production Air replacement, PhysicalScale runtime integration, multi-species runtime,
chemistry runtime, SDL3 migration and GPU compute remain blocked. Physical
correction/source-sink attribution, unsampled full-state finite/positivity, process
VRAM and a performance budget stay in the risk register; none is silently
reclassified as complete.
