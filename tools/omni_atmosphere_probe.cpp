#include "simulation/OmniAtmosphere.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <string_view>

namespace
{
int Fail(std::string_view message)
{
	std::cerr << "omni-atmosphere-probe: FAIL " << message << '\n';
	return 1;
}

bool Close(double left, double right, double absolute, double relative)
{
	return std::abs(left - right) <= absolute + relative * std::max(std::abs(left), std::abs(right));
}

OmniAtmosphereConfig Config(std::size_t width, std::size_t height, OmniAtmosphereBoundary boundary = OmniAtmosphereBoundary::Sealed)
{
	OmniAtmosphereConfig config;
	config.width = width;
	config.height = height;
	config.boundary = boundary;
	config.execution = OmniAtmosphereExecution::ReferenceCompressible;
	return config;
}
}

int main()
{
	OmniAtmosphere uniform(Config(32, 16, OmniAtmosphereBoundary::Periodic));
	const auto initialUniform = uniform.State(4, 4);
	uniform.StepReference(1.0e-5);
	const auto finalUniform = uniform.State(4, 4);
	if (!Close(initialUniform.density, finalUniform.density, 1.0e-14, 1.0e-13) ||
		!Close(initialUniform.momentumX, finalUniform.momentumX, 1.0e-14, 1.0e-13) ||
		!Close(initialUniform.totalEnergy, finalUniform.totalEnergy, 1.0e-10, 1.0e-13))
	{
		return Fail("uniform conservative state drifted");
	}
	if (std::abs(uniform.Ledger().massResidualKg()) > 1.0e-15 ||
		std::abs(uniform.Ledger().momentumXResidual()) > 1.0e-15 ||
		std::abs(uniform.Ledger().momentumYResidual()) > 1.0e-15 ||
		std::abs(uniform.Ledger().energyResidualJ()) > 1.0e-11 ||
		uniform.Ledger().nonFiniteCells || uniform.Ledger().densityFloorHits ||
		uniform.Ledger().pressureFloorHits || uniform.Ledger().energyFloorHits)
	{
		return Fail("uniform ledger did not close");
	}

	OmniAtmosphere heating(Config(32, 16));
	const double centrePressureBefore = heating.Primitive(16, 8).pressure;
	heating.AddEnergyDensity(16, 8, 25000.0);
	const double centrePressureAfterSource = heating.Primitive(16, 8).pressure;
	if (!(centrePressureAfterSource > centrePressureBefore))
		return Fail("sealed heating did not raise pressure");
	heating.StepReference(2.0e-5);
	if (!(heating.Primitive(15, 8).pressure > centrePressureBefore) ||
		std::abs(heating.Ledger().massResidualKg()) > 1.0e-14 ||
		std::abs(heating.Ledger().energyResidualJ()) > 1.0e-10 ||
		heating.Ledger().nonFiniteCells)
	{
		return Fail("pressure pulse or sealed-heating ledger failed");
	}

	OmniAtmosphere vacuum(Config(32, 8));
	vacuum.ResetVacuum(1.0e-8, 293.15);
	const auto centre = vacuum.State(16, 4);
	vacuum.SetCell(16, 4, {
		centre.density * 100.0,
		0.0,
		0.0,
		centre.totalEnergy * 100.0,
	});
	vacuum.StepReference(1.0e-6);
	if (!(vacuum.MinimumDensity() > 0.0) || !(vacuum.MinimumPressure() > 0.0) ||
		vacuum.Ledger().nonFiniteCells || std::abs(vacuum.Ledger().massResidualKg()) > 1.0e-16 ||
		std::abs(vacuum.Ledger().energyResidualJ()) > 1.0e-14 ||
		!vacuum.Ledger().pressureFloorHits || !vacuum.Ledger().energyFloorHits ||
		!(vacuum.Ledger().numericalEnergyCorrectionJ > 0.0))
	{
		return Fail("near-vacuum positivity, correction accounting, or conservation failed");
	}

	OmniAtmosphere leak(Config(24, 12, OmniAtmosphereBoundary::Open));
	leak.ResetUniform(2.0, 350.0);
	const double leakMassBefore = leak.TotalMassKg();
	for (int step = 0; step < 12; ++step)
		leak.StepReference(2.0e-6);
	const double leakMassAfter = leak.TotalMassKg();
	if (!(leakMassAfter < leakMassBefore) || !(leak.Ledger().boundaryMassOutKg > 0.0) ||
		std::abs(leak.Ledger().massResidualKg()) > 1.0e-13 ||
		std::abs(leak.Ledger().momentumXResidual()) > 1.0e-12 ||
		std::abs(leak.Ledger().momentumYResidual()) > 1.0e-12 || leak.Ledger().nonFiniteCells)
	{
		return Fail("open-boundary leak did not close its ledger");
	}

	OmniAtmosphere wall(Config(20, 10));
	for (std::size_t y = 0; y < wall.Height(); ++y)
		wall.SetBlocked(10, y, true);
	const double wallMassBefore = wall.TotalMassKg();
	wall.AddEnergyDensity(8, 5, 100000.0);
	wall.StepReference(1.0e-5);
	if (std::abs(wall.Ledger().massResidualKg()) > 1.0e-14 ||
		std::abs(wall.TotalMassKg() - wallMassBefore) > 1.0e-14 ||
		std::abs(wall.Ledger().momentumXResidual()) > 1.0e-12 ||
		std::abs(wall.Ledger().momentumYResidual()) > 1.0e-12 ||
		std::abs(wall.Ledger().energyResidualJ()) > 1.0e-10 ||
		wall.Ledger().nonFiniteCells)
	{
		return Fail("sealed wall conservation failed");
	}

	OmniAtmosphere blockedEdge(Config(20, 10));
	const auto asymmetricFrozen = blockedEdge.State(0, 4);
	blockedEdge.SetCell(0, 4, {
		asymmetricFrozen.density,
		asymmetricFrozen.density * 0.25,
		0.0,
		asymmetricFrozen.totalEnergy + 0.5 * asymmetricFrozen.density * 0.25 * 0.25,
	});
	for (std::size_t x = 0; x < blockedEdge.Width(); ++x)
	{
		blockedEdge.SetBlocked(x, 0, true);
		blockedEdge.SetBlocked(x, blockedEdge.Height() - 1, true);
	}
	for (std::size_t y = 1; y + 1 < blockedEdge.Height(); ++y)
	{
		blockedEdge.SetBlocked(0, y, true);
		blockedEdge.SetBlocked(blockedEdge.Width() - 1, y, true);
	}
	blockedEdge.AddEnergyDensity(2, 4, 75000.0);
	blockedEdge.StepReference(1.0e-5);
	if (std::abs(blockedEdge.Ledger().massResidualKg()) > 1.0e-14 ||
		std::abs(blockedEdge.Ledger().momentumXResidual()) > 1.0e-12 ||
		std::abs(blockedEdge.Ledger().momentumYResidual()) > 1.0e-12 ||
		std::abs(blockedEdge.Ledger().energyResidualJ()) > 1.0e-10 ||
		blockedEdge.Ledger().nonFiniteCells)
	{
		return Fail("blocked outer wall added a phantom external-boundary impulse");
	}

	OmniAtmosphere runtime(Config(153, 96, OmniAtmosphereBoundary::Open));
	runtime.SetExecutionMode(OmniAtmosphereExecution::RuntimeLowMach);
	const auto runtimeInitialCorner = runtime.State(0, 0);
	runtime.Step();
	if (runtime.Ledger().timestepLimited || runtime.Ledger().substeps != 1 ||
		!Close(runtime.Ledger().advancedTimestepS, runtime.Ledger().requestedTimestepS, 1.0e-15, 1.0e-13) ||
		std::abs(runtime.Ledger().massResidualKg()) > 1.0e-14 || runtime.Ledger().nonFiniteCells ||
		!Close(runtime.State(0, 0).density, runtimeInitialCorner.density, 1.0e-14, 1.0e-13) ||
		!Close(runtime.State(0, 0).momentumX, 0.0, 1.0e-12, 0.0) ||
		!Close(runtime.State(0, 0).momentumY, 0.0, 1.0e-12, 0.0))
	{
		return Fail("quiet runtime low-Mach path failed");
	}
	constexpr int RuntimeBenchmarkTicks = 64;
	const auto benchmarkStart = std::chrono::steady_clock::now();
	for (int tick = 0; tick < RuntimeBenchmarkTicks; ++tick)
		runtime.Step();
	const auto benchmarkEnd = std::chrono::steady_clock::now();
	const double runtimeMillisecondsPerTick =
		std::chrono::duration<double, std::milli>(benchmarkEnd - benchmarkStart).count() /
		static_cast<double>(RuntimeBenchmarkTicks);

	auto limitedConfig = Config(16, 8);
	limitedConfig.execution = OmniAtmosphereExecution::RuntimeLowMach;
	limitedConfig.maximumRuntimeSubsteps = 2;
	OmniAtmosphere limited(limitedConfig);
	limited.AddEnergyDensity(8, 4, 1.0e8);
	limited.Step();
	if (!limited.Ledger().timestepLimited ||
		!limited.Ledger().acousticRoute ||
		!(limited.Ledger().advancedTimestepS < limited.Ledger().requestedTimestepS) ||
		limited.Ledger().nonFiniteCells || !(limited.MinimumDensity() > 0.0) ||
		!(limited.MinimumPressure() > 0.0))
	{
		return Fail("runtime CFL-overload policy did not slow physical time safely");
	}
	const double limitedFirstAdvanced = limited.Ledger().advancedTimestepS;
	limited.Step();
	if (!limited.Ledger().acousticRoute || !limited.Ledger().timestepLimited ||
		!(limited.Ledger().advancedTimestepS < limited.Ledger().requestedTimestepS) ||
		limited.Ledger().nonFiniteCells || std::abs(limited.Ledger().massResidualKg()) > 1.0e-14 ||
		std::abs(limited.Ledger().energyResidualJ()) > 1.0e-8)
	{
		return Fail("compressible event did not remain on the acoustic route after its source frame");
	}
	const double limitedSecondAdvanced = limited.Ledger().advancedTimestepS;

	auto topologyConfig = Config(24, 12, OmniAtmosphereBoundary::Periodic);
	topologyConfig.execution = OmniAtmosphereExecution::RuntimeLowMach;
	OmniAtmosphere boundaryChange(topologyConfig);
	boundaryChange.ResetUniform(
		boundaryChange.Config().referenceDensity,
		boundaryChange.Config().referenceTemperature,
		0.05,
		0.0);
	boundaryChange.Step();
	if (boundaryChange.Ledger().acousticRoute || boundaryChange.Ledger().timestepLimited)
		return Fail("quiet periodic low-speed flow unexpectedly used the acoustic route");
	boundaryChange.SetBoundaryMode(OmniAtmosphereBoundary::Sealed);
	boundaryChange.Step();
	if (!boundaryChange.Ledger().acousticRoute || !boundaryChange.Ledger().timestepLimited ||
		std::abs(boundaryChange.Ledger().momentumXResidual()) > 1.0e-12 ||
		std::abs(boundaryChange.Ledger().momentumYResidual()) > 1.0e-12)
	{
		return Fail("reflecting boundary topology change did not activate a closed acoustic step");
	}

	OmniAtmosphere insertedWall(topologyConfig);
	insertedWall.ResetUniform(
		insertedWall.Config().referenceDensity,
		insertedWall.Config().referenceTemperature,
		0.05,
		0.0);
	insertedWall.Step();
	insertedWall.SetBlocked(12, 6, true);
	insertedWall.Step();
	if (!insertedWall.Ledger().acousticRoute || !insertedWall.Ledger().timestepLimited ||
		std::abs(insertedWall.Ledger().massResidualKg()) > 1.0e-14 ||
		std::abs(insertedWall.Ledger().momentumXResidual()) > 1.0e-12 ||
		std::abs(insertedWall.Ledger().momentumYResidual()) > 1.0e-12 ||
		std::abs(insertedWall.Ledger().energyResidualJ()) > 1.0e-10)
	{
		return Fail("inserted wall did not activate a conservative acoustic step");
	}

	auto cavityConfig = Config(3, 3, OmniAtmosphereBoundary::Sealed);
	cavityConfig.execution = OmniAtmosphereExecution::RuntimeLowMach;
	cavityConfig.maximumRuntimeSubsteps = 1;
	OmniAtmosphere cavity(cavityConfig);
	cavity.ResetUniform(
		cavity.Config().referenceDensity,
		cavity.Config().referenceTemperature,
		10.0,
		0.0);
	for (std::size_t y = 0; y < cavity.Height(); ++y)
	{
		for (std::size_t x = 0; x < cavity.Width(); ++x)
		{
			if (x != 1 || y != 1)
				cavity.SetBlocked(x, y, true);
		}
	}
	cavity.Step();
	cavity.Step();
	if (!cavity.Ledger().acousticRoute || !cavity.Ledger().timestepLimited ||
		std::abs(cavity.Ledger().massResidualKg()) > 1.0e-14 ||
		std::abs(cavity.Ledger().momentumXResidual()) > 1.0e-12 ||
		std::abs(cavity.Ledger().momentumYResidual()) > 1.0e-12 ||
		std::abs(cavity.Ledger().energyResidualJ()) > 1.0e-10)
	{
		return Fail("wall-normal cavity flow left the acoustic route before reflection resolved");
	}

	auto periodicSeamConfig = Config(3, 1, OmniAtmosphereBoundary::Periodic);
	periodicSeamConfig.execution = OmniAtmosphereExecution::ReferenceCompressible;
	OmniAtmosphere periodicSeam(periodicSeamConfig);
	periodicSeam.ResetUniform(
		periodicSeam.Config().referenceDensity,
		periodicSeam.Config().referenceTemperature);
	periodicSeam.SetBlocked(0, 0, true);
	periodicSeam.StepReference(1.0 / 60.0);
	if (std::abs(periodicSeam.State(1, 0).momentumX) > 1.0e-12 ||
		std::abs(periodicSeam.State(2, 0).momentumX) > 1.0e-12 ||
		std::abs(periodicSeam.Ledger().momentumXResidual()) > 1.0e-12 ||
		std::abs(periodicSeam.Ledger().momentumYResidual()) > 1.0e-12 ||
		std::abs(periodicSeam.Ledger().massResidualKg()) > 1.0e-14 ||
		std::abs(periodicSeam.Ledger().energyResidualJ()) > 1.0e-10)
	{
		return Fail("periodic blocked seam created a phantom impulse or ledger drift");
	}

	std::cout << "omni_atmosphere_cpu_mvp_pass=true\n";
	std::cout << "authoritative_fields=rho,rho_u,rho_v,rho_E\n";
	std::cout << "eos=single_species_ideal_gas\n";
	std::cout << "uniform_mass_residual_kg=" << uniform.Ledger().massResidualKg() << '\n';
	std::cout << "uniform_energy_residual_j=" << uniform.Ledger().energyResidualJ() << '\n';
	std::cout << "heated_centre_pressure_pa=" << centrePressureAfterSource << '\n';
	std::cout << "near_vacuum_min_density=" << vacuum.MinimumDensity() << '\n';
	std::cout << "near_vacuum_min_pressure_pa=" << vacuum.MinimumPressure() << '\n';
	std::cout << "near_vacuum_pressure_floor_hits=" << vacuum.Ledger().pressureFloorHits << '\n';
	std::cout << "near_vacuum_energy_correction_j=" << vacuum.Ledger().numericalEnergyCorrectionJ << '\n';
	std::cout << "near_vacuum_energy_residual_j=" << vacuum.Ledger().energyResidualJ() << '\n';
	std::cout << "leak_mass_before_kg=" << leakMassBefore << '\n';
	std::cout << "leak_mass_after_kg=" << leakMassAfter << '\n';
	std::cout << "runtime_grid_cells=" << runtime.CellCount() << '\n';
	std::cout << "authoritative_bytes_per_cell=" << sizeof(OmniAtmosphereConservative) << '\n';
	std::cout << "persistent_state_bytes_per_cell=" << (sizeof(OmniAtmosphereConservative) * 2 + sizeof(uint8_t)) << '\n';
	std::cout << "runtime_quiet_substeps=" << runtime.Ledger().substeps << '\n';
	std::cout << "runtime_quiet_ms_per_tick=" << runtimeMillisecondsPerTick << '\n';
	std::cout << "limited_requested_timestep_s=" << limited.Ledger().requestedTimestepS << '\n';
	std::cout << "limited_first_advanced_timestep_s=" << limitedFirstAdvanced << '\n';
	std::cout << "limited_second_advanced_timestep_s=" << limitedSecondAdvanced << '\n';
	std::cout << "limited_second_acoustic_route=" << (limited.Ledger().acousticRoute ? "true" : "false") << '\n';
	std::cout << "wall_momentum_x_residual=" << wall.Ledger().momentumXResidual() << '\n';
	std::cout << "wall_momentum_y_residual=" << wall.Ledger().momentumYResidual() << '\n';
	std::cout << "blocked_edge_momentum_x_residual=" << blockedEdge.Ledger().momentumXResidual() << '\n';
	std::cout << "boundary_change_acoustic_route=" << (boundaryChange.Ledger().acousticRoute ? "true" : "false") << '\n';
	std::cout << "inserted_wall_acoustic_route=" << (insertedWall.Ledger().acousticRoute ? "true" : "false") << '\n';
	std::cout << "cavity_second_acoustic_route=" << (cavity.Ledger().acousticRoute ? "true" : "false") << '\n';
	std::cout << "periodic_blocked_seam_momentum_x_residual=" << periodicSeam.Ledger().momentumXResidual() << '\n';
	return 0;
}
