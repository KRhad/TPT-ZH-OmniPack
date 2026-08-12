# 1.0.9 mixtures, solutions and material reality

```text
TARGET_VERSION=1.0.9
BASE_COMMIT=25172166376d909fe3837c61d7d6c81be2c46d85
IMPLEMENTATION_HEAD=5ec87a63940f95a1ffa3636ca82fec45d0435575
BRANCH=integration/omnicore-vnext
GATE=GREEN_READY_FOR_MILESTONE_COMMIT
ROLLBACK_COMMIT=25172166376d909fe3837c61d7d6c81be2c46d85
CLASSIC_CHANGED=false
GPU_BACKEND=not_implemented
ONLINE_REFERENCE_REFRESH=YELLOW_EXTERNAL_503_AUTH_UNAVAILABLE
```

## Scope and architecture

1.0.9 closes four bounded material-reality foundations without changing the
Particle ABI or entering SDL3/GPU work.

- Enhanced/Scientific SALT and SLTW own strict-double solvent and NaCl masses.
  Dissolution, evaporation concentration and crystallisation are rate limited;
  NaCl mass closes through the solution ledger.
- ACID and BASE are the deliberately narrow v1 mappings HCl(aq) and NaOH(aq).
  `HCl + NaOH -> NaCl + H2O` is finite-rate, releases `57.9 kJ/mol`, retains an
  excess reactant, and stores generated neutral salt separately from primary
  acid/base solute. Classic retains the official ACID/BASE path.
- TIAL, NSAL, WALY, ZRAL and NITI expose fixed composition definitions derived
  from the existing gameplay recipe counts. They are labelled
  `game_recipe_fraction_not_industrial_grade_claim` and preserve Element IDs.
- Enhanced IRON now accumulates separate `corrosion_progress` and
  `passivation_fraction` sidecars. Rate ordering uses atmosphere O2 partial
  pressure, humidity/liquid water, temperature, exposed surface, NaCl
  concentration and zinc protection. Conversion to BMTL occurs only after the
  process reaches its threshold. Classic keeps the upstream random-contact
  corrosion implementation, including immediate LO2 behavior.

The solution state adds three doubles and corrosion adds two doubles per
particle capacity slot. Both participate in create/replace/type-change/delete,
OPS, Simulation save/load, Snapshot, SnapshotDelta and undo/redo. Lua exposes
`sim.omniSolution()`, `sim.omniAlloyComposition(type)` and read-only
`sim.omniCorrosion([particleId])` diagnostics.

## Data and provenance

- The NaCl saturation fit and USGS attribution are preserved in the versioned
  solution contract. Scientific/database dumps are not bundled.
- HCl/NaOH molar masses and neutralisation energy are attributed in the
  contract; the initial ten mass-percent parcel policy and per-tick rate are
  explicitly game-tuned.
- Online reference refresh was retried on 2026-08-12 but the web channel
  returned `503 auth_unavailable`. No unverified new empirical corrosion
  constant was added. Corrosion rate scaling is explicitly
  `game_tuned_explicit` and does not claim a measured real-world rate.
- 1.0.9 does not yet own iron or oxide parcel mass and does not consume
  atmospheric O2 stoichiometrically for rust. This is a disclosed process
  model, not a mass-conserving iron-oxidation chemistry claim.

## Correctness and compatibility evidence

```text
strict_incremental_build=PASS
legacy_fast_incremental_build=PASS
strict_clean_build=864/864_PASS_build-vnext-phase9-gate
strict_static_suite=95/95_PASS
python_discovery=438/438_PASS
clean_targeted_phase9=4/4_PASS
upstream_remote_stable_tag=v100.1.400
upstream_remote_master=d768aeb89acad986bd252d7e904bf44bb374545f
```

Solution/acid-base signature, identical in strict and legacy-fast builds:

