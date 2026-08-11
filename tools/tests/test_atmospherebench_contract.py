from __future__ import annotations

import importlib.util
import json
from pathlib import Path
import re
import sys
import unittest
from decimal import Decimal


ROOT = Path(__file__).resolve().parents[2]
BENCH_ROOT = ROOT / "tools" / "atmospherebench"
PACKAGE_TOOL_PATH = ROOT / "tools" / "package_source_release.py"
PACKAGE_SPEC = importlib.util.spec_from_file_location(
    "package_source_release_for_atmospherebench_test", PACKAGE_TOOL_PATH
)
assert PACKAGE_SPEC is not None and PACKAGE_SPEC.loader is not None
package_tool = importlib.util.module_from_spec(PACKAGE_SPEC)
sys.modules[PACKAGE_SPEC.name] = package_tool
PACKAGE_SPEC.loader.exec_module(package_tool)


class AtmosphereBenchSourceContractTests(unittest.TestCase):
    def test_scaffold_isolated_from_production_simulation(self) -> None:
        text = "\n".join(path.read_text(encoding="utf-8") for path in BENCH_ROOT.glob("*.cpp"))
        forbidden = (
            "src/simulation", "Simulation.h", "Air.h", "GameSave",
            "LuaScriptInterface", "project_cpp_args", "-ffast-math",
            "-funsafe-math-optimizations",
        )
        for needle in forbidden:
            with self.subTest(needle=needle):
                self.assertNotIn(needle, text)
        includes = re.findall(r'^#include "([^"]+)"', text, flags=re.MULTILINE)
        self.assertEqual(includes.count("AtmosphereBench.h"), 2)
        self.assertEqual(includes.count("Rusanov1D.h"), 7)

    def test_bench_has_explicit_strict_fp_target_and_isolated_rusanov_candidates(self) -> None:
        meson = (ROOT / "meson.build").read_text(encoding="utf-8")
        self.assertIn("atmospherebench", meson)
        self.assertIn("-fno-fast-math", meson)
        self.assertIn("-ffp-contract=off", meson)
        start = meson.index("atmospherebench = executable(")
        end = meson.index("font_render_probe = executable(", start)
        target = meson[start:end]
        self.assertIn("cpp_args: atmospherebench_cpp_args", target)
        self.assertNotIn("cpp_args: project_cpp_args", target)
        self.assertNotIn("sta_libs['simulation']", target)
        self.assertIn("'atmospherebench-rusanov-uniform'", target)
        self.assertIn("args: [ '--run-rusanov-uniform' ]", target)
        self.assertIn("'atmospherebench-rusanov-pressure-pulse'", target)
        self.assertIn("args: [ '--run-rusanov-pressure-pulse' ]", target)
        self.assertIn("atmosphere_precision_double_strict", target)
        self.assertIn("atmosphere_precision_float_strict", target)
        self.assertIn("atmosphere_precision_float_fast", target)
        self.assertIn("-ffast-math", target)
        self.assertIn("'atmospherebench-rusanov-density-advection'", target)
        self.assertIn("args: [ '--run-rusanov-density-advection' ]", target)
        self.assertIn("'atmospherebench-hybrid-mixed-region'", target)
        self.assertIn("args: [ '--run-hybrid-mixed-region' ]", target)
        self.assertIn("'atmospherebench-hybrid-mixed-region-2d'", target)
        self.assertIn("args: [ '--run-hybrid-mixed-region-2d' ]", target)
        self.assertIn("'atmospherebench-low-mach-projection-2d'", target)
        self.assertIn("args: [ '--run-low-mach-projection-2d' ]", target)
        self.assertIn("'atmospherebench-rusanov-contact-discontinuity'", target)
        self.assertIn("args: [ '--run-rusanov-contact-discontinuity' ]", target)
        self.assertIn("'atmospherebench-rusanov-near-vacuum-expansion'", target)
        self.assertIn("args: [ '--run-rusanov-near-vacuum-expansion' ]", target)
        self.assertIn("'atmospherebench-rusanov-sod-shock-tube'", target)
        self.assertIn("args: [ '--run-rusanov-sod-shock-tube' ]", target)
        self.assertIn("'atmospherebench-rusanov-density-advection-refinement'", target)
        self.assertIn("args: [ '--run-rusanov-density-advection-refinement' ]", target)
        self.assertIn("'atmospherebench-rusanov-low-mach-advection'", target)
        self.assertIn("args: [ '--run-rusanov-low-mach-advection' ]", target)
        self.assertIn("'atmospherebench-all-speed-rusanov-low-mach-advection'", target)
        self.assertIn("args: [ '--run-all-speed-rusanov-low-mach-advection' ]", target)
        self.assertIn("args: [ '--run-hllc-rusanov-fallback-low-mach-advection' ]", target)
        self.assertIn("args: [ '--run-hllc-rusanov-fallback-near-vacuum-expansion' ]", target)
        self.assertIn("args: [ '--run-hllc-rusanov-fallback-sod-shock-tube' ]", target)
        self.assertIn("args: [ '--run-hllc-rusanov-fallback-open-boundary-leak' ]", target)
        self.assertIn("args: [ '--run-hllc-rusanov-fallback-performance' ]", target)
        self.assertIn("args: [ '--run-hllc-2d-uniform' ]", target)
        self.assertIn("args: [ '--run-hllc-2d-pressure-pulse' ]", target)
        self.assertIn("args: [ '--run-hllc-2d-sealed-heating' ]", target)
        self.assertIn("args: [ '--run-hllc-2d-natural-convection' ]", target)
        self.assertIn("args: [ '--run-hllc-2d-species-mixing' ]", target)
        self.assertIn("args: [ '--run-hllc-2d-performance' ]", target)
        self.assertIn("args: [ '--run-lbm-d2q9-uniform' ]", target)
        self.assertIn("args: [ '--run-lbm-d2q9-shear-wave' ]", target)
        self.assertIn("args: [ '--run-legacy-like-uniform' ]", target)
        self.assertIn("args: [ '--run-legacy-like-pressure-pulse' ]", target)
        self.assertIn("args: [ '--run-hybrid-all-speed-policy' ]", target)
        self.assertIn("'atmospherebench-rusanov-open-boundary-leak'", target)
        self.assertIn("args: [ '--run-rusanov-open-boundary-leak' ]", target)
        self.assertIn("'atmospherebench-rusanov-performance'", target)
        self.assertIn("args: [ '--run-rusanov-performance' ]", target)
        source = (BENCH_ROOT / "AtmosphereBench.cpp").read_text(encoding="utf-8")
        self.assertIn('"registered_only"', source)
        self.assertIn('"implemented_1d_uniform_pressure_pulse_density_advection_contact_near_vacuum_sod_refinement_low_mach_open_leak_performance_probes"', source)
        self.assertIn('"implemented_1d_low_mach_probe_rejected"', source)
        self.assertIn('"implemented_1d_low_mach_near_vacuum_sod_open_leak_performance_and_2d_uniform_pressure_pulse_sealed_heating_natural_convection_species_mixing_performance_probes"', source)
        self.assertIn("RUSANOV_UNIFORM_PROBE", source)
        self.assertIn("RUSANOV_PRESSURE_PULSE_PROBE", source)
        self.assertIn("RUSANOV_DENSITY_ADVECTION_PROBE", source)
        self.assertIn("RUSANOV_CONTACT_DISCONTINUITY_PROBE", source)
        self.assertIn("RUSANOV_NEAR_VACUUM_EXPANSION_PROBE", source)
        self.assertIn("RUSANOV_SOD_SHOCK_TUBE_PROBE", source)
        self.assertIn("RUSANOV_DENSITY_ADVECTION_REFINEMENT_PROBE", source)
        self.assertIn("RUSANOV_LOW_MACH_ADVECTION_PROBE", source)
        self.assertIn("RUSANOV_OPEN_BOUNDARY_LEAK_PROBE", source)
        self.assertIn("HLLC_2D_SEALED_HEATING_PROBE", source)
        self.assertIn("HLLC_2D_NATURAL_CONVECTION_PROBE", source)
        self.assertIn("HLLC_2D_SPECIES_MIXING_PROBE", source)
        self.assertIn("LBM_D2Q9_UNIFORM_PROBE", source)
        self.assertIn("LBM_D2Q9_SHEAR_WAVE_PROBE", source)
        self.assertIn("LEGACY_LIKE_UNIFORM_PROBE", source)
        self.assertIn("LEGACY_LIKE_PRESSURE_PULSE_PROBE", source)
        self.assertIn("HYBRID_ALL_SPEED_POLICY_PROBE", source)
        self.assertIn("HYBRID_ALL_SPEED_POLICY_SELECTION_READY", source)
        self.assertIn("=UNSELECTED", source)
        self.assertIn("synthetic_nondimensional", source)
        self.assertNotIn("287.05", source)

    def test_low_mach_projection_is_an_isolated_component_probe(self) -> None:
        header = (BENCH_ROOT / "LowMachProjection2D.h").read_text(encoding="utf-8")
        source = (BENCH_ROOT / "LowMachProjection2D.cpp").read_text(encoding="utf-8")
        main = (BENCH_ROOT / "main.cpp").read_text(encoding="utf-8")
        atmosphere = (BENCH_ROOT / "AtmosphereBench.cpp").read_text(encoding="utf-8")
        self.assertIn("LowMachProjection2DSummary", header)
        self.assertIn("RunLowMachProjection2D", header)
        self.assertIn("DivergenceL2", source)
        self.assertIn("RunConjugateGradient", source)
        self.assertIn("ApplyPositivePoissonOperator", source)
        self.assertIn("soundSpeedIndependent = true", source)
        self.assertIn("conjugateGradientWithinReferenceBudget", source)
        self.assertIn("candidate_solver_implemented=false", source)
        self.assertIn("production_runtime_integration=not_implemented", source)
        self.assertIn("--run-low-mach-projection-2d", main)
        self.assertIn("RunLowMachProjection2D", atmosphere)
        self.assertIn("LOW_MACH_PROJECTION_2D_PROBE", atmosphere)

    def test_low_mach_projection_target_matrix_is_strict_and_fail_closed(self) -> None:
        header = (BENCH_ROOT / "LowMachProjection2DPerformance.h").read_text(encoding="utf-8")
        source = (BENCH_ROOT / "LowMachProjection2DPerformance.cpp").read_text(encoding="utf-8")
        main = (BENCH_ROOT / "main.cpp").read_text(encoding="utf-8")
        meson = (ROOT / "meson.build").read_text(encoding="utf-8")
        self.assertIn("LowMachProjection2DPerformanceSummary", header)
        self.assertIn("RunSample<153, 96>", source)
        self.assertIn("RunSample<306, 192>", source)
        self.assertIn("RunSample<612, 384>", source)
        self.assertIn("MaximumIterations = 500", source)
        self.assertIn("--run-low-mach-projection-2d-performance", main)
        self.assertIn("atmospherebench-low-mach-projection-2d-performance", meson)

    def test_fvm_candidates_are_strict_double_and_hlle_remains_registered_only(self) -> None:
        header = (BENCH_ROOT / "Rusanov1D.h").read_text(encoding="utf-8")
        source = (BENCH_ROOT / "Rusanov1D.cpp").read_text(encoding="utf-8")
        self.assertIn("RusanovProbeSummary", header)
        self.assertIn("RunRusanovUniform", header)
        self.assertIn("RunRusanovPressurePulse", header)
        self.assertIn("RunRusanovDensityAdvection", header)
        self.assertIn("RunRusanovContactDiscontinuity", header)
        self.assertIn("RunRusanovNearVacuumExpansion", header)
        self.assertIn("RunRusanovSodShockTube", header)
        self.assertIn("RunRusanovOpenBoundaryLeak", header)
        self.assertIn("RunAllSpeedRusanovLowMachAdvection", header)
        self.assertIn("RunHllcRusanovFallbackLowMachAdvection", header)
        self.assertIn("RunHllcRusanovFallbackNearVacuumExpansion", header)
        self.assertIn("RunHllcRusanovFallbackSodShockTube", header)
        self.assertIn("RunHllcRusanovFallbackContract", header)
        self.assertIn("RusanovFluxX", source)
        self.assertIn("RunPeriodicProbe", source)
        self.assertIn("maximumWaveSpeed", source)
        self.assertIn("std::vector<ConservativeState>", source)
        self.assertIn("BoundaryMode::Periodic", source)
        self.assertIn("summary.ledger.Closes(conservationTolerance)", source)
        self.assertIn("eos, false, false, 1e-12", source)
        self.assertIn("eos, true, true, 1e-10", source)
        self.assertIn("summary.corrections.IsEmpty()", source)
        self.assertIn("pressurePeakReduced", source)
        self.assertIn("stateChangeL1", source)
        self.assertIn("densityL1Error", source)
        self.assertIn("advectionReferencePassed", source)
        self.assertIn("densityBoundsPreserved", source)
        self.assertIn("lowDensityRegionMassIncreased", source)
        self.assertIn("boundaryLedgerCloses", source)
        self.assertIn("leftBoundaryExchange", source)
        self.assertIn("rightBoundaryExchange", source)
        self.assertIn("BoundaryMode::Open", source)
        self.assertIn("shockReferencePassed", source)
        self.assertIn("RusanovRefinementSummary", header)
        self.assertIn("RusanovPerformanceSummary", header)
        self.assertIn("MeasurePerformanceCase", source)
        self.assertIn("std::chrono::steady_clock", source)
        self.assertIn('output << "state_evolved="', source)
        self.assertIn('output << "momentum_drift="', source)
        self.assertIn('output << "correction_event_count="', source)
        atmosphere = (BENCH_ROOT / "AtmosphereBench.cpp").read_text(encoding="utf-8")
        self.assertIn('"fvm_hlle", "registered_only", false', atmosphere)
        self.assertIn('"lbm_d2q9",', atmosphere)
        self.assertIn('"implemented_isothermal_uniform_shear_wave_only", true', atmosphere)
        self.assertIn('"fvm_all_speed_rusanov",', atmosphere)
        self.assertIn('"fvm_hllc_rusanov_fallback",', atmosphere)

    def test_cpp_scale_fixture_matches_the_unselected_json_candidate(self) -> None:
        candidate = json.loads(
            (ROOT / "resources/omnicore/v1/physical-scale-candidates.json").read_text(encoding="utf-8")
        )["geometry_candidates"][0]
        header = (BENCH_ROOT / "AtmosphereBench.h").read_text(encoding="utf-8")
        expected = {
            "pixelLengthM": candidate["pixel_length"]["value"],
            "cellLengthM": candidate["atmosphere_cell_length"]["value"],
            "effectiveDepthM": candidate["effective_depth"]["value"],
        }
        for field, value in expected.items():
            match = re.search(rf"{field}\s*=\s*([0-9.]+)", header)
            self.assertIsNotNone(match, field)
            self.assertEqual(Decimal(match.group(1)), Decimal(value), field)

    def test_shared_contract_types_remain_solver_free_and_nondimensional(self) -> None:
        header = (BENCH_ROOT / "AtmosphereBench.h").read_text(encoding="utf-8")
        source = (BENCH_ROOT / "AtmosphereBench.cpp").read_text(encoding="utf-8")
        for symbol in (
            "AtmosphereGrid", "BenchmarkCase", "NumericalCorrectionLedger",
            "ConservativeSourceLedger",
            "BenchmarkResult", "NondimensionalContract",
        ):
            with self.subTest(symbol=symbol):
                self.assertIn(symbol, header)
        self.assertIn('"case_time_domain=nondimensional_contract', source)
        self.assertIn('"case_step_count="', source)
        self.assertIn('"state_bytes_per_cell="', source)
        self.assertIn("!invalidGrid.IsValid()", source)
        self.assertIn("!invalidCase.IsValid()", source)
        self.assertIn("eventCount", header)
        self.assertIn("!uncountedCorrections.IsConsistent()", source)
        self.assertIn("ClosesWithSources", source)
        self.assertIn("ClosesWithSourcesAndBoundary", source)
        self.assertIn("!uncountedSource.IsConsistent()", source)
        self.assertIn("nonfiniteSourceRejected", source)

    def test_species_fixture_is_passive_conserved_and_fail_closed(self) -> None:
        header = (BENCH_ROOT / "Species2D.h").read_text(encoding="utf-8")
        source = (BENCH_ROOT / "Species2D.cpp").read_text(encoding="utf-8")
        self.assertIn("Hllc2DSpeciesMixingSummary", header)
        self.assertIn("RunHllc2DSpeciesMixing", header)
        self.assertIn("UpwindSpeciesFlux", source)
        self.assertIn("speciesA[index] - lambda", source)
        self.assertIn("summary.speciesLedgerCloses", source)
        self.assertIn("summary.speciesBoundsPreserved", source)
        self.assertIn("CompositionVariationDecreaseTolerance", source)
        self.assertIn('output << "species_eos_coupling=not_implemented', source)
        self.assertIn('output << "physical_diffusion=not_implemented', source)

    def test_hllc_2d_performance_uses_target_grids_and_steady_clock(self) -> None:
        header = (BENCH_ROOT / "Hllc2D.h").read_text(encoding="utf-8")
        source = (BENCH_ROOT / "Hllc2D.cpp").read_text(encoding="utf-8")
        self.assertIn("Hllc2DPerformanceSummary", header)
        self.assertIn("RunHllc2DPerformance", header)
        self.assertIn("PerformanceLegacyCellsX = 153", source)
        self.assertIn("PerformanceLegacyCellsY = 96", source)
        self.assertIn("PerformanceParticleCellsX = 612", source)
        self.assertIn("PerformanceParticleCellsY = 384", source)
        self.assertIn("std::chrono::steady_clock::now", source)
        self.assertIn("millisecondsPerStep", source)
        self.assertIn('output << "performance_budget_status=unselected', source)

    def test_lbm_d2q9_is_actual_isothermal_candidate_with_explicit_limits(self) -> None:
        header = (BENCH_ROOT / "LbmD2Q9.h").read_text(encoding="utf-8")
        source = (BENCH_ROOT / "LbmD2Q9.cpp").read_text(encoding="utf-8")
        self.assertIn("LbmD2Q9ProbeSummary", header)
        self.assertIn("RunLbmD2Q9Uniform", header)
        self.assertIn("RunLbmD2Q9ShearWave", header)
        self.assertIn("Directions = 9", source)
        self.assertIn("Equilibrium", source)
        self.assertIn("RelaxationTime = 0.8", source)
        self.assertIn("shearAmplitudeRelativeError <= 0.02", source)
        self.assertIn('output << "energy_conservation=not_applicable_no_energy_state', source)
        self.assertIn('output << "near_vacuum_support=unsupported', source)
        self.assertIn('output << "shock_support=unsupported', source)

    def test_legacy_like_is_actual_control_with_explicit_nonphysical_fields(self) -> None:
        header = (BENCH_ROOT / "LegacyLike.h").read_text(encoding="utf-8")
        source = (BENCH_ROOT / "LegacyLike.cpp").read_text(encoding="utf-8")
        self.assertIn("LegacyLikeProbeSummary", header)
        self.assertIn("RunLegacyLikeUniform", header)
        self.assertIn("RunLegacyLikePressurePulse", header)
        self.assertIn("dimensionless_pressure_velocity_stencil", source)
        self.assertIn("production_air_equivalence=not_claimed", source)
        self.assertIn("physical_mass_state=not_implemented", source)
        self.assertIn("mass_drift=not_applicable_no_mass_state", source)
        self.assertIn("energy_drift=not_applicable_no_energy_state", source)
        self.assertIn("summary.corrections.IsEmpty()", source)

    def test_hybrid_policy_probe_is_uncoupled_and_not_solver_selection(self) -> None:
        header = (BENCH_ROOT / "HybridPolicy1D.h").read_text(encoding="utf-8")
        source = (BENCH_ROOT / "HybridPolicy1D.cpp").read_text(encoding="utf-8")
        atmosphere = (BENCH_ROOT / "AtmosphereBench.cpp").read_text(encoding="utf-8")
        self.assertIn("HybridPolicyProbeSummary", header)
        self.assertIn("RunHybridAllSpeedPolicyProbe", header)
        self.assertIn("conservative_constant_pressure_transport", source)
        self.assertIn("hllc_rusanov_fallback_whole_case", source)
        self.assertIn("cross_route_boundary_coupling=not_implemented", source)
        self.assertIn("event_local_subcycling=not_implemented", source)
        self.assertIn("router_implemented=false", source)
        self.assertIn("mixed_region_reflux_conservation=not_implemented", source)
        self.assertIn("general_low_mach_pressure_coupling=not_implemented", source)
        self.assertIn("transport_step_count_independent_of_sound_speed=", source)
        self.assertIn("hybrid_end_to_end_passed=false", source)
        self.assertIn("component_probe_passed=", source)
        self.assertIn("policy_selection_ready=false", source)
        self.assertIn('"hybrid_all_speed_components",', atmosphere)
        self.assertIn('"implemented_uncoupled_low_mach_transport_and_whole_case_hllc_sod_policy_probe_not_solver", false', atmosphere)

    def test_hybrid_mixed_region_probe_has_real_router_reflux_and_explicit_limits(self) -> None:
        header = (BENCH_ROOT / "HybridMixedRegion1D.h").read_text(encoding="utf-8")
        source = (BENCH_ROOT / "HybridMixedRegion1D.cpp").read_text(encoding="utf-8")
        main = (BENCH_ROOT / "main.cpp").read_text(encoding="utf-8")
        self.assertIn("HybridMixedRegionProbeSummary", header)
        self.assertIn("RunHybridMixedRegionProbe", header)
        self.assertIn("router_implemented=true", source)
        self.assertIn("cross_route_boundary_coupling=implemented_1d_probe", source)
        self.assertIn("event_local_subcycling=implemented_1d_probe", source)
        self.assertIn("mixed_region_reflux_conservation=implemented_1d_probe", source)
        self.assertIn("interfaceEventExchange", source)
        self.assertIn("interfaceBulkExchange", source)
        self.assertIn("threshold_scan_passed=", source)
        self.assertIn("two_dimensional_hybrid_coupling=not_implemented", source)
        self.assertIn("physical_event_local_domain_of_dependence=not_implemented", source)
        self.assertIn("hybrid_end_to_end_passed=false", source)
        self.assertIn("--run-hybrid-mixed-region", main)

    def test_scaffold_contract_is_not_a_runtime_consumer(self) -> None:
        production = []
        for path in (ROOT / "src").rglob("*"):
            if path.suffix in {".cpp", ".h"}:
                text = path.read_text(encoding="utf-8")
                if "atmospherebench" in text.lower() or "physical-scale-candidates" in text:
                    production.append(str(path.relative_to(ROOT)))
        self.assertEqual(production, [])

    def test_hybrid_mixed_region_2d_probe_has_bounded_domain_evidence(self) -> None:
        header = (BENCH_ROOT / "HybridMixedRegion2D.h").read_text(encoding="utf-8")
        source = (BENCH_ROOT / "HybridMixedRegion2D.cpp").read_text(encoding="utf-8")
        main = (BENCH_ROOT / "main.cpp").read_text(encoding="utf-8")
        self.assertIn("HybridMixedRegion2DProbeSummary", header)
        self.assertIn("RunHybridMixedRegion2DProbe", header)
        self.assertIn("cross_route_boundary_coupling=implemented_2d_probe", source)
        self.assertIn("mixed_region_reflux_conservation=implemented_2d_probe", source)
        self.assertIn("physical_acoustic_domain_cells=", source)
        self.assertIn("target_grid_event_fraction_performance=matrix_measured_end_to_end_short_run", source)
        self.assertIn("physical_event_local_domain_of_dependence=not_implemented", source)
        self.assertIn("hybrid_near_vacuum_routing=implemented_fixture_only", source)
        self.assertIn("species_cross_route_transport=implemented_passive_interface_fixture_only", source)
        self.assertIn("passive_species_cross_route_ledger_passed=", source)
        self.assertIn("near_vacuum_evolution_passed=", source)
        self.assertIn("passive_species_evolution_passed=", source)
        self.assertIn("7 * sizeof(ConservativeState)", source)
        self.assertIn("--run-hybrid-mixed-region-2d", main)

    def test_source_package_keeps_physical_scale_and_bench_tools(self) -> None:
        tools = {
            "tools/physical_scale_check.py",
            "tools/atmosphere_policy_check.py",
            "tools/run_atmospherebench.ps1",
            "tools/atmospherebench/AtmosphereBench.cpp",
            "tools/atmospherebench/AtmosphereBench.h",
            "tools/atmospherebench/main.cpp",
            "tools/atmospherebench/Rusanov1D.cpp",
            "tools/atmospherebench/Rusanov1D.h",
            "tools/atmospherebench/LbmD2Q9.cpp",
            "tools/atmospherebench/LbmD2Q9.h",
            "tools/atmospherebench/LegacyLike.cpp",
            "tools/atmospherebench/LegacyLike.h",
            "tools/atmospherebench/LowMachProjection2D.cpp",
            "tools/atmospherebench/LowMachProjection2D.h",
            "tools/atmospherebench/LowMachProjection2DPerformance.cpp",
            "tools/atmospherebench/LowMachProjection2DPerformance.h",
            "tools/atmospherebench/PrecisionMatrix.cpp",
            "tools/atmospherebench/HybridPolicy1D.cpp",
            "tools/atmospherebench/HybridPolicy1D.h",
            "tools/atmospherebench/HybridMixedRegion1D.cpp",
            "tools/atmospherebench/HybridMixedRegion1D.h",
            "tools/atmospherebench/HybridMixedRegion2D.cpp",
            "tools/atmospherebench/HybridMixedRegion2D.h",
            "tools/atmospherebench/LowMachProjection2D.cpp",
            "tools/atmospherebench/LowMachProjection2D.h",
            "tools/atmospherebench/Species2D.cpp",
            "tools/atmospherebench/Species2D.h",
            "tools/run_atmosphere_precision_matrix.py",
        }
        required = tools | {
            "resources/omnicore/v1/physical-scale-candidates.json",
            "resources/omnicore/v1/atmosphere-policy-candidates.json",
        }
        self.assertTrue(tools.issubset(package_tool.ALLOWED_TOOLS))
        self.assertTrue(required.issubset(package_tool.REQUIRED_MEMBERS))
        for member in required:
            with self.subTest(member=member):
                self.assertFalse(package_tool.is_excluded(member))


if __name__ == "__main__":
    unittest.main()
