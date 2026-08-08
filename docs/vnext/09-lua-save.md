# Lua, Particle ABI and Save audit

## Particle and property compatibility

`Particle` is a 56-byte AoS record inside `std::array<Particle, NPART>` and can be
implicitly converted to `Particle *`. Its 14 four-byte fields are exposed through
`Particle::GetProperties()` using `offsetof`.

Stable indices such as `FIELD_TYPE=0` and `FIELD_TMP=9` are used by Lua and element
logic. Renderers, tools, updates, pmap/photons logic and callbacks directly depend on
the current layout or pointer semantics. Lua does not expose a raw FFI pointer, but
the C++ bridge performs offset-based access. A direct AoS-to-SoA rewrite is therefore
blocked.

The Property Tool and Lua are not fully equivalent: `AccessProperty` only special
cases type, while Lua routes coordinate writes through movement-aware logic. A future
access layer must preserve these observable differences until explicitly versioned.

## Legacy Air Lua API

Lua exposes pressure, velocity, ambient heat and related cell state as the current
Legacy arrays and settings. The current header also registers 505 SDL2 scan/key/mod/
button constants. SDL3 and OmniAtmosphere migrations must preserve names and values
through compatibility tables where SDL3 differs.

Enhanced pressure in Pascals cannot silently replace Lua `pv`; a Legacy projection or
Classic backing state is required.

## OPS wire format

OPS encodes particle fields rather than dumping `Particle` memory. It carries type,
integer position, quantized temperature, life, tmp, ctype, decoration, particle
velocity and tmp2/tmp3/tmp4 through versioned field data and identifier palettes.
`flags` and subpixel positions are not part of the normal persistent field set.

`pmap` and `photons` are reconstructed after load rather than serialized as raw maps.
High IDs use palette identifiers and an additional type byte; this behavior is
covered by current probes.

Air save planes are:

- `pressMap`, `vxMap`, `vyMap`: two bytes/cell at increments of `1/128` over the
  Legacy `[-256,256)` representation;
- `ambientMap`: two-byte integer Kelvin-like temperature;
- `blockAir`: pressure and heat block planes;
- fan and simulation settings stored separately.

`includePressure` controls whether pressure, velocity, ambient heat and Air block
maps are copied. Air mode, ambient temperature, edge pressure/velocity, vorticity and
convection mode are saved as simulation options.

OPS1 is actively tested. Readers for PSv/fuC remain in source, but no current fixed
PSv/fuC fixture has been executed in this phase.

## 100.1 correctness fixes

The upstream reset functions gained safe clamping for extreme integers. Audit found
that `sim.resetVelocity()` still used `XCELLS-1-x1` and `YCELLS-1-y1`, causing default
full-map calls and final-cell regions to omit the last row/column. Commit
`9c8767120` changes the extents to `XCELLS-x1` and `YCELLS-y1` and adds default,
final-row, final-column, final-cell and `INT_MIN` coverage. The 11-case runtime suite
passes. Official master still contained the off-by-one at the 2026-08-09 check.

The upstream many-authors change required local JSON/BSON adaptation. Commit
`729f72cba` preserves nested author links; the high-ID/complex-author probe passes.

## OmniAtmosphere save contract

- Store conservative atmosphere state in a separate versioned object/chunk; never
  reuse particle `tmp*` fields or reinterpret Legacy Air maps.
- Loading a Classic/old save initializes atmosphere from an explicit preset plus
  Legacy projection policy; missing atmosphere data is not treated as corruption.
- Unknown future species are preserved when possible or fail/degrade explicitly;
  they must not be silently remapped to O2/N2.
- Saving Classic preserves existing fields exactly. Enhanced may also emit a Legacy
  projection for older clients, marked as lossy.
- Keep stable element identifiers, FIELD indices, pmap/photons reconstruction and
  CPU reference semantics.

## Evidence and gate

| Check | Result |
|---|---|
| High-ID OPS and complex authors | PASS |
| Legacy OPS samples | PASS (`2` samples, `29` particles) |
| Eight OPS restart scenarios | PASS |
| Lua 100.1 boundary cases | PASS (`11/11`) |
| GUI save/load | `not_tested` |
| PSv/fuC runtime fixtures | `not_tested` |
| External Lua corpus | `not_tested` |

The bounded Lua/OPS sub-gate is GREEN. Broad Legacy compatibility remains YELLOW,
and new atmosphere persistence is RED pending schema and fallback tests.
