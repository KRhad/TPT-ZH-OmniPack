# 1.0.5 strict-double Rusanov candidate checkpoint

## Scope

This is an isolated AtmosphereBench candidate implementation. It does not
replace Legacy `Air`, `Simulation`, `Particle`, Save, Lua, SDL, or any other
production path. PhysicalScale, physical time, and solver selection remain
unselected. HLLE and LBM remain registration-only candidates.

```text
TARGET_VERSION=1.0.5
BASE_COMMIT=1ba507e89e3d713fe355c03c2fc6e7139aabcb49
UNIFORM_IMPLEMENTATION_COMMIT=4b0658d8ff7bc56169fd8ed5d649f8c6b4250b44
PRESSURE_PULSE_IMPLEMENTATION_COMMIT=cded7be672fbb2755174499214bb31622979eac6
STATUS=IN_PROGRESS_RUSANOV_PRESSURE_PULSE_CLEAN_VALIDATED
BRANCH=integration/omnicore-vnext
PHYSICAL_SCALE_SELECTION=UNSELECTED
PHYSICAL_TIME_POLICY=UNSELECTED
ATMOSPHERE_SOLVER_SELECTED=false
CANDIDATES_REGISTERED=4
CANDIDATE_SOLVERS_IMPLEMENTED=1
RUSANOV_SCOPE=1D_PERIODIC_UNIFORM_AND_PRESSURE_PULSE_DEBUG_PROBES_ONLY
HLLE_STATUS=REGISTERED_ONLY
LBM_STATUS=REGISTERED_ONLY
PRODUCTION_INTEGRATION=false
V1_0_5_GATE=IN_PROGRESS
```

## Candidate implementation

`tools/atmospherebench/Rusanov1D.{h,cpp}` contains a first-order finite-volume
Rusanov (local Lax-Friedrichs) flux over a one-dimensional periodic grid. The
reference state is strict `double`; the target is compiled with:

```text
-fno-fast-math
-fno-unsafe-math-optimizations
-ffp-contract=off
```

The probe uses a synthetic nondimensional ideal-gas fixture (`gamma=5/3`,
`R=1`) and four conservative values per cell: density, x/y momentum and total
energy density. It runs a `64 x 1` uniform preservation probe (`dt=0.05`,
`16` steps) and a `128 x 1` pressure-pulse probe (`dt=0.02`, `64` steps), both
with periodic faces. They record the maximum CFL, primitive positivity,
conservative ledger closure and numerical corrections. The state is intentionally
not presented as SI air.

The implemented candidate is explicitly not a solver selection:

```text
candidate=fvm_rusanov
candidate_solver_implemented=true
result_status=candidate_result_not_selection
atmosphere_solver_selection=unselected
```

The uniform and pressure-pulse probes are debugging floors. They do not establish
shock, contact accuracy, near-vacuum, low-Mach, leak, source-term, multi-species
or production performance behavior.

## Clean evidence

The clean runner was executed from `4b0658d8f` after rebuilding the unique
Meson target and checking its compile commands and strict flags.

Rusanov probe artifact:

```text
artifacts/vnext-atmospherebench/20260810T150358Z-1c03a6e5/result.json
result_sha256=CC64E1BC2B893C99F846E1DCE5E2FABE7C61A804CFE345F5486E2300FDA1AB76
benchmark_kind=atmospherebench_rusanov_uniform_probe
performance_gate=not_evaluated_candidate_probe
```

Measured probe values:

```text
grid=64x1
dt=0.05
steps=16
maximum_cfl=0.0645497
positivity_preserved=true
minimum_density=1
minimum_pressure=1
minimum_energy_density=1.5
mass_drift=0
momentum_x_drift=0
momentum_y_drift=0
energy_drift=0
numerical_correction_count=0
state_bytes_per_cell=32
state_and_flux_scratch_bytes_per_cell=96
probe_passed=true
```

The default runner mode was also rerun at the same code checkpoint:

```text
artifacts/vnext-atmospherebench/20260810T150425Z-56cb4efe/result.json
benchmark_kind=atmospherebench_contract_uniform
performance_gate=not_evaluated_contract_only
```

The candidate-list check in both runner modes confirms that HLLE and LBM are
still `registered_only|solver_implemented=false` and that candidate selection
is `unselected`.

## Pressure-pulse evidence

The pressure-pulse probe was clean-run from `cded7be67`. Its initial condition is
a generic Gaussian pressure perturbation over a uniform nondimensional gas; it is
not keyed to the test name in the solver. The runner requires measurable state
evolution, a reduced pressure peak, positive primitives, zero correction events,
periodic ledger closure and CFL at most one.

```text
artifacts/vnext-atmospherebench/20260810T161738Z-abc38f66/result.json
result_sha256=CB84DF1CB5341A98BB17E50E6351955F33282BC5649133C8230546EDCFF20106
benchmark_kind=atmospherebench_rusanov_pressure_pulse_probe
performance_gate=not_evaluated_candidate_probe
```

Measured values:

```text
grid=128x1
dt=0.02
steps=64
maximum_cfl=0.0270897
initial_maximum_pressure=1.1
final_maximum_pressure=1.09652
pressure_peak_reduced=true
state_change_l1=0.373615
state_evolved=true
minimum_density=0.99882
minimum_pressure=1
mass_drift=-1.42109e-14
momentum_x_drift=6.88814e-18
momentum_y_drift=0
energy_drift=5.68434e-14
numerical_correction_count=0
probe_passed=true
```

## Validation

- Full default Meson build: `80/80` targets passed.
- Meson static suite: `45/45` passed, including both Rusanov probes.
- Python discovery: `414` tests, `412` passed, `2` declared skips.
- Targeted AtmosphereBench/runner contract tests: `17/17` passed.
- Windows PowerShell 5.1 runner parse and clean execution: passed.
- Source package: `artifacts/vnext-phase5-source-cded7be67/`, `1304` zip
  entries; Rusanov sources and runner are present; package SHA-256 is
  `47C66A4E1A3E80201404C475FCB8C871F4989B940D862E5531797C4F0A70809E`.
- Production-consumer scan: no `src/` consumer of AtmosphereBench or Rusanov.

## Gate and next work

This checkpoint is GREEN for the isolated Rusanov debug probe only. It is not a
GREEN solver-selection gate. `V1_0_5_GATE` remains `IN_PROGRESS` because the
mandatory non-uniform hydrodynamics cases, near-vacuum behavior, boundary/source
ledger, memory/performance budget and physical-time policy are not established.

The next permitted work is another isolated Rusanov density-advection or contact
case with its own deterministic evidence. Do not add HLLE/LBM steps, select a
solver, or integrate production Atmosphere as part of this checkpoint.

Rollback commit: `f6c418c705796e2783a9db58b34080eb728b1d27` restores the
uniform-only Rusanov checkpoint.
