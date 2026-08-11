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
DENSITY_ADVECTION_IMPLEMENTATION_COMMIT=0ee4b756176413c4261f74c3b6a6bbcb4298eae8
CONTACT_IMPLEMENTATION_COMMIT=68bcc74a5574ee1fc9240576c04a31a8f9335484
NEAR_VACUUM_IMPLEMENTATION_COMMIT=54b3060ab996b6387e5aaf11283eaea1bb9e8faa
SOD_IMPLEMENTATION_COMMIT=588d38d32ec4904118e741e5f5f614c69b8de3de
REFINEMENT_IMPLEMENTATION_COMMIT=d541c2c2e9809691d625294e918c46009cd4a651
LOW_MACH_IMPLEMENTATION_COMMIT=a948a48db2c7d06b93dd0f26fb67ad7f1423968c
OPEN_LEAK_IMPLEMENTATION_COMMIT=ef0ca86c1
PERFORMANCE_IMPLEMENTATION_COMMIT=ee9290cb7
STATUS=IN_PROGRESS_RUSANOV_BOUNDARY_PERFORMANCE_CHARACTERIZED
BRANCH=integration/omnicore-vnext
PHYSICAL_SCALE_SELECTION=UNSELECTED
PHYSICAL_TIME_POLICY=UNSELECTED
ATMOSPHERE_SOLVER_SELECTED=false
CANDIDATES_REGISTERED=4
CANDIDATE_SOLVERS_IMPLEMENTED=1
RUSANOV_SCOPE=1D_UNIFORM_PRESSURE_PULSE_DENSITY_ADVECTION_CONTACT_NEAR_VACUUM_SOD_REFINEMENT_LOW_MACH_OPEN_LEAK_PERFORMANCE_DEBUG_PROBES_ONLY
RUSANOV_LOW_MACH_SUITABILITY=false
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
`16` steps), a `128 x 1` pressure-pulse probe (`dt=0.02`, `64` steps), a
`128 x 1` density-advection probe (`dt=0.1`, `20` steps), and a `128 x 1`
constant-pressure contact-discontinuity probe (`dt=0.1`, `20` steps), all with
periodic faces. It also runs a `128 x 1` near-vacuum expansion probe (`dt=0.02`,
`32` steps) from a `rho=1, p=1` half-domain into a `rho=1e-6, p=1e-8`
half-domain. They record the maximum CFL, primitive positivity,
conservative ledger closure and numerical corrections. The state is intentionally
not presented as SI air.

The sealed Sod case uses 256 cells over a unit interval, `gamma=1.4`, `dt=0.0005`
and 400 steps (`t=0.2`). It starts from the canonical left state `(rho=1, p=1)`
and right state `(rho=0.125, p=0.1)`. Sealed-wall pressure impulse is recorded as
an explicit boundary exchange before evaluating the conservative balance.

The implemented candidate is explicitly not a solver selection:

```text
candidate=fvm_rusanov
candidate_solver_implemented=true
result_status=candidate_result_not_selection
atmosphere_solver_selection=unselected
```

These probes are debugging floors. The contact probe exposes expected first-order
Rusanov diffusion but does not establish grid convergence. The near-vacuum case
establishes one positive, conservative synthetic density/pressure-ratio run with no
floor correction; it is not a production-vacuum model or a parameter sweep. The set
still does not establish a broad vacuum operating envelope. The smooth-advection
refinement case establishes first-order convergence over one
three-level test. The low-Mach characterization then demonstrates that the same
first-order compressible Rusanov method becomes excessively diffusive as nominal
Mach decreases. The set still does not establish leak, source-term, multi-species
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

## Smooth-advection refinement evidence

The three-level refinement probe was clean-run from `d541c2c2e`. It keeps the
unit domain, `t=0.25`, `u=0.5`, and nondimensional CFL policy fixed while doubling
resolution from 64 to 128 to 256 cells. The analytic periodic shifts are exactly
8, 16 and 32 cells. The observed L1 orders are checked against a published
first-order window of `0.8..1.2`.

```text
artifacts/vnext-atmospherebench/20260810T181559Z-940bf354/result.json
result_sha256=C433628882B6C25999C9B8C5AE17AB4120BCE7EBCF4EC8906CC962D456776306
benchmark_kind=atmospherebench_rusanov_density_advection_refinement_probe
performance_gate=not_evaluated_candidate_probe
```

