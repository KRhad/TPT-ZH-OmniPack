#include "Rusanov1D.h"

#include <algorithm>
#include <array>
#include <chrono>
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
	constexpr double CellLength = 1.0;
	constexpr std::size_t UniformCells = 64;
	constexpr std::size_t UniformSteps = 16;
	constexpr double UniformTimeStep = 0.05;
	constexpr std::size_t PressurePulseCells = 128;
	constexpr std::size_t PressurePulseSteps = 64;
	constexpr double PressurePulseTimeStep = 0.02;
	constexpr double PressurePulseAmplitude = 0.1;
	constexpr double PressurePulseWidthCells = 8.0;
	constexpr std::size_t DensityAdvectionCells = 128;
	constexpr std::size_t DensityAdvectionSteps = 20;
	constexpr double DensityAdvectionTimeStep = 0.1;
	constexpr double DensityAdvectionVelocity = 0.5;
	constexpr double DensityAdvectionAmplitude = 0.2;
	constexpr double Pi = 3.141592653589793238462643383279502884;
	constexpr std::size_t ContactCells = 128;
	constexpr std::size_t ContactSteps = 20;
	constexpr double ContactTimeStep = 0.1;
	constexpr double ContactVelocity = 0.5;
	constexpr double ContactLowDensity = 0.8;
	constexpr double ContactHighDensity = 1.2;
	constexpr std::size_t NearVacuumCells = 128;
	constexpr std::size_t NearVacuumSteps = 32;
	constexpr double NearVacuumTimeStep = 0.02;
	constexpr double NearVacuumHighDensity = 1.0;
	constexpr double NearVacuumHighPressure = 1.0;
	constexpr double NearVacuumLowDensity = 1e-6;
	constexpr double NearVacuumLowPressure = 1e-8;
	constexpr double SodGamma = 1.4;
	constexpr std::size_t SodCells = 256;
	constexpr std::size_t SodSteps = 400;
	constexpr double SodCellLength = 1.0 / static_cast<double>(SodCells);
	constexpr double SodTimeStep = 0.0005;
	constexpr double SodLeftDensity = 1.0;
	constexpr double SodLeftPressure = 1.0;
	constexpr double SodRightDensity = 0.125;
	constexpr double SodRightPressure = 0.1;
	constexpr double RefinementTotalTime = 0.25;
	constexpr std::size_t RefinementCoarseCells = 64;
	constexpr std::size_t RefinementMediumCells = 128;
	constexpr std::size_t RefinementFineCells = 256;
	constexpr std::size_t LowMachCells = 128;
	constexpr double LowMachShiftDistance = 0.125;
	constexpr double LowMachTargetCfl = 0.45;
	constexpr double ModerateMachVelocity = 0.5;
	constexpr double LowMachVelocity = 0.05;
	constexpr double VeryLowMachVelocity = 0.005;
	constexpr std::size_t LeakCells = 128;
	constexpr std::size_t LeakSteps = 120;
	constexpr double LeakCellLength = 1.0 / static_cast<double>(LeakCells);
	constexpr double LeakTimeStep = 0.001;
	constexpr double LeakInteriorDensity = 1.0;
	constexpr double LeakInteriorPressure = 1.0;
	constexpr double LeakExteriorDensity = 0.125;
	constexpr double LeakExteriorPressure = 0.1;
	constexpr std::size_t PerformanceSmallCells = 153 * 96;
	constexpr std::size_t PerformanceMediumCells = 2 * PerformanceSmallCells;
	constexpr std::size_t PerformanceLargeCells = 4 * PerformanceSmallCells;
	constexpr std::size_t PerformanceSteps = 64;
	constexpr std::size_t PerformanceWarmups = 1;
	constexpr std::size_t PerformanceRepeats = 3;
	constexpr double PerformanceVelocity = 0.5;
	constexpr double PerformanceTargetCfl = 0.2;

	enum class FluxDissipationModel
	{
		Rusanov,
		AllSpeedRusanov,
		HllcRusanovFallback,
	};

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

	ConservativeState AllSpeedRusanovFluxX(
		const ConservativeState &left,
		const ConservativeState &right,
		const IdealGasEOS &eos)
	{
		const auto leftPrimitive = eos.ToPrimitive(left);
		const auto rightPrimitive = eos.ToPrimitive(right);
		if (!leftPrimitive.valid || !rightPrimitive.valid)
			return {};
		const double localVelocity = std::max(
			std::abs(leftPrimitive.velocityX), std::abs(rightPrimitive.velocityX));
		const double soundSpeed = std::max(leftPrimitive.soundSpeed, rightPrimitive.soundSpeed);
		const double localMach = localVelocity / soundSpeed;
		const double minimumPressure = std::max(
			std::min(leftPrimitive.pressure, rightPrimitive.pressure), 1e-12);
		const double pressureJump = std::abs(rightPrimitive.pressure - leftPrimitive.pressure)
			/ minimumPressure;
		const double shockSensor = std::clamp(pressureJump / 0.1, 0.0, 1.0);
		const double acousticScale = std::max(0.01, std::max(localMach, shockSensor));
		const double maximumWaveSpeed = localVelocity + acousticScale * soundSpeed;
		return Subtract(
			Scale(0.5, Add(FluxX(left, leftPrimitive), FluxX(right, rightPrimitive))),
			Scale(0.5 * maximumWaveSpeed, Subtract(right, left))
		);
	}

	bool HllcFluxX(
		const ConservativeState &left,
		const ConservativeState &right,
		const IdealGasEOS &eos,
		ConservativeState &flux)
	{
		const auto leftPrimitive = eos.ToPrimitive(left);
		const auto rightPrimitive = eos.ToPrimitive(right);
		if (!leftPrimitive.valid || !rightPrimitive.valid)
			return false;
		const double leftWaveSpeed = std::min(
			leftPrimitive.velocityX - leftPrimitive.soundSpeed,
			rightPrimitive.velocityX - rightPrimitive.soundSpeed);
		const double rightWaveSpeed = std::max(
			leftPrimitive.velocityX + leftPrimitive.soundSpeed,
			rightPrimitive.velocityX + rightPrimitive.soundSpeed);
		const double contactDenominator =
			leftPrimitive.density * (leftWaveSpeed - leftPrimitive.velocityX)
			- rightPrimitive.density * (rightWaveSpeed - rightPrimitive.velocityX);
		if (!std::isfinite(contactDenominator) || std::abs(contactDenominator) <= 1e-14)
			return false;
		const double contactWaveSpeed = (
			rightPrimitive.pressure - leftPrimitive.pressure
			+ leftPrimitive.density * leftPrimitive.velocityX
				* (leftWaveSpeed - leftPrimitive.velocityX)
			- rightPrimitive.density * rightPrimitive.velocityX
				* (rightWaveSpeed - rightPrimitive.velocityX)) / contactDenominator;
		if (!std::isfinite(contactWaveSpeed)
			|| contactWaveSpeed <= leftWaveSpeed || contactWaveSpeed >= rightWaveSpeed)
			return false;

		auto starState = [contactWaveSpeed](
			const ConservativeState &state,
			const PrimitiveState &primitive,
			double waveSpeed,
			ConservativeState &star)
		{
			const double waveDenominator = waveSpeed - contactWaveSpeed;
			const double stateDenominator = waveSpeed - primitive.velocityX;
			if (!std::isfinite(waveDenominator) || !std::isfinite(stateDenominator)
				|| std::abs(waveDenominator) <= 1e-14 || std::abs(stateDenominator) <= 1e-14)
				return false;
			const double starDensity = primitive.density * stateDenominator / waveDenominator;
			const double specificTotalEnergy = state.totalEnergyDensity / primitive.density;
			const double starSpecificEnergy = specificTotalEnergy
				+ (contactWaveSpeed - primitive.velocityX)
					* (contactWaveSpeed + primitive.pressure
						/ (primitive.density * stateDenominator));
			star = {
				starDensity,
				starDensity * contactWaveSpeed,
				starDensity * primitive.velocityY,
				starDensity * starSpecificEnergy,
			};
			return std::isfinite(star.density) && std::isfinite(star.momentumX)
				&& std::isfinite(star.momentumY) && std::isfinite(star.totalEnergyDensity)
				&& star.density > 0.0 && star.totalEnergyDensity > 0.0;
		};

		const auto leftFlux = FluxX(left, leftPrimitive);
		const auto rightFlux = FluxX(right, rightPrimitive);
		if (0.0 <= leftWaveSpeed)
		{
			flux = leftFlux;
			return true;
		}
		if (rightWaveSpeed <= 0.0)
		{
			flux = rightFlux;
			return true;
		}
		if (0.0 <= contactWaveSpeed)
		{
			ConservativeState leftStar;
			if (!starState(left, leftPrimitive, leftWaveSpeed, leftStar)
				|| !eos.ToPrimitive(leftStar).valid)
				return false;
			flux = Add(leftFlux, Scale(leftWaveSpeed, Subtract(leftStar, left)));
			return true;
		}
		ConservativeState rightStar;
		if (!starState(right, rightPrimitive, rightWaveSpeed, rightStar)
			|| !eos.ToPrimitive(rightStar).valid)
			return false;
		flux = Add(rightFlux, Scale(rightWaveSpeed, Subtract(rightStar, right)));
		return true;
	}

	ConservativeState NumericalFluxX(
		const ConservativeState &left,
		const ConservativeState &right,
		const IdealGasEOS &eos,
		FluxDissipationModel model,
		std::size_t *fallbackCount)
	{
		if (model == FluxDissipationModel::AllSpeedRusanov)
			return AllSpeedRusanovFluxX(left, right, eos);
		if (model == FluxDissipationModel::HllcRusanovFallback)
		{
			ConservativeState flux;
			if (HllcFluxX(left, right, eos, flux))
				return flux;
			if (fallbackCount)
				++*fallbackCount;
		}
		return RusanovFluxX(left, right, eos);
	}

	ConservativeState SealedWallFluxX(const ConservativeState &state, const IdealGasEOS &eos)
	{
		const auto primitive = eos.ToPrimitive(state);
		if (!primitive.valid)
			return {};
		return {0.0, primitive.pressure, 0.0, 0.0};
	}

	bool StateNear(
		const ConservativeState &left,
		const ConservativeState &right,
		double tolerance)
	{
		return std::abs(left.density - right.density) <= tolerance
			&& std::abs(left.momentumX - right.momentumX) <= tolerance
			&& std::abs(left.momentumY - right.momentumY) <= tolerance
			&& std::abs(left.totalEnergyDensity - right.totalEnergyDensity) <= tolerance;
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

	double StateChangeL1(
		const std::vector<ConservativeState> &initial,
		const std::vector<ConservativeState> &final)
	{
		if (initial.size() != final.size())
			return std::numeric_limits<double>::quiet_NaN();
		double change = 0.0;
		for (std::size_t cell = 0; cell < initial.size(); ++cell)
		{
			change += std::abs(final[cell].density - initial[cell].density);
			change += std::abs(final[cell].momentumX - initial[cell].momentumX);
			change += std::abs(final[cell].momentumY - initial[cell].momentumY);
			change += std::abs(final[cell].totalEnergyDensity - initial[cell].totalEnergyDensity);
		}
		return change;
	}

	double DensityTotalVariation(const std::vector<ConservativeState> &cells)
	{
		if (cells.empty())
			return 0.0;
		double variation = 0.0;
		for (std::size_t cell = 0; cell < cells.size(); ++cell)
			variation += std::abs(cells[(cell + 1) % cells.size()].density - cells[cell].density);
		return variation;
	}

	RusanovProbeSummary RunOneDimensionalProbe(
		BenchmarkCase benchmarkCase,
		std::vector<ConservativeState> initialCells,
		const IdealGasEOS &eos,
		bool requireEvolution,
		bool requirePeakReduction,
		double conservationTolerance,
		const ConservativeState *leftBoundaryState,
		const ConservativeState *rightBoundaryState,
		std::vector<ConservativeState> *finalCellsOutput,
		FluxDissipationModel fluxModel = FluxDissipationModel::Rusanov)
	{
		RusanovProbeSummary summary{benchmarkCase};
		if (!summary.benchmarkCase.IsValid() || !eos.IsValid()
			|| initialCells.size() != summary.benchmarkCase.grid.CellCount()
			|| (summary.benchmarkCase.grid.boundaryMode != BoundaryMode::Periodic
				&& summary.benchmarkCase.grid.boundaryMode != BoundaryMode::Sealed
				&& summary.benchmarkCase.grid.boundaryMode != BoundaryMode::Open))
			return summary;

		std::vector<ConservativeState> cells = initialCells;
		const bool periodic = summary.benchmarkCase.grid.boundaryMode == BoundaryMode::Periodic;
		const bool open = summary.benchmarkCase.grid.boundaryMode == BoundaryMode::Open;
		if (open && rightBoundaryState == nullptr)
			return summary;
		std::vector<ConservativeState> fluxes(cells.size() + (periodic ? 0 : 1));
		std::vector<ConservativeState> next(cells.size());
		const double lambda = summary.benchmarkCase.timeStep / summary.benchmarkCase.grid.cellLength;
		summary.ledger.Begin(TotalState(cells));
		summary.initialMaximumPressure = MaximumPressure(cells, eos);
		if (!std::isfinite(summary.initialMaximumPressure))
			return summary;

		for (std::size_t step = 0; step < summary.benchmarkCase.stepCount; ++step)
		{
			double stepCfl = 0.0;
			for (const auto &cell : cells)
			{
				const auto primitive = eos.ToPrimitive(cell);
				if (!primitive.valid)
					return summary;
				stepCfl = std::max(stepCfl,
					lambda * (std::abs(primitive.velocityX) + primitive.soundSpeed));
			}
			summary.maximumCfl = std::max(summary.maximumCfl, stepCfl);
			if (!std::isfinite(stepCfl) || stepCfl <= 0.0 || stepCfl > 1.0)
				return summary;

			if (periodic)
			{
				for (std::size_t cell = 0; cell < cells.size(); ++cell)
					fluxes[cell] = NumericalFluxX(
						cells[cell], cells[(cell + 1) % cells.size()], eos, fluxModel,
						&summary.fluxFallbackCount);
				for (std::size_t cell = 0; cell < cells.size(); ++cell)
				{
					const auto leftFace = fluxes[(cell + cells.size() - 1) % cells.size()];
					next[cell] = Subtract(cells[cell], Scale(lambda, Subtract(fluxes[cell], leftFace)));
					if (!eos.ToPrimitive(next[cell]).valid)
						return summary;
				}
			}
			else
			{
				fluxes.front() = open && leftBoundaryState
					? NumericalFluxX(*leftBoundaryState, cells.front(), eos, fluxModel,
						&summary.fluxFallbackCount)
					: SealedWallFluxX(cells.front(), eos);
				for (std::size_t face = 1; face < cells.size(); ++face)
					fluxes[face] = NumericalFluxX(cells[face - 1], cells[face], eos, fluxModel,
						&summary.fluxFallbackCount);
				fluxes.back() = open
					? NumericalFluxX(cells.back(), *rightBoundaryState, eos, fluxModel,
						&summary.fluxFallbackCount)
					: SealedWallFluxX(cells.back(), eos);
				const auto leftExchange = Scale(lambda, fluxes.front());
				const auto rightExchange = Scale(-lambda, fluxes.back());
				summary.leftBoundaryExchange = Add(summary.leftBoundaryExchange, leftExchange);
				summary.rightBoundaryExchange = Add(summary.rightBoundaryExchange, rightExchange);
				summary.boundaryExchange = Add(summary.boundaryExchange,
					Add(leftExchange, rightExchange));
				for (std::size_t cell = 0; cell < cells.size(); ++cell)
				{
					next[cell] = Subtract(cells[cell],
						Scale(lambda, Subtract(fluxes[cell + 1], fluxes[cell])));
					if (!eos.ToPrimitive(next[cell]).valid)
						return summary;
				}
			}
			cells.swap(next);
		}

		summary.minimumDensity = std::numeric_limits<double>::infinity();
		summary.maximumDensity = 0.0;
		summary.minimumPressure = std::numeric_limits<double>::infinity();
		summary.minimumEnergyDensity = std::numeric_limits<double>::infinity();
		summary.minimumVelocityX = std::numeric_limits<double>::infinity();
		summary.maximumVelocityX = -std::numeric_limits<double>::infinity();
		summary.finalMaximumPressure = 0.0;
		summary.positivityPreserved = true;
		for (const auto &cell : cells)
		{
			const auto primitive = eos.ToPrimitive(cell);
			if (!primitive.valid)
			{
				summary.positivityPreserved = false;
				break;
			}
			summary.minimumDensity = std::min(summary.minimumDensity, primitive.density);
			summary.maximumDensity = std::max(summary.maximumDensity, primitive.density);
			summary.minimumPressure = std::min(summary.minimumPressure, primitive.pressure);
			summary.minimumEnergyDensity = std::min(summary.minimumEnergyDensity, cell.totalEnergyDensity);
			summary.finalMaximumPressure = std::max(summary.finalMaximumPressure, primitive.pressure);
			summary.minimumVelocityX = std::min(summary.minimumVelocityX, primitive.velocityX);
			summary.maximumVelocityX = std::max(summary.maximumVelocityX, primitive.velocityX);
		}
		summary.stateChangeL1 = StateChangeL1(initialCells, cells);
		summary.stateEvolved = std::isfinite(summary.stateChangeL1) && summary.stateChangeL1 > 1e-12;
		summary.pressurePeakReduced = summary.finalMaximumPressure < summary.initialMaximumPressure;
		summary.ledger.End(TotalState(cells));
		const auto expectedFinal = Add(summary.ledger.initial, summary.boundaryExchange);
		summary.boundaryLedgerCloses = periodic
			? summary.ledger.Closes(conservationTolerance)
			: StateNear(summary.ledger.final, expectedFinal, conservationTolerance);
		summary.passed = summary.positivityPreserved
		&& summary.boundaryLedgerCloses
		&& summary.corrections.IsEmpty() && summary.maximumCfl <= 1.0
		&& (!requireEvolution || summary.stateEvolved)
		&& (!requirePeakReduction || summary.pressurePeakReduced);
		if (finalCellsOutput)
			*finalCellsOutput = cells;
		return summary;
	}

	RusanovProbeSummary RunPeriodicProbe(
		BenchmarkCase benchmarkCase,
		std::vector<ConservativeState> initialCells,
		const IdealGasEOS &eos,
		bool requireEvolution,
		bool requirePeakReduction,
		double conservationTolerance,
		std::vector<ConservativeState> *finalCellsOutput,
		FluxDissipationModel fluxModel = FluxDissipationModel::Rusanov)
	{
		if (benchmarkCase.grid.boundaryMode != BoundaryMode::Periodic)
			return {benchmarkCase};
		return RunOneDimensionalProbe(benchmarkCase, std::move(initialCells), eos,
			requireEvolution, requirePeakReduction, conservationTolerance,
			nullptr, nullptr, finalCellsOutput, fluxModel);
	}

	RusanovProbeSummary RunSealedProbe(
		BenchmarkCase benchmarkCase,
		std::vector<ConservativeState> initialCells,
		const IdealGasEOS &eos,
		bool requireEvolution,
		double conservationTolerance,
		std::vector<ConservativeState> *finalCellsOutput,
		FluxDissipationModel fluxModel = FluxDissipationModel::Rusanov)
	{
		if (benchmarkCase.grid.boundaryMode != BoundaryMode::Sealed)
			return {benchmarkCase};
		return RunOneDimensionalProbe(benchmarkCase, std::move(initialCells), eos,
			requireEvolution, false, conservationTolerance,
			nullptr, nullptr, finalCellsOutput, fluxModel);
	}

	RusanovProbeSummary RunOpenProbe(
		BenchmarkCase benchmarkCase,
		std::vector<ConservativeState> initialCells,
		const ConservativeState &rightBoundaryState,
		const IdealGasEOS &eos,
		bool requireEvolution,
		double conservationTolerance,
		std::vector<ConservativeState> *finalCellsOutput,
		FluxDissipationModel fluxModel = FluxDissipationModel::Rusanov)
	{
		if (benchmarkCase.grid.boundaryMode != BoundaryMode::Open)
			return {benchmarkCase};
		return RunOneDimensionalProbe(benchmarkCase, std::move(initialCells), eos,
			requireEvolution, false, conservationTolerance,
			nullptr, &rightBoundaryState, finalCellsOutput, fluxModel);
	}

	void EvaluateAdvectedDensity(
		RusanovProbeSummary &summary,
		const std::vector<ConservativeState> &initial,
		const std::vector<ConservativeState> &final,
		const IdealGasEOS &eos,
		double referenceVelocity,
		double densityL1Tolerance,
		double densityLinfTolerance,
		double minimumExpectedDensity,
		double maximumExpectedDensity)
	{
		summary.referenceVelocity = referenceVelocity;
		const double shiftReal = referenceVelocity * summary.benchmarkCase.timeStep
			* static_cast<double>(summary.benchmarkCase.stepCount)
			/ summary.benchmarkCase.grid.cellLength;
		const auto shiftRounded = static_cast<long long>(std::llround(shiftReal));
		const bool integralShift = std::abs(shiftReal - static_cast<double>(shiftRounded)) <= 1e-12
			&& shiftRounded >= 0
			&& static_cast<unsigned long long>(shiftRounded)
				< static_cast<unsigned long long>(initial.size());
		if (!integralShift || final.size() != initial.size() || initial.empty())
			return;

		summary.referenceShiftCells = static_cast<std::size_t>(shiftRounded);
		double densityL1 = 0.0;
		double densityLinf = 0.0;
		double pressureLinf = 0.0;
		for (std::size_t cell = 0; cell < final.size(); ++cell)
		{
			const auto referenceCell = initial[
				(cell + initial.size() - summary.referenceShiftCells) % initial.size()];
			const double densityError = std::abs(final[cell].density - referenceCell.density);
			densityL1 += densityError;
			densityLinf = std::max(densityLinf, densityError);
			const auto primitive = eos.ToPrimitive(final[cell]);
			if (primitive.valid)
				pressureLinf = std::max(pressureLinf, std::abs(primitive.pressure - 1.0));
		}
		summary.densityL1Error = densityL1 / static_cast<double>(final.size());
		summary.densityLinfError = densityLinf;
		summary.pressureLinfError = pressureLinf;
		const double initialVariation = DensityTotalVariation(initial);
		const double finalVariation = DensityTotalVariation(final);
		summary.totalVariationRatio = initialVariation > 0.0 ? finalVariation / initialVariation : 0.0;
		summary.densityBoundsPreserved = summary.minimumDensity >= minimumExpectedDensity - 1e-12
			&& summary.maximumDensity <= maximumExpectedDensity + 1e-12;
		summary.advectionReferencePassed = summary.densityL1Error <= densityL1Tolerance
			&& summary.densityLinfError <= densityLinfTolerance
			&& summary.pressureLinfError <= 1e-10
			&& summary.totalVariationRatio > 0.0
			&& summary.totalVariationRatio <= 1.0 + 1e-10
			&& summary.densityBoundsPreserved;
	}

	RusanovProbeSummary RunDensityAdvectionRefinementCase(std::size_t cellsCount)
	{
		const IdealGasEOS eos(Gamma, SpecificGasConstant);
		const double cellLength = 1.0 / static_cast<double>(cellsCount);
		const std::size_t steps = cellsCount;
		const double timeStep = RefinementTotalTime / static_cast<double>(steps);
		std::vector<ConservativeState> initial(cellsCount);
		for (std::size_t cell = 0; cell < cellsCount; ++cell)
		{
			const double x = (static_cast<double>(cell) + 0.5) * cellLength;
			const double density = 1.0 + DensityAdvectionAmplitude * std::sin(2.0 * Pi * x);
			initial[cell] = eos.FromPrimitive(density, DensityAdvectionVelocity, 0.0, 1.0);
		}
		std::vector<ConservativeState> final;
		auto summary = RunPeriodicProbe(
			{"rusanov_density_advection_refinement_1d",
				{cellsCount, 1, cellLength, BoundaryMode::Periodic},
				TimeDomain::NondimensionalContract, timeStep, steps},
			initial, eos, true, false, 1e-9, &final);
		EvaluateAdvectedDensity(summary, initial, final, eos, DensityAdvectionVelocity,
			1.0, 1.0, 1.0 - DensityAdvectionAmplitude, 1.0 + DensityAdvectionAmplitude);
		summary.passed = summary.passed && summary.advectionReferencePassed;
		return summary;
	}

	RusanovProbeSummary RunLowMachAdvectionCase(
		double velocity,
		FluxDissipationModel fluxModel = FluxDissipationModel::Rusanov)
	{
		const IdealGasEOS eos(Gamma, SpecificGasConstant);
		const double cellLength = 1.0 / static_cast<double>(LowMachCells);
		const double totalTime = LowMachShiftDistance / velocity;
		const double maximumSoundSpeed = std::sqrt(
			Gamma / (1.0 - DensityAdvectionAmplitude));
		const double maximumTimeStep = LowMachTargetCfl * cellLength
			/ (velocity + maximumSoundSpeed);
		const auto steps = static_cast<std::size_t>(std::ceil(totalTime / maximumTimeStep));
		const double timeStep = totalTime / static_cast<double>(steps);
		std::vector<ConservativeState> initial(LowMachCells);
		for (std::size_t cell = 0; cell < LowMachCells; ++cell)
		{
			const double x = (static_cast<double>(cell) + 0.5) * cellLength;
			const double density = 1.0 + DensityAdvectionAmplitude * std::sin(2.0 * Pi * x);
			initial[cell] = eos.FromPrimitive(density, velocity, 0.0, 1.0);
		}
		std::vector<ConservativeState> final;
		auto summary = RunPeriodicProbe(
			{"rusanov_low_mach_advection_1d",
				{LowMachCells, 1, cellLength, BoundaryMode::Periodic},
				TimeDomain::NondimensionalContract, timeStep, steps},
			initial, eos, true, false, 1e-8, &final, fluxModel);
		EvaluateAdvectedDensity(summary, initial, final, eos, velocity,
			1.0, 1.0, 1.0 - DensityAdvectionAmplitude, 1.0 + DensityAdvectionAmplitude);
		summary.simulatedTime = totalTime;
		summary.passed = summary.passed && summary.advectionReferencePassed;
		return summary;
	}

	RusanovProbeSummary RunPerformanceCase(
		std::size_t cellsCount,
		FluxDissipationModel fluxModel,
		std::string_view caseId)
	{
		const IdealGasEOS eos(Gamma, SpecificGasConstant);
		const double cellLength = 1.0 / static_cast<double>(cellsCount);
		const double maximumSoundSpeed = std::sqrt(
			Gamma / (1.0 - DensityAdvectionAmplitude));
		const double timeStep = PerformanceTargetCfl * cellLength
			/ (PerformanceVelocity + maximumSoundSpeed);
		std::vector<ConservativeState> initial(cellsCount);
		for (std::size_t cell = 0; cell < cellsCount; ++cell)
		{
			const double x = (static_cast<double>(cell) + 0.5) * cellLength;
			const double density = 1.0 + DensityAdvectionAmplitude * std::sin(2.0 * Pi * x);
			initial[cell] = eos.FromPrimitive(density, PerformanceVelocity, 0.0, 1.0);
		}
		return RunPeriodicProbe(
			{caseId,
				{cellsCount, 1, cellLength, BoundaryMode::Periodic},
				TimeDomain::NondimensionalContract, timeStep, PerformanceSteps},
			std::move(initial), eos, true, false, 1e-7, nullptr, fluxModel);
	}

	RusanovPerformanceSample MeasurePerformanceCase(
		std::size_t cellsCount,
		FluxDissipationModel fluxModel,
		std::string_view caseId)
	{
		for (std::size_t warmup = 0; warmup < PerformanceWarmups; ++warmup)
		{
			if (!RunPerformanceCase(cellsCount, fluxModel, caseId).passed)
				return {};
		}
		std::array<double, PerformanceRepeats> elapsed{};
		RusanovProbeSummary finalProbe;
		for (std::size_t repeat = 0; repeat < PerformanceRepeats; ++repeat)
		{
			const auto start = std::chrono::steady_clock::now();
			auto probe = RunPerformanceCase(cellsCount, fluxModel, caseId);
			const auto stop = std::chrono::steady_clock::now();
			if (!probe.passed)
				return {};
			elapsed[repeat] = std::chrono::duration<double, std::milli>(stop - start).count();
			finalProbe = std::move(probe);
		}
		std::sort(elapsed.begin(), elapsed.end());
		const double medianMilliseconds = elapsed[elapsed.size() / 2];
		const double cellUpdates = static_cast<double>(cellsCount)
			* static_cast<double>(PerformanceSteps);
		const double throughput = medianMilliseconds > 0.0
			? cellUpdates * 1000.0 / medianMilliseconds
			: 0.0;
		return {
			std::move(finalProbe),
			medianMilliseconds,
			throughput,
			std::isfinite(medianMilliseconds) && medianMilliseconds > 0.0
				&& std::isfinite(throughput) && throughput > 0.0,
		};
	}
}

