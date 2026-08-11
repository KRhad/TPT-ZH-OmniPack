# 1.0.5 strict-double D2Q9 LBM comparison checkpoint

```text
TARGET_VERSION=1.0.5
BASE_COMMIT=088190d3b
IMPLEMENTATION_COMMIT=5e3c46fae8fc7754240c97931e59f7b2216c5417
STATUS=GREEN_ISOTHERMAL_COMPARISON_LIMITED_NOT_SELECTED
ATMOSPHERE_SOLVER_SELECTION=UNSELECTED
PHYSICAL_SCALE_SELECTION=UNSELECTED
ENERGY_STATE=NOT_IMPLEMENTED
NEAR_VACUUM=UNSUPPORTED
SHOCKS=UNSUPPORTED
HLLE=REGISTERED_ONLY
PRODUCTION_AIR_CHANGED=false
```

## Goal and algorithm

This checkpoint turns the D2Q9 LBM entry from `registered_only` into an actual
strict-double comparison candidate. It implements periodic BGK collide-and-stream
with nine populations, standard D2Q9 weights, lattice sound speed squared `1/3`,
relaxation time `tau=0.8` and kinematic viscosity `0.1` lattice units.

No third-party code or data was copied. The implementation is isolated to
`tools/atmospherebench` and does not link production simulation code.

## Uniform preservation

```text
grid=64x48
steps=64
velocity=(0.02,-0.01)
maximum_mach=0.0387298
minimum_density=1
minimum_population=0.0253694
mass_drift=0
momentum_x_drift=1.77636e-13
momentum_y_drift=-3.55271e-15
population_state_change_l1=4.26326e-14
mass_conserved=true
momentum_conserved=true
uniform_preserved=true
numerical_correction_count=0
```

Clean result SHA-256:
`D601D2DDA76BECB73C37CE5CB309F6460A10B2516D50C135C8D7B39C26FA4333`.

## Low-Mach shear-wave decay

The `64x64` periodic case starts with transverse velocity
`u_x=0.02 sin(2*pi*y/64)` and advances 128 lattice steps. The analytical
low-Mach viscous amplitude is computed from the configured lattice viscosity;
the solver is not told the expected final population state.

```text
maximum_mach=0.0345993
minimum_density=1
minimum_population=0.0261457
mass_drift=-1.36424e-12
momentum_x_drift=-1.2379e-14
momentum_y_drift=-1.90292e-13
shear_amplitude=0.02 -> 0.0176685
expected_shear_amplitude=0.0176787
relative_error=0.000577441
mass_conserved=true
momentum_conserved=true
shear_reference_passed=true
numerical_correction_count=0
```

Clean result SHA-256:
`26180CBA90702B0458D30FA05C1867CEEAFA743D7CCF50BE7F393C29E7C12AE6`.

The clean test-free source package is
`artifacts/vnext-phase5-source-c8ae4ea8a/`, SHA-256
`3F1DB05D65D61701B726DE12BD959E43739291F3026D7BAC251E8DDF7B025876`.
It contains `1313` source members plus one manifest, includes both D2Q9 sources
and this report, contains zero test assets, and binds revision
`c8ae4ea8a7989965972825b26382b4ed485f5030`.

## Unsupported mandatory physics

The runner and JSON record these as explicit strings, not numeric zero:

```text
energy_state=not_implemented
energy_conservation=not_applicable_no_energy_state
energy_drift=null
near_vacuum_support=unsupported_low_mach_positive_population_contract
shock_support=unsupported_isothermal_low_mach_model
species_support=not_implemented
maximum_cfl=not_applicable_lattice_streaming
```

Therefore this D2Q9 BGK implementation is useful evidence for simple low-Mach
isothermal flow, but it cannot satisfy OmniAtmosphere requirements for conserved
total energy, near vacuum, pressure waves/shocks, large density variation or
reacting multi-species gas. It is not the selected unified solver.

## Memory, validation and compatibility

```text
authoritative_populations=72 bytes/cell
current_plus_next=144 bytes/cell
full_build=75/75 build steps PASS
meson_static=67/67 PASS
python_discovery=415 PASS, 2 skipped, 417 total
targeted_atmospherebench_contracts=20/20 PASS
clean_runners=2/2 PASS
source_dirty=false
strict_reference_flags_verified=true
```

- Production Air, Simulation, Particle, Save, Lua, renderer and SDL files changed:
  `0`.
- Element IDs, Particle layout, Lua identifiers and save format are unchanged.
- No GPU backend, VRAM, upload, readback or synchronization measurement exists.
- CPU Reference remains available.

`V1_0_5_GATE=IN_PROGRESS`. LBM now has actual comparison results and an explicit
model-based rejection for mandatory physics outside its domain. Legacy-like
execution, accepted CPU/memory and physical-time policy, the remaining mandatory
matrix and final solver selection remain open.

Rollback commit: `088190d3b` restores D2Q9 to its prior registration-only state.