Measured values:

```text
cells=64,128,256
steps=64,128,256
total_simulated_time=0.25
reference_shift_cells=8,16,32
density_l1_error=0.0159718,0.00824503,0.00418803
density_linf_error=0.0264754,0.0138321,0.00707776
coarse_to_medium_l1_order=0.95393
medium_to_fine_l1_order=0.977252
maximum_cfl=0.48579,0.48583,0.485841
fine_mass_drift=4.26326e-13
fine_energy_drift=-4.54747e-13
pressure_linf_error<=6.66134e-16
numerical_correction_count=0
refinement_passed=true
```

## Sod shock-tube evidence

The sealed Sod probe was clean-run from `588d38d32`. It detects the right-moving
pressure front and verifies a declared first-order `x=0.8..0.9` window at `t=0.2`,
positive bounded primitives, CFL, and a boundary-adjusted conservation ledger. The
raw x-momentum change is not hidden: it equals the recorded wall-pressure impulse.

```text
artifacts/vnext-atmospherebench/20260810T180228Z-6cfef9c1/result.json
result_sha256=B348F45F7FD1D03CB49062FD7F4D2813EE5561146489DF1309662B4A12DC7AFA
benchmark_kind=atmospherebench_rusanov_sod_shock_tube_probe
performance_gate=not_evaluated_candidate_probe
```

Measured values:

```text
grid=256x1
cell_length=0.00390625
dt=0.0005
steps=400
simulated_time=0.2
maximum_cfl=0.280563
shock_position=0.855469
maximum_velocity_x=0.927941
minimum_density=0.125
maximum_density=1
minimum_pressure=0.1
maximum_pressure=1
mass_drift=-2.84217e-14
momentum_x_drift=46.08
boundary_momentum_x_exchange=46.08
momentum_x_balance_error=-3.83693e-13
energy_balance_error=-5.68434e-14
boundary_ledger_closes=true
density_floor_hits=0
pressure_floor_hits=0
numerical_correction_count=0
state_and_flux_scratch_bytes_per_cell=96.125
state_and_flux_scratch_bytes_total=24608
probe_passed=true
```

The default runner mode was also rerun at the same code checkpoint:

```text
artifacts/vnext-atmospherebench/20260810T150425Z-56cb4efe/result.json
benchmark_kind=atmospherebench_contract_uniform
performance_gate=not_evaluated_contract_only
```

The candidate-list check in all clean runner modes confirms that HLLE and LBM are
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

## Low-Mach characterization

Clean artifact:

```text
artifacts/vnext-atmospherebench/20260810T182953Z-88fe2d27/result.json
SHA256=F242646484BE49A791C2AD24B1475A6BA1272B57281C5C151A2FCE71B2E3454C
source_commit=a948a48db2c7d06b93dd0f26fb67ad7f1423968c
source_dirty=false
benchmark_execution_status=PASS
low_mach_suitability_passed=false
```

The periodic 128-cell probe transports the same smooth density profile at three
nominal Mach numbers while preserving the nondimensional pressure and reference
travel distance. The solver stays positive, closes its conservative ledger near
machine precision and records no floor/correction events, but its density error and
loss of total variation increase sharply as acoustic speed dominates advection:

```text
nominal_mach=0.387298 / 0.0387298 / 0.00387298
density_l1_error=0.00826755 / 0.0515261 / 0.126479
total_variation_ratio=0.934805 / 0.595493 / 0.00673822
low_to_moderate_l1_ratio=6.23233
very_low_to_moderate_l1_ratio=15.2982
mass_drift=1.13687e-13
momentum_x_drift=-1.11022e-16
energy_drift=-1.13687e-13
numerical_correction_count=0
```

This is a valid benchmark execution and a negative suitability result. It is not
acceptable to relax the published thresholds merely to select the candidate.
First-order compressible Rusanov remains useful as a strict reference/debug floor,
but it is not selected as the sole Enhanced-mode low-Mach atmosphere solver.

## Open-boundary leak ledger

Clean artifact:

```text
artifacts/vnext-atmospherebench/20260810T185503Z-6354ae45/result.json
SHA256=E0455CA1F474A8A1BA1F0BFB1DA31BAF226E58C24C155BE3E310F3E721B2457E
source_commit=ef0ca86c16f13d48deeb06be4e6fb3a3368fadee
source_dirty=false
benchmark_execution_status=PASS
```

