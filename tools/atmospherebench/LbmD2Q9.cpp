#include "LbmD2Q9.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <ostream>
#include <vector>

namespace omni::atmospherebench
{

namespace
{
	constexpr std::size_t Directions = 9;
	constexpr std::array<int, Directions> DirectionX{{0, 1, 0, -1, 0, 1, -1, -1, 1}};
	constexpr std::array<int, Directions> DirectionY{{0, 0, 1, 0, -1, 1, 1, -1, -1}};
	constexpr std::array<double, Directions> Weights{{
		4.0 / 9.0,
		1.0 / 9.0, 1.0 / 9.0, 1.0 / 9.0, 1.0 / 9.0,
		1.0 / 36.0, 1.0 / 36.0, 1.0 / 36.0, 1.0 / 36.0,
	}};
	constexpr double SoundSpeedSquared = 1.0 / 3.0;
	constexpr double SoundSpeed = 0.57735026918962576451;
	constexpr double RelaxationTime = 0.8;
	constexpr double RelaxationRate = 1.0 / RelaxationTime;
	constexpr double KinematicViscosity = SoundSpeedSquared * (RelaxationTime - 0.5);
	constexpr std::size_t UniformCellsX = 64;
	constexpr std::size_t UniformCellsY = 48;
	constexpr std::size_t UniformSteps = 64;
	constexpr double UniformVelocityX = 0.02;
	constexpr double UniformVelocityY = -0.01;
	constexpr std::size_t ShearCellsX = 64;
	constexpr std::size_t ShearCellsY = 64;
	constexpr std::size_t ShearSteps = 128;
	constexpr double ShearAmplitude = 0.02;
	constexpr double Pi = 3.141592653589793238462643383279502884;

	using Populations = std::array<double, Directions>;

	struct MacroscopicState
	{
		double density = 0.0;
		double velocityX = 0.0;
		double velocityY = 0.0;
		bool valid = false;
	};

	Populations Equilibrium(double density, double velocityX, double velocityY)
	{
		Populations result{};
		const double velocitySquared = velocityX * velocityX + velocityY * velocityY;
		for (std::size_t direction = 0; direction < Directions; ++direction)
		{
			const double projectedVelocity = static_cast<double>(DirectionX[direction]) * velocityX
				+ static_cast<double>(DirectionY[direction]) * velocityY;
			result[direction] = Weights[direction] * density
				* (1.0 + 3.0 * projectedVelocity
					+ 4.5 * projectedVelocity * projectedVelocity
					- 1.5 * velocitySquared);
		}
		return result;
	}

	MacroscopicState Macroscopic(const Populations &populations)
	{
		MacroscopicState result;
		for (std::size_t direction = 0; direction < Directions; ++direction)
		{
			const double value = populations[direction];
			if (!std::isfinite(value))
				return result;
			result.density += value;
			result.velocityX += value * static_cast<double>(DirectionX[direction]);
			result.velocityY += value * static_cast<double>(DirectionY[direction]);
		}
		if (!std::isfinite(result.density) || result.density <= 0.0)
			return result;
		result.velocityX /= result.density;
		result.velocityY /= result.density;
		result.valid = std::isfinite(result.velocityX) && std::isfinite(result.velocityY);
		return result;
	}

	double ShearProjection(
		const std::vector<Populations> &cells,
		std::size_t cellsX,
		std::size_t cellsY)
	{
		double projection = 0.0;
		for (std::size_t y = 0; y < cellsY; ++y)
		{
			const double phase = 2.0 * Pi
				* (static_cast<double>(y) + 0.5) / static_cast<double>(cellsY);
			for (std::size_t x = 0; x < cellsX; ++x)
			{
				const auto state = Macroscopic(cells[y * cellsX + x]);
				if (!state.valid)
					return std::numeric_limits<double>::quiet_NaN();
				projection += state.velocityX * std::sin(phase);
			}
		}
		return 2.0 * projection / static_cast<double>(cellsX * cellsY);
	}

	bool MeasureState(
		const std::vector<Populations> &cells,
		LbmD2Q9ProbeSummary &summary)
	{
		for (const auto &cell : cells)
		{
			const auto state = Macroscopic(cell);
			if (!state.valid)
				return false;
			summary.minimumDensity = std::min(summary.minimumDensity, state.density);
			summary.maximumDensity = std::max(summary.maximumDensity, state.density);
			summary.maximumMach = std::max(summary.maximumMach,
				std::hypot(state.velocityX, state.velocityY) / SoundSpeed);
			for (const double population : cell)
				summary.minimumPopulation = std::min(summary.minimumPopulation, population);
		}
		return true;
	}

