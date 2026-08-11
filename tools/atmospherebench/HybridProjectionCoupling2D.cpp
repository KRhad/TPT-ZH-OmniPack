#include "HybridProjectionCoupling2D.h"

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
	constexpr std::size_t CellsX = 32;
	constexpr std::size_t CellsY = 24;
	constexpr double Gamma = 5.0 / 3.0;
	constexpr double SpecificGasConstant = 1.0;
	constexpr double CellLength = 1.0;
	constexpr double EventTimeStep = 0.001;
	constexpr double ProjectionTimeStep = 0.1;
	constexpr double Pi = 3.141592653589793238462643383279502884;
	constexpr double DivergenceReductionTarget = 0.02;
	constexpr std::size_t MaximumProjectionIterations = 2000;

	std::size_t Index(std::size_t x, std::size_t y)
	{
		return y * CellsX + x;
	}

	ConservativeState Add(const ConservativeState &left, const ConservativeState &right)
	{
		return {left.density + right.density, left.momentumX + right.momentumX,
			left.momentumY + right.momentumY,
			left.totalEnergyDensity + right.totalEnergyDensity};
	}

	ConservativeState Scale(double factor, const ConservativeState &state)
	{
		return {factor * state.density, factor * state.momentumX,
			factor * state.momentumY, factor * state.totalEnergyDensity};
	}

	ConservativeState Total(const std::vector<ConservativeState> &states)
	{
		ConservativeState total;
		for (const auto &state : states)
			total = Add(total, state);
		return total;
	}

	void RemoveMean(std::vector<double> &values)
	{
		double sum = 0.0;
		for (const double value : values)
			sum += value;
		const double mean = sum / static_cast<double>(values.size());
		for (double &value : values)
			value -= mean;
	}

	double Divergence(const std::vector<double> &velocityX,
		const std::vector<double> &velocityY, bool periodic,
		std::vector<double> &output)
	{
		double sum = 0.0;
		for (std::size_t y = 0; y < CellsY; ++y)
			for (std::size_t x = 0; x < CellsX; ++x)
			{
				const auto index = Index(x, y);
				const double right = periodic || x + 1 < CellsX ? velocityX[index] : 0.0;
				const double left = x > 0 ? velocityX[Index(x - 1, y)]
					: periodic ? velocityX[Index(CellsX - 1, y)] : 0.0;
				const double top = periodic || y + 1 < CellsY ? velocityY[index] : 0.0;
				const double bottom = y > 0 ? velocityY[Index(x, y - 1)]
					: periodic ? velocityY[Index(x, CellsY - 1)] : 0.0;
				const double value = (right - left + top - bottom) / CellLength;
				output[index] = value;
				sum += value * value;
			}
		return std::sqrt(sum / static_cast<double>(output.size()));
	}

	void ApplyVariableOperator(const std::vector<double> &pressure,
		const std::vector<double> &density, bool periodic, std::vector<double> &output)
	{
		for (std::size_t y = 0; y < CellsY; ++y)
			for (std::size_t x = 0; x < CellsX; ++x)
			{
				const auto index = Index(x, y);
				double value = 0.0;
				if (periodic || x + 1 < CellsX)
				{
					const auto right = Index((x + 1) % CellsX, y);
					value += (pressure[index] - pressure[right]) / density[index];
				}
				if (periodic || x > 0)
				{
					const auto left = Index((x + CellsX - 1) % CellsX, y);
					value += (pressure[index] - pressure[left]) / density[left];
				}
				if (periodic || y + 1 < CellsY)
				{
					const auto top = Index(x, (y + 1) % CellsY);
					value += (pressure[index] - pressure[top]) / density[index];
				}
				if (periodic || y > 0)
				{
					const auto bottom = Index(x, (y + CellsY - 1) % CellsY);
					value += (pressure[index] - pressure[bottom]) / density[bottom];
				}
				output[index] = value / (CellLength * CellLength);
			}
	}

	std::size_t Project(std::vector<double> &velocityX, std::vector<double> &velocityY,
		const std::vector<double> &density, bool periodic, double &initialDivergence,
		double &finalDivergence)
	{
		std::vector<double> divergence(CellsX * CellsY);
		std::vector<double> pressure(CellsX * CellsY, 0.0);
		std::vector<double> residual(CellsX * CellsY);
		std::vector<double> direction(CellsX * CellsY);
		std::vector<double> operatorValue(CellsX * CellsY);
		initialDivergence = Divergence(velocityX, velocityY, periodic, divergence);
		for (std::size_t index = 0; index < divergence.size(); ++index)
		{
			residual[index] = -divergence[index] / ProjectionTimeStep;
			direction[index] = residual[index];
		}
		RemoveMean(residual);
		direction = residual;
		double residualSquared = 0.0;
		for (const double value : residual)
			residualSquared += value * value;
		std::size_t iterations = 0;
		for (; iterations < MaximumProjectionIterations; ++iterations)
		{
			ApplyVariableOperator(direction, density, periodic, operatorValue);
			double directionOperator = 0.0;
			for (std::size_t index = 0; index < direction.size(); ++index)
				directionOperator += direction[index] * operatorValue[index];
			if (!(directionOperator > 0.0) || !std::isfinite(directionOperator))
				break;
			const double alpha = residualSquared / directionOperator;
			for (std::size_t index = 0; index < pressure.size(); ++index)
			{
				pressure[index] += alpha * direction[index];
				residual[index] -= alpha * operatorValue[index];
			}
			RemoveMean(pressure);
			RemoveMean(residual);
			double nextResidualSquared = 0.0;
			for (const double value : residual)
				nextResidualSquared += value * value;
			if (std::sqrt(nextResidualSquared / residual.size()) * ProjectionTimeStep
				/ initialDivergence < DivergenceReductionTarget * 0.5)
			{
				++iterations;
				break;
			}
			const double beta = nextResidualSquared / residualSquared;
			for (std::size_t index = 0; index < direction.size(); ++index)
				direction[index] = residual[index] + beta * direction[index];
			residualSquared = nextResidualSquared;
		}
		for (std::size_t y = 0; y < CellsY; ++y)
			for (std::size_t x = 0; x < CellsX; ++x)
			{
				const auto index = Index(x, y);
				if (periodic || x + 1 < CellsX)
				{
					const auto right = Index((x + 1) % CellsX, y);
					velocityX[index] -= ProjectionTimeStep
						* (pressure[right] - pressure[index])
						/ (density[index] * CellLength);
				}
				else
					velocityX[index] = 0.0;
				if (periodic || y + 1 < CellsY)
				{
					const auto top = Index(x, (y + 1) % CellsY);
					velocityY[index] -= ProjectionTimeStep
						* (pressure[top] - pressure[index])
						/ (density[index] * CellLength);
				}
				else
					velocityY[index] = 0.0;
			}
		finalDivergence = Divergence(velocityX, velocityY, periodic, divergence);
		return iterations;
	}

	NumericalFluxResult FluxY(const ConservativeState &bottom,
		const ConservativeState &top, const IdealGasEOS &eos)
	{
		const ConservativeState rotatedBottom{bottom.density, bottom.momentumY,
			bottom.momentumX, bottom.totalEnergyDensity};
		const ConservativeState rotatedTop{top.density, top.momentumY,
			top.momentumX, top.totalEnergyDensity};
		auto result = ComputeHllcRusanovFallbackFluxX(rotatedBottom, rotatedTop, eos);
		std::swap(result.flux.momentumX, result.flux.momentumY);
		return result;
	}

	bool IsEvent(std::size_t x, std::size_t y)
	{
		return x >= CellsX / 2 - 2 && x <= CellsX / 2 + 1
			&& y >= CellsY / 2 - 2 && y <= CellsY / 2 + 1;
	}
}

