# OmniCore vNext orchestrator state

## Outcome

```text
REPORT_DATE=2026-08-09
CURRENT_BRANCH=integration/omnicore-vnext
BENCHMARK_IMPLEMENTATION_HEAD=c4490463f3819695cab734414f827e0e4e4be118
CHARACTERIZATION_IMPLEMENTATION_HEAD=1b8586877e6e7d3703ecd1dbab1eb89b3e4eb7a2
DIFFERENTIAL_IMPLEMENTATION_HEAD=c6eecaa77cd7d6025ef997c5dd46e53d112c6e08
LEGACY_LEDGER_IMPLEMENTATION_HEAD=b3aa56cf3914ab18da60d1ac9ff1e492377f6e88
REPORT_COMMIT=SELF
WORKTREE_DIRTY_AT_REPORT_START=false
UPSTREAM_STABLE_VERSION=100.1 build 400
UPSTREAM_MASTER_COMMIT=d768aeb89acad986bd252d7e904bf44bb374545f
FIXED_STEP_BENCHMARK=GREEN
CHARACTERIZATION_SAVES=GREEN
FIRST_DIVERGENCE_FIELD_CAPTURE=GREEN
LEGACY_SAMPLED_FINITE_PROXY_LEDGER=GREEN
PHYSICAL_CONSERVATION_LEDGER=RED
G0_UPSTREAM_BASELINE=RED
PRODUCTION_OMNIATMOSPHERE_ALLOWED=false
```

The official stable source is now an ancestor of the integration branch, and the
build, static tests, Lua 100.1 boundary regression, OPS round trips, explicit FP
builds, the current two-scene uncapped fixed-step baseline, C01-C14 deterministic
restart characterization and same-source CPU first-divergence field capture pass.
G0 is still **RED** because physical conservation/source/correction accounting,
unsampled full-state finite/positivity evidence, subsystem profiling, process VRAM
and an accepted performance budget do not yet exist. The red gate blocks production
OmniAtmosphere work; it does not roll back verified upstream,
benchmark, differential or sampled-ledger foundations.

The new Legacy ledger exports finite/range observations and diagnostic proxies at
fixed samples. It is GREEN only for that sampled subset. Physical mass, momentum,
energy, source/sink attribution, correction events, unsampled ticks and pressure
positivity remain RED and still block G0.

## Capability audit

| Capability | Available | Evidence boundary |
|---|---|---|
| Internet access | `true` | Official site, Git refs, GitHub releases and license files queried on 2026-08-09 |
| Git | `true` | Windows Git at `E:/Git/cmd/git.exe`; branch, log, remotes and worktrees inspected |
| Sub-agents | `true` | Three bounded read-only ledger Workers reviewed Lua, Python/schema and wrapper/provenance; Main Orchestrator independently reran clients, tests, artifact hashes and replay |
| Multiple shells/tool calls | `true` | Independent PowerShell commands can run concurrently |
| Git worktree | `true` | upstream/element worktrees remain isolated; the formal ledger ran from clean detached `ledger-formal-b3aa` |
| Compile project | `true` | clean baseline completed `787/787`; the final ledger wrapper built both current targets before probing and verified 754 compile commands per FP mode |
| Run automated tests | `true` | both current Meson suites `39/39`; Python 302 run, 302 passed, 0 skipped with UCRT64 compiler on PATH |
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

## Phase report

- Goal: establish latest-upstream, compatibility, research, and inventory evidence
  before any state-model replacement.
- Base commit: `fb72d5e8f` (pre-vNext OmniPack).
- Architecture changes: none to OmniCore production state; upstream integration,
  two bounded correctness fixes and isolated benchmark/characterization/differential/
  ledger tooling are integrated.
- Numerical verification: identical generated mixed state first diverges between FP
  modes at update step 1 in Particle velocity and Air state. A clean 1,000-step
  ledger then found `0` non-finite/range/bound observations across 102 sampled
  exported states on each side. This does not select a correct build or evaluate
  physical conservation, pressure positivity, unsampled ticks, CFL, near vacuum or
  species.
- Benchmark: clean `c4490463f` fixed-step baseline records 1,682.07/1,655.22 steps/s
  for empty and 189.87/189.81 for mixed Legacy/Strict. No speed winner is claimed.
- Memory: no production data structure added; measured whole-process peaks range
  from 132,689,920 to 155,041,792 bytes working set. Process VRAM is `not_tested`.
- Characterization: clean-source C01-C14 is `14/14 PASS`; 28 independent restart
  loads have identical traces, RNG, particle count and required-element counts.
- Differential: clean-source Legacy-fast/Strict CPU trace is `PASS`; step 1 contains
  4,935 differing particle IDs and 2,074 differing Air cells, with
  `unexplained_hash_divergence=false`. Omni CPU/GPU comparisons do not yet exist.
- Ledger: clean-source Legacy-fast/Strict sampled proxy comparison is `PASS`; the
  final manifest is `5F57C3F5...743390D`, artifact files are `18/18`, directory
  closure is 19 files/0 subdirectories, and frozen CSV replay is byte-identical.
  Physical conservation claims remain explicitly false.
- Compatibility: automated build/Lua/OPS evidence passes; GUI, PSv/fuC, portable
  runtime, broad external Lua corpus, and visual behavior remain `not_tested`.
- Known deviation: `resetVelocity` final-edge fix is ahead of official master.
- Gate: fixed-step, characterization, first-divergence and sampled proxy-ledger
  foundations are GREEN; physical conservation/source/correction ledgers,
  load-boundary fields, profiler/VRAM and performance budgets keep G0 RED.
- Rollback point: `fb72d5e8f` for all vNext integration, or the parent of each bounded
  commit for phase-local rollback. No rollback is currently recommended.

## Next permitted work

Only G0 blockers may proceed:

1. physical Legacy conservation/source/correction evidence and unsampled/full-state
   finite/positivity work, including the still-open load-boundary field comparison;
2. subsystem profiler export and process VRAM measurement;
3. strict/fast drift comparison and an accepted performance regression budget.

Production Air replacement, multi-species runtime, SDL3 migration, and GPU compute
remain blocked.
