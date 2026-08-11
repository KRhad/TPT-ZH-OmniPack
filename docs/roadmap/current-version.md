# OmniCore incremental version state

```text
CURRENT_VERSION=1.0.5
CURRENT_PHASE=Physical Scale + AtmosphereBench
PHASE_STATUS=IN_PROGRESS_HLLC_2D_NATURAL_CONVECTION
BASE_COMMIT=13b24f49e18c22c794fae457eba9c8069fd13e6b
IMPLEMENTATION_HEAD=83c5a0cd2ba01b623a1d500e1cecb13f5931a60d
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
V1_0_5_RUSANOV=GREEN_ISOLATED_STRICT_DOUBLE_UNIFORM_PROBE_BUILD_80_STATIC_44_TARGETED_17_PYTHON_414_TOTAL_412_PASS_2_SKIPS_ZERO_DRIFT_ZERO_CORRECTIONS_HLLE_LBM_REGISTERED_ONLY
V1_0_5_RUSANOV_PRESSURE_PULSE=GREEN_ISOLATED_STRICT_DOUBLE_NONUNIFORM_128X1_64_STEP_PROBE_BUILD_80_STATIC_45_TARGETED_17_PYTHON_414_TOTAL_412_PASS_2_SKIPS_POSITIVE_ZERO_CORRECTIONS_HLLE_LBM_REGISTERED_ONLY
V1_0_5_RUSANOV_DENSITY_ADVECTION=GREEN_ISOLATED_STRICT_DOUBLE_128X1_EXACT_ONE_CELL_SHIFT_BUILD_80_STATIC_46_TARGETED_17_PYTHON_414_TOTAL_412_PASS_2_SKIPS_L1_0_000543106_ZERO_CORRECTIONS_HLLE_LBM_REGISTERED_ONLY
V1_0_5_RUSANOV_CONTACT=GREEN_ISOLATED_STRICT_DOUBLE_128X1_EXACT_ONE_CELL_SHIFT_BUILD_80_STATIC_47_PYTHON_414_TOTAL_412_PASS_2_SKIPS_L1_0_00923098_LINF_0_161912_ZERO_CORRECTIONS_HLLE_LBM_REGISTERED_ONLY
V1_0_5_RUSANOV_NEAR_VACUUM=GREEN_ISOLATED_STRICT_DOUBLE_128X1_DENSITY_1E_MINUS_6_PRESSURE_1E_MINUS_8_BUILD_80_STATIC_48_PYTHON_414_TOTAL_412_PASS_2_SKIPS_MASS_TRANSFER_POSITIVE_ZERO_CORRECTIONS_HLLE_LBM_REGISTERED_ONLY
V1_0_5_RUSANOV_SOD=GREEN_ISOLATED_STRICT_DOUBLE_256X1_GAMMA_1_4_T_0_2_BUILD_80_STATIC_49_PYTHON_414_TOTAL_412_PASS_2_SKIPS_SHOCK_X_0_855469_BOUNDARY_LEDGER_ZERO_CORRECTIONS_HLLE_LBM_REGISTERED_ONLY
V1_0_5_RUSANOV_REFINEMENT=GREEN_ISOLATED_STRICT_DOUBLE_64_128_256_T_0_25_BUILD_82_STATIC_50_PYTHON_414_TOTAL_412_PASS_2_SKIPS_L1_ORDER_0_95393_0_977252_ZERO_CORRECTIONS_HLLE_LBM_REGISTERED_ONLY
V1_0_5_RUSANOV_LOW_MACH=EXECUTION_GREEN_SUITABILITY_FALSE_MACH_0_387298_0_0387298_0_00387298_L1_0_00826755_0_0515261_0_126479_TV_0_934805_0_595493_0_00673822_ZERO_CORRECTIONS
V1_0_5_RUSANOV_OPEN_LEAK=GREEN_128X1_SEALED_LEFT_OPEN_RIGHT_MASS_OUT_6_69874_BALANCE_ERRORS_LT_3E_MINUS_14_ZERO_CORRECTIONS
V1_0_5_RUSANOV_PERFORMANCE=RECORDED_NO_BUDGET_STRICT_DOUBLE_SINGLE_THREAD_31_0232M_31_3699M_32_8059M_CELL_UPDATES_PER_SECOND_96_BYTES_PER_CELL
V1_0_5_ALL_SPEED_RUSANOV=EXECUTION_GREEN_SUITABILITY_FALSE_MACH_0_387298_0_0387298_0_00387298_L1_0_00454342_0_0136636_0_10116_TV_0_964322_1_82225_1_46479_ZERO_CORRECTIONS
V1_0_5_ALL_SPEED_RUSANOV_COMMIT=fae9a0847608333c05a9d5e15b0c812b22407d3d
V1_0_5_HLLC_RUSANOV_FALLBACK=FRONT_RUNNER_NOT_SELECTED_LOW_MACH_NEAR_VACUUM_SOD_OPEN_LEAK_PERFORMANCE_GREEN_ZERO_FALLBACK_ZERO_CORRECTIONS
V1_0_5_HLLC_RUSANOV_FALLBACK_COMMIT=3eb235b8c27174f8e5ee8c31c33ceb167ffe545b
V1_0_5_HLLC_FALLBACK_CONTRACT=GREEN_ADVERSARIAL_INTERFACE_EXPECTED_COUNT_1_COMMIT_78bad784d
V1_0_5_HLLC_2D_PERIODIC=GREEN_UNIFORM_AND_PRESSURE_PULSE_32X24_ZERO_FALLBACK_ZERO_CORRECTIONS_160_BYTES_PER_CELL_COMMIT_d38120177
V1_0_5_HLLC_2D_SEALED_HEATING=GREEN_SOURCE_LEDGER_CLOSE_PRESSURE_TEMPERATURE_INCREASE_ENERGY_BALANCE_3_21876E_MINUS_11_ZERO_FALLBACK_ZERO_CORRECTIONS_162_333_BYTES_PER_CELL_COMMIT_be0ff2f37
V1_0_5_HLLC_2D_NATURAL_CONVECTION=GREEN_CONTROL_SUBTRACTED_THERMAL_RISE_0_225497_UPDRAFT_0_0257047_RETURN_MINUS_0_000157248_SOURCE_BOUNDARY_LEDGER_LT_8E_MINUS_12_ZERO_FALLBACK_ZERO_CORRECTIONS_COMMIT_83c5a0cd2
KNOWN_BLOCKERS=required 100/125/150 percent UI DPI matrix; G0 physical conservation; complete production source-sink/correction accounting; unsampled full-state positivity; process VRAM; accepted performance budget; selected PhysicalScale and physical-time policy; gas-mixing species-conservation and multidimensional performance evidence
NEXT_VERSION=1.0.6
NEXT_PHASE=add passive conserved-species gas-mixing checkpoint only inside AtmosphereBench; keep HLLE and LBM registered-only
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
The PhysicalScale/AtmosphereBench scaffold and shared contracts remain isolated.
One first-order strict-double Rusanov debug candidate now has 1D periodic uniform,
pressure-pulse, density-advection, contact-discontinuity, near-vacuum-expansion,
sealed Sod, smooth-grid-refinement and low-Mach probes, while HLLE and LBM remain
registration-only. The low-Mach execution is valid but reports suitability false,
so pure first-order compressible Rusanov is not selected. All scale, time and solver
selections remain unselected. Its open-boundary ledger and three-size standalone
performance record are now clean-validated, but no physical-time or performance
budget is selected, and no production
source consumes the bench. Details are in the [1.0.5 scaffold report](../vnext/phase-5-physical-scale-atmospherebench.md)
and the [Rusanov candidate checkpoint](../vnext/phase-5-rusanov-candidate.md).

The isolated all-speed Rusanov candidate is recorded at `fae9a0847`. Its
strict-double low-Mach probe executes with positivity, conservation and zero
numerical corrections, but it is rejected by the unchanged suitability gate:
very-low-Mach density L1 is `0.10116` (required `<= 0.05`) and the measured
total-variation ratio is `1.46479`. The clean runner bound the result to the
current commit and Meson target; no Sod/near-vacuum extension was claimed after
this mandatory Low-Mach gate failed.

The corresponding clean source checkpoint is
`artifacts/vnext-phase5-source-9b7e7d095/`, SHA-256
`553FA58C897715099B47B8B1E500F0FAE51336CFEC99DA2A1787754CC5E44387`.
It contains `1303` source members plus `SOURCE-MANIFEST.txt`, includes the
AtmosphereBench sources/runner, and contains zero test assets.

HLLC with explicit Rusanov fallback is now the isolated front-runner at
`3eb235b8c`. It passes the unchanged Low-Mach gate, near-vacuum expansion, sealed
Sod and open-leak ledger with zero fallbacks and zero numerical corrections. Its
clean strict-double throughput is `18.8104M / 18.4543M / 18.6026M`
cell-updates/s. It remains unselected after later 2D validation because gas mixing,
two-dimensional performance, physical-time and accepted budget evidence are still
absent.
See the [HLLC candidate checkpoint](../vnext/phase-5-hllc-candidate.md).

The defensive fallback path is independently exercised at `78bad784d`: a valid
ultra-low-pressure/acoustic interface invalidates the HLLC star state,
increments the fallback count exactly once and returns the strict Rusanov flux.

The first periodic two-dimensional checkpoint is clean-validated at `d38120177`.
The `32x24` uniform case has zero state change and zero mass, momentum and energy
drift. The `32x24` pressure pulse remains positive, evolves nontrivially and closes
mass/energy within `6.9e-13`; both cases record zero fallback and correction events.
The explicit initial/current/next plus Flux-X/Flux-Y allocation is
`160 bytes/cell`. This is not sealed-boundary, source-term, physical-time or
solver-selection evidence.

The sealed-heating checkpoint is clean-validated at `be0ff2f37`. It adds a
general conservative applied-source ledger and sealed face fluxes only to the
standalone bench. Nondimensional pressure and temperature rise from `1` to
`1.06667`; mass/momentum remain unchanged, and the `76.8` energy increase closes
against the source ledger within `3.21876e-11`. All `30720` source applications
are counted, with zero fallback and numerical-correction events. This does not
select PhysicalScale, physical time or the solver.

The gravity/source/boundary-ledger natural-convection checkpoint is clean-
validated at `83c5a0cd2`. A control-subtracted bottom thermal anomaly rises
`0.225497` cell and produces differential updraft/return-flow signals
`0.0257047 / -0.000157248`. Control and heated runs remain positive, stay below
CFL `0.032`, record zero fallback/corrections and close source plus wall exchange
within `8e-12`. It is not a well-balanced, physical-time or production-wall claim.

The clean source package for the natural-convection checkpoint is
`artifacts/vnext-phase5-source-20309c670/`, SHA-256
`043746D179C1D2A0691FCA3C4A9AA728263E9B9C901301890928F992E9173529`,
with `1306` source members plus manifest and zero test assets. The manifest binds
revision `20309c6703f210600a7f605e54790ab922ecc9c1`.

Milestone reports: [1.0.1 profiler foundation](../vnext/phase-1-profiler-export.md),
[1.0.2 upstream refresh](../vnext/phase-2-upstream-compatibility.md), and the
1.0.3 [UI checkpoint](../vnext/phase-3-ui-material-organization.md), and the
1.0.4 [material-data checkpoint](../vnext/phase-4-material-data-foundation.md).
