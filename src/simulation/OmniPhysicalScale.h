#pragma once

// The selected 1.0.6 runtime contract. The Phase 5 candidate JSON remains an
// immutable benchmark input; production defaults are owned here so the CPU
// reference cannot silently drift from the documented geometry and fixed dt.
namespace OmniPhysicalScale
{
inline constexpr int CellPixels = 4;
inline constexpr double PixelLengthM = 1.0e-3;
inline constexpr double CellLengthM = 4.0e-3;
inline constexpr double EffectiveDepthM = 4.0e-3;
inline constexpr double TimestepS = 1.0 / 60.0;
inline constexpr double ReferenceDensityKgM3 = 1.225;
inline constexpr double ReferenceTemperatureK = 293.15;
inline constexpr double GasConstantJKgK = 287.05;
inline constexpr double Gamma = 1.4;
inline constexpr double ReferencePressurePa = ReferenceDensityKgM3 * GasConstantJKgK * ReferenceTemperatureK;
inline constexpr double LegacyPressureScalePa = 1000.0;
}

static_assert(OmniPhysicalScale::CellLengthM == OmniPhysicalScale::PixelLengthM * OmniPhysicalScale::CellPixels);
static_assert(OmniPhysicalScale::CellLengthM > 0.0 && OmniPhysicalScale::EffectiveDepthM > 0.0);
static_assert(OmniPhysicalScale::TimestepS > 0.0);
