#include "HybridPolicy1D.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <ostream>
#include <vector>

namespace omni::atmospherebench
{

namespace
{
	constexpr double Gamma = 5.0 / 3.0;
	constexpr double SpecificGasConstant = 1.0;
	constexpr std::size_t Cells = 128;
	constexpr double CellLength = 1.0 / static_cast<double>(Cells);
	constexpr double ShiftDistance = 0.125;
	constexpr double TargetAdvectiveCfl = 0.45;
	constexpr double ModerateVelocity = 0.5;
	constexpr double LowVelocity = 0.05;
	constexpr double VeryLowVelocity = 0.005;
	constexpr double DensityAmplitude = 0.2;
	constexpr double Pi = 3.141592653589793238462643383279502884;

	ConservativeState Add(const ConservativeState &left, const ConservativeState &right)
	{
		return {
			left.density + right.density,
			left.momentumX + right.momentumX,
			left.momentumY + right.momentumY,
			left.totalEnergyDensity + right.totalEnergyDensity,
		};
	}

	ConservativeState Subtract(const ConservativeState &left, const ConservativeState &right)
	{
		return {
			left.density - right.density,
			left.momentumX - right.momentumX,
			left.momentumY - right.momentumY,
			left.totalEnergyDensity - right.totalEnergyDensity,
		};
	}

	ConservativeState Scale(double factor, const ConservativeState &state)
	{
		return {
			factor * state.density,
			factor * state.momentumX,
			factor * state.momentumY,
			factor * state.totalEnergyDensity,
		};
	}

	ConservativeState TotalState(const std::vector<ConservativeState> &cells)
	{
		ConservativeState total;
		for (const auto &cell : cells)
			total = Add(total, cell);
		return total;
	}

	double DensityVariation(const std::vector<ConservativeState> &cells)
	{
		double variation = 0.0;
		for (std::size_t index = 0; index < cells.size(); ++index)
			variation += std::abs(cells[(index + 1) % cells.size()].density - cells[index].density);
		return variation;
	}

	HybridTransportProbeSummary RunTransport(double velocity)
	{
		const IdealGasEOS eos(Gamma, SpecificGasConstant);
		const double totalTime = ShiftDistance / velocity;
		const double maximumTimeStep = TargetAdvectiveCfl * CellLength / velocity;
		const auto steps = static_cast<std::size_t>(std::ceil(totalTime / maximumTimeStep));
		const double timeStep = totalTime / static_cast<double>(steps);
		HybridTransportProbeSummary summary{
			{"hybrid_low_mach_transport_1d",
				{Cells, 1, CellLength, BoundaryMode::Periodic},
				TimeDomain::NondimensionalContract, timeStep, steps}
		};
		summary.referenceVelocity = velocity;
		summary.simulatedTime = totalTime;
		summary.maximumAdvectiveCfl = velocity * timeStep / CellLength;
		summary.referenceShiftCells = static_cast<std::size_t>(
			std::llround(ShiftDistance / CellLength));

		std::vector<ConservativeState> initial(Cells);
		for (std::size_t cell = 0; cell < Cells; ++cell)
		{
			const double x = (static_cast<double>(cell) + 0.5) * CellLength;
			const double density = 1.0 + DensityAmplitude * std::sin(2.0 * Pi * x);
			initial[cell] = eos.FromPrimitive(density, velocity, 0.0, 1.0);
		}
		std::vector<ConservativeState> cells = initial;
		std::vector<ConservativeState> next(Cells);
		summary.stateAndScratchBytesTotal = 3 * Cells * sizeof(ConservativeState);
		summary.ledger.Begin(TotalState(cells));
		const double lambdaVelocity = timeStep * velocity / CellLength;
		if (!summary.benchmarkCase.IsValid() || !eos.IsValid()
			|| lambdaVelocity <= 0.0 || lambdaVelocity > 1.0)
			return summary;

		for (std::size_t step = 0; step < steps; ++step)
		{
			for (std::size_t cell = 0; cell < Cells; ++cell)
			{
				const std::size_t left = (cell + Cells - 1) % Cells;
				next[cell] = Subtract(cells[cell],
					Scale(lambdaVelocity, Subtract(cells[cell], cells[left])));
				if (!eos.ToPrimitive(next[cell]).valid)
					return summary;
			}
			cells.swap(next);
		}

		summary.minimumDensity = std::numeric_limits<double>::infinity();
		summary.maximumDensity = 0.0;
		summary.minimumPressure = std::numeric_limits<double>::infinity();
		summary.positivityPreserved = true;
		for (std::size_t cell = 0; cell < Cells; ++cell)
		{
			const auto primitive = eos.ToPrimitive(cells[cell]);
			if (!primitive.valid)
			{
				summary.positivityPreserved = false;
				break;
			}
			summary.minimumDensity = std::min(summary.minimumDensity, primitive.density);
			summary.maximumDensity = std::max(summary.maximumDensity, primitive.density);
			summary.minimumPressure = std::min(summary.minimumPressure, primitive.pressure);
			const auto &reference = initial[(cell + Cells - summary.referenceShiftCells) % Cells];
			const double densityError = std::abs(cells[cell].density - reference.density);
			summary.densityL1Error += densityError;
			summary.densityLinfError = std::max(summary.densityLinfError, densityError);
			summary.pressureLinfError = std::max(
				summary.pressureLinfError, std::abs(primitive.pressure - 1.0));
		}
		summary.densityL1Error /= static_cast<double>(Cells);
		const double initialVariation = DensityVariation(initial);
		summary.totalVariationRatio = initialVariation > 0.0
			? DensityVariation(cells) / initialVariation
			: 0.0;
		summary.densityBoundsPreserved = summary.minimumDensity >= 1.0 - DensityAmplitude - 1e-12
			&& summary.maximumDensity <= 1.0 + DensityAmplitude + 1e-12;
		summary.advectionReferencePassed = summary.densityL1Error <= 0.05
			&& summary.densityLinfError <= 0.08
			&& summary.pressureLinfError <= 1e-10
			&& summary.totalVariationRatio >= 0.8
			&& summary.totalVariationRatio <= 1.0 + 1e-10
			&& summary.densityBoundsPreserved;
		summary.ledger.End(TotalState(cells));
		summary.passed = summary.positivityPreserved
			&& summary.ledger.Closes(1e-9)
			&& summary.corrections.IsEmpty()
			&& summary.maximumAdvectiveCfl <= 1.0
			&& summary.advectionReferencePassed;
		return summary;
	}

