# OmniCore data contract v1

This directory is the offline-only 1.0.4 data foundation. Nothing in `src/`
loads these files, and no simulation, save, Lua, Element, or UI behavior depends on
them in this version.

Files:

- `schema.json` defines `MaterialDefinition`, `SpeciesDefinition`,
  `ReactionDefinition`, scalar physical properties, complete provenance, and the
  identity-only Legacy mapping document.
- `unit-registry.schema.json` defines the canonical-unit registry format.
- `units.json` contains a deliberately small canonical-SI vocabulary. It defines
  unit dimensions only; it does not select pixel length, effective depth, particle
  mass, simulation timestep, or any other 1.0.5 PhysicalScale value.
- `catalog.json` is intentionally empty. It proves the document contract without
  bundling guessed physical properties, kinetics, or restricted reference data.
- `legacy-material-map.json` is generated from `docs/ELEMENT_REGISTRY.csv`. Its
  records are identity-only compatibility mappings and explicitly carry no SI
  physical-property claim. Its source hash is the canonical parsed CSV serialized
  as UTF-8/LF after normalizing every field's internal newline to LF, so CRLF
  checkout policy cannot make a clean clone appear stale.

`tools/omnicore_data_check.py` validates all documents with the Python standard
library. It rejects duplicate JSON keys, NaN/Inf, unknown fields, unsupported
versions, noncanonical units, incomplete property provenance, invalid ranges,
unknown references, Legacy ID mismatches, material-composition errors, and atom or
charge imbalance. Stoichiometric coefficients and composition fractions are exact
positive rational strings such as `2` or `1/2`, avoiding floating-point balance
tests.

Every bundled numeric physical property must contain its own source title, source
version, source date, access date, locator, reviewed `source_kind`, method,
confidence, redistribution status, license/terms and evidence, tuning status, and
temperature/pressure validity domains. Placeholder and AI-generated provenance are
rejected. Data marked `reference_only` or `permission_required` is rejected when a
numeric value or reaction definition is bundled. Synthetic values are accepted only
in `test.*` catalogs and are never part of the repository catalog.

Legacy `Element` fields and the existing CSV registries remain gameplay and
compatibility data. In particular, Legacy `HeatCapacity`, `HeatConduct`, transition
thresholds, and `pv` values are not imported or relabeled as SI properties.

`identity_only` has a strict meaning: if it carries a Legacy mapping, the mapping
relationship must be `identity`. Proxy/tool/visualization relationships are reserved
for later non-identity records and cannot silently claim physical identity.
