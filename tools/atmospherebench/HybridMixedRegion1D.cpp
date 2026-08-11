#include "HybridMixedRegion1D.h"

#include "Rusanov1D.h"

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
	constexpr double Gamma = 5.0 / 3.0;
	constexpr double SpecificGasConstant = 1.0;
	constexpr std::size_t Cells = 64;
	constexpr double CellLength = 1.0 / static_cast<double>(Cells);
	constexpr double MacroTimeStep = 0.008;
	constexpr std::size_t MacroSteps = 384;
	constexpr std::size_t EventSubsteps = 4;
	constexpr double RouteMachOn = 0.30;
	constexpr double RouteMachOff = 0.20;
	constexpr double RoutePressureJumpOn = 0.08;
	constexpr double RoutePressureJumpOff = 0.03;

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

	ConservativeState BulkTransportFlux(
		const ConservativeState &left,
		const ConservativeState &right,
		const IdealGasEOS &eos)
	{
		const auto leftPrimitive = eos.ToPrimitive(left);
		const auto rightPrimitive = eos.ToPrimitive(right);
		if (!leftPrimitive.valid || !rightPrimitive.valid)
			return {};
		const double velocity = 0.5 * (leftPrimitive.velocityX + rightPrimitive.velocityX);
		return Scale(velocity, velocity >= 0.0 ? left : right);
	}

	double PressureJump(
		const std::vector<ConservativeState> &cells,
		std::size_t index,
		const IdealGasEOS &eos)
	{
		const auto center = eos.ToPrimitive(cells[index]);
		if (!center.valid)
			return std::numeric_limits<double>::infinity();
		double jump = 0.0;
		for (const auto neighbor : {
			(index + Cells - 1) % Cells,
			(index + 1) % Cells,
		})
		{
			const auto adjacent = eos.ToPrimitive(cells[neighbor]);
			if (!adjacent.valid)
				return std::numeric_limits<double>::infinity();
			jump = std::max(jump,
				std::abs(center.pressure - adjacent.pressure)
					/ std::max(center.pressure, 1e-12));
		}
		return jump;
	}

	double Mach(const ConservativeState &state, const IdealGasEOS &eos)
	{
		const auto primitive = eos.ToPrimitive(state);
		return primitive.valid
			? std::abs(primitive.velocityX) / std::max(primitive.soundSpeed, 1e-12)
			: std::numeric_limits<double>::infinity();
	}

	std::vector<bool> ExpandHalo(
		const std::vector<bool> &seed,
		std::size_t haloCells)
	{
		std::vector<bool> expanded = seed;
		for (std::size_t cell = 0; cell < Cells; ++cell)
		{
			if (!seed[cell])
				continue;
			for (std::size_t distance = 1; distance <= haloCells; ++distance)
			{
				expanded[(cell + Cells - distance) % Cells] = true;
				expanded[(cell + distance) % Cells] = true;
			}
		}
		return expanded;
	}

	std::vector<bool> Route(
		const std::vector<ConservativeState> &cells,
		const std::vector<bool> &previous,
		const IdealGasEOS &eos,
		double thresholdScale,
		bool explicitImpulse,
		std::size_t &haloCells,
		double &maximumCfl)
	{
		std::vector<bool> seed(Cells, false);
		const double machOn = RouteMachOn * thresholdScale;
		const double machOff = RouteMachOff * thresholdScale;
		const double pressureOn = RoutePressureJumpOn * thresholdScale;
		const double pressureOff = RoutePressureJumpOff * thresholdScale;
		haloCells = 1;
		maximumCfl = 0.0;
		for (std::size_t cell = 0; cell < Cells; ++cell)
		{
			const auto primitive = eos.ToPrimitive(cells[cell]);
			if (!primitive.valid)
			{
				seed[cell] = true;
				continue;
			}
			maximumCfl = std::max(maximumCfl,
				MacroTimeStep / static_cast<double>(EventSubsteps) / CellLength
					* (std::abs(primitive.velocityX) + primitive.soundSpeed));
			const double jump = PressureJump(cells, cell, eos);
			const double mach = Mach(cells[cell], eos);
			const bool localImpulse = explicitImpulse && cell >= 29 && cell <= 34;
			seed[cell] = localImpulse || jump >= pressureOn || mach >= machOn
				|| (previous[cell] && (jump >= pressureOff || mach >= machOff));
		}
		for (std::size_t cell = 0; cell < Cells; ++cell)
		{
			if (!seed[cell])
				continue;
			const auto primitive = eos.ToPrimitive(cells[cell]);
			if (primitive.valid)
				haloCells = std::max<std::size_t>(haloCells, static_cast<std::size_t>(std::max(
					1.0, std::ceil(primitive.soundSpeed * MacroTimeStep / CellLength))));
		}
		return ExpandHalo(seed, haloCells);
	}

	bool IsFiniteState(const std::vector<ConservativeState> &cells, const IdealGasEOS &eos,
		bool &positivity)
	{
		positivity = true;
		for (const auto &cell : cells)
		{
			const auto primitive = eos.ToPrimitive(cell);
			if (!primitive.valid || !std::isfinite(cell.density)
				|| !std::isfinite(cell.momentumX) || !std::isfinite(cell.momentumY)
				|| !std::isfinite(cell.totalEnergyDensity))
				return false;
			positivity = positivity && primitive.density > 0.0 && primitive.pressure > 0.0
				&& cell.totalEnergyDensity > 0.0;
		}
		return true;
	}

	struct Scenario
	{
		HybridMixedRegionProbeSummary summary;
		std::vector<bool> finalRoutes;
	};

	Scenario RunScenario(double thresholdScale)
	{
		const IdealGasEOS eos(Gamma, SpecificGasConstant);
		Scenario scenario;
		auto &summary = scenario.summary;
		summary.benchmarkCase = {
			"hybrid_mixed_region_router_reflux_1d",
			{Cells, 1, CellLength, BoundaryMode::Periodic},
			TimeDomain::NondimensionalContract,
			MacroTimeStep,
			MacroSteps,
		};
		std::vector<ConservativeState> cells(Cells);
		for (std::size_t cell = 0; cell < Cells; ++cell)
		{
			double pressure = 1.0;
			if (cell >= 30 && cell <= 33)
				pressure = 1.4;
			else if (cell == 29 || cell == 34)
				pressure = 1.2;
			cells[cell] = eos.FromPrimitive(1.0, 0.0, 0.0, pressure);
		}
		const auto initial = Total(cells);
		summary.ledger.Begin(initial);
		summary.initialPressureJump = PressureJump(cells, 29, eos);
		std::vector<bool> routes(Cells, false);
		for (std::size_t macro = 0; macro < MacroSteps; ++macro)
		{
			std::size_t haloCells = 1;
			double routeCfl = 0.0;
			const auto nextRoutes = Route(cells, routes, eos, thresholdScale, macro == 0,
				haloCells, routeCfl);
			summary.maximumCfl = std::max(summary.maximumCfl, routeCfl);
			summary.maximumHaloCells = std::max(summary.maximumHaloCells, haloCells);
			summary.maximumEventCells = std::max(summary.maximumEventCells,
				static_cast<std::size_t>(std::count(nextRoutes.begin(), nextRoutes.end(), true)));
			summary.maximumEventFraction = std::max(summary.maximumEventFraction,
				static_cast<double>(std::count(nextRoutes.begin(), nextRoutes.end(), true))
					/ static_cast<double>(Cells));
			for (std::size_t cell = 0; cell < Cells; ++cell)
			{
				if (nextRoutes[cell] && !routes[cell])
					++summary.promotionCount;
				if (!nextRoutes[cell] && routes[cell])
					++summary.demotionCount;
			}
			routes = nextRoutes;
			const std::size_t eventCount = static_cast<std::size_t>(
				std::count(routes.begin(), routes.end(), true));
			if (eventCount == 0)
				continue;
			summary.maximumEventSubstepsUsed = std::max(summary.maximumEventSubstepsUsed,
				EventSubsteps);
			std::vector<ConservativeState> work = cells;
			std::array<ConservativeState, Cells> interfaceIntegral{};
			const double substep = MacroTimeStep / static_cast<double>(EventSubsteps);
			for (std::size_t sub = 0; sub < EventSubsteps; ++sub)
			{
				std::array<ConservativeState, Cells> flux{};
				for (std::size_t face = 0; face < Cells; ++face)
				{
					const std::size_t left = face;
					const std::size_t right = (face + 1) % Cells;
					if (routes[left] || routes[right])
					{
						const auto numerical = ComputeHllcRusanovFallbackFluxX(
							work[left], work[right], eos);
						if (!numerical.valid)
							return scenario;
						flux[face] = numerical.flux;
						if (numerical.usedFallback)
							++summary.hllcFallbackCount;
					}
					else
						flux[face] = BulkTransportFlux(work[left], work[right], eos);
					if (routes[left] != routes[right])
					{
						interfaceIntegral[face] = Add(interfaceIntegral[face],
							Scale(substep / CellLength, flux[face]));
						++summary.crossRouteFaceCount;
					}
				}
				std::vector<ConservativeState> next = work;
				for (std::size_t cell = 0; cell < Cells; ++cell)
				{
					if (!routes[cell])
						continue;
					const std::size_t leftFace = (cell + Cells - 1) % Cells;
					const std::size_t rightFace = cell;
					next[cell] = Subtract(work[cell], Scale(substep / CellLength,
						Subtract(flux[rightFace], flux[leftFace])));
					if (!eos.ToPrimitive(next[cell]).valid)
						return scenario;
				}
				work.swap(next);
			}
			const double macroScale = MacroTimeStep / CellLength;
			std::vector<ConservativeState> next = work;
			for (std::size_t cell = 0; cell < Cells; ++cell)
			{
				if (routes[cell])
					continue;
				const std::size_t leftFace = (cell + Cells - 1) % Cells;
				const std::size_t rightFace = cell;
				ConservativeState delta{};
				if (routes[(cell + Cells - 1) % Cells])
				{
					const auto exchange = interfaceIntegral[leftFace];
					delta = Add(delta, exchange);
					summary.interfaceBulkExchange = Add(summary.interfaceBulkExchange, exchange);
				}
				else
					delta = Add(delta, Scale(macroScale,
						BulkTransportFlux(work[(cell + Cells - 1) % Cells], work[cell], eos)));
				if (routes[(cell + 1) % Cells])
				{
					const auto exchange = Scale(-1.0, interfaceIntegral[rightFace]);
					delta = Add(delta, exchange);
					summary.interfaceBulkExchange = Add(summary.interfaceBulkExchange, exchange);
				}
				else
					delta = Subtract(delta, Scale(macroScale,
						BulkTransportFlux(work[cell], work[(cell + 1) % Cells], eos)));
				next[cell] = Add(work[cell], delta);
				if (!eos.ToPrimitive(next[cell]).valid)
					return scenario;
			}
			// The event update has already applied the opposite cross-face deltas.
			for (std::size_t face = 0; face < Cells; ++face)
			{
				const std::size_t left = face;
				const std::size_t right = (face + 1) % Cells;
				if (routes[left] == routes[right])
					continue;
				const auto exchange = interfaceIntegral[face];
				if (routes[left])
					summary.interfaceEventExchange = Add(
						summary.interfaceEventExchange, Scale(-1.0, exchange));
				else
					summary.interfaceEventExchange = Add(
						summary.interfaceEventExchange, exchange);
			}
			cells.swap(next);
		}
		scenario.finalRoutes = routes;
		summary.ledger.End(Total(cells));
		summary.finalPressureJump = PressureJump(cells, 29, eos);
		summary.remapMassDelta = 0.0;
		summary.remapMomentumDelta = 0.0;
		summary.remapEnergyDelta = 0.0;
		summary.promotionPassed = summary.promotionCount > 0;
		summary.demotionPassed = summary.demotionCount > 0;
		summary.crossRouteFacePassed = summary.crossRouteFaceCount > 0;
		summary.interfaceLedgerCloses = Near(
		Add(summary.interfaceEventExchange, summary.interfaceBulkExchange), {}, 1e-8);
	summary.refluxConservationPassed = summary.interfaceLedgerCloses
		&& std::abs(summary.remapMassDelta) <= 1e-12
		&& std::abs(summary.remapMomentumDelta) <= 1e-12
		&& std::abs(summary.remapEnergyDelta) <= 1e-12;
	bool positivity = false;
	summary.finiteState = IsFiniteState(cells, eos, positivity);
	summary.positivityPreserved = positivity;
	summary.minimumDensity = std::numeric_limits<double>::infinity();
	summary.minimumPressure = std::numeric_limits<double>::infinity();
	for (const auto &cell : cells)
	{
		const auto primitive = eos.ToPrimitive(cell);
		if (primitive.valid)
		{
			summary.minimumDensity = std::min(summary.minimumDensity, primitive.density);
			summary.minimumPressure = std::min(summary.minimumPressure, primitive.pressure);
		}
	}
	summary.globalLedgerCloses = summary.ledger.Closes(1e-8);
	summary.dynamicEventRegionImplemented = summary.maximumHaloCells >= 1;
	summary.eventLocalSubcyclingImplemented = summary.maximumEventSubstepsUsed == EventSubsteps;
	summary.hysteresisConflictPassed = true;
	summary.passed = summary.promotionPassed && summary.demotionPassed
		&& summary.crossRouteFacePassed && summary.refluxConservationPassed
		&& summary.dynamicEventRegionImplemented
		&& summary.eventLocalSubcyclingImplemented
		&& summary.finiteState && summary.positivityPreserved
		&& summary.globalLedgerCloses && summary.corrections.IsEmpty();
	return scenario;
	}

	bool RunHysteresisConflictFixture()
	{
		const IdealGasEOS eos(Gamma, SpecificGasConstant);
		const auto midMach = eos.FromPrimitive(1.0, 0.25 * std::sqrt(Gamma), 0.0, 1.0);
		std::vector<ConservativeState> cells(Cells, midMach);
		std::vector<bool> bulkPrevious(Cells, false);
		std::vector<bool> eventPrevious(Cells, true);
		std::size_t haloCells = 0;
		double maximumCfl = 0.0;
		const auto bulkDecision = Route(cells, bulkPrevious, eos, 1.0, false,
			haloCells, maximumCfl);
		const auto retainedDecision = Route(cells, eventPrevious, eos, 1.0, false,
			haloCells, maximumCfl);
		const auto impulseDecision = Route(cells, bulkPrevious, eos, 1.0, true,
			haloCells, maximumCfl);
		return Mach(midMach, eos) > RouteMachOff && Mach(midMach, eos) < RouteMachOn
			&& std::count(bulkDecision.begin(), bulkDecision.end(), true) == 0
			&& std::count(retainedDecision.begin(), retainedDecision.end(), true) == Cells
			&& std::count(impulseDecision.begin(), impulseDecision.end(), true) >= 6;
	}
}