RusanovProbeSummary RunRusanovUniform()
{
	const IdealGasEOS eos(Gamma, SpecificGasConstant);
	const auto initial = eos.FromPrimitive(1.0, 0.0, 0.0, 1.0);
	return RunPeriodicProbe(
		{"rusanov_uniform_1d", {UniformCells, 1, CellLength, BoundaryMode::Periodic},
			TimeDomain::NondimensionalContract, UniformTimeStep, UniformSteps},
		std::vector<ConservativeState>(UniformCells, initial), eos, false, false, 1e-12, nullptr);
}

RusanovProbeSummary RunRusanovPressurePulse()
{
	const IdealGasEOS eos(Gamma, SpecificGasConstant);
	std::vector<ConservativeState> initial(PressurePulseCells);
	const double center = static_cast<double>(PressurePulseCells / 2);
	for (std::size_t cell = 0; cell < PressurePulseCells; ++cell)
	{
		const double distance = static_cast<double>(cell) - center;
		const double pulse = PressurePulseAmplitude * std::exp(
			-(distance * distance) / (2.0 * PressurePulseWidthCells * PressurePulseWidthCells));
		initial[cell] = eos.FromPrimitive(1.0, 0.0, 0.0, 1.0 + pulse);
	}
	return RunPeriodicProbe(
		{"rusanov_pressure_pulse_1d",
			{PressurePulseCells, 1, CellLength, BoundaryMode::Periodic},
			TimeDomain::NondimensionalContract, PressurePulseTimeStep, PressurePulseSteps},
		std::move(initial), eos, true, true, 1e-10, nullptr);
}

