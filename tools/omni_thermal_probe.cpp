#include "simulation/OmniThermal.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>

namespace
{
int Fail(const char *message)
{
	std::cerr << "omni_thermal_probe: FAIL " << message << '\n';
	return 1;
}

bool Close(double left, double right, double relative, double absolute)
{
	return std::abs(left - right) <= absolute + relative * std::max(std::abs(left), std::abs(right));
}
}

int main()
{
	const double triplePressure = OmniThermal::SaturationPressurePa(273.16);
	const double boilingPressure = OmniThermal::SaturationPressurePa(373.15);
	if (!Close(triplePressure, 611.657, 5.0e-4, 0.05))
		return Fail("IAPWS triple-point saturation pressure drifted");
	if (!Close(boilingPressure, 101417.996, 5.0e-4, 5.0))
		return Fail("IAPWS 100 C saturation pressure drifted");
	const double normalBoiling = OmniThermal::BoilingTemperatureK(101325.0);
	if (!Close(normalBoiling, 373.124, 0.0, 0.05))
		return Fail("normal boiling-point inversion drifted");

	constexpr std::array<double, 7> temperatures = { 240.0, 272.65, 273.15, 273.65, 300.0, 373.15, 450.0 };
	for (double temperature : temperatures)
	{
		const double enthalpy = OmniThermal::WaterSpecificEnthalpyJPerKg(temperature);
		const auto restored = OmniThermal::WaterFromSpecificEnthalpy(enthalpy);
		if (!Close(restored.temperatureK, temperature, 0.0, 1.0e-9))
			return Fail("water enthalpy inverse mapping failed");
	}
	const auto halfMelt = OmniThermal::WaterFromSpecificEnthalpy(
		OmniThermal::WaterSpecificEnthalpyJPerKg(273.15));
	if (halfMelt.phase != OmniThermalWaterPhase::Mushy ||
		!(halfMelt.solidFraction > 0.0 && halfMelt.solidFraction < 1.0) ||
		!Close(halfMelt.solidFraction + halfMelt.liquidFraction, 1.0, 0.0, 1.0e-12))
		return Fail("water mushy phase fractions are invalid");
	const double saturatedLiquid = OmniThermal::WaterSpecificEnthalpyJPerKg(373.15) -
		OmniThermal::LatentHeatVaporizationJPerKg;
	const double boilingGapEnthalpy = saturatedLiquid + 1000.0;
	const auto boilingGap = OmniThermal::WaterFromSpecificEnthalpy(boilingGapEnthalpy);
	if (boilingGap.phase != OmniThermalWaterPhase::Boiling ||
		!Close(boilingGap.temperatureK, 373.15, 0.0, 1.0e-12) ||
		!Close(boilingGap.liquidFraction + boilingGap.vaporFraction, 1.0, 0.0, 1.0e-12) ||
		!Close(boilingGap.specificEnthalpyJPerKg, boilingGapEnthalpy, 0.0, 1.0e-9))
	{
		return Fail("water latent-vaporization enthalpy interval is not invertible");
	}

	const auto exchange = OmniThermal::ExchangeWaterParcel(4.0e-6, 260.0, 280.0);
	if (!(exchange.energyJ > 0.0) || exchange.residualJ != 0.0)
		return Fail("water parcel heat exchange failed");
	const double rh = OmniThermal::RelativeHumidity(0.5 * OmniThermal::SaturationPressurePa(293.15), 293.15);
	if (!Close(rh, 0.5, 0.0, 1.0e-12))
		return Fail("relative humidity derivation failed");

	const double parcelMassKg = 4.0e-6;
	const double particleEnthalpy = OmniThermal::WaterSpecificEnthalpyJPerKg(300.0);
	const double atmosphereEnthalpy = OmniThermal::WaterSpecificEnthalpyJPerKg(320.0);
	const double sensibleEnergy = OmniThermal::WaterSensibleExchangeEnergyJ(
		parcelMassKg, particleEnthalpy, atmosphereEnthalpy, 0.05);
	if (!(sensibleEnergy > 0.0))
		return Fail("strict water sensible-exchange transaction failed");
	const double lowPressureRequest = OmniThermal::WaterEvaporationRequestMassKg(
		parcelMassKg, 373.15, 0.0, 1000.0);
	const double highPressureRequest = OmniThermal::WaterEvaporationRequestMassKg(
		parcelMassKg, 293.15, 0.5, 2.0e5);
	if (!(lowPressureRequest > highPressureRequest && lowPressureRequest <= parcelMassKg * 0.20))
		return Fail("strict pressure-dependent evaporation request failed");
	const double vaporSpecificEnergy = OmniThermal::WaterVaporSpecificEnergyJPerKg(
		300.0, 1864.0, 0.01801528);
	const double minimumSpecificEnergy = OmniThermal::WaterSpecificEnthalpyJPerKg(1.0);
	const double transferMass = OmniThermal::WaterEvaporationTransferMassKg(
		lowPressureRequest, parcelMassKg, particleEnthalpy, minimumSpecificEnergy,
		vaporSpecificEnergy, 7.84e-8);
	if (!(transferMass > 0.0 && transferMass <= 7.84e-8))
		return Fail("strict water evaporation budget failed");
	const auto remaining = OmniThermal::WaterRemainingState(
		parcelMassKg, particleEnthalpy, transferMass, transferMass * vaporSpecificEnergy,
		minimumSpecificEnergy);
	if (!std::isfinite(remaining.temperatureK) || remaining.temperatureK <= 0.0)
		return Fail("strict remaining-water state failed");
	if (OmniThermal::CoupledEnergyResidualJ(2.0, 3.0, 1.0) != 0.0)
		return Fail("strict coupled-energy residual failed");

	std::cout << "omni_thermal_probe_pass=true\n";
	std::cout << "triple_point_pressure_pa=" << triplePressure << '\n';
	std::cout << "normal_boiling_temperature_k=" << normalBoiling << '\n';
	std::cout << "half_melt_solid_fraction=" << halfMelt.solidFraction << '\n';
	std::cout << "boiling_gap_vapor_fraction=" << boilingGap.vaporFraction << '\n';
	std::cout << "parcel_exchange_energy_j=" << exchange.energyJ << '\n';
	std::cout << "strict_water_transfer_mass_kg=" << transferMass << '\n';
	return 0;
}
