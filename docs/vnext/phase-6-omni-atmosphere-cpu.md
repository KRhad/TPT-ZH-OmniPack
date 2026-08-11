# 1.0.6 CPU OmniAtmosphere MVP

## Goal

Add the first production-owned CPU Reference atmosphere state without replacing
Classic Air or changing Element IDs. Enhanced is explicit opt-in; every old save
without Omni metadata remains Classic.

## Base and scope

```text
VERSION=1.0.6
BASE_COMMIT=193dc2ca812da4aafff9e7261fd52af7aef0d0b1
ROLLBACK_COMMIT=193dc2ca812da4aafff9e7261fd52af7aef0d0b1
CLASSIC_DEFAULT=true
MULTI_SPECIES=false_deferred_to_1.0.7
CHEMISTRY=false_deferred_to_1.0.8
SDL3_GPU_CUDA=false_deferred
```

The old Phase 5 candidate JSON files remain historical benchmark inputs. The
selected machine-readable contract is
`resources/omnicore/v1/omni-atmosphere-runtime-v1.json`; runtime constants are
centralized in `OmniPhysicalScale.h`, and a fail-closed static check binds them.
Production code does not reinterpret Legacy `pv/vx/vy/hv` as SI-authoritative state.

## Architecture

`OmniAtmosphere` owns four strict-double reference quantities per cell:

```text
rho
rho*u
rho*v
rho*E
```

Velocity, pressure, temperature and sound speed are derived through the
single-species ideal-gas EOS. The selected geometry is 1 mm per particle pixel,
4 mm atmosphere cells, 4 mm effective depth and fixed 1/60 s game ticks.
The production object is compiled once in the dedicated
`omni-atmosphere-reference` static library with `-fno-fast-math`,
`-fno-unsafe-math-optimizations` and `-ffp-contract=off`; both the client and
the standalone probe link that same object.

The numerical MVP contains:

- first-order conservative Rusanov reference flux;
- explicit periodic, open/ambient and reflecting-wall boundaries;
- adaptive reference CFL substeps;
- a bounded quiet/runtime low-Mach route;
- persistent acoustic routing while pressure, density or velocity gradients
  remain unresolved;
- density, pressure and internal-energy positivity floors;
- source, boundary and numerical-correction ledgers;
- a versioned Legacy display/script projection (`1 pv = 1000 Pa` deviation from
  the configured reference pressure), which is derived and non-authoritative.

The configured Open-boundary reservoir follows the active Legacy ambient-air
temperature. A quiet default Open world therefore remains on the one-substep
bulk path instead of seeing a false temperature/pressure discontinuity.

The production path is not yet the complete Phase 5 GMG + HLLC pipeline. The
HLLC/GMG evidence remains the target architecture for later optimization and
event routing; 1.0.6 deliberately establishes the independently verifiable CPU
reference floor first.

## Integration and compatibility

Simulation modes are:

```text
OMNI_CLASSIC=0
OMNI_ENHANCED=1
OMNI_SCIENTIFIC=2
```

Classic continues to call the unchanged Legacy `Air::update_air` and optional
`Air::update_airh`. Enhanced/Scientific call `OmniAtmosphere::Step` and export a
derived Legacy view for existing rendering and script consumers. No mode is
enabled automatically.

OPS receives optional `omniSimulationMode` metadata. Missing or invalid metadata
fails safe to Classic. The authoritative atmosphere arrays are deliberately not
serialized in 1.0.6; Lua reports `state_serialized=false`, so loading an Enhanced
save reconstructs the selected preset rather than falsely claiming bit-exact
atmosphere continuation.

Lua adds:

```lua
sim.omniSimulationMode([mode])
sim.omniAtmosphere()
```

Existing `sim.airMode`, pressure, velocity and ambient-temperature APIs remain.

## Verification evidence

Current implementation evidence:

```text
CPU_REFERENCE_PROBE=PASS
AUTHORITATIVE_FIELDS=rho,rho_u,rho_v,rho_E
UNIFORM_MASS_RESIDUAL_KG=0
UNIFORM_ENERGY_RESIDUAL_J=0
SEALED_HEATING_PRESSURE_INCREASE=PASS
PRESSURE_PULSE=PASS
NEAR_VACUUM_MIN_DENSITY=1e-8_kg_m3
NEAR_VACUUM_MIN_PRESSURE=0.001_Pa
NEAR_VACUUM_PRESSURE_FLOOR_HITS=251
NEAR_VACUUM_NUMERICAL_ENERGY_CORRECTION_J=6.36588e-9
NEAR_VACUUM_ENERGY_RESIDUAL_J=-6.61744e-23
OPEN_LEAK_MASS_BEFORE=3.6864e-5_kg
OPEN_LEAK_MASS_AFTER=3.30579e-5_kg
REFLECTING_WALL_MOMENTUM_X_RESIDUAL=1.05879e-21
REFLECTING_WALL_MOMENTUM_Y_RESIDUAL=-1.183e-20
QUIET_REFERENCE_GRID=153x96
QUIET_RUNTIME_SUBSTEPS=1
QUIET_STRICT_DOUBLE_STANDALONE_MS_PER_TICK=1.4966_representative
AUTHORITATIVE_BYTES_PER_CELL=32
PERSISTENT_STATE_BYTES_PER_CELL=65_including_ping_pong_and_block_flag
LIMITED_EVENT_FIRST_ADVANCED_TIMESTEP_S=5.31762e-7
LIMITED_EVENT_SECOND_ADVANCED_TIMESTEP_S=1.17045e-6
LIMITED_EVENT_SECOND_ACOUSTIC_ROUTE=true
BLOCKED_EDGE_MOMENTUM_X_RESIDUAL=-8.47033e-22
CAVITY_SECOND_ACOUSTIC_ROUTE=true
```

