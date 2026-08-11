#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <limits>
#include <string_view>
#include <vector>

#if defined(OMNI_PRECISION_DOUBLE_STRICT)
using Real = double;
constexpr std::string_view PrecisionMode = "strict_double";
constexpr bool FastMathContract = false;
#elif defined(OMNI_PRECISION_FLOAT_STRICT)
using Real = float;
constexpr std::string_view PrecisionMode = "strict_float";
constexpr bool FastMathContract = false;
#elif defined(OMNI_PRECISION_FLOAT_FAST)
using Real = float;
constexpr std::string_view PrecisionMode = "fast_float";
constexpr bool FastMathContract = true;
#else
#error Precision mode macro is required
#endif

namespace
{

struct State
{
	Real density = 0;
	Real momentum = 0;
	Real energy = 0;
};

struct Primitive
{
	Real density = 0;
	Real velocity = 0;
	Real pressure = 0;
	Real soundSpeed = 0;
	bool valid = false;
};

struct CaseResult
{
	double massDrift = 0;
	double momentumDrift = 0;
	double energyDrift = 0;
	double minimumDensity = 0;
	double minimumPressure = 0;
	double maximumPressure = 0;
	double densitySignature = 0;
	double momentumSignature = 0;
	double energySignature = 0;
	double maximumCfl = 0;
	bool finite = false;
	bool positive = false;
	bool passed = false;
};

State Add(const State &left, const State &right)
{
	return {left.density + right.density, left.momentum + right.momentum,
		left.energy + right.energy};
}

State Subtract(const State &left, const State &right)
{
	return {left.density - right.density, left.momentum - right.momentum,
		left.energy - right.energy};
}

State Scale(Real factor, const State &state)
{
	return {factor * state.density, factor * state.momentum, factor * state.energy};
}

Primitive ToPrimitive(const State &state, Real gamma)
{
	Primitive primitive;
	primitive.density = state.density;
	if (!(state.density > Real(0)) || !std::isfinite(state.density)
		|| !std::isfinite(state.momentum) || !std::isfinite(state.energy))
		return primitive;
	primitive.velocity = state.momentum / state.density;
	const Real kinetic = Real(0.5) * state.density * primitive.velocity * primitive.velocity;
	primitive.pressure = (gamma - Real(1)) * (state.energy - kinetic);
	if (!(primitive.pressure > Real(0)) || !std::isfinite(primitive.pressure))
		return primitive;
	primitive.soundSpeed = std::sqrt(gamma * primitive.pressure / primitive.density);
	primitive.valid = std::isfinite(primitive.velocity) && std::isfinite(primitive.soundSpeed);
	return primitive;
}

State FromPrimitive(Real density, Real velocity, Real pressure, Real gamma)
{
	return {density, density * velocity,
		pressure / (gamma - Real(1)) + Real(0.5) * density * velocity * velocity};
}

State PhysicalFlux(const State &state, const Primitive &primitive)
{
	return {state.momentum,
		state.momentum * primitive.velocity + primitive.pressure,
		primitive.velocity * (state.energy + primitive.pressure)};
}

State RusanovFlux(const State &left, const State &right, Real gamma, bool &valid)
{
	const auto leftPrimitive = ToPrimitive(left, gamma);
	const auto rightPrimitive = ToPrimitive(right, gamma);
	valid = leftPrimitive.valid && rightPrimitive.valid;
	if (!valid)
		return {};
	const Real waveSpeed = std::max(std::abs(leftPrimitive.velocity) + leftPrimitive.soundSpeed,
		std::abs(rightPrimitive.velocity) + rightPrimitive.soundSpeed);
	return Subtract(Scale(Real(0.5), Add(PhysicalFlux(left, leftPrimitive),
		PhysicalFlux(right, rightPrimitive))),
		Scale(Real(0.5) * waveSpeed, Subtract(right, left)));
}

State Total(const std::vector<State> &cells)
{
	State total;
	for (const auto &cell : cells)
		total = Add(total, cell);
	return total;
}

CaseResult EvolvePeriodic(std::vector<State> cells, std::size_t steps, Real dt,
	Real dx, Real gamma)
{
	CaseResult result;
	const auto initial = Total(cells);
	std::vector<State> next(cells.size());
	std::vector<State> flux(cells.size());
	const Real lambda = dt / dx;
	for (std::size_t step = 0; step < steps; ++step)
	{
		for (std::size_t face = 0; face < cells.size(); ++face)
		{
			bool valid = false;
			flux[face] = RusanovFlux(cells[face], cells[(face + 1) % cells.size()], gamma, valid);
			if (!valid)
				return result;
		}
		for (std::size_t cell = 0; cell < cells.size(); ++cell)
		{
			const auto leftFace = (cell + cells.size() - 1) % cells.size();
			next[cell] = Subtract(cells[cell], Scale(lambda, Subtract(flux[cell], flux[leftFace])));
			const auto primitive = ToPrimitive(next[cell], gamma);
			if (!primitive.valid)
				return result;
			result.maximumCfl = std::max(result.maximumCfl, static_cast<double>(lambda
				* (std::abs(primitive.velocity) + primitive.soundSpeed)));
		}
		cells.swap(next);
	}
	const auto final = Total(cells);
	result.massDrift = static_cast<double>(final.density - initial.density);
	result.momentumDrift = static_cast<double>(final.momentum - initial.momentum);
	result.energyDrift = static_cast<double>(final.energy - initial.energy);
	result.minimumDensity = std::numeric_limits<double>::infinity();
	result.minimumPressure = std::numeric_limits<double>::infinity();
	result.maximumPressure = 0;
	result.finite = true;
	result.positive = true;
	for (std::size_t index = 0; index < cells.size(); ++index)
	{
		const auto primitive = ToPrimitive(cells[index], gamma);
		if (!primitive.valid)
		{
			result.finite = false;
			result.positive = false;
			break;
		}
		result.minimumDensity = std::min(result.minimumDensity,
			static_cast<double>(primitive.density));
		result.minimumPressure = std::min(result.minimumPressure,
			static_cast<double>(primitive.pressure));
		result.maximumPressure = std::max(result.maximumPressure,
			static_cast<double>(primitive.pressure));
		result.positive = result.positive && primitive.density > Real(0)
			&& primitive.pressure > Real(0);
		const double weight = static_cast<double>(index + 1);
		result.densitySignature += weight * static_cast<double>(cells[index].density);
		result.momentumSignature += weight * static_cast<double>(cells[index].momentum);
		result.energySignature += weight * static_cast<double>(cells[index].energy);
	}
	result.passed = result.finite && result.positive && result.maximumCfl <= 1.0;
	return result;
}

CaseResult Uniform()
{
	constexpr std::size_t cells = 64;
	constexpr Real gamma = Real(5) / Real(3);
	std::vector<State> state(cells, FromPrimitive(Real(1), Real(0.1), Real(1), gamma));
	return EvolvePeriodic(std::move(state), 64, Real(0.02), Real(1), gamma);
}

CaseResult PressurePulse()
{
	constexpr std::size_t cells = 128;
	constexpr Real gamma = Real(5) / Real(3);
	std::vector<State> state(cells);
	for (std::size_t cell = 0; cell < cells; ++cell)
	{
		const Real x = static_cast<Real>(cell) - Real(cells) / Real(2);
		const Real pressure = Real(1) + Real(0.1) * std::exp(-(x * x) / Real(64));
		state[cell] = FromPrimitive(Real(1), Real(0), pressure, gamma);
	}
	return EvolvePeriodic(std::move(state), 64, Real(0.02), Real(1), gamma);
}

CaseResult NearVacuum()
{
	constexpr std::size_t cells = 128;
	constexpr Real gamma = Real(5) / Real(3);
	std::vector<State> state(cells);
	for (std::size_t cell = 0; cell < cells; ++cell)
		state[cell] = cell < cells / 2
			? FromPrimitive(Real(1), Real(0), Real(1), gamma)
			: FromPrimitive(Real(1e-6), Real(0), Real(1e-8), gamma);
	return EvolvePeriodic(std::move(state), 32, Real(0.02), Real(1), gamma);
}

CaseResult SodPeriodic()
{
	constexpr std::size_t cells = 256;
	constexpr Real gamma = Real(1.4);
	std::vector<State> state(cells);
	for (std::size_t cell = 0; cell < cells; ++cell)
		state[cell] = cell < cells / 2
			? FromPrimitive(Real(1), Real(0), Real(1), gamma)
			: FromPrimitive(Real(0.125), Real(0), Real(0.1), gamma);
	return EvolvePeriodic(std::move(state), 400, Real(0.0005), Real(1) / Real(cells), gamma);
}

void WriteCase(std::string_view prefix, const CaseResult &result)
{
	std::cout << prefix << "_mass_drift=" << result.massDrift << '\n';
	std::cout << prefix << "_momentum_drift=" << result.momentumDrift << '\n';
	std::cout << prefix << "_energy_drift=" << result.energyDrift << '\n';
	std::cout << prefix << "_minimum_density=" << result.minimumDensity << '\n';
	std::cout << prefix << "_minimum_pressure=" << result.minimumPressure << '\n';
	std::cout << prefix << "_maximum_pressure=" << result.maximumPressure << '\n';
	std::cout << prefix << "_density_signature=" << result.densitySignature << '\n';
	std::cout << prefix << "_momentum_signature=" << result.momentumSignature << '\n';
	std::cout << prefix << "_energy_signature=" << result.energySignature << '\n';
	std::cout << prefix << "_maximum_cfl=" << result.maximumCfl << '\n';
	std::cout << prefix << "_finite=" << (result.finite ? "true" : "false") << '\n';
	std::cout << prefix << "_positive=" << (result.positive ? "true" : "false") << '\n';
	std::cout << prefix << "_passed=" << (result.passed ? "true" : "false") << '\n';
}

} // namespace

int main()
{
	const auto uniform = Uniform();
	const auto pressurePulse = PressurePulse();
	const auto nearVacuum = NearVacuum();
	const auto sod = SodPeriodic();
	const bool passed = uniform.passed && pressurePulse.passed && nearVacuum.passed && sod.passed;
	std::cout.precision(std::numeric_limits<Real>::max_digits10);
	std::cout << "schema_version=1\n";
	std::cout << "precision_mode=" << PrecisionMode << '\n';
	std::cout << "scalar_bytes=" << sizeof(Real) << '\n';
	std::cout << "fast_math_contract=" << (FastMathContract ? "true" : "false") << '\n';
	std::cout << "algorithm=first_order_rusanov_same_template\n";
	std::cout << "case_count=4\n";
	WriteCase("uniform", uniform);
	WriteCase("pressure_pulse", pressurePulse);
	WriteCase("near_vacuum", nearVacuum);
	WriteCase("sod", sod);
	std::cout << "numerical_correction_count=0\n";
	std::cout << "precision_probe_passed=" << (passed ? "true" : "false") << '\n';
	return passed ? 0 : 1;
}
