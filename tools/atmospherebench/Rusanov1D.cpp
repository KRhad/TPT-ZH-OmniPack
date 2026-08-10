#include "Rusanov1D.h"

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
	constexpr std::size_t Cells = 64;
	constexpr std::size_t Steps = 16;
	constexpr double CellLength = 1.0;
	constexpr double TimeStep = 0.05;

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

	ConservativeState FluxX(const ConservativeState &state, const PrimitiveState &primitive)
	{
		return {
			state.momentumX,
			state.momentumX * primitive.velocityX + primitive.pressure,
			state.momentumY * primitive.velocityX,
			primitive.velocityX * (state.totalEnergyDensity + primitive.pressure),
		};
	}

	ConservativeState RusanovFluxX(
		const ConservativeState &left,
		const ConservativeState &right,
		const IdealGasEOS &eos)
	{
		const auto leftPrimitive = eos.ToPrimitive(left);
		const auto rightPrimitive = eos.ToPrimitive(right);
		if (!leftPrimitive.valid || !rightPrimitive.valid)
			return {};
		const auto maximumWaveSpeed = std::max(
			std::abs(leftPrimitive.velocityX) + leftPrimitive.soundSpeed,
			std::abs(rightPrimitive.velocityX) + rightPrimitive.soundSpeed
		);
		return Subtract(
			Scale(0.5, Add(FluxX(left, leftPrimitive), FluxX(right, rightPrimitive))),
			Scale(0.5 * maximumWaveSpeed, Subtract(right, left))
		);
	}
}

RusanovProbeSummary RunRusanovUniform()
{
	RusanovProbeSummary summary{
		{"rusanov_uniform_1d", {Cells, 1, CellLength, BoundaryMode::Periodic},
			TimeDomain::NondimensionalContract, TimeStep, Steps},
	};
	const IdealGasEOS eos(Gamma, SpecificGasConstant);
	const auto initial = eos.FromPrimitive(1.0, 0.0, 0.0, 1.0);
	std::vector<ConservativeState> cells(Cells, initial);
	std::vector<ConservativeState> fluxes(Cells);
	std::vector<ConservativeState> next(Cells);
	if (!summary.benchmarkCase.IsValid() || !eos.IsValid() || !eos.ToPrimitive(initial).valid)
		return summary;
	summary.ledger.Begin(TotalState(cells));
	const auto primitive = eos.ToPrimitive(initial);
	const auto lambda = TimeStep / CellLength;
	summary.maximumCfl = lambda * (std::abs(primitive.velocityX) + primitive.soundSpeed);
	if (!std::isfinite(summary.maximumCfl) || summary.maximumCfl <= 0.0 || summary.maximumCfl > 1.0)
		return summary;

	for (std::size_t step = 0; step < Steps; ++step)
	{
		for (std::size_t cell = 0; cell < Cells; ++cell)
			fluxes[cell] = RusanovFluxX(cells[cell], cells[(cell + 1) % Cells], eos);
		for (std::size_t cell = 0; cell < Cells; ++cell)
		{
			const auto leftFace = fluxes[(cell + Cells - 1) % Cells];
			next[cell] = Subtract(cells[cell], Scale(lambda, Subtract(fluxes[cell], leftFace)));
		}
		cells.swap(next);
	}

	summary.minimumDensity = std::numeric_limits<double>::infinity();
	summary.minimumPressure = std::numeric_limits<double>::infinity();
	summary.minimumEnergyDensity = std::numeric_limits<double>::infinity();
	summary.positivityPreserved = true;
	for (const auto &cell : cells)
	{
		const auto cellPrimitive = eos.ToPrimitive(cell);
		if (!cellPrimitive.valid)
		{
			summary.positivityPreserved = false;
			break;
		}
		summary.minimumDensity = std::min(summary.minimumDensity, cellPrimitive.density);
		summary.minimumPressure = std::min(summary.minimumPressure, cellPrimitive.pressure);
		summary.minimumEnergyDensity = std::min(summary.minimumEnergyDensity, cell.totalEnergyDensity);
	}
	summary.ledger.End(TotalState(cells));
	summary.passed = summary.positivityPreserved && summary.ledger.Closes(1e-12)
		&& summary.corrections.IsEmpty() && summary.maximumCfl <= 1.0;
	return summary;
}

bool WriteRusanovUniformProbe(std::ostream &output)
{
	const auto summary = RunRusanovUniform();
	const auto &initial = summary.ledger.initial;
	const auto &final = summary.ledger.final;
	output << "schema_version=1\n";
	output << "case=" << summary.benchmarkCase.id << '\n';
	output << "candidate=fvm_rusanov\n";
	output << "candidate_solver_implemented=true\n";
	output << "atmosphere_solver_selection=unselected\n";
	output << "physical_scale_selection=unselected\n";
	output << "result_status=candidate_result_not_selection\n";
	output << "case_time_domain=nondimensional_contract\n";
	output << "grid_cells_x=" << summary.benchmarkCase.grid.cellsX << '\n';
	output << "grid_cells_y=" << summary.benchmarkCase.grid.cellsY << '\n';
	output << "grid_cell_count=" << summary.benchmarkCase.grid.CellCount() << '\n';
	output << "case_timestep=" << summary.benchmarkCase.timeStep << '\n';
	output << "case_step_count=" << summary.benchmarkCase.stepCount << '\n';
	output << "maximum_cfl=" << summary.maximumCfl << '\n';
	output << "initial_mass=" << initial.density << '\n';
	output << "final_mass=" << final.density << '\n';
	output << "mass_drift=" << (final.density - initial.density) << '\n';
	output << "momentum_x_drift=" << (final.momentumX - initial.momentumX) << '\n';
	output << "momentum_y_drift=" << (final.momentumY - initial.momentumY) << '\n';
	output << "energy_drift=" << (final.totalEnergyDensity - initial.totalEnergyDensity) << '\n';
	output << "minimum_density=" << summary.minimumDensity << '\n';
	output << "minimum_pressure=" << summary.minimumPressure << '\n';
	output << "minimum_energy_density=" << summary.minimumEnergyDensity << '\n';
	output << "positivity_preserved=" << (summary.positivityPreserved ? "true" : "false") << '\n';
	output << "numerical_correction_count=" << summary.corrections.eventCount << '\n';
	output << "state_bytes_per_cell=" << sizeof(ConservativeState) << '\n';
	output << "state_and_flux_scratch_bytes_per_cell=" << (3 * sizeof(ConservativeState)) << '\n';
	output << "probe_passed=" << (summary.passed ? "true" : "false") << '\n';
	return summary.passed;
}

} // namespace omni::atmospherebench