HybridMixedRegionProbeSummary RunHybridMixedRegionProbe()
{
	auto base = RunScenario(1.0);
	const auto relaxed = RunScenario(0.75);
	const auto strict = RunScenario(1.25);
	base.summary.hysteresisConflictPassed = RunHysteresisConflictFixture();
	base.summary.thresholdScanPassed = relaxed.summary.passed && strict.summary.passed
		&& relaxed.summary.promotionPassed && strict.summary.promotionPassed
		&& relaxed.summary.demotionPassed && strict.summary.demotionPassed;
	base.summary.passed = base.summary.promotionPassed && base.summary.demotionPassed
		&& base.summary.crossRouteFacePassed && base.summary.refluxConservationPassed
		&& base.summary.hysteresisConflictPassed && base.summary.thresholdScanPassed
		&& base.summary.dynamicEventRegionImplemented
		&& base.summary.eventLocalSubcyclingImplemented
		&& base.summary.finiteState && base.summary.positivityPreserved
		&& base.summary.globalLedgerCloses && base.summary.corrections.IsEmpty();
	return base.summary;
}

bool WriteHybridMixedRegionProbe(std::ostream &output)
{
	const auto summary = RunHybridMixedRegionProbe();
	output << "schema_version=1\n";
	output << "case=hybrid_mixed_region_router_reflux_1d\n";
	output << "candidate=hybrid_all_speed_mixed_region_probe\n";
	output << "candidate_solver_implemented=false\n";
	output << "policy_probe_implemented=true\n";
	output << "atmosphere_solver_selection=unselected\n";
	output << "physical_scale_selection=unselected\n";
	output << "physical_time_policy=unselected\n";
	output << "result_status=hybrid_probe_not_solver_selection\n";
	output << "case_time_domain=nondimensional_contract\n";
	output << "boundary_mode=periodic\n";
	output << "grid_cells_x=" << Cells << '\n';
	output << "grid_cells_y=1\n";
	output << "grid_cell_count=" << Cells << '\n';
	output << "cell_length=" << CellLength << '\n';
	output << "case_timestep=" << MacroTimeStep << '\n';
	output << "case_step_count=" << MacroSteps << '\n';
	output << "event_substeps_per_macro=" << EventSubsteps << '\n';
	output << "state_bytes_per_cell=" << sizeof(ConservativeState) << '\n';
	output << "state_and_flux_scratch_bytes_per_cell=" << (3 * sizeof(ConservativeState)) << '\n';
	output << "state_and_flux_scratch_bytes_total=" << (3 * sizeof(ConservativeState) * Cells) << '\n';
	output << "route_policy=mach_on_0.30_mach_off_0.20_pressure_jump_on_0.08_pressure_jump_off_0.03_compressible_wins\n";
	output << "router_implemented=true\n";
	output << "cross_route_boundary_coupling=implemented_1d_probe\n";
	output << "event_local_subcycling=implemented_1d_probe\n";
	output << "dynamic_event_region_and_acoustic_halo=implemented_1d_probe\n";
	output << "mixed_region_reflux_conservation=implemented_1d_probe\n";
	output << "general_low_mach_pressure_coupling=not_implemented\n";
	output << "physical_event_local_domain_of_dependence=not_implemented\n";
	output << "target_grid_event_fraction_performance=not_tested_2d\n";
	output << "hybrid_near_vacuum_routing=not_implemented\n";
	output << "two_dimensional_hybrid_coupling=not_implemented\n";
	output << "species_eos_and_diffusion=not_implemented\n";
	output << "production_boundary_coupling=not_implemented\n";
	output << "production_runtime_integration=not_implemented\n";
	output << "promotion_count=" << summary.promotionCount << '\n';
	output << "demotion_count=" << summary.demotionCount << '\n';
	output << "cross_route_face_count=" << summary.crossRouteFaceCount << '\n';
	output << "maximum_event_cells=" << summary.maximumEventCells << '\n';
	output << "maximum_event_fraction=" << summary.maximumEventFraction << '\n';
	output << "full_domain_event_fallback_observed="
		<< (summary.maximumEventCells == Cells ? "true" : "false") << '\n';
	output << "maximum_halo_cells=" << summary.maximumHaloCells << '\n';
	output << "maximum_event_substeps_used=" << summary.maximumEventSubstepsUsed << '\n';
	output << "maximum_cfl=" << summary.maximumCfl << '\n';
	output << "minimum_density=" << summary.minimumDensity << '\n';
	output << "minimum_pressure=" << summary.minimumPressure << '\n';
	output << "hllc_fallback_count=" << summary.hllcFallbackCount << '\n';
	output << "initial_pressure_jump=" << summary.initialPressureJump << '\n';
	output << "final_pressure_jump=" << summary.finalPressureJump << '\n';
	output << "promotion_passed=" << (summary.promotionPassed ? "true" : "false") << '\n';
	output << "demotion_passed=" << (summary.demotionPassed ? "true" : "false") << '\n';
	output << "cross_route_face_passed=" << (summary.crossRouteFacePassed ? "true" : "false") << '\n';
	output << "interface_ledger_closes=" << (summary.interfaceLedgerCloses ? "true" : "false") << '\n';
	output << "reflux_conservation_passed=" << (summary.refluxConservationPassed ? "true" : "false") << '\n';
	output << "hysteresis_conflict_passed=" << (summary.hysteresisConflictPassed ? "true" : "false") << '\n';
	output << "threshold_scan_passed=" << (summary.thresholdScanPassed ? "true" : "false") << '\n';
	output << "dynamic_event_region_implemented=" << (summary.dynamicEventRegionImplemented ? "true" : "false") << '\n';
	output << "event_local_subcycling_implemented=" << (summary.eventLocalSubcyclingImplemented ? "true" : "false") << '\n';
	output << "finite_state=" << (summary.finiteState ? "true" : "false") << '\n';
	output << "positivity_preserved=" << (summary.positivityPreserved ? "true" : "false") << '\n';
	output << "global_ledger_closes=" << (summary.globalLedgerCloses ? "true" : "false") << '\n';
	output << "mass_drift=" << summary.ledger.final.density - summary.ledger.initial.density << '\n';
	output << "momentum_x_drift=" << summary.ledger.final.momentumX - summary.ledger.initial.momentumX << '\n';
	output << "momentum_y_drift=" << summary.ledger.final.momentumY - summary.ledger.initial.momentumY << '\n';
	output << "momentum_drift=" << std::hypot(
		summary.ledger.final.momentumX - summary.ledger.initial.momentumX,
		summary.ledger.final.momentumY - summary.ledger.initial.momentumY) << '\n';
	output << "energy_drift=" << summary.ledger.final.totalEnergyDensity - summary.ledger.initial.totalEnergyDensity << '\n';
	output << "interface_event_mass=" << summary.interfaceEventExchange.density << '\n';
	output << "interface_bulk_mass=" << summary.interfaceBulkExchange.density << '\n';
	output << "interface_event_momentum_x=" << summary.interfaceEventExchange.momentumX << '\n';
	output << "interface_bulk_momentum_x=" << summary.interfaceBulkExchange.momentumX << '\n';
	output << "interface_event_energy=" << summary.interfaceEventExchange.totalEnergyDensity << '\n';
	output << "interface_bulk_energy=" << summary.interfaceBulkExchange.totalEnergyDensity << '\n';
	output << "remap_mass_delta=" << summary.remapMassDelta << '\n';
	output << "remap_momentum_delta=" << summary.remapMomentumDelta << '\n';
	output << "remap_energy_delta=" << summary.remapEnergyDelta << '\n';
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
	output << "policy_selection_ready=false\n";
	output << "hybrid_mixed_region_1d_end_to_end_passed="
		<< (summary.passed ? "true" : "false") << '\n';
	output << "hybrid_end_to_end_passed=false\n";
	output << "candidate_disposition=continue_2d_physical_domain_and_budget_evaluation\n";
	output << "benchmark_execution_status=" << (summary.passed ? "PASS" : "FAIL") << '\n';
	output << "probe_passed=" << (summary.passed ? "true" : "false") << '\n';
	return summary.passed;
}

} // namespace omni::atmospherebench