RusanovProbeSummary RunRusanovDensityAdvection()
{
	const IdealGasEOS eos(Gamma, SpecificGasConstant);
	std::vector<ConservativeState> initial(DensityAdvectionCells);
	for (std::size_t cell = 0; cell < DensityAdvectionCells; ++cell)
	{
		const double phase = 2.0 * Pi * static_cast<double>(cell)
			/ static_cast<double>(DensityAdvectionCells);
		const double density = 1.0 + DensityAdvectionAmplitude * std::sin(phase);
		initial[cell] = eos.FromPrimitive(
			density, DensityAdvectionVelocity, 0.0, 1.0);
	}
	std::vector<ConservativeState> final;
	auto summary = RunPeriodicProbe(
		{"rusanov_density_advection_1d",
			{DensityAdvectionCells, 1, CellLength, BoundaryMode::Periodic},
			TimeDomain::NondimensionalContract, DensityAdvectionTimeStep, DensityAdvectionSteps},
		initial, eos, true, false, 1e-10, &final);
	EvaluateAdvectedDensity(summary, initial, final, eos, DensityAdvectionVelocity,
		0.01, 0.02, 1.0 - DensityAdvectionAmplitude, 1.0 + DensityAdvectionAmplitude);
	summary.passed = summary.passed && summary.advectionReferencePassed;
	return summary;
}

RusanovProbeSummary RunRusanovContactDiscontinuity()
{
	const IdealGasEOS eos(Gamma, SpecificGasConstant);
	std::vector<ConservativeState> initial(ContactCells);
	for (std::size_t cell = 0; cell < ContactCells; ++cell)
	{
		const double density = cell < ContactCells / 2
			? ContactHighDensity
			: ContactLowDensity;
		initial[cell] = eos.FromPrimitive(density, ContactVelocity, 0.0, 1.0);
	}
	std::vector<ConservativeState> final;
	auto summary = RunPeriodicProbe(
		{"rusanov_contact_discontinuity_1d",
			{ContactCells, 1, CellLength, BoundaryMode::Periodic},
			TimeDomain::NondimensionalContract, ContactTimeStep, ContactSteps},
		initial, eos, true, false, 1e-10, &final);
	EvaluateAdvectedDensity(summary, initial, final, eos, ContactVelocity,
		0.02, 0.2, ContactLowDensity, ContactHighDensity);
	summary.passed = summary.passed && summary.advectionReferencePassed;
	return summary;
}

