#include "Hllc2D.h"
#include "Rusanov1D.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <functional>
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
	constexpr double ConvectionTimeStep = 0.01;
	constexpr std::size_t ConvectionSteps = 400;
	constexpr double ConvectionGravityY = -0.04;
	constexpr double ConvectionHotAmplitude = 0.5;
	constexpr double ConvectionHotCenterX = 0.5 * static_cast<double>(CellsX);
	constexpr double ConvectionHotCenterY = 3.5;
	constexpr double ConvectionHotWidthX = 4.0;
	constexpr double ConvectionHotWidthY = 2.0;
	constexpr std::size_t PerformanceLegacyCellsX = 153;
	constexpr std::size_t PerformanceLegacyCellsY = 96;
	constexpr std::size_t PerformanceDoubledCellsX = 306;
	constexpr std::size_t PerformanceDoubledCellsY = 192;
	constexpr std::size_t PerformanceParticleCellsX = 612;
	constexpr std::size_t PerformanceParticleCellsY = 384;
	constexpr std::size_t PerformanceSteps = 32;
	constexpr std::size_t PerformanceWarmups = 1;
	constexpr std::size_t PerformanceRepeats = 3;
	constexpr double PerformanceVelocityX = 0.2;
	constexpr double PerformanceVelocityY = 0.1;
	constexpr double PerformanceDensityAmplitude = 0.05;
	constexpr double PerformanceTargetCfl = 0.2;
	constexpr double ReferenceFrameBudgetMilliseconds = 1000.0 / 60.0;
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
		summary.sourceAndBoundaryLedgerCloses = summary.ledger.ClosesWithSourcesAndBoundary(
			summary.sources, summary.boundary, conservationTolerance);
		summary.passed = summary.positivityPreserved
			&& summary.sourceLedgerCloses && summary.sourceAndBoundaryLedgerCloses
			&& summary.corrections.IsEmpty() && summary.maximumCfl <= 1.0
			&& (!requireEvolution || summary.stateEvolved)
			&& (!requirePeakReduction || summary.pressurePeakReduced);
		return summary;
	}

	using CellSource = std::function<bool(
		const ConservativeState &,
		std::size_t,
		std::size_t,
		std::size_t,
		double,
		ConservativeState &)>;

	bool EvolveSealedProbe(
		Hllc2DProbeSummary &summary,
		std::vector<ConservativeState> initial,
		const IdealGasEOS &eos,
		const CellSource &source,
		double conservationTolerance,
		std::vector<ConservativeState> &finalCells)
	{
		const auto &benchmarkCase = summary.benchmarkCase;
		if (!benchmarkCase.IsValid() || benchmarkCase.grid.boundaryMode != BoundaryMode::Sealed
			|| benchmarkCase.grid.cellsX < 2 || benchmarkCase.grid.cellsY < 2
			|| initial.size() != benchmarkCase.grid.CellCount() || !eos.IsValid() || !source)
			return false;
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
		for (std::size_t step = 0; step < benchmarkCase.stepCount; ++step)
		{
			for (std::size_t y = 0; y < cellsY; ++y)
			{
				const auto leftWall = SealedWallFluxX(cells[y * cellsX], eos);
				if (!leftWall.valid)
					return false;
				fluxX[y * (cellsX + 1)] = leftWall.flux;
				for (std::size_t faceX = 1; faceX < cellsX; ++faceX)
				{
					const auto result = ComputeHllcRusanovFallbackFluxX(
						cells[y * cellsX + faceX - 1], cells[y * cellsX + faceX], eos);
					if (!result.valid)
						return false;
					fluxX[y * (cellsX + 1) + faceX] = result.flux;
					summary.fluxFallbackCount += static_cast<std::size_t>(result.usedFallback);
				}
				const auto rightWall = SealedWallFluxX(cells[y * cellsX + cellsX - 1], eos);
				if (!rightWall.valid)
					return false;
				fluxX[y * (cellsX + 1) + cellsX] = rightWall.flux;
			}
			for (std::size_t x = 0; x < cellsX; ++x)
			{
				const auto bottomWall = SealedWallFluxY(cells[x], eos);
				if (!bottomWall.valid)
					return false;
				fluxY[x] = bottomWall.flux;
				for (std::size_t faceY = 1; faceY < cellsY; ++faceY)
				{
					const auto result = FluxY(
						cells[(faceY - 1) * cellsX + x], cells[faceY * cellsX + x], eos);
					if (!result.valid)
						return false;
					fluxY[faceY * cellsX + x] = result.flux;
					summary.fluxFallbackCount += static_cast<std::size_t>(result.usedFallback);
				}
				const auto topWall = SealedWallFluxY(cells[(cellsY - 1) * cellsX + x], eos);
				if (!topWall.valid)
					return false;
				fluxY[cellsY * cellsX + x] = topWall.flux;
			}
			ConservativeState boundaryDelta;
			for (std::size_t y = 0; y < cellsY; ++y)
			{
				boundaryDelta = Add(boundaryDelta, Scale(lambda,
					Subtract(fluxX[y * (cellsX + 1)],
						fluxX[y * (cellsX + 1) + cellsX])));
			}
			for (std::size_t x = 0; x < cellsX; ++x)
			{
				boundaryDelta = Add(boundaryDelta, Scale(lambda,
					Subtract(fluxY[x], fluxY[cellsY * cellsX + x])));
			}
			if (!summary.boundary.RecordAppliedSource(boundaryDelta))
				return false;
			for (std::size_t y = 0; y < cellsY; ++y)
			{
				for (std::size_t x = 0; x < cellsX; ++x)
				{
					const std::size_t index = y * cellsX + x;
					const auto fluxDifference = Add(
						Subtract(fluxX[y * (cellsX + 1) + x + 1],
							fluxX[y * (cellsX + 1) + x]),
						Subtract(fluxY[(y + 1) * cellsX + x], fluxY[y * cellsX + x]));
					next[index] = Subtract(cells[index], Scale(lambda, fluxDifference));
					ConservativeState sourceDelta;
					if (!source(next[index], x, y, step, benchmarkCase.timeStep, sourceDelta)
						|| !summary.sources.RecordAppliedSource(sourceDelta))
						return false;
					next[index] = Add(next[index], sourceDelta);
					const auto primitive = eos.ToPrimitive(next[index]);
					if (!primitive.valid)
						return false;
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
		summary.stateEvolved = std::isfinite(summary.stateChangeL1)
			&& summary.stateChangeL1 > 1e-12;
		summary.pressurePeakReduced = summary.finalMaximumPressure
			< summary.initialMaximumPressure;
		summary.pressureIncreased = summary.finalMeanPressure > summary.initialMeanPressure;
		summary.temperatureIncreased = summary.finalMeanTemperature
			> summary.initialMeanTemperature;
		summary.positivityPreserved = std::isfinite(summary.minimumDensity)
			&& std::isfinite(summary.minimumPressure)
			&& summary.minimumDensity > 0.0 && summary.minimumPressure > 0.0;
		summary.ledger.End(TotalState(cells));
		summary.sourceLedgerCloses = summary.ledger.ClosesWithSources(
			summary.sources, conservationTolerance);
		summary.sourceAndBoundaryLedgerCloses = summary.ledger.ClosesWithSourcesAndBoundary(
			summary.sources, summary.boundary, conservationTolerance);
		finalCells = std::move(cells);
		return true;
	}

	Hllc2DProbeSummary RunSealedHeatingProbe(
		const BenchmarkCase &benchmarkCase,
		std::vector<ConservativeState> initial,
		const IdealGasEOS &eos,
		double energyRateDensity,
		double conservationTolerance)
	{
		Hllc2DProbeSummary summary{benchmarkCase};
		if (!std::isfinite(energyRateDensity) || energyRateDensity <= 0.0)
			return summary;
		std::vector<ConservativeState> finalCells;
		const CellSource heatSource = [energyRateDensity](
			const ConservativeState &,
			std::size_t,
			std::size_t,
			std::size_t,
			double timeStep,
			ConservativeState &delta) {
			delta = {0.0, 0.0, 0.0, energyRateDensity * timeStep};
			return true;
		};
		if (!EvolveSealedProbe(summary, std::move(initial), eos, heatSource,
			conservationTolerance, finalCells))
			return summary;
		const double energyPerCell = energyRateDensity * benchmarkCase.timeStep
			* static_cast<double>(benchmarkCase.stepCount);
		summary.expectedFinalMeanPressure = summary.initialMeanPressure
			+ (Gamma - 1.0) * energyPerCell;
		summary.expectedFinalMeanTemperature = summary.expectedFinalMeanPressure;
		summary.passed = summary.positivityPreserved && summary.sourceLedgerCloses
			&& summary.sourceAndBoundaryLedgerCloses
			&& summary.corrections.IsEmpty() && summary.maximumCfl <= 1.0
			&& summary.stateEvolved && summary.pressureIncreased && summary.temperatureIncreased
			&& std::abs(summary.finalMeanPressure - summary.expectedFinalMeanPressure) <= 1e-12
			&& std::abs(summary.finalMeanTemperature - summary.expectedFinalMeanTemperature) <= 1e-12;
		return summary;
	}

	struct ThermalDiagnostics
	{
		double centerY = std::numeric_limits<double>::quiet_NaN();
		double weightedVelocityY = std::numeric_limits<double>::quiet_NaN();
		double maximumUpwardVelocity = -std::numeric_limits<double>::infinity();
		double minimumDownwardVelocity = std::numeric_limits<double>::infinity();
		double maximumAbsoluteVelocity = 0.0;
	};

	ThermalDiagnostics MeasureConvection(
		const std::vector<ConservativeState> &cells,
		std::size_t cellsX,
		std::size_t cellsY,
		const IdealGasEOS &eos)
	{
		ThermalDiagnostics result;
		double thermalWeight = 0.0;
		double weightedY = 0.0;
		double weightedVelocityY = 0.0;
		for (std::size_t y = 0; y < cellsY; ++y)
		{
			for (std::size_t x = 0; x < cellsX; ++x)
			{
				const auto primitive = eos.ToPrimitive(cells[y * cellsX + x]);
				if (!primitive.valid)
					return {};
				result.maximumUpwardVelocity = std::max(
					result.maximumUpwardVelocity, primitive.velocityY);
				result.minimumDownwardVelocity = std::min(
					result.minimumDownwardVelocity, primitive.velocityY);
				result.maximumAbsoluteVelocity = std::max(result.maximumAbsoluteVelocity,
					std::hypot(primitive.velocityX, primitive.velocityY));
				const double weight = std::max(0.0, primitive.temperature - 1.0);
				thermalWeight += weight;
				weightedY += weight * (static_cast<double>(y) + 0.5);
				weightedVelocityY += weight * primitive.velocityY;
			}
		}
		if (thermalWeight > 0.0)
		{
			result.centerY = weightedY / thermalWeight;
			result.weightedVelocityY = weightedVelocityY / thermalWeight;
		}
		return result;
	}

	ThermalDiagnostics MeasureDifferentialConvection(
		const std::vector<ConservativeState> &heated,
		const std::vector<ConservativeState> &control,
		std::size_t cellsX,
		std::size_t cellsY,
		const IdealGasEOS &eos)
	{
		if (heated.size() != control.size() || heated.size() != cellsX * cellsY)
			return {};
		ThermalDiagnostics result;
		double thermalWeight = 0.0;
		double weightedY = 0.0;
		double weightedVelocityY = 0.0;
		for (std::size_t y = 0; y < cellsY; ++y)
		{
			for (std::size_t x = 0; x < cellsX; ++x)
			{
				const std::size_t index = y * cellsX + x;
				const auto heatedPrimitive = eos.ToPrimitive(heated[index]);
				const auto controlPrimitive = eos.ToPrimitive(control[index]);
				if (!heatedPrimitive.valid || !controlPrimitive.valid)
					return {};
				const double velocityDifferenceX = heatedPrimitive.velocityX
					- controlPrimitive.velocityX;
				const double velocityDifferenceY = heatedPrimitive.velocityY
					- controlPrimitive.velocityY;
				result.maximumUpwardVelocity = std::max(
					result.maximumUpwardVelocity, velocityDifferenceY);
				result.minimumDownwardVelocity = std::min(
					result.minimumDownwardVelocity, velocityDifferenceY);
				result.maximumAbsoluteVelocity = std::max(result.maximumAbsoluteVelocity,
					std::hypot(velocityDifferenceX, velocityDifferenceY));
				const double weight = std::max(0.0,
					heatedPrimitive.temperature - controlPrimitive.temperature);
				thermalWeight += weight;
				weightedY += weight * (static_cast<double>(y) + 0.5);
				weightedVelocityY += weight * velocityDifferenceY;
			}
		}
		if (thermalWeight > 0.0)
		{
			result.centerY = weightedY / thermalWeight;
			result.weightedVelocityY = weightedVelocityY / thermalWeight;
		}
		return result;
	}

	std::vector<ConservativeState> MakeHydrostaticConvectionState(
		const IdealGasEOS &eos,
		double gravityY,
		double hotAmplitude)
	{
		std::vector<ConservativeState> cells(CellsX * CellsY);
		for (std::size_t y = 0; y < CellsY; ++y)
		{
			for (std::size_t x = 0; x < CellsX; ++x)
			{
				const double xCenter = static_cast<double>(x) + 0.5;
				const double yCenter = static_cast<double>(y) + 0.5;
				const double dx = (xCenter - ConvectionHotCenterX) / ConvectionHotWidthX;
				const double dy = (yCenter - ConvectionHotCenterY) / ConvectionHotWidthY;
				const double temperature = 1.0 + hotAmplitude
					* std::exp(-0.5 * (dx * dx + dy * dy));
				const double pressure = std::exp(gravityY * yCenter);
				const double density = pressure / temperature;
				cells[y * CellsX + x] = eos.FromPrimitive(
					density, 0.0, 0.0, pressure);
			}
		}
		return cells;
	}

	Hllc2DNaturalConvectionSummary RunNaturalConvectionProbe()
	{
		const BenchmarkCase benchmarkCase{
			"hllc_natural_convection_2d",
			{CellsX, CellsY, CellLength, BoundaryMode::Sealed},
			TimeDomain::NondimensionalContract,
			ConvectionTimeStep,
			ConvectionSteps,
		};
		Hllc2DNaturalConvectionSummary result{
			benchmarkCase,
			Hllc2DProbeSummary{benchmarkCase},
			Hllc2DProbeSummary{benchmarkCase},
		};
		result.gravityY = ConvectionGravityY;
		result.hotTemperatureAmplitude = ConvectionHotAmplitude;
		const IdealGasEOS eos(Gamma, SpecificGasConstant);
		auto controlInitial = MakeHydrostaticConvectionState(eos, ConvectionGravityY, 0.0);
		auto heatedInitial = MakeHydrostaticConvectionState(
			eos, ConvectionGravityY, ConvectionHotAmplitude);
		const auto initialThermal = MeasureDifferentialConvection(
			heatedInitial, controlInitial, CellsX, CellsY, eos);
		result.initialThermalCenterY = initialThermal.centerY;
		const CellSource gravitySource = [](const ConservativeState &state,
			std::size_t,
			std::size_t,
			std::size_t,
			double timeStep,
			ConservativeState &delta) {
			if (!std::isfinite(state.density) || !std::isfinite(state.momentumY)
				|| state.density <= 0.0)
				return false;
			const double momentumDelta = state.density * ConvectionGravityY * timeStep;
			const double velocityY = state.momentumY / state.density;
			const double energyDelta = velocityY * momentumDelta
				+ 0.5 * momentumDelta * momentumDelta / state.density;
			delta = {0.0, 0.0, momentumDelta, energyDelta};
			return std::isfinite(energyDelta);
		};
		std::vector<ConservativeState> controlFinal;
		std::vector<ConservativeState> heatedFinal;
		const bool controlExecuted = EvolveSealedProbe(result.control,
			std::move(controlInitial), eos, gravitySource, 1e-8, controlFinal);
		const bool heatedExecuted = EvolveSealedProbe(result.heated,
			std::move(heatedInitial), eos, gravitySource, 1e-8, heatedFinal);
		if (!controlExecuted || !heatedExecuted)
			return result;
		const auto controlMetrics = MeasureConvection(controlFinal, CellsX, CellsY, eos);
		const auto heatedMetrics = MeasureConvection(heatedFinal, CellsX, CellsY, eos);
		const auto differentialMetrics = MeasureDifferentialConvection(
			heatedFinal, controlFinal, CellsX, CellsY, eos);
		result.finalThermalCenterY = differentialMetrics.centerY;
		result.thermalCenterRise = result.finalThermalCenterY - result.initialThermalCenterY;
		result.thermalWeightedVelocityY = differentialMetrics.weightedVelocityY;
		result.controlMaximumAbsoluteVelocity = controlMetrics.maximumAbsoluteVelocity;
		result.heatedMaximumUpwardVelocity = heatedMetrics.maximumUpwardVelocity;
		result.heatedMinimumDownwardVelocity = heatedMetrics.minimumDownwardVelocity;
		result.heatedMaximumAbsoluteVelocity = heatedMetrics.maximumAbsoluteVelocity;
		result.maximumUpwardVelocityDifference = differentialMetrics.maximumUpwardVelocity;
		result.minimumDownwardVelocityDifference = differentialMetrics.minimumDownwardVelocity;
		result.maximumAbsoluteVelocityDifference = differentialMetrics.maximumAbsoluteVelocity;
		result.control.passed = result.control.positivityPreserved
			&& result.control.sourceAndBoundaryLedgerCloses
			&& result.control.corrections.IsEmpty() && result.control.maximumCfl <= 1.0;
		result.heated.passed = result.heated.positivityPreserved
			&& result.heated.sourceAndBoundaryLedgerCloses
			&& result.heated.corrections.IsEmpty() && result.heated.maximumCfl <= 1.0
			&& result.heated.stateEvolved;
		result.circulationObserved = std::isfinite(result.thermalCenterRise)
			&& std::isfinite(result.thermalWeightedVelocityY)
			&& result.thermalCenterRise >= 0.05
			&& result.thermalWeightedVelocityY > 0.0
			&& result.maximumUpwardVelocityDifference > 1e-4
			&& result.minimumDownwardVelocityDifference < -1e-4
			&& result.maximumAbsoluteVelocityDifference > 1e-4;
		result.passed = result.control.passed && result.heated.passed
			&& result.circulationObserved
			&& result.control.fluxFallbackCount == 0
			&& result.heated.fluxFallbackCount == 0;
		return result;
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
		output << "boundary_mass_net=" << summary.boundary.net.density << '\n';
		output << "boundary_momentum_x_net=" << summary.boundary.net.momentumX << '\n';
		output << "boundary_momentum_y_net=" << summary.boundary.net.momentumY << '\n';
		output << "boundary_energy_net=" << summary.boundary.net.totalEnergyDensity << '\n';
		output << "boundary_event_count=" << summary.boundary.eventCount << '\n';
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
		output << "source_and_boundary_ledger_closes="
			<< (summary.sourceAndBoundaryLedgerCloses ? "true" : "false") << '\n';
		output << "combined_mass_balance_error="
			<< (final.density - initial.density - summary.sources.net.density
				- summary.boundary.net.density) << '\n';
		output << "combined_momentum_x_balance_error="
			<< (final.momentumX - initial.momentumX - summary.sources.net.momentumX
				- summary.boundary.net.momentumX) << '\n';
		output << "combined_momentum_y_balance_error="
			<< (final.momentumY - initial.momentumY - summary.sources.net.momentumY
				- summary.boundary.net.momentumY) << '\n';
		output << "combined_energy_balance_error="
			<< (final.totalEnergyDensity - initial.totalEnergyDensity
				- summary.sources.net.totalEnergyDensity
				- summary.boundary.net.totalEnergyDensity) << '\n';
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

	void WriteConvectionSubresult(
		std::ostream &output,
		std::string_view prefix,
		const Hllc2DProbeSummary &summary)
	{
		const auto &initial = summary.ledger.initial;
		const auto &final = summary.ledger.final;
		output << prefix << "_minimum_density=" << summary.minimumDensity << '\n';
		output << prefix << "_minimum_pressure=" << summary.minimumPressure << '\n';
		output << prefix << "_maximum_cfl=" << summary.maximumCfl << '\n';
		output << prefix << "_state_evolved=" << (summary.stateEvolved ? "true" : "false") << '\n';
		output << prefix << "_positivity_preserved="
			<< (summary.positivityPreserved ? "true" : "false") << '\n';
		output << prefix << "_source_ledger_closes="
			<< (summary.sourceLedgerCloses ? "true" : "false") << '\n';
		output << prefix << "_source_and_boundary_ledger_closes="
			<< (summary.sourceAndBoundaryLedgerCloses ? "true" : "false") << '\n';
		output << prefix << "_source_mass_net=" << summary.sources.net.density << '\n';
		output << prefix << "_source_momentum_x_net=" << summary.sources.net.momentumX << '\n';
		output << prefix << "_source_momentum_y_net=" << summary.sources.net.momentumY << '\n';
		output << prefix << "_source_energy_net=" << summary.sources.net.totalEnergyDensity << '\n';
		output << prefix << "_source_event_count=" << summary.sources.eventCount << '\n';
		output << prefix << "_boundary_mass_net=" << summary.boundary.net.density << '\n';
		output << prefix << "_boundary_momentum_x_net=" << summary.boundary.net.momentumX << '\n';
		output << prefix << "_boundary_momentum_y_net=" << summary.boundary.net.momentumY << '\n';
		output << prefix << "_boundary_energy_net=" << summary.boundary.net.totalEnergyDensity << '\n';
		output << prefix << "_boundary_event_count=" << summary.boundary.eventCount << '\n';
		output << prefix << "_combined_mass_balance_error="
			<< (final.density - initial.density - summary.sources.net.density
				- summary.boundary.net.density) << '\n';
		output << prefix << "_combined_momentum_x_balance_error="
			<< (final.momentumX - initial.momentumX - summary.sources.net.momentumX
				- summary.boundary.net.momentumX) << '\n';
		output << prefix << "_combined_momentum_y_balance_error="
			<< (final.momentumY - initial.momentumY - summary.sources.net.momentumY
				- summary.boundary.net.momentumY) << '\n';
		output << prefix << "_combined_energy_balance_error="
			<< (final.totalEnergyDensity - initial.totalEnergyDensity
				- summary.sources.net.totalEnergyDensity
				- summary.boundary.net.totalEnergyDensity) << '\n';
		output << prefix << "_flux_fallback_count=" << summary.fluxFallbackCount << '\n';
		output << prefix << "_numerical_correction_count=" << summary.corrections.eventCount << '\n';
		output << prefix << "_probe_passed=" << (summary.passed ? "true" : "false") << '\n';
	}

	Hllc2DProbeSummary RunPerformanceCase(
		std::size_t cellsX,
		std::size_t cellsY,
		std::string_view caseId)
	{
		const IdealGasEOS eos(Gamma, SpecificGasConstant);
		const double maximumSoundSpeed = std::sqrt(
			Gamma / (1.0 - PerformanceDensityAmplitude));
		const double timeStep = PerformanceTargetCfl
			/ (std::abs(PerformanceVelocityX) + std::abs(PerformanceVelocityY)
				+ 2.0 * maximumSoundSpeed);
		std::vector<ConservativeState> initial(cellsX * cellsY);
		for (std::size_t y = 0; y < cellsY; ++y)
		{
			for (std::size_t x = 0; x < cellsX; ++x)
			{
				const double phaseX = 2.0 * Pi
					* (static_cast<double>(x) + 0.5) / static_cast<double>(cellsX);
				const double phaseY = 2.0 * Pi
					* (static_cast<double>(y) + 0.5) / static_cast<double>(cellsY);
				const double density = 1.0 + PerformanceDensityAmplitude
					* std::sin(phaseX) * std::cos(phaseY);
				initial[y * cellsX + x] = eos.FromPrimitive(
					density, PerformanceVelocityX, PerformanceVelocityY, 1.0);
			}
		}
		return RunPeriodicProbe(
			{caseId, {cellsX, cellsY, 1.0, BoundaryMode::Periodic},
				TimeDomain::NondimensionalContract, timeStep, PerformanceSteps},
			std::move(initial), eos, true, false, 1e-7);
	}

	Hllc2DPerformanceSample MeasurePerformanceCase(
		std::size_t cellsX,
		std::size_t cellsY,
		std::string_view caseId)
	{
		for (std::size_t warmup = 0; warmup < PerformanceWarmups; ++warmup)
		{
			if (!RunPerformanceCase(cellsX, cellsY, caseId).passed)
				return {};
		}
		std::array<double, PerformanceRepeats> elapsed{};
		Hllc2DProbeSummary finalProbe;
		for (std::size_t repeat = 0; repeat < PerformanceRepeats; ++repeat)
		{
			const auto start = std::chrono::steady_clock::now();
			auto probe = RunPerformanceCase(cellsX, cellsY, caseId);
			const auto stop = std::chrono::steady_clock::now();
			if (!probe.passed || probe.fluxFallbackCount != 0)
				return {};
			elapsed[repeat] = std::chrono::duration<double, std::milli>(stop - start).count();
			finalProbe = std::move(probe);
		}
		std::sort(elapsed.begin(), elapsed.end());
		const double medianMilliseconds = elapsed[elapsed.size() / 2];
		const double millisecondsPerStep = medianMilliseconds
			/ static_cast<double>(PerformanceSteps);
		const double cellUpdates = static_cast<double>(cellsX * cellsY)
			* static_cast<double>(PerformanceSteps);
		const double throughput = medianMilliseconds > 0.0
			? cellUpdates * 1000.0 / medianMilliseconds
			: 0.0;
		const bool valid = std::isfinite(medianMilliseconds) && medianMilliseconds > 0.0
			&& std::isfinite(millisecondsPerStep) && millisecondsPerStep > 0.0
			&& std::isfinite(throughput) && throughput > 0.0;
		return {std::move(finalProbe), medianMilliseconds,
			millisecondsPerStep, throughput, valid};
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

Hllc2DNaturalConvectionSummary RunHllc2DNaturalConvection()
{
	return RunNaturalConvectionProbe();
}

Hllc2DPerformanceSummary RunHllc2DPerformance()
{
	Hllc2DPerformanceSummary summary{
		MeasurePerformanceCase(PerformanceLegacyCellsX, PerformanceLegacyCellsY,
			"hllc_performance_2d"),
		MeasurePerformanceCase(PerformanceDoubledCellsX, PerformanceDoubledCellsY,
			"hllc_performance_2d"),
		MeasurePerformanceCase(PerformanceParticleCellsX, PerformanceParticleCellsY,
			"hllc_performance_2d"),
		PerformanceWarmups,
		PerformanceRepeats,
		PerformanceSteps,
		ReferenceFrameBudgetMilliseconds,
	};
	summary.passed = summary.legacyGrid.valid && summary.doubledGrid.valid
		&& summary.particleGrid.valid;
	return summary;
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

bool WriteHllc2DNaturalConvectionProbe(std::ostream &output)
{
	const auto summary = RunHllc2DNaturalConvection();
	output << "schema_version=1\n";
	output << "case=" << summary.benchmarkCase.id << '\n';
	output << "candidate=fvm_hllc_rusanov_fallback\n";
	output << "candidate_solver_implemented=true\n";
	output << "atmosphere_solver_selection=unselected\n";
	output << "physical_scale_selection=unselected\n";
	output << "result_status=candidate_result_not_selection\n";
	output << "case_time_domain=nondimensional_contract\n";
	output << "dimension=2\n";
	output << "boundary_mode=sealed\n";
	output << "grid_cells_x=" << summary.benchmarkCase.grid.cellsX << '\n';
	output << "grid_cells_y=" << summary.benchmarkCase.grid.cellsY << '\n';
	output << "grid_cell_count=" << summary.benchmarkCase.grid.CellCount() << '\n';
	output << "cell_length=" << summary.benchmarkCase.grid.cellLength << '\n';
	output << "case_timestep=" << summary.benchmarkCase.timeStep << '\n';
	output << "case_step_count=" << summary.benchmarkCase.stepCount << '\n';
	output << "gravity_y=" << summary.gravityY << '\n';
	output << "hot_temperature_amplitude=" << summary.hotTemperatureAmplitude << '\n';
	output << "initial_thermal_center_y=" << summary.initialThermalCenterY << '\n';
	output << "final_thermal_center_y=" << summary.finalThermalCenterY << '\n';
	output << "thermal_center_rise=" << summary.thermalCenterRise << '\n';
	output << "thermal_weighted_velocity_y=" << summary.thermalWeightedVelocityY << '\n';
	output << "control_maximum_absolute_velocity="
		<< summary.controlMaximumAbsoluteVelocity << '\n';
	output << "heated_maximum_upward_velocity="
		<< summary.heatedMaximumUpwardVelocity << '\n';
	output << "heated_minimum_downward_velocity="
		<< summary.heatedMinimumDownwardVelocity << '\n';
	output << "heated_maximum_absolute_velocity="
		<< summary.heatedMaximumAbsoluteVelocity << '\n';
	output << "maximum_upward_velocity_difference="
		<< summary.maximumUpwardVelocityDifference << '\n';
	output << "minimum_downward_velocity_difference="
		<< summary.minimumDownwardVelocityDifference << '\n';
	output << "maximum_absolute_velocity_difference="
		<< summary.maximumAbsoluteVelocityDifference << '\n';
	output << "circulation_observed=" << (summary.circulationObserved ? "true" : "false") << '\n';
	const auto &heatedInitial = summary.heated.ledger.initial;
	const auto &heatedFinal = summary.heated.ledger.final;
	output << "minimum_density=" << summary.heated.minimumDensity << '\n';
	output << "minimum_pressure=" << summary.heated.minimumPressure << '\n';
	output << "mass_drift=" << (heatedFinal.density - heatedInitial.density) << '\n';
	output << "momentum_x_drift="
		<< (heatedFinal.momentumX - heatedInitial.momentumX) << '\n';
	output << "momentum_y_drift="
		<< (heatedFinal.momentumY - heatedInitial.momentumY) << '\n';
	output << "momentum_drift=" << std::hypot(
		heatedFinal.momentumX - heatedInitial.momentumX,
		heatedFinal.momentumY - heatedInitial.momentumY) << '\n';
	output << "energy_drift="
		<< (heatedFinal.totalEnergyDensity - heatedInitial.totalEnergyDensity) << '\n';
	output << "numerical_correction_count="
		<< (summary.control.corrections.eventCount + summary.heated.corrections.eventCount) << '\n';
	output << "correction_mass_added="
		<< (summary.control.corrections.massAdded + summary.heated.corrections.massAdded) << '\n';
	output << "correction_mass_removed="
		<< (summary.control.corrections.massRemoved + summary.heated.corrections.massRemoved) << '\n';
	output << "correction_momentum_x_added="
		<< (summary.control.corrections.momentumXAdded
			+ summary.heated.corrections.momentumXAdded) << '\n';
	output << "correction_momentum_y_added="
		<< (summary.control.corrections.momentumYAdded
			+ summary.heated.corrections.momentumYAdded) << '\n';
	output << "correction_energy_added="
		<< (summary.control.corrections.energyAdded + summary.heated.corrections.energyAdded) << '\n';
	output << "correction_energy_removed="
		<< (summary.control.corrections.energyRemoved + summary.heated.corrections.energyRemoved) << '\n';
	output << "density_floor_hits="
		<< (summary.control.corrections.densityFloorHits
			+ summary.heated.corrections.densityFloorHits) << '\n';
	output << "pressure_floor_hits="
		<< (summary.control.corrections.pressureFloorHits
			+ summary.heated.corrections.pressureFloorHits) << '\n';
	output << "correction_event_count="
		<< (summary.control.corrections.eventCount + summary.heated.corrections.eventCount) << '\n';
	WriteConvectionSubresult(output, "control", summary.control);
	WriteConvectionSubresult(output, "heated", summary.heated);
	output << "state_bytes_per_cell=" << sizeof(ConservativeState) << '\n';
	output << "state_and_flux_scratch_bytes_per_cell="
		<< summary.heated.stateAndFluxScratchBytesPerCell << '\n';
	output << "state_and_flux_scratch_bytes_total="
		<< summary.heated.stateAndFluxScratchBytesTotal << '\n';
	output << "probe_passed=" << (summary.passed ? "true" : "false") << '\n';
	return summary.passed;
}

bool WriteHllc2DPerformanceProbe(std::ostream &output)
{
	const auto summary = RunHllc2DPerformance();
	const auto &probe = summary.particleGrid.probe;
	const auto &initial = probe.ledger.initial;
	const auto &final = probe.ledger.final;
	output << "schema_version=1\n";
	output << "case=hllc_performance_2d\n";
	output << "candidate=fvm_hllc_rusanov_fallback\n";
	output << "candidate_solver_implemented=true\n";
	output << "atmosphere_solver_selection=unselected\n";
	output << "physical_scale_selection=unselected\n";
	output << "physical_time_policy=unselected\n";
	output << "performance_budget_status=unselected\n";
	output << "result_status=candidate_result_not_selection\n";
	output << "case_time_domain=nondimensional_contract\n";
	output << "dimension=2\n";
	output << "boundary_mode=periodic\n";
	output << "grid_cells_x=" << probe.benchmarkCase.grid.cellsX << '\n';
	output << "grid_cells_y=" << probe.benchmarkCase.grid.cellsY << '\n';
	output << "grid_cell_count=" << probe.benchmarkCase.grid.CellCount() << '\n';
	output << "cell_length=" << probe.benchmarkCase.grid.cellLength << '\n';
	output << "case_timestep=" << probe.benchmarkCase.timeStep << '\n';
	output << "case_step_count=" << probe.benchmarkCase.stepCount << '\n';
	output << "performance_warmup_count=" << summary.warmupCount << '\n';
	output << "performance_repeat_count=" << summary.repeatCount << '\n';
	output << "performance_steps=" << summary.stepCount << '\n';
	output << "legacy_grid_cells_x="
		<< summary.legacyGrid.probe.benchmarkCase.grid.cellsX << '\n';
	output << "legacy_grid_cells_y="
		<< summary.legacyGrid.probe.benchmarkCase.grid.cellsY << '\n';
	output << "legacy_grid_cell_count="
		<< summary.legacyGrid.probe.benchmarkCase.grid.CellCount() << '\n';
	output << "doubled_grid_cells_x="
		<< summary.doubledGrid.probe.benchmarkCase.grid.cellsX << '\n';
	output << "doubled_grid_cells_y="
		<< summary.doubledGrid.probe.benchmarkCase.grid.cellsY << '\n';
	output << "doubled_grid_cell_count="
		<< summary.doubledGrid.probe.benchmarkCase.grid.CellCount() << '\n';
	output << "particle_grid_cells_x="
		<< summary.particleGrid.probe.benchmarkCase.grid.cellsX << '\n';
	output << "particle_grid_cells_y="
		<< summary.particleGrid.probe.benchmarkCase.grid.cellsY << '\n';
	output << "particle_grid_cell_count="
		<< summary.particleGrid.probe.benchmarkCase.grid.CellCount() << '\n';
	output << "legacy_grid_elapsed_milliseconds="
		<< summary.legacyGrid.elapsedMilliseconds << '\n';
	output << "doubled_grid_elapsed_milliseconds="
		<< summary.doubledGrid.elapsedMilliseconds << '\n';
	output << "particle_grid_elapsed_milliseconds="
		<< summary.particleGrid.elapsedMilliseconds << '\n';
	output << "legacy_grid_milliseconds_per_step="
		<< summary.legacyGrid.millisecondsPerStep << '\n';
	output << "doubled_grid_milliseconds_per_step="
		<< summary.doubledGrid.millisecondsPerStep << '\n';
	output << "particle_grid_milliseconds_per_step="
		<< summary.particleGrid.millisecondsPerStep << '\n';
	output << "legacy_grid_cell_updates_per_second="
		<< summary.legacyGrid.cellUpdatesPerSecond << '\n';
	output << "doubled_grid_cell_updates_per_second="
		<< summary.doubledGrid.cellUpdatesPerSecond << '\n';
	output << "particle_grid_cell_updates_per_second="
		<< summary.particleGrid.cellUpdatesPerSecond << '\n';
	output << "reference_frame_budget_milliseconds="
		<< summary.referenceFrameBudgetMilliseconds << '\n';
	output << "legacy_grid_fraction_of_reference_frame="
		<< summary.legacyGrid.millisecondsPerStep
			/ summary.referenceFrameBudgetMilliseconds << '\n';
	output << "doubled_grid_fraction_of_reference_frame="
		<< summary.doubledGrid.millisecondsPerStep
			/ summary.referenceFrameBudgetMilliseconds << '\n';
	output << "particle_grid_fraction_of_reference_frame="
		<< summary.particleGrid.millisecondsPerStep
			/ summary.referenceFrameBudgetMilliseconds << '\n';
	output << "maximum_cfl=" << probe.maximumCfl << '\n';
	output << "minimum_density=" << probe.minimumDensity << '\n';
	output << "minimum_pressure=" << probe.minimumPressure << '\n';
	output << "mass_drift=" << (final.density - initial.density) << '\n';
	output << "momentum_x_drift=" << (final.momentumX - initial.momentumX) << '\n';
	output << "momentum_y_drift=" << (final.momentumY - initial.momentumY) << '\n';
	output << "momentum_drift=" << std::hypot(
		final.momentumX - initial.momentumX,
		final.momentumY - initial.momentumY) << '\n';
	output << "energy_drift=" << (final.totalEnergyDensity - initial.totalEnergyDensity) << '\n';
	output << "state_change_l1=" << probe.stateChangeL1 << '\n';
	output << "state_evolved=" << (probe.stateEvolved ? "true" : "false") << '\n';
	output << "positivity_preserved="
		<< (probe.positivityPreserved ? "true" : "false") << '\n';
	output << "legacy_grid_flux_fallback_count="
		<< summary.legacyGrid.probe.fluxFallbackCount << '\n';
	output << "doubled_grid_flux_fallback_count="
		<< summary.doubledGrid.probe.fluxFallbackCount << '\n';
	output << "particle_grid_flux_fallback_count="
		<< summary.particleGrid.probe.fluxFallbackCount << '\n';
	output << "numerical_correction_count=" << probe.corrections.eventCount << '\n';
	output << "correction_mass_added=" << probe.corrections.massAdded << '\n';
	output << "correction_mass_removed=" << probe.corrections.massRemoved << '\n';
	output << "correction_momentum_x_added=" << probe.corrections.momentumXAdded << '\n';
	output << "correction_momentum_y_added=" << probe.corrections.momentumYAdded << '\n';
	output << "correction_energy_added=" << probe.corrections.energyAdded << '\n';
	output << "correction_energy_removed=" << probe.corrections.energyRemoved << '\n';
	output << "density_floor_hits=" << probe.corrections.densityFloorHits << '\n';
	output << "pressure_floor_hits=" << probe.corrections.pressureFloorHits << '\n';
	output << "correction_event_count=" << probe.corrections.eventCount << '\n';
	output << "state_bytes_per_cell=" << sizeof(ConservativeState) << '\n';
	output << "state_and_flux_scratch_bytes_per_cell="
		<< probe.stateAndFluxScratchBytesPerCell << '\n';
	output << "state_and_flux_scratch_bytes_total="
		<< probe.stateAndFluxScratchBytesTotal << '\n';
	output << "performance_measurement_passed=" << (summary.passed ? "true" : "false") << '\n';
	output << "probe_passed=" << (summary.passed ? "true" : "false") << '\n';
	return summary.passed;
}

} // namespace omni::atmospherebench
