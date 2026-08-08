# Chemistry and property library research

## Cantera

Verified candidate: Cantera `v3.2.0`, tag peeled to
`4a8358eb80cfeb50474386b5f9ec0b3a83519889`. Its source license is BSD-style and
compatible with this GPL project when notices are retained.

Selected role: `DIRECT_REUSE` as an optional offline development tool for:

- mechanism parsing and validation;
- kinetics/thermodynamics reference trajectories;
- equilibrium and reaction-path analysis;
- mechanism reduction and error measurement;
- generation of a compact, independently versioned runtime database.

It is not selected for per-cell/per-frame runtime calls or as a required game
dependency. Mechanism files and thermodynamic datasets bundled with or loaded into
Cantera have their own provenance and redistribution obligations; Cantera's code
license does not license every mechanism.

Primary sources:

- <https://github.com/Cantera/cantera/tree/v3.2.0>
- <https://cantera.org/stable/>

## CoolProp

Verified candidate: CoolProp `v8.0.0`, MIT license.

Selected role: `DIRECT_REUSE` as an optional offline development/reference tool for:

- water saturation pressure and phase curves;
- latent heat, density, Cp/Cv and transport-property validation;
- humid-air reference cases;
- bounded table generation with explicit validity ranges.

It is not selected for a full property flash in each particle or atmosphere cell per
frame. Runtime inclusion requires a separate benchmark and packaging review.

Primary sources:

- <https://github.com/CoolProp/CoolProp/tree/v8.0.0>
- <https://coolprop.org/>

## NIST Chemistry WebBook

NIST Chemistry WebBook is SRD 69. Its documented coverage is useful for validating
thermochemistry, phase transitions, vapor pressure, Cp/Cv, density and transport
properties. Selected role: `REFERENCE_ONLY`.

The WebBook explicitly identifies itself as Standard Reference Data and carries a
2026 U.S. Secretary of Commerce copyright notice. General NIST pages distinguish
ordinary public data from SRD, and SRD redistribution may require permission or a
specific license. Therefore this project must not scrape and bundle a bulk WebBook
derivative by default. Each distributable datum/table needs an item-specific rights
decision and source citation.

Primary sources:

- <https://webbook.nist.gov/>
- <https://www.nist.gov/open/copyright-fair-use-and-licensing-statements-srd-data-software-and-technical-series-publications>

## Offline output contract

Every generated datum must record source, source version/date, units, temperature and
pressure range, method, uncertainty/confidence, generator version, transformation,
redistribution status and a hash. Estimated/interpolated/game-tuned values are marked
as such.

Generated tables are reviewed independently from the tool's source license. If a
source only permits reference/validation, commit expected ranges or independently
derived tests where lawful, not a copied database.

## Gate

Tool-selection research is GREEN. No package has been added to the repository or
runtime. Mechanism/data redistribution and generated runtime databases remain RED
until their provenance records pass the third-party audit.
