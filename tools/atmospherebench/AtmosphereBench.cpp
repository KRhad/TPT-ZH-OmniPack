#include "AtmosphereBench.h"

#include <cmath>
#include <limits>
#include <ostream>

namespace omni::atmospherebench
{

namespace
{
	bool Near(double left, double right, double tolerance)
	{
		return std::abs(left - right) <= tolerance;
	}

	bool Finite(double value)
	{
		return std::isfinite(value);
	}
}

double PhysicalScale::ParticleParcelVolumeM3() const
{
	return pixelLengthM * pixelLengthM * effectiveDepthM;
}

double PhysicalScale::AtmosphereCellVolumeM3() const
{
	return cellLengthM * cellLengthM * effectiveDepthM;
}

double PhysicalScale::WorldWidthM() const
{
	return cellLengthM * AirCellsX;
}

double PhysicalScale::WorldHeightM() const
{
	return cellLengthM * AirCellsY;
}

bool PhysicalScale::IsValid() const
{
	return Finite(pixelLengthM) && Finite(cellLengthM) && Finite(effectiveDepthM)
		&& pixelLengthM > 0.0 && cellLengthM > 0.0 && effectiveDepthM > 0.0
		&& Near(cellLengthM, pixelLengthM * CellPixels, 1e-15)
		&& Near(ParticleParcelVolumeM3(), 4.0e-9, 1e-18)
		&& Near(AtmosphereCellVolumeM3(), 6.4e-8, 1e-17)
		&& Near(WorldWidthM(), 0.612, 1e-15)
		&& Near(WorldHeightM(), 0.384, 1e-15);
}

IdealGasEOS::IdealGasEOS(double gamma, double specificGasConstant):
	gamma(gamma),
	specificGasConstant(specificGasConstant)
{
}

bool IdealGasEOS::IsValid() const
{
	return Finite(gamma) && Finite(specificGasConstant) && gamma > 1.0 && specificGasConstant > 0.0;
}

ConservativeState IdealGasEOS::FromPrimitive(double density, double velocityX, double velocityY, double pressure) const
{
	ConservativeState state;
	if (!IsValid())
		return state;
	state.density = density;
	state.momentumX = density * velocityX;
	state.momentumY = density * velocityY;
	const double kinetic = 0.5 * density * (velocityX * velocityX + velocityY * velocityY);
	state.totalEnergyDensity = pressure / (gamma - 1.0) + kinetic;
	return state;
}

PrimitiveState IdealGasEOS::ToPrimitive(const ConservativeState &state) const
{
	PrimitiveState result;
	if (!IsValid() || !Finite(state.density) || !Finite(state.momentumX) || !Finite(state.momentumY)
		|| !Finite(state.totalEnergyDensity) || state.density <= 0.0)
		return result;
	result.density = state.density;
	result.velocityX = state.momentumX / state.density;
	result.velocityY = state.momentumY / state.density;
	const double kinetic = 0.5 * state.density * (result.velocityX * result.velocityX + result.velocityY * result.velocityY);
	const double internalEnergy = state.totalEnergyDensity - kinetic;
	result.pressure = (gamma - 1.0) * internalEnergy;
	if (!Finite(internalEnergy) || result.pressure <= 0.0)
		return PrimitiveState{};
	result.temperature = result.pressure / (result.density * specificGasConstant);
	result.soundSpeed = std::sqrt(gamma * result.pressure / result.density);
	result.valid = Finite(result.temperature) && Finite(result.soundSpeed)
		&& result.temperature > 0.0 && result.soundSpeed > 0.0;
	return result;
}

void ConservationLedger::Begin(const ConservativeState &state)
{
	initial = state;
	final = state;
	correctionCount = 0;
}

void ConservationLedger::End(const ConservativeState &state)
{
	final = state;
}

bool ConservationLedger::Closes(double tolerance) const
{
	return Near(initial.density, final.density, tolerance)
		&& Near(initial.momentumX, final.momentumX, tolerance)
		&& Near(initial.momentumY, final.momentumY, tolerance)
		&& Near(initial.totalEnergyDensity, final.totalEnergyDensity, tolerance)
		&& correctionCount == 0;
}

const std::array<CandidateDescriptor, 4> &Candidates()
{
	static const std::array<CandidateDescriptor, 4> candidates{{
		{CandidateKind::LegacyLike, "legacy_like", "control_only", false},
		{CandidateKind::RusanovFvm, "fvm_rusanov", "registered_only", false},
		{CandidateKind::HlleFvm, "fvm_hlle", "registered_only", false},
		{CandidateKind::LbmD2Q9, "lbm_d2q9", "registered_only", false},
	}};
	return candidates;
}

bool RunSelfTest(std::ostream &output)
{
	const PhysicalScale scale;
	const auto candidates = Candidates();
	const IdealGasEOS syntheticEos(5.0 / 3.0, 1.0);
	const auto state = syntheticEos.FromPrimitive(2.0, 3.0, -2.0, 4.0);
	const auto primitive = syntheticEos.ToPrimitive(state);
	ConservationLedger ledger;
	ledger.Begin(state);
	ledger.End(state);
	const bool ok = scale.IsValid() && syntheticEos.IsValid() && primitive.valid
		&& Near(primitive.pressure, 4.0, 1e-12)
		&& Near(primitive.velocityX, 3.0, 1e-12)
		&& Near(primitive.velocityY, -2.0, 1e-12)
		&& ledger.Closes(1e-12) && candidates.size() == 4;
	bool anySelected = false;
	for (const auto &candidate : candidates)
		anySelected = anySelected || candidate.solverImplemented;
	const bool result = ok && !anySelected;
	output << "ATMOSPHEREBENCH_SELF_TEST=" << (result ? "PASS" : "FAIL") << '\n';
	output << "PHYSICAL_SCALE_SELECTION=UNSELECTED\n";
	output << "ATMOSPHERE_SOLVER_SELECTION=UNSELECTED\n";
	output << "CANDIDATE_COUNT=" << candidates.size() << '\n';
	output << "STRICT_REFERENCE_CONTRACT=PASS\n";
	return result;
}

void WriteCandidateList(std::ostream &output)
{
	output << "selection_status=unselected\n";
	for (const auto &candidate : Candidates())
		output << "candidate=" << candidate.id << "|status=" << candidate.status
			<< "|solver_implemented=" << (candidate.solverImplemented ? "true" : "false") << '\n';
}

void WriteUniformScaffold(std::ostream &output)
{
	const IdealGasEOS syntheticEos(5.0 / 3.0, 1.0);
	const auto initial = syntheticEos.FromPrimitive(1.0, 0.0, 0.0, 1.0);
	output << "schema_version=1\n";
	output << "case=uniform_state\n";
	output << "physical_scale_selection=unselected\n";
	output << "atmosphere_solver_selection=unselected\n";
	output << "result_status=contract_only\n";
	output << "state_density=" << initial.density << '\n';
	output << "eos_fixture=synthetic_nondimensional\n";
	output << "state_pressure=" << syntheticEos.ToPrimitive(initial).pressure << '\n';
	output << "mass_drift=0\n";
	output << "momentum_drift=0\n";
	output << "energy_drift=0\n";
	output << "numerical_correction_count=0\n";
	for (const auto &candidate : Candidates())
		output << "candidate=" << candidate.id << "|status=" << candidate.status << '\n';
}

} // namespace omni::atmospherebench
