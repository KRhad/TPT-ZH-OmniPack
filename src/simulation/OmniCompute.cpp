#include "OmniCompute.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <mutex>

namespace
{
	struct ComputeState
	{
		OmniComputeBackend backend = OmniComputeBackend::CPU;
		OmniThermalDiffusionExecutor executor = nullptr;
		bool validationPassed = false;
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
		.detail = computeState.detail,
	};
}

void ConfigureBackend(OmniComputeBackend backend, OmniThermalDiffusionExecutor executor, std::string detail)
{
	std::scoped_lock lock(computeMutex);
	computeState.backend = executor ? backend : OmniComputeBackend::CPU;
	computeState.executor = executor;
	computeState.validationPassed = false;
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
		if (std::abs(candidate[index] - reference[index]) > tolerance)
		{
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

bool RunGPUValidation()
{
	constexpr std::size_t width = 4;
	constexpr std::size_t height = 3;
	const std::vector<float> temperature{
		290.0f, 292.0f, 296.0f, 301.0f,
		288.0f, 293.0f, 297.0f, 303.0f,
		286.0f, 291.0f, 299.0f, 305.0f,
	};
	const std::vector<float> conductivity{
		0.02f, 0.021f, 0.025f, 0.03f,
		0.019f, 0.022f, 0.026f, 0.031f,
		0.018f, 0.023f, 0.027f, 0.032f,
	};
	const std::vector<unsigned char> blocked(width * height, 0);
	const OmniThermalDiffusionInput input{
		.width = width,
		.height = height,
		.periodic = true,
		.timestepOverCellLengthSquared = 0.25f,
		.temperature = temperature,
		.conductivity = conductivity,
		.blocked = blocked,
	};
	std::vector<float> output;
	std::string error;
	return RunThermalDiffusion(input, output, error);
}
}