namespace
{
RusanovProbeSummary RunNearVacuumExpansionCase(
	FluxDissipationModel fluxModel,
	std::string_view caseId)
{
	const IdealGasEOS eos(Gamma, SpecificGasConstant);
	std::vector<ConservativeState> initial(NearVacuumCells);
	for (std::size_t cell = 0; cell < NearVacuumCells; ++cell)
	{
		const bool denseRegion = cell < NearVacuumCells / 2;
		initial[cell] = eos.FromPrimitive(
			denseRegion ? NearVacuumHighDensity : NearVacuumLowDensity,
			0.0,
			0.0,
			denseRegion ? NearVacuumHighPressure : NearVacuumLowPressure);
	}
	std::vector<ConservativeState> final;
	auto summary = RunPeriodicProbe(
		{caseId,
			{NearVacuumCells, 1, CellLength, BoundaryMode::Periodic},
			TimeDomain::NondimensionalContract, NearVacuumTimeStep, NearVacuumSteps},
		initial, eos, true, false, 1e-10, &final, fluxModel);
	if (final.size() != initial.size())
		return summary;
	for (std::size_t cell = NearVacuumCells / 2; cell < NearVacuumCells; ++cell)
	{
		summary.initialLowDensityRegionMass += initial[cell].density;
		summary.finalLowDensityRegionMass += final[cell].density;
	}
	summary.lowDensityRegionMassIncreased =
		summary.finalLowDensityRegionMass > summary.initialLowDensityRegionMass + 1e-12;
	summary.passed = summary.passed
		&& summary.minimumDensity > 0.0
		&& summary.minimumDensity <= 1e-4
		&& summary.minimumPressure > 0.0
		&& summary.lowDensityRegionMassIncreased;
	return summary;
}
} // namespace

RusanovProbeSummary RunRusanovNearVacuumExpansion()
{
	return RunNearVacuumExpansionCase(
		FluxDissipationModel::Rusanov, "rusanov_near_vacuum_expansion_1d");
}

RusanovProbeSummary RunHllcRusanovFallbackNearVacuumExpansion()
{
	return RunNearVacuumExpansionCase(
		FluxDissipationModel::HllcRusanovFallback,
		"hllc_rusanov_fallback_near_vacuum_expansion_1d");
}

namespace
{
RusanovProbeSummary RunSodShockTubeCase(
	FluxDissipationModel fluxModel,
	std::string_view caseId)
{
	const IdealGasEOS eos(SodGamma, SpecificGasConstant);
	std::vector<ConservativeState> initial(SodCells);
	for (std::size_t cell = 0; cell < SodCells; ++cell)
	{
		const bool leftState = (static_cast<double>(cell) + 0.5) * SodCellLength < 0.5;
		initial[cell] = eos.FromPrimitive(
			leftState ? SodLeftDensity : SodRightDensity,
			0.0,
			0.0,
			leftState ? SodLeftPressure : SodRightPressure);
	}
	std::vector<ConservativeState> final;
	auto summary = RunSealedProbe(
		{caseId,
			{SodCells, 1, SodCellLength, BoundaryMode::Sealed},
			TimeDomain::NondimensionalContract, SodTimeStep, SodSteps},
		initial, eos, true, 1e-9, &final, fluxModel);
	if (final.size() != initial.size())
		return summary;

	summary.simulatedTime = SodTimeStep * static_cast<double>(SodSteps);
	double maximumPressureGradient = 0.0;
	for (std::size_t face = SodCells / 2; face + 1 < SodCells; ++face)
	{
		const auto left = eos.ToPrimitive(final[face]);
		const auto right = eos.ToPrimitive(final[face + 1]);
		if (!left.valid || !right.valid)
			return summary;
		const double gradient = std::abs(right.pressure - left.pressure);
		if (gradient > maximumPressureGradient)
		{
			maximumPressureGradient = gradient;
			summary.shockPosition = static_cast<double>(face + 1) * SodCellLength;
		}
	}
	summary.densityBoundsPreserved = summary.minimumDensity >= SodRightDensity - 1e-10
		&& summary.maximumDensity <= SodLeftDensity + 1e-10
		&& summary.minimumPressure >= SodRightPressure - 1e-10
		&& summary.finalMaximumPressure <= SodLeftPressure + 1e-10;
	summary.shockReferencePassed = summary.densityBoundsPreserved
		&& summary.boundaryLedgerCloses
		&& summary.minimumVelocityX >= -1e-10
		&& summary.maximumVelocityX >= 0.5
		&& summary.maximumVelocityX <= 1.2
		&& summary.shockPosition >= 0.8
		&& summary.shockPosition <= 0.9;
	summary.passed = summary.passed && summary.shockReferencePassed;
	return summary;
}
} // namespace

RusanovProbeSummary RunRusanovSodShockTube()
{
	return RunSodShockTubeCase(
		FluxDissipationModel::Rusanov, "rusanov_sod_shock_tube_1d");
}

RusanovProbeSummary RunHllcRusanovFallbackSodShockTube()
{
	return RunSodShockTubeCase(
		FluxDissipationModel::HllcRusanovFallback,
		"hllc_rusanov_fallback_sod_shock_tube_1d");
}

namespace
{
RusanovProbeSummary RunOpenBoundaryLeakCase(
	FluxDissipationModel fluxModel,
	std::string_view caseId)
{
	const IdealGasEOS eos(Gamma, SpecificGasConstant);
	const auto initialState = eos.FromPrimitive(
		LeakInteriorDensity, 0.0, 0.0, LeakInteriorPressure);
	const auto exteriorState = eos.FromPrimitive(
		LeakExteriorDensity, 0.0, 0.0, LeakExteriorPressure);
	std::vector<ConservativeState> final;
	auto summary = RunOpenProbe(
		{caseId,
			{LeakCells, 1, LeakCellLength, BoundaryMode::Open},
			TimeDomain::NondimensionalContract, LeakTimeStep, LeakSteps},
		std::vector<ConservativeState>(LeakCells, initialState),
		exteriorState, eos, true, 1e-9, &final, fluxModel);
	if (final.size() != LeakCells)
		return summary;
	summary.simulatedTime = LeakTimeStep * static_cast<double>(LeakSteps);
	const double massChange = summary.ledger.final.density - summary.ledger.initial.density;
	const double rightMassExchange = summary.rightBoundaryExchange.density;
	summary.passed = summary.passed
		&& massChange < -1e-6
		&& rightMassExchange < -1e-6
		&& std::abs(summary.leftBoundaryExchange.density) <= 1e-12
		&& std::abs(massChange - summary.boundaryExchange.density) <= 1e-9;
	return summary;
}
} // namespace

RusanovProbeSummary RunRusanovOpenBoundaryLeak()
{
	return RunOpenBoundaryLeakCase(
		FluxDissipationModel::Rusanov, "rusanov_open_boundary_leak_1d");
}

RusanovProbeSummary RunHllcRusanovFallbackOpenBoundaryLeak()
{
	return RunOpenBoundaryLeakCase(
		FluxDissipationModel::HllcRusanovFallback,
		"hllc_rusanov_fallback_open_boundary_leak_1d");
}

RusanovRefinementSummary RunRusanovDensityAdvectionRefinement()
{
	RusanovRefinementSummary summary{
		RunDensityAdvectionRefinementCase(RefinementCoarseCells),
		RunDensityAdvectionRefinementCase(RefinementMediumCells),
		RunDensityAdvectionRefinementCase(RefinementFineCells),
	};
	if (summary.coarse.densityL1Error > 0.0 && summary.medium.densityL1Error > 0.0
		&& summary.fine.densityL1Error > 0.0)
	{
		summary.coarseToMediumL1Order = std::log2(
			summary.coarse.densityL1Error / summary.medium.densityL1Error);
		summary.mediumToFineL1Order = std::log2(
			summary.medium.densityL1Error / summary.fine.densityL1Error);
	}
	summary.passed = summary.coarse.passed && summary.medium.passed && summary.fine.passed
		&& summary.coarse.densityL1Error > summary.medium.densityL1Error
		&& summary.medium.densityL1Error > summary.fine.densityL1Error
		&& summary.coarseToMediumL1Order >= 0.8
		&& summary.mediumToFineL1Order >= 0.8
		&& summary.coarseToMediumL1Order <= 1.2
		&& summary.mediumToFineL1Order <= 1.2;
	return summary;
}

RusanovLowMachSummary RunRusanovLowMachAdvection()
{
	RusanovLowMachSummary summary{
		RunLowMachAdvectionCase(ModerateMachVelocity),
		RunLowMachAdvectionCase(LowMachVelocity),
		RunLowMachAdvectionCase(VeryLowMachVelocity),
	};
	const double nominalSoundSpeed = std::sqrt(Gamma);
	summary.moderateNominalMach = ModerateMachVelocity / nominalSoundSpeed;
	summary.lowNominalMach = LowMachVelocity / nominalSoundSpeed;
	summary.veryLowNominalMach = VeryLowMachVelocity / nominalSoundSpeed;
	if (summary.moderateMach.densityL1Error > 0.0)
	{
		summary.lowToModerateL1Ratio =
			summary.lowMach.densityL1Error / summary.moderateMach.densityL1Error;
		summary.veryLowToModerateL1Ratio =
			summary.veryLowMach.densityL1Error / summary.moderateMach.densityL1Error;
	}
	summary.suitabilityPassed = summary.veryLowMach.densityL1Error <= 0.05
		&& summary.veryLowMach.totalVariationRatio >= 0.8;
	summary.passed = summary.moderateMach.passed && summary.lowMach.passed
		&& summary.veryLowMach.passed
		&& std::isfinite(summary.lowToModerateL1Ratio)
		&& std::isfinite(summary.veryLowToModerateL1Ratio);
	return summary;
}

RusanovLowMachSummary RunAllSpeedRusanovLowMachAdvection()
{
	RusanovLowMachSummary summary{
		RunLowMachAdvectionCase(ModerateMachVelocity, FluxDissipationModel::AllSpeedRusanov),
		RunLowMachAdvectionCase(LowMachVelocity, FluxDissipationModel::AllSpeedRusanov),
		RunLowMachAdvectionCase(VeryLowMachVelocity, FluxDissipationModel::AllSpeedRusanov),
	};
	const double nominalSoundSpeed = std::sqrt(Gamma);
	summary.moderateNominalMach = ModerateMachVelocity / nominalSoundSpeed;
	summary.lowNominalMach = LowMachVelocity / nominalSoundSpeed;
	summary.veryLowNominalMach = VeryLowMachVelocity / nominalSoundSpeed;
	if (summary.moderateMach.densityL1Error > 0.0)
	{
		summary.lowToModerateL1Ratio =
			summary.lowMach.densityL1Error / summary.moderateMach.densityL1Error;
		summary.veryLowToModerateL1Ratio =
			summary.veryLowMach.densityL1Error / summary.moderateMach.densityL1Error;
	}
	summary.suitabilityPassed = summary.veryLowMach.densityL1Error <= 0.05
		&& summary.veryLowMach.totalVariationRatio >= 0.8;
	const auto executionPassed = [](const RusanovProbeSummary &probe)
	{
		return probe.positivityPreserved && probe.boundaryLedgerCloses
			&& probe.corrections.IsEmpty() && probe.maximumCfl <= 1.0
			&& probe.stateEvolved && std::isfinite(probe.densityL1Error)
			&& std::isfinite(probe.densityLinfError)
			&& std::isfinite(probe.pressureLinfError)
			&& std::isfinite(probe.totalVariationRatio);
	};
	summary.passed = executionPassed(summary.moderateMach)
		&& executionPassed(summary.lowMach)
		&& executionPassed(summary.veryLowMach)
		&& std::isfinite(summary.lowToModerateL1Ratio)
		&& std::isfinite(summary.veryLowToModerateL1Ratio);
	return summary;
}

