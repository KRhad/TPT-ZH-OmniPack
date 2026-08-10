#pragma once

#include <array>
#include <iosfwd>
#include <string_view>

namespace omni::atmospherebench
{

struct PhysicalScale
{
	static constexpr int CellPixels = 4;
	static constexpr int AirCellsX = 153;
	static constexpr int AirCellsY = 96;
	static constexpr int ParticlePixelsX = 612;
	static constexpr int ParticlePixelsY = 384;

	double pixelLengthM = 0.001;
	double cellLengthM = 0.004;
	double effectiveDepthM = 0.004;

	double ParticleParcelVolumeM3() const;
	double AtmosphereCellVolumeM3() const;
	double WorldWidthM() const;
	double WorldHeightM() const;
	bool IsValid() const;
};

struct ConservativeState
{
	double density = 0.0;
	double momentumX = 0.0;
	double momentumY = 0.0;
	double totalEnergyDensity = 0.0;
};

struct PrimitiveState
{
	double density = 0.0;
	double velocityX = 0.0;
	double velocityY = 0.0;
	double pressure = 0.0;
	double temperature = 0.0;
	double soundSpeed = 0.0;
	bool valid = false;
};

class IdealGasEOS
{
public:
	IdealGasEOS(double gamma, double specificGasConstant);

	ConservativeState FromPrimitive(double density, double velocityX, double velocityY, double pressure) const;
	PrimitiveState ToPrimitive(const ConservativeState &state) const;
	bool IsValid() const;

private:
	double gamma;
	double specificGasConstant;
};

struct ConservationLedger
{
	ConservativeState initial{};
	ConservativeState final{};
	std::size_t correctionCount = 0;

	void Begin(const ConservativeState &state);
	void End(const ConservativeState &state);
	bool Closes(double tolerance) const;
};

enum class CandidateKind
{
	LegacyLike,
	RusanovFvm,
	HlleFvm,
	LbmD2Q9,
};

struct CandidateDescriptor
{
	CandidateKind kind;
	std::string_view id;
	std::string_view status;
	bool solverImplemented;
};

const std::array<CandidateDescriptor, 4> &Candidates();

bool RunSelfTest(std::ostream &output);
void WriteCandidateList(std::ostream &output);
void WriteUniformScaffold(std::ostream &output);

} // namespace omni::atmospherebench
