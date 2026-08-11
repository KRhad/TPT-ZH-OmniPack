# 1.0.5 PhysicalScale and AtmosphereBench scaffold

## Outcome

```text
TARGET_VERSION=1.0.5
BASE_COMMIT=13b24f49e18c22c794fae457eba9c8069fd13e6b
IMPLEMENTATION_COMMIT=76300cd98ca2c7405d011ba4401445e3bd9eced8
STATUS=IN_PROGRESS_REFERENCE_BUDGET_GREEN_TIME_POLICY_RED
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
CANDIDATES_REGISTERED=6
CANDIDATE_SOLVERS_IMPLEMENTED=5
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
RUSANOV_REFINEMENT_COMMIT=d541c2c2e9809691d625294e918c46009cd4a651
RUSANOV_REFINEMENT_VALIDATION=GREEN_ISOLATED_STRICT_DOUBLE_64_128_256_T_0_25_BUILD_82_STATIC_50_PYTHON_414_TOTAL_412_PASS_2_SKIPS_L1_ORDER_0_95393_0_977252_ZERO_CORRECTIONS
RUSANOV_LOW_MACH_COMMIT=a948a48db2c7d06b93dd0f26fb67ad7f1423968c
RUSANOV_LOW_MACH_VALIDATION=EXECUTION_GREEN_SUITABILITY_FALSE_MACH_0_387298_0_0387298_0_00387298_L1_0_00826755_0_0515261_0_126479_TV_0_934805_0_595493_0_00673822_ZERO_CORRECTIONS
RUSANOV_OPEN_LEAK_COMMIT=ef0ca86c1
RUSANOV_OPEN_LEAK_VALIDATION=GREEN_STRICT_DOUBLE_128X1_SEALED_LEFT_OPEN_RIGHT_MASS_OUT_6_69874_BALANCE_ERRORS_LT_3E_MINUS_14_ZERO_CORRECTIONS
RUSANOV_PERFORMANCE_COMMIT=ee9290cb7
RUSANOV_PERFORMANCE_VALIDATION=GREEN_STRICT_DOUBLE_SINGLE_THREAD_MEDIAN_3_REPEATS_31_0232M_31_3699M_32_8059M_CELL_UPDATES_PER_SECOND_NO_BUDGET_SELECTED
HLLC_2D_SPECIES_MIXING_COMMIT=55088a8523421b8ef5c1c0b6be3170fdf505ac34
HLLC_2D_SPECIES_MIXING_VALIDATION=GREEN_32X24_320_STEPS_SPECIES_A_B_ZERO_DRIFT_TV_48_TO_47_9173_768_MIXED_CELLS_ZERO_FALLBACK_ZERO_CORRECTIONS_40_STATE_200_WORKING_BYTES_PER_CELL
HLLC_2D_PERFORMANCE_COMMIT=12e904f7574b690c600f1bbcb0b74d160badf540
HLLC_2D_PERFORMANCE_VALIDATION=RECORDED_NO_BUDGET_153X96_1_69129MS_306X192_6_56627MS_612X384_31_2385MS_160_WORKING_BYTES_PER_CELL_ZERO_FALLBACK_ZERO_CORRECTIONS
LBM_D2Q9_COMMIT=5e3c46fae8fc7754240c97931e59f7b2216c5417
LBM_D2Q9_VALIDATION=GREEN_ISOTHERMAL_UNIFORM_SHEAR_WAVE_ERROR_0_000577441_ZERO_CORRECTIONS_ENERGY_NA_NEAR_VACUUM_UNSUPPORTED_SHOCK_UNSUPPORTED
LEGACY_LIKE_COMMIT=696d0c9580f642db70bb419fc73cd9ea19b2afd0
LEGACY_LIKE_VALIDATION=GREEN_CONTROL_ONLY_UNIFORM_ZERO_CHANGE_PRESSURE_PULSE_PEAK_0_99005_TO_0_236479_PRESSURE_SUM_DRIFT_8_52651E_MINUS_13_ZERO_CORRECTIONS_PHYSICAL_DRIFTS_NULL
ATMOSPHERE_POLICY_BUDGET_COMMIT=76300cd98ca2c7405d011ba4401445e3bd9eced8
ATMOSPHERE_REFERENCE_BUDGET=GREEN_4_1666667MS_PER_TICK_64_AUTHORITATIVE_256_WORKING_BYTES_PER_CELL_HLLC_FITS_ONE_STEP_MAX_TWO
ATMOSPHERE_TIME_POLICY=RED_UNSELECTED_DIRECT_REAL_ACOUSTIC_7167_SUBSTEPS_REJECTED_UNIFORM_SCALING_0_096MPS_REJECTED_HYBRID_REQUIRED_UNIMPLEMENTED
CURRENT_VALIDATION=GREEN_BUILD_STATIC_70_TARGETED_21_PYTHON_426_TOTAL_424_PASS_2_SKIP_POLICY_RECOMPUTATION_AND_LICENSE_AUDIT
CURRENT_SOURCE_PACKAGE=GREEN_1319_SOURCE_PLUS_MANIFEST_NO_TEST_ASSETS_REVISION_40facfdfa_SHA256_293105F3E87D964608CE8D0ACF9D1EBD16417FF4C89623D3400941E9C753706D
HLLE_STATUS=REGISTERED_ONLY
LBM_STATUS=IMPLEMENTED_ISOTHERMAL_LIMITED_NOT_SELECTED
LEGACY_LIKE_STATUS=IMPLEMENTED_CONTROL_ONLY_NOT_SELECTED
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
at that historical checkpoint HLLE and LBM remained registered-only.

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

The smooth-density advection refinement probe is clean-validated at `d541c2c2e`.
Over 64/128/256 cells at the same unit-domain `t=0.25`, density L1 error decreases
`0.0159718 -> 0.00824503 -> 0.00418803`; observed orders are `0.95393` and
`0.977252`. Pressure remains constant to roundoff, all three conservative ledgers
close within `1e-9`, and the correction ledger stays empty. This confirms expected
first-order behavior for one smooth case, not low-Mach or multidimensional fitness.

The low-Mach advection characterization is clean-validated at `a948a48db`.
At nominal Mach `0.387298 / 0.0387298 / 0.00387298`, density L1 error grows
`0.00826755 / 0.0515261 / 0.126479` while total-variation ratio falls
`0.934805 / 0.595493 / 0.00673822`. Conservation remains near machine precision
and the correction ledger is empty, but `low_mach_suitability_passed=false`.
This is a valid negative result: first-order compressible Rusanov is retained as
a strict reference/debug floor and is not selected as the sole Enhanced solver.

The open-boundary leak probe is clean-validated at `ef0ca86c1`. A sealed-left,
open-right 128-cell domain loses `6.69874` nondimensional mass units to a fixed
low-pressure reservoir. The left mass exchange is zero, the right outflow equals
the domain loss, and adjusted mass/momentum/energy balance errors remain below
`3e-14`; CFL is `0.250374`, positivity holds and the correction ledger is empty.

The standalone performance record is clean-validated at `ee9290cb7`. The
single-threaded strict-double 1D end-to-end path measures `14,688 / 29,376 /
58,752` cells for 64 steps, one warm-up and three repetitions. Median throughput
is `31.0232 / 31.3699 / 32.8059 million cell-updates/s`; state plus flux scratch
is `96 bytes/cell`. `performance_gate=recorded_candidate_measurement_no_budget`:
no production frame budget or solver selection is claimed. The current test-free
source package is `artifacts/vnext-phase5-source-ee9290cb7/`, SHA-256
`5130F6B8871F196BB292DF590D74DC552924005EAB206CC16CB810D1AF1E5C9A`, with
`1303` source members plus one manifest and zero test assets.

`V1_0_5_GATE=IN_PROGRESS`. The isolated all-speed Rusanov follow-up is now
implemented and clean-validated at `fae9a0847`, but its low-Mach suitability gate
is negative. It preserves positivity and conservation with zero corrections, yet
the unchanged very-low-Mach density L1 is `0.10116` and total-variation ratio is
`1.46479`; it is explicitly recorded as
`candidate_disposition=reject_low_mach_suitability`. This result does not select a
solver and does not authorize production Atmosphere integration. At that
checkpoint HLLE and LBM remained registered-only. An acceptable low-Mach candidate, physical-time policy
and reviewed performance budget are still required for solver selection.
PhysicalScale, physical-time policy and solver selection remain RED until the
mandatory cases, conservation/positivity, memory and performance evidence exist.
Production Air replacement is still forbidden.

## All-speed Rusanov negative checkpoint

The fifth registered candidate uses a generic local-Mach and pressure-jump sensor
to scale Rusanov dissipation; it is not keyed to the experiment name. The probe is
strict-double and remains isolated to `tools/atmospherebench`.

```text
commit=fae9a0847608333c05a9d5e15b0c812b22407d3d
benchmark_execution_status=PASS
nominal_mach=0.387298 / 0.0387298 / 0.00387298
density_l1_error=0.00454342 / 0.0136636 / 0.10116
total_variation_ratio=0.964322 / 1.82225 / 1.46479
mass_drift=2.84217e-14
energy_drift=1.42109e-13
numerical_correction_count=0
low_mach_suitability_passed=false
candidate_disposition=reject_low_mach_suitability
atmosphere_solver_selection=unselected
```

The lower-Mach cases show non-monotone variation and unacceptable error. The
candidate is retained as negative evidence and a debug comparison, not as the
Enhanced solver.

The clean source package for the documented checkpoint is
`artifacts/vnext-phase5-source-9b7e7d095/`, SHA-256
`553FA58C897715099B47B8B1E500F0FAE51336CFEC99DA2A1787754CC5E44387`.
The archive has `1304` entries (`1303` source members plus the source manifest),
contains all required AtmosphereBench sources and runner, and includes zero test
assets.

## HLLC with Rusanov fallback front-runner

The next isolated candidate is clean-validated at `3eb235b8c`. It restores the
contact wave and uses Rusanov only when HLLC cannot construct valid interface
states. Current Low-Mach, near-vacuum, sealed Sod, open-leak and performance runs
record zero fallbacks and zero numerical corrections.

```text
very_low_mach_l1=0.0024269
very_low_mach_tv_ratio=0.980949
low_mach_suitability_passed=true
near_vacuum_min_density=1e-6
near_vacuum_min_pressure=1e-8
sod_shock_position=0.855469
open_boundary_mass_out=6.2341
throughput=18.8104M / 18.4543M / 18.6026M cell-updates/s
performance_gate=recorded_candidate_measurement_no_budget
atmosphere_solver_selection=unselected
```

This made HLLC/Rusanov-fallback the front-runner at the 1D checkpoint, not the
selected solver. At that checkpoint, RED items included PhysicalScale,
physical-time, multidimensional validation, convection, gas mixing and an accepted
CPU/memory budget. Details are in
[the HLLC checkpoint](phase-5-hllc-candidate.md).

The defensive fallback is independently covered at `78bad784d`: one valid
ultra-low-pressure/acoustic interface triggers exactly one fallback and the
returned flux matches the strict Rusanov reference.

The first multidimensional checkpoint is clean-validated at `d38120177`. A
`32x24` periodic uniform state remains unchanged with zero conservative drift,
fallback or correction. A `32x24` periodic pressure pulse evolves with positive
density and pressure, reduces its pressure peak from `1.09845` to `1.09381`, and
closes mass and energy to `6.9e-13`. X/Y flux storage is explicit; the measured
layout contract is `160 bytes/cell` for initial, current, next, Flux-X and Flux-Y
arrays. Both clean runners bind to the clean source commit and verified
strict-double flags. At this checkpoint, sealed heating, physical source
accounting, convection, gas mixing, physical-time selection and an accepted budget
were still open.

The sealed-heating/source-ledger checkpoint is clean-validated at `be0ff2f37`.
It adds a general conservative applied-source ledger and reusable sealed face
fluxes to the standalone 2D candidate. Uniform heating raises nondimensional mean
pressure and temperature from `1` to `1.06667`; mass and momentum drift remain
zero, while the `76.8` energy increase reconciles to the recorded source within
`3.21876e-11`. The source ledger closes, all `30720` source applications are
counted, and fallback/correction counts remain zero. The sealed face-array layout
is `162.333 bytes/cell` (`124672` bytes total for `32x24`). All three 2D clean
runners pass on the same commit. This is still not physical-time, material-data,
production TPT wall, performance or solver-selection evidence.

The gravity/source/boundary-ledger natural-convection checkpoint is clean-
validated at `83c5a0cd2`. An isothermal hydrostatic control is compared with a
localized bottom perturbation reaching nondimensional temperature `1.5`.
Control-subtracted thermal centre rises `0.225497` cell and the differential flow
contains a `0.0257047` updraft plus `-0.000157248` return flow. Both control and
heated runs remain positive, below CFL `0.032`, use zero fallback/corrections and
close combined gravity-source plus sealed-wall exchange within `8e-12`.
Source-only closure is deliberately false because wall pressure exchanges real
momentum. This is not a well-balanced hydrostatic proof or a physical-time claim.

Current HLLC two-dimensional checkpoint source package:
`artifacts/vnext-phase5-source-20309c670/`, SHA-256
`043746D179C1D2A0691FCA3C4A9AA728263E9B9C901301890928F992E9173529`,
`1306` source members plus manifest, zero test assets. Its manifest binds revision
`20309c6703f210600a7f605e54790ab922ecc9c1`.

## Passive conserved-species gas-mixing checkpoint

Checkpoint `55088a852` extends only the standalone HLLC 2D bench with one passive
species-A partial density. Species B is the complement against total gas density;
both species ledgers close with zero published drift over 320 nondimensional
periodic steps. Fractions remain in `[0,1]`, all 768 cells form a mixed region,
and total variation decreases from `48` to `47.9173`. Gas mass, momentum and
energy drift are zero; fallback and correction counts are also zero.

The authoritative fixture state is `40 bytes/cell`, and its explicit
initial/current/next plus X/Y flux working allocation is `200 bytes/cell`.
Species-EOS coupling and physical diffusion remain `not_implemented`, so this is
not yet a multi-species atmosphere. The clean result SHA-256 is
`F5B7C1FBB60CD2E7672AA5A50FC46A8600C3D8B43E7BE074234BDADC9D8982A4`.
The full scope, validation and rollback record are in
[the species-mixing checkpoint](phase-5-species-mixing.md).

The corresponding clean test-free source package is
`artifacts/vnext-phase5-source-03a83b865/`, SHA-256
`299EA74E8D98727CA945AACC2B894794A666351EFF9614E5937B006A7C4CE9BE`,
with `1309` source members plus manifest and zero test assets. The manifest binds
revision `03a83b865072ba20efe0072419cc428dd73f9a7e`.

Gas-mixing/species-conservation evidence is therefore no longer a Phase 5 gap.
At the species checkpoint, `V1_0_5_GATE` remained `IN_PROGRESS` for
two-dimensional performance and an accepted memory/frame budget, selected
PhysicalScale, physical-time policy and final solver selection. Production Air
remained unchanged and unauthorized for replacement.

## Target-size HLLC 2D performance checkpoint

Checkpoint `12e904f75` records strict-double, single-threaded, end-to-end periodic
performance over 32 steps after one warm-up, using the median of three runs:

```text
153x96=1.69129 ms/step, 8.68448M cell-updates/s
306x192=6.56627 ms/step, 8.94755M cell-updates/s
612x384=31.2385 ms/step, 7.52304M cell-updates/s
working_memory=160 bytes/cell
fallback_count=0 / 0 / 0
correction_count=0
```

The `153x96` case matches the current Legacy Air grid, while `612x384` matches
the particle grid. A 16.667 ms frame is recorded only as context; the candidate
budget and physical-time policy remain `unselected`. The clean result SHA-256 is
`2792A219A32A3F7C81EC93ABF9301B5D3D2EE48A6BC74D27A40CBB8DE7D1BF87`.
See [the performance checkpoint](phase-5-hllc-2d-performance.md).

The corresponding clean test-free source package is
`artifacts/vnext-phase5-source-ef423c4f5/`, SHA-256
`FDF8404AAFF38E23C1DBE5BCAF96EB750825CB95B899FE3B1BCE8C2DF2A267A5`,
with `1310` source members plus manifest and zero test assets. The manifest binds
revision `ef423c4f5a44a428e5f015855a087ace63c7ce96`.

At the performance checkpoint, the Phase 5 Gate remained open for an accepted
CPU/memory budget, PhysicalScale/time policy, actual Legacy-like/LBM comparison,
remaining mandatory cases and final solver selection.

## Isothermal D2Q9 LBM comparison

Checkpoint `5e3c46fae` turns D2Q9 from a registration placeholder into an actual
strict-double BGK collide-and-stream candidate. The periodic `64x48` uniform case
preserves population state to `4.26326e-14` L1. A `64x64` low-Mach shear wave
decays with `0.000577441` amplitude relative error against the configured lattice-
viscosity reference. Both runs conserve mass/momentum, retain positive populations
and record zero numerical corrections.

This comparison also supplies a decisive limitation: the implemented D2Q9 model
has no energy state and explicitly marks energy drift not applicable, near-vacuum
unsupported, shocks unsupported and species unimplemented. Its state/working
memory is `72/144 bytes/cell`. It is therefore not selected as the unified
OmniAtmosphere solver despite its low-Mach result. Clean result SHA-256 values are
`D601D2DDA76BECB73C37CE5CB309F6460A10B2516D50C135C8D7B39C26FA4333`
and `26180CBA90702B0458D30FA05C1867CEEAFA743D7CCF50BE7F393C29E7C12AE6`.
See [the D2Q9 checkpoint](phase-5-lbm-d2q9.md).

The corresponding clean test-free source package is
`artifacts/vnext-phase5-source-c8ae4ea8a/`, SHA-256
`3F1DB05D65D61701B726DE12BD959E43739291F3026D7BAC251E8DDF7B025876`,
with `1313` source members plus manifest and zero test assets. The manifest binds
revision `c8ae4ea8a7989965972825b26382b4ed485f5030`.

## Legacy-like executable control

Checkpoint `696d0c958` turns Legacy-like from a registration placeholder into an
actual strict-double comparison control. A `64x48` periodic uniform state remains
unchanged for 32 steps. A `64x48` pressure pulse evolves for 96 steps, reducing
its peak from `0.99005` to `0.236479`, generating maximum velocity `0.321191`,
and preserving the pressure-field sum within `8.52651e-13`. Both probes remain
finite and record zero corrections. State/current-plus-next memory is
`32/64 bytes/cell`.

The model is explicitly a dimensionless pressure/velocity stencil, not production
Legacy Air equivalence. It has no physical mass, density, momentum-density,
energy, species, EOS or vacuum state. The JSON mass/momentum/energy drift fields
are therefore `null`, not fake zero. Clean result SHA-256 values are
`505F69D6D87724CC1B8E4410CA13BE91AF9464E22B12BBE338C601DB74CF04AD`
and `D1FE38268F52B15AB6BFA7D135B77122A79D48C1227A1E1F5909AEBB4AD0DFBA`.
See [the Legacy-like checkpoint](phase-5-legacy-like-control.md).

The corresponding clean test-free source package is
`artifacts/vnext-phase5-source-950ba3974/`, SHA-256
`3DF031CEFC765DAEF8DE63D6FB749307CE7283C9C89F003BA63EE6CEDB2F2D77`,
with `1316` source members plus manifest and zero test assets. The manifest binds
revision `950ba397417605782622b057ee82b3d1511000ee`.

## Reference-machine budget and time-policy rejection

Checkpoint `76300cd98` accepts the Phase 5 reference-machine target of one quarter
of a 16.667 ms reference tick (`4.1666667 ms/tick`), `64` authoritative bytes/cell
and `256` working bytes/cell. This is not a cross-hardware release requirement and
does not derive simulation dt from presentation FPS. The current strict-double
single-thread HLLC result fits at `1.69129 ms/substep` and `32/160 bytes/cell`,
leaving room for at most two whole measured substeps.

The policy validator then binds the 4 mm cell, CFL `0.2`, the 1/60-second candidate
and a public NASA 344 m/s air sound-speed reference. It recomputes 7167 required
explicit substeps and `12121.47543 ms/tick`, rejecting direct real-acoustic explicit
HLLC. Two affordable substeps would cap signals around `0.096 m/s`, so uniform
acoustic scaling is rejected as the default realism policy. A hybrid/all-speed
low-Mach plus event-local compressible policy is therefore required.
See [the policy/budget checkpoint](phase-5-atmosphere-policy-budget.md).

The corresponding clean test-free source package is
`artifacts/vnext-phase5-source-40facfdfa/`, SHA-256
`293105F3E87D964608CE8D0ACF9D1EBD16417FF4C89623D3400941E9C753706D`,
with `1319` source members plus manifest and zero test assets. The manifest binds
revision `40facfdfa7da907e539b2eb109f78299167c4cd1`.

## Mixed-region hybrid checkpoint

The reviewed component implementation is `c63f4e652`; it remains valid as a
standalone comparison. Follow-up `fd3538192` adds a single-domain 1D mixed-region
router with Mach/pressure-jump hysteresis, promotion/demotion, cross-route HLLC
faces, four event-local substeps and conservative bulk reflux. The clean result
records `1696` promotions, `1634` demotions, `5432` cross-route face applications,
maximum CFL `0.272276`, and zero HLLC fallback/correction events. Interface and
global mass/momentum/energy ledgers close to floating-point tolerance, and the
threshold scan passes.

This is deliberately bounded evidence, not a selected solver or production
hybrid implementation. The worst case promotes all `64` cells
(`maximum_event_fraction=1.0`), so no performance benefit is claimed. Physical
acoustic domain of dependence is not implemented; the current halo is only a
benchmark-region diagnostic. General low-Mach pressure coupling, near-vacuum and
species routing, 2D coupling, target-grid cost, accepted physical time and
production integration remain open. `hybrid_end_to_end_passed=false`,
`policy_selection_ready=false` and `ATMOSPHERE_SOLVER_SELECTION=unselected` remain
the required disposition.

Clean evidence is bound to:

```text
source_commit=fd353819287cef3ee05db4f7d2a4309c1ba4924a
source_dirty=false
runner_artifact=artifacts/vnext-atmospherebench-hybrid-mixed-region/20260811T045949Z-9f874fff/result.json
result_sha256=861CB26AA846AE07E770880AD95878B4D9DDB7747526CA298D3CA23691CB2D07
source_package=artifacts/vnext-phase5-source-fd3538192/TPT-ZH-OmniPack-1.0.0-Source.zip
source_sha256=ED71B5E45DAD84D847B199D7D563ECBB13B5E2FA39698246F73C32CE0B9FDFAF
source_members=1324
test_assets=0
full_build=620/620
meson_static=72/72
python_discovery=428_total_426_pass_2_skipped
```

The Phase 5 reference-machine CPU/memory sub-gate and the 1D mixed-region
sub-gate are GREEN. PhysicalScale, physical-time policy, 2D/domain-of-dependence,
near-vacuum/species routing, target-grid budget, remaining precision matrix,
G0 physical-ledger semantics and formal solver selection remain RED/unselected.
HLLE and LBM remain registration-only candidates; production Air is unchanged.

## 2D hybrid and target-grid budget rejection

Checkpoint `ab273322d` adds a periodic 2D mixed-region router/reflux probe and a
short-run matrix for `153x96`, `306x192` and `612x384`. The bounded 32x24 fixture
passes promotion/demotion, X/Y cross-route exchange, reflux, threshold scan,
positivity and global conservation, but reaches maximum event fraction `0.927083`.

At the candidate 4 mm atmosphere-cell scale, `344 m/s` over one `1/60 s` tick is
`1434` cells. This exceeds the largest matrix dimension and keeps physical domain
of dependence unimplemented. The matrix costs are `5.56335`, `26.5715` and
`106.711 ms/macro step`, all above the `4.16667 ms` reference budget, despite
small initial event fractions. Therefore the current hybrid implementation is a
useful correctness probe but a rejected physical-time/default-performance policy.

Clean result SHA-256 is
`CE15A5764DDC9A2E9949ED854A357F2E423FA35F37BAF1C244ED8EF1ED069C1B`.
`hybrid_mixed_region_2d_end_to_end_passed=true` applies only to this bounded
benchmark; overall `hybrid_end_to_end_passed=false`, solver selection is
unselected and 1.0.6 remains blocked.
