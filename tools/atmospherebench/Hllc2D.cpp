#include "Hllc2D.h"
#include "Rusanov1D.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <ostream>
#include <utility>
#include <vector>

namespace omni::atmospherebench
{

namespace
{
	constexpr double Gamma = 5.0 / 3.0;
	constexpr double SpecificGasConstant = 1.0;
	constexpr std::size_t CellsX = 32;
	constexpr std::size_t CellsY = 24;
	constexpr double CellLength = 1.0;
	constexpr double UniformTimeStep = 0.02;
	constexpr std::size_t UniformSteps = 8;
	constexpr double PulseTimeStep = 0.01;
	constexpr std::size_t PulseSteps = 40;
	constexpr double PulseAmplitude = 0.1;
	constexpr double PulseWidthCells = 4.0;
	constexpr double HeatingTimeStep = 0.01;
	constexpr std::size_t HeatingSteps = 40;
	constexpr double HeatingEnergyRateDensity = 0.25;

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

	double StateChangeL1(
		const std::vector<ConservativeState> &initial,
		const std::vector<ConservativeState> &final)
	{
		if (initial.size() != final.size())
			return std::numeric_limits<double>::quiet_NaN();
		double result = 0.0;
		for (std::size_t index = 0; index < initial.size(); ++index)
		{
			const auto difference = Subtract(final[index], initial[index]);
			result += std::abs(difference.density) + std::abs(difference.momentumX)
				+ std::abs(difference.momentumY) + std::abs(difference.totalEnergyDensity);
		}
		return result;
	}

	double MaximumPressure(const std::vector<ConservativeState> &cells, const IdealGasEOS &eos)
	{
		double maximum = 0.0;
		for (const auto &cell : cells)
		{
			const auto primitive = eos.ToPrimitive(cell);
			if (!primitive.valid)
				return std::numeric_limits<double>::quiet_NaN();
			maximum = std::max(maximum, primitive.pressure);
		}
		return maximum;
	}

	std::pair<double, double> MeanPressureTemperature(
		const std::vector<ConservativeState> &cells,
		const IdealGasEOS &eos)
	{
		if (cells.empty())
			return {std::numeric_limits<double>::quiet_NaN(),
				std::numeric_limits<double>::quiet_NaN()};
		double pressure = 0.0;
		double temperature = 0.0;
		for (const auto &cell : cells)
		{
			const auto primitive = eos.ToPrimitive(cell);
			if (!primitive.valid)
				return {std::numeric_limits<double>::quiet_NaN(),
					std::numeric_limits<double>::quiet_NaN()};
			pressure += primitive.pressure;
			temperature += primitive.temperature;
		}
		const double count = static_cast<double>(cells.size());
		return {pressure / count, temperature / count};
	}

	const char *BoundaryName(BoundaryMode mode)
	{
		switch (mode)
		{
		case BoundaryMode::Periodic:
			return "periodic";
		case BoundaryMode::Sealed:
			return "sealed";
		case BoundaryMode::Open:
			return "open";
		}
		return "unknown";
	}

	NumericalFluxResult SealedWallFluxX(
		const ConservativeState &inside,
		const IdealGasEOS &eos)
	{
		const auto primitive = eos.ToPrimitive(inside);
		if (!primitive.valid)
			return {};
		return {{0.0, primitive.pressure, 0.0, 0.0}, true, false};
	}

	NumericalFluxResult SealedWallFluxY(
		const ConservativeState &inside,
		const IdealGasEOS &eos)
	{
		const auto primitive = eos.ToPrimitive(inside);
		if (!primitive.valid)
			return {};
		return {{0.0, 0.0, primitive.pressure, 0.0}, true, false};
	}

	NumericalFluxResult FluxY(
		const ConservativeState &bottom,
		const ConservativeState &top,
		const IdealGasEOS &eos)
	{
		const ConservativeState rotatedBottom{
			bottom.density, bottom.momentumY, bottom.momentumX, bottom.totalEnergyDensity};
		const ConservativeState rotatedTop{
			top.density, top.momentumY, top.momentumX, top.totalEnergyDensity};
		auto result = ComputeHllcRusanovFallbackFluxX(rotatedBottom, rotatedTop, eos);
		std::swap(result.flux.momentumX, result.flux.momentumY);
		return result;
	}

