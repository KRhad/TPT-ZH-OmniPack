# 1.0.5 passive conserved-species mixing checkpoint

```text
TARGET_VERSION=1.0.5
BASE_COMMIT=39eec18b2c76ee287d2ffecb705fe66f638b94f6
IMPLEMENTATION_COMMIT=55088a8523421b8ef5c1c0b6be3170fdf505ac34
STATUS=GREEN_ISOLATED_PASSIVE_SPECIES_FIXTURE_NOT_SELECTED
ATMOSPHERE_SOLVER_SELECTION=UNSELECTED
PHYSICAL_SCALE_SELECTION=UNSELECTED
PHYSICAL_TIME_POLICY=UNSELECTED
HLLE=REGISTERED_ONLY
LBM=REGISTERED_ONLY
PRODUCTION_AIR_CHANGED=false
```

## Goal and architecture

This checkpoint adds one passive binary-species transport fixture to the
standalone strict-double AtmosphereBench. It extends the current HLLC candidate
with one authoritative species-A partial density per cell. Species B is derived
as total gas density minus species A, so the fixture does not maintain two
independent composition totals that can disagree with gas mass.

At each face, the species flux is the conservative gas mass flux multiplied by
the upwind species mass fraction. The same periodic finite-volume divergence is
then applied to gas and species state. The experiment is not keyed to a desired
result and contains no clamp, floor or correction path.

This is deliberately not a complete multi-species atmosphere. Species are
passive and do not affect the EOS. Physical molecular diffusion, trace-species
storage, reactions and production coupling are all `not_implemented`.

## Numerical verification

The nondimensional fixture starts with uniform gas moving at `(0.2, 0.1)` on a
`32x24` periodic grid. Species A occupies the left half and species B the right.
It advances `320` first-order steps at `dt=0.1`.

```text
maximum_cfl=0.288199
minimum_density=1
minimum_pressure=1
gas_mass_drift=0
gas_momentum_x_drift=0
gas_momentum_y_drift=0
gas_energy_drift=0
species_a_mass=384 -> 384
species_b_mass=384 -> 384
species_a_mass_drift=0
species_b_mass_drift=0
species_a_fraction_bounds=0..1
composition_total_variation=48 -> 47.9173
composition_state_change_l1=307.156
mixed_cells=0 -> 768
fallback_count=0
numerical_correction_count=0
```

The original 40-step draft was rejected as evidence because its reported total-
variation decrease could be explained by floating-point tail error. The checked
fixture now requires a decrease of at least `1e-6`; the 320-step run provides a
measurable `0.0827` decrease while preserving both species masses exactly at the
published precision.

## Build, tests and evidence

```text
full_build=75/75 PASS
meson_static=64/64 PASS
python_discovery=413 PASS, 2 existing skips, 415 total
targeted_contracts=18/18 PASS
atmospherebench_self_test=PASS
clean_runner=PASS
source_dirty=false
strict_reference_flags_verified=true
result_json_sha256=F5B7C1FBB60CD2E7672AA5A50FC46A8600C3D8B43E7BE074234BDADC9D8982A4
```

The local clean-runner result is under
`artifacts/vnext-atmospherebench-species-mixing/`. The runner rebuilds the unique
Meson target, verifies strict floating-point flags and current source paths,
requires a clean worktree, validates conservation/bounds/mixing fail-closed, and
records the explicit `physical_diffusion=not_implemented` limitation.

The clean test-free source package is
`artifacts/vnext-phase5-source-03a83b865/`, SHA-256
`299EA74E8D98727CA945AACC2B894794A666351EFF9614E5937B006A7C4CE9BE`.
It contains `1309` source members plus one manifest, includes both Species2D
sources and this report, contains zero test assets, and binds revision
`03a83b865072ba20efe0072419cc428dd73f9a7e`.

## Memory and performance scope

```text
authoritative_state=40 bytes/cell
initial_current_next_plus_xy_flux_scratch=200 bytes/cell
working_bytes_total_32x24=153600
performance_gate=not_evaluated_candidate_probe
```

The `40 bytes/cell` state consists of four strict-double gas conserved values plus
one strict-double species partial density. The `200 bytes/cell` working figure is
the actual initial/current/next and X/Y flux allocation for this fixture. No
throughput, production frame budget, RAM process peak, VRAM, upload, readback or
GPU synchronization claim is made here.

## Compatibility, risks and gate

- Production Air, Simulation, Particle, Save, Lua, renderer and SDL files changed:
  `0`.
- Element IDs, Particle layout, Lua identifiers and save format are unchanged.
- HLLE and LBM remain `registered_only`.
- CPU Reference remains available; no GPU backend exists for this fixture.
- The binary complement representation is only a benchmark fixture and does not
  select the future common-plus-trace storage design.

`V1_0_5_GATE=IN_PROGRESS`. Gas mixing/species-conservation evidence is now
present, but two-dimensional throughput and an accepted memory/frame budget,
PhysicalScale, physical-time mapping and final solver selection remain open.
Production OmniAtmosphere integration is still unauthorized.

Rollback commit: `39eec18b2` restores the pre-species checkpoint.
