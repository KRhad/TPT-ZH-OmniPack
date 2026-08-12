# 1.0.8 OmniChem and combustion foundation

```text
TARGET_VERSION=1.0.8
BASE_COMMIT=8fb55277af245024a8dd7401a42d470f823b0622
REACTION_SCAFFOLD_COMMIT=a57eb8605ff0a710d979c56e0453c76e72b25291
BRANCH=integration/omnicore-vnext
GATE=GREEN_READY_FOR_MILESTONE_COMMIT
ROLLBACK_COMMIT=8fb55277af245024a8dd7401a42d470f823b0622
CLASSIC_CHANGED=false
GPU_BACKEND=not_implemented
```

## Goal and bounded scope

1.0.8 adds the first reduced, validated OmniChem runtime and connects exactly
one reaction to gameplay:

```text
C(s) + O2(g) -> CO2(g)
```

The implementation is deliberately smaller than a general chemistry engine.
It establishes atom/charge/molar-mass validation, finite-rate kinetics, an
explicit reaction-energy transaction, a condensed-carbon ownership model and
diagnostics. Cantera is still an offline/reference candidate; no external
scientific library runs per cell or per frame.

## Architecture changes

- `OmniReactionRuntime` is a strict-double, backend-independent planner. It
  validates species, stoichiometry, atom count, charge, mass and energy before
  producing a transaction plan.
- The Arrhenius-like kinetic constants are marked `game_tuned`; they are not
  presented as measured graphite kinetics.
- Standard carbon-combustion enthalpy is `-393.5 kJ/mol`, attributed to OpenStax
  Chemistry under CC BY 4.0. No database dump is bundled.
- `OmniAtmosphere::ApplyReactionSpeciesTransfer` atomically checks and commits
  O2 consumption, CO2 production, momentum and chemical/sensible/kinetic energy.
- Enhanced/Scientific COAL and BCOL use the new path after the existing
  metallurgy hook; that hook retains priority for crucible charcoal behavior.
  Classic returns through the unchanged legacy COAL implementation.
- Carbon mass is a dedicated `double` sidecar. `Particle::tmp4` remains
  untouched because existing metallurgy and other systems already own its
  semantics.
- Carbon sidecar state participates in create/replace/type-change/delete,
  Snapshot, SnapshotDelta, undo/redo, Simulation save/load and OPS persistence.
- Reaction-only residuals exclude explicitly recorded external lifecycle sinks;
  separate total-balance residuals close after applying those sinks.
- OPS `omniCarbonParcels` v1 is aligned with saved particle order and rejects
  wrong versions/counts, negative or non-finite values, and non-COAL/BCOL
  ownership. Water OPS remains v2 and reads v1.
- Lua exposes `sim.omniChemistry()`; the profiler reports Chemistry as
  `instrumented_omni_reaction_runtime`.

## Numerical and compatibility verification

Both `build-vnext-fp-mode-strict` and `build-vnext-fp-mode-legacy` build the
full client and probes. The simulation static library is compiled with strict
FP arguments even when Common/GUI retain the legacy-fast policy.

```text
strict_full_build=PASS
legacy_fast_full_build=PASS
meson_static=90/90_PASS
python_discovery=436/436_PASS
reaction_contract=PASS
multispecies_thermal_contract=PASS
omnicore_data_validation=PASS_materials_1_species_3_reactions_1_legacy_mappings_488
```

The strict and legacy-fast combustion probes produced the same recorded
signature:

```text
normal_o2_consumed_kg=1.209e-08
normal_co2_produced_kg=1.6628e-08
low_o2_consumed_kg=1.04506e-09
high_o2_consumed_kg=4.70277e-08
chemical_energy_released_j=0.148675
mass_residual_kg=5.42101e-20
carbon_atom_residual_mol=5.42101e-20
oxygen_atom_residual_mol=3.46945e-18
energy_residual_j=0
classic_reaction_transactions=0
carbon_storage=dedicated_double_sidecar
ops_carbon_roundtrip=PASS
snapshot_delta_undo_redo=PASS
```

Additional runtime evidence:

```text
lua_omni_chemistry=PASS_transactions_1_profiler_calls_1
lua_profiler=PASS
lua_profiler_rendering_concurrency=PASS
lua_ops_roundtrip=PASS_processes_3_restarts_2
lua_upstream_100_1=PASS_lua_bounds_11
lua_correction_ledger=PASS
lua_lifecycle_ledger=PASS
water_1000_steps=PASS_drift_2.5411e-21_kg_max_energy_residual_0_J
```

The post-review isolated `build-vnext-phase8-gate` run additionally records:

```text
full_build=564/564_PASS
targeted_phase7_phase8_static=4/4_PASS
save_snapshot_probe=0.70s_PASS
water_1000_step_probe=153.74s_PASS_timeout_240s
combustion_probe=0.47s_PASS
water_true_total_energy_before_after=true
implicit_delete_or_type_change_evaporation=false
```

## Benchmark and memory

The formal fixed-step `mixed-medium` benchmark used 30 warm-up steps, 120
measured steps per pass and five passes. It is a dirty-worktree development
record bound to scaffold HEAD `a57eb8605`; performance gate remains
`not_evaluated` rather than being promoted to a release claim.

```text
profiler_off_steps_per_second=114.319792
profiler_on_steps_per_second=90.627650
state_hash_both=4264546838
observed_profiler_slowdown_percent=20.724445
```

Memory added by 1.0.8 is one authoritative `double` per particle slot:

```text
carbon_sidecar_bytes_per_particle=8
carbon_sidecar_capacity_bytes=2350080_at_NPART_293760
ops_carbon_bytes_per_saved_particle=8_before_compression
gpu_memory=not_applicable_no_gpu_backend
```

## Known deviations and risks

- Only carbon oxidation is implemented; low-O2 incomplete products, catalysts,
  reversibility, gas fuels and corrosion remain future scoped work.
- FIRE remains legacy gameplay. Enhanced COAL combustion currently uses the
  reacting hot particle as its visual representation.
- The profiler-enabled overhead is high on this host and is recorded, not
  hidden. The profiler remains disabled by default.
- The carbon sidecar is intentionally allocated at particle capacity for ABI
  safety; compact/hot-cold layouts remain deferred until the Particle access
  audit phase.
- No GPU backend exists, so GPU differential and VRAM are `not_tested_no_gpu_backend`.

## Gate decision

```text
BUILD=GREEN
TEST=GREEN
REACTION_VALIDATION=GREEN
ATOM_CHARGE_MASS_ENERGY=GREEN
CLASSIC_COMPATIBILITY=GREEN
SAVE_SNAPSHOT_LUA=GREEN
BENCHMARK=RECORDED
DOCUMENTATION=GREEN
GATE=GREEN_READY_FOR_MILESTONE_COMMIT
```

After the milestone commit and tag, the next allowed phase is 1.0.9 mixtures,
solutions and material reality. SDL3, SDL_GPU and CUDA remain out of scope.
