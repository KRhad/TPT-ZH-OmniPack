# Third-party code and data audit

Checked 2026-08-09. `Selected use` is the sole classification for each row.

| Name | Version/commit | Purpose | License | Code reuse allowed? | Modification allowed? | Redistribution allowed? | Data license | GPL compatibility | Reference only? | Selected use |
|---|---|---|---|---|---|---|---|---|---|---|
| The Powder Toy | `v100.1.400`, `d768aeb89` | upstream game/compatibility baseline | GPL-3.0-only | yes, under GPL | yes | yes, with GPL source/license obligations | N/A | native | no | `ADAPT` |
| TPT benchmark | `099b858260421cff42f3553b7e0680b80af3dddf` | benchmark design reference | no LICENSE found; GitHub metadata had no license | no permission established | no permission established | no permission established | included save/script also unlicensed | unknown | yes | `REFERENCE_ONLY` |
| SDL3 including SDL_GPU | `release-3.4.14`, `147a8ee32` | future platform and compute API | zlib | yes | yes | yes, retain notice/disclaimer | N/A | yes | no | `ADAPT` |
| SDL_shadercross | `e55cf5e31ced6f3d1be5cc6d0c50e99384f9f4ba` | future offline shader pipeline | zlib | yes | yes | yes, retain notice/disclaimer and audit transitive tools | shader compiler inputs/outputs remain project-owned; transitive dependencies separate | yes | no | `ADAPT` |
| Athena++ | `ed4d1e3e3a3beb53ab9757dcc4b964dfbbad621a` | FVM/Riemann/test architecture study | BSD-3-Clause | legally yes with conditions | yes | yes, with notice/non-endorsement | example/reference data require separate review | yes | yes by design choice | `REFERENCE_ONLY` |
| hydro-cl-lua | `80b4119547556284debc0b8c2b5d0f53efa47acd` | solver comparison and GPU/OpenCL experimentation patterns | MIT | legally yes with notice | yes | yes, retain notice | examples require source review | yes | yes by design choice | `REFERENCE_ONLY` |
| Cantera | `v3.2.0`, `4a8358eb8` | offline kinetics/thermodynamics validation and reduction | BSD-style 3-clause license | yes | yes | yes, retain conditions | mechanisms/thermo inputs are separately licensed | yes | no | `DIRECT_REUSE` |
| CoolProp | `v8.0.0` | offline property/reference calculations | MIT | yes | yes | yes, retain notice | underlying correlations/sources retain attribution/provenance | yes | no | `DIRECT_REUSE` |
| NIST Chemistry WebBook | SRD 69, data updated 2025, accessed 2026-08-09 | validation of thermochemistry and properties | NIST Standard Reference Data copyright; all rights reserved notice | not a code dependency | not assumed | bulk redistribution not authorized by default; specific permission/license required | SRD 69 | N/A | yes | `REFERENCE_ONLY` |
| The Parallel Toy / historical parallel TPT experiments | forum/source snapshots not pinned for reuse | CPU parallelization hazards and timing ideas | derived-project/license evidence not fully pinned in this audit | not approved | not approved | not approved | N/A | not yet determined per snapshot | yes | `REFERENCE_ONLY` |
| GPU falling-sand experiments | research category; no selected repository/commit | proposal/arbitration/ping-pong algorithm ideas | varies; no approved candidate | not approved | not approved | not approved | N/A | not determined | yes | `REFERENCE_ONLY` |

## Primary evidence

- TPT official source/tag: <https://github.com/The-Powder-Toy/The-Powder-Toy/tree/v100.1.400>
- TPT benchmark repository: <https://github.com/The-Powder-Toy/tpt-bench/tree/099b858260421cff42f3553b7e0680b80af3dddf>
- SDL release/license: <https://github.com/libsdl-org/SDL/tree/release-3.4.14>
- SDL_shadercross: <https://github.com/libsdl-org/SDL_shadercross/tree/e55cf5e31ced6f3d1be5cc6d0c50e99384f9f4ba>
- Athena++: <https://github.com/PrincetonUniversity/athena/tree/ed4d1e3e3a3beb53ab9757dcc4b964dfbbad621a>
- hydro-cl-lua: <https://github.com/thenumbernine/hydro-cl-lua/tree/80b4119547556284debc0b8c2b5d0f53efa47acd>
- Cantera 3.2.0: <https://github.com/Cantera/cantera/tree/v3.2.0>
- CoolProp 8.0.0: <https://github.com/CoolProp/CoolProp/tree/v8.0.0>
- NIST WebBook: <https://webbook.nist.gov/>
- NIST SRD terms: <https://www.nist.gov/open/copyright-fair-use-and-licensing-statements-srd-data-software-and-technical-series-publications>
- Parallel Toy discussion: <https://powdertoy.co.uk/Discussions/Thread/View.html?Thread=27760>

## Rules derived from the audit

- `DIRECT_REUSE` for Cantera/CoolProp means an optional offline tool dependency, not
  vendoring all source/data or linking it into the game runtime.
- `ADAPT` for SDL means a separately gated migration with retained notices and a
  transitive dependency audit.
- No-license and unpinned research remains reference-only even if technically useful.
- Property tables, reaction mechanisms and examples are audited independently from
  the code that reads them.
- Any copied or generated artifact must record exact input commit/version, license,
  transformations, source URLs and a content hash.

## Gate

Research classification is GREEN. Redistribution of any new third-party code/data is
RED until the exact artifact and all transitive licenses are added to the repository's
existing license manifest and independently reviewed.
