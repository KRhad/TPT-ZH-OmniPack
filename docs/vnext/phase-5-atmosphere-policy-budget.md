# 1.0.5 Atmosphere CPU budget and physical-time policy checkpoint

```text
TARGET_VERSION=1.0.5
BASE_COMMIT=a3c5ec9b7
IMPLEMENTATION_COMMIT=76300cd98ca2c7405d011ba4401445e3bd9eced8
STATUS=GREEN_REFERENCE_BUDGET_DIRECT_ACOUSTIC_REJECTED_TIME_POLICY_UNSELECTED
PHYSICAL_SCALE_SELECTION=UNSELECTED
PHYSICAL_TIME_POLICY=UNSELECTED
ATMOSPHERE_SOLVER_SELECTION=UNSELECTED
REFERENCE_MACHINE_CPU_BUDGET=ACCEPTED_PHASE5_TARGET
REFERENCE_MACHINE_MEMORY_BUDGET=ACCEPTED_PHASE5_TARGET
DIRECT_REAL_ACOUSTIC_60HZ_EXPLICIT_HLLC=REJECTED
UNIFORM_ACOUSTIC_SCALING_DEFAULT=REJECTED
HYBRID_ALL_SPEED_EVENT_LOCAL_COMPRESSIBLE=REQUIRED_NEXT_CANDIDATE_UNIMPLEMENTED
PRODUCTION_AIR_CHANGED=false
```

## Decision

Phase 5 now has an explicit, fail-closed reference-machine budget:

```text
reference_tick=16.6666666666666667 ms
atmosphere_cpu_fraction=0.25
atmosphere_cpu_budget=4.166666666666666675 ms/tick
authoritative_state_budget=64 bytes/cell
working_state_and_scratch_budget=256 bytes/cell
scope=Phase 5 reference machine only
```

This is an engineering gate for the recorded Intel Core Ultra 9 275HX machine,
not a cross-hardware release requirement and not a coupling between simulation dt
and presentation FPS. The quarter-tick allocation is a game-tuned reference target
that leaves measured headroom around the existing strict mixed-scene Legacy CPU
baseline of `5.268476 ms/tick`.

The current strict-double single-threaded HLLC measurement fits one substep and the
memory bound:

```text
HLLC 153x96=1.69129 ms/substep
HLLC authoritative=32 bytes/cell <= 64
HLLC working=160 bytes/cell <= 256
HLLC working total=2,350,080 bytes <= 3,760,128 bytes
maximum whole HLLC substeps inside CPU budget=2
```

The existing passive-species fixture at `40/200 bytes/cell` also fits the byte
limits, but this checkpoint does not claim target-size species throughput.

## Acoustic feasibility result

The calculation binds these inputs:

```text
cell_length=0.004 m
tick_candidate=1/60 s
target_cfl=0.2
air_sound_speed_reference=344 m/s near 293 K
measured_HLLC_cost=1.69129 ms/substep
```

For an explicit compressible step:

```text
required_substeps=ceil(c * dt_tick / (CFL * dx))=7167
estimated_atmosphere_time=12121.47543 ms/tick
budget=4.166666666666666675 ms/tick
result=REJECTED_REFERENCE_MACHINE_BUDGET
```

Restricting the same solver to the two affordable substeps would cap the CFL-
compatible signal speed at approximately `0.096 m/s`. That would reduce the air
sound speed by roughly three orders of magnitude, so uniform acoustic scaling is
rejected as the default reality mapping. It may still be useful as an explicitly
game-tuned experimental mode, but it is not selected here.

The next required comparison is an all-speed/hybrid design that separates normal
low-Mach transport cost from event-local compressible pressure-wave/shock work, or
another measured method that satisfies the same contracts. That candidate is not
implemented, so physical time and the solver remain `unselected`.

## Data provenance

The single acoustic feasibility value comes from NASA NTRS document
`19720017735`, *Atmospheric sound propagation*, publication date 1969-01-01. The
report gives an approximate air sound speed of 344 m/s at 20 degrees Celsius and
explains its temperature dependence. NTRS marks the document public and a US
Government work with public use permitted. The project stores only this approximate
reference value and provenance; it is not a runtime material database.

The clean test-free source package is
`artifacts/vnext-phase5-source-40facfdfa/`, SHA-256
`293105F3E87D964608CE8D0ACF9D1EBD16417FF4C89623D3400941E9C753706D`.
It contains `1319` source members plus one manifest, includes the policy data,
validator and this report, contains zero test assets, and binds revision
`40facfdfa7da907e539b2eb109f78299167c4cd1`.

## Validation and compatibility

```text
policy_recomputation=PASS
required_substeps=7167
required_acoustic_milliseconds=12121.47543
maximum_budgeted_substeps=2
maximum_budgeted_signal_speed=0.096 m/s
full_build=PASS
meson_static=70/70 PASS
python_discovery=424 PASS, 2 skipped, 426 total
targeted_policy_package_license=21/21 PASS
third_party_license_audit=PASS
source_package=1319 source members plus manifest, 0 test assets
```

- No production Air, Simulation, Particle, Save, Lua, renderer or SDL file changed.
- Element IDs, Particle layout, Lua identifiers and save format are unchanged.
- The validator rejects a changed substep result, a false budget pass, memory
  overflow, FPS-derived physical time, or a hybrid policy claiming implementation.
- GPU/VRAM/upload/readback remain `not_tested_no_gpu_backend`.

`V1_0_5_GATE=IN_PROGRESS`. The reference-machine CPU and per-cell memory budget
sub-gate is GREEN. The physical-time policy remains RED/unselected because all
currently implemented explicit candidates fail the real-acoustic cost requirement,
and the required hybrid/all-speed alternative has not been implemented. PhysicalScale,
mandatory case/precision coverage, G0 ledger semantics and solver selection remain
open.

Rollback commit: `a3c5ec9b7` removes this policy/budget checkpoint.
