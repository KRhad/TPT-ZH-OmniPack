# Phase 1 all-tick exported-float ledger

## Outcome

```text
REPORT_DATE=2026-08-09
BASE_COMMIT=63266565007a85f9fb3b6726bcf4de349b093051
ALL_TICK_SCOPE_IMPLEMENTATION=632665650
FORMAL_CHARACTERIZATION=GREEN
ALL_TICK_POST_UPDATE_EXPORTED_FLOATS=GREEN
LEGACY_RANGE_CONTRACT_ALL_TICKS=GREEN
UNSAMPLED_FULL_STATE_FINITE=RED
PHYSICAL_MASS_CONSERVATION=NOT_EVALUATED
PHYSICAL_MOMENTUM_CONSERVATION=NOT_EVALUATED
PHYSICAL_ENERGY_CONSERVATION=NOT_EVALUATED
SOURCE_SINK_ATTRIBUTION=NOT_EVALUATED
CORRECTION_EVENT_LEDGER=NOT_EVALUATED
PHYSICAL_PRESSURE_POSITIVITY=NOT_EVALUATED
G0_UPSTREAM_BASELINE=RED
PRODUCTION_OMNIATMOSPHERE_ALLOWED=false
```

This closes only the narrow all-tick observation sub-gate. A fixed, generated
mixed-medium Legacy scene was scanned after every one of its 1,000 full logical
updates, plus the initial state: 1,001 ordered records per CPU FP mode. It proves
that the fields exported by this tool were finite and inside the documented Legacy
range contract at each post-update boundary. It is not a physical-conservation,
source/sink, correction-event, full-state, substep or pressure-positivity claim.

No production `Simulation`, `Particle`, `Air`, Save format or Lua core file changed.
The only implementation change is a fail-closed truthfulness fix: when
`SampleInterval=1`, the comparator and manifest now state
`all_tick_post_update_exported_fields=true` and
`sampled_states_only=false`; intervals greater than one retain the sampled-only
claim. The scope contract has regression tests for both modes.

## Observed and unobserved state

At every post-update boundary, the probe scans every active Particle's stored
`x`, `y`, `vx`, `vy` and `temp`, plus every Air cell's `pv`, `vx`, `vy` and `hv`.
It also checks particle/type counts and the Legacy bounds used by the prior proxy
ledger. `Particle` has no other floating-point member in the current AoS layout.

This intentionally does **not** observe private Air scratch buffers (`opv`, `ovx`,
`ovy`, `ohv`), fan/gravity/wall auxiliary fields, portal/wireless/stickman state,
integer-payload consistency, every internal subphase, clamp/correction events or
any unimplemented physical state. In particular, negative Legacy `pv` is valid;
the Legacy range check is not pressure positivity.

## Formal evidence

The clean formal artifact is:

```text
D:/CodexWork/OmniPack/worktrees/ledger-formal-alltick-632/artifacts/
vnext-legacy-ledger-all-tick/windows-276049E7945C/6326656500/
20260809T092432Z-41615658/manifest.json
```

Manifest SHA-256:
`BD58770EEDFA52C1F0EBE0389B4930C64782B369EA90A46D6057A96F0FD2F3CC`.
The clean detached source is `63266565007a85f9fb3b6726bcf4de349b093051`.
The manifest's `worktree_state_sha256` is
`F0BE2E48EDCDDB73FF89D3E6F5D87A06D919E85FB2212FC918BA2D08F8008B0A`;
an independent post-run reconstruction of the wrapper's state material matched it
with zero status lines. The wrapper also fail-closes if that source state changes
while it runs.

Fresh Meson builds used `fp_mode=legacy_fast` and `fp_mode=strict`, respectively,
with `static=prebuilt`, `build_tests=true`, `lto=false`, `x86_sse=auto`,
`resolve_vcs_tag=no`, `debug=true` and `optimization=2`. Each app target built
`763/763` steps and exposes 754 compile commands. Their independent identities are:

| Mode | Executable bytes | SHA-256 |
|---|---:|---|
| Legacy-fast | 321,522,433 | `69B1A8956E2D33D76E2364D38472F3E322E225CA46262D2F9147CF683ED795A4` |
| Strict | 321,542,204 | `CF3EF0F29B9658B00906724F4B3E98B4B6D83E37466D2D4A057203A7D4266CE0` |

Both fresh Meson suites passed `39/39`. The current nested Python discovery suite
passed `322` tests with `2` declared skips and zero failures; the focused ledger
suite passed `22/22`.

The artifact has 18 declared non-manifest files and 18 actual non-manifest files,
plus `manifest.json`; it has zero nested directories, missing files, extra files,
length mismatches, SHA-256 mismatches or `__pycache__` directories. The frozen
comparator was replayed with Python `-B` against the retained CSVs. Its output was
byte-identical to `ledger-comparison.json`: 21,875 bytes, SHA-256
`4B8D9765AFBA5D183CA327FDF99D418BBAED340E75BB5A7A8E6CBBF70BEFE77B`.

## Numerical observations

| Observation | Legacy-fast | Strict |
|---|---:|---:|
| Logical updates | 1,000 | 1,000 |
| Post-update records including step 0 | 1,001 | 1,001 |
| Exported non-finite observations | 0 | 0 |
| Legacy-range violations | 0 | 0 |
| Exact-bound occupancy observations | 0 | 0 |
| First state-hash divergence | step 1 | step 1 |
| First metadata divergence | step 34 | step 34 |
| First proxy-metric divergence | step 1 | step 1 |

The divergence rows reaffirm the existing fast-math risk; they do not establish
which trajectory is physically correct. The all-tick scan added collection work:
wall time was 47.501361 seconds for Legacy-fast and 47.919276 seconds for Strict,
with sampled working-set maxima of 139,460,608 and 138,895,360 bytes. These are
instrumentation costs for one controlled run, not throughput, memory-budget or GPU
benchmarks.

## Gate and rollback

```text
ALL_TICK_POST_UPDATE_EXPORTED_FLOATS=GREEN
LEGACY_RANGE_CONTRACT_ALL_TICKS=GREEN
UNSAMPLED_FULL_STATE_FINITE=RED
PHYSICAL_MASS_MOMENTUM_ENERGY_LEDGER=RED
SOURCE_SINK_ATTRIBUTION=RED
CORRECTION_EVENT_LEDGER=RED
PHYSICAL_PRESSURE_POSITIVITY=RED
SUBSYSTEM_PROFILER=RED
PROCESS_VRAM=RED
PERFORMANCE_REGRESSION_BUDGET=RED
G0_UPSTREAM_BASELINE=RED
```

Rollback is `git revert 632665650`. It removes only the comparator/wrapper scope
metadata and its 22 contract tests; no simulator behavior, save format, Lua API or
user data changes. The next permitted work is source/sink/correction-aware Legacy
instrumentation plus internal/full-state boundaries, or subsystem profiler and
process-VRAM export. Production OmniAtmosphere remains blocked.
