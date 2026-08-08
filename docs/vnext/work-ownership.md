# vNext work ownership

## Current status

There are no active write Workers. The Main Orchestrator owns integration and all
files under `docs/vnext/` for the first-round report commit.

| Worker | Branch | Worktree | Allowed paths | Forbidden paths | Base commit | Task | Status |
|---|---|---|---|---|---|---|---|
| Main Orchestrator | `integration/omnicore-vnext` | `D:/CodexWork/OmniPack/repos/TPT-ZH-OmniPack` | integration, review, `docs/vnext/**` | destructive Git operations, push/release | `fb72d5e8f` | integrate upstream and first-round evidence | ACTIVE |
| Worker-Upstream | `vnext/upstream-100.1` | `D:/CodexWork/OmniPack/worktrees/upstream-100.1` | upstream adaptation and isolated regression tools | unrelated OmniCore production architecture | `635eb9f92` | adapt TPT 100.1 build 400 | COMPLETE, merged through `729f72cba` |
| Worker-Elements | `vnext/element-inventory` | `D:/CodexWork/OmniPack/worktrees/element-inventory` | `tools/vnext/element_update_inventory.py`, inventory report/JSON | production simulation | `729f72cba` | static update inventory | COMPLETE, integrated as `f1320b48d` |
| Worker-LuaSave | read-only | main worktree | read-only Lua, Particle, Save inspection | every write | `729f72cba` | Lua/Save compatibility audit | COMPLETE |
| Worker-Benchmark | read-only | main worktree | read-only build/test/profiler inspection | every write | `729f72cba` | benchmark and FP audit | COMPLETE |
| Worker-Air | read-only | main worktree | read-only Air/Simulation inspection | every write | `729f72cba` | Legacy Air audit | COMPLETE |
| Worker-CFD | read-only | main worktree | read-only literature and solver comparison | every write | `729f72cba` | AtmosphereBench research | COMPLETE |
| Worker-Chemistry | read-only | main worktree | read-only chemistry/library inspection | every write | `729f72cba` | OmniChem gap analysis | COMPLETE |
| Worker-Materials | read-only | main worktree | read-only property/provenance inspection | every write | `729f72cba` | material-data research | COMPLETE |
| Worker-SDLGPU | read-only | main worktree | read-only SDL/GPU inspection | every write | `729f72cba` | SDL3/SDL_GPU research | COMPLETE |
| Worker-Differential-Evidence | read-only | main worktree | formal artifact verification and ignored recompute outputs | tracked-file writes | `c6eecaa77` | independently recompute first-divergence comparisons | COMPLETE, PASS |
| Worker-Strict-Head | read-only | main worktree | existing build/test outputs | tracked-file writes | `c6eecaa77` | re-run current-HEAD Strict and Python suites | COMPLETE, PASS |
| Worker-Differential-Docs | read-only | main worktree | read-only report/evidence review | every write | `c6eecaa77` | audit first-divergence report scope and gates | COMPLETE, PASS |

## Coordination rules

- Only the Main Orchestrator may merge or write the integration branch.
- A future Worker that changes code must receive a dedicated branch, worktree, base
  commit, and non-overlapping path ownership before editing.
- `Simulation`, `Particle`, `Air`, Lua core, and Save are serialized ownership zones.
- Read-only research may share a checkout, but its returned claims remain subject to
  main-agent source, diff, build, and test review.