The nondimensional 128-cell case uses a sealed left wall and a fixed low-pressure
right reservoir. It intentionally records raw domain drift and the separately
integrated left/right boundary exchanges. The adjusted conservative ledger closes:

```text
initial_mass=128
final_mass=121.301
right_boundary_mass_out=6.69874
left_boundary_mass_exchange=0
mass_balance_error=2.84217e-14
momentum_x_balance_error=-1.77636e-15
energy_balance_error=-2.84217e-14
maximum_cfl=0.250374
minimum_density=0.381382
minimum_pressure=0.305003
numerical_correction_count=0
```

This proves the candidate can make boundary loss explicit instead of disguising it
as conservation drift. It is not a TPT wall/permeability implementation and does
not select a physical scale or production boundary condition.

## Standalone strict-double performance record

Clean artifact:

```text
artifacts/vnext-atmospherebench/20260810T190429Z-b16432bf/result.json
SHA256=9C6EF0446F17A945D9F1957FA1F29C2F076BE7B3E1B1713C0DB21685B2FF5788
source_commit=ee9290cb7893e5f4a316db5372f93d403477e1a1
source_dirty=false
performance_gate=recorded_candidate_measurement_no_budget
```

The single-threaded 1D strict-double benchmark includes state allocation, flux
storage and validation. Each size has one warm-up and three measured repetitions;
the reported value is the median:

```text
cells=14688 / 29376 / 58752
steps_per_repeat=64
elapsed_ms=30.3009 / 59.9321 / 114.617
cell_updates_per_second=31.0232M / 31.3699M / 32.8059M
state_bytes_per_cell=32
state_and_flux_scratch_bytes_per_cell=96
```

This is a real recorded measurement for the isolated implementation, not an
accepted TPT frame budget. It excludes 2D fluxes, source terms, boundary cache,
species, rendering and production coupling.

## Near-vacuum expansion evidence

The near-vacuum probe was clean-run from `54b3060ab`. The periodic domain contains
equal dense and low-density halves. Generic Rusanov fluxes transfer mass into the
low-density half while the global conservative ledger remains closed. No density or
pressure floor exists in this candidate; the probe fails if a primitive becomes
non-positive.

```text
artifacts/vnext-atmospherebench/20260810T174444Z-1ba1f887/result.json
result_sha256=FE7C063A85E18B815B8CD8AD7B89C402BF253D1864316A83A3950DC488385924
benchmark_kind=atmospherebench_rusanov_near_vacuum_expansion_probe
performance_gate=not_evaluated_candidate_probe
```

Measured values:

```text
grid=128x1
dt=0.02
steps=32
maximum_cfl=0.0608536
minimum_density=1e-6
minimum_pressure=1e-8
initial_low_density_region_mass=6.4e-05
final_low_density_region_mass=0.875658
low_density_region_mass_increased=true
state_change_l1=5.89665
mass_drift=9.9476e-14
momentum_x_drift=0
momentum_y_drift=0
energy_drift=7.10543e-14
density_floor_hits=0
pressure_floor_hits=0
numerical_correction_count=0
probe_passed=true
```

## Density-advection evidence

The density-advection probe was clean-run from `0ee4b7561`. It initializes
`rho=1+0.2 sin(2 pi x/N)`, `p=1`, and `u=0.5`. Its nondimensional duration is
chosen so the analytic periodic reference is exactly one cell to the right. The
runner checks density L1/Linf error, pressure preservation, total-variation ratio,
conservative drift, positivity, CFL and the correction ledger.

```text
artifacts/vnext-atmospherebench/20260810T170713Z-917bb2b4/result.json
result_sha256=44D3E5F4A8367E9D8196593CA8BB81B231B1CDB1AC02B0054C0E1055EB36DFA1
benchmark_kind=atmospherebench_rusanov_density_advection_probe
performance_gate=not_evaluated_candidate_probe
```

Measured values:

```text
grid=128x1
dt=0.1
steps=20
maximum_cfl=0.194338
reference_velocity=0.5
reference_shift_cells=1
density_l1_error=0.000543106
density_linf_error=0.000921691
pressure_linf_error=4.44089e-16
total_variation_ratio=0.995707
advection_reference_passed=true
mass_drift=9.9476e-14
momentum_x_drift=3.55271e-14
energy_drift=-1.7053e-13
numerical_correction_count=0
probe_passed=true
```

