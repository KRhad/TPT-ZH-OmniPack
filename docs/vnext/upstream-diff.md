# Four-way upstream difference analysis

## Compared states

| State | Commit |
|---|---|
| LOCAL BASE | `bff38ce6959e1c1a7a4d17d0d5d44d127a0dfcbd` |
| CURRENT LOCAL OMNIPACK before vNext | `fb72d5e8f` |
| LATEST STABLE | `d768aeb89acad986bd252d7e904bf44bb374545f` (`v100.1.400`) |
| LATEST UPSTREAM MASTER | `d768aeb89acad986bd252d7e904bf44bb374545f` |
| CURRENT INTEGRATION | `430b3bd2868c17ba3d15c9a4580289173bcc79a6` |

Stable and master are identical at this audit time. The pre-vNext fork and stable
share LOCAL BASE. Pre-vNext had 181 local commits and was missing 13 upstream
commits; current integration contains both histories.

## 2026-08-10 refresh

The official download page, GitHub latest-release metadata and direct Git remote
all still identify `v100.1.400`; its peeled commit and `master` are both
`d768aeb89acad986bd252d7e904bf44bb374545f`. `git fetch official --prune` passed,
and `git rev-list ba30cd2e6..official/master` is zero. The tag-inclusive fetch
remains YELLOW only because it would clobber an unrelated local historical
`v99.5.394` tag; it was not overwritten. There is no new upstream delta after the
local 100.1 merge and thus no new category classification or source adaptation.

## Exhaustive classification of the 13 upstream commits

| Commit | Change | Classification | Regression disposition |
|---|---|---|---|
| `a5b12b719` | validate old coordinates in `create_part` | SIMULATION, BUGFIX | covered by upstream integration; targeted creation-boundary test still planned |
| `be1937d6a` | clamp `sim.resetPressure/resetVelocity`, including `INT_MIN` | LUA, AIR, BUGFIX | Lua 100.1 regression added; local final-row/column extension in `9c8767120` |
| `312774d23` | bounds-check `sim.neighbors` | LUA, SIMULATION, BUGFIX | Lua 100.1 out-of-range regression added |
| `9481bf05b` | correct `flood_water` horizontal-span test | SIMULATION, ELEMENT, BUGFIX | upstream code present; fixed water characterization save required |
| `beb9f2cbe` | reword non-C++ Lua error | LUA, BUILD | source assertion through upstream merge |
| `e6f481777` | update Lua BitOp changes | LUA, BUGFIX | static/build coverage; dedicated BitOp behavior corpus pending |
| `7ef296134` | add contribution/AI guidance | BUILD | documentation retained with local path adjustment |
| `7623e04e9` | prevent SPRK(TESC)/SPNG below-minimum temperature | THERMAL, ELEMENT, BUGFIX | source present; thermal regression save pending |
| `17ba33b01` | prevent OOB read during stamp migration | SAVE, BUGFIX | source present; old stamp fixture pending |
| `da11d071e` | darken render-option tooltip background | UI, RENDER | source present; visual GUI review `not_tested` |
| `51f1afbf8` | handle saves with many/complex authors | SAVE, LUA, UI, SIMULATION, BUGFIX | local BSON adaptation fixed in `729f72cba`; probe passes |
| `2ac390f60` | release 100.1 build 400 metadata | BUILD, UI | version metadata present |
| `d768aeb89` | correct version in `GameSave.cpp` | SAVE, BUILD, BUGFIX | source and OPS version path present |

No one of these commits introduces real gas density, mass, species, EOS, partial
pressure, or conservative thermodynamics. They must not be misreported as an
OmniAtmosphere baseline.

## Local OmniPack delta categories

At `fb72d5e8f`, the fork differed from LOCAL BASE across 894 files with 815,601
insertions and 1,350 deletions. Generated reports/data dominate the raw line count;
source concentration was highest under `src/simulation/elements`.

| Category | Local delta summary | Compatibility concern |
|---|---|---|
| SIMULATION | module filtering, event metrics, high-ID content and shared update helpers | iteration order, allocator and map semantics |
| AIR | mostly inherited Legacy fields plus many element pressure interactions | 86 explicit pressure writers; Lua field compatibility |
| THERMAL | per-element heat capacities and transition behavior | values lack uniform physical provenance/units; no latent heat |
| ELEMENT | registry expanded to 488 enabled elements | stable IDs and carried type fields must remain stable |
| REACTION | 328 registry rows and procedural local reactions | no generic conservation/kinetics loader |
| LUA | module/runtime automation and field exposure | stable property indices, offsets and SDL constants |
| SAVE | high-ID palette, module compatibility and OPS test infrastructure | OPS/legacy formats and author metadata |
| UI | Chinese localization, periodic/material navigation | GUI and DPI require visual proof |
| RENDER | localization/font and material presentation | threaded snapshot-copy cost |
| BUILD | Meson checks, release profiles and many audit tools | CRLF/VCS-tag path ordering and strict-FP isolation |
| PERFORMANCE | stress harness and event budgets | capped stability data is not an uncapped baseline |
| BUGFIX | numerous local compatibility and safety fixes | must remain separable from new physics work |

## Merge-specific resolutions

- `729f72cba` preserves the local author representation while correctly converting
  complex JSON arrays/objects to BSON; high-ID/author round trips pass.
- `9c8767120` changes reset-velocity extents from `XCELLS-1-x1` / `YCELLS-1-y1`
  to `XCELLS-x1` / `YCELLS-y1`. Official master still has the off-by-one at this
  audit, so this is a documented local bugfix, not an accidental upstream conflict.
- Local stable IDs, high-ID palette behavior, module filters and Omni content were
  retained. Upstream was adapted to the fork rather than replacing it.

## Gate

Upstream source adaptation is GREEN. Full G0 remains RED for the missing benchmark,
characterization, strict-FP numerical baseline, and differential runner documented
elsewhere.
