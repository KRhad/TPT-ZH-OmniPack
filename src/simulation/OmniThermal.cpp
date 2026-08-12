#include "OmniThermal.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
constexpr double FusionLowerK = 272.65;
constexpr double FusionUpperK = 273.65;
constexpr double UniversalGasConstant = 8.31446261815324;

double IceEnthalpyAtFusionLower()
{
	return OmniThermal::IceSpecificHeatJKgK * FusionLowerK;
}

double LiquidEnthalpyAtFusionUpper()
{
	return IceEnthalpyAtFusionLower() +
		OmniThermal::LiquidSpecificHeatJKgK * (FusionUpperK - FusionLowerK) +
		OmniThermal::LatentHeatFusionJPerKg;
}

double LiquidEnthalpyAtBoiling()
{
	return LiquidEnthalpyAtFusionUpper() +
		OmniThermal::LiquidSpecificHeatJKgK * (373.15 - FusionUpperK);
}

double VaporEnthalpyAtBoiling()
{
	return LiquidEnthalpyAtBoiling() + OmniThermal::LatentHeatVaporizationJPerKg;
}
}

double OmniThermal::SaturationPressurePa(double temperatureK)
{
	if (!std::isfinite(temperatureK) || temperatureK <= 0.0)
		return 0.0;
	if (temperatureK < TriplePointTemperatureK)
	{
		// Murphy-Koop (2005), vapour pressure over ice, Pa; valid over the
		// atmospheric ice range and used only as a bounded reference correlation.
		const double logPressure = 9.550426 - 5723.265 / temperatureK +
			3.53068 * std::log(temperatureK) - 0.00728332 * temperatureK;
		return std::exp(logPressure);
	}
	if (temperatureK >= 647.096)
		return 22.064e6;

	// IAPWS-IF97 Region 4 saturation-pressure equation (R7-97, 2012).
	constexpr double n1 = 0.11670521452767e4;
	constexpr double n2 = -0.72421316703206e6;
	constexpr double n3 = -0.17073846940092e2;
	constexpr double n4 = 0.12020824702470e5;
	constexpr double n5 = -0.32325550322333e7;
	constexpr double n6 = 0.14915108613530e2;
	constexpr double n7 = -0.48232657361591e4;
	constexpr double n8 = 0.40511340542057e6;
	constexpr double n9 = -0.23855557567849;
	constexpr double n10 = 0.65017534844798e3;
	const double theta = temperatureK + n9 / (temperatureK - n10);
	const double A = theta * theta + n1 * theta + n2;
	const double B = n3 * theta * theta + n4 * theta + n5;
	const double C = n6 * theta * theta + n7 * theta + n8;
	const double discriminant = std::max(B * B - 4.0 * A * C, 0.0);
	const double beta = 2.0 * C / (-B + std::sqrt(discriminant));
	return std::pow(beta, 4.0) * 1.0e6;
}

double OmniThermal::BoilingTemperatureK(double pressurePa)
{
	if (!std::isfinite(pressurePa) || pressurePa <= 0.0)
		return std::numeric_limits<double>::quiet_NaN();
	double lower = 180.0;
	double upper = 647.095;
	for (int iteration = 0; iteration < 80; ++iteration)
	{
		const double middle = 0.5 * (lower + upper);
		if (SaturationPressurePa(middle) < pressurePa)
			lower = middle;
		else
			upper = middle;
	}
	return 0.5 * (lower + upper);
}

double OmniThermal::RelativeHumidity(double waterPartialPressurePa, double temperatureK)
{
	const double saturation = SaturationPressurePa(temperatureK);
	if (!std::isfinite(waterPartialPressurePa) || saturation <= 0.0)
		return std::numeric_limits<double>::quiet_NaN();
	return std::max(0.0, waterPartialPressurePa) / saturation;
}

double OmniThermal::WaterSpecificEnthalpyJPerKg(double temperatureK)
{
	if (!std::isfinite(temperatureK))
		return std::numeric_limits<double>::quiet_NaN();
	if (temperatureK < FusionLowerK)
		return IceSpecificHeatJKgK * temperatureK;
	if (temperatureK <= FusionUpperK)
	{
		const double fraction = (temperatureK - FusionLowerK) / (FusionUpperK - FusionLowerK);
		return IceEnthalpyAtFusionLower() +
			LiquidSpecificHeatJKgK * (temperatureK - FusionLowerK) +
			LatentHeatFusionJPerKg * fraction;
	}
	if (temperatureK < 373.15)
		return LiquidEnthalpyAtFusionUpper() + LiquidSpecificHeatJKgK * (temperatureK - FusionUpperK);
	return VaporEnthalpyAtBoiling() + VaporSpecificHeatJKgK * (temperatureK - 373.15);
}

