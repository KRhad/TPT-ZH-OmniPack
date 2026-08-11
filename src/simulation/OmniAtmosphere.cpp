#include "OmniAtmosphere.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace
{
bool Finite(double value)
{
	return std::isfinite(value);
}

double Square(double value)
{
	return value * value;
}
}

OmniAtmosphere::OmniAtmosphere(OmniAtmosphereConfig newConfig):
	config(newConfig),
	state(config.width * config.height),
	next(config.width * config.height),
	blocked(config.width * config.height, 0)
{
	if (!config.width || !config.height ||
		!Finite(config.scale.cellLengthM) || config.scale.cellLengthM <= 0.0 ||
		!Finite(config.scale.effectiveDepthM) || config.scale.effectiveDepthM <= 0.0 ||
		!Finite(config.scale.timestepS) || config.scale.timestepS <= 0.0 ||
		!Finite(config.gamma) || config.gamma <= 1.0 ||
		!Finite(config.gasConstant) || config.gasConstant <= 0.0 ||
		!Finite(config.referenceDensity) || config.referenceDensity <= 0.0 ||
		!Finite(config.referenceTemperature) || config.referenceTemperature <= 0.0 ||
		!Finite(config.referencePressure) || config.referencePressure <= 0.0 ||
		!Finite(config.legacyPressureScalePa) || config.legacyPressureScalePa <= 0.0 ||
		!Finite(config.densityFloor) || config.densityFloor <= 0.0 ||
		!Finite(config.pressureFloor) || config.pressureFloor <= 0.0 ||
		!Finite(config.internalEnergyFloor) || config.internalEnergyFloor <= 0.0 ||
		!Finite(config.cfl) || config.cfl <= 0.0 || config.cfl > 1.0 ||
		!config.maximumRuntimeSubsteps || !config.maximumReferenceSubsteps)
	{
		throw std::invalid_argument("invalid OmniAtmosphere configuration");
	}
	ResetUniform(config.referenceDensity, config.referenceTemperature);
}

void OmniAtmosphere::ResetState(
	std::vector<OmniAtmosphereConservative> &target,
	double density,
	double temperature,
	double velocityX,
	double velocityY)
{
	if (!Finite(density) || !Finite(temperature) || !Finite(velocityX) || !Finite(velocityY) ||
		density <= 0.0 || temperature <= 0.0)
	{
		throw std::invalid_argument("invalid OmniAtmosphere uniform state");
	}
	const double pressure = density * config.gasConstant * temperature;
	const double kinetic = 0.5 * density * (Square(velocityX) + Square(velocityY));
	const OmniAtmosphereConservative value{
		density,
		density * velocityX,
		density * velocityY,
		pressure / (config.gamma - 1.0) + kinetic,
	};
	std::fill(target.begin(), target.end(), value);
}

void OmniAtmosphere::ResetUniform(double density, double temperature, double velocityX, double velocityY)
{
	ResetState(state, density, temperature, velocityX, velocityY);
	next = state;
	std::fill(blocked.begin(), blocked.end(), 0);
	ledger = {};
	pendingEvent = false;
	compressibleActive = false;
	pendingSourceMassKg = 0.0;
	pendingSourceMomentumX = 0.0;
	pendingSourceMomentumY = 0.0;
	pendingSourceEnergyJ = 0.0;
}

void OmniAtmosphere::ResetVacuum(double density, double temperature)
{
	ResetUniform(std::max(density, config.densityFloor), temperature);
}

void OmniAtmosphere::SetReferenceState(double density, double temperature)
{
	if (!Finite(density) || density <= 0.0 || !Finite(temperature) || temperature <= 0.0)
		throw std::invalid_argument("invalid OmniAtmosphere reference state");
	config.referenceDensity = density;
	config.referenceTemperature = temperature;
	config.referencePressure = density * config.gasConstant * temperature;
}

void OmniAtmosphere::SetBoundaryMode(OmniAtmosphereBoundary boundary)
{
	if (config.boundary == boundary)
		return;
	config.boundary = boundary;
	pendingEvent = true;
}

