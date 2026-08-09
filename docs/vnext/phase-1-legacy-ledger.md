# Phase 1 Legacy sampled numerical proxy ledger

## Outcome

```text
REPORT_DATE=2026-08-09
BASE_COMMIT=4765fc12320fa0f6ef96f63ba694cca04c4e314d
IMPLEMENTATION_COMMIT=8bd640c3e2aca501a17987aa462b2489901e6694
BYTE_STABLE_REPLAY_FIX=b3aa56cf3914ab18da60d1ac9ff1e492377f6e88
FORMAL_RUN_COMMIT=b3aa56cf3914ab18da60d1ac9ff1e492377f6e88
REPORT_COMMIT=SELF
LEGACY_SAMPLED_EXPORTED_FINITE_LEDGER=GREEN
LEGACY_SAMPLED_RANGE_CONTRACT=GREEN
SAME_SOURCE_CPU_FP_PROXY_COMPARISON=GREEN
PHYSICAL_MASS_CONSERVATION=NOT_EVALUATED
PHYSICAL_MOMENTUM_CONSERVATION=NOT_EVALUATED
PHYSICAL_ENERGY_CONSERVATION=NOT_EVALUATED
SOURCE_SINK_ATTRIBUTION=NOT_EVALUATED
CORRECTION_EVENT_LEDGER=NOT_EVALUATED
UNSAMPLED_TICK_FINITE_STATE=NOT_TESTED
PHYSICAL_PRESSURE_POSITIVITY=NOT_EVALUATED
CONSERVATION_POSITIVITY_FINITE_LEDGER=RED
G0_UPSTREAM_BASELINE=RED
PRODUCTION_OMNIATMOSPHERE_ALLOWED=false
```

> Historical status annotation (2026-08-09): `LOAD_BOUNDARY_FIELD_DIFF=RED` in
> this report describes the state at its report commit. The later
> [OPS load-boundary field-attribution report](phase-1-load-boundary.md) closes
> that independent sub-gate at `GREEN` with a clean `9b336fc40` artifact. This
> does not change this report's sampled-proxy findings or any of its remaining
> physical-ledger `RED` gates.

> Historical status annotation (2026-08-09): this report's
> `UNSAMPLED_TICK_FINITE_STATE=NOT_TESTED` remains accurate for its 102-sample
> artifact. The later [all-tick exported-float report](phase-1-all-tick-ledger.md)
> uses the same fail-closed ledger at `SampleInterval=1` and closes only that later
> post-update exported-field sub-gate. It does not change this report's physical,
> source/sink, correction or full-state limitations.

The sampled Legacy proxy-ledger foundation is **GREEN**. On one clean-source,
1,000-step mixed run, both Legacy-fast and Strict builds exported finite Particle
and Air values at all 102 scheduled samples and stayed inside the documented Legacy
range contract. The retained CSV evidence can be compared again with the frozen
comparator to produce byte-identical JSON.

This is deliberately not called a physical conservation ledger. Legacy Particle
records have no authoritative parcel mass, `pv` is not conserved gas mass,
temperature sums are not energy, and velocity sums are not momentum. The tool does
not attribute create/delete/type-change sources, observe internal clamp events, or
inspect every tick. G0 therefore remains **RED**.

## Goal and implementation

The implementation adds four tooling/test files and changes no production
simulation behavior:

- `tools/runtime/legacy_numerical_ledger.lua` scans every active Particle's
  `type/x/y/vx/vy/temp` and every Air cell's `pv/vx/vy/hv` at fixed samples;
- `tools/legacy_ledger_compare.py` validates the exact CSV schemas and cross-field
  invariants, compares the two FP modes and emits deterministic JSON;
- `tools/runtime_legacy_numerical_ledger.ps1` builds and binds both Meson targets,
  isolates client profiles, freezes tools, restricts child environments, records
  execution provenance and retains private evidence; and
- `tools/tests/test_legacy_numerical_ledger.py` supplies 21 parser, schema,
  semantics, provenance and platform-newline contract tests.