	Hllc2DProbeSummary RunPeriodicProbe(
		const BenchmarkCase &benchmarkCase,
		std::vector<ConservativeState> initial,
		const IdealGasEOS &eos,
		bool requireEvolution,
		bool requirePeakReduction,
		double conservationTolerance)
	{
		Hllc2DProbeSummary summary{benchmarkCase};
		if (!benchmarkCase.IsValid() || benchmarkCase.grid.boundaryMode != BoundaryMode::Periodic
			|| benchmarkCase.grid.cellsX < 2 || benchmarkCase.grid.cellsY < 2
			|| initial.size() != benchmarkCase.grid.CellCount() || !eos.IsValid())
			return summary;
		std::vector<ConservativeState> cells = initial;
		std::vector<ConservativeState> next(cells.size());
		std::vector<ConservativeState> fluxX(cells.size());
		std::vector<ConservativeState> fluxY(cells.size());
		summary.stateAndFluxScratchBytesTotal = 5 * cells.size() * sizeof(ConservativeState);
		summary.stateAndFluxScratchBytesPerCell = static_cast<double>(
			summary.stateAndFluxScratchBytesTotal) / static_cast<double>(cells.size());
		summary.ledger.Begin(TotalState(cells));
		summary.initialMaximumPressure = MaximumPressure(cells, eos);
		const auto initialMeans = MeanPressureTemperature(cells, eos);
		summary.initialMeanPressure = initialMeans.first;
		summary.initialMeanTemperature = initialMeans.second;
		summary.minimumDensity = std::numeric_limits<double>::infinity();
		summary.minimumPressure = std::numeric_limits<double>::infinity();
		const double lambda = benchmarkCase.timeStep / benchmarkCase.grid.cellLength;
		for (std::size_t step = 0; step < benchmarkCase.stepCount; ++step)
		{
			for (std::size_t y = 0; y < benchmarkCase.grid.cellsY; ++y)
			{
				for (std::size_t x = 0; x < benchmarkCase.grid.cellsX; ++x)
				{
					const std::size_t index = y * benchmarkCase.grid.cellsX + x;
					const std::size_t right = y * benchmarkCase.grid.cellsX
						+ (x + 1) % benchmarkCase.grid.cellsX;
					const std::size_t top = ((y + 1) % benchmarkCase.grid.cellsY)
						* benchmarkCase.grid.cellsX + x;
					const auto xResult = ComputeHllcRusanovFallbackFluxX(cells[index], cells[right], eos);
					const auto yResult = FluxY(cells[index], cells[top], eos);
					if (!xResult.valid || !yResult.valid)
						return summary;
					fluxX[index] = xResult.flux;
					fluxY[index] = yResult.flux;
					summary.fluxFallbackCount += static_cast<std::size_t>(xResult.usedFallback)
						+ static_cast<std::size_t>(yResult.usedFallback);
				}
			}
			for (std::size_t y = 0; y < benchmarkCase.grid.cellsY; ++y)
			{
				for (std::size_t x = 0; x < benchmarkCase.grid.cellsX; ++x)
				{
					const std::size_t index = y * benchmarkCase.grid.cellsX + x;
					const std::size_t left = y * benchmarkCase.grid.cellsX
						+ (x + benchmarkCase.grid.cellsX - 1) % benchmarkCase.grid.cellsX;
					const std::size_t bottom = ((y + benchmarkCase.grid.cellsY - 1)
						% benchmarkCase.grid.cellsY) * benchmarkCase.grid.cellsX + x;
					next[index] = Subtract(cells[index], Scale(lambda,
						Add(Subtract(fluxX[index], fluxX[left]),
							Subtract(fluxY[index], fluxY[bottom]))));
					const auto primitive = eos.ToPrimitive(next[index]);
					if (!primitive.valid)
						return summary;
					summary.minimumDensity = std::min(summary.minimumDensity, primitive.density);
					summary.minimumPressure = std::min(summary.minimumPressure, primitive.pressure);
					summary.maximumCfl = std::max(summary.maximumCfl, lambda
						* (std::abs(primitive.velocityX) + std::abs(primitive.velocityY)
							+ 2.0 * primitive.soundSpeed));
				}
			}
			cells.swap(next);
		}
		summary.finalMaximumPressure = MaximumPressure(cells, eos);
		const auto finalMeans = MeanPressureTemperature(cells, eos);
		summary.finalMeanPressure = finalMeans.first;
		summary.finalMeanTemperature = finalMeans.second;
		summary.stateChangeL1 = StateChangeL1(initial, cells);
		summary.stateEvolved = std::isfinite(summary.stateChangeL1) && summary.stateChangeL1 > 1e-12;
		summary.pressurePeakReduced = summary.finalMaximumPressure < summary.initialMaximumPressure;
		summary.pressureIncreased = summary.finalMeanPressure > summary.initialMeanPressure;
		summary.temperatureIncreased = summary.finalMeanTemperature > summary.initialMeanTemperature;
		summary.positivityPreserved = std::isfinite(summary.minimumDensity)
			&& std::isfinite(summary.minimumPressure)
			&& summary.minimumDensity > 0.0 && summary.minimumPressure > 0.0;
		summary.ledger.End(TotalState(cells));
		summary.sourceLedgerCloses = summary.ledger.ClosesWithSources(
			summary.sources, conservationTolerance);
		summary.passed = summary.positivityPreserved
			&& summary.sourceLedgerCloses
			&& summary.corrections.IsEmpty() && summary.maximumCfl <= 1.0
			&& (!requireEvolution || summary.stateEvolved)
			&& (!requirePeakReduction || summary.pressurePeakReduced);
		return summary;
	}