void OmniAtmosphere::SetBlocked(std::size_t x, std::size_t y, bool value)
{
	if (x >= config.width || y >= config.height)
		throw std::out_of_range("OmniAtmosphere blocked-cell coordinate");
	const auto index = Index(x, y);
	const uint8_t nextValue = value ? 1 : 0;
	if (blocked[index] == nextValue)
		return;
	blocked[index] = nextValue;
	pendingEvent = true;
}

bool OmniAtmosphere::IsBlocked(std::size_t x, std::size_t y) const
{
	if (x >= config.width || y >= config.height)
		throw std::out_of_range("OmniAtmosphere blocked-cell coordinate");
	return blocked[Index(x, y)] != 0;
}

void OmniAtmosphere::AddEnergyDensity(std::size_t x, std::size_t y, double joulesPerM3)
{
	if (x >= config.width || y >= config.height || !Finite(joulesPerM3))
		throw std::invalid_argument("invalid OmniAtmosphere energy source");
	state[Index(x, y)].totalEnergy += joulesPerM3;
	pendingSourceEnergyJ += joulesPerM3 * config.scale.cellVolumeM3();
	pendingEvent = true;
}

void OmniAtmosphere::AddMassDensity(std::size_t x, std::size_t y, double kilogramsPerM3)
{
	if (x >= config.width || y >= config.height || !Finite(kilogramsPerM3))
		throw std::invalid_argument("invalid OmniAtmosphere mass source");
	auto &cell = state[Index(x, y)];
	const auto ambient = AmbientState();
	const double ambientSpecificEnergy = ambient.totalEnergy / ambient.density;
	cell.density += kilogramsPerM3;
	cell.totalEnergy += kilogramsPerM3 * ambientSpecificEnergy;
	const double volume = config.scale.cellVolumeM3();
	pendingSourceMassKg += kilogramsPerM3 * volume;
	pendingSourceEnergyJ += kilogramsPerM3 * ambientSpecificEnergy * volume;
	pendingEvent = true;
}

void OmniAtmosphere::SetCell(std::size_t x, std::size_t y, OmniAtmosphereConservative value)
{
	if (x >= config.width || y >= config.height)
		throw std::out_of_range("OmniAtmosphere state coordinate");
	// SetCell defines an initial/debug state. Its normalization is not a solver
	// correction and the resulting value becomes the next ledger's baseline.
	ApplyFloors(value, false);
	state[Index(x, y)] = value;
	pendingEvent = true;
}

void OmniAtmosphere::ImportLegacyProjection(std::size_t x, std::size_t y, OmniAtmosphereConservative value)
{
	if (x >= config.width || y >= config.height)
		throw std::out_of_range("OmniAtmosphere Legacy projection coordinate");
	const auto index = Index(x, y);
	const auto &old = state[index];
	auto close = [](double left, double right, double relative, double absolute) {
		return std::abs(left - right) <= absolute + relative * std::max(std::abs(left), std::abs(right));
	};
	if (close(old.density, value.density, 5.0e-6, 1.0e-12) &&
		close(old.momentumX, value.momentumX, 5.0e-6, 1.0e-9) &&
		close(old.momentumY, value.momentumY, 5.0e-6, 1.0e-9) &&
		close(old.totalEnergy, value.totalEnergy, 5.0e-6, 1.0e-6))
	{
		return;
	}
	// Legacy edits are external source normalization. The normalized delta below
	// is recorded as source mass/momentum/energy, not as a numerical solver clamp.
	ApplyFloors(value, false);
	const double volume = config.scale.cellVolumeM3();
	pendingSourceMassKg += (value.density - old.density) * volume;
	pendingSourceMomentumX += (value.momentumX - old.momentumX) * volume;
	pendingSourceMomentumY += (value.momentumY - old.momentumY) * volume;
	pendingSourceEnergyJ += (value.totalEnergy - old.totalEnergy) * volume;
	state[index] = value;
	pendingEvent = true;
}

OmniAtmosphereConservative OmniAtmosphere::AmbientState() const
{
	const double pressure = config.referenceDensity * config.gasConstant * config.referenceTemperature;
	return {
		config.referenceDensity,
		0.0,
		0.0,
		pressure / (config.gamma - 1.0),
	};
}