HybridProjectionCoupling2DSummary RunHybridProjectionCoupling2D()
{
	HybridProjectionCoupling2DSummary summary;
	summary.cellsX = CellsX;
	summary.cellsY = CellsY;
	const IdealGasEOS eos(Gamma, SpecificGasConstant);
	std::vector<ConservativeState> states(CellsX * CellsY);
	std::vector<bool> event(CellsX * CellsY, false);
	for (std::size_t y = 0; y < CellsY; ++y)
		for (std::size_t x = 0; x < CellsX; ++x)
		{
			const double density = 1.0 + 0.12
				* std::sin(2.0 * Pi * (static_cast<double>(x) + 0.5) / CellsX)
				* std::sin(2.0 * Pi * (static_cast<double>(y) + 0.5) / CellsY);
			const double velocityX = 0.045 * std::sin(
				2.0 * Pi * (static_cast<double>(x) + 0.5) / CellsX);
			const double velocityY = 0.035 * std::cos(
				2.0 * Pi * (static_cast<double>(y) + 0.5) / CellsY);
			const bool routed = IsEvent(x, y);
			event[Index(x, y)] = routed;
			states[Index(x, y)] = eos.FromPrimitive(density, velocityX, velocityY,
				routed ? 1.35 : 1.0);
		}
	const auto initial = Total(states);
	summary.ledger.Begin(initial);
	const auto beforeEvent = states;
	std::vector<ConservativeState> delta(states.size());
	for (std::size_t y = 0; y < CellsY; ++y)
		for (std::size_t x = 0; x < CellsX; ++x)
		{
			const auto index = Index(x, y);
			const auto right = Index((x + 1) % CellsX, y);
			const auto top = Index(x, (y + 1) % CellsY);
			if (event[index] || event[right])
			{
				const auto flux = ComputeHllcRusanovFallbackFluxX(states[index], states[right], eos);
				if (!flux.valid)
					return summary;
				const auto exchange = Scale(EventTimeStep / CellLength, flux.flux);
				delta[index] = Add(delta[index], Scale(-1.0, exchange));
				delta[right] = Add(delta[right], exchange);
				summary.hllcFallbackCount += static_cast<std::size_t>(flux.usedFallback);
				if (event[index] != event[right])
					++summary.crossRouteFaceCount;
			}
			if (event[index] || event[top])
			{
				const auto flux = FluxY(states[index], states[top], eos);
				if (!flux.valid)
					return summary;
				const auto exchange = Scale(EventTimeStep / CellLength, flux.flux);
				delta[index] = Add(delta[index], Scale(-1.0, exchange));
				delta[top] = Add(delta[top], exchange);
				summary.hllcFallbackCount += static_cast<std::size_t>(flux.usedFallback);
				if (event[index] != event[top])
					++summary.crossRouteFaceCount;
			}
		}
	for (std::size_t index = 0; index < states.size(); ++index)
		states[index] = Add(states[index], delta[index]);

	std::vector<double> density(states.size());
	std::vector<double> velocityX(states.size());
	std::vector<double> velocityY(states.size());
	summary.minimumDensity = std::numeric_limits<double>::infinity();
	summary.maximumDensity = 0.0;
	for (std::size_t index = 0; index < states.size(); ++index)
	{
		const auto primitive = eos.ToPrimitive(states[index]);
		if (!primitive.valid)
			return summary;
		density[index] = primitive.density;
		velocityX[index] = primitive.velocityX;
		velocityY[index] = primitive.velocityY;
		summary.minimumDensity = std::min(summary.minimumDensity, primitive.density);
		summary.maximumDensity = std::max(summary.maximumDensity, primitive.density);
	}
	summary.projectionIterations = Project(velocityX, velocityY, density, true,
		summary.initialDivergenceL2, summary.finalDivergenceL2);
	summary.divergenceReductionRatio = summary.finalDivergenceL2
		/ summary.initialDivergenceL2;
	summary.minimumPressure = std::numeric_limits<double>::infinity();
	for (std::size_t index = 0; index < states.size(); ++index)
	{
		states[index].momentumX = states[index].density * velocityX[index];
		states[index].momentumY = states[index].density * velocityY[index];
		const auto primitive = eos.ToPrimitive(states[index]);
		if (!primitive.valid)
			return summary;
		summary.minimumPressure = std::min(summary.minimumPressure, primitive.pressure);
		const double change = std::abs(states[index].density - beforeEvent[index].density)
			+ std::abs(states[index].momentumX - beforeEvent[index].momentumX)
			+ std::abs(states[index].momentumY - beforeEvent[index].momentumY)
			+ std::abs(states[index].totalEnergyDensity
				- beforeEvent[index].totalEnergyDensity);
		if (event[index])
			summary.eventStateChangeL1 += change;
		else
			summary.bulkStateChangeL1 += change;
	}
	summary.ledger.End(Total(states));
	summary.variableDensityPassed = summary.maximumDensity - summary.minimumDensity > 0.1;
	summary.crossRouteFluxPassed = summary.crossRouteFaceCount > 0;
	summary.conservationPassed = summary.ledger.Closes(1e-9);
	summary.positivityPassed = summary.minimumDensity > 0.0 && summary.minimumPressure > 0.0;
	summary.hllcProjectionCouplingPassed = summary.projectionIterations > 0
		&& summary.divergenceReductionRatio < DivergenceReductionTarget
		&& summary.eventStateChangeL1 > 0.0 && summary.bulkStateChangeL1 > 0.0;

	std::vector<double> wallDensity(CellsX * CellsY);
	std::vector<double> wallVelocityX(CellsX * CellsY);
	std::vector<double> wallVelocityY(CellsX * CellsY);
	for (std::size_t y = 0; y < CellsY; ++y)
		for (std::size_t x = 0; x < CellsX; ++x)
		{
			const auto index = Index(x, y);
			wallDensity[index] = 0.85 + 0.3
				* (static_cast<double>(x) + 0.5) / static_cast<double>(CellsX);
			wallVelocityX[index] = x + 1 < CellsX ? 0.04 * std::sin(
				Pi * (static_cast<double>(x) + 0.5) / CellsX) : 0.0;
			wallVelocityY[index] = y + 1 < CellsY ? 0.03 * std::sin(
				Pi * (static_cast<double>(y) + 0.5) / CellsY) : 0.0;
		}
	Project(wallVelocityX, wallVelocityY, wallDensity, false,
		summary.wallInitialDivergenceL2, summary.wallFinalDivergenceL2);
	summary.wallDivergenceReductionRatio = summary.wallFinalDivergenceL2
		/ summary.wallInitialDivergenceL2;
	for (std::size_t y = 0; y < CellsY; ++y)
		summary.wallNormalVelocityMaximum = std::max(summary.wallNormalVelocityMaximum,
			std::abs(wallVelocityX[Index(CellsX - 1, y)]));
	for (std::size_t x = 0; x < CellsX; ++x)
		summary.wallNormalVelocityMaximum = std::max(summary.wallNormalVelocityMaximum,
			std::abs(wallVelocityY[Index(x, CellsY - 1)]));
	summary.wallProjectionPassed = summary.wallDivergenceReductionRatio
		< DivergenceReductionTarget && summary.wallNormalVelocityMaximum <= 1e-14;
	summary.productionSolverImplemented = false;
	summary.passed = summary.variableDensityPassed
		&& summary.hllcProjectionCouplingPassed && summary.crossRouteFluxPassed
		&& summary.conservationPassed && summary.positivityPassed
		&& summary.wallProjectionPassed && summary.corrections.IsEmpty()
		&& !summary.productionSolverImplemented;
	return summary;
}

