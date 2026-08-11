#include "Species2D.h"
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
	constexpr double TimeStep = 0.1;
	constexpr std::size_t Steps = 320;
	constexpr double VelocityX = 0.2;
	constexpr double VelocityY = 0.1;
	constexpr double MixedFractionTolerance = 1e-12;
	constexpr double CompositionVariationDecreaseTolerance = 1e-6;

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
		ConservativeState result;
		for (const auto &cell : cells)
			result = Add(result, cell);
		return result;
	}

	double TotalSpecies(const std::vector<double> &partialDensity)
	{
		double result = 0.0;
		for (const double value : partialDensity)
			result += value;
		return result;
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

	double SpeciesFraction(double partialDensity, const ConservativeState &gas)
	{
		if (!std::isfinite(partialDensity) || !std::isfinite(gas.density) || gas.density <= 0.0)
			return std::numeric_limits<double>::quiet_NaN();
		return partialDensity / gas.density;
	}

	double UpwindSpeciesFlux(
		double massFlux,
		double leftPartialDensity,
		const ConservativeState &leftGas,
		double rightPartialDensity,
		const ConservativeState &rightGas)
	{
		const double leftFraction = SpeciesFraction(leftPartialDensity, leftGas);
		const double rightFraction = SpeciesFraction(rightPartialDensity, rightGas);
		if (!std::isfinite(leftFraction) || !std::isfinite(rightFraction))
			return std::numeric_limits<double>::quiet_NaN();
		return massFlux * (massFlux >= 0.0 ? leftFraction : rightFraction);
	}

	double CompositionTotalVariation(
		const std::vector<double> &partialDensity,
		const std::vector<ConservativeState> &gas)
	{
		if (partialDensity.size() != gas.size() || gas.size() != CellsX * CellsY)
			return std::numeric_limits<double>::quiet_NaN();
		double result = 0.0;
		for (std::size_t y = 0; y < CellsY; ++y)
		{
			for (std::size_t x = 0; x < CellsX; ++x)
			{
				const std::size_t index = y * CellsX + x;
				const std::size_t right = y * CellsX + (x + 1) % CellsX;
				const std::size_t top = ((y + 1) % CellsY) * CellsX + x;
				const double fraction = SpeciesFraction(partialDensity[index], gas[index]);
				const double rightFraction = SpeciesFraction(partialDensity[right], gas[right]);
				const double topFraction = SpeciesFraction(partialDensity[top], gas[top]);
				if (!std::isfinite(fraction) || !std::isfinite(rightFraction)
					|| !std::isfinite(topFraction))
					return std::numeric_limits<double>::quiet_NaN();
				result += std::abs(rightFraction - fraction) + std::abs(topFraction - fraction);
			}
		}
		return result;
	}

	std::size_t MixedCellCount(
		const std::vector<double> &partialDensity,
		const std::vector<ConservativeState> &gas)
	{
		std::size_t result = 0;
		for (std::size_t index = 0; index < gas.size(); ++index)
		{
			const double fraction = SpeciesFraction(partialDensity[index], gas[index]);
			if (std::isfinite(fraction) && fraction > MixedFractionTolerance
				&& fraction < 1.0 - MixedFractionTolerance)
				++result;
		}
		return result;
	}
} // namespace

