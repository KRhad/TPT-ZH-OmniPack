#include "simulation/OmniAtmosphere.h"
#include "simulation/OmniThermal.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <numeric>
#include <vector>

namespace
{
int Fail(const char *message)
{
	std::cerr << "omni_multispecies_probe: FAIL " << message << '\n';
	return 1;
}

bool Close(double left, double right, double relative, double absolute)
{
	return std::abs(left - right) <= absolute + relative * std::max(std::abs(left), std::abs(right));
}

OmniAtmosphereConfig Config(std::size_t width, std::size_t height, OmniAtmosphereBoundary boundary)
{
	OmniAtmosphereConfig config;
	config.width = width;
	config.height = height;
	config.boundary = boundary;
	config.execution = OmniAtmosphereExecution::ReferenceCompressible;
	config.maximumReferenceSubsteps = 100000;
	return config;
}
}

int main()
{
	auto uniformConfig = Config(8, 4, OmniAtmosphereBoundary::Periodic);
	uniformConfig.speciesDiffusion = false;
	uniformConfig.thermalConduction = false;
	uniformConfig.waterPhaseEquilibrium = false;
	OmniAtmosphere uniform(uniformConfig);
	if (uniform.SpeciesCount() != OMNI_COMMON_SPECIES_COUNT)
		return Fail("common species registry size drifted");
	const auto primitive = uniform.Primitive(0, 0);
	double partialPressureSum = 0.0;
	double fractionSum = 0.0;
	for (std::size_t species = 0; species < uniform.SpeciesCount(); ++species)
	{
		partialPressureSum += uniform.SpeciesPartialPressurePa(0, 0, species);
		fractionSum += uniform.SpeciesMassFraction(0, 0, species);
	}
	if (!primitive.finite || !Close(fractionSum, 1.0, 0.0, 1.0e-12) ||
		!Close(partialPressureSum, primitive.pressure, 1.0e-12, 1.0e-8))
		return Fail("Earth-like mixture EOS or partial pressures failed");

	auto transportConfig = Config(32, 1, OmniAtmosphereBoundary::Periodic);
	transportConfig.speciesDiffusion = true;
	transportConfig.thermalConduction = false;
	transportConfig.waterPhaseEquilibrium = false;
	OmniAtmosphere transport(transportConfig);
	for (std::size_t x = 0; x < transport.Width(); ++x)
	{
		std::vector<double> fractions(transport.SpeciesCount(), 0.0);
		fractions[x < transport.Width() / 2 ? OMNI_SPECIES_N2 : OMNI_SPECIES_O2] = 1.0;
		transport.SetSpeciesMassFractions(x, 0, fractions);
	}
	std::vector<double> initialMass(transport.SpeciesCount(), 0.0);
	for (std::size_t species = 0; species < transport.SpeciesCount(); ++species)
		initialMass[species] = transport.TotalSpeciesMassKg(species);
	const double initialInterfaceDifference = std::abs(
		transport.SpeciesMassFraction(15, 0, OMNI_SPECIES_O2) -
		transport.SpeciesMassFraction(16, 0, OMNI_SPECIES_O2));
	for (int step = 0; step < 80; ++step)
		transport.StepReference(2.0e-5);
	const double finalInterfaceDifference = std::abs(
		transport.SpeciesMassFraction(15, 0, OMNI_SPECIES_O2) -
		transport.SpeciesMassFraction(16, 0, OMNI_SPECIES_O2));
	if (!(finalInterfaceDifference < initialInterfaceDifference))
		return Fail("mixture-averaged diffusion did not mix the interface");
	for (std::size_t species = 0; species < transport.SpeciesCount(); ++species)
	{
		if (!Close(transport.TotalSpeciesMassKg(species), initialMass[species], 1.0e-10, 1.0e-14))
			return Fail("closed species transport did not conserve species mass");
	}

	auto humidConfig = Config(1, 1, OmniAtmosphereBoundary::Sealed);
	humidConfig.execution = OmniAtmosphereExecution::RuntimeLowMach;
	humidConfig.maximumRuntimeSubsteps = 8;
	humidConfig.speciesDiffusion = false;
	humidConfig.thermalConduction = false;
	OmniAtmosphere humid(humidConfig);
	humid.AddSpeciesMassDensity(0, 0, OMNI_SPECIES_H2O, 0.03);
	humid.Step();
	const auto saturated = humid.Primitive(0, 0);
	if (!(humid.TotalCondensedWaterMassKg() > 0.0) || !saturated.finite ||
		!(saturated.relativeHumidity <= 1.000001))
	{
		std::cerr << "condensation_debug condensed_mass_kg=" << humid.TotalCondensedWaterMassKg()
			<< " finite=" << saturated.finite
			<< " relative_humidity=" << saturated.relativeHumidity
			<< " water_density=" << humid.SpeciesMassDensity(0, 0, OMNI_SPECIES_H2O)
			<< " temperature_k=" << saturated.temperature << '\n';
		return Fail("supersaturated water did not condense to equilibrium");
	}
	if (std::abs(humid.Ledger().massResidualKg()) > 1.0e-12 ||
		std::abs(humid.Ledger().energyResidualJ()) > 1.0e-8 ||
		std::abs(humid.Ledger().speciesMassResidualKg(OMNI_SPECIES_H2O)) > 1.0e-12)
		return Fail("condensation ledger did not close");

	auto vacuumConfig = Config(1, 1, OmniAtmosphereBoundary::Sealed);
	vacuumConfig.execution = OmniAtmosphereExecution::RuntimeLowMach;
	vacuumConfig.maximumRuntimeSubsteps = 8;
	vacuumConfig.speciesDiffusion = false;
	vacuumConfig.thermalConduction = false;
	OmniAtmosphere vacuum(vacuumConfig);
	vacuum.ResetVacuum(1.0e-8, 293.15);
	vacuum.SetCondensedWaterDensity(0, 0, 0.01);
	const double condensedBefore = vacuum.TotalCondensedWaterMassKg();
	vacuum.Step();
	if (!(vacuum.TotalSpeciesMassKg(OMNI_SPECIES_H2O) > 0.0) ||
		!(vacuum.TotalCondensedWaterMassKg() < condensedBefore) ||
		std::abs(vacuum.Ledger().massResidualKg()) > 1.0e-12 ||
		std::abs(vacuum.Ledger().energyResidualJ()) > 1.0e-8)
		return Fail("vacuum-water evaporation or ledger failed");

	std::cout << "omni_multispecies_probe_pass=true\n";
	std::cout << "species_count=" << uniform.SpeciesCount() << '\n';
	std::cout << "partial_pressure_sum_pa=" << partialPressureSum << '\n';
	std::cout << "mixture_pressure_pa=" << primitive.pressure << '\n';
	std::cout << "diffusion_interface_before=" << initialInterfaceDifference << '\n';
	std::cout << "diffusion_interface_after=" << finalInterfaceDifference << '\n';
	std::cout << "condensed_water_mass_kg=" << humid.TotalCondensedWaterMassKg() << '\n';
	std::cout << "vacuum_evaporated_water_mass_kg=" << vacuum.TotalSpeciesMassKg(OMNI_SPECIES_H2O) << '\n';
	return 0;
}
