# Phase 5 isolated low-Mach pressure-projection component

## Goal

Evaluate whether a strict-double periodic pressure projection can supply the
missing bulk low-Mach pressure coupling evidence without changing production
Air, Simulation, Particle, Save, Lua or SDL code. This is a component probe only;
it does not select an Atmosphere solver or authorize 1.0.6.

## Implementation

Base commit: `27f7847a89e30639a5e209237901588dac4443a8`

Implementation commit: `c5641fe1cb4fb1f1ba25f47c6a36fe75700c2984`

Runner portability follow-ups: `7b1ae2482`, `f337a079f`

The probe is a 64x48 periodic, nondimensional velocity projection. It computes a
Jacobi Poisson correction for a backward-difference velocity divergence and
applies the forward pressure gradient. The 4000-iteration strict-double solve is
deliberately separate from the compressible HLLC event route.

Files changed:

- `tools/atmospherebench/LowMachProjection2D.{h,cpp}`
- `tools/atmospherebench/AtmosphereBench.cpp`
- `tools/atmospherebench/main.cpp`
- `meson.build`
- `tools/run_atmospherebench.ps1`
- `tools/package_source_release.py`
- targeted AtmosphereBench contract tests

No production simulation source was changed.

## Probe result

```text
grid=64x48
iterations=4000
time_step=0.1
initial_divergence_l2=0.00722534
final_divergence_l2=3.60466e-07
divergence_reduction_ratio=4.98891e-05
elapsed_milliseconds=11.7071
working_bytes_per_cell=40
finite_state=true
divergence_reduced=true
projection_probe_passed=true
```

The initial 800-iteration run was correctly rejected (`divergence_reduction_ratio`
`0.113605`). The threshold was met only after increasing the iteration count;
this is retained as a real numerical adjustment, not hidden test logic.

## Explicit boundaries

```text
candidate_solver_implemented=false
general_low_mach_pressure_coupling=implemented_periodic_projection_component_only
compressible_event_coupling=not_implemented_in_this_component
near_vacuum_support=not_applicable_bulk_component_routes_vacuum_to_compressible
production_runtime_integration=not_implemented
physical_scale_selection=unselected
physical_time_policy=unselected
atmosphere_solver_selection=unselected
```

The projection acts only on velocity and pressure-correction fields. It does not
carry density, total energy, species, EOS, boundary exchange or a conservation
ledger for a physical gas state. Its 11.7071 ms component measurement also exceeds
the current 4.16667 ms atmosphere budget before any coupling or event work.

## Validation

- targeted Python contract tests: `25/25 PASS`;
- full Python discovery at the implementation checkpoint: `432 OK`, `2 skipped`;
- full Meson static suite: `77/77 PASS`;
- full build: `618/618 PASS`;
- direct `atmospherebench --self-test`: `PASS`;
- Meson low-Mach test: `1/1 PASS`;
- PowerShell parse: `PASS`;
- clean provenance runner: `PASS`;
- `git diff --check`: `PASS`.

Clean runner artifact:

```text
artifacts/vnext-atmospherebench-low-mach-projection-final/20260811T080256Z-5a1739b8/result.json
sha256=C1C15D54456D5D5FFCB37551DD27AF321F925AFE68FDF3864699C89663544887
source_commit=f337a079f9801c1cf2c0c140ef7771235a909d99
source_dirty=false
```

Clean test-free source package:

```text
artifacts/vnext-phase5-source-f337a079/TPT-ZH-OmniPack-1.0.0-Source.zip
sha256=BFEC6F8D81A5044D95065CA140BEBA1C2315CE671127235AFE16B55000679889
source_members=1331
test_or_artifact_members=0
```

## Gate decision

```text
LOW_MACH_PROJECTION_COMPONENT=GREEN
PHASE_5=IN_PROGRESS
PHYSICAL_SCALE_SELECTION=UNSELECTED
PHYSICAL_TIME_POLICY=UNSELECTED
ATMOSPHERE_SOLVER_SELECTION=UNSELECTED
V1_0_6_RELEASE_ALLOWED=false
```

Rollback point: `27f7847a89e30639a5e209237901588dac4443a8` (before this isolated
component). The next permitted work is comparative 2D coupled/domain-of-
dependence and budget evidence; production OmniAtmosphere remains forbidden.
