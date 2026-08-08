# Physical scale and unit proposal

## Status

This is a benchmark input, not an accepted contract. It makes every conversion
explicit so AtmosphereBench can reject or revise it. Production physical algorithms
remain blocked until Phase 2 selects a time/scale policy.

## Geometry candidate

| Quantity | Candidate | Consequence |
|---|---:|---|
| particle pixel length | `1.0e-3 m` | 612 x 384 world is 0.612 m x 0.384 m |
| atmosphere cell width/height | `4.0e-3 m` | matches current `CELL=4` |
| 2D effective depth | `4.0e-3 m` | atmosphere control volume is approximately cubic |
| atmosphere cell volume | `6.4e-8 m^3` | `dx * dy * effective_depth` |
| particle parcel volume | `4.0e-9 m^3` | one pixel area times effective depth |
| water parcel mass at 1000 kg/m3 | `4.0e-6 kg` | 4 mg per fully occupied particle parcel |
| dry-air cell mass near 293.15 K, 1 atm | about `7.7e-8 kg` | derived from density, not a fixed constant |

Partial occupancy, porosity and special particle types require explicit volume
fractions; they cannot all claim the full parcel volume.

## Unit contract candidate

| Quantity | External physical unit | Conservative storage |
|---|---|---|
| length/depth | m | scaled float based on `L0` |
| time | s | fixed simulation tick plus CFL substeps |
| mass | kg | density times cell volume |
| density/species density | kg/m3 | `rho`, `rho_i` |
| velocity | m/s | derived from momentum/density |
| momentum density | kg/(m2 s) | `rho*u`, `rho*v` |
| temperature | K | derived from energy/EOS |
| absolute pressure | Pa | derived, never independent in Enhanced |
| total energy density | J/m3 | `rho*E` |
| amount of substance | mol | derived from species mass/molar mass |
| reaction rate | mol/(m3 s) or kg/(m3 s) | converted by schema |

Runtime arrays may be nondimensional/scaled float32, but `PhysicalScale` and
`UnitConversion` own every transform. Scattered `*1000`, `/273` or implicit
per-frame coefficients are forbidden in new systems.

For an ideal-gas mixture:

```text
p = T * sum_i(rho_i * R_universal / molar_mass_i)
rho_E = rho_internal_energy + 0.5 * rho * (u^2 + v^2)
```

Pressure, temperature, velocity, mole fractions, partial pressures and humidity are
derived from the conservative state and material/species database.

## Time contract and unresolved CFL gate

The compatibility baseline to measure is one simulation tick = exactly `1/60 s` of
game time, independent of presentation FPS. Speed controls change how many fixed
ticks execute; rendering never changes the outcome for a fixed seed/tick count.

At `dx=0.004 m` and real air sound speed, an explicit compressible CFL step is on the
order of `1e-5 s`, making a direct 1/60-second tick require far too many substeps.
Therefore `physical_seconds_per_game_second=1` is a deliberately hard baseline, not
an accepted performance promise.

Phase 2/3 must choose and publish exactly one default policy after comparison:

- real-time mapping with solver subcycling/time dilation;
- documented acoustic scaling (reduced effective sound speed);
- all-speed/preconditioned or hybrid algorithm;
- another measured policy that preserves the intended phenomena.

Until then:

```text
SIMULATION_TICK_FIXED=true
PRESENTATION_INDEPENDENT=true
DEFAULT_PHYSICAL_TIME_MAPPING=UNSELECTED
PHYSICAL_SCALE_GATE=RED
```

## Atmosphere presets

Earth-like, Vacuum, Pure O2, Pure N2, CO2-rich and Custom are versioned data records.
Composition is not hard-coded in solver branches. Each preset supplies temperature,
absolute pressure or density, species fractions, humidity/water-vapor state and
provenance. Vacuum uses low gas mass with floors, not `rho=0` or negative pressure.

## Candidate cell memory

With five common species in float32:

- conservative fields: 4 core + 5 species = 36 bytes/cell;
- flags/active-registry metadata/alignment: target persistent total 40-48 bytes/cell;
- ping-pong conservative state: 72 bytes/cell;
- primitives and diagnostics: roughly 24-36 bytes/cell;
- X/Y flux or tiled scratch: roughly 48-72 bytes/cell.

Expected float32 working set is about 128-180 bytes/cell depending on tiled flux
storage. A strict double reference can exceed 300 bytes/cell and is an offline/test
backend, not the shipping memory target. Every PoC reports persistent and peak
scratch separately.

## Species storage proposal

Benchmark these layouts:

1. fixed common channels;
2. common channels plus sparse per-chunk trace species;
3. per-world active species registry with dense active channels;
4. shared mixture table.

The leading design is a per-world active registry with a small fixed/dense common set
and sparse trace chunks. It preserves GPU-coherent common channels without allocating
hundreds of species in every cell. Selection remains benchmark-gated.

## Legacy projection

Classic owns its current `pv/vx/vy/hv`. Enhanced exposes explicit Pa, kg/m3, K and
m/s APIs and may generate a bounded Legacy display/script projection. The mapping is
versioned and tested; no arbitrary assumption such as `1 pv = 1 Pa` is made.
