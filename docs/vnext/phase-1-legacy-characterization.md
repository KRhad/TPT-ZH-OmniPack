# Phase 1 Legacy characterization

## Outcome

```text
REPORT_DATE=2026-08-09
BASE_COMMIT=b9ae20bf03232cd13418064b2951e993df6c9f20
SAVE_CONTINUATION_FIX=908878f747a4703bf9380868fb7e2407efaeaf41
SAVE_EDGE_MODE_FIX=ce8087d07573ca07b80e9a6cdcae33adb53fa56b
CHARACTERIZATION_IMPLEMENTATION=1b8586877e6e7d3703ecd1dbab1eb89b3e4eb7a2
CHARACTERIZATION_SAVES=GREEN
LOAD_BOUNDARY_IMPLEMENTATION=971687a24
LOAD_BOUNDARY_CLOSURE=a09c6d716
LOAD_BOUNDARY_FIELD_DIFF=GREEN
ALL_TICK_LEDGER_SCOPE=632665650
ALL_TICK_POST_UPDATE_EXPORTED_FLOATS=GREEN
G0_UPSTREAM_BASELINE=RED
PRODUCTION_OMNIATMOSPHERE_ALLOWED=false
```

The bounded C01-C14 Legacy characterization sub-gate is **GREEN**. A clean-source
Legacy-fast run generated all 14 project-owned OPS saves, then loaded each save in
two fresh client processes and produced identical per-step traces. Raw saves,
traces, process logs and result JSON remain under ignored `artifacts/` and are not
publication assets.

G0 remains **RED**. The later same-source Legacy-fast/Strict CPU first-divergence
capture, the OPS load-boundary field-attribution sub-gate and the all-tick
exported-float sub-gate are GREEN, but conservation/finite-value ledgers,
internal/full-state finite evidence, subsystem profiling, process VRAM and accepted
performance budgets are still missing. No production OmniAtmosphere implementation
is permitted.

## Goal and scope

This subphase establishes fixed Legacy scenes for sand, water, gases, fire,
explosion, Legacy vacuum/pressure, heat, electricity, photons, PIPE, complex
electronics, a mixed scene and allocator saturation. Every case fixes its seed,
settings profile, warmup count and trace count. The suite exercises the real client,
Lua API and OPS implementation; it does not fabricate saves or special-case the
simulation for expected outcomes.

The implementation adds:

- a 14-case data manifest and project-owned Lua generators;
- an isolated PowerShell three-process runner with bounded cleanup;
- raw and canonical OPS SHA-256 inspection;
- source, build, executable, machine, settings and process-memory provenance;
- exact cross-restart trace comparison; and
- contract tests for case identity, private-artifact policy, geometry, cleanup and
  OPS canonicalization.

## Formal artifact binding

The formal run is bound to clean commit `1b8586877e6e7d3703ecd1dbab1eb89b3e4eb7a2`
and Legacy-fast executable SHA-256
`41FE38FB76C0F4323DA9109B6C3616B40F423FFF12274A0010061F87A232BE6D`.
Its ignored local manifest is:

```text
artifacts/vnext-characterization/windows-276049E7945C/1b8586877e/
20260808T215535Z-0c7d18f1/manifest.json
```

Manifest SHA-256:
`9FC7656F12328C622129A202E370A2250642A09308671743E9FF4A5E5B86F5EB`.
Independent validation rehashed the executable, four committed tool inputs, 14 OPS
files, 14 canonical payloads, 28 traces, 14 result JSON files and 42 configs. It
also checked all phase result markers and all 42 nonzero RAM samples. Result:
`FORMAL_CHARACTERIZATION_VALIDATION=PASS`, `225/225` artifact files accounted for.

## Scenario results

`Saved particles` is measured after the configured generation warmup. `Final` is
the particle count after one of the two identical loaded traces.