RusanovLowMachSummary RunHllcRusanovFallbackLowMachAdvection()
{
	RusanovLowMachSummary summary{
		RunLowMachAdvectionCase(ModerateMachVelocity, FluxDissipationModel::HllcRusanovFallback),
		RunLowMachAdvectionCase(LowMachVelocity, FluxDissipationModel::HllcRusanovFallback),
		RunLowMachAdvectionCase(VeryLowMachVelocity, FluxDissipationModel::HllcRusanovFallback),
	};
	const double nominalSoundSpeed = std::sqrt(Gamma);
	summary.moderateNominalMach = ModerateMachVelocity / nominalSoundSpeed;
	summary.lowNominalMach = LowMachVelocity / nominalSoundSpeed;
	summary.veryLowNominalMach = VeryLowMachVelocity / nominalSoundSpeed;
	if (summary.moderateMach.densityL1Error > 0.0)
	{
		summary.lowToModerateL1Ratio =
			summary.lowMach.densityL1Error / summary.moderateMach.densityL1Error;
		summary.veryLowToModerateL1Ratio =
			summary.veryLowMach.densityL1Error / summary.moderateMach.densityL1Error;
	}
	summary.suitabilityPassed = summary.veryLowMach.densityL1Error <= 0.05
		&& summary.veryLowMach.totalVariationRatio >= 0.8;
	summary.passed = summary.moderateMach.passed && summary.lowMach.passed
		&& summary.veryLowMach.passed
		&& std::isfinite(summary.lowToModerateL1Ratio)
		&& std::isfinite(summary.veryLowToModerateL1Ratio);
	return summary;
}

RusanovPerformanceSummary RunRusanovPerformance()
{
	RusanovPerformanceSummary summary{
		MeasurePerformanceCase(PerformanceSmallCells, FluxDissipationModel::Rusanov, "rusanov_performance_1d"),
		MeasurePerformanceCase(PerformanceMediumCells, FluxDissipationModel::Rusanov, "rusanov_performance_1d"),
		MeasurePerformanceCase(PerformanceLargeCells, FluxDissipationModel::Rusanov, "rusanov_performance_1d"),
		PerformanceWarmups,
		PerformanceRepeats,
	};
	summary.passed = summary.small.passed && summary.medium.passed && summary.large.passed;
	return summary;
}

RusanovPerformanceSummary RunHllcRusanovFallbackPerformance()
{
	RusanovPerformanceSummary summary{
		MeasurePerformanceCase(PerformanceSmallCells, FluxDissipationModel::HllcRusanovFallback,
			"hllc_rusanov_fallback_performance_1d"),
		MeasurePerformanceCase(PerformanceMediumCells, FluxDissipationModel::HllcRusanovFallback,
			"hllc_rusanov_fallback_performance_1d"),
		MeasurePerformanceCase(PerformanceLargeCells, FluxDissipationModel::HllcRusanovFallback,
			"hllc_rusanov_fallback_performance_1d"),
		PerformanceWarmups,
		PerformanceRepeats,
	};
	summary.passed = summary.small.passed && summary.medium.passed && summary.large.passed;
	return summary;
}

bool RunHllcRusanovFallbackContract()
{
	const IdealGasEOS eos(Gamma, SpecificGasConstant);
	const auto left = eos.FromPrimitive(1e-7, 400.0, 0.0, 0.2);
	const auto right = eos.FromPrimitive(100.0, -700.0, 0.0, 1e-12);
	std::size_t fallbackCount = 0;
	const auto flux = NumericalFluxX(
		left, right, eos, FluxDissipationModel::HllcRusanovFallback, &fallbackCount);
	const auto referenceFlux = RusanovFluxX(left, right, eos);
	return fallbackCount == 1 && StateNear(flux, referenceFlux, 1e-12);
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
	output << "momentum_drift=" << std::hypot(
		final.momentumX - initial.momentumX,
		final.momentumY - initial.momentumY
	) << '\n';
	output << "momentum_x_drift=" << (final.momentumX - initial.momentumX) << '\n';
	output << "momentum_y_drift=" << (final.momentumY - initial.momentumY) << '\n';
	output << "energy_drift=" << (final.totalEnergyDensity - initial.totalEnergyDensity) << '\n';
	output << "minimum_density=" << summary.minimumDensity << '\n';
	output << "minimum_pressure=" << summary.minimumPressure << '\n';
	output << "minimum_energy_density=" << summary.minimumEnergyDensity << '\n';
	output << "positivity_preserved=" << (summary.positivityPreserved ? "true" : "false") << '\n';
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
	output << "state_and_flux_scratch_bytes_per_cell=" << (3 * sizeof(ConservativeState)) << '\n';
	output << "probe_passed=" << (summary.passed ? "true" : "false") << '\n';
	return summary.passed;
}

bool WriteRusanovPressurePulseProbe(std::ostream &output)
{
	const auto summary = RunRusanovPressurePulse();
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
	output << "momentum_drift=" << std::hypot(
		final.momentumX - initial.momentumX,
		final.momentumY - initial.momentumY
	) << '\n';
	output << "momentum_x_drift=" << (final.momentumX - initial.momentumX) << '\n';
	output << "momentum_y_drift=" << (final.momentumY - initial.momentumY) << '\n';
	output << "energy_drift=" << (final.totalEnergyDensity - initial.totalEnergyDensity) << '\n';
	output << "minimum_density=" << summary.minimumDensity << '\n';
	output << "minimum_pressure=" << summary.minimumPressure << '\n';
	output << "minimum_energy_density=" << summary.minimumEnergyDensity << '\n';
	output << "initial_maximum_pressure=" << summary.initialMaximumPressure << '\n';
	output << "final_maximum_pressure=" << summary.finalMaximumPressure << '\n';
	output << "pressure_peak_reduced=" << (summary.pressurePeakReduced ? "true" : "false") << '\n';
	output << "state_change_l1=" << summary.stateChangeL1 << '\n';
	output << "state_evolved=" << (summary.stateEvolved ? "true" : "false") << '\n';
	output << "positivity_preserved=" << (summary.positivityPreserved ? "true" : "false") << '\n';
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
	output << "state_and_flux_scratch_bytes_per_cell=" << (3 * sizeof(ConservativeState)) << '\n';
	output << "probe_passed=" << (summary.passed ? "true" : "false") << '\n';
	return summary.passed;
}

bool WriteRusanovDensityAdvectionProbe(std::ostream &output)
{
	const auto summary = RunRusanovDensityAdvection();
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
	output << "reference_velocity=" << summary.referenceVelocity << '\n';
	output << "reference_shift_cells=" << summary.referenceShiftCells << '\n';
	output << "density_l1_error=" << summary.densityL1Error << '\n';
	output << "density_linf_error=" << summary.densityLinfError << '\n';
	output << "pressure_linf_error=" << summary.pressureLinfError << '\n';
	output << "total_variation_ratio=" << summary.totalVariationRatio << '\n';
	output << "advection_reference_passed="
		<< (summary.advectionReferencePassed ? "true" : "false") << '\n';
	output << "initial_mass=" << initial.density << '\n';
	output << "final_mass=" << final.density << '\n';
	output << "mass_drift=" << (final.density - initial.density) << '\n';
	output << "momentum_drift=" << std::hypot(
		final.momentumX - initial.momentumX,
		final.momentumY - initial.momentumY
	) << '\n';
	output << "momentum_x_drift=" << (final.momentumX - initial.momentumX) << '\n';
	output << "momentum_y_drift=" << (final.momentumY - initial.momentumY) << '\n';
	output << "energy_drift=" << (final.totalEnergyDensity - initial.totalEnergyDensity) << '\n';
	output << "minimum_density=" << summary.minimumDensity << '\n';
	output << "minimum_pressure=" << summary.minimumPressure << '\n';
	output << "minimum_energy_density=" << summary.minimumEnergyDensity << '\n';
	output << "state_change_l1=" << summary.stateChangeL1 << '\n';
	output << "state_evolved=" << (summary.stateEvolved ? "true" : "false") << '\n';
	output << "positivity_preserved=" << (summary.positivityPreserved ? "true" : "false") << '\n';
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
	output << "state_and_flux_scratch_bytes_per_cell=" << (3 * sizeof(ConservativeState)) << '\n';
	output << "probe_passed=" << (summary.passed ? "true" : "false") << '\n';
	return summary.passed;
}

bool WriteRusanovContactDiscontinuityProbe(std::ostream &output)
{
	const auto summary = RunRusanovContactDiscontinuity();
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
	output << "reference_velocity=" << summary.referenceVelocity << '\n';
	output << "reference_shift_cells=" << summary.referenceShiftCells << '\n';
	output << "density_l1_error=" << summary.densityL1Error << '\n';
	output << "density_linf_error=" << summary.densityLinfError << '\n';
	output << "pressure_linf_error=" << summary.pressureLinfError << '\n';
	output << "total_variation_ratio=" << summary.totalVariationRatio << '\n';
	output << "density_bounds_preserved="
		<< (summary.densityBoundsPreserved ? "true" : "false") << '\n';
	output << "advection_reference_passed="
		<< (summary.advectionReferencePassed ? "true" : "false") << '\n';
	output << "initial_mass=" << initial.density << '\n';
	output << "final_mass=" << final.density << '\n';
	output << "mass_drift=" << (final.density - initial.density) << '\n';
	output << "momentum_drift=" << std::hypot(
		final.momentumX - initial.momentumX,
		final.momentumY - initial.momentumY
	) << '\n';
	output << "momentum_x_drift=" << (final.momentumX - initial.momentumX) << '\n';
	output << "momentum_y_drift=" << (final.momentumY - initial.momentumY) << '\n';
	output << "energy_drift=" << (final.totalEnergyDensity - initial.totalEnergyDensity) << '\n';
	output << "minimum_density=" << summary.minimumDensity << '\n';
	output << "maximum_density=" << summary.maximumDensity << '\n';
	output << "minimum_pressure=" << summary.minimumPressure << '\n';
	output << "minimum_energy_density=" << summary.minimumEnergyDensity << '\n';
	output << "state_change_l1=" << summary.stateChangeL1 << '\n';
	output << "state_evolved=" << (summary.stateEvolved ? "true" : "false") << '\n';
	output << "positivity_preserved=" << (summary.positivityPreserved ? "true" : "false") << '\n';
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
	output << "state_and_flux_scratch_bytes_per_cell=" << (3 * sizeof(ConservativeState)) << '\n';
	output << "probe_passed=" << (summary.passed ? "true" : "false") << '\n';
	return summary.passed;
}