OmniThermalState OmniThermal::WaterFromSpecificEnthalpy(double specificEnthalpyJPerKg)
{
	OmniThermalState result{};
	if (!std::isfinite(specificEnthalpyJPerKg))
		return result;
	const double fusionLowerH = IceEnthalpyAtFusionLower();
	const double fusionUpperH = LiquidEnthalpyAtFusionUpper();
	if (specificEnthalpyJPerKg < fusionLowerH)
	{
		result.temperatureK = specificEnthalpyJPerKg / IceSpecificHeatJKgK;
		result.solidFraction = 1.0;
		result.phase = OmniThermalWaterPhase::Ice;
	}
	else if (specificEnthalpyJPerKg <= fusionUpperH)
	{
		const double width = fusionUpperH - fusionLowerH;
		const double fraction = std::clamp((specificEnthalpyJPerKg - fusionLowerH) / width, 0.0, 1.0);
		result.temperatureK = FusionLowerK + fraction * (FusionUpperK - FusionLowerK);
		result.solidFraction = 1.0 - fraction;
		result.liquidFraction = fraction;
		result.phase = OmniThermalWaterPhase::Mushy;
	}
	else if (specificEnthalpyJPerKg <= LiquidEnthalpyAtBoiling())
	{
		result.temperatureK = FusionUpperK + (specificEnthalpyJPerKg - fusionUpperH) / LiquidSpecificHeatJKgK;
		result.liquidFraction = 1.0;
		result.phase = OmniThermalWaterPhase::Liquid;
	}
	else if (specificEnthalpyJPerKg <= VaporEnthalpyAtBoiling())
	{
		const double fraction = std::clamp((specificEnthalpyJPerKg - LiquidEnthalpyAtBoiling()) /
			LatentHeatVaporizationJPerKg, 0.0, 1.0);
		result.temperatureK = 373.15;
		result.liquidFraction = 1.0 - fraction;
		result.vaporFraction = fraction;
		result.phase = OmniThermalWaterPhase::Boiling;
	}
	else
	{
		result.temperatureK = 373.15 +
			(specificEnthalpyJPerKg - VaporEnthalpyAtBoiling()) / VaporSpecificHeatJKgK;
		result.vaporFraction = 1.0;
		result.phase = OmniThermalWaterPhase::Vapor;
	}
	result.specificEnthalpyJPerKg = specificEnthalpyJPerKg;
	return result;
}

OmniThermalExchange OmniThermal::ExchangeWaterParcel(
	double massKg,
	double beforeTemperatureK,
	double afterTemperatureK)
{
	OmniThermalExchange result{};
	if (!std::isfinite(massKg) || massKg < 0.0 || !std::isfinite(beforeTemperatureK) ||
		!std::isfinite(afterTemperatureK))
		return result;
	result.massKg = massKg;
	const double before = WaterSpecificEnthalpyJPerKg(beforeTemperatureK);
	const double after = WaterSpecificEnthalpyJPerKg(afterTemperatureK);
	result.energyJ = massKg * (after - before);
	result.residualJ = 0.0;
	return result;
}

double OmniThermal::WaterSensibleExchangeEnergyJ(
	double massKg,
	double particleSpecificEnthalpyJPerKg,
	double atmosphereEquivalentSpecificEnthalpyJPerKg,
	double relaxationFraction)
{
	if (!std::isfinite(massKg) || massKg < 0.0 ||
		!std::isfinite(particleSpecificEnthalpyJPerKg) ||
		!std::isfinite(atmosphereEquivalentSpecificEnthalpyJPerKg) ||
		!std::isfinite(relaxationFraction) || relaxationFraction < 0.0)
		return 0.0;
	return relaxationFraction * massKg *
		(atmosphereEquivalentSpecificEnthalpyJPerKg - particleSpecificEnthalpyJPerKg);
}

double OmniThermal::WaterEvaporationRequestMassKg(
	double massKg,
	double particleTemperatureK,
	double relativeHumidity,
	double pressurePa)
{
	if (!std::isfinite(massKg) || massKg <= 0.0 ||
		!std::isfinite(particleTemperatureK) ||
		!std::isfinite(relativeHumidity) || !std::isfinite(pressurePa))
		return 0.0;
	const double undersaturation = std::clamp(1.0 - relativeHumidity, 0.0, 1.0);
	const bool boiling = SaturationPressurePa(particleTemperatureK) >= pressurePa;
	const double fraction = boiling
		? 0.20
		: 0.002 * undersaturation * std::clamp(particleTemperatureK / 293.15, 0.1, 4.0);
	return massKg * std::clamp(fraction, 0.0, 0.20);
}