Particle property counts use six record classes: powder, liquid, solid, gas,
energy and unclassified. The last class is required because valid local elements
such as `STKM`, `STKM2` and `FIGH` have no `TYPE_*` bit. No Element weight is used as
mass. Compensated sums reduce aggregation-order loss but do not turn the sums into
physical conserved quantities.

The comparator rejects malformed schedules, extra or short CSV rows, non-finite
aggregates, impossible count combinations, invalid uint32/type data, negative
squared-speed sums, inverted or impossible extrema, inconsistent type totals and
derived JSON overflow. Divergence fields are named `first_sampled_*` because steps
between scheduled samples are not observed.

## Ledger semantics

| Export | Actual unit/meaning | Explicitly not claimed |
|---|---|---|
| Particle count | active records | condensed mass |
| Property-class histogram | records by Legacy `TYPE_*` bit or unclassified | mass by phase |
| Type histogram | records by Element ID | species/atom inventory |
| Particle `x/y/vx/vy/temp` | stored Legacy floats | position/momentum/energy in physical units |
| Air `pv/vx/vy/hv` | Legacy pressure-like, velocity-like and Kelvin-like fields | gas mass, conserved momentum or total energy |
| Proxy sums | compensated diagnostic aggregates | conservation laws |
| Bound occupancy | sampled values exactly at Legacy limits | internal clamp-event counts |
| `sim.hash()` | existing 32-bit FNV state signature | cryptographic content digest |

The manifest and both probe results set all of these to `false`:

```text
physical_mass_conservation_evaluated
physical_momentum_conservation_evaluated
physical_energy_conservation_evaluated
source_sink_attribution_evaluated
correction_events_evaluated
```

## Formal artifact and provenance

The final clean run is:

```text
artifacts/vnext-legacy-ledger/windows-276049E7945C/b3aa56cf39/
20260809T044806Z-1f154dee/manifest.json
```

Manifest SHA-256:
`5F57C3F57E4B80D01F8C94FE8A8EC0CB3D496EE95B8D9309A5E6DAFAD743390D`.
It was generated from the clean detached worktree
`D:/CodexWork/OmniPack/worktrees/ledger-formal-b3aa` at `b3aa56cf3`; the source
manifest records `dirty=false`. The directory contains exactly the manifest plus 18
declared evidence files: 19 files, zero subdirectories. Independent length and
SHA-256 verification passed `18/18`, with no missing or extra path, both before and
after replay.

| Frozen input | SHA-256 |
|---|---|
| Lua ledger | `8078CCFDB99BD95FD465F26E2EFCF9C4F0CADA0743563B49C95DDAC6E289A290` |
| Python comparator | `BB1E262FEC1F7D8F68FA0626047A505465E15D152A1149773A145B2CB3565042` |
| PowerShell wrapper | `BF3F6ED799C19190B0710B226581CAA3A39FFFCA873C5D9336CBD5C5E9B33BBC` |

These are hashes of the physical Windows-checkout bytes that were frozen and
executed, including checkout line endings; the corresponding Git blobs remain bound
to commit `b3aa56cf3`.

| Mode | Executable bytes | SHA-256 | Build-provenance SHA-256 |
|---|---:|---|---|
| Legacy-fast | 321,484,028 | `8828592F758563B9D025064CA25F441A6BD267EF627ABFD920EEBEB18037DC85` | `5C5292D4DB609DAFC9971EF6C15ED8B0CB0ABA1E497ABE88B829B649D83A4F13` |
| Strict | 321,504,311 | `319C0AB8D0CCFEAB3727BF9FA827E82B5F1493AC8EE948B3272F24706C819145` | `50998720D932F4A956C47A7EAAA6371F6B36A4915CD1DA70A4AFD189FB3CFAAD` |

Before the probes, Ninja actually built both targets because four generated-data
targets are intentionally `build_always_stale`. All 754 compile commands per build
were checked for their expected FP contract; after removing only the allowed FP
flags, the Legacy-fast and Strict commands matched `754/754`. Meson source root,
compiler and all other options also matched. Executables and build provenance were
re-hashed after execution.