	std::array<double, 3> Totals(const std::vector<Populations> &cells)
	{
		std::array<double, 3> totals{};
		for (const auto &cell : cells)
		{
			const auto state = Macroscopic(cell);
			if (!state.valid)
				return {std::numeric_limits<double>::quiet_NaN(), 0.0, 0.0};
			totals[0] += state.density;
			totals[1] += state.density * state.velocityX;
			totals[2] += state.density * state.velocityY;
		}
		return totals;
	}

	LbmD2Q9ProbeSummary RunProbe(
		const BenchmarkCase &benchmarkCase,
		std::vector<Populations> initial,
		bool requireUniform,
		bool requireShearReference)
	{
		LbmD2Q9ProbeSummary summary{benchmarkCase};
		summary.relaxationTime = RelaxationTime;
		summary.kinematicViscosity = KinematicViscosity;
		if (!benchmarkCase.IsValid() || benchmarkCase.grid.boundaryMode != BoundaryMode::Periodic
			|| benchmarkCase.grid.cellsX < 2 || benchmarkCase.grid.cellsY < 2
			|| initial.size() != benchmarkCase.grid.CellCount())
			return summary;
		std::vector<Populations> cells = initial;
		std::vector<Populations> next(cells.size());
		summary.stateAndScratchBytesTotal = 2 * cells.size() * sizeof(Populations);
		summary.stateAndScratchBytesPerCell = static_cast<double>(
			summary.stateAndScratchBytesTotal) / static_cast<double>(cells.size());
		const auto initialTotals = Totals(cells);
		summary.initialMass = initialTotals[0];
		summary.initialMomentumX = initialTotals[1];
		summary.initialMomentumY = initialTotals[2];
		summary.initialShearAmplitude = ShearProjection(
			cells, benchmarkCase.grid.cellsX, benchmarkCase.grid.cellsY);
		summary.minimumDensity = std::numeric_limits<double>::infinity();
		summary.maximumDensity = -std::numeric_limits<double>::infinity();
		summary.minimumPopulation = std::numeric_limits<double>::infinity();
		if (!MeasureState(cells, summary))
			return summary;
		for (std::size_t step = 0; step < benchmarkCase.stepCount; ++step)
		{
			for (std::size_t y = 0; y < benchmarkCase.grid.cellsY; ++y)
			{
				for (std::size_t x = 0; x < benchmarkCase.grid.cellsX; ++x)
				{
					const std::size_t index = y * benchmarkCase.grid.cellsX + x;
					const auto state = Macroscopic(cells[index]);
					if (!state.valid)
						return summary;
					const auto equilibrium = Equilibrium(
						state.density, state.velocityX, state.velocityY);
					for (std::size_t direction = 0; direction < Directions; ++direction)
					{
						const double postCollision = cells[index][direction]
							- RelaxationRate * (cells[index][direction] - equilibrium[direction]);
						if (!std::isfinite(postCollision))
							return summary;
						const std::size_t destinationX = static_cast<std::size_t>(
							(static_cast<long long>(x) + DirectionX[direction]
								+ static_cast<long long>(benchmarkCase.grid.cellsX))
							% static_cast<long long>(benchmarkCase.grid.cellsX));
						const std::size_t destinationY = static_cast<std::size_t>(
							(static_cast<long long>(y) + DirectionY[direction]
								+ static_cast<long long>(benchmarkCase.grid.cellsY))
							% static_cast<long long>(benchmarkCase.grid.cellsY));
						next[destinationY * benchmarkCase.grid.cellsX + destinationX][direction]
							= postCollision;
					}
				}
			}
			cells.swap(next);
			if (!MeasureState(cells, summary))
				return summary;
		}
		const auto finalTotals = Totals(cells);
		summary.finalMass = finalTotals[0];
		summary.finalMomentumX = finalTotals[1];
		summary.finalMomentumY = finalTotals[2];
		summary.massDrift = summary.finalMass - summary.initialMass;
		summary.momentumXDrift = summary.finalMomentumX - summary.initialMomentumX;
		summary.momentumYDrift = summary.finalMomentumY - summary.initialMomentumY;
		summary.finalShearAmplitude = ShearProjection(
			cells, benchmarkCase.grid.cellsX, benchmarkCase.grid.cellsY);
		const double waveNumber = 2.0 * Pi / static_cast<double>(benchmarkCase.grid.cellsY);
		summary.expectedShearAmplitude = summary.initialShearAmplitude * std::exp(
			-KinematicViscosity * waveNumber * waveNumber
				* static_cast<double>(benchmarkCase.stepCount));
		if (std::abs(summary.expectedShearAmplitude) > 0.0)
			summary.shearAmplitudeRelativeError = std::abs(
				summary.finalShearAmplitude - summary.expectedShearAmplitude)
				/ std::abs(summary.expectedShearAmplitude);
		for (std::size_t index = 0; index < cells.size(); ++index)
		{
			for (std::size_t direction = 0; direction < Directions; ++direction)
				summary.stateChangeL1 += std::abs(
					cells[index][direction] - initial[index][direction]);
		}
		summary.massConserved = std::abs(summary.massDrift) <= 1e-8;
		summary.momentumConserved = std::abs(summary.momentumXDrift) <= 1e-8
			&& std::abs(summary.momentumYDrift) <= 1e-8;
		summary.positivityPreserved = summary.minimumDensity > 0.0
			&& summary.minimumPopulation > 0.0;
		summary.uniformPreserved = summary.stateChangeL1 <= 1e-10;
		summary.shearReferencePassed = std::isfinite(summary.shearAmplitudeRelativeError)
			&& summary.shearAmplitudeRelativeError <= 0.02
			&& summary.maximumMach <= 0.1;
		summary.passed = summary.massConserved && summary.momentumConserved
			&& summary.positivityPreserved && summary.corrections.IsEmpty()
			&& (!requireUniform || summary.uniformPreserved)
			&& (!requireShearReference || summary.shearReferencePassed);
		return summary;
	}

