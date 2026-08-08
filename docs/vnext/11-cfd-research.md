# CFD research for AtmosphereBench

## Decision status

No atmosphere algorithm is selected for production. The leading CPU-reference
candidate is a conservative finite-volume compressible mixture solver using strict
double precision and a first-order HLLE/Einfeldt-class flux. This is a hypothesis to
test, not an architectural commitment.

## Candidate comparison

| Candidate | Strengths | Principal risks | First PoC role |
|---|---|---|---|
| Legacy-like | known TPT boundaries, cheap, stable gameplay | no mass/species/EOS conservation | characterization/control |
| Rusanov / local Lax-Friedrichs FVM | simple, robust, positivity-friendly, GPU-regular | very diffusive contacts/mixing | minimum conservative baseline |
| HLLE FVM | robust shocks/rarefactions and near-vacuum behavior when correctly implemented | diffuses contacts; still acoustic-CFL limited | preferred CPU reference candidate |
| HLLC FVM | sharper contact discontinuities and material transport | more fragile near vacuum and under bad reconstructed states | quality candidate with HLLE fallback |
| LBM | regular local stencil and strong GPU fit; good low-Mach flow | weak compressibility assumptions, shock/near-vacuum/large density-ratio difficulty | GPU-friendly comparison, not default |
| Hybrid/all-speed | can separate low-Mach gameplay flow from compressible events | coupling complexity and conservation proof | only if benchmark exposes FVM cost/fidelity failure |

MUSCL/PLM and slope limiters are second-stage improvements. The first conservative
implementation should remain first-order until positivity, boundaries and ledgers
are trustworthy.

## Mandatory experiments

AtmosphereBench must run the same scale, boundary and precision contracts for:

1. uniform state preservation;
2. density advection;
3. pressure pulse;
4. Sod shock tube;
5. contact discontinuity;
6. near-vacuum expansion;
7. sealed gas heating;
8. leak;
9. hydrostatic atmosphere;
10. natural convection;
11. gas mixing;
12. thermal plume;
13. blast-like pressure event.

Each test reports mass, momentum, total energy and species drift; positivity;
numerical diffusion; shock/contact error; low-Mach and near-vacuum stability;
floor/correction counts; CPU time; working memory; GPU mapping; and compatibility
with TPT wall/fan/open-edge behavior.

## State and flux contract

The conservative candidate stores:

```text
U = [rho, rho*u, rho*v, rho*E, rho_species_0 ... rho_species_n]
```

Pressure, temperature, velocity, Mach number, sound speed and humidity are derived.
For a five-species ideal-gas mixture, all five partial densities may be stored during
the reference phase; closure compression is only considered after differential
tests. Species fluxes use the same face mass flux so their sum remains consistent
with total density.

Sources for gravity, boundary exchange, particle coupling, reactions and user tools
are split from flux updates and entered into the ledger. A floor is a numerical
correction, not invisible mass creation.

## Near vacuum

The solver never accepts ordinary `rho=0` cells. It uses explicit density, internal-
energy/pressure and species floors, conservative reconstruction fallback, HLLE or
Rusanov fallback on invalid HLLC states, and a `NumericalCorrectionLedger` containing
mass/species/energy introduced or removed by every repair. Atmosphere vacuum is a
low-mass state, not negative pressure.

Thresholds must be scaled and derived from the physical contract. Their total effect
is tested per step and per 1,000 steps.

## Acoustic CFL risk

At the physical-scale proposal of a 4 mm atmosphere cell, real air sound speed makes
an explicit compressible step roughly microseconds, far below a 1/60 second game
tick. A naive fully dimensional solver would require an unacceptable number of
substeps. This is the largest unresolved solver risk.

AtmosphereBench must compare at least:

- physical time dilation/fixed small simulation `dt`;
- a documented scaled speed-of-sound model;
- low-Mach preconditioning or an all-speed/hybrid method;
- event-local compressible refinement/subcycling.

No result may be called physically timed until this mapping is explicit.

## Research references

- Athena++ at `ed4d1e3e3a3beb53ab9757dcc4b964dfbbad621a`, including its public
  method paper and test philosophy: <https://github.com/PrincetonUniversity/athena>
- hydro-cl-lua at `80b4119547556284debc0b8c2b5d0f53efa47acd` for comparative solver
  experimentation patterns: <https://github.com/thenumbernine/hydro-cl-lua>
- official TPT Air and boundary semantics in this repository.

Both external solvers are `REFERENCE_ONLY`; no source is copied into production.

## Gate

Research plan is GREEN. Solver selection and implementation remain RED until the
physical-scale decision and reproducible AtmosphereBench results pass.
