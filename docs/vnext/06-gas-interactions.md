# Gas interactions and ownership

## Current model

The particle registry contains many `TYPE_GAS` elements: a lexical source scan finds
50 element files, including physical names such as O2, H2, CO2, N, noble gases and
WTRV, but also FIRE, PLSM, SMKE, FOG, WARP and other gameplay/visual proxies. Each is
a normal `Particle` occupying particle-map space. This is not an atmosphere species
registry.

Current gas behavior combines:

- per-element particle `Advection`, `AirDrag`, `AirLoss`, `Diffusion`, gravity and
  `HotAir` coefficients;
- local custom updates and neighbor reactions;
- a separate Legacy Air pressure/velocity/temperature field with no composition.

Therefore current O2/H2/CO2 particles do not make `pv` into a mixture. Their count is
not converted into cell gas density, moles or partial pressure.

## Answers to required questions

| Question | Current answer |
|---|---|
| Does blank particle space mean vacuum? | No. It means no particle. Legacy Air still has `pv/vx/vy/hv`, with an implicit ambient baseline but no stored gas mass. |
| Is there real gas density or mass? | No. |
| Is there atmosphere composition or partial pressure? | No. |
| Does current combustion consume atmosphere O2? | No atmosphere O2 exists. Some rules consume neighboring O2 particles. |
| Does a pump remove gas mass? | No. Legacy PUMP/VAC manipulate `pv`; gas particles remain separate. |
| Does gas naturally rise/fall from mixture density? | No. Element gravity, random diffusion, velocity advection and Boussinesq-like Air convection drive motion. |
| Is water vapor tied to humidity? | No. WTRV is a particle element; humidity is not a derived state. |

Local O2-particle reactions are real gameplay dependencies: 58 `PT_O2` source
references appear in the simulation tree, including oxidation, biology, electronics,
periodic, metallurgy and chemistry helpers. They must remain intact in Classic.

## Authoritative ownership proposal

For Enhanced/Scientific, conserved gas mass belongs to OmniAtmosphere. A drawable
gas element must have exactly one explicit role:

| Role | Meaning | Mass accounting |
|---|---|---|
| Injection parcel | short-lived request that deposits a bounded species mass and energy into a cell | particle mass decreases as atmosphere mass increases |
| Source tool | user-controlled boundary/source term | ledger records user source |
| Tracer | visual marker sampling atmosphere motion/composition | zero physical gas mass |
| Temporary parcel | unresolved coupling state during migration | exclusive ownership flag; never counted in both systems |

No design may simultaneously count a drawn O2 particle's full mass and an equal O2
atmosphere deposit. The transfer event must be atomic and appear in the conservation
ledger.

Classic keeps particle-gas ownership and Legacy pressure behavior unchanged.

## Species and aerosols

Initial common-species candidates are N2, O2, Ar, CO2 and H2O vapor. Architecture
must allow a per-world active registry and trace species rather than hard-coding five
channels forever. Candidate storage is evaluated in `17-atmosphere-solver-plan.md`.

The following remain outside gas species storage:

- soot and dust: suspended solid aerosols;
- fog: liquid droplets/aerosol, not H2O vapor;
- smoke particles: visual/solid/liquid proxy unless converted by a defined transfer;
- condensed liquids and solids: particle/material layer.

CO, H2, CH4 and other true gaseous molecules may become active species, but only
when a mechanism and memory budget require them.

## Coupling requirements

Enhanced coupling must account for:

- gas-to-particle and particle-to-gas mass transfer;
- equal-and-opposite momentum transfer to the selected approximation level;
- heat/internal-energy exchange;
- species-resolved sources/sinks;
- boundary/user/Legacy/numerical correction ledgers.

Evaporation reduces condensed mass while increasing H2O vapor and consuming latent
energy. Condensation does the inverse and releases energy. A true pump exports gas
mass through a boundary/source term; a gas source imports it.

## Gate

Current-state audit is GREEN. Gas ownership design is YELLOW pending scale and save
decisions. Multi-species implementation is RED until G0 and AtmosphereBench pass.
