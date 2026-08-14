#include "OmniCompute.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <mutex>
#include <utility>

namespace
{
	struct ComputeState
	{
		OmniComputeBackend backend = OmniComputeBackend::CPU;
		OmniThermalDiffusionExecutor executor = nullptr;
		bool validationPassed = false;
		double maxAbsoluteError = 0.0;
		double maxRelativeError = 0.0;
		std::size_t firstMismatchIndex = static_cast<std::size_t>(-1);
		std::string detail = "CPU";
	};

	std::mutex computeMutex;
	ComputeState computeState;

	bool ValidInput(const OmniThermalDiffusionInput &input)
	{
		const auto cellCount = input.width * input.height;
		return input.width && input.height &&
			input.temperature.size() == cellCount &&
			input.conductivity.size() == cellCount &&
			input.blocked.size() == cellCount &&
			std::isfinite(input.timestepOverCellLengthSquared) &&
			input.timestepOverCellLengthSquared >= 0.0f;
	}

	bool Finite(float value)
	{
		return std::isfinite(value);
	}
}

namespace OmniCompute
{
const char *BackendName(OmniComputeBackend backend)
{
	switch (backend)
	{
	case OmniComputeBackend::CPU: return "CPU";
	case OmniComputeBackend::SDL_GPU_D3D12: return "SDL_GPU D3D12";
	case OmniComputeBackend::SDL_GPU_Vulkan: return "SDL_GPU Vulkan";
	case OmniComputeBackend::CUDA: return "CUDA";
	}
	return "unknown";
}

bool IsGPUAvailable()
{
	std::scoped_lock lock(computeMutex);
	return computeState.executor != nullptr && computeState.backend != OmniComputeBackend::CPU;
}

OmniComputeBackend GetComputeBackend()
{
	std::scoped_lock lock(computeMutex);
	return computeState.backend;
}

OmniComputeStatus GetStatus()
{
	std::scoped_lock lock(computeMutex);
	return {
		.backend = computeState.backend,
		.available = computeState.executor != nullptr && computeState.backend != OmniComputeBackend::CPU,
		.validationPassed = computeState.validationPassed,
		.maxAbsoluteError = computeState.maxAbsoluteError,
		.maxRelativeError = computeState.maxRelativeError,
		.firstMismatchIndex = computeState.firstMismatchIndex,
		.detail = computeState.detail,
	};
}

void ConfigureBackend(OmniComputeBackend backend, OmniThermalDiffusionExecutor executor, std::string detail)
{
	std::scoped_lock lock(computeMutex);
	computeState.backend = executor ? backend : OmniComputeBackend::CPU;
	computeState.executor = executor;
	computeState.validationPassed = false;
	computeState.maxAbsoluteError = 0.0;
	computeState.maxRelativeError = 0.0;
	computeState.firstMismatchIndex = static_cast<std::size_t>(-1);
	computeState.detail = std::move(detail);
}

void ResetBackend(std::string detail)
{
	std::scoped_lock lock(computeMutex);
	computeState = {
		.backend = OmniComputeBackend::CPU,
		.executor = nullptr,
		.validationPassed = false,
		.detail = std::move(detail),
	};
}

std::vector<float> ComputeThermalDiffusionReference(const OmniThermalDiffusionInput &input)
{
	if (!ValidInput(input))
		return {};
	const auto cellCount = input.width * input.height;
	std::vector<float> result(cellCount, 0.0f);
	auto applyNeighbour = [&](std::size_t cell, std::size_t neighbour) {
		if (input.blocked[cell] || input.blocked[neighbour])
			return;
		const auto temperature = input.temperature[cell];
		const auto neighbourTemperature = input.temperature[neighbour];
		const auto conductivity = 0.5f * (input.conductivity[cell] + input.conductivity[neighbour]);
		if (!Finite(temperature) || !Finite(neighbourTemperature) || !Finite(conductivity) || conductivity < 0.0f)
			return;
		result[cell] += input.timestepOverCellLengthSquared * conductivity *
			(neighbourTemperature - temperature);
	};
	for (std::size_t y = 0; y < input.height; ++y)
	{
		for (std::size_t x = 0; x < input.width; ++x)
		{
			const auto cell = y * input.width + x;
			if (x > 0)
				applyNeighbour(cell, cell - 1);
			else if (input.periodic)
				applyNeighbour(cell, y * input.width + input.width - 1);
			if (x + 1 < input.width)
				applyNeighbour(cell, cell + 1);
			else if (input.periodic)
				applyNeighbour(cell, y * input.width);
			if (y > 0)
				applyNeighbour(cell, cell - input.width);
			else if (input.periodic)
				applyNeighbour(cell, (input.height - 1) * input.width + x);
			if (y + 1 < input.height)
				applyNeighbour(cell, cell + input.width);
			else if (input.periodic)
				applyNeighbour(cell, x);
		}
	}
	return result;
}

bool RunThermalDiffusion(const OmniThermalDiffusionInput &input, std::vector<float> &energyDelta,
	std::string &error, float absoluteEpsilon, float relativeEpsilon)
{
	if (!ValidInput(input))
	{
		error = "invalid_thermal_diffusion_input";
		return false;
	}
	OmniThermalDiffusionExecutor executor = nullptr;
	{
		std::scoped_lock lock(computeMutex);
		executor = computeState.executor;
	}
	if (!executor)
	{
		error = "gpu_unavailable";
		return false;
	}
	const auto reference = ComputeThermalDiffusionReference(input);
	std::vector<float> candidate;
	if (!executor(input, candidate, error))
		return false;
	if (candidate.size() != reference.size())
	{
		error = "gpu_result_size_mismatch";
		return false;
	}
	for (std::size_t index = 0; index < reference.size(); ++index)
	{
		if (!Finite(candidate[index]))
		{
			error = "gpu_result_nonfinite";
			return false;
		}
		const auto tolerance = absoluteEpsilon + relativeEpsilon * std::abs(reference[index]);
		const auto absoluteError = std::abs(static_cast<double>(candidate[index]) - reference[index]);
		const auto relativeError = absoluteError / std::max(
			static_cast<double>(absoluteEpsilon), std::abs(static_cast<double>(reference[index])));
		{
			std::scoped_lock lock(computeMutex);
			computeState.maxAbsoluteError = std::max(computeState.maxAbsoluteError, absoluteError);
			computeState.maxRelativeError = std::max(computeState.maxRelativeError, relativeError);
		}
		if (absoluteError > tolerance)
		{
			std::scoped_lock lock(computeMutex);
			if (computeState.firstMismatchIndex == static_cast<std::size_t>(-1))
				computeState.firstMismatchIndex = index;
			error = "cpu_gpu_thermal_mismatch";
			return false;
		}
	}
	energyDelta = std::move(candidate);
	{
		std::scoped_lock lock(computeMutex);
		computeState.validationPassed = true;
	}
	return true;
}

bool RunGPUValidation(std::string *validationError)
{
	constexpr std::uint32_t seed = 0x1A2B3C4Du;
	constexpr std::array dimensions{
		std::pair<std::size_t, std::size_t>{ 8, 8 },
		std::pair<std::size_t, std::size_t>{ 31, 17 },
		std::pair<std::size_t, std::size_t>{ 64, 64 },
		std::pair<std::size_t, std::size_t>{ 127, 73 },
	};
	for (std::size_t pattern = 0; pattern < 5; ++pattern)
	{
		for (const auto [width, height] : dimensions)
		{
			const auto cellCount = width * height;
			std::vector<float> temperature(cellCount, 300.0f);
			std::vector<float> conductivity(cellCount, 0.025f);
			std::vector<unsigned char> blocked(cellCount, 0);
			std::uint32_t random = seed;
			for (std::size_t index = 0; index < cellCount; ++index)
			{
				const auto x = index % width;
				const auto y = index / width;
				switch (pattern)
				{
				case 0: break;
				case 1:
					if (index == cellCount / 2)
						temperature[index] = 1800.0f;
					break;
				case 2:
					temperature[index] = 250.0f + 750.0f * static_cast<float>(x) /
						static_cast<float>(std::max<std::size_t>(1, width - 1));
					conductivity[index] = 0.005f + 0.05f * static_cast<float>(y) /
						static_cast<float>(std::max<std::size_t>(1, height - 1));
					break;
				case 3:
					random = random * 1664525u + 1013904223u;
					temperature[index] = 100.0f + static_cast<float>(random & 0xFFFFu) * (1900.0f / 65535.0f);
					random = random * 1664525u + 1013904223u;
					conductivity[index] = 0.001f + static_cast<float>(random & 0xFFFFu) * (0.099f / 65535.0f);
					blocked[index] = (random % 37u == 0u) ? 1u : 0u;
					break;
				case 4:
					temperature[index] = (x == 0 || y == 0 || x + 1 == width || y + 1 == height) ? 5000.0f : 1.0f;
					conductivity[index] = (index & 1u) ? 0.1f : 0.0001f;
					break;
				}
			}
			const OmniThermalDiffusionInput input{
				.width = width,
				.height = height,
				.periodic = pattern != 4,
				.timestepOverCellLengthSquared = pattern == 4 ? 0.01f : 0.25f,
				.temperature = temperature,
				.conductivity = conductivity,
				.blocked = blocked,
			};
			std::vector<float> output;
			std::string error;
			if (!RunThermalDiffusion(input, output, error))
			{
				if (validationError)
					*validationError = error;
				return false;
			}
		}
	}
	if (validationError)
		validationError->clear();
	return true;
}
}
