# AtmosphereBench and solver selection plan

## Goal

Select the least complex solver that meets conservation, positivity, TPT boundary,
real-time and backend requirements. AtmosphereBench is an independent CPU tool first;
it does not replace `Air.cpp` or alter saves/gameplay.

## Planned comparison order

The 1.0.5 scaffold registers the Legacy-like, Rusanov, HLLE and LBM names. The
current isolated checkpoint implements only first-order strict-double Rusanov 1D
periodic uniform, pressure-pulse, density-advection, contact-discontinuity and near-vacuum-expansion debug probes. Legacy-like remains a control
registration; HLLE and LBM remain registered-only. These probes are not comparison
evidence and do not select a solver.

1. Legacy-like baseline using the current field semantics.
2. First-order strict-double Rusanov FVM.
3. First-order strict-double HLLE/Einfeldt FVM.
4. HLLC with automatic HLLE fallback.
5. A low-Mach LBM candidate.
6. Hybrid/all-speed candidate only if the above expose a measured need.

The provisional recommendation is HLLE FVM because one conservative state naturally
carries mass, momentum, total energy and species; it handles compressibility,
pressure waves and large density changes more directly than LBM. Rusanov is the
debugging floor; its current probes have passed only the isolated contracts described
in `phase-5-rusanov-candidate.md`. HLLC is a later accuracy option, and LBM remains
valuable for low-Mach/GPU comparison. Results may overturn this recommendation.

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