Hllc2DSpeciesMixingSummary RunHllc2DSpeciesMixing()
{
	const BenchmarkCase benchmarkCase{
		"hllc_passive_species_mixing_2d",
		{CellsX, CellsY, CellLength, BoundaryMode::Periodic},
		TimeDomain::NondimensionalContract,
		TimeStep,
		Steps,
	};
	Hllc2DSpeciesMixingSummary summary{benchmarkCase};
	const IdealGasEOS eos(Gamma, SpecificGasConstant);
	if (!benchmarkCase.IsValid() || !eos.IsValid())
		return summary;
	const auto uniformGas = eos.FromPrimitive(1.0, VelocityX, VelocityY, 1.0);
	std::vector<ConservativeState> initialGas(CellsX * CellsY, uniformGas);
	std::vector<ConservativeState> gas = initialGas;
	std::vector<ConservativeState> nextGas(gas.size());
	std::vector<ConservativeState> gasFluxX(gas.size());
	std::vector<ConservativeState> gasFluxY(gas.size());
	std::vector<double> initialSpeciesA(gas.size());
	for (std::size_t y = 0; y < CellsY; ++y)
	{
		for (std::size_t x = 0; x < CellsX; ++x)
			initialSpeciesA[y * CellsX + x] = x < CellsX / 2 ? uniformGas.density : 0.0;
	}
	std::vector<double> speciesA = initialSpeciesA;
	std::vector<double> nextSpeciesA(speciesA.size());
	std::vector<double> speciesFluxX(speciesA.size());
	std::vector<double> speciesFluxY(speciesA.size());
	const std::size_t storedBytes = 5 * gas.size() * sizeof(ConservativeState)
		+ 5 * speciesA.size() * sizeof(double);
	summary.stateAndFluxScratchBytesTotal = storedBytes;
	summary.stateAndFluxScratchBytesPerCell = static_cast<double>(storedBytes)
		/ static_cast<double>(gas.size());
	summary.gasLedger.Begin(TotalState(gas));
	summary.initialSpeciesAMass = TotalSpecies(speciesA);
	summary.initialSpeciesBMass = summary.gasLedger.initial.density - summary.initialSpeciesAMass;
	summary.initialCompositionTotalVariation = CompositionTotalVariation(speciesA, gas);
	summary.initialMixedCellCount = MixedCellCount(speciesA, gas);
	summary.minimumDensity = std::numeric_limits<double>::infinity();
	summary.minimumPressure = std::numeric_limits<double>::infinity();
	summary.minimumSpeciesAFraction = std::numeric_limits<double>::infinity();
	summary.maximumSpeciesAFraction = -std::numeric_limits<double>::infinity();
	const double lambda = TimeStep / CellLength;
	for (std::size_t step = 0; step < Steps; ++step)
	{
		for (std::size_t y = 0; y < CellsY; ++y)
		{
			for (std::size_t x = 0; x < CellsX; ++x)
			{
				const std::size_t index = y * CellsX + x;
				const std::size_t right = y * CellsX + (x + 1) % CellsX;
				const std::size_t top = ((y + 1) % CellsY) * CellsX + x;
				const auto xResult = ComputeHllcRusanovFallbackFluxX(gas[index], gas[right], eos);
				const auto yResult = FluxY(gas[index], gas[top], eos);
				if (!xResult.valid || !yResult.valid)
					return summary;
				gasFluxX[index] = xResult.flux;
				gasFluxY[index] = yResult.flux;
				speciesFluxX[index] = UpwindSpeciesFlux(
					xResult.flux.density, speciesA[index], gas[index], speciesA[right], gas[right]);
				speciesFluxY[index] = UpwindSpeciesFlux(
					yResult.flux.density, speciesA[index], gas[index], speciesA[top], gas[top]);
				if (!std::isfinite(speciesFluxX[index]) || !std::isfinite(speciesFluxY[index]))
					return summary;
				summary.fluxFallbackCount += static_cast<std::size_t>(xResult.usedFallback)
					+ static_cast<std::size_t>(yResult.usedFallback);
			}
		}
		for (std::size_t y = 0; y < CellsY; ++y)
		{
			for (std::size_t x = 0; x < CellsX; ++x)
			{
				const std::size_t index = y * CellsX + x;
				const std::size_t left = y * CellsX + (x + CellsX - 1) % CellsX;
				const std::size_t bottom = ((y + CellsY - 1) % CellsY) * CellsX + x;
				nextGas[index] = Subtract(gas[index], Scale(lambda,
					Add(Subtract(gasFluxX[index], gasFluxX[left]),
						Subtract(gasFluxY[index], gasFluxY[bottom]))));
				nextSpeciesA[index] = speciesA[index] - lambda
					* ((speciesFluxX[index] - speciesFluxX[left])
						+ (speciesFluxY[index] - speciesFluxY[bottom]));
				const auto primitive = eos.ToPrimitive(nextGas[index]);
				const double fraction = SpeciesFraction(nextSpeciesA[index], nextGas[index]);
				if (!primitive.valid || !std::isfinite(fraction))
					return summary;
				summary.minimumDensity = std::min(summary.minimumDensity, primitive.density);
				summary.minimumPressure = std::min(summary.minimumPressure, primitive.pressure);
				summary.minimumSpeciesAFraction = std::min(summary.minimumSpeciesAFraction, fraction);
				summary.maximumSpeciesAFraction = std::max(summary.maximumSpeciesAFraction, fraction);
				summary.maximumCfl = std::max(summary.maximumCfl, lambda
					* (std::abs(primitive.velocityX) + std::abs(primitive.velocityY)
						+ 2.0 * primitive.soundSpeed));
			}
		}
		gas.swap(nextGas);
		speciesA.swap(nextSpeciesA);
	}
	summary.gasLedger.End(TotalState(gas));
	summary.finalSpeciesAMass = TotalSpecies(speciesA);
	summary.finalSpeciesBMass = summary.gasLedger.final.density - summary.finalSpeciesAMass;
	summary.speciesAMassDrift = summary.finalSpeciesAMass - summary.initialSpeciesAMass;
	summary.speciesBMassDrift = summary.finalSpeciesBMass - summary.initialSpeciesBMass;
	summary.finalCompositionTotalVariation = CompositionTotalVariation(speciesA, gas);
	summary.finalMixedCellCount = MixedCellCount(speciesA, gas);
	for (std::size_t index = 0; index < speciesA.size(); ++index)
		summary.compositionStateChangeL1 += std::abs(speciesA[index] - initialSpeciesA[index]);
	summary.gasLedgerCloses = summary.gasLedger.Closes(1e-10);
	summary.speciesLedgerCloses = std::abs(summary.speciesAMassDrift) <= 1e-10
		&& std::abs(summary.speciesBMassDrift) <= 1e-10;
	summary.speciesBoundsPreserved = summary.minimumSpeciesAFraction >= -1e-12
		&& summary.maximumSpeciesAFraction <= 1.0 + 1e-12;
	summary.compositionEvolved = summary.compositionStateChangeL1 > 1e-6;
	summary.mixedRegionFormed = summary.initialMixedCellCount == 0
		&& summary.finalMixedCellCount > 0
		&& summary.finalCompositionTotalVariation
			<= summary.initialCompositionTotalVariation - CompositionVariationDecreaseTolerance;
	summary.passed = summary.gasLedgerCloses && summary.speciesLedgerCloses
		&& summary.speciesBoundsPreserved && summary.compositionEvolved
		&& summary.mixedRegionFormed && summary.corrections.IsEmpty()
		&& summary.maximumCfl <= 1.0 && summary.minimumDensity > 0.0
		&& summary.minimumPressure > 0.0 && summary.fluxFallbackCount == 0;
	return summary;
}