OmniAtmospherePrimitive OmniAtmosphere::Derive(const OmniAtmosphereConservative &value) const
{
	OmniAtmospherePrimitive primitive{};
	primitive.density = value.density;
	if (!Finite(value.density) || !Finite(value.momentumX) || !Finite(value.momentumY) ||
		!Finite(value.totalEnergy) || value.density <= 0.0)
	{
		return primitive;
	}
	primitive.velocityX = value.momentumX / value.density;
	primitive.velocityY = value.momentumY / value.density;
	const double kinetic = 0.5 * value.density * (Square(primitive.velocityX) + Square(primitive.velocityY));
	const double internal = value.totalEnergy - kinetic;
	primitive.pressure = (config.gamma - 1.0) * internal;
	if (!Finite(primitive.velocityX) || !Finite(primitive.velocityY) ||
		!Finite(primitive.pressure) || primitive.pressure <= 0.0)
	{
		return primitive;
	}
	primitive.temperature = primitive.pressure / (value.density * config.gasConstant);
	primitive.soundSpeed = std::sqrt(config.gamma * primitive.pressure / value.density);
	primitive.finite = Finite(primitive.temperature) && primitive.temperature > 0.0 && Finite(primitive.soundSpeed);
	return primitive;
}

OmniAtmosphere::Flux OmniAtmosphere::PhysicalFlux(const OmniAtmosphereConservative &value, bool xDirection) const
{
	const auto primitive = Derive(value);
	if (!primitive.finite)
		return {};
	if (xDirection)
	{
		return {
			value.momentumX,
			value.momentumX * primitive.velocityX + primitive.pressure,
			value.momentumY * primitive.velocityX,
			(value.totalEnergy + primitive.pressure) * primitive.velocityX,
		};
	}
	return {
		value.momentumY,
		value.momentumX * primitive.velocityY,
		value.momentumY * primitive.velocityY + primitive.pressure,
		(value.totalEnergy + primitive.pressure) * primitive.velocityY,
	};
}

OmniAtmosphere::Flux OmniAtmosphere::RusanovFlux(
	const OmniAtmosphereConservative &left,
	const OmniAtmosphereConservative &right,
	bool xDirection,
	bool acoustic) const
{
	const auto leftPrimitive = Derive(left);
	const auto rightPrimitive = Derive(right);
	if (!leftPrimitive.finite || !rightPrimitive.finite)
		return {};
	const auto leftFlux = PhysicalFlux(left, xDirection);
	const auto rightFlux = PhysicalFlux(right, xDirection);
	const double leftNormalVelocity = xDirection ? leftPrimitive.velocityX : leftPrimitive.velocityY;
	const double rightNormalVelocity = xDirection ? rightPrimitive.velocityX : rightPrimitive.velocityY;
	const double leftSignal = std::abs(leftNormalVelocity) + (acoustic ? leftPrimitive.soundSpeed : 0.0);
	const double rightSignal = std::abs(rightNormalVelocity) + (acoustic ? rightPrimitive.soundSpeed : 0.0);
	const double signal = std::max(leftSignal, rightSignal);
	return {
		0.5 * (leftFlux.density + rightFlux.density) - 0.5 * signal * (right.density - left.density),
		0.5 * (leftFlux.momentumX + rightFlux.momentumX) - 0.5 * signal * (right.momentumX - left.momentumX),
		0.5 * (leftFlux.momentumY + rightFlux.momentumY) - 0.5 * signal * (right.momentumY - left.momentumY),
		0.5 * (leftFlux.totalEnergy + rightFlux.totalEnergy) - 0.5 * signal * (right.totalEnergy - left.totalEnergy),
	};
}

void OmniAtmosphere::RecordBoundaryFlux(const Flux &flux, bool, bool positiveOutward, double dt)
{
	const double orientation = positiveOutward ? 1.0 : -1.0;
	const double mass = orientation * flux.density * dt * config.scale.faceAreaM2();
	const double energy = orientation * flux.totalEnergy * dt * config.scale.faceAreaM2();
	ledger.boundaryMomentumXOut += orientation * flux.momentumX * dt * config.scale.faceAreaM2();
	ledger.boundaryMomentumYOut += orientation * flux.momentumY * dt * config.scale.faceAreaM2();
	if (mass >= 0.0)
		ledger.boundaryMassOutKg += mass;
	else
		ledger.boundaryMassInKg -= mass;
	if (energy >= 0.0)
		ledger.boundaryEnergyOutJ += energy;
	else
		ledger.boundaryEnergyInJ -= energy;
}

