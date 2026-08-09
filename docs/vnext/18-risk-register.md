# OmniCore vNext risk register

## Active risks

| ID | Severity | Area | Evidence / failure mode | Required mitigation | Gate |
|---|---|---|---|---|---|
| C-01 | CRITICAL | correctness/performance | 4 mm cells with real sound speed make explicit compressible CFL far smaller than a game tick | select and validate time/acoustic/all-speed policy in AtmosphereBench | RED |
| C-02 | CRITICAL | numerical | identical generated mixed state diverges on step 1; 1,001 post-update exported-float states remain finite/in Legacy range, but RNG splits by step 34 and physical/conservation error remains unknown | retain field/proxy evidence and add double/float/fast physical conservation, positivity, correction and internal/full-state matrix | GREEN capture/proxy, RED physical comparison |
| C-03 | CRITICAL | conservation | floors/clamps could silently create mass/species/energy; source audit identifies 12 explicit Air cap branches but no runtime correction ledger | optional branch-outcome observer, correction limits and fail-closed reconciliation tests | RED |
| C-04 | HIGH | correctness | Legacy particle updates directly write Air and depend on iteration/same-frame state | C01-C14, inventory, same-source FP field capture and OPS load-boundary field attribution exist; add Classic/Omni compatibility adapter | YELLOW foundation, RED replacement |
| C-05 | HIGH | correctness | scale/time/effective depth are not yet accepted | Phase 2 contract and dimensional checks | RED |
| C-06 | HIGH | chemistry | current reactions lack generic atom/charge validation and kinetics | versioned species/reaction loader with rejection tests | RED |
| C-07 | HIGH | thermal | phase changes lack latent heat and unified energy | enthalpy model and closed energy experiments | RED |
| L-01 | CRITICAL | Lua compatibility | scripts directly use Legacy `pv/vx/vy/hv` semantics | preserve Classic fields and versioned Enhanced APIs/projection | RED for replacement |
| L-02 | CRITICAL | save compatibility | no versioned conservative atmosphere schema or unknown-species fallback | independent OPS object/chunk plus old/new/corrupt fixture matrix | RED |
| L-03 | HIGH | Particle ABI | AoS pointers, `offsetof` and FIELD indices are widely depended upon | ParticleAccessor/View first; defer SoA | RED for layout rewrite |
| L-04 | HIGH | upstream | future stable may touch Air, Particle, Heat, Lua, Save or build | phase-start `ls-remote/fetch`, isolated impact Worker, regression conversion | YELLOW |
| L-05 | HIGH | gameplay | replacing FIRE/pressure/vacuum could break old works | immutable Classic backend and differential traces | RED for replacement |
| L-06 | MEDIUM | legacy formats | PSv/fuC and GUI save/load lack current runtime fixtures | add safe fixed fixtures and visible GUI pass | YELLOW |
| L-07 | HIGH | save/differential | pre-save versus loaded Snapshot hash differs in 14/14 characterization cases because OPS normalizes/quantizes state | formal field-level report now attributes Particle/Air/settings differences and loaded-A/B is repeatable; treat loaded OPS as baseline and never claim bit-exact checkpointing | YELLOW, characterized |
| P-01 | HIGH | benchmark | two-scene fixed-step and 14-scene process-RAM characterization exist; differential/ledger process diagnostics are not benchmarks; no accepted noise model, subsystem timings or regression budget exists | controlled repeats, profiler export and evidence-based budgets | YELLOW |
| P-02 | HIGH | memory | multi-species state/flux/scratch can exceed budget | report persistent/peak bytes per cell for every design | RED |
| P-03 | HIGH | rendering | renderer snapshot already copies about 14.596 MiB lower bound per frame | selected-plane debug copies and measured snapshot timing | YELLOW |
| P-04 | HIGH | GPU | CPU-special/GPU-generic may force full readback and stalls | residency design and upload/readback/fence metrics | RED |
| P-05 | HIGH | GPU semantics | allocator, movement and same-pass neighbor writes are order-conflicted | multi-pass proposal/arbitration/apply and bounded request queues | RED |
| P-06 | HIGH | element coverage | 488 classifications remain UNKNOWN; lexical risk flags 420 high | AST/path-sensitive/manual classification before GPU coverage claims | RED |
| P-07 | HIGH | benchmark determinism | Legacy `clear_sim()` leaves derived Air blocking maps; mixed replay diverged before an explicit empty-step sanitation | centralize pristine-reset contract and add cache-state regression before differential traces | YELLOW, runner-controlled |
| D-01 | CRITICAL | data license | NIST WebBook is SRD with explicit copyright restrictions | reference-only unless item-specific redistribution permission is recorded | RED for bundling |
| D-02 | HIGH | code license | tpt-bench has no detected license | reference-only; write independent runner and cases | RED for reuse |
| D-03 | HIGH | mechanism data | Cantera code license does not license every mechanism/data file | per-input provenance/license audit | RED for bundling |
| D-04 | HIGH | data quality | current real-material values are not uniformly source/range/units documented | property-level provenance schema and validation | RED |
| B-01 | MEDIUM | build provenance | MSYS2 Git first on PATH falsely marks CRLF checkout dirty and adds `+` VCS tag | enforce Windows Git first and assert status/tag | YELLOW, controlled |
| B-02 | MEDIUM | build warning | GCC reports possible uninitialized `ByteString` optional path | isolate/reproduce and compare upstream before disposition | YELLOW |
| B-03 | MEDIUM | evidence provenance | ledger builds both targets, checks 754 compile commands and re-hashes EXEs/tools/DLL inventory, but the executable does not cryptographically embed the source commit and direct DLL/Python-module attribution is incomplete | retain exact hashes and declared limits; require reproducible/embedded build identity before portable claims | YELLOW, declared |
| S-01 | HIGH | SDL migration | 505 Lua SDL2 constants plus window/input/clipboard behavior | independent SDL3 phase and compatibility table | RED |
| S-02 | HIGH | shader pipeline | no local dxc/glslc/validation/shadercross | isolated toolchain PoC and transitive license audit | RED |

