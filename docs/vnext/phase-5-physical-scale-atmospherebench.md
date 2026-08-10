# 1.0.5 PhysicalScale and AtmosphereBench scaffold

## Outcome

```text
TARGET_VERSION=1.0.5
BASE_COMMIT=13b24f49e18c22c794fae457eba9c8069fd13e6b
IMPLEMENTATION_COMMIT=pending
STATUS=IN_PROGRESS_SCAFFOLD_VALIDATED_DIRTY_SOURCE
UPSTREAM_WEBSITE=GREEN_STABLE_100.1
UPSTREAM_STABLE_TAG=v100.1.400
UPSTREAM_STABLE_COMMIT=d768aeb89acad986bd252d7e904bf44bb374545f
UPSTREAM_MASTER_COMMIT=d768aeb89acad986bd252d7e904bf44bb374545f
LOCAL_AHEAD_BEHIND=218_0
PRODUCTION_SIMULATION_CHANGE=false
SAVE_FORMAT_CHANGE=false
LUA_CHANGE=false
PHYSICAL_SCALE_SELECTION=UNSELECTED
PHYSICAL_TIME_POLICY=UNSELECTED
ATMOSPHERE_SOLVER_SELECTED=false
CANDIDATES_REGISTERED=4
CANDIDATE_SOLVERS_IMPLEMENTED=0
TARGETED_TESTS=GREEN_24_24
PYTHON_DISCOVERY=GREEN_409_TOTAL_407_PASS_2_SKIPPED
MESON_STATIC=GREEN_43_43
STRICT_FP_TARGET=GREEN_NO_LEGACY_FAST_MATH
CLEAN_ARTIFACT=PENDING_IMPLEMENTATION_CHECKPOINT
V1_0_5_GATE=IN_PROGRESS
```

## Entry and upstream gate

The official download page returned HTTP 200 and identifies 100.1 as the current
stable version. A no-tag official Git fetch and `ls-remote` show `master` at
`d768aeb89` and the signed `v100.1.400` tag peeling to the same commit. The local
branch is 218 commits ahead and zero behind; there is no new upstream delta to
integrate before this scaffold. The GitHub REST release endpoint was rate-limited
and one web-tool channel returned an external authentication 503; neither failure
is presented as release evidence because the official site and Git refs succeeded.

## Scope and compatibility facts

The current source fixes Legacy geometry at `CELL=4`, an Air/wall grid of `153x96`,
a particle grid of `612x384`, and 235,008 particle slots. OPS stores and validates
the Legacy cell size, so this phase does not change it. Legacy `pv`, `vx/vy`, and
`hv` are not relabeled as Pa, m/s, or internal energy, and `M_GRAV=0.6673` remains
a gameplay coefficient rather than an SI acceleration.

There is no implemented physical `dt`: a Legacy fixed step is one tick, while
presentation rate can be capped or uncapped. The nominal 60 Hz time candidate is
therefore only a benchmark input. Every production selection remains unselected.

## PhysicalScale candidate contract

`resources/omnicore/v1/physical-scale-candidates.json` records:

- the exact Legacy geometry fingerprint and explicit non-SI Legacy tick semantics;
- one `1 mm pixel / 4 mm atmosphere cell / 4 mm effective depth` geometry candidate;
- exact derived parcel volume `4e-9 m^3`, cell volume `6.4e-8 m^3`, and world extent
  `0.612 m x 0.384 m`;
- a top-left origin, right/down axes, cell-centre sampling and explicit SI gravity
  input convention;
- nominal 60 Hz, acoustic scaling, event-local subcycling and all-speed/hybrid time
  policy candidates, all unselected;
- prohibitions against interpreting Legacy tick/FPS/Air fields as SI state or
  treating candidate values as defaults.

`tools/physical_scale_check.py` is a strict standard-library validator. It rejects
duplicate keys, BOM/non-finite JSON, unknown fields, non-positive scale, geometry or
unit mismatch, broken volume/world identities, accepted time policy, FPS-derived
physical `dt`, and drift in the Legacy/SI prohibitions. The 1.0.5 contract also
extends the canonical unit registry with exact SI area, volume, acceleration,
momentum-density and molar-concentration definitions; PhysicalScale now verifies
that its metre, cubic-metre, second and acceleration references resolve through
that registry. No property value or runtime consumer is introduced.

## AtmosphereBench scaffold

The standalone C++20 target does not include or link Air, Simulation, Particle,
GameSave, Lua, SDL, rendering, or any production library. Even in a `legacy_fast`
build tree, its compile commands use:

```text
-fno-fast-math
-fno-unsafe-math-optimizations
-ffp-contract=off
```

It currently provides only the contracts required before a solver plugin:

- candidate PhysicalScale geometry calculations;
- conservative `rho`, `rho*u`, `rho*v`, `rho*E` storage;
- primitive/EOS conversion using an explicitly synthetic nondimensional fixture,
  not dry-air property data;
- a zero-correction conservation-ledger closure check;
- deterministic registration of Legacy-like control, Rusanov FVM, HLLE FVM and
  D2Q9 LBM candidates.

All four candidates report `solver_implemented=false`. `--run-uniform` emits
`result_status=contract_only`; its zero drift means the unchanged fixture closes,
not that any CFD scheme passed uniform-state preservation.

`tools/run_atmospherebench.ps1` refuses dirty source, verifies that the Meson build
tree belongs to the current repository, rebuilds the standalone target before
measurement, obtains the unique `atmospherebench@exe` output from Meson instead of
trusting a caller-provided program, binds its compile commands to the current source
files, rechecks that HEAD stayed clean and unchanged through measurement, verifies
GNU-like strict flags or MSVC `/fp:strict`, rejects either family's fast mode,
executes only the standalone contract tool, and writes ignored provenance/result
artifacts under `artifacts/vnext-atmospherebench/`. It cannot launch the game client
or claim a performance gate.

## Validation and current gate

- Targeted PhysicalScale/AtmosphereBench/runner tests: **24/24 PASS**.
- Full Python discovery: **409 total**, **407 PASS**, **2 declared skips**.
- Meson static suite: **43/43 PASS**, including `physical-scale-contract` and
  `atmospherebench-contract`.
- Standalone compile and self-test: **PASS**.
- Production-boundary scan: zero consumers in `src/`.
- Source-package contract: the candidate data, validator, runner and three C++
  files are required and cannot be silently filtered from a clean archive.

These results are currently from a dirty implementation worktree. The next
checkpoint must commit the scaffold, rerun on clean source, generate the first
contract-only artifact, and receive an independent diff review.

`V1_0_5_GATE=IN_PROGRESS`. The next implementation may add the shared grid/case/
result contracts and one first-order Rusanov plugin, but must keep Rusanov, HLLE,
HLLC and LBM independently replaceable. PhysicalScale, physical-time policy and
solver selection remain RED until the mandatory cases, conservation/positivity,
memory and performance evidence exist. Production Air replacement is still
forbidden.