	Hllc2DProbeSummary RunSealedHeatingProbe(
		const BenchmarkCase &benchmarkCase,
		std::vector<ConservativeState> initial,
		const IdealGasEOS &eos,
		double energyRateDensity,
		double conservationTolerance)
	{
		Hllc2DProbeSummary summary{benchmarkCase};
		if (!benchmarkCase.IsValid() || benchmarkCase.grid.boundaryMode != BoundaryMode::Sealed
			|| benchmarkCase.grid.cellsX < 2 || benchmarkCase.grid.cellsY < 2
			|| initial.size() != benchmarkCase.grid.CellCount() || !eos.IsValid()
			|| !std::isfinite(energyRateDensity) || energyRateDensity <= 0.0)
			return summary;
		const std::size_t cellsX = benchmarkCase.grid.cellsX;
		const std::size_t cellsY = benchmarkCase.grid.cellsY;
		std::vector<ConservativeState> cells = initial;
		std::vector<ConservativeState> next(cells.size());
		std::vector<ConservativeState> fluxX((cellsX + 1) * cellsY);
		std::vector<ConservativeState> fluxY(cellsX * (cellsY + 1));
		const std::size_t storedStates = initial.size() + cells.size() + next.size()
			+ fluxX.size() + fluxY.size();
		summary.stateAndFluxScratchBytesTotal = storedStates * sizeof(ConservativeState);
		summary.stateAndFluxScratchBytesPerCell = static_cast<double>(
			summary.stateAndFluxScratchBytesTotal) / static_cast<double>(cells.size());
		summary.ledger.Begin(TotalState(cells));
		summary.initialMaximumPressure = MaximumPressure(cells, eos);
		const auto initialMeans = MeanPressureTemperature(cells, eos);
		summary.initialMeanPressure = initialMeans.first;
		summary.initialMeanTemperature = initialMeans.second;
		summary.minimumDensity = std::numeric_limits<double>::infinity();
		summary.minimumPressure = std::numeric_limits<double>::infinity();
		const double lambda = benchmarkCase.timeStep / benchmarkCase.grid.cellLength;
		const ConservativeState sourceDelta{
			0.0, 0.0, 0.0, energyRateDensity * benchmarkCase.timeStep};
		for (std::size_t step = 0; step < benchmarkCase.stepCount; ++step)
		{
			for (std::size_t y = 0; y < cellsY; ++y)
			{
				auto leftWall = SealedWallFluxX(cells[y * cellsX], eos);
				if (!leftWall.valid)
					return summary;
				fluxX[y * (cellsX + 1)] = leftWall.flux;
				for (std::size_t faceX = 1; faceX < cellsX; ++faceX)
				{
					const auto result = ComputeHllcRusanovFallbackFluxX(
						cells[y * cellsX + faceX - 1], cells[y * cellsX + faceX], eos);
					if (!result.valid)
						return summary;
					fluxX[y * (cellsX + 1) + faceX] = result.flux;
					summary.fluxFallbackCount += static_cast<std::size_t>(result.usedFallback);
				}
				auto rightWall = SealedWallFluxX(cells[y * cellsX + cellsX - 1], eos);
				if (!rightWall.valid)
					return summary;
				fluxX[y * (cellsX + 1) + cellsX] = rightWall.flux;
			}
			for (std::size_t x = 0; x < cellsX; ++x)
			{
				auto bottomWall = SealedWallFluxY(cells[x], eos);
				if (!bottomWall.valid)
					return summary;
				fluxY[x] = bottomWall.flux;
				for (std::size_t faceY = 1; faceY < cellsY; ++faceY)
				{
					const auto result = FluxY(
						cells[(faceY - 1) * cellsX + x], cells[faceY * cellsX + x], eos);
					if (!result.valid)
						return summary;
					fluxY[faceY * cellsX + x] = result.flux;
					summary.fluxFallbackCount += static_cast<std::size_t>(result.usedFallback);
				}
				auto topWall = SealedWallFluxY(cells[(cellsY - 1) * cellsX + x], eos);
				if (!topWall.valid)
					return summary;
				fluxY[cellsY * cellsX + x] = topWall.flux;
			}
			for (std::size_t y = 0; y < cellsY; ++y)
			{
				for (std::size_t x = 0; x < cellsX; ++x)
				{
					const std::size_t index = y * cellsX + x;
					const auto fluxDifference = Add(
						Subtract(fluxX[y * (cellsX + 1) + x + 1],
							fluxX[y * (cellsX + 1) + x]),
						Subtract(fluxY[(y + 1) * cellsX + x], fluxY[y * cellsX + x]));
					next[index] = Add(Subtract(cells[index], Scale(lambda, fluxDifference)),
						sourceDelta);
					if (!summary.sources.RecordAppliedSource(sourceDelta))
						return summary;
					const auto primitive = eos.ToPrimitive(next[index]);
					if (!primitive.valid)
						return summary;
					summary.minimumDensity = std::min(summary.minimumDensity, primitive.density);
					summary.minimumPressure = std::min(summary.minimumPressure, primitive.pressure);
					summary.maximumCfl = std::max(summary.maximumCfl, lambda
						* (std::abs(primitive.velocityX) + std::abs(primitive.velocityY)
							+ 2.0 * primitive.soundSpeed));
				}
			}
			cells.swap(next);
		}
		summary.finalMaximumPressure = MaximumPressure(cells, eos);
		const auto finalMeans = MeanPressureTemperature(cells, eos);
		summary.finalMeanPressure = finalMeans.first;
		summary.finalMeanTemperature = finalMeans.second;
		const double energyPerCell = energyRateDensity * benchmarkCase.timeStep
			* static_cast<double>(benchmarkCase.stepCount);
		summary.expectedFinalMeanPressure = summary.initialMeanPressure
			+ (Gamma - 1.0) * energyPerCell;
		summary.expectedFinalMeanTemperature = summary.expectedFinalMeanPressure;
		summary.stateChangeL1 = StateChangeL1(initial, cells);
		summary.stateEvolved = std::isfinite(summary.stateChangeL1) && summary.stateChangeL1 > 1e-12;
		summary.pressurePeakReduced = summary.finalMaximumPressure < summary.initialMaximumPressure;
		summary.pressureIncreased = summary.finalMeanPressure > summary.initialMeanPressure;
		summary.temperatureIncreased = summary.finalMeanTemperature > summary.initialMeanTemperature;
		summary.positivityPreserved = std::isfinite(summary.minimumDensity)
			&& std::isfinite(summary.minimumPressure)
			&& summary.minimumDensity > 0.0 && summary.minimumPressure > 0.0;
		summary.ledger.End(TotalState(cells));
		summary.sourceLedgerCloses = summary.ledger.ClosesWithSources(
			summary.sources, conservationTolerance);
		summary.passed = summary.positivityPreserved && summary.sourceLedgerCloses
			&& summary.corrections.IsEmpty() && summary.maximumCfl <= 1.0
			&& summary.stateEvolved && summary.pressureIncreased && summary.temperatureIncreased
			&& std::abs(summary.finalMeanPressure - summary.expectedFinalMeanPressure) <= 1e-12
			&& std::abs(summary.finalMeanTemperature - summary.expectedFinalMeanTemperature) <= 1e-12;
		return summary;
	}