## Gate summary

```text
DATA_LOSS_RISK=not_observed
BUILD=GREEN
BOUNDED_LEGACY_LUA_OPS=GREEN
LEGACY_CHARACTERIZATION=GREEN
DETERMINISTIC_SAVE_RESTART=GREEN
UPSTREAM_SOURCE_ADAPTATION=GREEN
LEGACY_FAST_BUILD=GREEN
STRICT_FP_BUILD=GREEN
STRICT_FAST_NUMERICAL_COMPARISON=RED
FIRST_DIVERGENCE_FIELD_CAPTURE=GREEN
LEGACY_SAMPLED_FINITE_PROXY_LEDGER=GREEN
STRICT_FAST_SAMPLED_PROXY_COMPARISON=GREEN
ALL_TICK_POST_UPDATE_EXPORTED_FLOATS=GREEN
PHYSICAL_LEDGER_FEASIBILITY=GREEN
UNSAMPLED_FULL_STATE_FINITE=RED
PHYSICAL_CONSERVATION_LEDGER=RED
SOURCE_SINK_CORRECTION_LEDGER=RED
LOAD_BOUNDARY_FIELD_DIFF=GREEN
LEGACY_CPU_VS_OMNI_CPU=RED
OMNI_CPU_VS_OMNI_GPU=RED
CONSERVATION_POSITIVITY_FINITE_LEDGER=RED
FIXED_STEP_BENCHMARK_FOUNDATION=GREEN
PERFORMANCE_REGRESSION_BUDGET=RED
PROCESS_RAM_BASELINE=GREEN
PROCESS_VRAM_BASELINE=RED
NUMERICAL_FOUNDATION=RED
PHYSICAL_SCALE=RED
ATMOSPHERE_IMPLEMENTATION=RED
SDL3=RED
GPU=RED
OMNICHEM=RED
MATERIAL_DATA_IMPORT=RED
G0_UPSTREAM_BASELINE=RED
```

RED stops the affected downstream path. It does not prohibit work whose sole purpose
is to remove the stated blocker.
