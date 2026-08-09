# OmniCore incremental version state

```text
CURRENT_VERSION=1.0.3
CURRENT_PHASE=UI & Material Organization
PHASE_STATUS=YELLOW
BASE_COMMIT=c8a0190c10afba907c1c934c42d147e46c40b240
IMPLEMENTATION_HEAD=no production UI-routing or simulation change required
BRANCH=integration/omnicore-vnext
UPSTREAM_STABLE=v100.1.400 / d768aeb89acad986bd252d7e904bf44bb374545f
UPSTREAM_MASTER=d768aeb89acad986bd252d7e904bf44bb374545f
UPSTREAM_STATUS=GREEN_NO_NEW_DELTA
HISTORICAL_TAG_COLLISION=YELLOW_v99.5.394_not_overwritten
UI_ROUTE_CONTRACT=GREEN_488_487_1
I18N_STATIC=GREEN_1839_1839_0_ERRORS_38_WARNINGS
STATIC_BUILD=GREEN_40_40
PYTHON_DISCOVERY=GREEN_343_PASS_2_SKIPPED
PERIODIC_RUNTIME=GREEN_118_MAPPINGS
MATERIAL_RUNTIME=GREEN_IDS_521_532
GUI_ZH_EN_CURRENT_SYSTEM_DPI_200=GREEN
GUI_DPI_100_125_150=NOT_TESTED
V1_0_3_GATE=YELLOW
KNOWN_BLOCKERS=required 100/125/150 percent UI DPI matrix; G0 physical conservation; complete source-sink/correction accounting; unsampled full-state positivity; process VRAM; accepted performance budget
NEXT_VERSION=1.0.3
NEXT_PHASE=Complete the required UI DPI matrix or record a user-authorized isolated environment; then evaluate the 1.0.3 gate
```

The 1.0.2 refresh remains GREEN: official download, GitHub release, tag and master
all resolve to 100.1 build 400 at `d768aeb89`, and no upstream commit arrived after
the local 100.1 merge. No code adaptation was required.

The 1.0.3 implementation is deliberately limited to UI contracts, current
localization audit records, and private validation evidence. Existing data-driven
routing already satisfies the intended organization; no Element ID, Lua identifier,
save format, simulation, particle, Air, material behavior, or production UI route
was changed. The current static client validates Chinese and English periodic,
detail, scroll, long-press, and search paths at the host's actual 200% DPI. The
required 100%, 125%, and 150% operating-system DPI checks have not been run, so the
version gate remains YELLOW and 1.0.4 is not yet allowed. Global OmniCore G0 remains
RED and continues to forbid OmniAtmosphere implementation.

Milestone reports: [1.0.1 profiler foundation](../vnext/phase-1-profiler-export.md),
[1.0.2 upstream refresh](../vnext/phase-2-upstream-compatibility.md), and the
current [1.0.3 UI checkpoint](../vnext/phase-3-ui-material-organization.md).
