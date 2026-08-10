# OmniCore vNext roadmap

## Current phase decision

Versions **1.0.1**, **1.0.2** and **1.0.4** are GREEN. Version 1.0.3 remains a
user-accepted YELLOW because the 100/125/150 percent DPI matrix is deferred. The
current version is **1.0.5 Physical Scale + AtmosphereBench**. Its phase-entry
refresh found stable/master unchanged at `d768aeb89`; the isolated strict-double
scaffold, shared contracts and Rusanov uniform/pressure-pulse/density-advection/contact-discontinuity/near-vacuum/Sod/refinement debug probes are now
clean-validated. Production Air replacement remains unauthorized.

The official 100.1 source adaptation, explicit FP modes, fixed-step
throughput/process-RAM baseline, C01-C14 deterministic restart suite and scoped
Legacy-fast/Strict CPU first-divergence capture plus sampled/all-tick numerical
proxy ledger, physical-ledger feasibility audit, optional record-level lifecycle
reconciliation, audited-Air-cap correction observation and OPS load-boundary field
attribution are integrated. Overall OmniCore G0 remains RED. No production
OmniAtmosphere state or solver will be integrated yet.

## Phase status

| Phase | Scope | Status / next gate |
|---:|---|---|
| -1 | external research and license audit | YELLOW: classifications complete; no new artifact redistribution authorized |
| 0 | latest upstream adaptation | 1.0.2 COMPLETE / GREEN: official stable/master unchanged at `d768aeb89`; no adaptation required |
| 1 | Legacy characterization, regression, profiler, benchmark | 1.0.1 COMPLETE / GREEN version gate; global physical-ledger G0 remains RED |
| 2 | physical scale and unit system | 1.0.5 IN_PROGRESS: candidate contract scaffold; selection RED |
| 3 | AtmosphereBench | 1.0.5 IN_PROGRESS: Rusanov has strict-double 1D uniform, pressure-pulse, density-advection, contact-discontinuity, near-vacuum, sealed Sod and smooth-refinement probes; HLLE/LBM remain registered-only |
| 4 | solver selection | BLOCKED |
| 5 | CPU single-species OmniAtmosphere MVP | BLOCKED |
| 6-10 | species, diffusion, convection, boundaries, coupling | BLOCKED |
| 11-18 | humidity, OmniThermal, materials, offline tools, OmniChem, mixtures/corrosion | BLOCKED |
| 19 | Particle access abstraction | planned; no AoS rewrite before audit |
| 20 | SDL3 stable migration | RED / independent after simulation baselines |
| 21-25 | SDL_GPU PoC and GPU/hybrid optimization | RED / CPU reference prerequisite |
| 26 | Scientific advanced systems | BLOCKED |

## Immediate executable work

The next integration commits should be small and independently reversible:

1. `tests/strict-fp`: COMPLETE in `abeca81bd`; explicit Legacy-fast/default and
   strict modes build and pass the bounded suites. Numerical comparison remains open.
2. `tests/kernel-benchmark`: COMPLETE in `c4490463f`; uncapped fixed-step runner,
   deterministic state signatures and structured machine/build/result manifest.
3. `tests/characterization`: COMPLETE in `1b8586877`; C01-C14 deterministic
   generators, private OPS artifacts, exact restart traces and aggregate manifest.
4. `tests/differential-first-frame`: COMPLETE in `c6eecaa77`; clean same-source
   Legacy-fast/Strict CPU trace locates step 1 and captures Particle, Air and
   neighborhood state. Omni CPU/GPU topology remains unimplemented.
5. `tests/legacy-proxy-ledger`: COMPLETE in `8bd640c3e` plus byte-stable replay fix
   `b3aa56cf3`; 102 sampled exported states per FP mode are finite/in Legacy range.
6. `tests/all-tick-ledger`: COMPLETE in `632665650`; `SampleInterval=1` records
   1,001 post-update exported-float states per FP mode as finite/in Legacy range.
   It does not claim internal/full state or pressure positivity.
7. `tests/load-boundary`: COMPLETE in `971687a24` plus closure commit `a09c6d716`;
   clean C01-C14 pre-save/loaded fields, loaded-A/B byte equality, 369 declared
   private files and frozen-comparator replay are recorded without a bit-exact or
   physical claim.
