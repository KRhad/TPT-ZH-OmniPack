# Legacy Air audit

## Finding

Legacy Air is not absent. It is a coarse 153 x 96, four-particle-pixels-per-cell
pressure/velocity/ambient-temperature simulation with smoothing, pressure-velocity
coupling, semi-Lagrangian-style advection, vorticity confinement, walls, fans and
two convection approximations. OmniAtmosphere is therefore a state-model upgrade,
not the addition of an air feature from zero.

## Stored fields

| Field | Current meaning | Unit/status | Authoritative physical quantity? |
|---|---|---|---|
| `pv[y][x]` | signed pressure-like gameplay field, relaxed toward `edgePressure` | dimensionless, clamped `[-256, 256]` | no |
| `vx[y][x]`, `vy[y][x]` | coarse velocity-like gameplay field | cells/update-like, clamped `[-256, 256]` | no |
| `hv[y][x]` | ambient air temperature | Kelvin-like, clamped to TPT min/max temperature | temperature only, not energy |
| `fvx`, `fvy` | fan source velocity | Legacy field units | source planes |
| `bmap_blockair` | cell blocks pressure/velocity flow | byte mask | boundary cache |
| `bmap_blockairh` | ambient-heat block/count state; bit `0x8` is hard block | byte mask/counter | boundary cache |

Current and scratch Air planes plus block masks occupy about 34 bytes/cell; including
fan planes, about 42 bytes/cell (`616,896` bytes for 14,688 cells). These figures do
not include unrelated gravity or renderer arrays.

## Pressure and velocity update

`Air::update_air()` performs, in order:

1. edge cells relax toward configured edge pressure and velocity;
2. blocked cells and adjacent normal velocity components are zeroed;
3. velocity divergence updates `pv` using `AIR_TSTEPP=0.3`, while pressure decays by
   `AIR_PLOSS=0.9999`;
4. pressure gradients update `vx/vy` using `AIR_TSTEPV=0.4`, while velocity decays by
   `AIR_VLOSS=0.999`;
5. a normalized 3 x 3 kernel smooths fields;
6. velocity is backtraced by `0.7 * velocity` and bilinearly sampled with
   `AIR_VADV=0.3`, stopping at walls;
7. optional vorticity confinement, fan sources, caps and Air mode overrides apply;
8. scratch `ovx/ovy/opv` planes commit to the live fields.

This resembles a stable real-time flow approximation, but it does not update gas
mass, density, momentum density, total energy or species. `pv` can be assigned by
tools/elements independently of any EOS.

## Ambient heat and convection

`Air::update_airh()` pins two edge-cell layers to `ambientAirTemp`, smooths `hv`,
backtraces it through `vx/vy`, bilinearly samples around heat-blocking cells, and
commits through `ohv`.

Convection is already present:

- `AIRC_LEGACY` derives a gravity-aligned velocity increment from neighboring
  temperature differences;
- default `AIRC_BOUSSINESQ` assumes only the gravity term sees density variation and
  adds velocity from `(hv - ambientAirTemp) / 10000`, capped at `0.01`.

This is an approximation, not buoyancy emerging from an EOS and conserved density.

## Walls and update ordering

Hard Air blocks come from `WL_WALL`, `WL_WALLELEC`, `WL_BLOCKAIR`, inactive
`WL_EWALL`, and sufficient TTAN/RSSS behavior. Ambient heat also treats `WL_GRAV`
and heat insulators specially. `ApproximateBlockAirMaps()` reconstructs a conservative
first-frame mask while loading saves/stamps to reduce leaks.

`Simulation::BeforeSim()` updates Air and ambient heat before rebuilding the current
frame wall masks; particle updates later add dynamic blockers. This previous-frame
cache behavior is part of Legacy characterization and must not be silently changed.

## Particle coupling

The generic particle loop:

- damps cell velocity by an element's `AirLoss` and adds particle velocity through
  `AirDrag`;
- writes `HotAir` directly to `pv`;
- adds cell velocity to particle velocity through `Advection`;
- applies random per-particle `Diffusion` to gas-like particles.

Custom update inventory detects 86 elements across 38 roots explicitly writing
`pv`, DMG writing `vx/vy`, and LIGH writing `hv`. Explosions and special elements
commonly add fixed pressure impulses. VAC subtracts from `pv`; PUMP targets a `pv`
value. None removes or adds conserved gas mass.

## Vacuum and missing physical state

Legacy vacuum is negative `pv`, bounded at `-256`; it is not a low-density gas state.
An empty particle pixel still coexists with the Air fields, but those fields have no
default Earth-like mass or composition. Current Air has:

```text
real_gas_density=false
real_gas_mass=false
atmosphere_composition=false
species_conservation=false
partial_pressure=false
equation_of_state=false
humidity=false
mass_conserving_vacuum=false
internal_energy=false
```

## Compatibility contract

Classic must retain existing `pv/vx/vy/hv`, Air modes, Lua exposure, save maps,
element thresholds and visual behavior. Enhanced/Scientific may introduce a separate
conservative atmosphere whose derived Legacy projection is explicitly versioned.
Pressure in Enhanced must be derived from state plus EOS; it must not redefine the
existing Lua `pv` number as Pascals.

## Gate

The first-pass source audit is GREEN. Full behavioral characterization is RED until
fixed saves/seeds/traces cover edges, walls, fan flow, vorticity, convection,
explosions, vacuum and save/load. Production replacement remains blocked.