bool WriteHllc2DSpeciesMixingProbe(std::ostream &output)
{
	const auto summary = RunHllc2DSpeciesMixing();
	const auto &initial = summary.gasLedger.initial;
	const auto &final = summary.gasLedger.final;
	output << "schema_version=1\n";
	output << "case=" << summary.benchmarkCase.id << '\n';
	output << "candidate=fvm_hllc_rusanov_fallback\n";
	output << "candidate_solver_implemented=true\n";
	output << "atmosphere_solver_selection=unselected\n";
	output << "physical_scale_selection=unselected\n";
	output << "result_status=candidate_result_not_selection\n";
	output << "case_time_domain=nondimensional_contract\n";
	output << "dimension=2\n";
	output << "boundary_mode=periodic\n";
	output << "species_model=passive_conserved_binary_fixture\n";
	output << "species_eos_coupling=not_implemented\n";
	output << "physical_diffusion=not_implemented\n";
	output << "grid_cells_x=" << summary.benchmarkCase.grid.cellsX << '\n';
	output << "grid_cells_y=" << summary.benchmarkCase.grid.cellsY << '\n';
	output << "grid_cell_count=" << summary.benchmarkCase.grid.CellCount() << '\n';
	output << "cell_length=" << summary.benchmarkCase.grid.cellLength << '\n';
	output << "case_timestep=" << summary.benchmarkCase.timeStep << '\n';
	output << "case_step_count=" << summary.benchmarkCase.stepCount << '\n';
	output << "maximum_cfl=" << summary.maximumCfl << '\n';
	output << "minimum_density=" << summary.minimumDensity << '\n';
	output << "minimum_pressure=" << summary.minimumPressure << '\n';
	output << "initial_species_a_mass=" << summary.initialSpeciesAMass << '\n';
	output << "final_species_a_mass=" << summary.finalSpeciesAMass << '\n';
	output << "initial_species_b_mass=" << summary.initialSpeciesBMass << '\n';
	output << "final_species_b_mass=" << summary.finalSpeciesBMass << '\n';
	output << "species_a_mass_drift=" << summary.speciesAMassDrift << '\n';
	output << "species_b_mass_drift=" << summary.speciesBMassDrift << '\n';
	output << "minimum_species_a_fraction=" << summary.minimumSpeciesAFraction << '\n';
	output << "maximum_species_a_fraction=" << summary.maximumSpeciesAFraction << '\n';
	output << "initial_composition_total_variation="
		<< summary.initialCompositionTotalVariation << '\n';
	output << "final_composition_total_variation="
		<< summary.finalCompositionTotalVariation << '\n';
	output << "composition_state_change_l1=" << summary.compositionStateChangeL1 << '\n';
	output << "initial_mixed_cell_count=" << summary.initialMixedCellCount << '\n';
	output << "final_mixed_cell_count=" << summary.finalMixedCellCount << '\n';
	output << "gas_ledger_closes=" << (summary.gasLedgerCloses ? "true" : "false") << '\n';
	output << "species_ledger_closes="
		<< (summary.speciesLedgerCloses ? "true" : "false") << '\n';
	output << "species_bounds_preserved="
		<< (summary.speciesBoundsPreserved ? "true" : "false") << '\n';
	output << "composition_evolved=" << (summary.compositionEvolved ? "true" : "false") << '\n';
	output << "mixed_region_formed=" << (summary.mixedRegionFormed ? "true" : "false") << '\n';
	output << "mass_drift=" << (final.density - initial.density) << '\n';
	output << "momentum_x_drift=" << (final.momentumX - initial.momentumX) << '\n';
	output << "momentum_y_drift=" << (final.momentumY - initial.momentumY) << '\n';
	output << "momentum_drift=" << std::hypot(
		final.momentumX - initial.momentumX, final.momentumY - initial.momentumY) << '\n';
	output << "energy_drift=" << (final.totalEnergyDensity - initial.totalEnergyDensity) << '\n';
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
	output << "state_bytes_per_cell=" << sizeof(ConservativeState) + sizeof(double) << '\n';
	output << "state_and_flux_scratch_bytes_per_cell="
		<< summary.stateAndFluxScratchBytesPerCell << '\n';
	output << "state_and_flux_scratch_bytes_total="
		<< summary.stateAndFluxScratchBytesTotal << '\n';
	output << "probe_passed=" << (summary.passed ? "true" : "false") << '\n';
	return summary.passed;
}

} // namespace omni::atmospherebench
