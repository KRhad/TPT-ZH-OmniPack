# 1.0.7 Multi-Species Atmosphere + Thermal

## Gate

```text
TARGET_VERSION=1.0.7
BASE_COMMIT=3527beb7c4c153f44bede1779a9b311705a778d1
STATUS=GREEN_POST_MILESTONE_CORRECTION_VALIDATED_WITH_1_0_8
CLASSIC_DEFAULT=true
PARTICLE_ABI_CHANGED=false
ATMOSPHERE_STATE_VERSION=2
WATER_PARCEL_STATE_VERSION=2
CHEMISTRY_DEFERRED_TO=1.0.8
SDL3_GPU_CUDA_DEFERRED=true
```

## Authoritative atmosphere state

Enhanced and Scientific now use five common gas channels:

```text
rho_N2
rho_O2
rho_Ar
rho_CO2
rho_H2O
rho_u
rho_v
rho_E
```

Density is the sum of species partial densities. Mixture gas constant, Cp, Cv,
gamma, pressure, temperature, velocity, partial pressure and relative humidity
are derived. The runtime does not store an independent humidity field.

Species use conservative advection and a bounded zero-sum mixture diffusion
step. The two-dimensional CFL bound includes both directional acoustic terms,
and diffusion face transfers are limited by donor availability. A compatibility
shadow prevents the derived Legacy `pv/vx/vy/hv` projection from being imported
back as a fresh source when no Legacy writer changed it.

## OmniThermal and water ownership

`OmniThermal.cpp` is part of the same dedicated strict-double object as
`OmniAtmosphere.cpp`. It contains:

- IAPWS-IF97 Region 4 saturation pressure above the triple point;
- Murphy-Koop saturation pressure over ice;
- a documented 1 K apparent-enthalpy fusion interval;
- water/ice/vapour sensible and latent energy relations;
- pressure-dependent evaporation/boiling requests;
- strict-double water transfer, remaining-state and energy-residual helpers.

Particle ABI remains unchanged. Enhanced owns separate strict-double water mass
and specific-enthalpy sidecars (`16 bytes/particle`) only for the pure-water
family: `WATR`, `ICEI` and `WTRV`.

- `WATR` and `ICEI` own condensed pure-water parcel mass;
- `WTRV` is a rate-limited injection/visual parcel;
- atmosphere H2O owns gas mass;
- atmosphere condensed-water density owns unresolved cell-scale condensate.

The default full parcel is `4e-6 kg`. A single transfer is capped at the
reference gas mass of one atmosphere cell (`7.84e-8 kg` on the selected scale),
preventing one particle from injecting roughly fifty cell masses in one tick.
Legacy ambient-heat and threshold phase paths are bypassed only for that narrow
pure-water family in Enhanced/Scientific, preventing double energy application.
`DSTW`, `SLTW`, `CBNW`, `SNOW`, `FOG` and `RIME` intentionally retain upstream
behavior: their solution/aerosol composition cannot be represented as pure water
without silently changing gameplay. Their ownership migration is deferred to the
1.0.9 mixture/solution scope. Classic continues through the upstream path.

Visible fog droplet nucleation remains deferred: cell condensate is conserved
and serialized, but is not yet converted into `FOG` particles.

## Persistence, compatibility and undo

OPS `omniAtmosphere/stateVersion=2` stores cell-major little-endian f64 values:

```text
five species partial densities
momentum X/Y
total energy
condensed water density
cell validity mask
```

OPS `omniWaterParcels/stateVersion=2` stores particle-order f64 parcel masses and
specific enthalpies; version 1 mass-only saves remain readable and derive their
initial enthalpy from the saved temperature.
Classic saves omit both payloads. `includePressure=false` omits regional
atmosphere state. Unknown species, malformed sizes, negative/non-finite state
and corrupt OPS are rejected. Region transforms rotate momentum and keep water
sidecars aligned with particle order. Payload-free 1.0.6 Enhanced saves report
`migrated_1_0_6_legacy_projection` rather than claiming exact continuation.

Snapshot and SnapshotDelta now include authoritative atmosphere, species,
energy, condensed water and particle water mass/enthalpy. Undo/redo therefore restores
the real Enhanced state instead of only its Legacy display projection. Classic
snapshot hashes intentionally exclude hidden Omni state and retain the previous
hash contract.

## Memory

```text
authoritative_gas_bytes_per_cell=64
derived_density_cache_bytes_per_cell=8
condensed_water_sidecar_bytes_per_cell=8
working_state_bytes_per_cell=161
water_sidecar_bytes_per_particle=16
```

The fixed common-channel design is versioned but not treated as the final trace
species architecture. Sparse/per-world trace storage remains a later benchmark.

## Post-milestone correction evidence

The historical `dev-1.0.7` tag remains unchanged. The 1.0.8 integration branch
adds OPS v2 enthalpy state, removes implicit kill/type-change evaporation, and
records non-water conversions/deletion/wall/off-screen removal as external
sources or sinks. The corrected real-client probes on the current tree pass:

```text
new_clean_build=564/564_PASS
targeted_static=4/4_PASS
save_snapshot_roundtrip=0.70s_PASS
water_1000_step_probe=153.74s_PASS_timeout_240s
combustion_probe=0.47s_PASS
water_lifecycle_type_change_replacement_delete=true
true_coupled_energy_before_after_residual=PASS
```

The full 1.0.8 suite remains the authoritative final gate; the figures below
are retained only as historical 1.0.7 evidence.

## Historical numerical evidence

```text
thermal_probe=PASS
multispecies_probe=PASS
atmosphere_probe=PASS
save_snapshot_probe=PASS
water_1000_step_probe=PASS
long_run_water_drift_kg=-9.34332e-16
long_run_max_energy_residual_j=0
water_mass_residual_kg=0
coupled_energy_residual_j=0
strict_water_transfer_mass_kg=7.84e-8
partial_pressure_sum_matches_mixture=true
snapshot_undo_roundtrip=true
snapshot_delta_roundtrip=true
classic_snapshot_hash_compatible=true
```

## Historical validation

```text
legacy_fast_full_build=PASS
strict_fp_full_build=683/683 PASS
meson_static=87/87 PASS
python_discovery=436 total / 434 pass / 2 skipped
lua_omni_atmosphere=PASS
lua_ops_roundtrip=PASS
lua_upstream_100_1=PASS
lua_profiler=PASS
fixed_step_empty=1609.516712 steps/s
fixed_step_mixed_medium=177.269204 steps/s
```

Five isolated real-client Enhanced quiet measurements were:

```text
3.7992671132088
4.0004029870033
4.0798261761665
4.0798261761665
3.8713514804840
median=4.0004029870033 ms/tick
reference_atmosphere_budget=4.1666667 ms/tick
```

The empty-world AfterSim path no longer repeats a second full-grid Legacy
projection reconciliation when there are no particle writers or queued water
transfers. Non-empty worlds keep the conservative reconciliation path.

## Known limits and rollback

- Scientific mode still shares the Enhanced equations and is not claimed as a
  higher-accuracy product mode.
- Visible droplet nucleation, trace species, multicomponent diffusion and full
  material-dependent transport remain deferred.
- User deletion and direct Legacy/Lua field writes are explicit source/sink
  operations, not conservation failures.
- Benchmark artifacts were recorded from the pre-milestone dirty tree and are
  evidence only; the clean milestone benchmark is recorded after commit.

Rollback is the base commit shown above. The milestone commit and `dev-1.0.7`
tag are created only after this report, build, tests and diff review are clean.
