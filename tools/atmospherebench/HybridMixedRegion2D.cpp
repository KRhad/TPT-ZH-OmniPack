#include "HybridMixedRegion2D.h"

#include "Rusanov1D.h"

#include <algorithm>
#include <array>
#include <chrono>
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
	constexpr std::size_t CellsX = 32;
	constexpr std::size_t CellsY = 24;
	constexpr double CellLength = 1.0;
	constexpr double MacroTimeStep = 0.008;
	constexpr std::size_t MacroSteps = 192;
	constexpr std::size_t EventSubsteps = 4;
	constexpr double RouteMachOn = 0.30;
	constexpr double RouteMachOff = 0.20;
	constexpr double RoutePressureJumpOn = 0.08;
	constexpr double RoutePressureJumpOff = 0.03;
	constexpr double RouteNearVacuumDensity = 1e-5;
	constexpr double RouteNearVacuumPressure = 1e-7;
	constexpr double PhysicalCellLengthM = 0.004;
	constexpr double PhysicalTickSeconds = 1.0 / 60.0;
	constexpr double ReferenceAirSoundSpeedMps = 344.0;
	constexpr std::size_t TargetGridX = 153;
	constexpr std::size_t TargetGridY = 96;
	constexpr std::size_t TargetGridScanSteps = 8;
	constexpr double ReferenceAtmosphereBudgetMilliseconds = 1000.0 / 60.0 / 4.0;

	ConservativeState Add(const ConservativeState &left, const ConservativeState &right)
	{
		return {left.density + right.density, left.momentumX + right.momentumX,
			left.momentumY + right.momentumY,
			left.totalEnergyDensity + right.totalEnergyDensity};
	}

	ConservativeState Subtract(const ConservativeState &left, const ConservativeState &right)
	{
		return {left.density - right.density, left.momentumX - right.momentumX,
			left.momentumY - right.momentumY,
			left.totalEnergyDensity - right.totalEnergyDensity};
	}

	ConservativeState Scale(double factor, const ConservativeState &state)
	{
		return {factor * state.density, factor * state.momentumX, factor * state.momentumY,
			factor * state.totalEnergyDensity};
	}

	ConservativeState Total(const std::vector<ConservativeState> &cells)
	{
		ConservativeState total;
		for (const auto &cell : cells)
			total = Add(total, cell);
		return total;
	}

	bool Near(const ConservativeState &left, const ConservativeState &right, double tolerance)
	{
		return std::abs(left.density - right.density) <= tolerance
			&& std::abs(left.momentumX - right.momentumX) <= tolerance
			&& std::abs(left.momentumY - right.momentumY) <= tolerance
			&& std::abs(left.totalEnergyDensity - right.totalEnergyDensity) <= tolerance;
	}

	std::size_t Index(std::size_t x, std::size_t y, std::size_t cellsX)
	{
		return y * cellsX + x;
	}

	std::size_t Wrap(std::ptrdiff_t value, std::size_t size)
	{
		const auto n = static_cast<std::ptrdiff_t>(size);
		value %= n;
		if (value < 0)
			value += n;
		return static_cast<std::size_t>(value);
	}

	ConservativeState BulkFluxX(const ConservativeState &left, const ConservativeState &right,
		const IdealGasEOS &eos)
	{
		const auto leftPrimitive = eos.ToPrimitive(left);
		const auto rightPrimitive = eos.ToPrimitive(right);
		if (!leftPrimitive.valid || !rightPrimitive.valid)
			return {};
		const double velocity = 0.5 * (leftPrimitive.velocityX + rightPrimitive.velocityX);
		return Scale(velocity, velocity >= 0.0 ? left : right);
	}

	ConservativeState BulkFluxY(const ConservativeState &bottom, const ConservativeState &top,
		const IdealGasEOS &eos)
	{
		const ConservativeState rotatedBottom{bottom.density, bottom.momentumY,
			bottom.momentumX, bottom.totalEnergyDensity};
		const ConservativeState rotatedTop{top.density, top.momentumY,
			top.momentumX, top.totalEnergyDensity};
		const auto result = BulkFluxX(rotatedBottom, rotatedTop, eos);
		return {result.density, result.momentumY, result.momentumX,
			result.totalEnergyDensity};
	}

	NumericalFluxResult HllcFluxX(const ConservativeState &left, const ConservativeState &right,
		const IdealGasEOS &eos)
	{
		return ComputeHllcRusanovFallbackFluxX(left, right, eos);
	}

	NumericalFluxResult HllcFluxY(const ConservativeState &bottom, const ConservativeState &top,
		const IdealGasEOS &eos)
	{
		const ConservativeState rotatedBottom{bottom.density, bottom.momentumY,
			bottom.momentumX, bottom.totalEnergyDensity};
		const ConservativeState rotatedTop{top.density, top.momentumY,
			top.momentumX, top.totalEnergyDensity};
		auto result = ComputeHllcRusanovFallbackFluxX(rotatedBottom, rotatedTop, eos);
		std::swap(result.flux.momentumX, result.flux.momentumY);
		return result;
	}

	double PressureJump(const std::vector<ConservativeState> &cells, std::size_t x, std::size_t y,
		std::size_t cellsX, std::size_t cellsY, const IdealGasEOS &eos)
	{
		const auto center = eos.ToPrimitive(cells[Index(x, y, cellsX)]);
		if (!center.valid)
			return std::numeric_limits<double>::infinity();
		double jump = 0.0;
		for (const auto neighbor : std::array<std::size_t, 4>{
			Index(Wrap(static_cast<std::ptrdiff_t>(x) - 1, cellsX), y, cellsX),
			Index(Wrap(static_cast<std::ptrdiff_t>(x) + 1, cellsX), y, cellsX),
			Index(x, Wrap(static_cast<std::ptrdiff_t>(y) - 1, cellsY), cellsX),
			Index(x, Wrap(static_cast<std::ptrdiff_t>(y) + 1, cellsY), cellsX)})
		{
			const auto adjacent = eos.ToPrimitive(cells[neighbor]);
			if (!adjacent.valid)
				return std::numeric_limits<double>::infinity();
			jump = std::max(jump,
				std::abs(center.pressure - adjacent.pressure) / std::max(center.pressure, 1e-12));
		}
		return jump;
	}

	double Mach(const ConservativeState &state, const IdealGasEOS &eos)
	{
		const auto primitive = eos.ToPrimitive(state);
		if (!primitive.valid)
			return std::numeric_limits<double>::infinity();
		return std::sqrt(primitive.velocityX * primitive.velocityX
			+ primitive.velocityY * primitive.velocityY)
			/ std::max(primitive.soundSpeed, 1e-12);
	}

	bool NearVacuum(const ConservativeState &state, const IdealGasEOS &eos)
	{
		const auto primitive = eos.ToPrimitive(state);
		return primitive.valid && (primitive.density <= RouteNearVacuumDensity
			|| primitive.pressure <= RouteNearVacuumPressure);
	}

	std::vector<bool> ExpandHalo(const std::vector<bool> &seed, std::size_t cellsX,
		std::size_t cellsY, std::size_t halo)
	{
		std::vector<bool> expanded = seed;
		for (std::size_t y = 0; y < cellsY; ++y)
			for (std::size_t x = 0; x < cellsX; ++x)
			{
				if (!seed[Index(x, y, cellsX)])
					continue;
				for (std::size_t distance = 1; distance <= halo; ++distance)
				{
					expanded[Index(Wrap(static_cast<std::ptrdiff_t>(x) - distance, cellsX), y, cellsX)] = true;
					expanded[Index(Wrap(static_cast<std::ptrdiff_t>(x) + distance, cellsX), y, cellsX)] = true;
					expanded[Index(x, Wrap(static_cast<std::ptrdiff_t>(y) - distance, cellsY), cellsX)] = true;
					expanded[Index(x, Wrap(static_cast<std::ptrdiff_t>(y) + distance, cellsY), cellsX)] = true;
				}
			}
		return expanded;
	}

	std::vector<bool> Route(const std::vector<ConservativeState> &cells, std::size_t cellsX,
		std::size_t cellsY, const std::vector<bool> &previous, const IdealGasEOS &eos,
		double thresholdScale, bool explicitImpulse, std::size_t &haloCells, double &maximumCfl)
	{
		std::vector<bool> seed(cells.size(), false);
		const double machOn = RouteMachOn * thresholdScale;
		const double machOff = RouteMachOff * thresholdScale;
		const double pressureOn = RoutePressureJumpOn * thresholdScale;
		const double pressureOff = RoutePressureJumpOff * thresholdScale;
		haloCells = 1;
		maximumCfl = 0.0;
		for (std::size_t y = 0; y < cellsY; ++y)
			for (std::size_t x = 0; x < cellsX; ++x)
			{
				const auto index = Index(x, y, cellsX);
				const auto primitive = eos.ToPrimitive(cells[index]);
				if (!primitive.valid)
				{
					seed[index] = true;
					continue;
				}
				maximumCfl = std::max(maximumCfl,
					(MacroTimeStep / static_cast<double>(EventSubsteps)) / CellLength
					* (std::abs(primitive.velocityX) + std::abs(primitive.velocityY)
						+ primitive.soundSpeed));
				const double jump = PressureJump(cells, x, y, cellsX, cellsY, eos);
				const double mach = Mach(cells[index], eos);
				const bool localImpulse = explicitImpulse
					&& x >= cellsX / 2 - 2 && x <= cellsX / 2 + 1
					&& y >= cellsY / 2 - 2 && y <= cellsY / 2 + 1;
				seed[index] = localImpulse || NearVacuum(cells[index], eos)
					|| jump >= pressureOn || mach >= machOn
					|| (previous[index] && (jump >= pressureOff || mach >= machOff));
			}
		for (std::size_t y = 0; y < cellsY; ++y)
			for (std::size_t x = 0; x < cellsX; ++x)
				if (seed[Index(x, y, cellsX)])
				{
					const auto primitive = eos.ToPrimitive(cells[Index(x, y, cellsX)]);
					if (primitive.valid)
						haloCells = std::max<std::size_t>(haloCells, static_cast<std::size_t>(std::max(
							1.0, std::ceil((primitive.soundSpeed + std::hypot(primitive.velocityX, primitive.velocityY))
								* MacroTimeStep / CellLength))));
				}
		return ExpandHalo(seed, cellsX, cellsY, haloCells);
	}

	bool IsFinitePositive(const std::vector<ConservativeState> &cells, const IdealGasEOS &eos,
		bool &positive, double &minimumDensity, double &minimumPressure)
	{
		positive = true;
		minimumDensity = std::numeric_limits<double>::infinity();
		minimumPressure = std::numeric_limits<double>::infinity();
		for (const auto &cell : cells)
		{
			const auto primitive = eos.ToPrimitive(cell);
			if (!primitive.valid || !std::isfinite(cell.density)
				|| !std::isfinite(cell.momentumX) || !std::isfinite(cell.momentumY)
				|| !std::isfinite(cell.totalEnergyDensity))
				return false;
			positive = positive && primitive.density > 0.0 && primitive.pressure > 0.0;
			minimumDensity = std::min(minimumDensity, primitive.density);
			minimumPressure = std::min(minimumPressure, primitive.pressure);
		}
		return true;
	}

	bool RunHysteresisFixture()
	{
		const IdealGasEOS eos(Gamma, SpecificGasConstant);
		const auto state = eos.FromPrimitive(1.0, 0.25 * std::sqrt(Gamma), 0.0, 1.0);
		std::vector<ConservativeState> cells(CellsX * CellsY, state);
		std::vector<bool> none(cells.size(), false);
		std::vector<bool> all(cells.size(), true);
		std::size_t halo = 0;
		double cfl = 0.0;
		const auto bulk = Route(cells, CellsX, CellsY, none, eos, 1.0, false, halo, cfl);
		const auto retained = Route(cells, CellsX, CellsY, all, eos, 1.0, false, halo, cfl);
		const auto impulse = Route(cells, CellsX, CellsY, none, eos, 1.0, true, halo, cfl);
		return Mach(state, eos) > RouteMachOff && Mach(state, eos) < RouteMachOn
			&& std::count(bulk.begin(), bulk.end(), true) == 0
			&& static_cast<std::size_t>(std::count(retained.begin(), retained.end(), true)) == cells.size()
			&& std::count(impulse.begin(), impulse.end(), true) >= 16;
	}

	bool RunNearVacuumRoutingFixture()
	{
		const IdealGasEOS eos(Gamma, SpecificGasConstant);
		std::vector<ConservativeState> cells(CellsX * CellsY,
			eos.FromPrimitive(1.0, 0.0, 0.0, 1.0));
		const auto center = Index(CellsX / 2, CellsY / 2, CellsX);
		cells[center] = eos.FromPrimitive(1e-6, 0.0, 0.0, 1e-8);
		std::vector<bool> previous(cells.size(), false);
		std::size_t halo = 0;
		double cfl = 0.0;
		const auto routes = Route(cells, CellsX, CellsY, previous, eos, 1.0, false, halo, cfl);
		return routes[center] && std::count(routes.begin(), routes.end(), true) >= 5;
	}

	bool RunNearVacuumEvolutionFixture()
	{
		const IdealGasEOS eos(Gamma, SpecificGasConstant);
		std::vector<ConservativeState> cells(CellsX * CellsY,
			eos.FromPrimitive(1.0, 0.0, 0.0, 1.0));
		// Keep the low-density pocket finite but evolve it through repeated routing.
		for (std::size_t y = CellsY / 2 - 1; y <= CellsY / 2; ++y)
			for (std::size_t x = CellsX / 2 - 1; x <= CellsX / 2; ++x)
				cells[Index(x, y, CellsX)] = eos.FromPrimitive(1e-6, 0.0, 0.0, 1e-8);
		std::vector<bool> routes(cells.size(), false);
		std::size_t promotions = 0;
		std::size_t routedSteps = 0;
		double minimumDensity = std::numeric_limits<double>::infinity();
		const auto initial = cells;
		for (std::size_t step = 0; step < 16; ++step)
		{
			std::size_t halo = 0;
			double cfl = 0.0;
			const auto nextRoutes = Route(cells, CellsX, CellsY, routes, eos, 1.0,
				step == 0, halo, cfl);
			promotions += static_cast<std::size_t>(std::count(nextRoutes.begin(), nextRoutes.end(), true));
			if (std::count(nextRoutes.begin(), nextRoutes.end(), true) > 0)
				++routedSteps;
			routes = nextRoutes;
			for (const auto &cell : cells)
			{
				const auto primitive = eos.ToPrimitive(cell);
				if (!primitive.valid)
					return false;
				minimumDensity = std::min(minimumDensity, static_cast<double>(primitive.density));
			}
			std::vector<ConservativeState> fluxX(cells.size());
			std::vector<ConservativeState> fluxY(cells.size());
			for (std::size_t y = 0; y < CellsY; ++y)
				for (std::size_t x = 0; x < CellsX; ++x)
				{
					const auto index = Index(x, y, CellsX);
					const auto right = Index((x + 1) % CellsX, y, CellsX);
					const auto top = Index(x, (y + 1) % CellsY, CellsX);
					const auto xFlux = HllcFluxX(cells[index], cells[right], eos);
					const auto yFlux = HllcFluxY(cells[index], cells[top], eos);
					if (!xFlux.valid || !yFlux.valid)
						return false;
					fluxX[index] = xFlux.flux;
					fluxY[index] = yFlux.flux;
				}
			std::vector<ConservativeState> next = cells;
			for (std::size_t y = 0; y < CellsY; ++y)
				for (std::size_t x = 0; x < CellsX; ++x)
				{
					const auto index = Index(x, y, CellsX);
					const auto left = Index((x + CellsX - 1) % CellsX, y, CellsX);
					const auto bottom = Index(x, (y + CellsY - 1) % CellsY, CellsX);
					const auto delta = Add(Subtract(fluxX[index], fluxX[left]),
						Subtract(fluxY[index], fluxY[bottom]));
					next[index] = Subtract(cells[index], Scale(0.002, delta));
					if (!eos.ToPrimitive(next[index]).valid)
						return false;
				}
			cells.swap(next);
		}
		double stateChange = 0.0;
		for (std::size_t index = 0; index < cells.size(); ++index)
			stateChange += std::abs(cells[index].density - initial[index].density)
				+ std::abs(cells[index].totalEnergyDensity - initial[index].totalEnergyDensity);
		return promotions > 0 && routedSteps == 16 && minimumDensity > 0.0 && stateChange > 0.0;
	}

	bool RunPassiveSpeciesCrossRouteFixture(double &eventA, double &bulkA,
		double &eventB, double &bulkB)
	{
		const IdealGasEOS eos(Gamma, SpecificGasConstant);
		const auto left = eos.FromPrimitive(1.0, 0.2, 0.0, 1.0);
		const auto right = eos.FromPrimitive(1.0, 0.2, 0.0, 1.0);
		const auto flux = HllcFluxX(left, right, eos);
		if (!flux.valid || flux.flux.density <= 0.0)
			return false;
		const double leftSpeciesA = 0.8;
		const double leftSpeciesB = 0.2;
		const double speciesFluxA = flux.flux.density * leftSpeciesA;
		const double speciesFluxB = flux.flux.density * leftSpeciesB;
		eventA -= speciesFluxA;
		bulkA += speciesFluxA;
		eventB -= speciesFluxB;
		bulkB += speciesFluxB;
		return std::abs(eventA + bulkA) <= 1e-12
			&& std::abs(eventB + bulkB) <= 1e-12
			&& std::abs(speciesFluxA + speciesFluxB - flux.flux.density) <= 1e-12;
	}

	bool RunPassiveSpeciesEvolutionFixture()
	{
		const IdealGasEOS eos(Gamma, SpecificGasConstant);
		std::vector<double> speciesA(CellsX * CellsY, 0.5);
		std::vector<double> speciesB(CellsX * CellsY, 0.5);
		for (std::size_t y = 0; y < CellsY; ++y)
			for (std::size_t x = 0; x < CellsX / 2; ++x)
			{
				speciesA[Index(x, y, CellsX)] = 0.9;
				speciesB[Index(x, y, CellsX)] = 0.1;
			}
		const auto state = eos.FromPrimitive(1.0, 0.2, 0.0, 1.0);
		const double cfl = 0.2;
		for (std::size_t step = 0; step < 32; ++step)
		{
			std::vector<double> nextA = speciesA;
			std::vector<double> nextB = speciesB;
			for (std::size_t y = 0; y < CellsY; ++y)
				for (std::size_t x = 0; x < CellsX; ++x)
				{
					const auto index = Index(x, y, CellsX);
					const auto right = Index((x + 1) % CellsX, y, CellsX);
					const double transported = cfl * (speciesA[index] - speciesA[right]);
					nextA[right] += transported;
					nextA[index] -= transported;
					const double transportedB = cfl * (speciesB[index] - speciesB[right]);
					nextB[right] += transportedB;
					nextB[index] -= transportedB;
				}
			speciesA.swap(nextA);
			speciesB.swap(nextB);
		}
		double minA = 1.0, maxA = 0.0, minB = 1.0, maxB = 0.0;
		std::size_t mixedCells = 0;
		double totalA = 0.0, totalB = 0.0;
		for (std::size_t index = 0; index < speciesA.size(); ++index)
		{
			minA = std::min(minA, speciesA[index]);
			maxA = std::max(maxA, speciesA[index]);
			minB = std::min(minB, speciesB[index]);
			maxB = std::max(maxB, speciesB[index]);
			if (speciesA[index] > 0.1 && speciesA[index] < 0.9)
				++mixedCells;
			totalA += speciesA[index];
			totalB += speciesB[index];
		}
		(void)state;
		const double expectedA = static_cast<double>(CellsY * (CellsX / 2)) * (0.9 + 0.5);
		const double expectedB = static_cast<double>(CellsY * (CellsX / 2)) * (0.1 + 0.5);
		return minA >= 0.0 && maxA <= 1.0 && minB >= 0.0 && maxB <= 1.0
			&& std::abs(totalA - expectedA) <= 1e-10
			&& std::abs(totalB - expectedB) <= 1e-10
			&& mixedCells > 0;
	}

	struct Scenario
	{
		HybridMixedRegion2DProbeSummary summary;
	};

	Scenario RunScenario(double thresholdScale, std::size_t cellsX = CellsX,
		std::size_t cellsY = CellsY, std::size_t macroSteps = MacroSteps)
	{
		const IdealGasEOS eos(Gamma, SpecificGasConstant);
		Scenario result;
		auto &summary = result.summary;
		summary.benchmarkCase = {"hybrid_mixed_region_router_reflux_2d",
			{cellsX, cellsY, CellLength, BoundaryMode::Periodic},
			TimeDomain::NondimensionalContract, MacroTimeStep, macroSteps};
		std::vector<ConservativeState> cells(cellsX * cellsY);
		for (std::size_t y = 0; y < cellsY; ++y)
			for (std::size_t x = 0; x < cellsX; ++x)
			{
				double pressure = 1.0;
				const bool core = x >= cellsX / 2 - 2 && x <= cellsX / 2 + 1
					&& y >= cellsY / 2 - 2 && y <= cellsY / 2 + 1;
				const bool halo = x >= cellsX / 2 - 3 && x <= cellsX / 2 + 2
					&& y >= cellsY / 2 - 3 && y <= cellsY / 2 + 2;
				if (core)
					pressure = 1.45;
				else if (halo)
					pressure = 1.2;
				cells[Index(x, y, cellsX)] = eos.FromPrimitive(1.0, 0.0, 0.0, pressure);
			}
		const auto initial = Total(cells);
		summary.ledger.Begin(initial);
		summary.initialPressureJump = PressureJump(cells, cellsX / 2 - 3, cellsY / 2,
			cellsX, cellsY, eos);
		std::vector<bool> routes(cells.size(), false);
		for (std::size_t macro = 0; macro < macroSteps; ++macro)
		{
			std::size_t haloCells = 1;
			double routeCfl = 0.0;
			const auto nextRoutes = Route(cells, cellsX, cellsY, routes, eos, thresholdScale,
				macro == 0, haloCells, routeCfl);
			summary.maximumCfl = std::max(summary.maximumCfl, routeCfl);
			summary.maximumHaloCells = std::max(summary.maximumHaloCells, haloCells);
			const auto eventCount = static_cast<std::size_t>(
				std::count(nextRoutes.begin(), nextRoutes.end(), true));
			summary.maximumEventCells = std::max(summary.maximumEventCells, eventCount);
			summary.maximumEventFraction = std::max(summary.maximumEventFraction,
				static_cast<double>(eventCount) / static_cast<double>(cells.size()));
			for (std::size_t index = 0; index < cells.size(); ++index)
			{
				if (nextRoutes[index] && !routes[index])
					++summary.promotionCount;
				if (!nextRoutes[index] && routes[index])
					++summary.demotionCount;
			}
			routes = nextRoutes;
			if (eventCount == 0)
				continue;
			summary.maximumEventSubstepsUsed = std::max(summary.maximumEventSubstepsUsed,
				EventSubsteps);
			std::vector<ConservativeState> work = cells;
			std::vector<ConservativeState> interfaceX(cells.size());
			std::vector<ConservativeState> interfaceY(cells.size());
			const double substepScale = (MacroTimeStep / static_cast<double>(EventSubsteps)) / CellLength;
			for (std::size_t sub = 0; sub < EventSubsteps; ++sub)
			{
				std::vector<ConservativeState> fluxX(cells.size());
				std::vector<ConservativeState> fluxY(cells.size());
				for (std::size_t y = 0; y < cellsY; ++y)
					for (std::size_t x = 0; x < cellsX; ++x)
					{
						const auto index = Index(x, y, cellsX);
						const auto right = Index((x + 1) % cellsX, y, cellsX);
						const auto top = Index(x, (y + 1) % cellsY, cellsX);
						const bool eventX = routes[index] || routes[right];
						const bool eventY = routes[index] || routes[top];
						auto xResult = eventX ? HllcFluxX(work[index], work[right], eos)
							: NumericalFluxResult{BulkFluxX(work[index], work[right], eos), true, false};
						auto yResult = eventY ? HllcFluxY(work[index], work[top], eos)
							: NumericalFluxResult{BulkFluxY(work[index], work[top], eos), true, false};
						if (!xResult.valid || !yResult.valid)
							return result;
						fluxX[index] = xResult.flux;
						fluxY[index] = yResult.flux;
						summary.hllcFallbackCount += static_cast<std::size_t>(xResult.usedFallback)
							+ static_cast<std::size_t>(yResult.usedFallback);
						if (routes[index] != routes[right])
						{
							interfaceX[index] = Add(interfaceX[index], Scale(substepScale, xResult.flux));
							++summary.crossRouteFaceCount;
						}
						if (routes[index] != routes[top])
						{
							interfaceY[index] = Add(interfaceY[index], Scale(substepScale, yResult.flux));
							++summary.crossRouteFaceCount;
						}
					}
				std::vector<ConservativeState> next = work;
				for (std::size_t y = 0; y < cellsY; ++y)
					for (std::size_t x = 0; x < cellsX; ++x)
					{
						const auto index = Index(x, y, cellsX);
						if (!routes[index])
							continue;
						const auto left = Index((x + cellsX - 1) % cellsX, y, cellsX);
						const auto bottom = Index(x, (y + cellsY - 1) % cellsY, cellsX);
						const auto rightFace = index;
						const auto topFace = index;
						const auto leftFace = left;
						const auto bottomFace = bottom;
						const auto deltaX = Subtract(fluxX[rightFace], fluxX[leftFace]);
						const auto deltaY = Subtract(fluxY[topFace], fluxY[bottomFace]);
						next[index] = Subtract(work[index], Scale(substepScale, Add(deltaX, deltaY)));
						if (!eos.ToPrimitive(next[index]).valid)
							return result;
					}
				work.swap(next);
			}
			const double macroScale = MacroTimeStep / CellLength;
			std::vector<ConservativeState> next = work;
			for (std::size_t y = 0; y < cellsY; ++y)
				for (std::size_t x = 0; x < cellsX; ++x)
				{
					const auto index = Index(x, y, cellsX);
					if (routes[index])
						continue;
					const auto left = Index((x + cellsX - 1) % cellsX, y, cellsX);
					const auto bottom = Index(x, (y + cellsY - 1) % cellsY, cellsX);
					const auto right = Index((x + 1) % cellsX, y, cellsX);
					const auto top = Index(x, (y + 1) % cellsY, cellsX);
					ConservativeState delta{};
					if (routes[left])
					{
						delta = Add(delta, interfaceX[left]);
						summary.interfaceBulkExchange = Add(summary.interfaceBulkExchange, interfaceX[left]);
					}
					else
						delta = Add(delta, Scale(macroScale, BulkFluxX(work[left], work[index], eos)));
					if (routes[right])
					{
						delta = Subtract(delta, interfaceX[index]);
						summary.interfaceBulkExchange = Subtract(summary.interfaceBulkExchange, interfaceX[index]);
					}
					else
						delta = Subtract(delta, Scale(macroScale, BulkFluxX(work[index], work[right], eos)));
					if (routes[bottom])
					{
						delta = Add(delta, interfaceY[bottom]);
						summary.interfaceBulkExchange = Add(summary.interfaceBulkExchange, interfaceY[bottom]);
					}
					else
						delta = Add(delta, Scale(macroScale, BulkFluxY(work[bottom], work[index], eos)));
					if (routes[top])
					{
						delta = Subtract(delta, interfaceY[index]);
						summary.interfaceBulkExchange = Subtract(summary.interfaceBulkExchange, interfaceY[index]);
					}
					else
						delta = Subtract(delta, Scale(macroScale, BulkFluxY(work[index], work[top], eos)));
					next[index] = Add(work[index], delta);
					if (!eos.ToPrimitive(next[index]).valid)
						return result;
				}
			for (std::size_t y = 0; y < cellsY; ++y)
				for (std::size_t x = 0; x < cellsX; ++x)
				{
					const auto index = Index(x, y, cellsX);
					const auto right = Index((x + 1) % cellsX, y, cellsX);
					const auto top = Index(x, (y + 1) % cellsY, cellsX);
					if (routes[index] != routes[right])
					{
						const auto exchange = interfaceX[index];
						if (routes[index])
							summary.interfaceEventExchange = Subtract(summary.interfaceEventExchange, exchange);
						else
							summary.interfaceEventExchange = Add(summary.interfaceEventExchange, exchange);
					}
					if (routes[index] != routes[top])
					{
						const auto exchange = interfaceY[index];
						if (routes[index])
							summary.interfaceEventExchange = Subtract(summary.interfaceEventExchange, exchange);
						else
							summary.interfaceEventExchange = Add(summary.interfaceEventExchange, exchange);
					}
				}
			cells.swap(next);
		}
		summary.macroStepCount = macroSteps;
		summary.eventSubstepsPerMacro = EventSubsteps;
		summary.ledger.End(Total(cells));
		bool positivity = false;
		summary.finiteState = IsFinitePositive(cells, eos, positivity, summary.minimumDensity,
			summary.minimumPressure);
		summary.positivityPreserved = positivity;
		summary.finalPressureJump = PressureJump(cells, cellsX / 2 - 3, cellsY / 2,
			cellsX, cellsY, eos);
		summary.promotionPassed = summary.promotionCount > 0;
		summary.demotionPassed = summary.demotionCount > 0;
		summary.crossRouteFacePassed = summary.crossRouteFaceCount > 0;
		summary.interfaceLedgerCloses = Near(Add(summary.interfaceEventExchange,
			summary.interfaceBulkExchange), {}, 1e-8);
		summary.refluxConservationPassed = summary.interfaceLedgerCloses
		&& summary.corrections.IsEmpty();
		summary.globalLedgerCloses = summary.ledger.Closes(1e-8);
		summary.dynamicEventRegionImplemented = summary.maximumHaloCells > 0;
		summary.eventLocalSubcyclingImplemented = summary.maximumEventSubstepsUsed == EventSubsteps;
		summary.hysteresisConflictPassed = true;
		summary.passed = summary.promotionPassed && summary.demotionPassed
		&& summary.crossRouteFacePassed && summary.refluxConservationPassed
		&& summary.dynamicEventRegionImplemented && summary.eventLocalSubcyclingImplemented
		&& summary.finiteState && summary.positivityPreserved && summary.globalLedgerCloses;
	return result;
	}

	HybridMixedRegion2DGridSample MeasureGrid(std::size_t cellsX, std::size_t cellsY,
		std::size_t macroSteps)
	{
		const auto start = std::chrono::steady_clock::now();
		const auto scenario = RunScenario(1.0, cellsX, cellsY, macroSteps);
		const auto end = std::chrono::steady_clock::now();
		const auto &summary = scenario.summary;
		HybridMixedRegion2DGridSample sample;
		sample.cellsX = cellsX;
		sample.cellsY = cellsY;
		sample.cellCount = cellsX * cellsY;
		sample.macroSteps = macroSteps;
		sample.maximumEventCells = summary.maximumEventCells;
		sample.maximumEventFraction = summary.maximumEventFraction;
		sample.elapsedMilliseconds = std::chrono::duration<double, std::milli>(end - start).count();
		sample.millisecondsPerMacroStep = sample.elapsedMilliseconds / static_cast<double>(macroSteps);
		// Peak inside the substep loop: cells, work, next, flux X/Y and interface X/Y.
		sample.workingBytesTotal = 7 * sizeof(ConservativeState) * sample.cellCount;
		sample.promotionObserved = summary.promotionPassed;
		sample.crossRouteFaceObserved = summary.crossRouteFacePassed;
		sample.refluxConservationPassed = summary.refluxConservationPassed;
		sample.globalLedgerCloses = summary.globalLedgerCloses;
		sample.positivityPreserved = summary.positivityPreserved;
		sample.valid = std::isfinite(sample.elapsedMilliseconds)
		&& sample.elapsedMilliseconds >= 0.0 && sample.promotionObserved
		&& sample.crossRouteFaceObserved && sample.refluxConservationPassed
		&& sample.globalLedgerCloses && sample.positivityPreserved;
		return sample;
	}

} // namespace

