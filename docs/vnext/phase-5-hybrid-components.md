# Phase 5 uncoupled hybrid component checkpoint

## Goal and disposition

This checkpoint corrects the scope of the initial hybrid experiment. It proves two
standalone components only:

- constant-pressure conservative bulk transport whose step count is based on
  advective CFL; and
- the existing whole-case HLLC/Rusanov-fallback Sod fixture.

It does **not** implement a router, a mixed-domain hybrid solver, event-local
subcycling, cross-route face coupling or reflux. Therefore:

```text
candidate=hybrid_all_speed_components
candidate_solver_implemented=false
component_probe_passed=true
hybrid_end_to_end_passed=false
policy_selection_ready=false
atmosphere_solver_selection=unselected
V1_0_5_GATE=IN_PROGRESS
V1_0_6=BLOCKED_BY_GATE
```

## Revision and rollback

```text
BASE_COMMIT=c3607c349f87457efad2d95615b29b7deeaa109f
IMPLEMENTATION_COMMIT=c63f4e652e98d71dded4783ccec79b0e320d2f31
ROLLBACK_COMMIT=c3607c349f87457efad2d95615b29b7deeaa109f
BRANCH=integration/omnicore-vnext
```

The change is isolated to AtmosphereBench, its runner/contracts, the offline
policy registry and documentation. No production `Air`, `Simulation`, `Particle`,
Save, Lua, SDL or gameplay source changed.

## Component results

The periodic density-wave fixtures all use 128 cells, an exact 16-cell shift and
36 advective steps. The density result is identical across the three velocities:

```text
density_l1_error=0.00135653
density_linf_error=0.00213007
total_variation_ratio=0.989347
maximum_advective_cfl=0.444444
```

The diagnostic acoustic CFL rises as the advective velocity falls:

```text
moderate=1.7274
low=13.274
very_low=128.74
```

An alternate-EOS very-low-velocity fixture changes the acoustic CFL to `118.029`
while retaining 36 steps and the same density L1/TV results. This directly proves
that this component's step count and passive density transport are independent of
EOS sound speed. It does not prove a general low-Mach pressure solver.

The whole-case sealed Sod fixture remains positive with maximum CFL `0.280647`,
minimum density `0.125`, minimum pressure `0.1`, no fallback and no numerical
correction. Mass and energy drift are `1.7053e-13`. X-momentum changes by `46.08`
and the sealed-wall boundary ledger records the same `46.08`; the runner verifies
closure against boundary exchange instead of incorrectly requiring zero momentum.

## Mandatory unimplemented boundaries

The executable and JSON explicitly retain:

```text
router_implemented=false
routing_thresholds=not_implemented
cross_route_boundary_coupling=not_implemented
event_local_subcycling=not_implemented
dynamic_event_region_and_acoustic_halo=not_implemented
mixed_region_reflux_conservation=not_implemented
general_low_mach_pressure_coupling=not_implemented
physical_event_local_domain_of_dependence=not_implemented
target_grid_event_fraction_performance=not_tested
hybrid_near_vacuum_routing=not_implemented
two_dimensional_hybrid_coupling=not_implemented
species_eos_and_diffusion=not_implemented
production_boundary_coupling=not_implemented
production_runtime_integration=not_implemented
```

The moderate nominal Mach value is `0.387298`; it is a fixture label, not evidence
that a future router should classify the case as low Mach. No routing thresholds
are selected in this checkpoint.

## Clean evidence

The clean runner artifact is:

```text
artifacts/vnext-atmospherebench-reviewfix/20260811T041857Z-3e561ae3/result.json
source_commit=c63f4e652e98d71dded4783ccec79b0e320d2f31
source_dirty=false
result_sha256=6088F054F2C11CEE5B75912C9B9BA2F4AAA0F5CABD8970924E811E2A08B33DEA
stdout_sha256=60B251ACEC3E3F284659553E7F1E5847596D7C835CED197C10665DFAE19A23A1
```

Validation at the implementation commit:

- full Windows/UCRT64 build: `610/610 PASS`;
- Meson static suite: `71/71 PASS`;
- Python discovery: `427 total`, `425 PASS`, `2` declared skips;
- focused policy/AtmosphereBench/runner tests: `30/30 PASS`;
- PowerShell parser and `git diff --check`: PASS;
- AtmosphereBench self-test and candidate list: PASS.

The first runner artifact from `c3607c349` with SHA-256
`55E50525AC7363DCCD006C3C32DC5987290F638F60E4AF043B9255298689CB17`
is superseded because its naming overclaimed a router and hybrid policy.

## Memory and performance boundary

The constant-pressure component uses `32 bytes/cell` authoritative state and
`96 bytes/cell` for its current/next/initial working allocation. It is inside the
Phase 5 `64/256 bytes/cell` reference limits. Timing is intentionally recorded as
`not_evaluated_uncoupled_policy_probe`; no target-grid event-fraction performance
or physical-time claim is made.

## Source package

The clean test-free implementation package is:

```text
artifacts/vnext-phase5-source-c63f4e652/TPT-ZH-OmniPack-1.0.0-Source.zip
sha256=9D3D945EBC7EE0F974D5FE499713B025933AA7B50B5461303A6556C26CB64B72
revision=c63f4e652e98d71dded4783ccec79b0e320d2f31
source_members=1321
manifest_members=1
test_assets=0
```

## Next gate

The next candidate must be a single mixed domain with at least one promotion,
one cross-route face, one demotion and a reflux ledger whose two sides are equal
and opposite. It must also test routing hysteresis/conflicts, dynamic event-region
growth, acoustic domain of dependence and target-grid event-fraction cost. Until
that evidence passes, solver selection and 1.0.6 remain blocked.