bool WriteHybridProjectionCoupling2D(std::ostream &output)
{
	const auto summary = RunHybridProjectionCoupling2D();
	output << "schema_version=1\n";
	output << "case=hybrid_projection_hllc_coupling_2d\n";
	output << "candidate=hybrid_fvm_projection_hllc_rusanov_v1\n";
	output << "result_status=coupled_component_fixture_not_production\n";
	output << "grid_cells_x=" << summary.cellsX << '\n';
	output << "grid_cells_y=" << summary.cellsY << '\n';
	output << "variable_density_passed=" << (summary.variableDensityPassed ? "true" : "false") << '\n';
	output << "projection_iteration_count=" << summary.projectionIterations << '\n';
	output << "initial_divergence_l2=" << summary.initialDivergenceL2 << '\n';
	output << "final_divergence_l2=" << summary.finalDivergenceL2 << '\n';
	output << "divergence_reduction_ratio=" << summary.divergenceReductionRatio << '\n';
	output << "minimum_density=" << summary.minimumDensity << '\n';
	output << "maximum_density=" << summary.maximumDensity << '\n';
	output << "minimum_pressure=" << summary.minimumPressure << '\n';
	output << "cross_route_face_count=" << summary.crossRouteFaceCount << '\n';
	output << "hllc_fallback_count=" << summary.hllcFallbackCount << '\n';
	output << "event_state_change_l1=" << summary.eventStateChangeL1 << '\n';
	output << "bulk_state_change_l1=" << summary.bulkStateChangeL1 << '\n';
	output << "cross_route_flux_passed=" << (summary.crossRouteFluxPassed ? "true" : "false") << '\n';
	output << "hllc_projection_coupling_passed=" << (summary.hllcProjectionCouplingPassed ? "true" : "false") << '\n';
	output << "conservation_passed=" << (summary.conservationPassed ? "true" : "false") << '\n';
	output << "mass_drift=" << summary.ledger.final.density
		- summary.ledger.initial.density << '\n';
	output << "momentum_x_drift=" << summary.ledger.final.momentumX
		- summary.ledger.initial.momentumX << '\n';
	output << "momentum_y_drift=" << summary.ledger.final.momentumY
		- summary.ledger.initial.momentumY << '\n';
	output << "energy_drift=" << summary.ledger.final.totalEnergyDensity
		- summary.ledger.initial.totalEnergyDensity << '\n';
	output << "positivity_passed=" << (summary.positivityPassed ? "true" : "false") << '\n';
	output << "wall_initial_divergence_l2=" << summary.wallInitialDivergenceL2 << '\n';
	output << "wall_final_divergence_l2=" << summary.wallFinalDivergenceL2 << '\n';
	output << "wall_divergence_reduction_ratio=" << summary.wallDivergenceReductionRatio << '\n';
	output << "wall_normal_velocity_maximum=" << summary.wallNormalVelocityMaximum << '\n';
	output << "wall_projection_passed=" << (summary.wallProjectionPassed ? "true" : "false") << '\n';
	output << "numerical_correction_count=" << summary.corrections.eventCount << '\n';
	output << "candidate_solver_implemented=false\n";
	output << "production_solver_implemented=false\n";
	output << "production_runtime_integration=not_implemented\n";
	output << "coupling_scope=single_operator_split_fixture_periodic_event_plus_sealed_wall_projection\n";
	output << "coupled_component_fixture_passed=" << (summary.passed ? "true" : "false") << '\n';
	return summary.passed;
}

} // namespace omni::atmospherebench
