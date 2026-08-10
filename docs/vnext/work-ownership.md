# vNext work ownership

## Current status

There are no active write Workers. The Main Orchestrator owns the isolated 1.0.4
data-contract implementation. Two Workers are read-only reviewers and may not edit
the shared checkout.

| Worker | Branch | Worktree | Allowed paths | Forbidden paths | Base commit | Task | Status |
|---|---|---|---|---|---|---|---|
| Main Orchestrator | `integration/omnicore-vnext` | `D:/CodexWork/OmniPack/repos/TPT-ZH-OmniPack` | integration, `resources/omnicore/**`, bounded validation tooling/tests, Meson static-test registration, `docs/vnext/**`, `docs/roadmap/**` | destructive Git operations, push/release, Simulation/Particle/Air/Save/Lua core, runtime physics, SDL3/GPU | `477372373` | 1.0.4 schema, units, provenance, and validation foundation only | ACTIVE, IN_PROGRESS |
| Worker-Materials-Schema-Audit | read-only | main worktree | existing registries, vNext research, proposed schema boundaries | every write | `477372373` | independent schema and compatibility audit | ACTIVE, READ_ONLY |
| Worker-Data-Validation-Audit | read-only | main worktree | validation/test conventions and proposed fail-closed checks | every write | `477372373` | independent validator and negative-test audit | ACTIVE, READ_ONLY |
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
| Worker-Ledger-Lua | read-only | main worktree | read-only Lua ledger and local API/Element semantics | every write | `4765fc123` | audit sampling, finite checks, property classes and non-perturbation | COMPLETE, PASS |
| Worker-Ledger-Python | read-only | main worktree | read-only comparator/schema/tests and local Element classifications | every write | `4765fc123` | adversarial fail-closed parser and byte-stable replay review | COMPLETE, PASS |
| Worker-Ledger-Wrapper | read-only | main worktree | read-only wrapper, Meson metadata and execution provenance | every write | `4765fc123` | audit isolation, build pairing, FP flags, hashes, PS5 and artifact closure | COMPLETE; old artifact CHANGES_REQUIRED, replacement ACCEPT_REPORT |
| Main-Formal-Ledger-Evidence | detached at `b3aa56cf3` | `D:/CodexWork/OmniPack/worktrees/ledger-formal-b3aa` | read source/build inputs; write ignored private ledger artifact only | tracked-file writes, merge, deletion of prior evidence | `b3aa56cf3` | produce and independently verify closed formal 1,000-step evidence | COMPLETE, PASS; worktree retained clean |
| Main-All-Tick-Ledger-Scope | `integration/omnicore-vnext` | main worktree | `tools/legacy_ledger_compare.py`, ledger wrapper and tests | production Simulation/Particle/Air/Save/Lua core, release | `db7fb3e1e` | make `SampleInterval=1` scope truthful and fail-closed | COMPLETE, integrated as `632665650` |
| Main-Formal-All-Tick-Evidence | detached at `632665650` | `D:/CodexWork/OmniPack/worktrees/ledger-formal-alltick-632` | read source/build inputs; write ignored private all-tick artifact only | tracked-file writes, merge, deletion of prior evidence | `632665650` | produce/replay clean 1,001-record per-mode all-tick evidence | COMPLETE, PASS; worktree retained clean |
| Main-Physical-Ledger-Feasibility | `integration/omnicore-vnext` | main worktree | `tools/physical_ledger_feasibility.py`, its tests and feasibility report | production Simulation/Particle/Air/Save/Lua core, release | `ba09372eb` | bind Legacy state, mutation and cap anchors before runtime instrumentation | COMPLETE, integrated as `86a2b3386` |
| Main-Runtime-Lifecycle-Ledger | `integration/omnicore-vnext` | main worktree | serialized `Simulation`/Lua observer, dedicated local runtime runner and Phase 1 report | Particle layout, Air state model, Save schema, release/push | `7d3e66daf` | optional record-level lifecycle attribution and reconciliation | COMPLETE, implementation `4fa0ec2f3`; physical claims remain blocked |
| Main-Runtime-Correction-Ledger | `integration/omnicore-vnext` | main worktree | serialized `Simulation`/Air/Lua observer, private runtime fixture and Phase 1 report | Particle layout, Air state model, Save schema, release/push | `9274fdb3a` | audited Legacy Air cap branch outcomes, bounded overflow evidence and fail-closed Lua export | COMPLETE, implementation `4c9f3b909`; complete physical/source-sink ledger remains blocked |
| Main-Profiler-1.0.1 | `integration/omnicore-vnext` | main worktree | `FrameTime`, Game model/controller/view, Lua profiler API, isolated test wrappers and profiler report | Air state model, Particle layout, Save schema, SDL3/GPU/CUDA, release/push | `ca3d643c8` | default-off steady-clock profiling, safe renderer lifetime and runtime/overhead validation | COMPLETE, integrated as `2b6b6e39e` and `97d2fc2c1`; 1.0.1 gate GREEN |
| Main-Upstream-Refresh-1.0.2 | `integration/omnicore-vnext` | main worktree | official read-only references and `docs/vnext/*upstream*.md` | production source, tags, remotes, release/push | `430b3bd28` | refresh stable/master/release and classify new impact | COMPLETE; 100.1 remains current, no source adaptation required |
| Main-Load-Boundary | `vnext/load-boundary-ledger` | `D:/CodexWork/OmniPack/worktrees/load-boundary-ledger-c386` | `tools/load_boundary_compare.py`, characterization runner/exporter and tests | production Simulation/Particle/Air/Save/Lua core, integration merge | `c386edbbe` | build fail-closed OPS pre-save/loaded field attribution | COMPLETE; reviewed and integrated as `971687a24` and `a09c6d716` |
| Main-Formal-Load-Boundary-Evidence | detached at `9b336fc40` | `D:/CodexWork/OmniPack/worktrees/ledger-formal-9b336` | read source/build inputs; write ignored private load-boundary artifact only | tracked-file writes, merge, deletion of prior evidence | `9b336fc40` | produce/replay closed C01-C14 field-attribution evidence | COMPLETE, PASS; worktree retained clean |
| Worker-Load-Boundary-Audit | N/A; read-only task never started | N/A; service unavailable | read-only planned audit | every write | `a09c6d716` | independent closure review | NO_EVIDENCE; platform returned 429 before task execution |
| Worker-Physical-Ledger-Feasibility | N/A; read-only task never started | N/A; service unavailable | read-only planned feasibility audit | every write | `a09c6d716` | next-blocker feasibility review | NO_EVIDENCE; platform returned 429 before task execution |
| Worker-Profiler-VRAM-Audit | N/A; read-only task never started | N/A; service unavailable | read-only planned profiler/VRAM audit | every write | `a09c6d716` | next-blocker readiness review | NO_EVIDENCE; platform returned 429 before task execution |
| Worker-Profiler-Design-Review | N/A; read-only task interrupted | N/A; local service unavailable | read-only profiler design review | every write | `97d2fc2c1` | independent 1.0.1 design review | NO_EVIDENCE; platform returned 504 before an auditable result |
| Worker-Profiler-Final-Review | N/A; read-only task interrupted | N/A; local service unavailable | read-only final profiler review | every write | `97d2fc2c1` | independent 1.0.1 final review | NO_EVIDENCE; platform returned 504 before an auditable result |
| Worker-Correction-Observer-Review | read-only | main worktree | correction observer sources, fixture and reports | every write | `9274fdb3a` | independent scope review of audited Air-cap observer | COMPLETE, PASS_WITH_DECLARED_LIMITS; not physical conservation evidence |
| Worker-UI-Material-Audit | read-only | main worktree | UI routes, periodic/search/detail views, localization and compatibility audits | every write | `c8a0190c1` | independent 1.0.3 UI/material-organization review | COMPLETE; data model accepted, visual DPI matrix remained YELLOW |

## Coordination rules

- Only the Main Orchestrator may merge or write the integration branch.
- A future Worker that changes code must receive a dedicated branch, worktree, base
  commit, and non-overlapping path ownership before editing.
- `Simulation`, `Particle`, `Air`, Lua core, and Save are serialized ownership zones.
- Read-only research may share a checkout, but its returned claims remain subject to
  main-agent source, diff, build, and test review.
