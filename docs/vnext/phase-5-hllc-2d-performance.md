# 1.0.5 HLLC two-dimensional performance checkpoint

```text
TARGET_VERSION=1.0.5
BASE_COMMIT=a70a805b3
IMPLEMENTATION_COMMIT=12e904f7574b690c600f1bbcb0b74d160badf540
STATUS=GREEN_MEASUREMENT_NO_BUDGET_NO_SELECTION
ATMOSPHERE_SOLVER_SELECTION=UNSELECTED
PHYSICAL_SCALE_SELECTION=UNSELECTED
PHYSICAL_TIME_POLICY=UNSELECTED
PERFORMANCE_BUDGET=UNSELECTED
PRODUCTION_AIR_CHANGED=false
```

## Goal and measurement contract

This checkpoint measures the current strict-double, single-threaded, periodic
HLLC/Rusanov-fallback 2D candidate at three real project grid sizes:

- `153x96`: current Legacy Air grid;
- `306x192`: two times the Air resolution in each dimension;
- `612x384`: current particle-grid resolution.

Each sample performs one warm-up followed by three measured repetitions of 32
steps. The published value is the median. Timing is end-to-end for the standalone
case: initial-state allocation/generation, flux and state allocation, 32 solver
steps, positivity/conservation diagnostics and final validation are included.

The smooth periodic state uses constant pressure, velocity `(0.2, 0.1)` and a
two-dimensional density perturbation. The timestep is chosen for nondimensional
CFL `0.2`. No physical-time conversion or gameplay substep count is inferred.

## Clean results

```text
grid                         ms/step   cell-updates/s   fraction of 16.667 ms
153x96  (14,688 cells)       1.69129   8.68448M         0.101478
306x192 (58,752 cells)       6.56627   8.94755M         0.393976
612x384 (235,008 cells)     31.2385    7.52304M         1.87431

maximum_cfl=0.2
minimum_density=0.950002
minimum_pressure=1
mass_drift=-2.91038e-10
momentum_x_drift=3.63798e-10
momentum_y_drift=1.81899e-10
energy_drift=5.64614e-9
fallback_count=0 / 0 / 0
numerical_correction_count=0
```

The 16.667 ms value is only a 60 Hz reference denominator. It is not an accepted
Atmosphere budget. The current Legacy-size coarse grid has measurable headroom in
this isolated single-substep case; a full particle-resolution strict-double CPU
step already exceeds the entire reference frame before particles, rendering,
Lua, sources, boundaries, species or additional acoustic substeps are counted.

## Memory

```text
grid            authoritative gas state   current working allocation
153x96          470,016 bytes              2,350,080 bytes
306x192       1,880,064 bytes              9,400,320 bytes
612x384       7,520,256 bytes             37,601,280 bytes

state=32 bytes/cell
initial/current/next/flux-x/flux-y=160 bytes/cell
```

These are container payload sizes reported by the candidate, not process peak
RAM. The passive-species fixture previously measured `40 bytes/cell` authoritative
and `200 bytes/cell` working, but its target-size throughput is not measured by
this checkpoint.

## Validation and provenance

```text
full_build=75/75 build steps PASS
meson_static=65/65 PASS
python_discovery=414 PASS, 2 skipped, 416 total
targeted_atmospherebench_contracts=19/19 PASS
clean_runner=PASS
source_dirty=false
strict_reference_flags_verified=true
performance_gate=recorded_candidate_measurement_no_budget
result_json_sha256=2792A219A32A3F7C81EC93ABF9301B5D3D2EE48A6BC74D27A40CBB8DE7D1BF87
```

The local clean result is under
`artifacts/vnext-atmospherebench-hllc-2d-performance/` and binds the full commit
`12e904f7574b690c600f1bbcb0b74d160badf540`.

The clean test-free source package is
`artifacts/vnext-phase5-source-ef423c4f5/`, SHA-256
`FDF8404AAFF38E23C1DBE5BCAF96EB750825CB95B899FE3B1BCE8C2DF2A267A5`.
It contains `1310` source members plus one manifest, contains zero test assets,
includes this report and all AtmosphereBench sources, and binds revision
`ef423c4f5a44a428e5f015855a087ace63c7ce96`.

## Compatibility, limitations and gate

- No production Air, Simulation, Particle, Save, Lua, renderer or SDL file changed.
- Element IDs, Particle layout, Lua identifiers and save format are unchanged.
- This result is strict-double, single-threaded and periodic; it does not measure
  sealed/open boundaries, sources, species, physical subcycling or GPU work.
- GPU/VRAM/upload/readback remain `not_tested_no_gpu_backend`.
- HLLE and LBM remain `registered_only` at this checkpoint.

`V1_0_5_GATE=IN_PROGRESS`. Two-dimensional throughput and memory are now recorded,
but no CPU atmosphere budget, PhysicalScale, physical-time policy or solver is
selected. Legacy-like/LBM comparative execution and the remaining mandatory
solver matrix also remain open.

Rollback commit: `a70a805b3` restores the pre-performance checkpoint.
