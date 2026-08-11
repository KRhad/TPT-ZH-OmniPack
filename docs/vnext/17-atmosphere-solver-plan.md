# AtmosphereBench and solver selection plan

## Goal

Select the least complex solver that meets conservation, positivity, TPT boundary,
real-time and backend requirements. AtmosphereBench is an independent CPU tool first;
it does not replace `Air.cpp` or alter saves/gameplay.

## Planned comparison order

The 1.0.5 scaffold registers Legacy-like, Rusanov-family FVM, HLLE, HLLC and LBM
names. Current isolated execution covers the strict-double Rusanov debug floor,
the rejected all-speed Rusanov experiment, HLLC/Rusanov-fallback across the
current 1D/2D physics and performance matrix, and isothermal D2Q9 uniform/shear
probes. Legacy-like remains a control registration and HLLE remains registered-
only. D2Q9 is now actual comparison evidence, but its missing energy,
near-vacuum and shock support makes it ineligible as the unified solver. No
candidate is selected.

1. Legacy-like baseline using the current field semantics.
2. First-order strict-double Rusanov FVM.
3. First-order strict-double HLLE/Einfeldt FVM.
4. HLLC with automatic HLLE fallback.
5. A low-Mach LBM candidate.
6. Hybrid/all-speed candidate only if the above expose a measured need.

The original provisional recommendation was HLLE FVM because one conservative
state naturally carries mass, momentum, total energy and species. Measurements
have since made HLLC with explicit Rusanov fallback the current front-runner;
HLLE remains unimplemented, and D2Q9 is now limited comparison evidence rather
than a registration placeholder. Rusanov remains the debugging floor. None of
these observations is final solver selection.

## Tool architecture

AtmosphereBench contains no TPT element rules. It has:

- `PhysicalScale` and nondimensional conversion;
- versioned species and ideal-gas-mixture EOS data;
- conservative state and derived primitive views;
- pluggable flux, reconstruction, source and boundary policies;
- strict double reference and float32 variants;
- deterministic initial-condition generators;
- conservation/correction ledgers;
- CSV/JSON result and trace output with source/build hashes;
- optional field snapshots for plots, never required for pass/fail.

## Boundary adapters

Test boundaries include reflecting wall, open/far-field, periodic, inflow/outflow,
fan/source, porous face and moving/dirty particle-derived masks. The bench consumes a
face-openness/permeability map so fluid substeps do not scan all particles.

The game adapter later rebuilds only dirty boundary chunks. It must reproduce Classic
hard walls and quantify any Enhanced permeability approximation.

## Test matrix and thresholds

All thirteen cases from `11-cfd-research.md` run at multiple resolutions and at
double-strict, float-strict and float-fast precision. Reports include:

- absolute and relative mass/momentum/energy/species drift per step and per 1,000;
- first divergence and L1/L2/Linf reference error;
- minimum density/pressure/internal energy and every floor hit;
- contact/shock location, diffusion width and oscillation/overshoot;
- CFL, substeps and failed/fallback cells;
- wall leakage and boundary source/sink ledger;
- milliseconds/step and cells/second at small/medium/large grids;
- persistent/peak bytes per cell;
- branch/SIMD/GPU suitability notes.

Thresholds are stored with each test and derived from analytic/reference convergence,
not tuned merely to make an implementation pass.

## Differential runner path

The game-side runner eventually executes:

```text
Legacy CPU vs Omni CPU
Omni strict CPU vs Omni float CPU
Omni CPU vs Omni GPU
```

It fixes save, seed, settings and frame count and records the first divergence frame,
particle ID/position/type/temperature, atmosphere cell, neighbors and differing
field. Classic comparison allows only enumerated intentional deviations; CPU/GPU
comparison uses metric tolerances plus ledgers, not arbitrary screenshot similarity.

## Debug/ledger requirements

Views: pressure, density, temperature, velocity, Mach, O2, CO2, H2O vapor, CO,
selected gas, humidity, vacuum level and energy.

Numerical telemetry: CFL, substeps, bad cells, density/pressure/energy floor hits,
NaN/Inf and reaction-stiffness warnings.

Ledger: condensed mass, gas mass, each species, elemental atoms, charge, energy,
momentum, boundary flow, user/Legacy sources/sinks and numerical corrections.

## Solver-selection gate

GREEN requires:

- deterministic/reproducible result artifacts;
- no unexplained NaN/Inf or negative thermodynamic states;
- ledger closure within per-test published tolerances;
- acceptable low-Mach, shock and near-vacuum results;
- a measured CPU budget and memory budget for target TPT sizes;
- a documented physical-time policy;
- no dependency on test names or special-case expected outcomes.

Until these are true, `ATMOSPHERE_SOLVER_SELECTED=false` and production integration
is RED.

### Current first-order Rusanov result

The isolated strict-double 1D Rusanov candidate now covers uniform preservation,
pressure pulse, smooth/contact advection, near-vacuum expansion, sealed Sod,
refinement and low-Mach characterization. It is positive and conservative for the
published probes, but the low-Mach test reports:

```text
nominal_mach=0.387298 / 0.0387298 / 0.00387298
density_l1_error=0.00826755 / 0.0515261 / 0.126479
total_variation_ratio=0.934805 / 0.595493 / 0.00673822
low_mach_suitability_passed=false
```

Therefore first-order compressible Rusanov is a reference/debug floor, not the
selected Enhanced-mode solver. The leak/open-boundary ledger is now explicit and
the standalone performance record is now measured, but neither changes selection:

```text
open_boundary_mass_out=6.69874
open_boundary_balance_errors<3e-14
performance=31.0232M / 31.3699M / 32.8059M cell-updates/s
performance_gate=recorded_candidate_measurement_no_budget
```

Solver selection additionally requires an all-speed/hybrid or otherwise
low-Mach-suitable path. HLLC currently passes the low-Mach fixture; D2Q9 also has
a strong isothermal shear result but cannot meet the total-energy, vacuum and
shock requirements. HLLE remains registration-only.

The first all-speed Rusanov experiment is now measured, not merely registered.
It is a valid negative comparison: the local-Mach/pressure-jump dissipation
scaling preserves the conservative ledger but produces
`very_low_density_l1_error=0.10116` and
`very_low_total_variation_ratio=1.46479`, so it fails the fixed Low-Mach gate.
The comparison remains open; no candidate is selected until an acceptable path is
benchmarked across the required cases and a physical-time/performance policy is
documented.

HLLC with explicit Rusanov fallback is now the front-runner for the remaining
PoC comparison. At nominal Mach `0.00387298` it records L1 `0.0024269` and TV
ratio `0.980949`; it also passes the current 1D near-vacuum, sealed Sod and open
leak contracts with zero fallbacks/corrections. Strict-double throughput is
approximately `18.5M` cell-updates/s, slower than the Rusanov debug floor. These
results justify continued evaluation, not selection. Periodic 2D, sealed heating,
natural convection, passive species mixing and target-size performance now also
have evidence. Physical-time policy, an accepted budget, actual Legacy-like
control and the remaining mandatory cases remain open.
