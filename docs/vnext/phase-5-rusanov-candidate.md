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
STATUS=IN_PROGRESS_RUSANOV_NEAR_VACUUM_CLEAN_VALIDATED
BRANCH=integration/omnicore-vnext
PHYSICAL_SCALE_SELECTION=UNSELECTED
PHYSICAL_TIME_POLICY=UNSELECTED
ATMOSPHERE_SOLVER_SELECTED=false
CANDIDATES_REGISTERED=4
CANDIDATE_SOLVERS_IMPLEMENTED=1
RUSANOV_SCOPE=1D_PERIODIC_UNIFORM_PRESSURE_PULSE_DENSITY_ADVECTION_CONTACT_NEAR_VACUUM_DEBUG_PROBES_ONLY
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
still does not establish shock convergence, low-Mach, leak, source-term,
multi-species or production performance behavior.

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

- Full default Meson build: `80/80` targets passed.
- Meson static suite: `48/48` passed, including all five Rusanov probes.
- Python discovery: `414` tests, `412` passed, `2` declared skips.
- Targeted AtmosphereBench/runner contract tests: `17/17` passed.
- Windows PowerShell 5.1 runner parse and clean execution: passed.
- Source package: `artifacts/vnext-phase5-source-0ee4b7561/`, `1304` zip
  entries; the package predates the contact and near-vacuum follow-ups, while its Rusanov sources
  and runner are present; package SHA-256 is
  `A4255697997C5BDD6A10EE15EDC3EE2E9F23635CDF6BD6639AD8BE8E8FD83717`.
- Production-consumer scan: no `src/` consumer of AtmosphereBench or Rusanov.

## Gate and next work

This checkpoint is GREEN for the isolated Rusanov debug probe only. It is not a
GREEN solver-selection gate. `V1_0_5_GATE` remains `IN_PROGRESS` because the
mandatory shock/hydrodynamics cases, boundary/source
ledger, memory/performance budget and physical-time policy are not established.

The next permitted work is an isolated Rusanov Sod-shock
case with its own deterministic evidence. Do not add HLLE/LBM steps, select a solver,
or integrate production Atmosphere as part of this checkpoint.

Rollback commit: `f9a62fb3a` restores the uniform, pressure-pulse, smooth
density-advection and contact-discontinuity Rusanov checkpoint.
