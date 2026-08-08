# OmniCore vNext orchestrator state

## Outcome

```text
REPORT_DATE=2026-08-09
CURRENT_BRANCH=integration/omnicore-vnext
CURRENT_HEAD=f1320b48dfd5a570edc353a6d52dcd8adc094345
WORKTREE_DIRTY_AT_REPORT_START=false
UPSTREAM_STABLE_VERSION=100.1 build 400
UPSTREAM_MASTER_COMMIT=d768aeb89acad986bd252d7e904bf44bb374545f
G0_UPSTREAM_BASELINE=RED
PRODUCTION_OMNIATMOSPHERE_ALLOWED=false
```

The official stable source is now an ancestor of the integration branch, and the
build, static tests, Lua 100.1 boundary regression, and OPS round trips pass. G0 is
still **RED** because a current uncapped fixed-step benchmark, the C01-C14
characterization saves, a strict-FP numerical baseline, and a differential runner do
not yet exist. The red gate blocks production OmniAtmosphere work; it does not roll
back the verified upstream adaptation.

## Capability audit

| Capability | Available | Evidence boundary |
|---|---|---|
| Internet access | `true` | Official site, Git refs, GitHub releases and license files queried on 2026-08-09 |
| Git | `true` | Windows Git at `E:/Git/cmd/git.exe`; branch, log, remotes and worktrees inspected |
| Sub-agents | `true` | Existing research Workers completed; no Worker may merge integration |
| Multiple shells/tool calls | `true` | Independent PowerShell commands can run concurrently |
| Git worktree | `true` | upstream and element inventory worktrees are present and isolated |
| Compile project | `true` | clean Ninja compile completed `787/787`; current HEAD relink completed `66/66` |
| Run automated tests | `true` | Meson, Python, Lua and OPS runners execute locally |
| GPU hardware | `true` | NVIDIA GeForce RTX 5070 Ti Laptop GPU, driver 591.86, reported 12,227 MiB, compute capability 12.0 |
| SDL application process | `true` | isolated Lua/runtime clients execute; visible interactive GUI acceptance is `not_tested` |
| SDL3 / SDL_GPU runtime | `false` | repository is SDL2; no SDL3 build or GPU compute pipeline exists |
| Shader toolchain | `false` | `dxc`, `glslc`, `spirv-val`, `shadercross`, and `sdl3-config` unavailable on PATH |
| Collect process metrics | `true` | existing stress harness records limited runtime/resource data |
| Accepted throughput benchmark | `false` | no uncapped fixed-step kernel runner bound to current HEAD |

The Intel graphics adapter reports an error through WMI and two virtual adapters are
present. No GPU correctness or performance conclusion is inferred from hardware
enumeration.

## Git safety state

The repository began vNext at clean commit `fb72d5e8f`. No `reset --hard`, `clean
-fd`, automatic stash, force push, PR, release, or upload was performed. Existing
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

## Phase report

- Goal: establish latest-upstream, compatibility, research, and inventory evidence
  before any state-model replacement.
- Base commit: `fb72d5e8f` (pre-vNext OmniPack).
- Architecture changes: none to OmniCore; only upstream integration and two bounded
  correctness fixes.
- Numerical verification: `not_tested` for conservation, strict-vs-fast FP, solver
  positivity, CFL, near vacuum, and species.
- Benchmark before/after: `not_tested`; historical capped 60 FPS results are excluded.
- Memory before/after: no production data structure added; legacy Air working arrays
  remain about 42 bytes/cell including fan planes.
- Compatibility: automated build/Lua/OPS evidence passes; GUI, PSv/fuC, portable
  runtime, broad external Lua corpus, and visual behavior remain `not_tested`.
- Known deviation: `resetVelocity` final-edge fix is ahead of official master.
- Gate: `RED` for G0 as a whole; verified sub-gates are listed in
  `01-local-baseline.md`.
- Rollback point: `fb72d5e8f` for all vNext integration, or the parent of each bounded
  commit for phase-local rollback. No rollback is currently recommended.

## Next permitted work

Only G0 blockers may proceed:

1. strict/fast numerical build modes;
2. uncapped fixed-step kernel benchmark and profiler export;
3. C01-C14 generated characterization saves with manifests;
4. Legacy CPU versus candidate CPU differential runner and ledgers.

Production Air replacement, multi-species runtime, SDL3 migration, and GPU compute
remain blocked.
