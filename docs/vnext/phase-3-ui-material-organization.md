# 1.0.3 UI and material-organization checkpoint

## Outcome

```text
TARGET_VERSION=1.0.3
BASE_COMMIT=c8a0190c10afba907c1c934c42d147e46c40b240
BRANCH=integration/omnicore-vnext
SCOPE=UI contracts, material routing audit, localization-report refresh, private visual validation
PRODUCTION_SIMULATION_CHANGE=false
ELEMENT_ID_CHANGE=false
LUA_IDENTIFIER_CHANGE=false
SAVE_FORMAT_CHANGE=false
STATIC_CLIENT_SHA256=D12A1AA1469FC233E3576584619C26DF8EAC7E1E8AB5FF5F9551889B380026C9
STATIC_BUILD=GREEN_40_40
PYTHON_DISCOVERY=GREEN_343_PASS_2_SKIPPED
PERIODIC_RUNTIME=GREEN_118_MAPPINGS_1024_EVENT_BUDGET
MATERIAL_RUNTIME=GREEN_IDS_521_532_12_PATHS_1536_EVENT_PEAK
MATERIAL_UI_AUDIT=GREEN_488_487_1
I18N=GREEN_1839_1839_0_ERRORS_38_WARNINGS
GUI_ZH_EN_200_PERCENT_DPI=GREEN
GUI_DPI_100_PERCENT=NOT_TESTED
GUI_DPI_125_PERCENT=NOT_TESTED
GUI_DPI_150_PERCENT=NOT_TESTED
V1_0_3_GATE=YELLOW
NEXT_VERSION=1.0.3
```

## Goal and scope

1.0.3 organizes existing materials without renumbering them or replacing their
simulation behavior. Its allowed changes are UI contracts, localization records,
and evidence. `Simulation`, `Particle`, `Air`, `GameSave`, Lua core, element IDs,
and material/chemistry behavior remain outside the scope.

The read-only implementation audit found that the intended organization already
exists: Search and Periodic Table have separate toolbar entries; selecting a
periodic cell opens its related-material picker rather than placing a particle; and
menu routing is data-driven. The new contract test fixes the currently observed
routing policy in place:

| Policy | Count | Required target |
|---|---:|---|
| `periodic_only` | 165 | `periodic_table` |
| `organic_menu` | 38 | `material_library:organic+periodic_table` |
| `alloy_menu` | 55 | `material_library:alloy_engineering+periodic_table` |
| `hidden` | 1 | `hidden_or_runtime_only` |

The generated audit reports 488 implemented slots: 487 canonical records and one
compatibility alias. There are 276 periodic-linked materials, including 93
supplemental links; 18 ordinary-material menu entries have been relocated from the
main selector. This preserves existing IDs, identifiers, module-disable semantics,
and the OPS identifier palette.

## Verification

### Static and runtime

- `py -3 -m unittest discover -s tools/tests -p 'test_*.py' -v`: **343 PASS**, 2
  declared skips.
- A new isolated `debugoptimized`, `static=prebuilt`, `legacy_fast` build passed
  Meson **40/40**. Its import table contains no development-only
  `libwinpthread-1.dll`, `libstdc++-6.dll`, or `libgcc_s_seh-1.dll` dependency.
- `runtime_lua_periodic_test.ps1`: **PASS**; 118 implemented mappings and a 1024
  event budget.
- `runtime_lua_materials_test.ps1`: **PASS**; IDs 521–532, 12 material paths,
  peak 1536 events.
- Current direct audits all pass: material UI, periodic mapping, periodic content
  links, UI style, alias, and save compatibility.
- `i18n_audit.py`: **PASS**, 1839 English and 1839 Chinese keys, zero errors and
  38 non-blocking review warnings. The stale generated report was refreshed from
  its 1624-key/406-element historical counts.

### Visual UI evidence

The screenshots and JSON records are isolated, ignored local artifacts under
`artifacts/vnext/1.0.3/ui/`; they are not public test or release content. The host
had `LogPixels=192` (200% system DPI). At that actual setting, both Chinese and
English passed these visible interactions with no observed clipping in the tested
controls:

- periodic grid, title, filters, search field, series labels, and hover status;
- periodic query `stainless`, yielding Cr/Fe/Ni and the related stainless-steel
  status;
- iron detail page, 13 related materials, scroll to the lower material groups,
  and a long press into the element-information panel;
- normal element search with `stainless`, yielding SSIL, NICL, CHRM, and STEL;
- Nobelium long-press information, which exercises the prior `OMNI_PT_NO` width
  warning. Its Chinese description was fully visible; English wrapped to two
  visible lines rather than clipping.

The actual visual evidence is stronger than static layout heuristics, but it is
not a substitute for the requested operating-system DPI matrix. No system display
setting was changed during this validation.

## Compatibility and performance

The phase changes only a test contract and generated localization evidence; it
does not alter menu records, particles, saves, Lua APIs, UI routing implementation,
or renderer code. Therefore existing 1.0.1 fixed-step benchmark evidence remains
the performance baseline. No new speed, RAM, or rendering-performance claim is
made for 1.0.3.

## Risks, gate, and rollback

The static client initially required a new isolated static build because the
existing dynamic development build cannot launch outside the MSYS2 runtime path.
That was a test-environment linkage condition, not a client UI failure; the static
validation executable was used for all visible checks.

The remaining 1.0.3 blocker is precise: Chinese and English still need real
100%, 125%, and 150% **operating-system** DPI validation. Internal scale or a
synthetic screenshot must not be reported as that matrix. The 38 localization
warnings are documented review items, not hidden errors; the only width warning was
visually sampled at the host DPI but remains subject to the cross-DPI check.

`V1_0_3_GATE=YELLOW`. Do not start 1.0.4 or modify physics, data schemas, SDL,
or GPU backends. The reversible implementation checkpoint is this report and its
small UI-route contract; rollback can remove that contract/doc commit without
affecting saved worlds or simulation semantics.
