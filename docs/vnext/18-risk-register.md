# OmniCore vNext risk register

## Active risks

## 1.0.8 closure

```text
V1_0_8_OMNICHEM=GREEN_ONE_REDUCED_CARBON_OXIDATION_RUNTIME
V1_0_8_COMBUSTION=GREEN_ENHANCED_O2_CO2_ENERGY_ATOMIC_LEDGER
V1_0_8_CARBON_OWNERSHIP=GREEN_DEDICATED_DOUBLE_SIDECAR
V1_0_8_PERSISTENCE=GREEN_OPS_CARBON_V1_SNAPSHOT_UNDO_REDO
V1_0_8_FP_MATRIX=GREEN_STRICT_AND_LEGACY_FAST_SPECIALIZED_PROBES
V1_0_8_VALIDATION=GREEN_BUILD_90_STATIC_436_PYTHON_LUA_SAVE_UPSTREAM
V1_0_8_PERFORMANCE=RECORDED_STRICT_MIXED_MEDIUM_OFF_114_319792_ON_90_627650_STEPS_PER_SECOND
V1_0_8_PERFORMANCE_OVERHEAD=RECORDED_PROFILER_SLOWDOWN_20_724445_PERCENT_NOT_A_RELEASE_GATE
V1_0_8_NEXT=1.0.9_MIXTURES_SOLUTIONS_MATERIAL_REALITY
```

The 1.0.8 chemistry scope is intentionally bounded: one versioned,
strict-double carbon-oxidation transaction (`C(s) + O2(g) -> CO2(g)`) is
integrated in Enhanced/Scientific mode. Classic keeps its legacy COAL path.
Kinetics are explicitly game-tuned and the OpenStax enthalpy value is recorded
with CC BY 4.0 attribution. No generic mechanism or third-party runtime is
embedded in the simulation loop.

## 1.0.7 closure

```text
V1_0_7_CLOSED_CHEMISTRY_GATE=not_applicable_deferred_to_1.0.8
V1_0_7_CLOSED_THERMAL_OWNERSHIP=GREEN_STRICT_DOUBLE_WATER_TRANSACTION
V1_0_7_CLOSED_SAVE_STATE=GREEN_OPS_V2_WATER_SIDECAR_V1
V1_0_7_CLOSED_UNDO_STATE=GREEN_SNAPSHOT_AND_DELTA_AUTHORITATIVE_RESTORE
V1_0_7_CLOSED_PERFORMANCE=GREEN_MEDIAN_4.000402987MS_PER_TICK
V1_0_7_REMAINING_FOG_DROPLETS=YELLOW_CONDENSED_CELL_SIDECAR_NOT_VISIBLE_PARTICLES
V1_0_7_REMAINING_TRACE_SPECIES=YELLOW_FIXED_COMMON_CHANNELS_ONLY
V1_0_7_REMAINING_SCIENTIFIC_MODE=YELLOW_SAME_RUNTIME_EQUATIONS_AS_ENHANCED
```

