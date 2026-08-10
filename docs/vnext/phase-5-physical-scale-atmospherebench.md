# 1.0.5 PhysicalScale and AtmosphereBench scaffold

## Outcome

```text
TARGET_VERSION=1.0.5
BASE_COMMIT=13b24f49e18c22c794fae457eba9c8069fd13e6b
IMPLEMENTATION_COMMIT=a21eafba301a6e02a94f81cf6da46961ec72d3cd
STATUS=IN_PROGRESS_RUSANOV_SOD_CLEAN_VALIDATED
UPSTREAM_WEBSITE=GREEN_STABLE_100.1
UPSTREAM_STABLE_TAG=v100.1.400
UPSTREAM_STABLE_COMMIT=d768aeb89acad986bd252d7e904bf44bb374545f
UPSTREAM_MASTER_COMMIT=d768aeb89acad986bd252d7e904bf44bb374545f
LOCAL_ENTRY_AHEAD_BEHIND=218_0
PRODUCTION_SIMULATION_CHANGE=false
SAVE_FORMAT_CHANGE=false
LUA_CHANGE=false
PHYSICAL_SCALE_SELECTION=UNSELECTED
PHYSICAL_TIME_POLICY=UNSELECTED
ATMOSPHERE_SOLVER_SELECTED=false
CANDIDATES_REGISTERED=4
CANDIDATE_SOLVERS_IMPLEMENTED=1
TARGETED_TESTS=GREEN_24_24
PYTHON_DISCOVERY=GREEN_409_TOTAL_407_PASS_2_SKIPPED
MESON_STATIC=GREEN_43_43
STRICT_FP_TARGET=GREEN_GNU_NO_LEGACY_FAST_MATH_MSVC_CONTRACT_NOT_EXECUTED
CLEAN_ARTIFACT=GREEN_CONTRACT_ONLY_a21eafba3
CLEAN_SOURCE_PACKAGE=GREEN_1300_FILES_NO_TEST_ASSETS
SHARED_CONTRACT_IMPLEMENTATION_COMMIT=1ba507e89e3d713fe355c03c2fc6e7139aabcb49
SHARED_CONTRACT_VALIDATION=GREEN_CLEAN_BUILD_80_STATIC_43_TARGETED_26_PYTHON_411_TOTAL_409_PASS_2_SKIPS_CONTRACT_ARTIFACT_AND_SOURCE_PACKAGE
RUSANOV_IMPLEMENTATION_COMMIT=4b0658d8ff7bc56169fd8ed5d649f8c6b4250b44
RUSANOV_VALIDATION=GREEN_ISOLATED_STRICT_DOUBLE_1D_PERIODIC_UNIFORM_PROBE_BUILD_80_STATIC_44_PYTHON_414_TOTAL_412_PASS_2_SKIPS_ZERO_DRIFT_ZERO_CORRECTIONS
RUSANOV_PRESSURE_PULSE_COMMIT=cded7be672fbb2755174499214bb31622979eac6
RUSANOV_PRESSURE_PULSE_VALIDATION=GREEN_ISOLATED_STRICT_DOUBLE_1D_PERIODIC_128X1_64_STEP_NONUNIFORM_PROBE_BUILD_80_STATIC_45_PYTHON_414_TOTAL_412_PASS_2_SKIPS_POSITIVE_ZERO_CORRECTIONS
RUSANOV_DENSITY_ADVECTION_COMMIT=0ee4b756176413c4261f74c3b6a6bbcb4298eae8
RUSANOV_DENSITY_ADVECTION_VALIDATION=GREEN_ISOLATED_STRICT_DOUBLE_128X1_EXACT_ONE_CELL_SHIFT_BUILD_80_STATIC_46_PYTHON_414_TOTAL_412_PASS_2_SKIPS_L1_0_000543106_ZERO_CORRECTIONS
RUSANOV_CONTACT_COMMIT=68bcc74a5574ee1fc9240576c04a31a8f9335484
RUSANOV_CONTACT_VALIDATION=GREEN_ISOLATED_STRICT_DOUBLE_128X1_EXACT_ONE_CELL_SHIFT_BUILD_80_STATIC_47_PYTHON_414_TOTAL_412_PASS_2_SKIPS_L1_0_00923098_LINF_0_161912_ZERO_CORRECTIONS
RUSANOV_NEAR_VACUUM_COMMIT=54b3060ab996b6387e5aaf11283eaea1bb9e8faa
RUSANOV_NEAR_VACUUM_VALIDATION=GREEN_ISOLATED_STRICT_DOUBLE_128X1_DENSITY_1E_MINUS_6_PRESSURE_1E_MINUS_8_BUILD_80_STATIC_48_PYTHON_414_TOTAL_412_PASS_2_SKIPS_MASS_TRANSFER_POSITIVE_ZERO_CORRECTIONS
RUSANOV_SOD_COMMIT=588d38d32ec4904118e741e5f5f614c69b8de3de
RUSANOV_SOD_VALIDATION=GREEN_ISOLATED_STRICT_DOUBLE_256X1_GAMMA_1_4_T_0_2_BUILD_80_STATIC_49_PYTHON_414_TOTAL_412_PASS_2_SKIPS_SHOCK_X_0_855469_BOUNDARY_LEDGER_ZERO_CORRECTIONS
HLLE_STATUS=REGISTERED_ONLY
LBM_STATUS=REGISTERED_ONLY
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
GameSave, Lua, SDL, rendering, or any production library. In the validated GNU
`legacy_fast` build tree, its compile commands use:

```text
-fno-fast-math
-fno-unsafe-math-optimizations
-ffp-contract=off
```

The MSVC `/fp:strict` target branch is source-contract tested but not executed in
this checkpoint; it remains `NOT_TESTED` rather than a cross-compiler claim.

It currently provides only the contracts required before a solver plugin:

- candidate PhysicalScale geometry calculations;
- conservative `rho`, `rho*u`, `rho*v`, `rho*E` storage;
- primitive/EOS conversion using an explicitly synthetic nondimensional fixture,
  not dry-air property data;
- a zero-correction conservation-ledger closure check;
- deterministic registration of Legacy-like control, Rusanov FVM, HLLE FVM and
  D2Q9 LBM candidates.

At the initial scaffold checkpoint, all four candidates reported
`solver_implemented=false`. `--run-uniform` continues to emit
`result_status=contract_only`; its zero drift means the unchanged fixture closes,
not that any CFD scheme passed uniform-state preservation. The later isolated
Rusanov checkpoint changes only `fvm_rusanov` to an implemented 1D uniform probe;
HLLE and LBM remain registered-only.

`tools/run_atmospherebench.ps1` refuses dirty source, verifies that the Meson build
tree belongs to the current repository, rebuilds the standalone target before
measurement, obtains the unique `atmospherebench@exe` output from Meson instead of
trusting a caller-provided program, binds its compile commands to the current source
files, rechecks that HEAD stayed clean and unchanged through measurement, verifies
GNU-like strict flags or MSVC `/fp:strict`, rejects either family's fast mode,
executes only the standalone contract tool, and writes ignored provenance/result
artifacts under `artifacts/vnext-atmospherebench/`. It cannot launch the game client
or claim a performance gate.

## Shared experiment contract layer

The shared-contract follow-up adds the solver-free contracts required before a
numerical plugin: `AtmosphereGrid`, `BenchmarkCase`, `BenchmarkResult`, and a
`NumericalCorrectionLedger`. The only registered case is still `uniform_state` and
explicitly declares `nondimensional_contract`, a timestep of `1`, and zero solver
steps. It reports a `4 x 3` metadata grid and `32 bytes/cell` for its four-double
conservative state, not an eventual solver's total memory budget.

The runner now rejects a dimensional, stepped, or grid-drifted scaffold case. The
shared contracts remain isolated from production and do not implement a flux,
boundary update, source term, CFL calculation, or candidate solver.

Every correction ledger now carries an explicit `eventCount`. A nonempty ledger
without an event count fails the C++ self-test; `numerical_correction_count` is the
actual recorded count rather than a boolean-like placeholder. The zero-step fixture
continues to report all correction quantities and counts as zero.

Shared-contract checkpoint `1ba507e89` passed a clean default **80/80** Meson build,
clean **43/43** Meson static suite and **411 total / 409 PASS / 2 skipped** Python
discovery. Its ignored runner artifact is
`artifacts/vnext-atmospherebench/20260810T133938Z-57dca7f9/result.json`; it records
`source_dirty=false`, `case_time_domain=nondimensional_contract`, zero steps, a
`4x3=12` metadata grid, `32 bytes/cell`, and zero correction events. The associated
test-free source package is at `artifacts/vnext-phase5-source-1ba507e89/` with
SHA-256 `3C6BD0A389258E5EC5DE65FA4491402565DD2FD3734365154F5FBCABB3BDBE8D`.

## Clean checkpoint evidence

Implementation checkpoint `a21eafba3` was built from a clean worktree. The default
Meson build completed all **80/80** targets. The runner rebuilt and then resolved
the unique `atmospherebench@exe` target itself; its local, ignored result is
`artifacts/vnext-atmospherebench/20260810T125117Z-7e2e68be/result.json` and records:

```text
source_commit=a21eafba301a6e02a94f81cf6da46961ec72d3cd
source_dirty=false
strict_reference_mode=gnu_strict
result_status=contract_only
performance_gate=not_evaluated_contract_only
```

The actual test-free source archive passed at the same source checkpoint. It has
1,300 source entries plus its manifest, includes every required 1.0.5 member, and
contains no test asset. Its local ignored SHA-256 is:

```text
29ADC7015045B38C5DE403B88D2B62322178142995E17133A6DF21B8655D45DE
```

at `artifacts/vnext-phase5-source-a21eafba3/`. No production source changed, so
the prior Legacy save/Lua/runtime evidence remains applicable; this scaffold does
not claim a new GUI or runtime exercise.

## Validation and current gate

- Targeted PhysicalScale/AtmosphereBench/runner tests: **24/24 PASS**.
- Full Python discovery: **409 total**, **407 PASS**, **2 declared skips**.
- Meson static suite: **43/43 PASS**, including `physical-scale-contract` and
  `atmospherebench-contract`.
- Clean default Meson build: **80/80 PASS**.
- Standalone compile and self-test: **PASS**.
- Production-boundary scan: zero consumers in `src/`.
- Source-package contract: the candidate data, validator, runner and three C++
  files are required and cannot be silently filtered from a clean archive.

An independent final review found no remaining P1/P2 issues and approved the
isolated scaffold checkpoint only. It did not approve solver selection or production
integration.

The shared-contract follow-up is now clean-validated: **26/26 targeted**,
**411 total / 409 PASS / 2 skipped** Python tests, **43/43** Meson static tests and
the default **80/80** build all pass. It remains contract-only, not CFD evidence.

The isolated strict-double Rusanov follow-up is clean-validated at `4b0658d8f`.
It runs a first-order 1D periodic uniform probe for 16 nondimensional steps at
`dt=0.05`. The observed maximum CFL is `0.0645497`; density, pressure and energy
remain positive; mass, x/y momentum and energy drift are all zero; and no numerical
correction is recorded. The default build is **80/80**, Meson static is **44/44**,
and Python discovery is **414 total / 412 PASS / 2 skipped**. This is candidate
debug evidence, not shock/near-vacuum/performance evidence or solver selection.
Full details and artifact hashes are in
[the Rusanov checkpoint](phase-5-rusanov-candidate.md).

The subsequent Rusanov pressure-pulse probe is clean-validated at `cded7be67`.
It evolves a non-uniform periodic 128x1 Gaussian pressure perturbation for 64
nondimensional steps. Its pressure maximum falls from `1.1` to `1.09652`,
state-change L1 is `0.373615`, the maximum CFL is `0.0270897`, all primitive
states remain positive and no correction event occurs. Mass, x/y momentum and
energy drift remain within the published `1e-10` periodic tolerance. It is still
candidate evidence, not a shock/near-vacuum/performance result or solver selection.

The Rusanov density-advection probe is clean-validated at `0ee4b7561`. A smooth
periodic density wave at constant pressure and velocity has an exact one-cell
reference shift. The measured density L1/Linf errors are `0.000543106` and
`0.000921691`, pressure Linf error is `4.44089e-16`, total-variation ratio is
`0.995707`, all primitives remain positive and no correction occurs. This measures
first-order transport diffusion but is not a convergence study or solver selection.

The constant-pressure contact-discontinuity probe is clean-validated at
`68bcc74a5`. It preserves the exact density bounds and pressure to roundoff, closes
the conservative ledger near machine precision, and records zero corrections. Its
`0.161912` density Linf error is explicitly accepted only against a declared `0.2`
first-order debug bound; this is evidence of Rusanov diffusion, not solver accuracy
selection.

The near-vacuum expansion probe is clean-validated at `54b3060ab`. A periodic
`rho=1, p=1` half-domain expands into a `rho=1e-6, p=1e-8` half-domain. The low-
density region gains mass through generic fluxes, all primitives remain positive,
the periodic conservative ledger closes near machine precision, and the correction
ledger stays empty. This is one synthetic robustness point, not a production vacuum
model, scale choice or solver selection.

The sealed Sod shock-tube probe is clean-validated at `588d38d32`. At `t=0.2`
the detected pressure front is `x=0.855469`, maximum velocity is `0.927941`, and
maximum CFL is `0.280563`. Mass and energy close near machine precision. The raw
x-momentum change `46.08` exactly matches the wall-pressure impulse ledger, leaving
`-3.83693e-13` adjusted error. The sealed face layout is reported as `96.125`
state/flux/scratch bytes per cell (`24608` bytes total). This is a first-order
bounded result, not a convergence study or solver selection.

`V1_0_5_GATE=IN_PROGRESS`. The next implementation may extend only the Rusanov
candidate with grid-refinement/low-Mach or leak evidence while keeping HLLE and LBM
registered-only and independently replaceable. PhysicalScale, physical-time policy
and solver selection remain RED until the mandatory cases, conservation/positivity,
memory and performance evidence exist. Production Air replacement is still
forbidden.