	bool WriteProbe(std::ostream &output, const Hllc2DProbeSummary &summary)
	{
		const auto &initial = summary.ledger.initial;
		const auto &final = summary.ledger.final;
		output << "schema_version=1\n";
		output << "case=" << summary.benchmarkCase.id << '\n';
		output << "candidate=fvm_hllc_rusanov_fallback\n";
		output << "candidate_solver_implemented=true\n";
		output << "atmosphere_solver_selection=unselected\n";
		output << "physical_scale_selection=unselected\n";
		output << "result_status=candidate_result_not_selection\n";
		output << "case_time_domain=nondimensional_contract\n";
		output << "dimension=2\n";
		output << "boundary_mode=" << BoundaryName(summary.benchmarkCase.grid.boundaryMode) << '\n';
		output << "grid_cells_x=" << summary.benchmarkCase.grid.cellsX << '\n';
		output << "grid_cells_y=" << summary.benchmarkCase.grid.cellsY << '\n';
		output << "grid_cell_count=" << summary.benchmarkCase.grid.CellCount() << '\n';
		output << "cell_length=" << summary.benchmarkCase.grid.cellLength << '\n';
		output << "case_timestep=" << summary.benchmarkCase.timeStep << '\n';
		output << "case_step_count=" << summary.benchmarkCase.stepCount << '\n';
		output << "maximum_cfl=" << summary.maximumCfl << '\n';
		output << "minimum_density=" << summary.minimumDensity << '\n';
		output << "minimum_pressure=" << summary.minimumPressure << '\n';
		output << "initial_maximum_pressure=" << summary.initialMaximumPressure << '\n';
		output << "final_maximum_pressure=" << summary.finalMaximumPressure << '\n';
		output << "initial_mean_pressure=" << summary.initialMeanPressure << '\n';
		output << "final_mean_pressure=" << summary.finalMeanPressure << '\n';
		output << "initial_mean_temperature=" << summary.initialMeanTemperature << '\n';
		output << "final_mean_temperature=" << summary.finalMeanTemperature << '\n';
		if (summary.benchmarkCase.grid.boundaryMode == BoundaryMode::Sealed)
		{
			output << "expected_final_mean_pressure=" << summary.expectedFinalMeanPressure << '\n';
			output << "expected_final_mean_temperature=" << summary.expectedFinalMeanTemperature << '\n';
		}
		output << "mass_drift=" << (final.density - initial.density) << '\n';
		output << "momentum_drift=" << std::hypot(
			final.momentumX - initial.momentumX,
			final.momentumY - initial.momentumY) << '\n';
		output << "momentum_x_drift=" << (final.momentumX - initial.momentumX) << '\n';
		output << "momentum_y_drift=" << (final.momentumY - initial.momentumY) << '\n';
		output << "energy_drift=" << (final.totalEnergyDensity - initial.totalEnergyDensity) << '\n';
		output << "source_mass_net=" << summary.sources.net.density << '\n';
		output << "source_momentum_x_net=" << summary.sources.net.momentumX << '\n';
		output << "source_momentum_y_net=" << summary.sources.net.momentumY << '\n';
		output << "source_energy_net=" << summary.sources.net.totalEnergyDensity << '\n';
		output << "source_event_count=" << summary.sources.eventCount << '\n';
		output << "source_mass_balance_error="
			<< (final.density - initial.density - summary.sources.net.density) << '\n';
		output << "source_momentum_x_balance_error="
			<< (final.momentumX - initial.momentumX - summary.sources.net.momentumX) << '\n';
		output << "source_momentum_y_balance_error="
			<< (final.momentumY - initial.momentumY - summary.sources.net.momentumY) << '\n';
		output << "source_energy_balance_error="
			<< (final.totalEnergyDensity - initial.totalEnergyDensity
				- summary.sources.net.totalEnergyDensity) << '\n';
		output << "source_ledger_closes=" << (summary.sourceLedgerCloses ? "true" : "false") << '\n';
		output << "state_change_l1=" << summary.stateChangeL1 << '\n';
		output << "state_evolved=" << (summary.stateEvolved ? "true" : "false") << '\n';
		output << "pressure_peak_reduced=" << (summary.pressurePeakReduced ? "true" : "false") << '\n';
		output << "pressure_increased=" << (summary.pressureIncreased ? "true" : "false") << '\n';
		output << "temperature_increased=" << (summary.temperatureIncreased ? "true" : "false") << '\n';
		output << "positivity_preserved=" << (summary.positivityPreserved ? "true" : "false") << '\n';
		output << "flux_fallback_count=" << summary.fluxFallbackCount << '\n';
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
		output << "state_bytes_per_cell=" << sizeof(ConservativeState) << '\n';
		output << "state_and_flux_scratch_bytes_per_cell="
			<< summary.stateAndFluxScratchBytesPerCell << '\n';
		output << "state_and_flux_scratch_bytes_total="
			<< summary.stateAndFluxScratchBytesTotal << '\n';
		output << "probe_passed=" << (summary.passed ? "true" : "false") << '\n';
		return summary.passed;
	}
} // namespace

Hllc2DProbeSummary RunHllc2DUniform()
{
	const IdealGasEOS eos(Gamma, SpecificGasConstant);
	const auto state = eos.FromPrimitive(1.0, 0.1, -0.05, 1.0);
	return RunPeriodicProbe(
		{"hllc_uniform_2d", {CellsX, CellsY, CellLength, BoundaryMode::Periodic},
			TimeDomain::NondimensionalContract, UniformTimeStep, UniformSteps},
		std::vector<ConservativeState>(CellsX * CellsY, state), eos, false, false, 1e-10);
}

Hllc2DProbeSummary RunHllc2DPressurePulse()
{
	const IdealGasEOS eos(Gamma, SpecificGasConstant);
	std::vector<ConservativeState> initial(CellsX * CellsY);
	const double centerX = 0.5 * static_cast<double>(CellsX);
	const double centerY = 0.5 * static_cast<double>(CellsY);
	for (std::size_t y = 0; y < CellsY; ++y)
	{
		for (std::size_t x = 0; x < CellsX; ++x)
		{
			const double dx = static_cast<double>(x) + 0.5 - centerX;
			const double dy = static_cast<double>(y) + 0.5 - centerY;
			const double pulse = PulseAmplitude * std::exp(
				-(dx * dx + dy * dy) / (2.0 * PulseWidthCells * PulseWidthCells));
			initial[y * CellsX + x] = eos.FromPrimitive(1.0, 0.0, 0.0, 1.0 + pulse);
		}
	}
	return RunPeriodicProbe(
		{"hllc_pressure_pulse_2d", {CellsX, CellsY, CellLength, BoundaryMode::Periodic},
			TimeDomain::NondimensionalContract, PulseTimeStep, PulseSteps},
		std::move(initial), eos, true, true, 1e-9);
}

Hllc2DProbeSummary RunHllc2DSealedHeating()
{
	const IdealGasEOS eos(Gamma, SpecificGasConstant);
	const auto state = eos.FromPrimitive(1.0, 0.0, 0.0, 1.0);
	return RunSealedHeatingProbe(
		{"hllc_sealed_heating_2d", {CellsX, CellsY, CellLength, BoundaryMode::Sealed},
			TimeDomain::NondimensionalContract, HeatingTimeStep, HeatingSteps},
		std::vector<ConservativeState>(CellsX * CellsY, state), eos,
		HeatingEnergyRateDensity, 1e-9);
}

bool WriteHllc2DUniformProbe(std::ostream &output)
{
	return WriteProbe(output, RunHllc2DUniform());
}

bool WriteHllc2DPressurePulseProbe(std::ostream &output)
{
	return WriteProbe(output, RunHllc2DPressurePulse());
}

bool WriteHllc2DSealedHeatingProbe(std::ostream &output)
{
	return WriteProbe(output, RunHllc2DSealedHeating());
}

} // namespace omni::atmospherebench
