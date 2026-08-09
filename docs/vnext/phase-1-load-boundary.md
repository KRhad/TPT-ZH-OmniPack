# Phase 1 OPS load-boundary field attribution

## Outcome

```text
REPORT_DATE=2026-08-09
BASE_COMMIT=c386edbbe2673e6f1d9f19c7000328b274bcebad
INTEGRATION_IMPLEMENTATION=971687a24
INTEGRATION_CLOSURE=a09c6d716
FORMAL_SOURCE_COMMIT=9b336fc40e3dcca1470fd672796fe325bec472a9
FORMAL_CHARACTERIZATION=GREEN
LOAD_BOUNDARY_CAPTURE=GREEN
LOAD_BOUNDARY_FIELD_DIFF=GREEN
PHYSICAL_MASS_CONSERVATION=NOT_EVALUATED
PHYSICAL_MOMENTUM_CONSERVATION=NOT_EVALUATED
PHYSICAL_ENERGY_CONSERVATION=NOT_EVALUATED
SOURCE_SINK_ATTRIBUTION=NOT_EVALUATED
BIT_EXACT_CHECKPOINT=FALSE
G0_UPSTREAM_BASELINE=RED
PRODUCTION_OMNIATMOSPHERE_ALLOWED=false
```

This closes the remaining OPS load-boundary characterization sub-gate. It is a
field-attribution result, not an equality, conservation or physical-correctness
claim. Across all 14 cases, fresh loaded-A and loaded-B captures are byte-identical
and the comparator replay is byte-identical to every retained comparison JSON.
The pre-save and loaded Snapshot hashes remain unequal in all 14 cases, as expected
for OPS normalization/quantization and derived-state reconstruction.

No production `Simulation`, `Particle`, `Air`, Save format or Lua core file changed.
The changes are isolated to the characterization Lua exporter, comparator, runner
and tests. All saves, captures, logs and manifests remain ignored private artifacts.

## Matching and scope

The exporter writes three explicit CSV domains on both sides of the boundary:

- Particle payload fields are paired by `save_ordinal`. The ordinal follows the
  OPS rounded-pixel `(y, x)` order and pre-save runtime-ID order; runtime IDs after
  loading are diagnostic only and are never treated as persistent identity.
- Air fields are paired by cell coordinate and include pressure, velocity, ambient
  heat, wall/electrical maps, fan fields and gravity fields.
- Simulation settings are paired by stable setting name and include pause,
  determinism, edge/gravity/air/convection, heat, water equalization, ambient-air,
  edge pressure and edge velocity.

The parser rejects wrong schemas, extra columns, duplicate IDs/cells/settings,
non-sequential ordinals, non-finite floats and out-of-range integer payloads. A
comparison with field differences still returns `PASS` only to mean “complete and
attributable”; the JSON reports every difference and explicitly sets all physical
accounting and bit-exact claims to false.

## Formal evidence

The clean formal artifact is:

```text
artifacts/vnext-load-boundary-ledger/windows-276049E7945C/9b336fc40e/
20260809T073655Z-d678510e/manifest.json
```

Manifest SHA-256 is
`F47F4119420A42F1B8ED83583B5B551B84E2CFBFCBC0FCD59024C3400468DC6C`.
The source worktree was clean at `9b336fc40`; the independently configured
legacy-fast build was `D:/CodexWork/OmniPack/builds/ledger-9b336-legacy`, with
`static=prebuilt`, `build_tests=true`, `lto=false`, `x86_sse=auto`,
`resolve_vcs_tag=no` and `fp_mode=legacy_fast`. The executable is 321,491,196
bytes with SHA-256
`AD4BE609CB25A58943F858FE39EACE2D2481DD242F4DB5C6E1B0A8321152126B`.

The formal run passed `39/39` Meson tests and the nested Python suite reported
`321/321 OK`. The artifact manifest declares 369 non-manifest files; independent
closure found 369 declared/actual files plus `manifest.json`, 15 expected
directories, zero missing/extra files, zero hash/length mismatches, zero frozen
input mismatches and zero `__pycache__` files. The frozen comparator replayed all
14 cases in memory with `PYTHONDONTWRITEBYTECODE=1` and Python `-B`; all 14 JSON
outputs were byte-identical.

### Superseded pre-closure artifact

The retained earlier candidate at

