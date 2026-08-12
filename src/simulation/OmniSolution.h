#pragma once

#include "OmniPhysicalScale.h"

#include <algorithm>

namespace OmniSolution
{
// NaCl saturation fit from Potter, Babcock and Brown, USGS (1977):
// wt% NaCl = 26.218 + 0.0072 t + 0.000106 t^2.  The 1.0.9 runtime
// deliberately clamps the fit to 0..100 C, the aqueous gameplay range
// covered by the companion USGS low-temperature study.  Provenance and
// redistribution status are recorded in the versioned data contract.
inline double SodiumChlorideSaturationMassFraction(double temperatureK)
{
	const double temperatureC = std::clamp(temperatureK - 273.15, 0.0, 100.0);
	const double weightPercent =
		26.218 + 0.0072 * temperatureC + 0.000106 * temperatureC * temperatureC;
	return std::clamp(weightPercent / 100.0, 0.0, 0.95);
}

inline double MaximumDissolvedSoluteKg(double solventMassKg, double temperatureK)
{
	if (!(solventMassKg > 0.0))
		return 0.0;
	const double fraction = SodiumChlorideSaturationMassFraction(temperatureK);
	return solventMassKg * fraction / (1.0 - fraction);
}

inline double SoluteMassFraction(double solventMassKg, double soluteMassKg)
{
	const double total = solventMassKg + soluteMassKg;
	return total > 0.0 ? std::clamp(soluteMassKg / total, 0.0, 1.0) : 0.0;
}

// One drawn SALT parcel can saturate roughly three default water parcels,
// matching the historical gameplay scale without claiming a measured bulk
// density for a TPT particle.
inline constexpr double DefaultSolidSoluteMassKg = OmniPhysicalScale::DefaultWaterParcelMassKg;
inline constexpr double DefaultSolutionSolventMassKg = OmniPhysicalScale::DefaultWaterParcelMassKg;
inline const double DefaultSolutionSoluteMassKg = MaximumDissolvedSoluteKg(
	DefaultSolutionSolventMassKg, OmniPhysicalScale::ReferenceTemperatureK);

// Both phase changes are explicitly rate limited.  Several neighbours cannot
// consume one SALT parcel in a single tick because Simulation also enforces a
// per-solid-particle transaction limit.
inline constexpr double MaximumDissolutionMassPerTransactionKg =
	DefaultSolidSoluteMassKg / 120.0;
inline constexpr double MaximumCrystallisationMassPerTickKg =
	DefaultSolidSoluteMassKg / 120.0;
}