bool OmniAtmosphere::CompressibleFeaturesPresent() const
{
	auto differs = [](double left, double right, double relative, double absolute) {
		return std::abs(left - right) > absolute + relative * std::max(std::abs(left), std::abs(right));
	};
	for (std::size_t y = 0; y < config.height; ++y)
	{
		for (std::size_t x = 0; x < config.width; ++x)
		{
			const auto index = Index(x, y);
			if (blocked[index])
				continue;
			const auto current = Derive(state[index]);
			if (!current.finite)
				return true;
			const double wallVelocityTolerance = 1.0e-6;
			if (config.boundary == OmniAtmosphereBoundary::Sealed &&
				(((x == 0 || x + 1 == config.width) && std::abs(current.velocityX) > wallVelocityTolerance) ||
				 ((y == 0 || y + 1 == config.height) && std::abs(current.velocityY) > wallVelocityTolerance)))
			{
				return true;
			}
			auto compare = [&](std::size_t nx, std::size_t ny) {
				const auto neighbourIndex = Index(nx, ny);
				if (blocked[neighbourIndex])
				{
					const bool xFace = nx != x;
					const double normalVelocity = xFace ? current.velocityX : current.velocityY;
					return std::abs(normalVelocity) > wallVelocityTolerance;
				}
				const auto neighbour = Derive(state[neighbourIndex]);
				return !neighbour.finite ||
					differs(current.pressure, neighbour.pressure, 1.0e-8, 1.0e-5) ||
					differs(current.density, neighbour.density, 1.0e-8, 1.0e-12) ||
					differs(current.velocityX, neighbour.velocityX, 1.0e-8, 1.0e-6) ||
					differs(current.velocityY, neighbour.velocityY, 1.0e-8, 1.0e-6);
			};
			if (x + 1 < config.width && compare(x + 1, y))
				return true;
			if (y + 1 < config.height && compare(x, y + 1))
				return true;
		}
	}
	return false;
}

