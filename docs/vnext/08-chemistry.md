# Chemistry audit and OmniChem boundary

## Existing system

`docs/REACTION_REGISTRY.csv` currently contains 328 data rows. It documents local
game reactions, tests, event budgets, thresholds and failure behavior. Runtime logic
is primarily procedural C++ spread across `OmniChemistry`, `OmniPeriodic`,
`OmniMaterials`, `OmniMetallurgy`, `OmniOrganics`, environment/biology helpers and
classic element updates.

The code provides useful controls:

- bounded successful events per frame;
- local-neighborhood searches rather than global scans;
- temperature/pressure/electricity/catalyst gates in selected reactions;
- explicit particle products and fixed/probabilistic gameplay behavior;
- extensive audit and Lua regression coverage.

These are compatibility assets, not yet a general chemistry engine.

## Missing scientific contracts

```text
generic_reaction_loader=false
species_elemental_composition=false
automatic_atom_conservation=false
automatic_charge_conservation=false
arrhenius_runtime=false
concentration_rate_law=false
partial_pressure_rate_law=false
central_reaction_time_policy=false
unified_chemical_energy=false
mechanism_reduction_pipeline=false
```

Some reactions consume a neighboring O2 particle, but none consumes conserved
atmosphere O2. Many reactions are deterministic on contact or use frame probabilities.
Heat is often represented by direct temperature increments or a `reactionHeat`
constant rather than chemical energy transferred into a common internal-energy
ledger.

The registry phrase `source-confirmed` must be read narrowly: it currently means the
entry passed local allowlists/checkers and bounded tests. It does not mean every
stoichiometric coefficient, thermodynamic value or kinetic constant has been
verified against primary scientific literature.

## OmniChem staged design

The runtime schema will eventually support:

- reactants/products with species IDs, phase and stoichiometric coefficients;
- valid temperature/pressure domain;
- the smallest implemented rate expression required by the selected mechanism;
- catalysts, third bodies and reversibility only when needed;
- reaction enthalpy/energy transfer;
- elemental composition and charge on every species;
- loader rejection on atom or charge imbalance;
- provenance, confidence and redistribution status.

The workflow is:

```text
scientific mechanism/reference
-> offline validation
-> select relevant species and reactions
-> reduce mechanism
-> quantify error on fixed cases
-> compile compact versioned runtime database
```

The first runtime should not attempt arbitrary detailed chemistry. It should cover a
small, validated combustion mechanism after OmniAtmosphere supplies concentration,
partial pressure, temperature, density and `dt`.

## Classic versus Enhanced

- Classic preserves FIRE and all official/local procedural update behavior.
- Enhanced uses fuel, oxidizer concentration, temperature, kinetics, `dt`, products
  and energy. FIRE can remain a visual/reacting-region representation.
- No test-specific branches or one-frame universal conversion are permitted.
- Existing registry IDs and reaction tests remain a Legacy regression suite.

## Gate

Legacy chemistry regression is GREEN within its bounded test set. Scientific data
provenance and OmniChem runtime are RED. No current reaction count may be advertised
as scientifically validated chemistry coverage.
