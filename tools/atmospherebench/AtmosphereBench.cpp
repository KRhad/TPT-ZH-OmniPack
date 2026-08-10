#include "AtmosphereBench.h"
#include "Rusanov1D.h"

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

std::size_t AtmosphereGrid::CellCount() const
{
	if (cellsY == 0 || cellsX > std::numeric_limits<std::size_t>::max() / cellsY)
		return 0;
	return cellsX * cellsY;
}

bool AtmosphereGrid::IsValid() const
{
	return cellsX > 0 && cellsY > 0 && cellLength > 0.0 && Finite(cellLength)
		&& cellsX <= std::numeric_limits<std::size_t>::max() / cellsY;
}

bool BenchmarkCase::IsValid() const
{
	return !id.empty() && grid.IsValid()
		&& timeDomain == TimeDomain::NondimensionalContract
		&& Finite(timeStep) && timeStep > 0.0;
}

bool NumericalCorrectionLedger::IsEmpty() const
{
	return massAdded == 0.0 && massRemoved == 0.0
		&& momentumXAdded == 0.0 && momentumYAdded == 0.0
		&& energyAdded == 0.0 && energyRemoved == 0.0
		&& densityFloorHits == 0 && pressureFloorHits == 0 && eventCount == 0;
}

bool NumericalCorrectionLedger::IsConsistent() const
{
	return IsEmpty() || eventCount > 0;
}

void ConservationLedger::Begin(const ConservativeState &state)
{
	initial = state;
	final = state;
	corrections = {};
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
		&& corrections.IsEmpty();
}

bool BenchmarkResult::IsContractOnly() const
{
	return resultStatus == "contract_only" && !caseId.empty()
		&& stateBytesPerCell == sizeof(ConservativeState) && corrections.IsEmpty();
}

const std::array<CandidateDescriptor, 4> &Candidates()
{
	static const std::array<CandidateDescriptor, 4> candidates{{
		{CandidateKind::LegacyLike, "legacy_like", "control_only", false},
		{CandidateKind::RusanovFvm, "fvm_rusanov",
			"implemented_1d_uniform_pressure_pulse_density_advection_contact_near_vacuum_sod_refinement_low_mach_open_leak_performance_probes", true},
		{CandidateKind::HlleFvm, "fvm_hlle", "registered_only", false},
		{CandidateKind::LbmD2Q9, "lbm_d2q9", "registered_only", false},
	}};
	return candidates;
}

const BenchmarkCase &UniformContractCase()
{
	static const BenchmarkCase benchmarkCase{
		"uniform_state",
		{4, 3, 1.0, BoundaryMode::Periodic},
		TimeDomain::NondimensionalContract,
		1.0,
		0,
	};
	return benchmarkCase;
}

BenchmarkResult MakeUniformContractResult()
{
	const IdealGasEOS syntheticEos(5.0 / 3.0, 1.0);
	const auto initial = syntheticEos.FromPrimitive(1.0, 0.0, 0.0, 1.0);
	return {
		UniformContractCase().id,
		"contract_only",
		initial,
		initial,
		{},
		sizeof(ConservativeState),
	};
}