void OmniAtmosphere::AdvanceOnce(double dt, bool acoustic)
{
	const std::size_t width = config.width;
	const std::size_t height = config.height;
	std::vector<Flux> fluxX((width + 1) * height);
	std::vector<Flux> fluxY(width * (height + 1));
	const auto ambient = AmbientState();
	auto reflect = [](OmniAtmosphereConservative value, bool xDirection) {
		if (xDirection)
			value.momentumX = -value.momentumX;
		else
			value.momentumY = -value.momentumY;
		return value;
	};

	for (std::size_t y = 0; y < height; ++y)
	{
		for (std::size_t faceX = 0; faceX <= width; ++faceX)
		{
			Flux flux{};
			if (faceX == 0 || faceX == width)
			{
				if (config.boundary == OmniAtmosphereBoundary::Periodic)
				{
					const bool leftBlocked = blocked[Index(width - 1, y)] != 0;
					const bool rightBlocked = blocked[Index(0, y)] != 0;
					if (!leftBlocked && !rightBlocked)
					{
						flux = RusanovFlux(state[Index(width - 1, y)], state[Index(0, y)], true, acoustic);
					}
					else if (leftBlocked != rightBlocked)
					{
						// A periodic seam is still a physical face when one side is a
						// blocked cell. Mirror the fluid state exactly as for an internal
						// wall; a zero flux would create a one-sided pressure impulse.
						if (leftBlocked)
						{
							flux = RusanovFlux(reflect(state[Index(0, y)], true), state[Index(0, y)], true, acoustic);
							if (faceX == 0)
								RecordBoundaryFlux(flux, true, false, dt);
						}
						else
						{
							flux = RusanovFlux(state[Index(width - 1, y)], reflect(state[Index(width - 1, y)], true), true, acoustic);
							if (faceX == 0)
								RecordBoundaryFlux(flux, true, true, dt);
						}
					}
				}
				else if (config.boundary == OmniAtmosphereBoundary::Open)
				{
					const auto cellX = faceX == 0 ? 0U : width - 1;
					if (!blocked[Index(cellX, y)])
					{
						flux = faceX == 0
							? RusanovFlux(ambient, state[Index(0, y)], true, acoustic)
							: RusanovFlux(state[Index(width - 1, y)], ambient, true, acoustic);
						RecordBoundaryFlux(flux, true, faceX == width, dt);
					}
				}
				else
				{
					const auto cellX = faceX == 0 ? 0U : width - 1;
					if (!blocked[Index(cellX, y)])
					{
						const auto &cell = state[Index(cellX, y)];
						flux = faceX == 0
							? RusanovFlux(reflect(cell, true), cell, true, acoustic)
							: RusanovFlux(cell, reflect(cell, true), true, acoustic);
						RecordBoundaryFlux(flux, true, faceX == width, dt);
					}
				}
			}
			else if (!blocked[Index(faceX - 1, y)] && !blocked[Index(faceX, y)])
			{
				flux = RusanovFlux(state[Index(faceX - 1, y)], state[Index(faceX, y)], true, acoustic);
			}
			else if (!blocked[Index(faceX - 1, y)])
			{
				const auto &cell = state[Index(faceX - 1, y)];
				flux = RusanovFlux(cell, reflect(cell, true), true, acoustic);
				RecordBoundaryFlux(flux, true, true, dt);
			}
			else if (!blocked[Index(faceX, y)])
			{
				const auto &cell = state[Index(faceX, y)];
				flux = RusanovFlux(reflect(cell, true), cell, true, acoustic);
				RecordBoundaryFlux(flux, true, false, dt);
			}
			fluxX[y * (width + 1) + faceX] = flux;
		}
	}

	for (std::size_t faceY = 0; faceY <= height; ++faceY)
	{
		for (std::size_t x = 0; x < width; ++x)
		{
			Flux flux{};
			if (faceY == 0 || faceY == height)
			{
				if (config.boundary == OmniAtmosphereBoundary::Periodic)
				{
					const bool topBlocked = blocked[Index(x, height - 1)] != 0;
					const bool bottomBlocked = blocked[Index(x, 0)] != 0;
					if (!topBlocked && !bottomBlocked)
					{
						flux = RusanovFlux(state[Index(x, height - 1)], state[Index(x, 0)], false, acoustic);
					}
					else if (topBlocked != bottomBlocked)
					{
						if (topBlocked)
						{
							flux = RusanovFlux(reflect(state[Index(x, 0)], false), state[Index(x, 0)], false, acoustic);
							if (faceY == 0)
								RecordBoundaryFlux(flux, false, false, dt);
						}
						else
						{
							flux = RusanovFlux(state[Index(x, height - 1)], reflect(state[Index(x, height - 1)], false), false, acoustic);
							if (faceY == 0)
								RecordBoundaryFlux(flux, false, true, dt);
						}
					}
				}
				else if (config.boundary == OmniAtmosphereBoundary::Open)
				{
					const auto cellY = faceY == 0 ? 0U : height - 1;
					if (!blocked[Index(x, cellY)])
					{
						flux = faceY == 0
							? RusanovFlux(ambient, state[Index(x, 0)], false, acoustic)
							: RusanovFlux(state[Index(x, height - 1)], ambient, false, acoustic);
						RecordBoundaryFlux(flux, false, faceY == height, dt);
					}
				}
				else
				{
					const auto cellY = faceY == 0 ? 0U : height - 1;
					if (!blocked[Index(x, cellY)])
					{
						const auto &cell = state[Index(x, cellY)];
						flux = faceY == 0
							? RusanovFlux(reflect(cell, false), cell, false, acoustic)
							: RusanovFlux(cell, reflect(cell, false), false, acoustic);
						RecordBoundaryFlux(flux, false, faceY == height, dt);
					}
				}
			}
			else if (!blocked[Index(x, faceY - 1)] && !blocked[Index(x, faceY)])
			{
				flux = RusanovFlux(state[Index(x, faceY - 1)], state[Index(x, faceY)], false, acoustic);
			}
			else if (!blocked[Index(x, faceY - 1)])
			{
				const auto &cell = state[Index(x, faceY - 1)];
				flux = RusanovFlux(cell, reflect(cell, false), false, acoustic);
				RecordBoundaryFlux(flux, false, true, dt);
			}
			else if (!blocked[Index(x, faceY)])
			{
				const auto &cell = state[Index(x, faceY)];
				flux = RusanovFlux(reflect(cell, false), cell, false, acoustic);
				RecordBoundaryFlux(flux, false, false, dt);
			}
			fluxY[faceY * width + x] = flux;
		}
	}

	const double factor = dt / config.scale.cellLengthM;
	for (std::size_t y = 0; y < height; ++y)
	{
		for (std::size_t x = 0; x < width; ++x)
		{
			const auto index = Index(x, y);
			if (blocked[index])
			{
				next[index] = state[index];
				continue;
			}
			const auto &left = fluxX[y * (width + 1) + x];
			const auto &right = fluxX[y * (width + 1) + x + 1];
			const auto &top = fluxY[y * width + x];
			const auto &bottom = fluxY[(y + 1) * width + x];
			auto value = state[index];
			value.density -= factor * ((right.density - left.density) + (bottom.density - top.density));
			value.momentumX -= factor * ((right.momentumX - left.momentumX) + (bottom.momentumX - top.momentumX));
			value.momentumY -= factor * ((right.momentumY - left.momentumY) + (bottom.momentumY - top.momentumY));
			value.totalEnergy -= factor * ((right.totalEnergy - left.totalEnergy) + (bottom.totalEnergy - top.totalEnergy));
			ApplyFloors(value);
			next[index] = value;
		}
	}
	state.swap(next);
}

