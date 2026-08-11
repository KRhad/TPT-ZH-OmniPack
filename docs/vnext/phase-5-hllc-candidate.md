# 1.0.5 strict-double HLLC with Rusanov fallback checkpoint

```text
TARGET_VERSION=1.0.5
BASE_COMMIT=52e94c5aa
IMPLEMENTATION_COMMIT=d38120177ff22984fd69539d6fc2ff37a8063fa2
STATUS=GREEN_ISOLATED_CANDIDATE_FRONT_RUNNER_NOT_SELECTED
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

## Validation

```text
full_build=75/75 PASS
python_discovery=414/414 PASS, 0 skipped
meson_static=61/61 PASS
targeted_contracts=29/29 PASS
hllc_targeted_meson=6/6 PASS
hllc_2d_targeted_meson=3/3 PASS
hllc_fallback_contract=PASS, expected fallback count 1
clean_runner=7/7 PASS
production_source_files_changed=0
```

The clean runner verifies the current commit, freshly rebuilt Meson target,
source root, strict GNU floating-point flags, candidate identity, finite metrics,
ledger closure and zero correction/fallback counts.

Clean source package: `artifacts/vnext-phase5-source-5ee23cd3d/`, SHA-256
`23309AEC8E0B5F1406BA68A87E0F522C05DD0EFD37C2CF0FF74B57854A31065F`.
It has `1305` entries (`1304` source members plus source manifest), includes the
HLLC report and AtmosphereBench sources/runner, and contains zero test assets.

## Compatibility and memory

- Particle layout, Element IDs, Lua identifiers and save format are unchanged.
- State remains `32 bytes/cell`. The 1D probe uses `96 bytes/cell`; the 2D
  checkpoint's actual initial/current/next plus X/Y face-flux storage is
  `160 bytes/cell`.
- No GPU backend, upload, readback, VRAM or synchronization measurement exists.
- CPU Reference remains authoritative.

## Gate, risks and next work

HLLC with Rusanov fallback is the current front-runner for continued PoC work,
not the selected production solver. `V1_0_5_GATE=IN_PROGRESS` because the phase
still lacks a selected PhysicalScale/physical-time policy, accepted performance
budget, the remaining multidimensional mandatory cases, sealed heating, natural
convection and gas mixing. The fallback contract is verified, but broader
multidimensional and
long-running adversarial coverage remains required before production use.

Rollback commit: `52e94c5aa` restores the pre-HLLC checkpoint.