8. `tests/physical-ledger-feasibility`: COMPLETE in `86a2b3386`; source-bound
   storage/mutation/cap anchors prove why Legacy proxy fields are not physical units.
9. `tests/lifecycle-ledger`: COMPLETE in `4fa0ec2f3`; the disabled-by-default
   observer reconciles record deltas on every boundary of an isolated eight-tick
   runtime client, with central APIs plus SPRK and BRMT/TUNG direct paths covered.
   It has no physical units.
10. `tests/correction-ledger`: COMPLETE in `4c9f3b909` for the 12 audited Legacy
    Air cap branches, including a real FAN fixture and 2,048-event overflow
    replay. Complete correction/source-sink accounting and internal/full-state
    boundaries remain RED; do not call the current observer physical mass/energy.
11. `tests/profiler-export`: COMPLETE in `2b6b6e39e` plus fixture stabilization
   `97d2fc2c1`; final build has 40/40 Meson, 342 Python pass/2 skipped, runtime
   worker heartbeat and sequential OFF/ON pairs. Process VRAM remains explicitly
   `not_tested_no_gpu_backend`.
12. `upstream-impact`: COMPLETE in 1.0.2. Official stable/master remained
   `d768aeb89`; the tag-fetch collision is isolated to unrelated historical
   `v99.5.394`, and no source adaptation was needed.
13. `ui-material-organization`: COMPLETE in `477372373`; gate remains YELLOW only
   for deferred 100/125/150 percent DPI evidence.
14. `omnimaterials-data-foundation`: COMPLETE in `9cc2b11c5` plus gate closure
   `13b24f49e`; offline contracts and clean validation are GREEN.
15. `physical-scale-atmospherebench`: IN PROGRESS in 1.0.5; candidate scale,
   strict-double target, contract-only fixture and isolated Rusanov uniform,
   pressure-pulse and density-advection probes exist. Scale/time/solver selection remains RED.

The mixed benchmark starts from the same generated Strict/Legacy hash; the later
capture locates the first difference after update step 1 in both Particle and Air
state. The all-tick ledger finds no exported non-finite/range violation across all
1,001 post-update observations but observes RNG divergence by step 34. This
does not identify the correct result, so physical conservation/correction and
internal/full-state finite evidence remain mandatory before any fast-math safety
decision.

Each commit records goal, base, files, tests, benchmark/memory evidence, compatibility,
risks, gate and rollback parent. Before each new phase, re-query official stable and
master. A new upstream release triggers an isolated read-only impact analysis before
the Main Orchestrator chooses adaptation timing.

## First-round required answers