void OmniAtmosphere::ApplyFloors(OmniAtmosphereConservative &value, bool recordCorrection)
{
	const double oldDensity = value.density;
	const double oldMomentumX = value.momentumX;
	const double oldMomentumY = value.momentumY;
	const double oldEnergy = value.totalEnergy;
	if (!Finite(value.density) || !Finite(value.momentumX) || !Finite(value.momentumY) || !Finite(value.totalEnergy))
	{
		if (recordCorrection)
			ledger.nonFiniteCells++;
		value = AmbientState();
	}
	if (value.density < config.densityFloor)
	{
		value.density = config.densityFloor;
		value.momentumX = 0.0;
		value.momentumY = 0.0;
		if (recordCorrection)
			ledger.densityFloorHits++;
	}
	const double kinetic = 0.5 * (Square(value.momentumX) + Square(value.momentumY)) / value.density;
	const double requiredInternal = std::max(config.internalEnergyFloor, config.pressureFloor / (config.gamma - 1.0));
	if (value.totalEnergy - kinetic < requiredInternal)
	{
		value.totalEnergy = kinetic + requiredInternal;
		if (recordCorrection)
		{
			ledger.energyFloorHits++;
			ledger.pressureFloorHits++;
		}
	}
	if (!recordCorrection)
		return;
	const double volume = config.scale.cellVolumeM3();
	if (Finite(oldDensity))
		ledger.numericalMassCorrectionKg += (value.density - oldDensity) * volume;
	if (Finite(oldMomentumX))
		ledger.numericalMomentumXCorrection += (value.momentumX - oldMomentumX) * volume;
	if (Finite(oldMomentumY))
		ledger.numericalMomentumYCorrection += (value.momentumY - oldMomentumY) * volume;
	if (Finite(oldEnergy))
		ledger.numericalEnergyCorrectionJ += (value.totalEnergy - oldEnergy) * volume;
}

void OmniAtmosphere::BeginLedger()
{
	ledger = {};
	ledger.sourceMassKg = pendingSourceMassKg;
	ledger.sourceMomentumX = pendingSourceMomentumX;
	ledger.sourceMomentumY = pendingSourceMomentumY;
	ledger.sourceEnergyJ = pendingSourceEnergyJ;
	ledger.initialMassKg = TotalMassKg() - pendingSourceMassKg;
	ledger.initialMomentumX = TotalMomentumX();
	ledger.initialMomentumX -= pendingSourceMomentumX;
	ledger.initialMomentumY = TotalMomentumY() - pendingSourceMomentumY;
	ledger.initialEnergyJ = TotalEnergyJ() - pendingSourceEnergyJ;
}