The comparator ran with Python 3.14.6, executable SHA-256
`09162FAF445D60FD856490A9ED3860BD6A3B14FA13612B310D0E6D08B0655EDF`.
Ninja SHA-256 was
`6DB3D9E4443835FE40D4B1425A0A13012AE810C449F74BA5CB059E6CA16DF4F0`.
The selected UCRT64 directory contained 44 DLLs; every name/length/hash passed an
independent check and its recorded inventory digest is
`0D0398A26774F746DE129DA8E58B2D67784D53DF2CBBA015F9F637227BC64BE9`.

The build metadata does not cryptographically embed the Git commit, so the manifest
states `source_commit_embedded_in_executable=not_verified`. Python standard-library
and extension-module files are not fully captured, and direct loaded-DLL attribution
is `not_evaluated`. These are declared reproducibility boundaries, not silent proof.

## Numerical observations

The scenario used seed `(101, 202, 303, 404)`, 59,084 generated particles and
14,688 Air cells. Samples are steps `0`, `1`, every tenth step, and step `1000`, for
102 samples total.

| Observation | Legacy-fast | Strict |
|---|---:|---:|
| Initial state hash | 3,129,219,535 | identical |
| Step-1 state hash | 2,089,865,305 | 1,195,260,457 |
| First sampled proxy-metric divergence | step 1 | step 1 |
| First sampled RNG divergence | step 40 | step 40 |
| First sampled type-histogram divergence | step 40 | step 40 |
| First sampled particle-count divergence | step 90: 58,696 | step 90: 58,868 |
| Final particles | 50,471 | 50,477 |
| Final state hash | 254,804,381 | 2,274,235,009 |

At step 1, 11 proxy fields differ: Particle position/velocity/speed-squared sums,
Air velocity/speed-squared/ambient-heat sums, and two velocity extrema. Type counts
are still equal. At sampled step 40, the RNG words and nine type counts differ while
the total particle count remains equal. This is a chaotic Legacy trajectory split,
not evidence that either FP mode is physically correct.

Across all 102 samples on both sides:

```text
nonfinite_observations=0
range_violation_observations=0
bound_occupancy_observations=0
finite_exported_state=true
range_contract_pass=true
```

This means the exported floats were finite and inside the selected Legacy bounds at
the sampled states. It does not prove unsampled ticks, unexported fields, physical
pressure positivity, or absence of an internal clamp between samples.

Endpoint proxy drift is large because the scene contains movement, reactions,
creation/deletion and open boundaries. For example, Particle temperature sum changes
from 18,465,658.32 to 16,890,295.05 in Legacy-fast and 16,916,893.95 in Strict;
Air ambient-heat sum changes from 4,389,163.11 to 5,536,347.98 and 5,528,617.43.
These values characterize behavior only and are not energy measurements.

## Independent replay and tests

With `PYTHONDONTWRITEBYTECODE=1` and Python `-B`, the frozen comparator was loaded
directly from the artifact and rerun in memory against the four retained CSV files.
Its generated JSON was byte-identical to
`ledger-comparison.json`: 13,848 bytes, SHA-256
`54D3B2BB7B00F21B9D8C1A20888B8511FB8CEF75B60A2728A19328837379FBC1`.
The replay wrote no output into the artifact and created no `__pycache__`.

An earlier clean run at `8bd640c3e` was retained but superseded: semantic replay was
equal, while Windows text-mode CRLF prevented byte equality. Commit `b3aa56cf3`
forces LF output and adds a regression test. The first `b3aa56cf3` formal directory,
`20260809T034504Z-7c909590`, is also retained but superseded: its 18 declared files
still pass their hashes, while a later independent import added an undeclared
`__pycache__/ledger-comparator.cpython-314.pyc`, so that old directory is not a
closed formal artifact. No old artifact or generated file was deleted or relabeled.