| ID | Scene | Warmup / trace ticks | Saved particles | Final | Result |
|---|---|---:|---:|---:|---|
| C01 | sand | 30 / 60 | 44,735 | 44,735 | PASS |
| C02 | water | 30 / 60 | 49,360 | 49,360 | PASS |
| C03 | gas | 30 / 60 | 21,650 | 21,650 | PASS |
| C04 | fire | 5 / 60 | 26,845 | 26,845 | PASS |
| C05 | explosion | 0 / 40 | 25,432 | 25,432 | PASS |
| C06 | Legacy vacuum | 0 / 60 | 20,228 | 20,228 | PASS |
| C07 | pressure | 0 / 60 | 13,405 | 13,405 | PASS |
| C08 | heat | 0 / 60 | 25,978 | 25,978 | PASS |
| C09 | electrical | 0 / 60 | 17,820 | 17,820 | PASS |
| C10 | photons | 0 / 40 | 9,944 | 9,944 | PASS |
| C11 | PIPE | 10 / 60 | 13,644 | 13,665 | PASS |
| C12 | complex electronics | 0 / 60 | 19,392 | 19,392 | PASS |
| C13 | mixed | 20 / 60 | 46,358 | 45,957 | PASS |
| C14 | maximum particle | 0 / 3 | 235,008 | 227,104 | PASS |

C14 fills exactly `NPART=235008`. Its first loaded update removes the four-pixel
outer simulation border, leaving `(612-8)*(384-8)=227104` interior particles. This
is recorded Legacy boundary behavior, not hidden by changing the expected result.

Across all cases, the suite generated 569,799 saved particles, ran 125 generation
warmup ticks and recorded 743 trace ticks per restart side.

## Determinism and save boundary

| Check | Result |
|---|---:|
| Fresh client processes | 42 |
| Independent restart/load verifications | 28 |
| Cross-restart traces equal | 14/14 |
| Loaded RNG equals saved RNG | 14/14 |
| Loaded particle count equals saved count | 14/14 |
| Loaded required-element counts equal saved counts | 14/14 |
| Duplicate same-state canonical OPS payloads equal | 14/14 |
| Duplicate raw OPS containers equal | 0/14 |
| Pre-save and loaded `Snapshot::Hash` equal | 0/14 |

Raw OPS containers are not reproducible because stamp `authors.name` contains the
new stamp ID and `authors.date` changes. The canonicalizer only normalizes those two
exactly-counted volatile fields; all 14 logical payload pairs then match. Both raw
and canonical hashes remain in the private manifest.

Legacy OPS is not a bit-exact in-memory checkpoint. It reconstructs maps and
particle allocation and quantizes persistent fields, so the generated state and its
loaded state have different `Snapshot::Hash` values. The authoritative
characterization baseline is therefore the independently repeatable loaded state.
The manifest exposes `snapshot_hash_equal=false` for every case instead of treating
the mismatch as equality. The later differential runner reports generated-scene FP
field differences. The formal C01-C14 pre-save/loaded attribution now completes
this boundary comparison with loaded-A/B byte equality, while preserving the
non-bit-exact limitation; see `phase-1-load-boundary.md`.

## Correctness fixes found by the suite

Full save loading previously applied saved simulation parameters before
`Simulation::clear_sim()`. The clear reset `frameCount` and `ensureDeterminism`, and
particle `Create` callbacks during load could advance the restored RNG. Commit
`908878f74` reapplies the saved continuation state after `Simulation::Load` in both
`SetSave` and `SetSaveFile`.

Non-default edge modes had a second split-state defect: loading assigned
`Simulation::edgeMode` directly while Lua and UI read `GameModel::edgeMode`. Commit
`ce8087d07` routes loading through `GameModel::SetEdgeMode`, updating both states.
C01 first exposed the deterministic-state loss; C02 exposed the edge-mode split;
C04, which loads FIRE particles with random creation callbacks, verifies saved RNG
restoration across two fresh processes.

Two generator defects were also rejected rather than ignored: C09 and C12 originally
placed vertical INSL on occupied conductor pixels. The final geometry splits each
lane around the barrier and places BTRY within its real two-pixel activation range.

## Tests and compatibility