void OmniAtmosphere::FinishLedger()
{
	ledger.finalMassKg = TotalMassKg();
	ledger.finalMomentumX = TotalMomentumX();
	ledger.finalMomentumY = TotalMomentumY();
	ledger.finalEnergyJ = TotalEnergyJ();
	pendingSourceMassKg = 0.0;
	pendingSourceMomentumX = 0.0;
	pendingSourceMomentumY = 0.0;
	pendingSourceEnergyJ = 0.0;
}

void OmniAtmosphere::Step()
{
	BeginLedger();
	bool openBoundaryEvent = false;
	if (config.boundary == OmniAtmosphereBoundary::Open)
	{
		const auto ambient = AmbientState();
		auto differsFromAmbient = [&ambient](const OmniAtmosphereConservative &cell) {
			const double densityScale = std::max(std::abs(ambient.density), 1.0);
			const double energyScale = std::max(std::abs(ambient.totalEnergy), 1.0);
			return std::abs(cell.density - ambient.density) > densityScale * 1.0e-12 ||
				std::abs(cell.momentumX) > densityScale * 1.0e-12 ||
				std::abs(cell.momentumY) > densityScale * 1.0e-12 ||
				std::abs(cell.totalEnergy - ambient.totalEnergy) > energyScale * 1.0e-12;
		};
		for (std::size_t x = 0; x < config.width && !openBoundaryEvent; ++x)
			openBoundaryEvent = differsFromAmbient(state[Index(x, 0)]) || differsFromAmbient(state[Index(x, config.height - 1)]);
		for (std::size_t y = 0; y < config.height && !openBoundaryEvent; ++y)
			openBoundaryEvent = differsFromAmbient(state[Index(0, y)]) || differsFromAmbient(state[Index(config.width - 1, y)]);
	}
	const bool acoustic = config.execution == OmniAtmosphereExecution::ReferenceCompressible || pendingEvent || compressibleActive || openBoundaryEvent;
	ledger.acousticRoute = acoustic;
	double maximumSignal = 0.0;
	for (std::size_t index = 0; index < state.size(); ++index)
	{
		if (blocked[index])
			continue;
		const auto primitive = Derive(state[index]);
		if (!primitive.finite)
			continue;
		const double advective = std::max(std::abs(primitive.velocityX), std::abs(primitive.velocityY));
		maximumSignal = std::max(maximumSignal, advective + (acoustic ? primitive.soundSpeed : 0.0));
	}
	std::size_t requiredSubsteps = 1;
	if (maximumSignal > 0.0)
	{
		requiredSubsteps = static_cast<std::size_t>(std::ceil(
			maximumSignal * config.scale.timestepS / (config.cfl * config.scale.cellLengthM)));
		requiredSubsteps = std::max<std::size_t>(requiredSubsteps, 1);
	}
	const std::size_t maximum = config.execution == OmniAtmosphereExecution::ReferenceCompressible
		? config.maximumReferenceSubsteps
		: config.maximumRuntimeSubsteps;
	const std::size_t substeps = std::min(requiredSubsteps, maximum);
	ledger.timestepLimited = requiredSubsteps > maximum;
	ledger.substeps = substeps;
	ledger.requestedTimestepS = config.scale.timestepS;
	const double stableSubstep = maximumSignal > 0.0
		? config.cfl * config.scale.cellLengthM / maximumSignal
		: config.scale.timestepS;
	const double substep = ledger.timestepLimited
		? stableSubstep
		: config.scale.timestepS / static_cast<double>(substeps);
	ledger.advancedTimestepS = substep * static_cast<double>(substeps);
	for (std::size_t index = 0; index < substeps; ++index)
		AdvanceOnce(substep, acoustic);
	pendingEvent = false;
	compressibleActive = config.execution == OmniAtmosphereExecution::RuntimeLowMach && CompressibleFeaturesPresent();
	FinishLedger();
}

