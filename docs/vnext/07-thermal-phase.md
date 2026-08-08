# Thermal and phase audit

## Current thermal state

Particles store one `temp` float, and Air cells store one `hv` float. Both are
Kelvin-like temperatures. Elements have game coefficients such as `HeatConduct`,
`HeatCapacity`, low/high transition thresholds and target types.

The non-Legacy particle heat path:

1. probabilistically enables conduction using `HeatConduct`;
2. exchanges particle and ambient temperature using an element heat-capacity scalar;
3. explicitly notes that it ignores the `CELL^2` particle pixels represented by an
   Air cell and ignores actual air heat capacity;
4. equilibrates selected neighboring particles through a heat-capacity-weighted
   average;
5. checks transition thresholds and changes particle type immediately.

This gives useful gameplay heat behavior, but it is not a closed energy model.

## Phase behavior

For a liquid-to-gas transition, current code shifts the effective high-temperature
threshold by `-2 * pv`; gas-to-liquid does the same to the low threshold. This makes
boiling/condensation respond qualitatively to Legacy pressure, but it is not a
saturation-vapor-pressure relation and `pv` is not Pascals.

Current status:

```text
temperature_state=true
internal_energy_state=false
enthalpy_state=false
latent_heat=false
partial_phase=false
saturation_vapor_pressure=false
pressure_aware_boiling=legacy_linear_approximation
humidity=false
energy_conservation=false
```

Many transitions are instantaneous type changes once a threshold is met. Heat is not
reserved for melting/vaporization, and a phase can change without a latent-energy
budget. Existing `HeatCapacity` values include useful game tuning but are not a
uniform SI dataset with property-level provenance.

## OmniThermal target

The recommended first model is an enthalpy formulation with a piecewise material
relation:

```text
specific_enthalpy + composition + pressure
    -> temperature + phase fractions + sensible/latent energy
```

Required authoritative or derived values are:

- authoritative condensed mass and specific/total enthalpy;
- material heat capacity and thermal conductivity over stated validity ranges;
- solid/liquid/vapor latent heats and transition curves;
- derived temperature and compact phase fraction;
- explicit gas/condensed mass and energy transfers.

The apparent-heat-capacity method remains an AtmosphereBench/MaterialBench candidate
if it provides better stability and simpler coupling. Neither method is selected
without fixed melting/freezing/boiling experiments.

## Compatibility boundary

Classic retains threshold transitions, existing `temp`, element coefficients and Lua
property behavior. Enhanced may interpret `temp` as a projection of enthalpy, but Lua
writes must pass through a documented energy update rather than silently desynchronizing
state. Old saves that only contain temperature initialize enthalpy from material,
phase and temperature under a versioned compatibility rule.

## Required verification

- isolated sensible-heat exchange with a closed energy ledger;
- ice melting and refreezing with hysteresis policy documented;
- water boiling at multiple pressures using a reference saturation curve;
- condensation and latent heat release;
- strict/fast FP drift and clamp/floor counts;
- Classic differential traces to prove unchanged threshold behavior.

## Gate

Audit is GREEN. OmniThermal implementation is RED until the physical-scale contract,
atmosphere pressure, material provenance, strict-FP tests and CPU reference exist.
