#pragma once

#include <cstdint>

enum class OmniThermalWaterPhase : uint8_t
{
	Ice,
	Mushy,
	Liquid,
	Vapor,
};

struct OmniThermalState
{
	double temperatureK = 0.0;
	double specificEnthalpyJPerKg = 0.0;
	double solidFraction = 0.0;
	double liquidFraction = 0.0;
	OmniThermalWaterPhase phase = OmniThermalWaterPhase::Ice;
};

struct OmniThermalExchange
{
	double massKg = 0.0;
	double energyJ = 0.0;
	double residualJ = 0.0;
};

class OmniThermal
{
public:
	static constexpr double TriplePointTemperatureK = 273.16;
	static constexpr double ReferencePressurePa = 101325.0;
	static constexpr double IceSpecificHeatJKgK = 2108.0;
	static constexpr double LiquidSpecificHeatJKgK = 4181.3;
	static constexpr double VaporSpecificHeatJKgK = 1864.0;
	static constexpr double LatentHeatFusionJPerKg = 333550.0;
	static constexpr double LatentHeatVaporizationJPerKg = 2500800.0;

	static double SaturationPressurePa(double temperatureK);
	static double BoilingTemperatureK(double pressurePa);
	static double RelativeHumidity(double waterPartialPressurePa, double temperatureK);
	static double WaterSpecificEnthalpyJPerKg(double temperatureK);
	static OmniThermalState WaterFromSpecificEnthalpy(double specificEnthalpyJPerKg);
	static OmniThermalExchange ExchangeWaterParcel(double massKg, double beforeTemperatureK, double afterTemperatureK);
	static double WaterSensibleExchangeEnergyJ(double massKg, double particleSpecificEnthalpyJPerKg,
		double atmosphereEquivalentSpecificEnthalpyJPerKg, double relaxationFraction);
	static double WaterEvaporationRequestMassKg(double massKg, double particleTemperatureK,
		double relativeHumidity, double pressurePa);
	static double WaterVaporSpecificEnergyJPerKg(double atmosphereTemperatureK,
		double speciesSpecificHeatCpJKgK, double speciesMolarMassKgPerMol);
	static double WaterEvaporationTransferMassKg(double requestedMassKg, double ownedMassKg,
		double liquidSpecificEnergyJPerKg, double minimumSpecificEnergyJPerKg,
		double vaporSpecificEnergyJPerKg, double referenceCellGasMassKg);
	static OmniThermalState WaterRemainingState(double initialMassKg,
		double initialSpecificEnergyJPerKg, double transferredMassKg,
		double atmosphereEnergyAddedJ, double minimumSpecificEnergyJPerKg);
	static double CoupledEnergyResidualJ(double sensibleEnergyToParticlesJ,
		double particleEnergyRemovedJ, double atmosphereEnergyAddedJ);
};
