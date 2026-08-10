# 1.0.4 OmniMaterials data-foundation checkpoint

## Outcome

```text
TARGET_VERSION=1.0.4
BASE_COMMIT=477372373cb1be30c3c04bca4309a7d6fd9fa799
BRANCH=integration/omnicore-vnext
STATUS=VALIDATED_PENDING_CLEAN_CHECKPOINT
SCOPE=Material/Species/Reaction schemas, canonical units, property provenance, fail-closed validation
PRODUCTION_SIMULATION_CHANGE=false
ELEMENT_ID_CHANGE=false
LUA_IDENTIFIER_CHANGE=false
SAVE_FORMAT_CHANGE=false
RUNTIME_DATA_CONSUMER=false
CATALOG_RECORDS=0_MATERIALS_0_SPECIES_0_REACTIONS
CANONICAL_QUANTITY_KINDS=23
CANONICAL_UNITS=23
LEGACY_IDENTITY_MAPPINGS=488_487_CANONICAL_1_ALIAS
LEGACY_MAP_SOURCE_HASH_CONTRACT=canonical_csv_utf8_lf_v1
LEGACY_MAP_SOURCE_SHA256=E7BCE2976E7F1F6243899049DFBD8DE06CF0CBACB8AD7FAF9168FF888CE93962
PYTHON_DISCOVERY=GREEN_383_PASS_2_SKIPPED
MESON_STATIC=GREEN_41_41
BUILD=GREEN_789_789
LUA_UPSTREAM=GREEN
LUA_MODULE=GREEN
OPS_SAVE_LOAD=GREEN_8_SCENARIOS_24_PROCESSES_16_RESTARTS_16_LOADS
BENCHMARK=RECORDED_DIRTY_SOURCE_NOT_A_PERFORMANCE_GATE
INDEPENDENT_REVIEW=2_READ_ONLY_REVIEWS_CHANGES_REQUIRED_RESOLVED
V1_0_4_GATE=READY_FOR_CLEAN_CHECKPOINT
```

## Entry decision

The 1.0.3 gate remains YELLOW because real Windows 100%, 125%, and 150% DPI
validation is not available. After that was reported, the user explicitly directed
the orchestrator to enter the next version. This is a scoped
`USER_ACCEPTED_YELLOW / DPI_DEFERRED` exception, not a GREEN claim.

The official Git remote was refreshed at phase entry. `official/master` and the
peeled `v100.1.400` tag both resolve to
`d768aeb89acad986bd252d7e904bf44bb374545f`; there is no new upstream delta.
Official web-page refresh returned an external `503 auth_not_found`, so web status
remains `ONLINE_WEB_REFRESH=EXTERNAL_BLOCKED` while Git evidence is GREEN.

## Scope contract

This phase may add versioned machine-readable contracts, canonical unit metadata,
property-level provenance requirements, deterministic validators, negative tests,
and documentation. Existing gameplay CSV values remain Legacy/game-tuned metadata
and are never relabeled as SI physical properties.

This phase must not connect the new records to the production simulation or change
Air, Particle, Simulation, GameSave, Lua core, material behavior, physical scale,
Atmosphere, chemistry runtime, SDL, GPU, or CUDA code.

## Implementation

`resources/omnicore/v1/` introduces an offline-only data contract:

- `schema.json` contains strict `MaterialDefinition`, `SpeciesDefinition`,
  `ReactionDefinition`, property, provenance, and Legacy-map schemas.
- `unit-registry.schema.json` and `units.json` define a validator-owned, small
  canonical SI vocabulary: 23 quantity kinds and 23 canonical units. It does not
  define pixel length, depth, parcel mass, `dt`, or a PhysicalScale runtime mapping.
- `catalog.json` has zero material/species/reaction records by design. It proves the
  schema without bundling guessed numbers, kinetic constants, or third-party data.
- `legacy-material-map.json` contains 488 generated identity-only mappings (487
  canonical records and one compatibility alias). It imports no physical property.
  Its source hash is a canonical parsed CSV serialized as UTF-8/LF, so a CRLF/LF
  checkout cannot make a clean clone look stale.

