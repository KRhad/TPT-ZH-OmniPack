# 1.0.5 strict-double HLLC with Rusanov fallback checkpoint

```text
TARGET_VERSION=1.0.5
BASE_COMMIT=52e94c5aa
IMPLEMENTATION_COMMIT=55088a8523421b8ef5c1c0b6be3170fdf505ac34
STATUS=GREEN_ISOLATED_CANDIDATE_PASSIVE_SPECIES_NOT_SELECTED
ATMOSPHERE_SOLVER_SELECTION=UNSELECTED
PHYSICAL_SCALE_SELECTION=UNSELECTED
PHYSICAL_TIME_POLICY=UNSELECTED
HLLE=REGISTERED_ONLY
LBM=REGISTERED_ONLY
PRODUCTION_AIR_CHANGED=false
```

## Goal and architecture

This checkpoint adds a first-order strict-double HLLC numerical flux to the
standalone AtmosphereBench. HLLC restores the contact wave missing from a two-wave
HLL approximation; the original method is described by Toro, Spruce and Speares,
[Restoration of the contact surface in the HLL-Riemann solver](https://doi.org/10.1007/BF01414629).

The implementation validates primitive and star states and falls back to the
existing conservative Rusanov flux when HLLC cannot construct a valid interface
state. Every fallback is counted. The five published 1D runs and both published
2D runs used zero fallbacks; a separate ultra-low-pressure/acoustic interface
contract records exactly one
fallback and verifies that the returned flux matches the strict Rusanov path.
No third-party source code or data was copied.

The candidate remains isolated to `tools/atmospherebench`. It does not modify or
link production Air, Simulation, Particle, Save, Lua, renderer, SDL or element
code. HLLE and LBM remain `registered_only`.

The shared bench contract now also has a `ConservativeSourceLedger`, separate
from `NumericalCorrectionLedger`. It records applied mass, momentum and energy
source deltas and rejects non-finite or uncounted state. A result with intentional
heating closes against the source ledger; it is not mislabeled as numerical drift
or correction.

## Numerical results

All results are nondimensional strict-double candidate measurements. They are not
SI-scale or production gameplay claims.

### Low-Mach advection

```text
nominal_mach=0.387298 / 0.0387298 / 0.00387298
density_l1_error=0.00215349 / 0.0023944 / 0.0024269
total_variation_ratio=0.983093 / 0.981204 / 0.980949
very_low_mass_drift=0
very_low_energy_drift=-1.98952e-13
fallback_count=0 / 0 / 0
numerical_correction_count=0
low_mach_suitability_passed=true
```

The unchanged gate requires very-low-Mach L1 `<= 0.05` and TV ratio `>= 0.8`.
Unlike the Rusanov candidates, HLLC passes without tuning the threshold.

### Near-vacuum expansion

```text
minimum_density=1e-6
minimum_pressure=1e-8
low_density_region_mass=6.4e-5 -> 0.612243
mass_drift=8.52651e-14
energy_drift=8.52651e-14
fallback_count=0
numerical_correction_count=0
```

### Sealed Sod shock tube

```text
shock_position=0.855469
maximum_velocity_x=0.929874
mass_balance_error=1.7053e-13
momentum_x_balance_error=-3.62377e-13
energy_balance_error=1.7053e-13
fallback_count=0
numerical_correction_count=0
```

The raw momentum change is the sealed-wall pressure impulse and closes after the
boundary ledger adjustment.

### Open-boundary leak

```text
right_boundary_mass_out=6.2341
mass_balance_error=7.10543e-14
momentum_x_balance_error=-2.66454e-15
energy_balance_error=8.52651e-14
fallback_count=0
numerical_correction_count=0
```

### Candidate performance

The clean runner records one warm-up and three repetitions over 64 steps:

```text
cells=14688 / 29376 / 58752
throughput=18.8104M / 18.4543M / 18.6026M cell-updates/s
state_and_flux_scratch=96 bytes/cell
performance_gate=recorded_candidate_measurement_no_budget
```

HLLC is measurably slower than the current Rusanov reference (`31.0M-32.8M`
cell-updates/s), but no production frame budget or CPU crossover has been
accepted, so this is not a performance PASS/FAIL decision.

### Periodic two-dimensional probes

Checkpoint `d38120177` extends the same flux into X and Y directions. The Y flux
uses an explicit momentum-component rotation, and the update is dimensionally
split over periodic faces. It is still first-order strict-double candidate code,
not a production boundary or source-term implementation.

```text
uniform_grid=32x24
uniform_maximum_cfl=0.0546398
uniform_mass_momentum_energy_drift=0 / 0 / 0
uniform_state_change_l1=0

pressure_pulse_grid=32x24
pressure_pulse_maximum_cfl=0.0270629
minimum_density=0.997462
minimum_pressure=1
pressure_peak=1.09845 -> 1.09381
mass_drift=-3.41061e-13
energy_drift=6.82121e-13
state_change_l1=2.27797

fallback_count=0 / 0
numerical_correction_count=0 / 0
state_and_flux_scratch=160 bytes/cell
```

The two clean runner results are bound to `d38120177`, report
`source_dirty=false` and `strict_reference_flags_verified=true`, and retain
`physical_scale_selection=unselected` and
`atmosphere_solver_selection=unselected`. Their local result SHA-256 values are
`67A9C60521996ACBFD1529008AD4EF7EB201A08560FFCFEA387064359275A496` and
`8DC3B28D950831B77780C0694D4A72F4EBCF78B0817503B7BA022DB33E687A89`.

### Sealed heating and applied-source ledger

Checkpoint `be0ff2f37` adds reusable reflective face fluxes and a uniform
volumetric energy-source application to the isolated 2D solver. The source is
applied through the general conservative source ledger for every affected cell;
the probe does not special-case its expected final state inside the solver step.

```text
grid=32x24
boundary_mode=sealed
timestep=0.01
steps=40
maximum_cfl=0.0266667
minimum_density=1
minimum_pressure=1.00167
mean_pressure=1 -> 1.06667
mean_temperature=1 -> 1.06667
mass_drift=0
momentum_x_drift=0
momentum_y_drift=0
energy_drift=76.8
source_energy_net=76.8
source_energy_balance_error=3.21876e-11
source_event_count=30720
source_ledger_closes=true
fallback_count=0
numerical_correction_count=0
state_and_flux_scratch=162.333 bytes/cell / 124672 bytes total
```

The clean result is bound to `be0ff2f37`, `source_dirty=false` and verified
strict-double flags. Its local result SHA-256 is
`43382E81EC4F81683FC900DC9723EAD9BE2B6D454597E5AD543442AA87B57400`.
The periodic uniform and pulse runners were rerun on the same commit and also
passed; their local result SHA-256 values are
`3AD9EFCD61A25B2BE751C0FD5ACCD105CF9BB98335422B3284670D36B27DDC06`
and `3800A97790D8EBD250517AFF6EDA22EC48F50765114EF9568569757CF7CF0DA4`.

### Gravity source, sealed-wall ledger and natural convection

Checkpoint `83c5a0cd2` extends the generic sealed-grid step with a gravity source.
Momentum receives `rho*g*dt`; total energy receives the exact kinetic change of
that impulse, so the source step does not silently create internal energy. Sealed-
wall pressure impulses are recorded in a separate boundary ledger.

The experiment compares an isothermal hydrostatic control with the same state plus
a localized nondimensional bottom temperature perturbation (`1 -> 1.5`). The
differential diagnostic subtracts control temperature and velocity, preventing
the control's first-order hydrostatic imbalance from being mislabeled as buoyancy.

```text
grid=32x24
boundary_mode=sealed
timestep=0.01
steps=400
gravity_y=-0.04
hot_temperature_amplitude=0.5
thermal_center_y=3.67552 -> 3.90102
thermal_center_rise=0.225497
thermal_weighted_velocity_y=0.0153885
maximum_upward_velocity_difference=0.0257047
minimum_downward_velocity_difference=-0.000157248
maximum_absolute_velocity_difference=0.0257065
circulation_observed=true
control_combined_max_balance_error=7.87281e-12
heated_combined_max_balance_error=1.90289e-12
control_source_and_boundary_ledger_closes=true
heated_source_and_boundary_ledger_closes=true
control_fallback/correction=0/0
heated_fallback/correction=0/0
maximum_cfl_control/heated=0.0261907/0.0317075
state_and_flux_scratch=162.333 bytes/cell / 124672 bytes total
```

The source-only ledger reports `false` for both runs because wall pressure applies
real momentum exchange; source plus boundary reports `true`. This is intentional
accounting, not a hidden correction. The clean natural-convection result SHA-256
is `965ED55823BB2706A233A5DFBE4E16D4CC8964CC63DA1D3993A5DB14FA6F39C0`.
The sealed-heating regression on the same commit also passes, SHA-256
`7850D3A93DA081C289CBE687C9ABCE5E8AACC21382F7ED1178417CE7C33958CB`.

This is a nondimensional candidate signal. The control is not a proof of a
well-balanced hydrostatic method, and no physical time, real gravity scale,
material property or production TPT wall behavior is claimed.

### Passive conserved-species mixing

Checkpoint `55088a852` adds one passive species partial-density channel to the
periodic 2D candidate. Species flux is gas mass flux times the upwind mass
fraction; species B is derived as the complement of total gas density. No clamp,
floor or physical-diffusion term is used.

```text
grid=32x24
timestep=0.1
steps=320
maximum_cfl=0.288199
species_a_mass=384 -> 384
species_b_mass=384 -> 384
species_fraction_bounds=0..1
composition_total_variation=48 -> 47.9173
mixed_cells=0 -> 768
gas_mass_momentum_energy_drift=0 / 0 / 0 / 0
fallback_count=0
numerical_correction_count=0
state=40 bytes/cell
state_and_flux_scratch=200 bytes/cell / 153600 bytes total
```

The clean result is bound to `55088a852`, reports `source_dirty=false`, and has
SHA-256 `F5B7C1FBB60CD2E7672AA5A50FC46A8600C3D8B43E7BE074234BDADC9D8982A4`.
Species-EOS coupling and physical diffusion are explicitly `not_implemented`;
this is not a production multi-species atmosphere or solver selection. The
standalone report is [phase-5-species-mixing.md](phase-5-species-mixing.md).

## Validation

```text
full_build=75/75 build steps PASS
python_discovery=413 PASS, 2 skipped, 415 total
meson_static=64/64 PASS
targeted_atmospherebench_contracts=18/18 PASS
hllc_targeted_meson=6/6 PASS
hllc_2d_targeted_meson=4/4 PASS
hllc_fallback_contract=PASS, expected fallback count 1
latest_species_clean_runner=PASS
hllc_2d_clean_runner_at_be0ff2f37=3/3 PASS
hllc_unique_clean_modes=9/9 PASS
hllc_natural_plus_heating_clean_at_83c5a0cd2=2/2 PASS
production_source_files_changed=0
```

The clean runner verifies the current commit, freshly rebuilt Meson target,
source root, strict GNU floating-point flags, candidate identity, finite metrics,
ledger closure and zero correction/fallback counts.

Clean source package: `artifacts/vnext-phase5-source-20309c670/`, SHA-256
`043746D179C1D2A0691FCA3C4A9AA728263E9B9C901301890928F992E9173529`.
It has `1307` entries (`1306` source members plus source manifest), includes the
HLLC report and AtmosphereBench sources/runner, contains zero test assets, and
binds revision `20309c6703f210600a7f605e54790ab922ecc9c1`.

## Compatibility and memory

- Particle layout, Element IDs, Lua identifiers and save format are unchanged.
- Base gas state remains `32 bytes/cell`. The 1D probe uses `96 bytes/cell`; the 2D
  checkpoint's actual initial/current/next plus X/Y face-flux storage is
  `160 bytes/cell` for periodic faces and `162.333 bytes/cell` for the sealed
  face arrays (`124672` bytes total on `32x24`). The passive binary fixture adds
  one authoritative double and its matching working arrays: `40 bytes/cell`
  authoritative and `200 bytes/cell` with its current scratch layout.
- No GPU backend, upload, readback, VRAM or synchronization measurement exists.
- CPU Reference remains authoritative.

## Gate, risks and next work

HLLC with Rusanov fallback is the current front-runner for continued PoC work,
not the selected production solver. `V1_0_5_GATE=IN_PROGRESS` because the phase
still lacks a selected PhysicalScale/physical-time policy, accepted performance
budget and two-dimensional performance. Sealed heating, gravity accounting,
natural convection and passive species conservation now pass, but broader
multidimensional and
long-running adversarial coverage remains required before production use.

Rollback commit: `52e94c5aa` restores the pre-HLLC checkpoint.