- Ledger-specific Python tests: `21/21 PASS`.
- Full Python suite: `302/302 PASS`.
- Legacy-fast Meson suite: `39/39 PASS`.
- Strict Meson suite: `39/39 PASS`.
- Final-commit empty Windows PowerShell smoke: `PASS`, no sampled divergence,
  manifest SHA-256
  `2F5B782C198118E1F6F20D7D451B860C3332AC87EC1406D45DE6AA39E1420407`.
- Empty frozen-comparator byte replay: `PASS`, comparison SHA-256
  `D41E9108D49D8F89721E98DC9325F231C62887E2E3590EBECA14F52F8869A111`.
- EXE/build-directory mismatch negative test: `PASS`.
- Independent read-only reviews: Lua `PASS`, Python comparator/schema `PASS`, and
  wrapper/provenance code `PASS_WITH_DECLARED_LIMITS`; the later closure review
  correctly rejected the superseded artifact, then returned `ACCEPT_REPORT` after
  independently validating this replacement.

These tests do not constitute visible GUI, portable-runtime, long-run, physical-
correctness or performance acceptance.

## Benchmark and memory boundary

No production hot loop or persistent state changed, so the fixed-step throughput
baseline has no before/after change. The ledger deliberately scans state and must
not be used as a simulator speed benchmark. Its manifest sets
`performance_gate=not_evaluated`.

| Probe | Wall seconds | CPU seconds | Max sampled working set | Max sampled private bytes |
|---|---:|---:|---:|---:|
| Legacy-fast | 10.646510 | 10.250000 | 138,743,808 | 137,580,544 |
| Strict | 10.519088 | 10.234375 | 138,739,712 | 137,601,024 |

Memory was polled every 50 ms, so these are maximum sampled process values, not OS-
guaranteed peaks. Process VRAM remains `not_tested`. No bytes per Particle or Air
cell were added to production.

## Compatibility and risks

The commits do not modify `Simulation`, `Particle`, Air, Save, Lua core, Element IDs,
update order or persistent formats. They use existing read-only Lua getters in
isolated temporary profiles and write only ignored private artifacts. The CPU
reference backend remains unchanged.

Unobserved domains include Particle integer payload fields, wall/fan/gravity/portal/
wireless/stickman auxiliary state, physical condensed/gas mass, source/sink causes
and internal correction events. Future Elements with multiple `TYPE_*` bits require
re-auditing the record-class equality contract. Ninja target builds have a 600-second
cap and are intended only for explicit isolated build directories.

## Gate and rollback

```text
LEDGER_SCHEMA_AND_FAIL_CLOSED_PARSER=GREEN
FORMAL_ARTIFACT_INTEGRITY=GREEN
BYTE_IDENTICAL_COMPARATOR_REPLAY=GREEN
LEGACY_SAMPLED_EXPORTED_FINITE_LEDGER=GREEN
LEGACY_SAMPLED_RANGE_CONTRACT=GREEN
SAME_SOURCE_CPU_FP_PROXY_COMPARISON=GREEN
UNSAMPLED_TICK_OR_FULL_STATE_FINITE=RED
PHYSICAL_MASS_MOMENTUM_ENERGY_LEDGER=RED
SOURCE_SINK_ATTRIBUTION=RED
CORRECTION_EVENT_LEDGER=RED
PHYSICAL_PRESSURE_POSITIVITY=RED
LOAD_BOUNDARY_FIELD_DIFF=RED
SUBSYSTEM_PROFILER=RED
PROCESS_VRAM=RED
PERFORMANCE_REGRESSION_BUDGET=RED
G0_UPSTREAM_BASELINE=RED
```

The rollback point is `4765fc123`. Reverting `b3aa56cf3` and `8bd640c3e` removes
only the private ledger tools/tests; no save or data migration is required. No
rollback is recommended.

The current permitted work is a real source/sink/correction-aware conservation
ledger with internal/full-state boundaries, or subsystem profiler plus process-VRAM
export. Production OmniAtmosphere remains blocked.