void OmniAtmosphere::StepReference(double timestepS)
{
	if (!Finite(timestepS) || timestepS <= 0.0)
		throw std::invalid_argument("invalid OmniAtmosphere reference timestep");
	BeginLedger();
	double maximumSignal = 0.0;
	for (std::size_t index = 0; index < state.size(); ++index)
	{
		if (blocked[index])
			continue;
		const auto primitive = Derive(state[index]);
		if (primitive.finite)
		{
			maximumSignal = std::max(maximumSignal,
				std::max(std::abs(primitive.velocityX), std::abs(primitive.velocityY)) + primitive.soundSpeed);
		}
	}
	std::size_t requiredSubsteps = std::max<std::size_t>(1, static_cast<std::size_t>(std::ceil(
		maximumSignal * timestepS / (config.cfl * config.scale.cellLengthM))));
	const std::size_t substeps = std::min(requiredSubsteps, config.maximumReferenceSubsteps);
	ledger.timestepLimited = requiredSubsteps > config.maximumReferenceSubsteps;
	ledger.substeps = substeps;
	ledger.requestedTimestepS = timestepS;
	const double stableSubstep = maximumSignal > 0.0
		? config.cfl * config.scale.cellLengthM / maximumSignal
		: timestepS;
	const double substep = ledger.timestepLimited
		? stableSubstep
		: timestepS / static_cast<double>(substeps);
	ledger.advancedTimestepS = substep * static_cast<double>(substeps);
	for (std::size_t index = 0; index < substeps; ++index)
		AdvanceOnce(substep, true);
	pendingEvent = false;
	compressibleActive = CompressibleFeaturesPresent();
	ledger.acousticRoute = true;
	FinishLedger();
}

const OmniAtmosphereConservative &OmniAtmosphere::State(std::size_t x, std::size_t y) const
{
	if (x >= config.width || y >= config.height)
		throw std::out_of_range("OmniAtmosphere state coordinate");
	return state[Index(x, y)];
}

OmniAtmospherePrimitive OmniAtmosphere::Primitive(std::size_t x, std::size_t y) const
{
	return Derive(State(x, y));
}

double OmniAtmosphere::TotalMassKg() const
{
	double total = 0.0;
	for (const auto &cell : state)
		total += cell.density;
	return total * config.scale.cellVolumeM3();
}

double OmniAtmosphere::TotalMomentumX() const
{
	double total = 0.0;
	for (const auto &cell : state)
		total += cell.momentumX;
	return total * config.scale.cellVolumeM3();
}

double OmniAtmosphere::TotalMomentumY() const
{
	double total = 0.0;
	for (const auto &cell : state)
		total += cell.momentumY;
	return total * config.scale.cellVolumeM3();
}

double OmniAtmosphere::TotalEnergyJ() const
{
	double total = 0.0;
	for (const auto &cell : state)
		total += cell.totalEnergy;
	return total * config.scale.cellVolumeM3();
}

double OmniAtmosphere::MinimumDensity() const
{
	double minimum = std::numeric_limits<double>::infinity();
	for (const auto &cell : state)
		minimum = std::min(minimum, cell.density);
	return minimum;
}

double OmniAtmosphere::MinimumPressure() const
{
	double minimum = std::numeric_limits<double>::infinity();
	for (const auto &cell : state)
		minimum = std::min(minimum, Derive(cell).pressure);
	return minimum;
}

float OmniAtmosphere::LegacyPressure(std::size_t x, std::size_t y) const
{
	const auto primitive = Primitive(x, y);
	return static_cast<float>((primitive.pressure - config.referencePressure) / config.legacyPressureScalePa);
}

float OmniAtmosphere::LegacyVelocityX(std::size_t x, std::size_t y) const
{
	return static_cast<float>(Primitive(x, y).velocityX);
}

float OmniAtmosphere::LegacyVelocityY(std::size_t x, std::size_t y) const
{
	return static_cast<float>(Primitive(x, y).velocityY);
}

float OmniAtmosphere::LegacyTemperature(std::size_t x, std::size_t y) const
{
	return static_cast<float>(Primitive(x, y).temperature);
}
