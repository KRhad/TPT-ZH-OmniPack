# OmniCore data contract v1

This directory began as the offline-only 1.0.4 data foundation. Version 1.0.5 adds
explicitly unselected PhysicalScale and Atmosphere policy benchmark-input documents. Nothing in
`src/` loads these files, and no simulation, save, Lua, Element, or UI behavior
depends on them.

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
- `physical-scale-candidates.json` freezes the Legacy geometry fingerprint and
  carries only benchmark candidates for pixel length, atmosphere-cell length,
  effective depth and time policies. Its `selection_status`, every time policy and
  the Atmosphere solver remain `unselected`; none is a production default.
- `atmosphere-policy-candidates.json` binds the measured strict-double HLLC
  checkpoint to an accepted Phase 5 reference-machine CPU/memory target and a
  public NASA acoustic reference. It recomputes and rejects direct real-acoustic
  explicit subcycling at the 60-tick candidate, rejects the required uniform
  sound-speed reduction as a default realism policy. Its uncoupled constant-pressure
  transport and whole-case HLLC components have standalone probes, and a separate
  one-dimensional mixed-region probe now exercises router promotion/demotion,
  cross-route flux/reflux coupling and event-local subcycling. General low-Mach
  pressure coupling, physical domain of dependence, 2D coupling and target-grid
  budget remain unimplemented, so solver and physical-time selection stay
  `unselected`.

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

`tools/physical_scale_check.py` validates the scale candidate, exact volume/world
geometry identities, unit roles, Legacy `CELL=4` / `153x96` / `612x384` fingerprint,
and the prohibition on deriving physical `dt` from presentation FPS. The standalone
strict-double `tools/atmospherebench/` scaffold uses a synthetic nondimensional EOS
fixture only; no real gas property is introduced without provenance.

`tools/atmosphere_policy_check.py` recomputes the acoustic-CFL substep count,
reference-machine time budget, memory bounds and maximum budget-compatible signal
speed. It fails closed if a rejected policy claims selection or if measured state
or working memory exceeds the declared Phase 5 budget.

For that contract, 1.0.5 extends the canonical SI registry with area, volume,
acceleration, momentum-density and molar-concentration dimensions. These are exact
unit definitions only: they do not add a material property, select a scale or make
the benchmark fixture dimensional.
