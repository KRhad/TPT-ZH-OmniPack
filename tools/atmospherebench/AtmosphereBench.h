#pragma once

#include <array>
#include <cstddef>
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

enum class BoundaryMode
{
	Periodic,
	Sealed,
	Open,
};

enum class TimeDomain
{
	NondimensionalContract,
};

struct AtmosphereGrid
{
	std::size_t cellsX = 0;
	std::size_t cellsY = 0;
	double cellLength = 0.0;
	BoundaryMode boundaryMode = BoundaryMode::Periodic;

	std::size_t CellCount() const;
	bool IsValid() const;
};

struct BenchmarkCase
{
	std::string_view id;
	AtmosphereGrid grid;
	TimeDomain timeDomain = TimeDomain::NondimensionalContract;
	double timeStep = 0.0;
	std::size_t stepCount = 0;

	bool IsValid() const;
};

struct NumericalCorrectionLedger
{
	double massAdded = 0.0;
	double massRemoved = 0.0;
	double momentumXAdded = 0.0;
	double momentumYAdded = 0.0;
	double energyAdded = 0.0;
	double energyRemoved = 0.0;
	std::size_t densityFloorHits = 0;
	std::size_t pressureFloorHits = 0;
	std::size_t eventCount = 0;

	bool IsEmpty() const;
	bool IsConsistent() const;
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
	NumericalCorrectionLedger corrections{};

	void Begin(const ConservativeState &state);
	void End(const ConservativeState &state);
	bool Closes(double tolerance) const;
};

struct BenchmarkResult
{
	std::string_view caseId;
	std::string_view resultStatus;
	ConservativeState initial{};
	ConservativeState final{};
	NumericalCorrectionLedger corrections{};
	std::size_t stateBytesPerCell = sizeof(ConservativeState);

	bool IsContractOnly() const;
};

enum class CandidateKind
{
	LegacyLike,
	RusanovFvm,
	AllSpeedRusanovFvm,
	HllcRusanovFallbackFvm,
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

const std::array<CandidateDescriptor, 6> &Candidates();
const BenchmarkCase &UniformContractCase();
BenchmarkResult MakeUniformContractResult();

bool RunSelfTest(std::ostream &output);
void WriteCandidateList(std::ostream &output);
void WriteUniformScaffold(std::ostream &output);

} // namespace omni::atmospherebench