	void WriteTransport(std::ostream &output, const char *prefix,
		const HybridTransportProbeSummary &summary)
	{
		output << prefix << "_velocity=" << summary.referenceVelocity << '\n';
		output << prefix << "_steps=" << summary.benchmarkCase.stepCount << '\n';
		output << prefix << "_simulated_time=" << summary.simulatedTime << '\n';
		output << prefix << "_advective_cfl=" << summary.maximumAdvectiveCfl << '\n';
		output << prefix << "_density_l1_error=" << summary.densityL1Error << '\n';
		output << prefix << "_density_linf_error=" << summary.densityLinfError << '\n';
		output << prefix << "_pressure_linf_error=" << summary.pressureLinfError << '\n';
		output << prefix << "_total_variation_ratio=" << summary.totalVariationRatio << '\n';
		output << prefix << "_mass_drift="
			<< summary.ledger.final.density - summary.ledger.initial.density << '\n';
		output << prefix << "_momentum_x_drift="
			<< summary.ledger.final.momentumX - summary.ledger.initial.momentumX << '\n';
		output << prefix << "_energy_drift="
			<< summary.ledger.final.totalEnergyDensity
				- summary.ledger.initial.totalEnergyDensity << '\n';
		output << prefix << "_passed=" << (summary.passed ? "true" : "false") << '\n';
	}
} // namespace

HybridPolicyProbeSummary RunHybridAllSpeedPolicyProbe()
{
	HybridPolicyProbeSummary summary{
		RunTransport(ModerateVelocity),
		RunTransport(LowVelocity),
		RunTransport(VeryLowVelocity),
		RunHllcRusanovFallbackSodShockTube(),
	};
	const double nominalSoundSpeed = std::sqrt(Gamma);
	summary.moderateNominalMach = ModerateVelocity / nominalSoundSpeed;
	summary.lowNominalMach = LowVelocity / nominalSoundSpeed;
	summary.veryLowNominalMach = VeryLowVelocity / nominalSoundSpeed;
	if (summary.moderateMach.densityL1Error > 0.0)
	{
		summary.lowToModerateL1Ratio =
			summary.lowMach.densityL1Error / summary.moderateMach.densityL1Error;
		summary.veryLowToModerateL1Ratio =
			summary.veryLowMach.densityL1Error / summary.moderateMach.densityL1Error;
	}
	summary.lowMachRouteCount = 3;
	summary.compressibleRouteCount = 1;
	summary.lowMachSuitabilityPassed = summary.veryLowMach.densityL1Error <= 0.05
		&& summary.veryLowMach.totalVariationRatio >= 0.8;
	summary.crossRouteBoundaryCouplingImplemented = false;
	summary.eventLocalSubcyclingImplemented = false;
	summary.policySelectionReady = false;
	summary.passed = summary.moderateMach.passed && summary.lowMach.passed
		&& summary.veryLowMach.passed && summary.compressibleSod.passed
		&& summary.lowMachSuitabilityPassed
		&& std::isfinite(summary.lowToModerateL1Ratio)
		&& std::isfinite(summary.veryLowToModerateL1Ratio);
	return summary;
}

bool WriteHybridAllSpeedPolicyProbe(std::ostream &output)
{
	const auto summary = RunHybridAllSpeedPolicyProbe();
	output << "schema_version=1\n";
	output << "case=hybrid_all_speed_policy_router_1d\n";
	output << "candidate=hybrid_all_speed_event_local\n";
	output << "candidate_solver_implemented=false\n";
	output << "policy_probe_implemented=true\n";
	output << "atmosphere_solver_selection=unselected\n";
	output << "physical_scale_selection=unselected\n";
	output << "physical_time_policy=unselected\n";
	output << "result_status=policy_probe_not_solver_selection\n";
	output << "case_time_domain=nondimensional_contract\n";
	output << "boundary_mode=periodic_low_mach_and_sealed_sod\n";
	output << "grid_cells_x=" << Cells << '\n';
	output << "grid_cells_y=1\n";
	output << "grid_cell_count=" << Cells << '\n';
	output << "cell_length=" << CellLength << '\n';
	output << "case_timestep=" << summary.veryLowMach.benchmarkCase.timeStep << '\n';
	output << "case_step_count=" << summary.veryLowMach.benchmarkCase.stepCount << '\n';
	output << "state_bytes_per_cell=" << sizeof(ConservativeState) << '\n';
	output << "state_and_flux_scratch_bytes_per_cell=" << (3 * sizeof(ConservativeState)) << '\n';
	output << "state_and_flux_scratch_bytes_total="
		<< summary.veryLowMach.stateAndScratchBytesTotal << '\n';
	output << "low_mach_bulk_route=conservative_constant_pressure_transport\n";
	output << "compressible_event_route=hllc_rusanov_fallback_whole_case\n";
	output << "cross_route_boundary_coupling=not_implemented\n";
	output << "event_local_subcycling=not_implemented\n";
	output << "production_boundary_coupling=not_implemented\n";
	output << "low_mach_route_count=" << summary.lowMachRouteCount << '\n';
	output << "compressible_route_count=" << summary.compressibleRouteCount << '\n';
	output << "moderate_nominal_mach=" << summary.moderateNominalMach << '\n';
	output << "low_nominal_mach=" << summary.lowNominalMach << '\n';
	output << "very_low_nominal_mach=" << summary.veryLowNominalMach << '\n';
	WriteTransport(output, "moderate", summary.moderateMach);
	WriteTransport(output, "low", summary.lowMach);
	WriteTransport(output, "very_low", summary.veryLowMach);
	output << "low_to_moderate_l1_ratio=" << summary.lowToModerateL1Ratio << '\n';
	output << "very_low_to_moderate_l1_ratio=" << summary.veryLowToModerateL1Ratio << '\n';
	output << "low_mach_suitability_passed="
		<< (summary.lowMachSuitabilityPassed ? "true" : "false") << '\n';
	output << "compressible_sod_passed=" << (summary.compressibleSod.passed ? "true" : "false") << '\n';
	output << "compressible_sod_shock_reference_passed="
		<< (summary.compressibleSod.shockReferencePassed ? "true" : "false") << '\n';
	output << "compressible_sod_flux_fallback_count="
		<< summary.compressibleSod.fluxFallbackCount << '\n';
	output << "compressible_sod_correction_count="
		<< summary.compressibleSod.corrections.eventCount << '\n';
	output << "minimum_density=" << summary.veryLowMach.minimumDensity << '\n';
	output << "maximum_density=" << summary.veryLowMach.maximumDensity << '\n';
	output << "minimum_pressure=" << summary.veryLowMach.minimumPressure << '\n';
	output << "maximum_cfl=" << summary.veryLowMach.maximumAdvectiveCfl << '\n';
	output << "mass_drift="
		<< summary.veryLowMach.ledger.final.density
			- summary.veryLowMach.ledger.initial.density << '\n';
	output << "momentum_x_drift="
		<< summary.veryLowMach.ledger.final.momentumX
			- summary.veryLowMach.ledger.initial.momentumX << '\n';
	output << "momentum_y_drift="
		<< summary.veryLowMach.ledger.final.momentumY
			- summary.veryLowMach.ledger.initial.momentumY << '\n';
	output << "momentum_drift=" << std::hypot(
		summary.veryLowMach.ledger.final.momentumX - summary.veryLowMach.ledger.initial.momentumX,
		summary.veryLowMach.ledger.final.momentumY - summary.veryLowMach.ledger.initial.momentumY) << '\n';
	output << "energy_drift="
		<< summary.veryLowMach.ledger.final.totalEnergyDensity
			- summary.veryLowMach.ledger.initial.totalEnergyDensity << '\n';
	output << "numerical_correction_count=0\n";
	output << "correction_mass_added=0\n";
	output << "correction_mass_removed=0\n";
	output << "correction_momentum_x_added=0\n";
	output << "correction_momentum_y_added=0\n";
	output << "correction_energy_added=0\n";
	output << "correction_energy_removed=0\n";
	output << "density_floor_hits=0\n";
	output << "pressure_floor_hits=0\n";
	output << "correction_event_count=0\n";
	output << "policy_selection_ready=false\n";
	output << "candidate_disposition=continue_coupling_evaluation\n";
	output << "benchmark_execution_status=" << (summary.passed ? "PASS" : "FAIL") << '\n';
	output << "probe_passed=" << (summary.passed ? "true" : "false") << '\n';
	return summary.passed;
}

} // namespace omni::atmospherebench