- Legacy-fast build: PASS; formal executable relinked `9/9` on clean source.
- Legacy-fast Meson: `39/39 PASS`.
- Strict Meson: `39/39 PASS`.
- Full Python suite: 273 run, 271 passed, 2 skipped, 0 failed.
- Later formal load-boundary Python suite: `321/321 OK`; its Legacy-fast Meson
  suite is `39/39 PASS`.
- Later clean all-tick scope run: both fresh Meson suites `39/39 PASS`; Python
  discovery is 322 run, 2 declared skips, zero failures; see
  `phase-1-all-tick-ledger.md`.
- Characterization contracts: `15/15 PASS` before the formal run.
- Lua upstream 100.1 boundary regression: PASS (`11` bounds assertions).
- Existing eight-class OPS matrix: PASS (`24` processes, `16` restart loads,
  `368` particles and `436` field assertions per load).
- Existing mixed OPS matrix: PASS (`3` processes, `2` restart loads, `11` particles
  and `21` field assertions per load).

Post-integration at `c6eecaa77`, the current-HEAD Strict Meson suite is
`39/39 PASS` and the Python suite is 281 run, 281 passed, 0 skipped, 0 failed with
the UCRT64 compiler explicitly on PATH. These later results include eight
first-divergence contracts; they do not rewrite the historical formal
characterization binding above.

The two GameModel changes affect only complete save loading. Partial paste semantics
remain unchanged. Existing stable IDs, Particle layout, OPS schema and Classic
simulation update logic are unchanged. GUI visual save/load, PSv/fuC fixtures,
portable clean-machine execution and a broad external Lua corpus remain
`not_tested`.

## Benchmark and memory

No hot-loop code or persistent production structure changed. The prior fixed-step
throughput baseline remains the before measurement; a controlled after-throughput
matrix was not rerun in this subphase and is `not_tested`. No speed claim is made.

The characterization runner measured 42 process phases. Summed phase wall time was
27.066040 seconds and summed process CPU time was 24.093750 seconds. Sampled working
set ranged from 133,210,112 to 186,638,336 bytes; peak private bytes were
187,117,568. These are whole-process values, not bytes per particle or an accepted
memory budget. Process VRAM remains `not_tested`.

## Known deviations and risks

- Complete save loads now preserve the persisted deterministic continuation state
  and synchronize edge mode; both fixes are ahead of audited official master and
  require explicit review during future upstream merges.
- Loaded OPS state is deterministic but not identical to the pre-save in-memory
  Snapshot. The field-level attribution is now complete; it reports the unequal
  fields without claiming bit-exact checkpointing or physical conservation. See
  `phase-1-load-boundary.md`.
- C14 records Legacy border culling after allocator saturation.
- All-tick exported Particle/Air float observations are finite/in Legacy range, but
  no physical conservation, positivity, full-state, energy, mass or species ledger
  exists yet; see `phase-1-all-tick-ledger.md`.
- The runner records process RAM but not subsystem time or process VRAM.

## Gate and rollback

```text
CHARACTERIZATION_DEFINITIONS=GREEN
PRIVATE_ARTIFACT_POLICY=GREEN
DETERMINISTIC_RESTART_TRACES=GREEN
BOUNDED_SAVE_LOAD=GREEN
CHARACTERIZATION_PROCESS_RAM=GREEN
LOAD_BOUNDARY_FIELD_DIFF=GREEN
ALL_TICK_POST_UPDATE_EXPORTED_FLOATS=GREEN
FIRST_DIVERGENCE_RUNNER=GREEN
SAME_SOURCE_CPU_FP_DIFFERENTIAL=GREEN
LEGACY_CPU_VS_OMNI_CPU=RED
OMNI_CPU_VS_OMNI_GPU=RED
SUBSYSTEM_PROFILER=RED
PROCESS_VRAM=RED
STRICT_FAST_NUMERICAL_COMPARISON=RED
G0_UPSTREAM_BASELINE=RED
```

Rollback base is `b9ae20bf03232cd13418064b2951e993df6c9f20`. The three bounded
commits can be reverted independently in reverse order; no rollback is recommended.
The next permitted integration work is the physical Legacy conservation/source/
correction ledger, internal/full-state finite/positivity evidence or profiler/VRAM
export, followed by G0 reassessment.
