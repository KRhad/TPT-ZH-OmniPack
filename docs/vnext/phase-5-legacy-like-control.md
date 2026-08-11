# 1.0.5 strict-double Legacy-like control checkpoint

```text
TARGET_VERSION=1.0.5
BASE_COMMIT=1e61fb536
IMPLEMENTATION_COMMIT=696d0c9580f642db70bb419fc73cd9ea19b2afd0
STATUS=GREEN_CONTROL_ONLY_NOT_SELECTED
ATMOSPHERE_SOLVER_SELECTION=UNSELECTED
PHYSICAL_SCALE_SELECTION=UNSELECTED
PHYSICAL_TIME_POLICY=UNSELECTED
PRODUCTION_AIR_EQUIVALENCE=NOT_CLAIMED
PHYSICAL_MASS_STATE=NOT_IMPLEMENTED
PHYSICAL_ENERGY_STATE=NOT_IMPLEMENTED
HLLE=REGISTERED_ONLY
PRODUCTION_AIR_CHANGED=false
```

## Goal and scope

This checkpoint closes the missing executable Legacy-like comparison in the
isolated AtmosphereBench. It implements a strict-double, periodic,
dimensionless pressure/velocity stencil with separate current/next buffers and
two probes: uniform preservation and a two-dimensional pressure pulse.

This is deliberately a control model, not a copy of the production Legacy Air
implementation. It does not relabel `pv`, `vx`, `vy` or `hv` as physical mass,
density, momentum density or energy. No production Air, Simulation, Particle,
Save, Lua, renderer or SDL source is linked or modified.

## Results

Uniform preservation:

```text
grid=64x48
steps=32
state_change_l1=0
pressure_sum_drift=0
uniform_preserved=true
finite_state=true
numerical_correction_count=0
```

Clean result SHA-256:
`505F69D6D87724CC1B8E4410CA13BE91AF9464E22B12BBE338C601DB74CF04AD`.

Pressure pulse:

```text
grid=64x48
steps=96
pressure_peak=0.99005 -> 0.236479
minimum_pressure=-0.10854
pressure_sum_drift=8.52651e-13
maximum_absolute_velocity=0.321191
state_change_l1=463.151
pressure_peak_reduced=true
finite_state=true
numerical_correction_count=0
```

Clean result SHA-256:
`D1FE38268F52B15AB6BFA7D135B77122A79D48C1227A1E1F5909AEBB4AD0DFBA`.

The negative control pressure is retained as an observed Legacy-like field
value. It is not interpreted as negative absolute gas pressure.

## Explicit non-applicable physics

The text output records why the following metrics do not exist:

```text
mass_conservation=not_applicable_no_mass_state
momentum_conservation=not_applicable_no_momentum_density_state
energy_conservation=not_applicable_no_energy_state
maximum_cfl=not_applicable_legacy_dimensionless_stencil
```

The clean JSON records `mass_drift`, `momentum_drift`, both momentum components
and `energy_drift` as `null`. It does not convert those strings to a number and
does not publish fake zero conservation.

## Memory, validation and compatibility

```text
state=32 bytes/cell
current_plus_next=64 bytes/cell
full_build=75 build steps PASS
meson_static=69/69 PASS
python_discovery=416 PASS, 2 skipped, 418 total
targeted_atmospherebench_contracts=21/21 PASS
direct_probes=2/2 PASS
atmospherebench_self_test=PASS
clean_runners=2/2 PASS
source_dirty=false
strict_reference_flags_verified=true
```

- Element IDs, Particle layout, Lua identifiers and save format are unchanged.
- No physical or performance budget is inferred from this control.
- No GPU backend exists here; VRAM remains `not_tested_no_gpu_backend`.
- CPU Reference candidates remain isolated and available.

`V1_0_5_GATE=IN_PROGRESS`. Legacy-like, LBM and conservative FVM now all have
actual executable comparison evidence. Accepted CPU/memory budgets, PhysicalScale,
physical-time/acoustic policy, remaining mandatory cases, precision comparison,
G0 ledger semantics and final solver selection remain open.

Rollback commit: `1e61fb536` removes this Legacy-like control checkpoint.