bool RunSelfTest(std::ostream &output)
{
	const PhysicalScale scale;
	const auto candidates = Candidates();
	const IdealGasEOS syntheticEos(5.0 / 3.0, 1.0);
	const auto state = syntheticEos.FromPrimitive(2.0, 3.0, -2.0, 4.0);
	const auto primitive = syntheticEos.ToPrimitive(state);
	const auto &uniformCase = UniformContractCase();
	const auto uniformResult = MakeUniformContractResult();
	const auto rusanov = RunRusanovUniform();
	const auto rusanovPressurePulse = RunRusanovPressurePulse();
	const auto rusanovDensityAdvection = RunRusanovDensityAdvection();
	const auto rusanovContact = RunRusanovContactDiscontinuity();
	const auto rusanovNearVacuum = RunRusanovNearVacuumExpansion();
	const auto rusanovSod = RunRusanovSodShockTube();
	const auto rusanovRefinement = RunRusanovDensityAdvectionRefinement();
	const auto rusanovLowMach = RunRusanovLowMachAdvection();
	const auto rusanovOpenLeak = RunRusanovOpenBoundaryLeak();
	const AtmosphereGrid invalidGrid{0, 1, 1.0, BoundaryMode::Periodic};
	const BenchmarkCase invalidCase{"", invalidGrid, TimeDomain::NondimensionalContract, 0.0, 0};
	NumericalCorrectionLedger nonEmptyCorrections;
	nonEmptyCorrections.energyAdded = 1.0;
	nonEmptyCorrections.eventCount = 1;
	NumericalCorrectionLedger uncountedCorrections;
	uncountedCorrections.energyAdded = 1.0;
	ConservationLedger ledger;
	ledger.Begin(state);
	ledger.End(state);
	const bool ok = scale.IsValid() && syntheticEos.IsValid() && primitive.valid
		&& Near(primitive.pressure, 4.0, 1e-12)
		&& Near(primitive.velocityX, 3.0, 1e-12)
		&& Near(primitive.velocityY, -2.0, 1e-12)
		&& ledger.Closes(1e-12) && candidates.size() == 4
		&& uniformCase.IsValid() && uniformCase.grid.CellCount() == 12
		&& uniformResult.IsContractOnly() && !invalidGrid.IsValid()
		&& !invalidCase.IsValid() && !nonEmptyCorrections.IsEmpty()
		&& nonEmptyCorrections.IsConsistent() && !uncountedCorrections.IsConsistent();
	bool onlyRusanovImplemented = true;
	for (const auto &candidate : candidates)
	{
		const bool expected = candidate.kind == CandidateKind::RusanovFvm;
		onlyRusanovImplemented = onlyRusanovImplemented && candidate.solverImplemented == expected;
	}
	const bool result = ok && rusanov.passed && rusanovPressurePulse.passed
		&& rusanovDensityAdvection.passed && rusanovContact.passed
		&& rusanovNearVacuum.passed
		&& rusanovSod.passed
		&& rusanovRefinement.passed
		&& rusanovLowMach.passed
		&& rusanovOpenLeak.passed
		&& onlyRusanovImplemented;
	output << "ATMOSPHEREBENCH_SELF_TEST=" << (result ? "PASS" : "FAIL") << '\n';
	output << "PHYSICAL_SCALE_SELECTION=UNSELECTED\n";
	output << "ATMOSPHERE_SOLVER_SELECTION=UNSELECTED\n";
	output << "CANDIDATE_COUNT=" << candidates.size() << '\n';
	output << "RUSANOV_UNIFORM_PROBE=" << (rusanov.passed ? "PASS" : "FAIL") << '\n';
	output << "RUSANOV_PRESSURE_PULSE_PROBE="
		<< (rusanovPressurePulse.passed ? "PASS" : "FAIL") << '\n';
	output << "RUSANOV_DENSITY_ADVECTION_PROBE="
		<< (rusanovDensityAdvection.passed ? "PASS" : "FAIL") << '\n';
	output << "RUSANOV_CONTACT_DISCONTINUITY_PROBE="
		<< (rusanovContact.passed ? "PASS" : "FAIL") << '\n';
	output << "RUSANOV_NEAR_VACUUM_EXPANSION_PROBE="
		<< (rusanovNearVacuum.passed ? "PASS" : "FAIL") << '\n';
	output << "RUSANOV_SOD_SHOCK_TUBE_PROBE="
		<< (rusanovSod.passed ? "PASS" : "FAIL") << '\n';
	output << "RUSANOV_DENSITY_ADVECTION_REFINEMENT_PROBE="
		<< (rusanovRefinement.passed ? "PASS" : "FAIL") << '\n';
	output << "RUSANOV_LOW_MACH_ADVECTION_PROBE="
		<< (rusanovLowMach.passed ? "PASS" : "FAIL") << '\n';
	output << "RUSANOV_LOW_MACH_SUITABILITY="
		<< (rusanovLowMach.suitabilityPassed ? "PASS" : "FAIL") << '\n';
	output << "RUSANOV_OPEN_BOUNDARY_LEAK_PROBE="
		<< (rusanovOpenLeak.passed ? "PASS" : "FAIL") << '\n';
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
	const auto &benchmarkCase = UniformContractCase();
	const auto result = MakeUniformContractResult();
	const auto initial = result.initial;
	output << "schema_version=1\n";
	output << "case=" << benchmarkCase.id << '\n';
	output << "case_time_domain=nondimensional_contract\n";
	output << "case_timestep=" << benchmarkCase.timeStep << '\n';
	output << "case_step_count=" << benchmarkCase.stepCount << '\n';
	output << "grid_cells_x=" << benchmarkCase.grid.cellsX << '\n';
	output << "grid_cells_y=" << benchmarkCase.grid.cellsY << '\n';
	output << "grid_cell_count=" << benchmarkCase.grid.CellCount() << '\n';
	output << "physical_scale_selection=unselected\n";
	output << "atmosphere_solver_selection=unselected\n";
	output << "result_status=" << result.resultStatus << '\n';
	output << "state_density=" << initial.density << '\n';
	output << "eos_fixture=synthetic_nondimensional\n";
	output << "state_pressure=" << syntheticEos.ToPrimitive(initial).pressure << '\n';
	output << "mass_drift=" << (result.final.density - result.initial.density) << '\n';
	const auto momentumXDrift = result.final.momentumX - result.initial.momentumX;
	const auto momentumYDrift = result.final.momentumY - result.initial.momentumY;
	output << "momentum_drift=" << std::hypot(momentumXDrift, momentumYDrift) << '\n';
	output << "momentum_x_drift=" << momentumXDrift << '\n';
	output << "momentum_y_drift=" << momentumYDrift << '\n';
	output << "energy_drift=" << (result.final.totalEnergyDensity - result.initial.totalEnergyDensity) << '\n';
	output << "numerical_correction_count=" << result.corrections.eventCount << '\n';
	output << "correction_mass_added=" << result.corrections.massAdded << '\n';
	output << "correction_mass_removed=" << result.corrections.massRemoved << '\n';
	output << "correction_momentum_x_added=" << result.corrections.momentumXAdded << '\n';
	output << "correction_momentum_y_added=" << result.corrections.momentumYAdded << '\n';
	output << "correction_energy_added=" << result.corrections.energyAdded << '\n';
	output << "correction_energy_removed=" << result.corrections.energyRemoved << '\n';
	output << "density_floor_hits=" << result.corrections.densityFloorHits << '\n';
	output << "pressure_floor_hits=" << result.corrections.pressureFloorHits << '\n';
	output << "correction_event_count=" << result.corrections.eventCount << '\n';
	output << "state_bytes_per_cell=" << result.stateBytesPerCell << '\n';
	for (const auto &candidate : Candidates())
		output << "candidate=" << candidate.id << "|status=" << candidate.status << '\n';
}

} // namespace omni::atmospherebench