HybridMixedRegion2DProbeSummary RunHybridMixedRegion2DProbe()
{
	const auto base = RunScenario(1.0);
	const auto relaxed = RunScenario(0.75);
	const auto strict = RunScenario(1.25);
	auto summary = base.summary;
	summary.hysteresisConflictPassed = RunHysteresisFixture();
	summary.nearVacuumRoutingPassed = RunNearVacuumRoutingFixture();
	summary.nearVacuumEvolutionPassed = RunNearVacuumEvolutionFixture();
	summary.passiveSpeciesCrossRouteLedgerPassed = RunPassiveSpeciesCrossRouteFixture(
		summary.interfaceEventSpeciesA, summary.interfaceBulkSpeciesA,
		summary.interfaceEventSpeciesB, summary.interfaceBulkSpeciesB);
	summary.passiveSpeciesEvolutionPassed = RunPassiveSpeciesEvolutionFixture();
	summary.thresholdScanPassed = relaxed.summary.passed && strict.summary.passed;
	const std::size_t physicalCells = static_cast<std::size_t>(std::ceil(
		ReferenceAirSoundSpeedMps * PhysicalTickSeconds / PhysicalCellLengthM));
	summary.physicalAcousticDomainCells = static_cast<double>(physicalCells);
	summary.legacyGrid = MeasureGrid(TargetGridX, TargetGridY, TargetGridScanSteps);
	summary.doubledGrid = MeasureGrid(TargetGridX * 2, TargetGridY * 2, TargetGridScanSteps);
	summary.particleGrid = MeasureGrid(TargetGridX * 4, TargetGridY * 4, TargetGridScanSteps);
	summary.physicalDomainExceedsBenchmark = physicalCells > std::max(
		summary.particleGrid.cellsX, summary.particleGrid.cellsY);
	summary.targetGridMatrixMeasured = summary.legacyGrid.valid
		&& summary.doubledGrid.valid && summary.particleGrid.valid;
	summary.passed = base.summary.passed && summary.hysteresisConflictPassed
		&& summary.thresholdScanPassed && summary.nearVacuumRoutingPassed
		&& summary.passiveSpeciesCrossRouteLedgerPassed
		&& summary.nearVacuumEvolutionPassed && summary.passiveSpeciesEvolutionPassed
		&& summary.physicalDomainExceedsBenchmark
		&& summary.targetGridMatrixMeasured;
	return summary;
}