	bool WriteProbe(
		std::ostream &output,
		const LbmD2Q9ProbeSummary &summary,
		bool shearWave)
	{
		output << "schema_version=1\n";
		output << "case=" << summary.benchmarkCase.id << '\n';
		output << "candidate=lbm_d2q9\n";
		output << "candidate_solver_implemented=true\n";
		output << "atmosphere_solver_selection=unselected\n";
		output << "physical_scale_selection=unselected\n";
		output << "result_status=candidate_result_not_selection\n";
		output << "case_time_domain=nondimensional_contract\n";
		output << "dimension=2\n";
		output << "boundary_mode=periodic\n";
		output << "lbm_model=d2q9_bgk_isothermal\n";
		output << "energy_state=not_implemented\n";
		output << "energy_conservation=not_applicable_no_energy_state\n";
		output << "near_vacuum_support=unsupported_low_mach_positive_population_contract\n";
		output << "shock_support=unsupported_isothermal_low_mach_model\n";
		output << "species_support=not_implemented\n";
		output << "grid_cells_x=" << summary.benchmarkCase.grid.cellsX << '\n';
		output << "grid_cells_y=" << summary.benchmarkCase.grid.cellsY << '\n';
		output << "grid_cell_count=" << summary.benchmarkCase.grid.CellCount() << '\n';
		output << "cell_length=" << summary.benchmarkCase.grid.cellLength << '\n';
		output << "case_timestep=" << summary.benchmarkCase.timeStep << '\n';
		output << "case_step_count=" << summary.benchmarkCase.stepCount << '\n';
		output << "relaxation_time=" << summary.relaxationTime << '\n';
		output << "kinematic_viscosity=" << summary.kinematicViscosity << '\n';
		output << "maximum_cfl=not_applicable_lattice_streaming\n";
		output << "maximum_mach=" << summary.maximumMach << '\n';
		output << "minimum_density=" << summary.minimumDensity << '\n';
		output << "maximum_density=" << summary.maximumDensity << '\n';
		output << "minimum_pressure=" << SoundSpeedSquared * summary.minimumDensity << '\n';
		output << "minimum_population=" << summary.minimumPopulation << '\n';
		output << "initial_mass=" << summary.initialMass << '\n';
		output << "final_mass=" << summary.finalMass << '\n';
		output << "mass_drift=" << summary.massDrift << '\n';
		output << "initial_momentum_x=" << summary.initialMomentumX << '\n';
		output << "final_momentum_x=" << summary.finalMomentumX << '\n';
		output << "initial_momentum_y=" << summary.initialMomentumY << '\n';
		output << "final_momentum_y=" << summary.finalMomentumY << '\n';
		output << "momentum_x_drift=" << summary.momentumXDrift << '\n';
		output << "momentum_y_drift=" << summary.momentumYDrift << '\n';
		output << "momentum_drift="
			<< std::hypot(summary.momentumXDrift, summary.momentumYDrift) << '\n';
		output << "energy_drift=not_applicable_no_energy_state\n";
		output << "state_change_l1=" << summary.stateChangeL1 << '\n';
		output << "state_evolved=" << (summary.stateChangeL1 > 1e-12 ? "true" : "false") << '\n';
		output << "mass_conserved=" << (summary.massConserved ? "true" : "false") << '\n';
		output << "momentum_conserved=" << (summary.momentumConserved ? "true" : "false") << '\n';
		output << "positivity_preserved="
			<< (summary.positivityPreserved ? "true" : "false") << '\n';
		output << "uniform_preserved=" << (summary.uniformPreserved ? "true" : "false") << '\n';
		output << "initial_shear_amplitude=" << summary.initialShearAmplitude << '\n';
		output << "final_shear_amplitude=" << summary.finalShearAmplitude << '\n';
		output << "expected_shear_amplitude=" << summary.expectedShearAmplitude << '\n';
		output << "shear_amplitude_relative_error="
			<< summary.shearAmplitudeRelativeError << '\n';
		output << "shear_reference_passed="
			<< (summary.shearReferencePassed ? "true" : "false") << '\n';
		output << "probe_kind=" << (shearWave ? "shear_wave_decay" : "uniform_preservation") << '\n';
		output << "numerical_correction_count=" << summary.corrections.eventCount << '\n';
		output << "correction_mass_added=" << summary.corrections.massAdded << '\n';
		output << "correction_mass_removed=" << summary.corrections.massRemoved << '\n';
		output << "correction_momentum_x_added=" << summary.corrections.momentumXAdded << '\n';
		output << "correction_momentum_y_added=" << summary.corrections.momentumYAdded << '\n';
		output << "correction_energy_added=" << summary.corrections.energyAdded << '\n';
		output << "correction_energy_removed=" << summary.corrections.energyRemoved << '\n';
		output << "density_floor_hits=" << summary.corrections.densityFloorHits << '\n';
		output << "pressure_floor_hits=" << summary.corrections.pressureFloorHits << '\n';
		output << "correction_event_count=" << summary.corrections.eventCount << '\n';
		output << "state_bytes_per_cell=" << sizeof(Populations) << '\n';
		output << "state_and_flux_scratch_bytes_per_cell="
			<< summary.stateAndScratchBytesPerCell << '\n';
		output << "state_and_flux_scratch_bytes_total="
			<< summary.stateAndScratchBytesTotal << '\n';
		output << "probe_passed=" << (summary.passed ? "true" : "false") << '\n';
		return summary.passed;
	}
} // namespace

LbmD2Q9ProbeSummary RunLbmD2Q9Uniform()
{
	const auto equilibrium = Equilibrium(1.0, UniformVelocityX, UniformVelocityY);
	return RunProbe(
		{"lbm_d2q9_uniform_2d",
			{UniformCellsX, UniformCellsY, 1.0, BoundaryMode::Periodic},
			TimeDomain::NondimensionalContract, 1.0, UniformSteps},
		std::vector<Populations>(UniformCellsX * UniformCellsY, equilibrium), true, false);
}

LbmD2Q9ProbeSummary RunLbmD2Q9ShearWave()
{
	std::vector<Populations> initial(ShearCellsX * ShearCellsY);
	for (std::size_t y = 0; y < ShearCellsY; ++y)
	{
		const double phase = 2.0 * Pi
			* (static_cast<double>(y) + 0.5) / static_cast<double>(ShearCellsY);
		const double velocityX = ShearAmplitude * std::sin(phase);
		for (std::size_t x = 0; x < ShearCellsX; ++x)
			initial[y * ShearCellsX + x] = Equilibrium(1.0, velocityX, 0.0);
	}
	return RunProbe(
		{"lbm_d2q9_shear_wave_2d",
			{ShearCellsX, ShearCellsY, 1.0, BoundaryMode::Periodic},
			TimeDomain::NondimensionalContract, 1.0, ShearSteps},
		std::move(initial), false, true);
}

bool WriteLbmD2Q9UniformProbe(std::ostream &output)
{
	return WriteProbe(output, RunLbmD2Q9Uniform(), false);
}

bool WriteLbmD2Q9ShearWaveProbe(std::ostream &output)
{
	return WriteProbe(output, RunLbmD2Q9ShearWave(), true);
}

} // namespace omni::atmospherebench