```text
dissolved_mass_kg=3.33333e-08
evaporated_solvent_mass_kg=3.92e-09
concentration_before=0.264277
concentration_after=0.264468
crystallised_mass_kg=3.33333e-08
neutralisation_transactions=1
neutralised_acid_mass_kg=3.7037e-09
neutralised_base_mass_kg=4.06291e-09
neutral_salt_produced_kg=5.93662e-09
neutralisation_water_produced_kg=1.82999e-09
neutralisation_energy_released_j=0.00588148
total_solution_mass_residual_kg=8.47033e-22
unequal_excess_base_retained=true
ops_v1_migration=PASS
ops_v2_neutral_salt=PASS
snapshot_delta_undo_redo=PASS
classic_solution_active=false
```

Corrosion signature, identical in strict and legacy-fast builds:

```text
dry_progress_200_ticks=0
wet_progress_20_ticks=0.0743279
salt_progress_20_ticks=0.374289
hot_333K_progress_20_ticks=0.291065
wet_passivation=0.0148656
zinc_protected_progress_200_ticks=0
ops_roundtrip_progress=0.374289
completion_endpoint=BMTL_tmp20
classic_lo2_path=true
malformed_out_of_range_payload=REJECTED
snapshot_delta_undo_redo=PASS
```

Runtime and compatibility regression:

```text
lua_solution=PASS_total_residual_8.470329472543e-22kg
lua_corrosion=PASS_progress_0.0038478527565916
lua_chemistry=PASS_transactions_1
lua_upstream_100_1=PASS_lua_bounds_11
lua_ops_mixed=PASS_processes_3_restarts_2_particles_11
correction_ledger=PASS_events_4_overflow_accounted
lifecycle_ledger=PASS_ticks_8_reconciliation_failures_0
profiler=PASS
profiler_rendering_concurrency=PASS_ticks_3_calls_1_copy_calls_1
water_1000_step_static_probe=PASS_156.98s
```

## Benchmark and memory

Formal real-client fixed-step benchmark, `mixed-medium`, 30 warm-up steps, 120
measured steps per pass, five passes, strict FP:

```text
profiler_off_steps_per_second=170.112753
profiler_on_steps_per_second=165.114293
profiler_overhead_percent=2.938322
final_state_hash_off=4264546838
final_state_hash_on=4264546838
peak_working_set_off_bytes=194961408
peak_working_set_on_bytes=193941504
performance_gate=not_evaluated
```

Capacity memory added during 1.0.9:

```text
solution_sidecar_bytes_per_particle=24
corrosion_sidecar_bytes_per_particle=16
total_1_0_9_bytes_per_particle=40
solution_capacity_bytes_at_NPART_293760=7050240
corrosion_capacity_bytes_at_NPART_293760=4700160
total_capacity_bytes=11750400
ops_uncompressed_bytes_per_saved_particle=40
gpu_memory=not_applicable_no_gpu_backend
```

## Gate decision and deferred work

```text
BUILD=GREEN
TEST=GREEN
SOLUTION_MASS_LEDGER=GREEN
ACID_BASE_MASS_AND_ENERGY=GREEN
ALLOY_COMPOSITION_CONTRACT=GREEN
CORROSION_PROCESS=GREEN
CLASSIC_COMPATIBILITY=GREEN
SAVE_SNAPSHOT_LUA=GREEN
BENCHMARK=RECORDED_PERFORMANCE_THRESHOLD_NOT_EVALUATED
REFERENCE_REFRESH=YELLOW_EXTERNAL_503
IRON_OXIDE_STOICHIOMETRIC_MASS_COUPLING=DEFERRED_NOT_CLAIMED
GATE=GREEN_READY_FOR_MILESTONE_COMMIT
```

After the milestone commit and `dev-1.0.9` tag, the next allowed phase is the
independent 1.0.10 SDL2-to-SDL3 migration. Atmosphere equations, chemistry,
Particle layout, SDL_GPU and CUDA remain frozen during that migration.