```text
D:/CodexWork/OmniPack/worktrees/ledger-formal-f740/artifacts/
vnext-load-boundary-ledger/windows-276049E7945C/f740bf435e/
20260809T070914Z-65f69cd3/manifest.json
```

reports `status=PASS` and its 14 cases passed, but its manifest has no recursive
`artifact_files` inventory, non-manifest file count or directory count. It cannot
therefore prove a closed artifact boundary and is explicitly **superseded** by the
`9b336fc40` artifact above. It is retained unchanged as historical evidence; no
artifact was deleted, relabeled or used to satisfy this gate.

## Case results

`particle_diff` is the number of save-ordinal records with payload or rounded-pixel
differences. `runtime_ids_changed` is diagnostic only. `cells_diff` and
`settings_diff` count field-attributed records; none are physical conservation
measurements.

| Case | Scene | Particles before/loaded | particle_diff | runtime_ids_changed | cells_diff | settings_diff | Snapshot hash equal |
|---|---|---:|---:|---:|---:|---:|---|
| C01 | sand | 44,735 / 44,735 | 37,791 | 42,443 | 14,688 | 1 | false |
| C02 | water | 49,360 / 49,360 | 44,860 | 47,065 | 14,688 | 1 | false |
| C03 | gas | 21,650 / 21,650 | 21,644 | 19,358 | 14,688 | 1 | false |
| C04 | fire | 26,845 / 26,845 | 14,693 | 24,553 | 14,688 | 1 | false |
| C05 | explosion | 25,432 / 25,432 | 96 | 23,140 | 14,688 | 1 | false |
| C06 | vacuum | 20,228 / 20,228 | 0 | 17,932 | 14,688 | 1 | false |
| C07 | pressure | 13,405 / 13,405 | 0 | 11,113 | 14,688 | 1 | false |
| C08 | heat | 25,978 / 25,978 | 2,091 | 23,686 | 6,988 | 1 | false |
| C09 | electrical | 17,820 / 17,820 | 0 | 15,528 | 14,688 | 1 | false |
| C10 | photons | 9,944 / 9,944 | 992 | 7,652 | 14,688 | 1 | false |
| C11 | PIPE | 13,644 / 13,644 | 0 | 11,352 | 14,688 | 1 | false |
| C12 | complex electronics | 19,392 / 19,392 | 224 | 17,100 | 14,688 | 1 | false |
| C13 | mixed | 46,358 / 46,358 | 43,965 | 44,063 | 14,688 | 1 | false |
| C14 | maximum particle | 235,008 / 235,008 | 0 | 0 | 0 | 1 | false |

The C14 run exercised the full `NPART=235008` allocator. It is the maximum-load
closure check, not a throughput benchmark. The one repeated setting difference is
reported in every case and is not silently normalized away.

## Compatibility and limits

Windows PowerShell 5 was used for the formal wrapper; its missing `Get-FileHash`,
`ArgumentList` and compatible `Environment` APIs are handled by local byte-stream
hashing, quoted `Arguments` fallback and `EnvironmentVariables` dictionary access.
The clean source/build binding and post-run rehash checks passed.

This sub-gate does not provide physical mass, momentum, energy, source/sink,
correction-event, unsampled-tick, pressure-positivity, GUI, process-VRAM or
performance-budget evidence. `EMPTY`/OPS behavior is still Legacy behavior, not a
real gas model. G0 therefore remains RED and production OmniAtmosphere work stays
blocked.

## Gate and rollback

```text
LOAD_BOUNDARY_SCHEMA_AND_FAIL_CLOSED=GREEN
LOAD_BOUNDARY_FORMAL_ARTIFACT=GREEN
LOAD_BOUNDARY_BYTE_REPLAY=GREEN
LOAD_BOUNDARY_FIELD_DIFF=GREEN
PHYSICAL_CONSERVATION_LEDGER=RED
UNSAMPLED_FULL_STATE_FINITE_POSITIVITY=RED
SUBSYSTEM_PROFILER=RED
PROCESS_VRAM=RED
PERFORMANCE_REGRESSION_BUDGET=RED
G0_UPSTREAM_BASELINE=RED
```

The tooling implementation rolls back to `c386edbbe` by reverting
`a09c6d716` and `971687a24` in reverse order. No save migration or production
rollback is required.
