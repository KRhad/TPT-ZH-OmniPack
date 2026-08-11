# Phase 5 hybrid component and mixed-region checkpoint

## Goal and disposition

The initial component checkpoint remains available as a standalone comparison. The
follow-up mixed-region probe adds a bounded 1D coupling proof without claiming a
production solver:

- constant-pressure conservative bulk transport whose step count is based on
  advective CFL; and
- the existing whole-case HLLC/Rusanov-fallback Sod fixture.

It does **not** implement physical 2D domain coupling, accepted physical-time
policy, near-vacuum/species routing or production integration. Therefore:

```text
candidate=hybrid_all_speed_event_local_1d_probe
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
BASE_COMMIT=c63f4e652e98d71dded4783ccec79b0e320d2f31
IMPLEMENTATION_COMMIT=fd353819287cef3ee05db4f7d2a4309c1ba4924a
ROLLBACK_COMMIT=c63f4e652e98d71dded4783ccec79b0e320d2f31
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

The executable and JSON explicitly retain these boundaries:

```text
router_implemented=true_1d_benchmark_only
routing_thresholds=mach_0.30_0.20_pressure_jump_0.08_0.03_benchmark_only
cross_route_boundary_coupling=implemented_1d_hllc_benchmark_only
event_local_subcycling=implemented_1d_four_substeps_benchmark_only
dynamic_event_region_and_acoustic_halo=implemented_1d_bounded_diagnostic_only
mixed_region_reflux_conservation=implemented_1d_benchmark_only
general_low_mach_pressure_coupling=not_implemented
physical_event_local_domain_of_dependence=not_implemented
target_grid_event_fraction_performance=not_tested
hybrid_near_vacuum_routing=not_implemented
two_dimensional_hybrid_coupling=not_implemented
species_eos_and_diffusion=not_implemented
production_boundary_coupling=not_implemented
production_runtime_integration=not_implemented
```

The 1D mixed-region fixture uses Mach thresholds `M_on=0.30`, `M_off=0.20` and
relative-pressure-jump thresholds `0.08/0.03`; compressible conditions win
conflicts and source impulses force the event route. These are benchmark policy
parameters, not production selections. The event halo is dynamically measured,
but the physical acoustic domain of dependence is explicitly unimplemented.

## Mixed-region result

```text
promotion_count=1696
demotion_count=1634
cross_route_face_count=5432
maximum_event_cells=64
maximum_event_fraction=1.0
maximum_halo_cells=1
maximum_event_substeps_used=4
maximum_cfl=0.272276
hllc_fallback_count=0
interface_ledger_closes=true
reflux_conservation_passed=true
hysteresis_conflict_passed=true
threshold_scan_passed=true
global_ledger_closes=true
mass_drift=-7.10543e-15
momentum_x_drift=6.17562e-16
energy_drift=2.84217e-14
remap_mass_delta=0
remap_momentum_delta=0
remap_energy_delta=0
numerical_correction_count=0
```

## Clean evidence

The clean runner artifact for the mixed-region probe is:

```text
artifacts/vnext-atmospherebench-hybrid-mixed-region/20260811T045949Z-9f874fff/result.json
source_commit=fd353819287cef3ee05db4f7d2a4309c1ba4924a
source_dirty=false
result_sha256=861CB26AA846AE07E770880AD95878B4D9DDB7747526CA298D3CA23691CB2D07
```

Validation at the implementation commit:

- full Windows/UCRT64 build: `620/620 PASS`;
- Meson static suite: `72/72 PASS`;
- Python discovery: `428 total`, `426 PASS`, `2` declared skips;
- focused policy/AtmosphereBench/runner tests: `31/31 PASS`;
- PowerShell parser and `git diff --check`: PASS;
- AtmosphereBench self-test and candidate list: PASS.

The earlier component artifact remains retained for comparison; it is not a
coupled-policy result and is superseded for the current checkpoint.

## Memory and performance boundary

The 1D mixed probe uses the bounded benchmark allocation defined by its runner and
reports `maximum_event_fraction=1.0`. It is therefore not evidence of a speedup or
of an acceptable target-grid budget. Physical-time and 2D memory/performance
results remain untested.

## Source package

The clean test-free implementation package is:

```text
artifacts/vnext-phase5-source-fd3538192/TPT-ZH-OmniPack-1.0.0-Source.zip
sha256=ED71B5E45DAD84D847B199D7D563ECBB13B5E2FA39698246F73C32CE0B9FDFAF
revision=fd353819287cef3ee05db4f7d2a4309c1ba4924a
source_members=1324
manifest_members=1
test_assets=0
```

## Next gate

The next candidate is an isolated 2D mixed-region/domain-of-dependence probe with
target-grid event-fraction and memory/time evidence. It must preserve the same
interface and global ledgers, exercise promotion/demotion across both axes, and
keep near-vacuum/species/physical-time boundaries explicit. Until that evidence,
plus the remaining PhysicalScale and precision gates, passes, solver selection and
1.0.6 remain blocked.
