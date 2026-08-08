# Material data and provenance research

## Current state

The fork has extensive element registries and source-level properties, including many
new heat-capacity values and documented concept origins. It does not yet provide a
property-level scientific provenance record for density, Cp, conductivity, melting/
boiling points, latent heat, vapor pressure, viscosity, diffusion or kinetics.

Many current numbers are gameplay scalars or bounded design values. They must not be
relabeled SI properties merely because a real material name is attached.

## MaterialDefinition proposal

Each material record is split into:

- Identity: stable ID, names, aliases, version and Legacy mapping;
- Composition: elements/species, fractions, solvent/solute or alloy basis;
- Mechanical: density, strength, permeability and phase-dependent behavior;
- Thermal: Cp/Cv, conductivity, enthalpy reference and latent heats;
- Electrical and Optical;
- Chemical: elemental composition, charge, reactive species and mechanism links;
- Phase: valid states, equilibrium/transition models and fractions;
- Transport: viscosity, diffusivity, surface tension and mass-transfer parameters;
- Behavior class: Classic, data-driven Enhanced, or special/fantasy CPU behavior.

Every individual property carries:

```text
source
source_version_or_access_date
units
temperature_range
pressure_range
method
uncertainty_or_confidence
redistribution_status
tuning_status = measured | estimated | interpolated | game_tuned
```

## Source policy

- CoolProp may validate/generate bounded water, humid-air and fluid properties under
  its MIT code license, while its source data references are still recorded.
- NIST WebBook SRD 69 is reference/validation only unless a property-specific
  redistribution right is confirmed.
- Peer-reviewed papers, standards and vendor datasheets need item-specific citation
  and rights review; a citation does not automatically authorize bulk redistribution.
- AI-generated values are never accepted as provenance.

## Mixtures and alloys

Solutions store solvent, solutes, composition, concentration and saturation; they do
not multiply element IDs for each concentration. Initial solubility and crystallization
may use a bounded database/interpolation model and finite kinetics.

Alloys store composition. Early property models may use a documented database,
interpolation or empirical relation with confidence bounds. The runtime must not claim
general predictive metallurgy.

## Validation plan

1. Define units and scale before importing numbers.
2. Start with a tiny set: dry-air species, water/ice/vapor, one inert solid, one fuel
   and one oxidizer.
3. Cross-check dimensions and validity ranges at load time.
4. Test reference values at multiple temperatures/pressures.
5. Hash generated tables and retain generator/tool versions.
6. Reject incomplete atom composition for chemistry-enabled species.
7. Keep game-tuned overrides separate from reference values and visible in reports.

## Gate

Schema direction is YELLOW. Material data import and OmniMaterials production use are
RED until the unit contract and redistribution audit pass.