| Question | Answer |
|---|---|
| Current latest stable? | TPT `100.1 build 400`, tag `v100.1.400`. |
| Current upstream master? | `d768aeb89acad986bd252d7e904bf44bb374545f`, equal to stable at audit time. |
| What is local OmniPack based on? | Common base `bff38ce6959...`; pre-vNext fork tip `fb72d5e8f`; stable is now merged into `f1320b48d`. |
| Distance to stable/master? | Pre-adaptation: 181 local-only / 13 upstream-only. At the 1.0.5 entry base `13b24f49e`: 218 local-only / 0 upstream-only. |
| SDL2? | Yes, `2.30.9-tpt-libs`; no SDL3 production code. |
| Current Air? | Existing coarse pressure/velocity/temperature solver with advection, smoothing, walls, fans, vorticity and convection approximations. |
| `pv/vx/vy/hv`? | Dimensionless pressure-like field; two velocity-like fields; Kelvin-like ambient temperature, respectively. |
| Real gas density/mass/composition/partial pressure? | All `false`. |
| Current vacuum? | Negative `pv`, not low conserved gas mass. |
| Does combustion consume atmosphere O2? | No; selected rules consume O2 particles only. |
| Does boiling use ambient pressure? | It shifts thresholds by Legacy `-2*pv`; it does not use absolute pressure/saturation curves. |
| Humidity / latent heat? | Both absent. |
| Mass / energy conserved? | No physical conservation contract or ledger; the source audit confirms no authoritative mass/density/moles/energy state. Sampled/all-tick proxy fields are finite/in Legacy range but their counts/sums are not mass or energy. |
| Which elements write Air? | Static custom-update scan detects DMG writing Air velocity, LIGH writing Air heat, and 86 elements/38 roots writing pressure; complete IDs/evidence are in `element-update-inventory.json`. Generic `AirDrag/AirLoss/HotAir` adds more coupling. |
| Most dangerous GPU elements? | WARP, PSTN, PIPE/PPIP, PRTI/PRTO, ARAY/CRAY/DRAY, WIFI, SPRK, stickmen/fighters and shared Omni update roots. Formal classifications remain 488 UNKNOWN. |
| Who depends on Particle layout? | Lua/property descriptors, renderers, tools, updates, callbacks, pmap/photons and snapshot/copy paths. |
| Does Lua depend on particle properties? | Yes, through stable property names/indices and C++ `offsetof` access. |
| How is Air saved? | Quantized OPS `pressMap/vxMap/vyMap`, integer-K `ambientMap`, block/fan maps and simulation options. |
| Release math optimizations? | Vectorization, unsafe/fast math, omit frame pointer and SSE2; current validated build is `-O2`, `lto=false`. |
| Fast-math risk? | Critical: clean evidence diverges at step 1 and later splits RNG/types/counts. Sampled exports stay finite/in Legacy range, but neither side is physically validated; fast math is disallowed for OmniCore until physical conservation/positivity/unsampled-state comparison passes. |
| AtmosphereBench schemes? | Legacy-like, Rusanov, HLLE, HLLC, LBM and, if justified, hybrid/all-speed. |
| Leading PoC and why? | Strict-double first-order HLLE FVM: directly conserves mass/momentum/energy/species and is robust around shocks/rarefactions; benchmark may overturn it. |
| Near vacuum? | Positive density/pressure/internal-energy floors, robust flux fallback and a fully visible correction ledger; never ordinary `rho=0`. |
| Physical scale? | Candidate 1 mm pixel, 4 mm Air cell and 4 mm effective depth; fixed tick independent of rendering, but physical-time/acoustic mapping remains a RED decision. |
| Atmosphere cell state? | `rho`, `rho*u`, `rho*v`, `rho*E`, and active species partial densities; pressure/temperature/composition are derived. |
| Expected bytes/cell? | 40-48 persistent float32 bytes and about 128-180 working bytes for five common species; strict double reference can exceed 300. |
| Species storage? | Benchmark per-world active registry with dense common channels plus sparse trace chunks against fixed/shared alternatives. |
| Cantera role? | Optional offline validation, reduction and compact-database generation; not per-cell runtime. |
| CoolProp role? | Optional offline property/reference/table validation; not per-cell flash by default. |
| Useful NIST validation? | Thermochemistry, reaction enthalpy, Cp/Cv, phase transitions, vapor pressure, density and transport references. |
| What NIST data can be redistributed? | None by default from WebBook SRD 69; require item-specific permission/license and provenance. |
| Directly usable third parties? | Cantera and CoolProp offline; SDL/SDL_shadercross only through later gated adaptation. |
| Reference-only candidates? | tpt-bench, Athena++, hydro-cl-lua, NIST WebBook, and unpinned TPT/GPU falling-sand experiments. |
| Largest correctness risks? | acoustic CFL/time mapping, fast-math, hidden floor/clamp drift, missing energy/atom/charge contracts. |
| Largest compatibility risks? | Legacy Lua Air semantics, OPS schema, Particle AoS/indices, update order and Classic FIRE/vacuum behavior. |
| Largest performance risks? | species/flux memory, excessive substeps, renderer copies, CPU/GPU synchronization and special-element conflicts. |
| Next stage? | Add low-Mach or leak Rusanov evidence while HLLE/LBM remain registered-only. Production OmniAtmosphere remains unauthorized. |

## Long-term acceptance

Success requires all target phenomena to emerge from shared state, data and solvers
while Classic, old saves, Lua and CPU reference remain available. GPU coverage and
modernity never outrank correctness, data safety, compatibility or measurable
real-time behavior.