namespace
{
bool WriteNearVacuumExpansionProbe(
	std::ostream &output,
	const RusanovProbeSummary &summary,
	const char *candidateId)
{
	const auto &initial = summary.ledger.initial;
	const auto &final = summary.ledger.final;
	output << "schema_version=1\n";
	output << "case=" << summary.benchmarkCase.id << '\n';
	output << "candidate=" << candidateId << '\n';
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
	output << "initial_maximum_pressure=" << summary.initialMaximumPressure << '\n';
	output << "final_maximum_pressure=" << summary.finalMaximumPressure << '\n';
	output << "state_change_l1=" << summary.stateChangeL1 << '\n';
	output << "state_evolved=" << (summary.stateEvolved ? "true" : "false") << '\n';
	output << "initial_low_density_region_mass=" << summary.initialLowDensityRegionMass << '\n';
	output << "final_low_density_region_mass=" << summary.finalLowDensityRegionMass << '\n';
	output << "low_density_region_mass_increased="
		<< (summary.lowDensityRegionMassIncreased ? "true" : "false") << '\n';
	output << "initial_mass=" << initial.density << '\n';
	output << "final_mass=" << final.density << '\n';
	output << "mass_drift=" << (final.density - initial.density) << '\n';
	output << "momentum_drift=" << std::hypot(
		final.momentumX - initial.momentumX,
		final.momentumY - initial.momentumY
	) << '\n';
	output << "momentum_x_drift=" << (final.momentumX - initial.momentumX) << '\n';
	output << "momentum_y_drift=" << (final.momentumY - initial.momentumY) << '\n';
	output << "energy_drift=" << (final.totalEnergyDensity - initial.totalEnergyDensity) << '\n';
	output << "minimum_density=" << summary.minimumDensity << '\n';
	output << "maximum_density=" << summary.maximumDensity << '\n';
	output << "minimum_pressure=" << summary.minimumPressure << '\n';
	output << "minimum_energy_density=" << summary.minimumEnergyDensity << '\n';
	output << "positivity_preserved=" << (summary.positivityPreserved ? "true" : "false") << '\n';
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
	output << "flux_fallback_count=" << summary.fluxFallbackCount << '\n';
	output << "state_bytes_per_cell=" << sizeof(ConservativeState) << '\n';
	output << "state_and_flux_scratch_bytes_per_cell=" << (3 * sizeof(ConservativeState)) << '\n';
	output << "probe_passed=" << (summary.passed ? "true" : "false") << '\n';
	return summary.passed;
}
} // namespace

bool WriteRusanovNearVacuumExpansionProbe(std::ostream &output)
{
	return WriteNearVacuumExpansionProbe(
		output, RunRusanovNearVacuumExpansion(), "fvm_rusanov");
}

bool WriteHllcRusanovFallbackNearVacuumExpansionProbe(std::ostream &output)
{
	return WriteNearVacuumExpansionProbe(
		output, RunHllcRusanovFallbackNearVacuumExpansion(), "fvm_hllc_rusanov_fallback");
}

namespace
{
bool WriteSodShockTubeProbe(
	std::ostream &output,
	const RusanovProbeSummary &summary,
	const char *candidateId)
{
	const auto &initial = summary.ledger.initial;
	const auto &final = summary.ledger.final;
	const auto expectedFinal = Add(initial, summary.boundaryExchange);
	const auto stateAndFluxScratchBytesTotal =
		(3 * summary.benchmarkCase.grid.CellCount() + 1) * sizeof(ConservativeState);
	output << "schema_version=1\n";
	output << "case=" << summary.benchmarkCase.id << '\n';
	output << "candidate=" << candidateId << '\n';
	output << "candidate_solver_implemented=true\n";
	output << "atmosphere_solver_selection=unselected\n";
	output << "physical_scale_selection=unselected\n";
	output << "result_status=candidate_result_not_selection\n";
	output << "case_time_domain=nondimensional_contract\n";
	output << "boundary_mode=sealed\n";
	output << "eos_gamma=" << SodGamma << '\n';
	output << "eos_specific_gas_constant=" << SpecificGasConstant << '\n';
	output << "initial_left_density=" << SodLeftDensity << '\n';
	output << "initial_left_pressure=" << SodLeftPressure << '\n';
	output << "initial_right_density=" << SodRightDensity << '\n';
	output << "initial_right_pressure=" << SodRightPressure << '\n';
	output << "grid_cells_x=" << summary.benchmarkCase.grid.cellsX << '\n';
	output << "grid_cells_y=" << summary.benchmarkCase.grid.cellsY << '\n';
	output << "grid_cell_count=" << summary.benchmarkCase.grid.CellCount() << '\n';
	output << "cell_length=" << summary.benchmarkCase.grid.cellLength << '\n';
	output << "case_timestep=" << summary.benchmarkCase.timeStep << '\n';
	output << "case_step_count=" << summary.benchmarkCase.stepCount << '\n';
	output << "simulated_time=" << summary.simulatedTime << '\n';
	output << "maximum_cfl=" << summary.maximumCfl << '\n';
	output << "shock_position=" << summary.shockPosition << '\n';
	output << "minimum_velocity_x=" << summary.minimumVelocityX << '\n';
	output << "maximum_velocity_x=" << summary.maximumVelocityX << '\n';
	output << "density_bounds_preserved="
		<< (summary.densityBoundsPreserved ? "true" : "false") << '\n';
	output << "shock_reference_passed="
		<< (summary.shockReferencePassed ? "true" : "false") << '\n';
	output << "initial_mass=" << initial.density << '\n';
	output << "final_mass=" << final.density << '\n';
	output << "mass_drift=" << (final.density - initial.density) << '\n';
	output << "momentum_drift=" << std::hypot(
		final.momentumX - initial.momentumX,
		final.momentumY - initial.momentumY
	) << '\n';
	output << "momentum_x_drift=" << (final.momentumX - initial.momentumX) << '\n';
	output << "momentum_y_drift=" << (final.momentumY - initial.momentumY) << '\n';
	output << "energy_drift=" << (final.totalEnergyDensity - initial.totalEnergyDensity) << '\n';
	output << "boundary_mass_exchange=" << summary.boundaryExchange.density << '\n';
	output << "boundary_momentum_x_exchange=" << summary.boundaryExchange.momentumX << '\n';
	output << "boundary_momentum_y_exchange=" << summary.boundaryExchange.momentumY << '\n';
	output << "boundary_energy_exchange=" << summary.boundaryExchange.totalEnergyDensity << '\n';
	output << "mass_balance_error=" << (final.density - expectedFinal.density) << '\n';
	output << "momentum_x_balance_error=" << (final.momentumX - expectedFinal.momentumX) << '\n';
	output << "momentum_y_balance_error=" << (final.momentumY - expectedFinal.momentumY) << '\n';
	output << "energy_balance_error="
		<< (final.totalEnergyDensity - expectedFinal.totalEnergyDensity) << '\n';
	output << "boundary_ledger_closes=" << (summary.boundaryLedgerCloses ? "true" : "false") << '\n';
	output << "minimum_density=" << summary.minimumDensity << '\n';
	output << "maximum_density=" << summary.maximumDensity << '\n';
	output << "minimum_pressure=" << summary.minimumPressure << '\n';
	output << "maximum_pressure=" << summary.finalMaximumPressure << '\n';
	output << "minimum_energy_density=" << summary.minimumEnergyDensity << '\n';
	output << "state_change_l1=" << summary.stateChangeL1 << '\n';
	output << "state_evolved=" << (summary.stateEvolved ? "true" : "false") << '\n';
	output << "positivity_preserved=" << (summary.positivityPreserved ? "true" : "false") << '\n';
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
	output << "flux_fallback_count=" << summary.fluxFallbackCount << '\n';
	output << "state_bytes_per_cell=" << sizeof(ConservativeState) << '\n';
	output << "state_and_flux_scratch_bytes_total=" << stateAndFluxScratchBytesTotal << '\n';
	output << "state_and_flux_scratch_bytes_per_cell="
		<< static_cast<double>(stateAndFluxScratchBytesTotal)
			/ static_cast<double>(summary.benchmarkCase.grid.CellCount()) << '\n';
	output << "probe_passed=" << (summary.passed ? "true" : "false") << '\n';
	return summary.passed;
}
} // namespace

bool WriteRusanovSodShockTubeProbe(std::ostream &output)
{
	return WriteSodShockTubeProbe(output, RunRusanovSodShockTube(), "fvm_rusanov");
}

bool WriteHllcRusanovFallbackSodShockTubeProbe(std::ostream &output)
{
	return WriteSodShockTubeProbe(
		output, RunHllcRusanovFallbackSodShockTube(), "fvm_hllc_rusanov_fallback");
}

