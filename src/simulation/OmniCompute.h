#pragma once

#include <cstddef>
#include <span>
#include <string>
#include <vector>

// The numerical solver owns the CPU reference implementation.  SDL_GPU is an
// optional execution device, never an authority: an unavailable or invalid
// device result falls back to the same CPU stencil for that step.
enum class OmniComputeBackend
{
	CPU,
	SDL_GPU_D3D12,
	SDL_GPU_Vulkan,
	CUDA,
};

struct OmniThermalDiffusionInput
{
	std::size_t width = 0;
	std::size_t height = 0;
	bool periodic = false;
	float timestepOverCellLengthSquared = 0.0f;
	std::span<const float> temperature;
	std::span<const float> conductivity;
	std::span<const unsigned char> blocked;
};

using OmniThermalDiffusionExecutor = bool (*) (
	const OmniThermalDiffusionInput &input,
	std::vector<float> &energyDelta,
	std::string &error
);

struct OmniComputeStatus
{
	OmniComputeBackend backend = OmniComputeBackend::CPU;
	bool available = false;
	bool validationPassed = false;
	double maxAbsoluteError = 0.0;
	double maxRelativeError = 0.0;
	std::size_t firstMismatchIndex = static_cast<std::size_t>(-1);
	std::string detail = "CPU";
};

namespace OmniCompute
{
	const char *BackendName(OmniComputeBackend backend);
	bool IsGPUAvailable();
	OmniComputeBackend GetComputeBackend();
	OmniComputeStatus GetStatus();

	// Called only by a platform backend after a complete device/pipeline setup.
	// Passing a null executor intentionally restores the always-safe CPU route.
	void ConfigureBackend(OmniComputeBackend backend, OmniThermalDiffusionExecutor executor,
		std::string detail);
	void ResetBackend(std::string detail);

	std::vector<float> ComputeThermalDiffusionReference(const OmniThermalDiffusionInput &input);
	bool RunThermalDiffusion(const OmniThermalDiffusionInput &input,
		std::vector<float> &energyDelta, std::string &error,
		float absoluteEpsilon = 1.0e-4f, float relativeEpsilon = 2.0e-5f);
	bool RunGPUValidation(std::string *error = nullptr);
}
