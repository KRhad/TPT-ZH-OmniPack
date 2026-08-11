# OmniCore incremental version state

```text
CURRENT_VERSION=1.0.5
CURRENT_PHASE=Physical Scale + AtmosphereBench
PHASE_STATUS=IN_PROGRESS_LEGACY_LIKE_CONTROL_COMPLETE
BASE_COMMIT=13b24f49e18c22c794fae457eba9c8069fd13e6b
IMPLEMENTATION_HEAD=696d0c9580f642db70bb419fc73cd9ea19b2afd0
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
V1_0_5_HLLC_2D_SPECIES_MIXING=GREEN_PASSIVE_BINARY_32X24_320_STEPS_SPECIES_A_B_ZERO_DRIFT_FRACTION_BOUNDS_0_1_TV_48_TO_47_9173_768_MIXED_CELLS_ZERO_FALLBACK_ZERO_CORRECTIONS_40_STATE_200_WORKING_BYTES_PER_CELL_COMMIT_55088a852
V1_0_5_HLLC_2D_PERFORMANCE=RECORDED_NO_BUDGET_STRICT_DOUBLE_SINGLE_THREAD_153X96_1_69129MS_306X192_6_56627MS_612X384_31_2385MS_160_WORKING_BYTES_PER_CELL_COMMIT_12e904f75
V1_0_5_LBM_D2Q9=GREEN_ISOTHERMAL_ONLY_UNIFORM_AND_SHEAR_WAVE_RELATIVE_ERROR_0_000577441_ZERO_CORRECTIONS_72_STATE_144_WORKING_BYTES_PER_CELL_ENERGY_NA_NEAR_VACUUM_UNSUPPORTED_SHOCK_UNSUPPORTED_COMMIT_5e3c46fae
V1_0_5_LEGACY_LIKE=GREEN_CONTROL_ONLY_UNIFORM_ZERO_CHANGE_PRESSURE_PULSE_PEAK_0_99005_TO_0_236479_PRESSURE_SUM_DRIFT_8_52651E_MINUS_13_ZERO_CORRECTIONS_32_STATE_64_WORKING_BYTES_PER_CELL_PHYSICAL_DRIFTS_NULL_COMMIT_696d0c958
KNOWN_BLOCKERS=required 100/125/150 percent UI DPI matrix; G0 physical conservation semantics; complete production source-sink/correction accounting; unsampled full-state positivity; process VRAM; accepted CPU and memory budget; selected PhysicalScale and physical-time policy; remaining mandatory solver and precision matrix
NEXT_VERSION=1.0.6
NEXT_PHASE=define and validate CPU/memory budget and physical-scale/time policy, then complete the mandatory solver and precision matrix before selection
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
sealed Sod, smooth-grid-refinement and low-Mach probes. HLLE remains registration-
only; D2Q9 LBM now has limited isothermal uniform/shear comparison results. The
Rusanov low-Mach execution is valid but reports suitability false,
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
cell-updates/s. Passive binary gas mixing now also closes both species ledgers in
2D, but the candidate remains unselected because two-dimensional performance,
physical-time and accepted budget evidence are still absent.
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

The passive conserved-species checkpoint is clean-validated at `55088a852`.
Uniform gas advects a binary left/right composition for 320 nondimensional steps
on a periodic `32x24` grid. Gas mass, momentum and energy plus species A/B mass
all report zero drift; fractions remain within `[0,1]`, all 768 cells enter the
mixed region, and composition total variation decreases measurably from `48` to
`47.9173`. Fallback and correction counts remain zero. Authoritative state is
`40 bytes/cell`; the explicit current/next/initial plus X/Y flux working set is
`200 bytes/cell`. Species-EOS coupling and physical diffusion remain
`not_implemented`. The clean result SHA-256 is
`F5B7C1FBB60CD2E7672AA5A50FC46A8600C3D8B43E7BE074234BDADC9D8982A4`.

The strict-double single-threaded 2D performance checkpoint is clean-validated at
`12e904f75`. Median end-to-end time per step is `1.69129 ms` for the current
`153x96` Air grid, `6.56627 ms` for `306x192`, and `31.2385 ms` for the
`612x384` particle grid. Throughput is `8.68448M / 8.94755M / 7.52304M`
cell-updates/s; the current periodic working allocation is `160 bytes/cell`.
All samples preserve positivity and use zero fallback/corrections. The 60 Hz frame
fraction is recorded only as context; `performance_budget_status=unselected`.
The clean result SHA-256 is
`2792A219A32A3F7C81EC93ABF9301B5D3D2EE48A6BC74D27A40CBB8DE7D1BF87`.

The D2Q9 BGK LBM comparison is clean-validated at `5e3c46fae`. Its `64x48`
uniform state is preserved to `4.26326e-14` population L1, and a `64x64`
low-Mach shear wave decays with `0.000577441` relative error against the lattice-
viscosity reference. Both runs conserve mass/momentum, retain positive populations
and use zero corrections. State/working memory is `72/144 bytes/cell`. The result
also explicitly records no energy state, no near-vacuum or shock support and no
species transport; `energy_drift` is JSON `null`, not fake zero. Clean result
SHA-256 values are
`D601D2DDA76BECB73C37CE5CB309F6460A10B2516D50C135C8D7B39C26FA4333`
and `26180CBA90702B0458D30FA05C1867CEEAFA743D7CCF50BE7F393C29E7C12AE6`.

The Legacy-like comparison is clean-validated at `696d0c958`. Its standalone
strict-double `64x48` dimensionless control preserves a uniform state exactly and
evolves a two-dimensional pressure pulse from peak `0.99005` to `0.236479` while
preserving the pressure-field sum within `8.52651e-13`; it records zero numerical
corrections and uses `32/64 bytes/cell` for state/current-plus-next. This is not
production Legacy Air equivalence and has no physical mass, density, momentum-
density, energy or species state. The corresponding JSON physical drift fields
are `null`, not fake zero. Clean result SHA-256 values are
`505F69D6D87724CC1B8E4410CA13BE91AF9464E22B12BBE338C601DB74CF04AD`
and `D1FE38268F52B15AB6BFA7D135B77122A79D48C1227A1E1F5909AEBB4AD0DFBA`.
See the [Legacy-like checkpoint](../vnext/phase-5-legacy-like-control.md).

The clean source package for the current D2Q9 comparison checkpoint is
`artifacts/vnext-phase5-source-c8ae4ea8a/`, SHA-256
`3F1DB05D65D61701B726DE12BD959E43739291F3026D7BAC251E8DDF7B025876`,
with `1313` source members plus manifest and zero test assets. The manifest binds
revision `c8ae4ea8a7989965972825b26382b4ed485f5030`.

Milestone reports: [1.0.1 profiler foundation](../vnext/phase-1-profiler-export.md),
[1.0.2 upstream refresh](../vnext/phase-2-upstream-compatibility.md), and the
1.0.3 [UI checkpoint](../vnext/phase-3-ui-material-organization.md), and the
1.0.4 [material-data checkpoint](../vnext/phase-4-material-data-foundation.md).