`tools/omnicore_data_check.py` is a Python-standard-library checker and explicit
map refresher. It uses atomic replacement only after a complete preflight. The
validator rejects duplicate keys, BOM/invalid UTF-8, NaN/Infinity, unknown or
missing fields, schema semantic drift, unit drift, unsafe local license evidence,
placeholders and AI-generated provenance, non-redistributable numeric/reaction
records, malformed validity ranges, Legacy identifier/ID mismatches, unresolved
composition, noncanonical fractions/integers, and atom/charge/phase-invalid
reactions. No C++ generator or runtime JSON loader is added.

The existing `COMPOUND_REGISTRY.csv` had two malformed CSV records: the formulas
`(Fe,Zn)3O4` and `Pb(Zr,Ti)O3` contained unquoted commas. They are now quoted and a
strict-width regression test prevents their fields from being silently shifted.

## Verification

- Targeted OmniCore contract suite: **40/40 PASS**.
- Full Python discovery: **381 PASS**, with **2 pre-existing declared skips**.
- JSON Schema Draft 2020-12 meta-validation and validation of catalog, units, and
  Legacy map: **PASS** (independent optional environment check; no dependency added).
- Clean isolated `debugoptimized`, `static=prebuilt`, `legacy_fast` build:
  **789/789 PASS**. GCC emitted the pre-existing `PowderToy.cpp`
  `maybe-uninitialized` warning; this phase changes no `src/` file.
- Meson static suite: **41/41 PASS**, including the new
  `omnicore-data-foundation` gate.
- Runtime: upstream Lua smoke and module Lua smoke both **PASS**; eight OPS
  save/load scenarios pass with 24 fresh processes, 16 restarts, and 16 load
  verifications.
- Boundary audit: `git diff -- src` is empty and a contract test confirms no
  production source references the foundation files.

The first independent validation audit found line-ending, nested-schema, provenance,
reaction, refresh-order, package-membership, negative-zero, and canonical-integer
gaps. A second independent review additionally found field-internal newline
normalization, complete source-package member enforcement, and identity-only/proxy
semantic gaps. Every finding is now covered by an adversarial test. The audited
result is `2_READ_ONLY_REVIEWS_CHANGES_REQUIRED_RESOLVED`; neither review is
misreported as an unconditional approval.

## Benchmark and memory

The existing fixed-step client runner recorded this pre-checkpoint, dirty-source
execution (the changes are data/tools only):

| Scene | Steps/s | ms/step | Steps | Final hash |
|---|---:|---:|---:|---:|
| empty | 1476.078458 | 0.677471 | 600 | 3182293864 |
| mixed-medium | 176.608632 | 5.662237 | 600 | 798083730 |

Its executable SHA-256 is
`A588112D58F5762C77CE88FC2BA117D3E4994C96069090E65B4F1A80A46B48A4`.
The runner explicitly reports `performance_gate=not_evaluated`; these observations
are not a speed claim or a regression attribution, especially because no simulation
source or runtime path changed. A clean-source repeat is required after the
checkpoint.

The five JSON files total 121,325 bytes. Runtime RAM/VRAM delta is exactly zero for
this phase because the catalog has no production consumer. A 20-run in-process
validator measurement averaged 12.563 ms; it is offline tooling, not simulation
frame cost.

## Compatibility and remaining scope

Element IDs, Lua identifiers, saves, renderer/UI behavior, Legacy Air, Particle,
Simulation, and chemistry gameplay rules are unchanged. Existing gameplay CSVs are
not promoted to SI values. The source-release allowlist now includes the checker so
the public source package does not contain a contract without its validator.

This phase deliberately does **not** select `PhysicalScale`, `dt`, gas EOS,
Atmosphere solver, runtime material values, mechanism data, or a C++/GPU backend.
Those remain later-version work.

## Gate checklist

- [x] Schema/version validation is fail-closed, including complete semantic hashes.
- [x] Canonical units and quantity dimensions are validator-owned and validated.
- [x] Every physical property requires complete provenance and validity ranges.
- [x] Chemistry-enabled species require elemental composition and integer charge.
- [x] Reactions reject unknown species, phase mismatch, atom imbalance, and charge imbalance.
- [x] Legacy mappings reject unknown identifiers or stable-ID mismatches.
- [x] No unreviewed real-world property, kinetics value, or restricted source data is bundled.
- [x] Python discovery and Meson static suite pass.
- [x] Production-boundary audit confirms no runtime consumer or physics change.
- [x] Independent review is recorded; its required corrections are covered by tests.

The phase is ready for a clean commit, clean-source rerun, actual source-archive
manifest verification, and final gate update.