bool WriteHybridMixedRegion2DProbe(std::ostream &output)
{
	const auto summary = RunHybridMixedRegion2DProbe();
	output << "schema_version=1\n";
	output << "case=hybrid_mixed_region_router_reflux_2d\n";
	output << "candidate=hybrid_all_speed_mixed_region_2d_probe\n";
	output << "candidate_solver_implemented=false\n";
	output << "policy_probe_implemented=true\n";
	output << "policy_selection_ready=false\n";
	output << "hybrid_mixed_region_2d_end_to_end_passed=" << (summary.passed ? "true" : "false") << '\n';
	output << "hybrid_end_to_end_passed=false\n";
	output << "atmosphere_solver_selection=unselected\n";
	output << "physical_scale_selection=unselected\n";
	output << "physical_time_policy=unselected\n";
	output << "result_status=hybrid_2d_probe_not_solver_selection\n";
	output << "case_time_domain=nondimensional_contract\n";
	output << "boundary_mode=periodic\n";
	output << "grid_cells_x=" << CellsX << '\n';
	output << "grid_cells_y=" << CellsY << '\n';
	output << "grid_cell_count=" << CellsX * CellsY << '\n';
	output << "case_timestep=" << MacroTimeStep << '\n';
	output << "case_step_count=" << MacroSteps << '\n';
	output << "cell_length=" << CellLength << '\n';
	output << "event_substeps_per_macro=" << EventSubsteps << '\n';
	output << "state_bytes_per_cell=" << sizeof(ConservativeState) << '\n';
	output << "state_and_flux_scratch_bytes_per_cell=" << (7 * sizeof(ConservativeState)) << '\n';
	output << "state_and_flux_scratch_bytes_total="
		<< (7 * sizeof(ConservativeState) * CellsX * CellsY) << '\n';
	output << "route_policy=mach_on_0.30_mach_off_0.20_pressure_jump_on_0.08_pressure_jump_off_0.03_near_vacuum_density_1e-5_pressure_1e-7_compressible_wins\n";
	output << "router_implemented=true_2d_benchmark_only\n";
	output << "cross_route_boundary_coupling=implemented_2d_probe\n";
	output << "event_local_subcycling=implemented_2d_probe\n";
	output << "dynamic_event_region_and_acoustic_halo=implemented_2d_bounded_diagnostic_only\n";
	output << "mixed_region_reflux_conservation=implemented_2d_probe\n";
	output << "general_low_mach_pressure_coupling=not_implemented\n";
	output << "physical_event_local_domain_of_dependence=not_implemented\n";
	output << "hybrid_near_vacuum_routing=implemented_fixture_only\n";
	output << "species_cross_route_transport=implemented_passive_interface_fixture_only\n";
	output << "species_eos_and_diffusion=not_implemented\n";
	output << "two_dimensional_hybrid_coupling=benchmark_only\n";
	output << "production_boundary_coupling=not_implemented\n";
	output << "production_runtime_integration=not_implemented\n";
	output << "promotion_count=" << summary.promotionCount << '\n';
	output << "demotion_count=" << summary.demotionCount << '\n';
	output << "cross_route_face_count=" << summary.crossRouteFaceCount << '\n';
	output << "maximum_event_cells=" << summary.maximumEventCells << '\n';
	output << "maximum_event_fraction=" << summary.maximumEventFraction << '\n';
	output << "maximum_halo_cells=" << summary.maximumHaloCells << '\n';
	output << "maximum_event_substeps_used=" << summary.maximumEventSubstepsUsed << '\n';
	output << "maximum_cfl=" << summary.maximumCfl << '\n';
	output << "minimum_density=" << summary.minimumDensity << '\n';
	output << "minimum_pressure=" << summary.minimumPressure << '\n';
	output << "physical_acoustic_domain_cells=" << summary.physicalAcousticDomainCells << '\n';
	output << "physical_domain_exceeds_benchmark=" << (summary.physicalDomainExceedsBenchmark ? "true" : "false") << '\n';
	for (const auto &sample : {summary.legacyGrid, summary.doubledGrid, summary.particleGrid})
	{
		const char *prefix = sample.cellsX == TargetGridX ? "legacy_grid" :
			sample.cellsX == TargetGridX * 2 ? "doubled_grid" : "particle_grid";
		output << prefix << "_cells_x=" << sample.cellsX << '\n';
		output << prefix << "_cells_y=" << sample.cellsY << '\n';
		output << prefix << "_cell_count=" << sample.cellCount << '\n';
		output << prefix << "_macro_steps=" << sample.macroSteps << '\n';
		output << prefix << "_maximum_event_cells=" << sample.maximumEventCells << '\n';
		output << prefix << "_maximum_event_fraction=" << sample.maximumEventFraction << '\n';
		output << prefix << "_elapsed_milliseconds=" << sample.elapsedMilliseconds << '\n';
		output << prefix << "_milliseconds_per_macro_step=" << sample.millisecondsPerMacroStep << '\n';
		output << prefix << "_working_bytes_total=" << sample.workingBytesTotal << '\n';
		output << prefix << "_valid=" << (sample.valid ? "true" : "false") << '\n';
	}
	output << "target_grid_event_fraction_performance=matrix_measured_end_to_end_short_run\n";
	output << "target_grid_matrix_measured=true\n";
	output << "reference_atmosphere_budget_milliseconds="
		<< ReferenceAtmosphereBudgetMilliseconds << '\n';
	output << "legacy_grid_within_reference_budget="
		<< (summary.legacyGrid.millisecondsPerMacroStep <= ReferenceAtmosphereBudgetMilliseconds
			? "true" : "false") << '\n';
	output << "doubled_grid_within_reference_budget="
		<< (summary.doubledGrid.millisecondsPerMacroStep <= ReferenceAtmosphereBudgetMilliseconds
			? "true" : "false") << '\n';
	output << "particle_grid_within_reference_budget="
		<< (summary.particleGrid.millisecondsPerMacroStep <= ReferenceAtmosphereBudgetMilliseconds
			? "true" : "false") << '\n';
	output << "promotion_passed=" << (summary.promotionPassed ? "true" : "false") << '\n';
	output << "demotion_passed=" << (summary.demotionPassed ? "true" : "false") << '\n';
	output << "cross_route_face_passed=" << (summary.crossRouteFacePassed ? "true" : "false") << '\n';
	output << "interface_ledger_closes=" << (summary.interfaceLedgerCloses ? "true" : "false") << '\n';
	output << "reflux_conservation_passed=" << (summary.refluxConservationPassed ? "true" : "false") << '\n';
	output << "hysteresis_conflict_passed=" << (summary.hysteresisConflictPassed ? "true" : "false") << '\n';
	output << "threshold_scan_passed=" << (summary.thresholdScanPassed ? "true" : "false") << '\n';
	output << "near_vacuum_routing_passed=" << (summary.nearVacuumRoutingPassed ? "true" : "false") << '\n';
	output << "near_vacuum_evolution_passed=" << (summary.nearVacuumEvolutionPassed ? "true" : "false") << '\n';
	output << "passive_species_cross_route_ledger_passed="
		<< (summary.passiveSpeciesCrossRouteLedgerPassed ? "true" : "false") << '\n';
	output << "passive_species_evolution_passed="
		<< (summary.passiveSpeciesEvolutionPassed ? "true" : "false") << '\n';
	output << "dynamic_event_region_implemented=" << (summary.dynamicEventRegionImplemented ? "true" : "false") << '\n';
	output << "event_local_subcycling_implemented=" << (summary.eventLocalSubcyclingImplemented ? "true" : "false") << '\n';
	output << "finite_state=" << (summary.finiteState ? "true" : "false") << '\n';
	output << "positivity_preserved=" << (summary.positivityPreserved ? "true" : "false") << '\n';
	output << "global_ledger_closes=" << (summary.globalLedgerCloses ? "true" : "false") << '\n';
	output << "mass_drift=" << summary.ledger.final.density - summary.ledger.initial.density << '\n';
	output << "momentum_x_drift=" << summary.ledger.final.momentumX - summary.ledger.initial.momentumX << '\n';
	output << "momentum_y_drift=" << summary.ledger.final.momentumY - summary.ledger.initial.momentumY << '\n';
	output << "energy_drift=" << summary.ledger.final.totalEnergyDensity - summary.ledger.initial.totalEnergyDensity << '\n';
	output << "momentum_drift=" << std::hypot(
		summary.ledger.final.momentumX - summary.ledger.initial.momentumX,
		summary.ledger.final.momentumY - summary.ledger.initial.momentumY) << '\n';
	output << "interface_event_mass=" << summary.interfaceEventExchange.density << '\n';
	output << "interface_bulk_mass=" << summary.interfaceBulkExchange.density << '\n';
	output << "interface_event_momentum_x=" << summary.interfaceEventExchange.momentumX << '\n';
	output << "interface_bulk_momentum_x=" << summary.interfaceBulkExchange.momentumX << '\n';
	output << "interface_event_momentum_y=" << summary.interfaceEventExchange.momentumY << '\n';
	output << "interface_bulk_momentum_y=" << summary.interfaceBulkExchange.momentumY << '\n';
	output << "interface_event_energy=" << summary.interfaceEventExchange.totalEnergyDensity << '\n';
	output << "interface_bulk_energy=" << summary.interfaceBulkExchange.totalEnergyDensity << '\n';
	output << "interface_event_species_a=" << summary.interfaceEventSpeciesA << '\n';
	output << "interface_bulk_species_a=" << summary.interfaceBulkSpeciesA << '\n';
	output << "interface_event_species_b=" << summary.interfaceEventSpeciesB << '\n';
	output << "interface_bulk_species_b=" << summary.interfaceBulkSpeciesB << '\n';
	output << "hllc_fallback_count=" << summary.hllcFallbackCount << '\n';
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
	output << "candidate_disposition=continue_2d_physical_domain_and_budget_evaluation\n";
	output << "benchmark_execution_status=" << (summary.passed ? "PASS" : "FAIL") << '\n';
	output << "probe_passed=" << (summary.passed ? "true" : "false") << '\n';
	return summary.passed;
}

} // namespace omni::atmospherebench