namespace
{
bool WriteOpenBoundaryLeakProbe(
	std::ostream &output,
	const RusanovProbeSummary &summary,
	const char *candidateId)
{
	const auto &initial = summary.ledger.initial;
	const auto &final = summary.ledger.final;
	const auto expectedFinal = Add(initial, summary.boundaryExchange);
	const auto stateAndFluxScratchBytesTotal =
		(3 * summary.benchmarkCase.grid.CellCount() + 1) * sizeof(ConservativeState);
	output << "schema_version=1\n";
	output << "case=" << summary.benchmarkCase.id << '\n';
	output << "candidate=" << candidateId << '\n';
	output << "candidate_solver_implemented=true\n";
	output << "atmosphere_solver_selection=unselected\n";
	output << "physical_scale_selection=unselected\n";
	output << "result_status=candidate_result_not_selection\n";
	output << "case_time_domain=nondimensional_contract\n";
	output << "boundary_mode=sealed_left_open_right_reservoir\n";
	output << "eos_gamma=" << Gamma << '\n';
	output << "eos_specific_gas_constant=" << SpecificGasConstant << '\n';
	output << "initial_left_density=" << LeakInteriorDensity << '\n';
	output << "initial_left_pressure=" << LeakInteriorPressure << '\n';
	output << "initial_right_density=" << LeakExteriorDensity << '\n';
	output << "initial_right_pressure=" << LeakExteriorPressure << '\n';
	output << "grid_cells_x=" << summary.benchmarkCase.grid.cellsX << '\n';
	output << "grid_cells_y=" << summary.benchmarkCase.grid.cellsY << '\n';
	output << "grid_cell_count=" << summary.benchmarkCase.grid.CellCount() << '\n';
	output << "cell_length=" << summary.benchmarkCase.grid.cellLength << '\n';
	output << "case_timestep=" << summary.benchmarkCase.timeStep << '\n';
	output << "case_step_count=" << summary.benchmarkCase.stepCount << '\n';
	output << "simulated_time=" << summary.simulatedTime << '\n';
	output << "maximum_cfl=" << summary.maximumCfl << '\n';
	output << "initial_mass=" << initial.density << '\n';
	output << "final_mass=" << final.density << '\n';
	output << "mass_drift=" << (final.density - initial.density) << '\n';
	output << "momentum_drift=" << std::hypot(
		final.momentumX - initial.momentumX,
		final.momentumY - initial.momentumY) << '\n';
	output << "momentum_x_drift=" << (final.momentumX - initial.momentumX) << '\n';
	output << "momentum_y_drift=" << (final.momentumY - initial.momentumY) << '\n';
	output << "energy_drift=" << (final.totalEnergyDensity - initial.totalEnergyDensity) << '\n';
	output << "left_boundary_mass_exchange=" << summary.leftBoundaryExchange.density << '\n';
	output << "left_boundary_momentum_x_exchange=" << summary.leftBoundaryExchange.momentumX << '\n';
	output << "left_boundary_momentum_y_exchange=" << summary.leftBoundaryExchange.momentumY << '\n';
	output << "left_boundary_energy_exchange=" << summary.leftBoundaryExchange.totalEnergyDensity << '\n';
	output << "right_boundary_mass_exchange=" << summary.rightBoundaryExchange.density << '\n';
	output << "right_boundary_momentum_x_exchange=" << summary.rightBoundaryExchange.momentumX << '\n';
	output << "right_boundary_momentum_y_exchange=" << summary.rightBoundaryExchange.momentumY << '\n';
	output << "right_boundary_energy_exchange=" << summary.rightBoundaryExchange.totalEnergyDensity << '\n';
	output << "boundary_mass_exchange=" << summary.boundaryExchange.density << '\n';
	output << "boundary_momentum_x_exchange=" << summary.boundaryExchange.momentumX << '\n';
	output << "boundary_momentum_y_exchange=" << summary.boundaryExchange.momentumY << '\n';
	output << "boundary_energy_exchange=" << summary.boundaryExchange.totalEnergyDensity << '\n';
	output << "mass_balance_error=" << (final.density - expectedFinal.density) << '\n';
	output << "momentum_x_balance_error=" << (final.momentumX - expectedFinal.momentumX) << '\n';
	output << "momentum_y_balance_error=" << (final.momentumY - expectedFinal.momentumY) << '\n';
	output << "energy_balance_error="
		<< (final.totalEnergyDensity - expectedFinal.totalEnergyDensity) << '\n';
	output << "boundary_ledger_closes=" << (summary.boundaryLedgerCloses ? "true" : "false") << '\n';
	output << "mass_decreased=" << (final.density < initial.density ? "true" : "false") << '\n';
	output << "right_boundary_mass_out=" << -summary.rightBoundaryExchange.density << '\n';
	output << "minimum_density=" << summary.minimumDensity << '\n';
	output << "maximum_density=" << summary.maximumDensity << '\n';
	output << "minimum_pressure=" << summary.minimumPressure << '\n';
	output << "maximum_pressure=" << summary.finalMaximumPressure << '\n';
	output << "minimum_energy_density=" << summary.minimumEnergyDensity << '\n';
	output << "state_change_l1=" << summary.stateChangeL1 << '\n';
	output << "state_evolved=" << (summary.stateEvolved ? "true" : "false") << '\n';
	output << "positivity_preserved=" << (summary.positivityPreserved ? "true" : "false") << '\n';
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
	output << "flux_fallback_count=" << summary.fluxFallbackCount << '\n';
	output << "state_bytes_per_cell=" << sizeof(ConservativeState) << '\n';
	output << "state_and_flux_scratch_bytes_total=" << stateAndFluxScratchBytesTotal << '\n';
	output << "state_and_flux_scratch_bytes_per_cell="
		<< static_cast<double>(stateAndFluxScratchBytesTotal)
			/ static_cast<double>(summary.benchmarkCase.grid.CellCount()) << '\n';
	output << "probe_passed=" << (summary.passed ? "true" : "false") << '\n';
	return summary.passed;
}
} // namespace

bool WriteRusanovOpenBoundaryLeakProbe(std::ostream &output)
{
	return WriteOpenBoundaryLeakProbe(
		output, RunRusanovOpenBoundaryLeak(), "fvm_rusanov");
}

bool WriteHllcRusanovFallbackOpenBoundaryLeakProbe(std::ostream &output)
{
	return WriteOpenBoundaryLeakProbe(
		output, RunHllcRusanovFallbackOpenBoundaryLeak(), "fvm_hllc_rusanov_fallback");
}

bool WriteRusanovDensityAdvectionRefinementProbe(std::ostream &output)
{
	const auto summary = RunRusanovDensityAdvectionRefinement();
	output << "schema_version=1\n";
	output << "case=rusanov_density_advection_refinement_1d\n";
	output << "candidate=fvm_rusanov\n";
	output << "candidate_solver_implemented=true\n";
	output << "atmosphere_solver_selection=unselected\n";
	output << "physical_scale_selection=unselected\n";
	output << "result_status=candidate_result_not_selection\n";
	output << "case_time_domain=nondimensional_contract\n";
	output << "boundary_mode=periodic\n";
	output << "grid_cells_x=" << summary.fine.benchmarkCase.grid.cellsX << '\n';
	output << "grid_cells_y=" << summary.fine.benchmarkCase.grid.cellsY << '\n';
	output << "grid_cell_count=" << summary.fine.benchmarkCase.grid.CellCount() << '\n';
	output << "case_timestep=" << summary.fine.benchmarkCase.timeStep << '\n';
	output << "case_step_count=" << summary.fine.benchmarkCase.stepCount << '\n';
	output << "refinement_levels=3\n";
	output << "total_simulated_time=" << RefinementTotalTime << '\n';
	output << "reference_velocity=" << DensityAdvectionVelocity << '\n';
	output << "coarse_cells=" << summary.coarse.benchmarkCase.grid.cellsX << '\n';
	output << "medium_cells=" << summary.medium.benchmarkCase.grid.cellsX << '\n';
	output << "fine_cells=" << summary.fine.benchmarkCase.grid.cellsX << '\n';
	output << "coarse_steps=" << summary.coarse.benchmarkCase.stepCount << '\n';
	output << "medium_steps=" << summary.medium.benchmarkCase.stepCount << '\n';
	output << "fine_steps=" << summary.fine.benchmarkCase.stepCount << '\n';
	output << "coarse_reference_shift_cells=" << summary.coarse.referenceShiftCells << '\n';
	output << "medium_reference_shift_cells=" << summary.medium.referenceShiftCells << '\n';
	output << "fine_reference_shift_cells=" << summary.fine.referenceShiftCells << '\n';
	output << "coarse_density_l1_error=" << summary.coarse.densityL1Error << '\n';
	output << "medium_density_l1_error=" << summary.medium.densityL1Error << '\n';
	output << "fine_density_l1_error=" << summary.fine.densityL1Error << '\n';
	output << "coarse_density_linf_error=" << summary.coarse.densityLinfError << '\n';
	output << "medium_density_linf_error=" << summary.medium.densityLinfError << '\n';
	output << "fine_density_linf_error=" << summary.fine.densityLinfError << '\n';
	output << "coarse_pressure_linf_error=" << summary.coarse.pressureLinfError << '\n';
	output << "medium_pressure_linf_error=" << summary.medium.pressureLinfError << '\n';
	output << "fine_pressure_linf_error=" << summary.fine.pressureLinfError << '\n';
	output << "minimum_density=" << summary.fine.minimumDensity << '\n';
	output << "maximum_density=" << summary.fine.maximumDensity << '\n';
	output << "minimum_pressure=" << summary.fine.minimumPressure << '\n';
	output << "maximum_pressure=" << summary.fine.finalMaximumPressure << '\n';
	output << "state_change_l1=" << summary.fine.stateChangeL1 << '\n';
	output << "state_evolved=" << (summary.fine.stateEvolved ? "true" : "false") << '\n';
	output << "positivity_preserved=" << (summary.fine.positivityPreserved ? "true" : "false") << '\n';
	output << "coarse_to_medium_l1_order=" << summary.coarseToMediumL1Order << '\n';
	output << "medium_to_fine_l1_order=" << summary.mediumToFineL1Order << '\n';
	output << "coarse_maximum_cfl=" << summary.coarse.maximumCfl << '\n';
	output << "medium_maximum_cfl=" << summary.medium.maximumCfl << '\n';
	output << "fine_maximum_cfl=" << summary.fine.maximumCfl << '\n';
	output << "coarse_mass_drift="
		<< (summary.coarse.ledger.final.density - summary.coarse.ledger.initial.density) << '\n';
	output << "medium_mass_drift="
		<< (summary.medium.ledger.final.density - summary.medium.ledger.initial.density) << '\n';
	output << "fine_mass_drift="
		<< (summary.fine.ledger.final.density - summary.fine.ledger.initial.density) << '\n';
	output << "coarse_energy_drift="
		<< (summary.coarse.ledger.final.totalEnergyDensity - summary.coarse.ledger.initial.totalEnergyDensity) << '\n';
	output << "medium_energy_drift="
		<< (summary.medium.ledger.final.totalEnergyDensity - summary.medium.ledger.initial.totalEnergyDensity) << '\n';
	output << "fine_energy_drift="
		<< (summary.fine.ledger.final.totalEnergyDensity - summary.fine.ledger.initial.totalEnergyDensity) << '\n';
	output << "mass_drift="
		<< (summary.fine.ledger.final.density - summary.fine.ledger.initial.density) << '\n';
	output << "momentum_drift=" << std::hypot(
		summary.fine.ledger.final.momentumX - summary.fine.ledger.initial.momentumX,
		summary.fine.ledger.final.momentumY - summary.fine.ledger.initial.momentumY) << '\n';
	output << "momentum_x_drift="
		<< (summary.fine.ledger.final.momentumX - summary.fine.ledger.initial.momentumX) << '\n';
	output << "momentum_y_drift="
		<< (summary.fine.ledger.final.momentumY - summary.fine.ledger.initial.momentumY) << '\n';
	output << "energy_drift="
		<< (summary.fine.ledger.final.totalEnergyDensity - summary.fine.ledger.initial.totalEnergyDensity) << '\n';
	output << "numerical_correction_count="
		<< (summary.coarse.corrections.eventCount + summary.medium.corrections.eventCount
			+ summary.fine.corrections.eventCount) << '\n';
	output << "correction_mass_added=0\n";
	output << "correction_mass_removed=0\n";
	output << "correction_momentum_x_added=0\n";
	output << "correction_momentum_y_added=0\n";
	output << "correction_energy_added=0\n";
	output << "correction_energy_removed=0\n";
	output << "density_floor_hits=0\n";
	output << "pressure_floor_hits=0\n";
	output << "correction_event_count=0\n";
	output << "state_bytes_per_cell=" << sizeof(ConservativeState) << '\n';
	output << "state_and_flux_scratch_bytes_per_cell=" << (3 * sizeof(ConservativeState)) << '\n';
	output << "refinement_passed=" << (summary.passed ? "true" : "false") << '\n';
	output << "probe_passed=" << (summary.passed ? "true" : "false") << '\n';
	return summary.passed;
}

