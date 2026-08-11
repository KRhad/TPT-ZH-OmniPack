#include "Phase5Selection.h"

#include "AtmosphereBench.h"

#include <cmath>
#include <ostream>

namespace omni::atmospherebench
{

namespace
{
	constexpr double PhysicalSecondsPerTick = 1.0 / 60.0;
	constexpr double ReferenceAtmosphereBudgetMilliseconds = 1000.0 / 60.0 / 4.0;
	constexpr double ReferenceSoundSpeedMPerS = 344.0;
}

Phase5SelectionSummary RunPhase5Selection()
{
	Phase5SelectionSummary summary;
	const PhysicalScale scale;
	summary.physicalSecondsPerTick = PhysicalSecondsPerTick;
	summary.referenceAtmosphereBudgetMilliseconds = ReferenceAtmosphereBudgetMilliseconds;
	summary.referenceSoundSpeedMPerS = ReferenceSoundSpeedMPerS;
	summary.physicalAcousticDomainCells = static_cast<std::size_t>(std::ceil(
		ReferenceSoundSpeedMPerS * PhysicalSecondsPerTick / scale.cellLengthM));
	summary.physicalScaleValid = scale.IsValid();
	summary.presentationIndependent = true;
	summary.uniformAcousticScalingUsed = false;
	summary.strictDoubleReferenceSelected = true;
	summary.lowMachComponentSelected = true;
	summary.compressibleComponentSelected = true;
	summary.productionSolverImplemented = false;
	summary.selectionValidated = summary.physicalScaleValid
		&& summary.physicalAcousticDomainCells == 1434
		&& summary.presentationIndependent
		&& !summary.uniformAcousticScalingUsed
		&& summary.strictDoubleReferenceSelected
		&& summary.lowMachComponentSelected
		&& summary.compressibleComponentSelected
		&& !summary.productionSolverImplemented;
	return summary;
}

bool WritePhase5Selection(std::ostream &output)
{
	const auto summary = RunPhase5Selection();
	const PhysicalScale scale;
	output << "schema_version=1\n";
	output << "case=phase5_physical_scale_time_solver_selection\n";
	output << "result_status=phase5_selection_contract\n";
	output << "target_version=1.0.5\n";
	output << "physical_scale_selection=selected_tpt_mm_scale_v1\n";
	output << "pixel_length_m=" << scale.pixelLengthM << '\n';
	output << "atmosphere_cell_length_m=" << scale.cellLengthM << '\n';
	output << "effective_depth_m=" << scale.effectiveDepthM << '\n';
	output << "particle_parcel_volume_m3=" << scale.ParticleParcelVolumeM3() << '\n';
	output << "atmosphere_cell_volume_m3=" << scale.AtmosphereCellVolumeM3() << '\n';
	output << "world_width_m=" << scale.WorldWidthM() << '\n';
	output << "world_height_m=" << scale.WorldHeightM() << '\n';
	output << "physical_scale_valid=" << (summary.physicalScaleValid ? "true" : "false") << '\n';
	output << "physical_time_policy=selected_all_speed_split_v1\n";
	output << "simulation_tick_fixed=true\n";
	output << "presentation_independent=" << (summary.presentationIndependent ? "true" : "false") << '\n';
	output << "physical_seconds_per_tick=" << summary.physicalSecondsPerTick << '\n';
	output << "reference_sound_speed_m_per_s=" << summary.referenceSoundSpeedMPerS << '\n';
	output << "physical_acoustic_domain_cells=" << summary.physicalAcousticDomainCells << '\n';
	output << "uniform_acoustic_scaling_used=" << (summary.uniformAcousticScalingUsed ? "true" : "false") << '\n';
	output << "bulk_low_mach_time_integration=advective_step_plus_geometric_multigrid_projection\n";
	output << "compressible_event_time_integration=hllc_rusanov_fallback_cfl_subcycling\n";
	output << "compressible_event_region_policy=dynamic_expansion_up_to_full_domain\n";
	output << "overload_policy=correctness_first_allow_tick_slowdown\n";
	output << "reference_atmosphere_budget_milliseconds="
		<< summary.referenceAtmosphereBudgetMilliseconds << '\n';
	output << "reference_budget_scope=normal_153x96_low_mach_bulk_component\n";
	output << "full_domain_compressible_budget=yellow_measured_over_budget\n";
	output << "accepted_precision_policy=strict_double_cpu_reference\n";
	output << "fast_math_policy=disabled_for_selected_reference\n";
	output << "atmosphere_solver_selection=selected_hybrid_fvm_projection_hllc_rusanov_v1\n";
	output << "authoritative_state=rho_rho_u_rho_v_rho_E_species_partial_densities\n";
	output << "selected_low_mach_component=geometric_multigrid_v_cycle\n";
	output << "selected_compressible_flux=hllc_with_rusanov_fallback\n";
	output << "legacy_like_disposition=classic_control_only\n";
	output << "rusanov_disposition=strict_double_debug_floor_and_fallback\n";
	output << "lbm_disposition=reject_as_unified_solver_isothermal_no_energy_vacuum_shock\n";
	output << "hlle_disposition=deferred_optional_fallback_not_required_for_1_0_6_mvp\n";
	output << "candidate_solver_implemented=false\n";
	output << "production_solver_implemented="
		<< (summary.productionSolverImplemented ? "true" : "false") << '\n';
	output << "production_runtime_integration=deferred_to_1.0.6\n";
	output << "phase5_selection_validated=" << (summary.selectionValidated ? "true" : "false") << '\n';
	output << "v1_0_5_gate_ready_for_final_validation="
		<< (summary.selectionValidated ? "true" : "false") << '\n';
	return summary.selectionValidated;
}

} // namespace omni::atmospherebench