## Contact-discontinuity evidence

The contact probe was clean-run from `68bcc74a5`. It initializes a periodic
density step (`1.2 / 0.8`) at constant `p=1` and `u=0.5`; after 20 steps the
analytic reference is exactly one cell to the right. The public first-order
acceptance bounds are density L1 at most `0.02`, density Linf at most `0.2`, and
pressure Linf at most `1e-10`. An earlier `0.12` Linf proposal was rejected after
the measured `0.161912` exposed the expected Rusanov diffusion; the recorded
bound is a declared first-order debug limit, not an accuracy-selection result.

```text
artifacts/vnext-atmospherebench/20260810T172104Z-1716955e/result.json
result_sha256=67CE5BAD2E05E3A6B7E827301CF55A70E6FC4286803BF93DCBD38F872015EB39
benchmark_kind=atmospherebench_rusanov_contact_discontinuity_probe
performance_gate=not_evaluated_candidate_probe
```

Measured values:

```text
grid=128x1
dt=0.1
steps=20
maximum_cfl=0.194338
density_l1_error=0.00923098
density_linf_error=0.161912
pressure_linf_error=3.33067e-16
total_variation_ratio=1
density_bounds_preserved=true
minimum_density=0.8
maximum_density=1.2
mass_drift=7.10543e-14
momentum_x_drift=3.55271e-14
energy_drift=2.84217e-14
numerical_correction_count=0
probe_passed=true
```

## Validation

- Full default Meson build: `79/79` build steps passed at the performance
  checkpoint (`atmospherebench` was already up to date before the full build).
- Meson static suite: `53/53` passed, including the low-Mach, open-boundary and
  performance probes.
- Python discovery: `414` tests, `414` passed, `0` declared skips.
- Targeted PhysicalScale/AtmosphereBench/runner contract tests: `29/29` passed.
- Windows PowerShell 5.1 runner parse and clean execution: passed.
- Source package: `artifacts/vnext-phase5-source-ee9290cb7/`, `1304` zip
  entries (`1303` source members plus manifest); all current Rusanov sources and
  runner are present, no test assets are included, and package SHA-256 is
  `5130F6B8871F196BB292DF590D74DC552924005EAB206CC16CB810D1AF1E5C9A`.
- Production-consumer scan: no `src/` consumer of AtmosphereBench or Rusanov.

## Gate and next work

This checkpoint is GREEN for executing and characterizing the isolated Rusanov
debug candidate. Its measured low-Mach suitability is **false**, so it is not a
GREEN solver-selection gate. `V1_0_5_GATE` remains `IN_PROGRESS` because an
acceptable low-Mach path, accepted performance budget and physical-time policy are
not established. Leak/source accounting is now explicit for the isolated
open-boundary case, and standalone performance is recorded without a selected
budget.

The next permitted work is an isolated all-speed/hybrid low-Mach candidate study,
with Rusanov retained as the strict reference/debug floor. No solver may be
selected until the candidate comparison includes acceptable low-Mach, shock,
near-vacuum, boundary-ledger and measured budget evidence.
HLLE and LBM remain registered-only until separately authorized; production
Atmosphere integration remains forbidden.

## All-speed Rusanov follow-up (negative result)

Commit `fae9a0847` adds a fifth, independently registered
`fvm_all_speed_rusanov` candidate. It shares the conservative state and runner
but injects a generic local-Mach/pressure-jump dissipation scale. The candidate
is not selected and does not touch production simulation code.

The clean runner result is bound to the current source commit and rebuilt Meson
target. The execution passed with zero numerical corrections, but the suitability
gate failed at very low Mach:

```text
benchmark_execution_status=PASS
very_low_density_l1_error=0.10116
very_low_total_variation_ratio=1.46479
low_mach_suitability_passed=false
candidate_disposition=reject_low_mach_suitability
atmosphere_solver_selection=unselected
```

Since the mandatory Low-Mach gate failed, no Sod, near-vacuum, boundary or
performance claims are attached to this candidate. HLLE and LBM remain
`registered_only`.

Clean source package: `artifacts/vnext-phase5-source-9b7e7d095/`, SHA-256
`553FA58C897715099B47B8B1E500F0FAE51336CFEC99DA2A1787754CC5E44387`,
`1303` source members plus manifest, zero test assets.

Rollback commit: `18ddcec3f` restores the documented pre-leak/performance
Rusanov checkpoint.