namespace
{
bool WriteLowMachAdvectionProbe(
	std::ostream &output,
	const RusanovLowMachSummary &summary,
	const char *caseId,
	const char *candidateId)
{
	output << "schema_version=1\n";
	output << "case=" << caseId << '\n';
	output << "candidate=" << candidateId << '\n';
	output << "candidate_solver_implemented=true\n";
	output << "atmosphere_solver_selection=unselected\n";
	output << "physical_scale_selection=unselected\n";
	output << "result_status=candidate_result_not_selection\n";
	output << "case_time_domain=nondimensional_contract\n";
	output << "boundary_mode=periodic\n";
	output << "grid_cells_x=" << LowMachCells << '\n';
	output << "grid_cells_y=1\n";
	output << "grid_cell_count=" << LowMachCells << '\n';
	output << "case_timestep=" << summary.veryLowMach.benchmarkCase.timeStep << '\n';
	output << "case_step_count=" << summary.veryLowMach.benchmarkCase.stepCount << '\n';
	output << "state_bytes_per_cell=" << sizeof(ConservativeState) << '\n';
	output << "state_and_flux_scratch_bytes_per_cell=" << (3 * sizeof(ConservativeState)) << '\n';
	output << "reference_shift_distance=" << LowMachShiftDistance << '\n';
	output << "moderate_velocity=" << ModerateMachVelocity << '\n';
	output << "low_velocity=" << LowMachVelocity << '\n';
	output << "very_low_velocity=" << VeryLowMachVelocity << '\n';
	output << "moderate_nominal_mach=" << summary.moderateNominalMach << '\n';
	output << "low_nominal_mach=" << summary.lowNominalMach << '\n';
	output << "very_low_nominal_mach=" << summary.veryLowNominalMach << '\n';
	output << "moderate_steps=" << summary.moderateMach.benchmarkCase.stepCount << '\n';
	output << "low_steps=" << summary.lowMach.benchmarkCase.stepCount << '\n';
	output << "very_low_steps=" << summary.veryLowMach.benchmarkCase.stepCount << '\n';
	output << "moderate_simulated_time=" << summary.moderateMach.simulatedTime << '\n';
	output << "low_simulated_time=" << summary.lowMach.simulatedTime << '\n';
	output << "very_low_simulated_time=" << summary.veryLowMach.simulatedTime << '\n';
	output << "moderate_density_l1_error=" << summary.moderateMach.densityL1Error << '\n';
	output << "low_density_l1_error=" << summary.lowMach.densityL1Error << '\n';
	output << "very_low_density_l1_error=" << summary.veryLowMach.densityL1Error << '\n';
	output << "moderate_total_variation_ratio=" << summary.moderateMach.totalVariationRatio << '\n';
	output << "low_total_variation_ratio=" << summary.lowMach.totalVariationRatio << '\n';
	output << "very_low_total_variation_ratio=" << summary.veryLowMach.totalVariationRatio << '\n';
	output << "moderate_advection_reference_passed="
		<< (summary.moderateMach.advectionReferencePassed ? "true" : "false") << '\n';
	output << "low_advection_reference_passed="
		<< (summary.lowMach.advectionReferencePassed ? "true" : "false") << '\n';
	output << "very_low_advection_reference_passed="
		<< (summary.veryLowMach.advectionReferencePassed ? "true" : "false") << '\n';
	output << "low_to_moderate_l1_ratio=" << summary.lowToModerateL1Ratio << '\n';
	output << "very_low_to_moderate_l1_ratio=" << summary.veryLowToModerateL1Ratio << '\n';
	output << "moderate_maximum_cfl=" << summary.moderateMach.maximumCfl << '\n';
	output << "low_maximum_cfl=" << summary.lowMach.maximumCfl << '\n';
	output << "very_low_maximum_cfl=" << summary.veryLowMach.maximumCfl << '\n';
	output << "minimum_density=" << summary.veryLowMach.minimumDensity << '\n';
	output << "maximum_density=" << summary.veryLowMach.maximumDensity << '\n';
	output << "minimum_pressure=" << summary.veryLowMach.minimumPressure << '\n';
	output << "maximum_pressure=" << summary.veryLowMach.finalMaximumPressure << '\n';
	output << "state_change_l1=" << summary.veryLowMach.stateChangeL1 << '\n';
	output << "state_evolved=" << (summary.veryLowMach.stateEvolved ? "true" : "false") << '\n';
	output << "positivity_preserved=" << (summary.veryLowMach.positivityPreserved ? "true" : "false") << '\n';
	output << "mass_drift="
		<< (summary.veryLowMach.ledger.final.density - summary.veryLowMach.ledger.initial.density) << '\n';
	output << "momentum_drift=" << std::hypot(
		summary.veryLowMach.ledger.final.momentumX - summary.veryLowMach.ledger.initial.momentumX,
		summary.veryLowMach.ledger.final.momentumY - summary.veryLowMach.ledger.initial.momentumY) << '\n';
	output << "momentum_x_drift="
		<< (summary.veryLowMach.ledger.final.momentumX - summary.veryLowMach.ledger.initial.momentumX) << '\n';
	output << "momentum_y_drift="
		<< (summary.veryLowMach.ledger.final.momentumY - summary.veryLowMach.ledger.initial.momentumY) << '\n';
	output << "energy_drift="
		<< (summary.veryLowMach.ledger.final.totalEnergyDensity
			- summary.veryLowMach.ledger.initial.totalEnergyDensity) << '\n';
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
	output << "moderate_flux_fallback_count=" << summary.moderateMach.fluxFallbackCount << '\n';
	output << "low_flux_fallback_count=" << summary.lowMach.fluxFallbackCount << '\n';
	output << "very_low_flux_fallback_count=" << summary.veryLowMach.fluxFallbackCount << '\n';
	output << "low_mach_suitability_passed="
		<< (summary.suitabilityPassed ? "true" : "false") << '\n';
	output << "candidate_disposition="
		<< (summary.suitabilityPassed ? "continue_evaluation" : "reject_low_mach_suitability") << '\n';
	output << "benchmark_execution_status=" << (summary.passed ? "PASS" : "FAIL") << '\n';
	output << "probe_passed=" << (summary.passed ? "true" : "false") << '\n';
	return summary.passed;
}
} // namespace

bool WriteRusanovLowMachAdvectionProbe(std::ostream &output)
{
	return WriteLowMachAdvectionProbe(output, RunRusanovLowMachAdvection(),
		"rusanov_low_mach_advection_1d", "fvm_rusanov");
}

bool WriteAllSpeedRusanovLowMachAdvectionProbe(std::ostream &output)
{
	return WriteLowMachAdvectionProbe(output, RunAllSpeedRusanovLowMachAdvection(),
		"all_speed_rusanov_low_mach_advection_1d", "fvm_all_speed_rusanov");
}

bool WriteHllcRusanovFallbackLowMachAdvectionProbe(std::ostream &output)
{
	return WriteLowMachAdvectionProbe(output, RunHllcRusanovFallbackLowMachAdvection(),
		"hllc_rusanov_fallback_low_mach_advection_1d", "fvm_hllc_rusanov_fallback");
}

namespace
{
bool WritePerformanceProbe(
	std::ostream &output,
	const RusanovPerformanceSummary &summary,
	const char *caseId,
	const char *candidateId)
{
	const auto &probe = summary.large.probe;
	const auto &initial = probe.ledger.initial;
	const auto &final = probe.ledger.final;
	output << "schema_version=1\n";
	output << "case=" << caseId << '\n';
	output << "candidate=" << candidateId << '\n';
	output << "candidate_solver_implemented=true\n";
	output << "atmosphere_solver_selection=unselected\n";
	output << "physical_scale_selection=unselected\n";
	output << "result_status=candidate_result_not_selection\n";
	output << "case_time_domain=nondimensional_contract\n";
	output << "boundary_mode=periodic\n";
	output << "grid_cells_x=" << probe.benchmarkCase.grid.cellsX << '\n';
	output << "grid_cells_y=1\n";
	output << "grid_cell_count=" << probe.benchmarkCase.grid.CellCount() << '\n';
	output << "case_timestep=" << probe.benchmarkCase.timeStep << '\n';
	output << "case_step_count=" << probe.benchmarkCase.stepCount << '\n';
	output << "performance_warmup_count=" << summary.warmupCount << '\n';
	output << "performance_repeat_count=" << summary.repeatCount << '\n';
	output << "small_cells=" << summary.small.probe.benchmarkCase.grid.cellsX << '\n';
	output << "medium_cells=" << summary.medium.probe.benchmarkCase.grid.cellsX << '\n';
	output << "large_cells=" << summary.large.probe.benchmarkCase.grid.cellsX << '\n';
	output << "performance_steps=" << PerformanceSteps << '\n';
	output << "small_elapsed_milliseconds=" << summary.small.elapsedMilliseconds << '\n';
	output << "medium_elapsed_milliseconds=" << summary.medium.elapsedMilliseconds << '\n';
	output << "large_elapsed_milliseconds=" << summary.large.elapsedMilliseconds << '\n';
	output << "small_cell_updates_per_second=" << summary.small.cellUpdatesPerSecond << '\n';
	output << "medium_cell_updates_per_second=" << summary.medium.cellUpdatesPerSecond << '\n';
	output << "large_cell_updates_per_second=" << summary.large.cellUpdatesPerSecond << '\n';
	output << "maximum_cfl=" << probe.maximumCfl << '\n';
	output << "minimum_density=" << probe.minimumDensity << '\n';
	output << "maximum_density=" << probe.maximumDensity << '\n';
	output << "minimum_pressure=" << probe.minimumPressure << '\n';
	output << "maximum_pressure=" << probe.finalMaximumPressure << '\n';
	output << "minimum_energy_density=" << probe.minimumEnergyDensity << '\n';
	output << "state_change_l1=" << probe.stateChangeL1 << '\n';
	output << "state_evolved=" << (probe.stateEvolved ? "true" : "false") << '\n';
	output << "positivity_preserved=" << (probe.positivityPreserved ? "true" : "false") << '\n';
	output << "mass_drift=" << (final.density - initial.density) << '\n';
	output << "momentum_drift=" << std::hypot(
		final.momentumX - initial.momentumX,
		final.momentumY - initial.momentumY) << '\n';
	output << "momentum_x_drift=" << (final.momentumX - initial.momentumX) << '\n';
	output << "momentum_y_drift=" << (final.momentumY - initial.momentumY) << '\n';
	output << "energy_drift=" << (final.totalEnergyDensity - initial.totalEnergyDensity) << '\n';
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
	output << "small_flux_fallback_count=" << summary.small.probe.fluxFallbackCount << '\n';
	output << "medium_flux_fallback_count=" << summary.medium.probe.fluxFallbackCount << '\n';
	output << "large_flux_fallback_count=" << summary.large.probe.fluxFallbackCount << '\n';
	output << "state_bytes_per_cell=" << sizeof(ConservativeState) << '\n';
	output << "state_and_flux_scratch_bytes_per_cell=" << (3 * sizeof(ConservativeState)) << '\n';
	output << "performance_measurement_passed=" << (summary.passed ? "true" : "false") << '\n';
	output << "probe_passed=" << (summary.passed ? "true" : "false") << '\n';
	return summary.passed;
}
} // namespace

bool WriteRusanovPerformanceProbe(std::ostream &output)
{
	return WritePerformanceProbe(
		output, RunRusanovPerformance(), "rusanov_performance_1d", "fvm_rusanov");
}

bool WriteHllcRusanovFallbackPerformanceProbe(std::ostream &output)
{
	return WritePerformanceProbe(output, RunHllcRusanovFallbackPerformance(),
		"hllc_rusanov_fallback_performance_1d", "fvm_hllc_rusanov_fallback");
}

} // namespace omni::atmospherebench
