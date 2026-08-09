# OmniCore vNext orchestrator state

## Outcome

```text
REPORT_DATE=2026-08-09
CURRENT_BRANCH=integration/omnicore-vnext
BENCHMARK_IMPLEMENTATION_HEAD=c4490463f3819695cab734414f827e0e4e4be118
CHARACTERIZATION_IMPLEMENTATION_HEAD=1b8586877e6e7d3703ecd1dbab1eb89b3e4eb7a2
DIFFERENTIAL_IMPLEMENTATION_HEAD=c6eecaa77cd7d6025ef997c5dd46e53d112c6e08
LEGACY_LEDGER_IMPLEMENTATION_HEAD=b3aa56cf3914ab18da60d1ac9ff1e492377f6e88
ALL_TICK_LEDGER_IMPLEMENTATION_HEAD=632665650
PHYSICAL_LEDGER_FEASIBILITY_HEAD=86a2b3386
RUNTIME_RECORD_LIFECYCLE_LEDGER_HEAD=4fa0ec2f3
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
LOAD_BOUNDARY_FIELD_DIFF=GREEN
PHYSICAL_CONSERVATION_LEDGER=RED
G0_UPSTREAM_BASELINE=RED
PRODUCTION_OMNIATMOSPHERE_ALLOWED=false
```

The official stable source is now an ancestor of the integration branch, and the
build, static tests, Lua 100.1 boundary regression, OPS round trips, explicit FP
builds, the current two-scene uncapped fixed-step baseline, C01-C14 deterministic
restart characterization and same-source CPU first-divergence field capture pass.
G0 is still **RED** because physical conservation/source/correction accounting,
unsampled **full-state** finite/positivity evidence, subsystem profiling, process
VRAM and an accepted performance budget do not yet exist. The all-tick result is
limited to post-update exported floats. The red gate blocks production
OmniAtmosphere work; it does not roll back verified upstream,
benchmark, differential or sampled-ledger foundations.

The Legacy ledger exports finite/range observations and diagnostic proxies. Its
all-tick post-update exported-float sub-gate is GREEN; the source-bound physical
ledger feasibility audit, record-only runtime lifecycle observer and OPS
load-boundary field-attribution sub-gates are also GREEN. The lifecycle observer
does not give records physical units. Physical mass, momentum, energy, runtime
source/sink attribution, correction events, full state and pressure positivity
remain RED and still block G0.

## Capability audit

| Capability | Available | Evidence boundary |
|---|---|---|
| Internet access | `true` | Official site, Git refs, GitHub releases and license files queried on 2026-08-09 |
| Git | `true` | Windows Git at `E:/Git/cmd/git.exe`; branch, log, remotes and worktrees inspected |
| Sub-agents | `true` | Worker slots were available; three requested read-only ledger audits exhausted service retries with 429 and contributed no evidence. Main Orchestrator independently reran clients, tests, artifact hashes and replay |
| Multiple shells/tool calls | `true` | Independent PowerShell commands can run concurrently |
| Git worktree | `true` | upstream/element worktrees remain isolated; formal load-boundary and all-tick-ledger runs used clean detached worktrees |
| Compile project | `true` | fresh lifecycle-ledger target completed successfully; its app SHA-256 is recorded in `phase-1-runtime-lifecycle-ledger.md` |
| Run automated tests | `true` | fresh lifecycle-ledger Meson suite `39/39`; nested Python discovery `327` run, `2` declared skips, no failures |
| GPU hardware | `true` | NVIDIA GeForce RTX 5070 Ti Laptop GPU, driver 591.86, reported 12,227 MiB, compute capability 12.0 |
| SDL application process | `true` | isolated Lua/runtime clients execute; visible interactive GUI acceptance is `not_tested` |
| SDL3 / SDL_GPU runtime | `false` | repository is SDL2; no SDL3 build or GPU compute pipeline exists |
| Shader toolchain | `false` | `dxc`, `glslc`, `spirv-val`, `shadercross`, and `sdl3-config` unavailable on PATH |
| Collect process metrics | `true` | fixed-step and ledger runners record process CPU/RAM; per-process VRAM is `not_tested` |
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

## Phase report

- Goal: establish latest-upstream, compatibility, research, and inventory evidence
  before any state-model replacement.
- Base commit: `fb72d5e8f` (pre-vNext OmniPack).
- Architecture changes: no OmniCore physical state model; upstream integration,
  two bounded correctness fixes, isolated benchmark/characterization/differential/
  ledger tooling, and a disabled-by-default record-only lifecycle diagnostic are
  integrated.
- Numerical verification: identical generated mixed state first diverges between FP
  modes at update step 1 in Particle velocity and Air state. A clean 1,000-step
  ledger then found `0` non-finite/range/bound observations across 102 sampled
  exported states and 1,001 all-tick post-update exported states on each side. This
  does not select a correct build or evaluate physical conservation, pressure
  positivity, full state, CFL, near vacuum or species.
- Benchmark: clean `c4490463f` fixed-step baseline records 1,682.07/1,655.22 steps/s
  for empty and 189.87/189.81 for mixed Legacy/Strict. No speed winner is claimed.
- Memory: the observer adds two `int64_t[PT_NUM]` arrays (16,384-byte array floor)
  plus scalar state per `Simulation`; enabled-overhead benchmark and process VRAM
  are `not_tested`. Earlier measured whole-process peaks range from 132,689,920 to
  155,041,792 bytes.
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
  unattributed delta, with direct SPRK and BRMT/TUNG paths observed. Physical
  conservation claims remain explicitly false.
- Compatibility: automated build/Lua/OPS evidence passes; GUI, PSv/fuC, portable
  runtime, broad external Lua corpus, and visual behavior remain `not_tested`.
- Known deviation: `resetVelocity` final-edge fix is ahead of official master.
- Gate: fixed-step, characterization, first-divergence, sampled/all-tick
  proxy-ledger, physical-ledger feasibility, record-only runtime lifecycle ledger
  and load-boundary field-attribution foundations are GREEN; runtime physical
  conservation/source/correction ledgers, profiler/VRAM and performance budgets
  keep G0 RED.
- Rollback point: `fb72d5e8f` for all vNext integration, or the parent of each bounded
  commit for phase-local rollback. No rollback is currently recommended.

## Next permitted work

Only G0 blockers may proceed:

1. runtime correction observer and internal/full-state finite/positivity work;
2. subsystem profiler export and process VRAM measurement;
3. strict/fast drift comparison and an accepted performance regression budget.

Production Air replacement, multi-species runtime, SDL3 migration, and GPU compute
remain blocked.
