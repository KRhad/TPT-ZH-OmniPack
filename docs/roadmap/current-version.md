# OmniCore incremental version state

```text
CURRENT_VERSION=1.0.5
CURRENT_PHASE=Physical Scale + AtmosphereBench
PHASE_STATUS=IN_PROGRESS_SHARED_CONTRACT_CLEAN_VALIDATED
BASE_COMMIT=13b24f49e18c22c794fae457eba9c8069fd13e6b
IMPLEMENTATION_HEAD=1ba507e89e3d713fe355c03c2fc6e7139aabcb49
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
V1_0_3_DISPOSITION=USER_ACCEPTED_YELLOW
V1_0_3_DPI_MATRIX=DPI_DEFERRED
V1_0_4_GATE=GREEN
V1_0_4_ROLLBACK=477372373cb1be30c3c04bca4309a7d6fd9fa799
V1_0_4_CLEAN_VALIDATION=GREEN_BUILD_80_STATIC_41_PYTHON_385_PLUS_2_SKIPS_LUA_OPS_PACKAGE_BENCHMARK
V1_0_5_GATE=IN_PROGRESS
V1_0_5_UPSTREAM=GREEN_ENTRY_STABLE_MASTER_d768aeb89_LOCAL_AHEAD_218_BEHIND_0
V1_0_5_PHYSICAL_SCALE_SELECTION=UNSELECTED
V1_0_5_TIME_POLICY=UNSELECTED
V1_0_5_ATMOSPHERE_SOLVER_SELECTED=false
V1_0_5_SCAFFOLD=GREEN_CLEAN_BUILD_80_STATIC_43_TARGETED_24_PYTHON_409_TOTAL_407_PASS_2_SKIPS_CONTRACT_ARTIFACT_AND_SOURCE_PACKAGE
V1_0_5_SHARED_CONTRACT=GREEN_CLEAN_BUILD_80_STATIC_43_TARGETED_26_PYTHON_411_TOTAL_409_PASS_2_SKIPS_CONTRACT_ARTIFACT_AND_SOURCE_PACKAGE
KNOWN_BLOCKERS=required 100/125/150 percent UI DPI matrix; G0 physical conservation; complete source-sink/correction accounting; unsampled full-state positivity; process VRAM; accepted performance budget
NEXT_VERSION=1.0.6
NEXT_PHASE=add the first-order strict-double Rusanov debugging candidate only; keep all other candidates registered-only
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
version gate remains YELLOW. After that limitation was reported, the user explicitly
directed the orchestrator to enter the next version. The exception is recorded as
`USER_ACCEPTED_YELLOW / DPI_DEFERRED`; it does not convert missing evidence to GREEN.
Global OmniCore G0 remains RED and continues to forbid OmniAtmosphere implementation.

Version 1.0.4 is **GREEN** at `9cc2b11c5`. It adds only offline versioned
Material/Species/Reaction contracts, canonical units, provenance validation and a
488-entry identity map; no runtime source, Element ID, save format, Lua identifier,
or simulation behavior changed. Clean build/static/Python/Lua/OPS/package and
fixed-step evidence are recorded in
[the 1.0.4 material-data checkpoint](../vnext/phase-4-material-data-foundation.md).
Version 1.0.5 is now **IN_PROGRESS**. Its entry refresh found the official 100.1
stable tag and master unchanged at `d768aeb89`, with zero upstream-only commits.
The new work is an isolated PhysicalScale/AtmosphereBench scaffold only: all scale,
time and solver selections remain unselected, no numerical solver is implemented,
and no production source consumes it. Details are in
[the 1.0.5 scaffold report](../vnext/phase-5-physical-scale-atmospherebench.md).

Milestone reports: [1.0.1 profiler foundation](../vnext/phase-1-profiler-export.md),
[1.0.2 upstream refresh](../vnext/phase-2-upstream-compatibility.md), and the
1.0.3 [UI checkpoint](../vnext/phase-3-ui-material-organization.md), and the
1.0.4 [material-data checkpoint](../vnext/phase-4-material-data-foundation.md).