double OmniThermal::WaterVaporSpecificEnergyJPerKg(
	double atmosphereTemperatureK,
	double speciesSpecificHeatCpJKgK,
	double speciesMolarMassKgPerMol)
{
	if (!std::isfinite(atmosphereTemperatureK) ||
		!std::isfinite(speciesSpecificHeatCpJKgK) ||
		!std::isfinite(speciesMolarMassKgPerMol) ||
		atmosphereTemperatureK <= 0.0 || speciesMolarMassKgPerMol <= 0.0)
		return std::numeric_limits<double>::quiet_NaN();
	const double gasConstant = UniversalGasConstant / speciesMolarMassKgPerMol;
	const double vaporCv = speciesSpecificHeatCpJKgK - gasConstant;
	if (!(vaporCv > 0.0))
		return std::numeric_limits<double>::quiet_NaN();
	return vaporCv * atmosphereTemperatureK + LatentHeatVaporizationJPerKg;
}

double OmniThermal::WaterEvaporationTransferMassKg(
	double requestedMassKg,
	double ownedMassKg,
	double liquidSpecificEnergyJPerKg,
	double minimumSpecificEnergyJPerKg,
	double vaporSpecificEnergyJPerKg,
	double referenceCellGasMassKg)
{
	if (!std::isfinite(requestedMassKg) || !std::isfinite(ownedMassKg) ||
		!std::isfinite(liquidSpecificEnergyJPerKg) ||
		!std::isfinite(minimumSpecificEnergyJPerKg) ||
		!std::isfinite(vaporSpecificEnergyJPerKg) ||
		!std::isfinite(referenceCellGasMassKg) || ownedMassKg <= 0.0 ||
		requestedMassKg <= 0.0 || referenceCellGasMassKg <= 0.0)
		return 0.0;
	double maximumMassKg = std::min(requestedMassKg, ownedMassKg);
	if (vaporSpecificEnergyJPerKg > minimumSpecificEnergyJPerKg)
	{
		maximumMassKg = std::min(maximumMassKg,
			ownedMassKg * std::max(liquidSpecificEnergyJPerKg - minimumSpecificEnergyJPerKg, 0.0) /
			(vaporSpecificEnergyJPerKg - minimumSpecificEnergyJPerKg));
	}
	return std::clamp(maximumMassKg, 0.0,
		std::min(ownedMassKg * 0.20, referenceCellGasMassKg));
}

OmniThermalState OmniThermal::WaterRemainingState(
	double initialMassKg,
	double initialSpecificEnergyJPerKg,
	double transferredMassKg,
	double atmosphereEnergyAddedJ,
	double minimumSpecificEnergyJPerKg)
{
	if (!std::isfinite(initialMassKg) || !std::isfinite(initialSpecificEnergyJPerKg) ||
		!std::isfinite(transferredMassKg) || !std::isfinite(atmosphereEnergyAddedJ) ||
		!std::isfinite(minimumSpecificEnergyJPerKg) || initialMassKg <= transferredMassKg ||
		initialMassKg <= 0.0)
		return WaterFromSpecificEnthalpy(minimumSpecificEnergyJPerKg);
	const double remainingMassKg = initialMassKg - transferredMassKg;
	const double initialEnergyJ = initialMassKg * initialSpecificEnergyJPerKg;
	const double remainingSpecificEnergy = std::max(
		(initialEnergyJ - atmosphereEnergyAddedJ) / remainingMassKg,
		minimumSpecificEnergyJPerKg);
	return WaterFromSpecificEnthalpy(remainingSpecificEnergy);
}

double OmniThermal::CoupledEnergyResidualJ(
	double sensibleEnergyToParticlesJ,
	double particleEnergyRemovedJ,
	double atmosphereEnergyAddedJ)
{
	if (!std::isfinite(sensibleEnergyToParticlesJ) ||
		!std::isfinite(particleEnergyRemovedJ) ||
		!std::isfinite(atmosphereEnergyAddedJ))
		return std::numeric_limits<double>::quiet_NaN();
	return sensibleEnergyToParticlesJ - particleEnergyRemovedJ + atmosphereEnergyAddedJ;
}
