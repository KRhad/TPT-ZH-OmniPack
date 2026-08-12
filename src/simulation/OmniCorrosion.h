#pragma once

#include <algorithm>
#include <cmath>

namespace OmniCorrosion
{
// This is a bounded gameplay-time process law, not a measured corrosion-rate
// equation. Inputs preserve the physical ordering needed by 1.0.9 while the
// time compression remains explicit and centralized.
inline constexpr double ProgressPerWetReferenceTick = 0.004;
inline constexpr double ReferenceOxygenPartialPressurePa = 21200.0;
inline constexpr double HumidityOnset = 0.35;
inline constexpr double ChlorideAcceleration = 4.0;
inline constexpr double MaximumPassivation = 0.85;
inline constexpr double PassivationGrowthFraction = 0.20;

inline double TemperatureFactor(double temperatureK)
{
	return std::clamp(std::pow(2.0, (temperatureK - 293.15) / 20.0), 0.1, 8.0);
}

inline double HumidityMoistureFactor(double relativeHumidity)
{
	return std::clamp((relativeHumidity - HumidityOnset) / (1.0 - HumidityOnset), 0.0, 1.0);
}
}