Automated checks recorded before final milestone:

```text
FULL_ISOLATED_BUILD=814/814_PASS
MESON_STATIC=82/82_PASS
PYTHON_DISCOVERY=435_total_433_pass_2_skipped
MESON_STATIC_FINAL=82/82_PASS
PRODUCTION_STRICT_OBJECT=PASS_one_object_no_positive_fast_math_flags
LUA_OMNIATMOSPHERE_RUNTIME=PASS_14688_cells_mass_and_energy_residual_0_event_persists_sealed_boundary_and_cavity_ledgers
LUA_LEGACY_PROFILER=PASS
LUA_PROFILER_RENDERING_CONCURRENCY=PASS
LUA_UPSTREAM_100_1=PASS
OPS_ROUNDTRIP=PASS_8_scenarios_24_processes_16_restarts
SAVE_MODE_ROUNDTRIP=PASS_in_high_id_save_probe
CLASSIC_FIXED_STEP_MIXED_MEDIUM=174.254352_steps_per_second_5.738737590_ms_per_step
CLASSIC_FIXED_STEP_REPLAY=true_600_measured_steps
ENHANCED_REAL_CLIENT_QUIET=2.1821111440659_ms_per_tick_representative
FINAL_VALIDATED_EXE_SHA256=20A58B4788DBCE5B75E39361A6BD7253C7F9BD471D1F4820B0B40CE9D74D550B
```

The Classic fixed-step performance result was recorded from the dirty
pre-milestone worktree and therefore reports `performance_gate=not_evaluated`;
it is evidence, not a fabricated regression threshold. The Enhanced timing is
the full real-client adapter path, including Legacy import/export. There is no
GPU backend in 1.0.6, so GPU and per-process VRAM remain explicitly untested.

## Independent blocker review closure

The first independent numerical review held the gate RED. Its two blockers and
associated high-risk findings are now closed by direct evidence:

- pressure events remain on the acoustic route on later frames;
- runtime CFL overload exposes requested versus actually advanced physical time;
- production and probe share the same strict-double object;
- Open ambient reference temperature is synchronized;
- density, momentum and energy floor corrections are ledgered;
- reflecting external/internal wall impulses enter the momentum boundary ledger;
- boundary-mode changes and dynamic wall insertion explicitly trigger the
  acoustic route before reflecting fluxes are evaluated;
- wall-normal velocity next to blocked cells and sealed outer boundaries keeps
  an unresolved reflecting cavity on the acoustic route;
- frozen outer Legacy wall cells do not contribute a duplicate external impulse
  to the boundary ledger;
- the near-vacuum fixture reports its real nonzero correction instead of a fake zero;
- constructor and reference-state inputs are fail-closed.

## Known limits

- single species only;
- no atmosphere state payload in OPS yet;
- no particle/gas mass or energy ownership transfer yet;
- no humidity, evaporation, condensation or latent heat;
- no production multi-species EOS/diffusion;
- no runtime HLLC/GMG end-to-end route yet;
- blocked cells retain a frozen authoritative state and remain included in global
  mass/energy totals; 1.0.6 does not yet expose a separate active-fluid subtotal;
- when a runtime compressible event exceeds the bounded substep budget,
  physical time advances only by the stable interval and exports both
  `requested_timestep_s` and `advanced_timestep_s`; the overload is never hidden;
- no GPU backend, VRAM or CPU/GPU differential result.

These are explicit later-version scopes, not filled with fake zeroes.

## Gate

```text
V1_0_6_GATE=GREEN
IMPLEMENTATION_HEAD=ed1695af7abf43c206c8a4079fc73ad36517677c
SOURCE_PACKAGE=GREEN_ZIP_ENTRIES_1344_TEST_OR_PRIVATE_MATCHES_0
SOURCE_PACKAGE_SHA256=393163DE16035548EE824BA9009322EC172343DC5BC34EE1307D08B790D0F6E3
ROLLBACK=193dc2ca812da4aafff9e7261fd52af7aef0d0b1
NEXT_VERSION=1.0.7
```