| ID | Severity | Area | Evidence / failure mode | Required mitigation | Gate |
|---|---|---|---|---|---|
| C-01 | CRITICAL | correctness/performance | direct real-acoustic mapping remains rejected at 7167 substeps/12121.47543 ms per 60 Hz tick; isolated 64x48 Jacobi projection reduces divergence to ratio 4.98891e-5 but costs 11.7071 ms for 4000 iterations and is not a coupled solver; bounded 1D/2D ledgers, evolving near-vacuum and passive species probes pass, but 32x24 reaches event fraction 0.927083, physical acoustic domain is 1434 cells, and the three-grid matrix exceeds 4.16667 ms budget | choose a physical scale/time and general low-Mach pressure policy that meets domain/budget constraints; replace or reject the component-only projection and keep solver selection RED | GREEN bounded routing/evolution and component projection evidence, RED physical policy |
| C-02 | CRITICAL | numerical | same-template strict-double/strict-float/fast-float Rusanov matrix stays positive with max signature delta 1.70385e-6/1.73887e-6, but float conservation drifts are nonzero (near-vacuum mass up to 6.10352e-5; strict-float Sod energy 1.83105e-4) | define accepted relative conservation tolerances, add longer/stiffer cases and keep fast-math unselected until safety evidence closes | GREEN first precision matrix, RED fast-math selection |
| C-03 | CRITICAL | conservation | floors/clamps could silently create mass/species/energy; 12 explicit Air cap branches now have a bounded Legacy-field observer, but all other correction/source paths remain unobserved and no physical units exist | extend branch-outcome coverage beyond audited Air caps, then add physical source/sink attribution and correction limits after the scale/state contract | GREEN audited Air-cap sub-gate, RED complete physical ledger |
| C-08 | HIGH | lifecycle accounting | record observer covers central APIs plus three audited direct paths, but not physical units, all raw writes, correction branches or full state | retain reconciliation failure as evidence, expand only with explicit branch hooks and never promote record deltas to physical conservation | YELLOW foundation, RED physical |
| C-04 | HIGH | correctness | Legacy particle updates directly write Air and depend on iteration/same-frame state | C01-C14, inventory, same-source FP field capture and OPS load-boundary field attribution exist; add Classic/Omni compatibility adapter | YELLOW foundation, RED replacement |
| C-05 | HIGH | correctness | scale/time/effective depth are not yet accepted | Phase 2 contract and dimensional checks | RED |
| C-06 | HIGH | chemistry | 1.0.8 validates atom/charge/molar mass and finite-rate kinetics for one compiled carbon-oxidation subset, but no generic data loader or broader mechanism exists | retain the validated subset; add versioned loader and rejection tests only in later chemistry scope | GREEN bounded subset, RED general runtime |
| C-07 | HIGH | thermal | 1.0.7 water enthalpy/latent-heat coupling closes the bounded water transaction, but other materials still use Legacy thresholds | extend unified energy state material-by-material without changing Classic | GREEN water subset, RED general thermal replacement |
| L-01 | CRITICAL | Lua compatibility | scripts directly use Legacy `pv/vx/vy/hv` semantics | preserve Classic fields and versioned Enhanced APIs/projection | RED for replacement |
| L-02 | CRITICAL | save compatibility | no versioned conservative atmosphere schema or unknown-species fallback | independent OPS object/chunk plus old/new/corrupt fixture matrix | RED |
| L-03 | HIGH | Particle ABI | AoS pointers, `offsetof` and FIELD indices are widely depended upon | ParticleAccessor/View first; defer SoA | RED for layout rewrite |
| L-04 | HIGH | upstream | future stable may touch Air, Particle, Heat, Lua, Save or build; 2026-08-10 refresh found no new commit after 100.1, while unrelated local `v99.5.394` blocks only tag-inclusive fetch | phase-start `ls-remote/fetch`, isolated impact Worker, regression conversion; never force-overwrite historical tags | YELLOW |
| L-05 | HIGH | gameplay | replacing FIRE/pressure/vacuum could break old works | immutable Classic backend and differential traces | RED for replacement |
| L-06 | MEDIUM | legacy formats | PSv/fuC and GUI save/load lack current runtime fixtures | add safe fixed fixtures and visible GUI pass | YELLOW |
| L-07 | HIGH | save/differential | pre-save versus loaded Snapshot hash differs in 14/14 characterization cases because OPS normalizes/quantizes state | formal field-level report now attributes Particle/Air/settings differences and loaded-A/B is repeatable; treat loaded OPS as baseline and never claim bit-exact checkpointing | YELLOW, characterized |
| P-01 | HIGH | benchmark | fixed-step/process-RAM characterization and default-off subsystem timing now exist; final profiler OFF/ON pairs are deterministic but span `-2.493%` through `+4.663%`, with no accepted noise model or regression budget | retain raw paired evidence, define workload/repeat/noise policy and set evidence-based budgets | YELLOW |
| P-02 | HIGH | memory | Phase 5 reference target is now 64 authoritative and 256 working bytes/cell; HLLC 32/160 and passive species 40/200 fit, but target-size multi-species throughput and production peak RAM remain unmeasured | enforce the budget for every candidate and measure target-size multi-species process peak | YELLOW |
| P-03 | HIGH | rendering | renderer snapshot already copies about 14.596 MiB lower bound per frame | selected-plane debug copies and measured snapshot timing | YELLOW |
| P-04 | HIGH | GPU | CPU-special/GPU-generic may force full readback and stalls | residency design and upload/readback/fence metrics | RED |
| P-05 | HIGH | GPU semantics | allocator, movement and same-pass neighbor writes are order-conflicted | multi-pass proposal/arbitration/apply and bounded request queues | RED |
| P-06 | HIGH | element coverage | 488 classifications remain UNKNOWN; lexical risk flags 420 high | AST/path-sensitive/manual classification before GPU coverage claims | RED |
| P-07 | HIGH | benchmark determinism | Legacy `clear_sim()` leaves derived Air blocking maps; mixed replay diverged before an explicit empty-step sanitation | centralize pristine-reset contract and add cache-state regression before differential traces | YELLOW, runner-controlled |
| P-08 | MEDIUM | diagnostic overhead | enabled lifecycle observer scans `parts.active` twice/tick and keeps two `PT_NUM` int64 histograms (16,384-byte array floor at current capacity) | keep default disabled; benchmark enabled cost before using it in long-run evidence | YELLOW |
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
RUNTIME_RECORD_LIFECYCLE_OBSERVER=GREEN
RUNTIME_CORRECTION_OBSERVER=GREEN
AUDITED_AIR_CAPS_RUNTIME_EVIDENCE=GREEN
UNSAMPLED_FULL_STATE_FINITE=RED
PHYSICAL_CONSERVATION_LEDGER=RED
SOURCE_SINK_CORRECTION_LEDGER=RED
LOAD_BOUNDARY_FIELD_DIFF=GREEN
LEGACY_CPU_VS_OMNI_CPU=RED
OMNI_CPU_VS_OMNI_GPU=RED
CONSERVATION_POSITIVITY_FINITE_LEDGER=RED
FIXED_STEP_BENCHMARK_FOUNDATION=GREEN
SUBSYSTEM_PROFILER_EXPORT=GREEN
PROFILER_OVERHEAD_MEASURED=GREEN
INDEPENDENT_PROFILER_REVIEW=not_available
PERFORMANCE_REGRESSION_BUDGET=RED
PROCESS_RAM_BASELINE=GREEN
PROCESS_VRAM_BASELINE=RED
NUMERICAL_FOUNDATION=RED
PHYSICAL_SCALE=RED
ATMOSPHERE_IMPLEMENTATION=RED
SDL3=RED
GPU=RED
OMNICHEM=GREEN_BOUNDED_CARBON_OXIDATION_SUBSET_RED_GENERAL_MECHANISM
MATERIAL_DATA_IMPORT=RED
G0_UPSTREAM_BASELINE=RED
```

RED stops the affected downstream path. It does not prohibit work whose sole purpose
is to remove the stated blocker.
