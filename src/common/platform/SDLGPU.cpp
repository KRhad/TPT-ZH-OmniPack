#include "Platform.h"
#include "SDLCompat.h"
#include "simulation/OmniCompute.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <mutex>
#include <sstream>
#include <string>
#include <iomanip>

#ifndef TPT_SDLGPU_SHADER_AVAILABLE
# define TPT_SDLGPU_SHADER_AVAILABLE 0
#endif

#if TPT_SDL3
# include <SDL3/SDL_gpu.h>
# include <SDL3/SDL_render.h>
# include <SDL3/SDL_surface.h>
# if TPT_SDLGPU_SHADER_AVAILABLE
#  include "sdlgpu_compute_spirv.h"
#  include "sdlgpu_thermal_diffusion_spirv.h"
# endif
#endif

namespace
{
constexpr int kGPUValidationPass = 0;
constexpr int kGPUUnsupported = 10;
constexpr int kSDLInitializationFailure = 11;
constexpr int kGPUDeviceCreationFailure = 12;
constexpr int kShaderUnavailable = 13;
constexpr int kComputeDispatchFailure = 14;
constexpr int kNumericalMismatch = 15;
constexpr int kReadbackFailure = 16;
constexpr int kGPUInternalFailure = 18;

std::string JsonEscape(const char *value)
{
	std::string result;
	for (const unsigned char ch : std::string(value ? value : ""))
	{
		switch (ch)
		{
		case '\\': result += "\\\\"; break;
		case '"': result += "\\\""; break;
		case '\n': result += "\\n"; break;
		case '\r': result += "\\r"; break;
		case '\t': result += "\\t"; break;
		default:
			if (ch >= 0x20)
				result += static_cast<char>(ch);
			break;
		}
	}
	return result;
}

void WriteValidationJson(const char *path, bool passed, const char *backend,
	const char *status, bool supported, bool fallback, double maxAbs = 0.0,
	double maxRel = 0.0, std::size_t firstMismatch = static_cast<std::size_t>(-1))
{
	if (!path || !*path)
		return;
	std::ofstream output(path, std::ios::binary | std::ios::trunc);
	if (!output)
		return;
	output << std::setprecision(17)
		<< "{\n"
		<< "  \"test\": \"gpu_validation\",\n"
		<< "  \"passed\": " << (passed ? "true" : "false") << ",\n"
		<< "  \"supported\": " << (supported ? "true" : "false") << ",\n"
		<< "  \"fallback\": " << (fallback ? "true" : "false") << ",\n"
		<< "  \"backend\": \"" << JsonEscape(backend ? backend : "unknown") << "\",\n"
		<< "  \"status\": \"" << JsonEscape(status ? status : "unknown") << "\",\n"
		<< "  \"max_abs_error\": " << maxAbs << ",\n"
		<< "  \"max_rel_error\": " << maxRel << ",\n"
		<< "  \"first_mismatch_index\": "
		<< (firstMismatch == static_cast<std::size_t>(-1) ? -1 : static_cast<long long>(firstMismatch)) << "\n"
		<< "}\n";
}

void WriteFallbackJson(const char *path, bool passed, const char *status)
{
	if (!path || !*path)
		return;
	std::ofstream output(path, std::ios::binary | std::ios::trunc);
	if (!output)
		return;
	output << "{\n"
		<< "  \"test\": \"cpu_fallback\",\n"
		<< "  \"passed\": " << (passed ? "true" : "false") << ",\n"
		<< "  \"backend\": \"CPU\",\n"
		<< "  \"fallback\": true,\n"
		<< "  \"status\": \"" << JsonEscape(status ? status : "unknown") << "\"\n"
		<< "}\n";
}

void WriteSimpleJson(const char *path, const char *test, bool passed, const char *status,
	const char *artifact = nullptr, bool window = false, bool frame = false,
	bool resize = false, bool fullscreen = false, bool keyboard = false,
	bool mouse = false, bool textInput = false, bool clipboard = false,
	bool screenshot = false, bool shutdown = false, bool restart = false)
{
	if (!path || !*path)
		return;
	std::ofstream output(path, std::ios::binary | std::ios::trunc);
	if (!output)
		return;
	output << "{\n  \"test\": \"" << JsonEscape(test) << "\",\n"
		<< "  \"passed\": " << (passed ? "true" : "false") << ",\n"
		<< "  \"status\": \"" << JsonEscape(status) << "\"";
	if (artifact)
		output << ",\n  \"artifact\": \"" << JsonEscape(artifact) << "\"";
	if (std::strcmp(test, "sdl3_gui") == 0)
		output << ",\n  \"window_created\": " << (window ? "true" : "false")
			<< ",\n  \"frame_rendered\": " << (frame ? "true" : "false")
			<< ",\n  \"resize\": " << (resize ? "true" : "false")
			<< ",\n  \"fullscreen_toggle\": " << (fullscreen ? "true" : "false")
			<< ",\n  \"keyboard\": " << (keyboard ? "true" : "false")
			<< ",\n  \"mouse\": " << (mouse ? "true" : "false")
			<< ",\n  \"text_input\": " << (textInput ? "true" : "false")
			<< ",\n  \"clipboard\": " << (clipboard ? "true" : "false")
			<< ",\n  \"screenshot_created\": " << (screenshot ? "true" : "false")
			<< ",\n  \"clean_shutdown\": " << (shutdown ? "true" : "false")
			<< ",\n  \"restart\": " << (restart ? "true" : "false");
	output << "\n}\n";
}

constexpr std::size_t kWordCount = 16;
constexpr std::size_t kBufferBytes = kWordCount * sizeof(std::uint32_t);

std::uint64_t HashWords(const std::array<std::uint32_t, kWordCount> &words)
{
	std::uint64_t hash = 1469598103934665603ULL;
	for (auto word : words)
	{
		for (unsigned shift = 0; shift < 32; shift += 8)
		{
			hash ^= (word >> shift) & 0xFFu;
			hash *= 1099511628211ULL;
		}
	}
	return hash;
}

void PrintValue(const char *key, const std::string &value)
{
	std::cout << key << '=' << value << '\n';
}

void PrintBool(const char *key, bool value)
{
	PrintValue(key, value ? "true" : "false");
}

#if TPT_SDL3 && TPT_SDLGPU_SHADER_AVAILABLE
SDL_GPUDevice *productionDevice = nullptr;
SDL_GPUComputePipeline *thermalDiffusionPipeline = nullptr;
std::mutex productionMutex;

struct alignas(16) ThermalCell
{
	float temperature = 0.0f;
	float conductivity = 0.0f;
	float blocked = 0.0f;
	float reserved = 0.0f;
};

struct alignas(16) ThermalOutput
{
	float energyDelta = 0.0f;
	float reserved0 = 0.0f;
	float reserved1 = 0.0f;
	float reserved2 = 0.0f;
};

struct alignas(16) ThermalParameters
{
	std::uint32_t width = 0;
	std::uint32_t height = 0;
	std::uint32_t periodic = 0;
	std::uint32_t cellCount = 0;
	float timestepOverCellLengthSquared = 0.0f;
	float reserved0 = 0.0f;
	float reserved1 = 0.0f;
	float reserved2 = 0.0f;
};

void DestroyProductionCompute()
{
	std::scoped_lock lock(productionMutex);
	OmniCompute::ResetBackend("SDL_GPU shutdown; CPU fallback");
	if (productionDevice)
	{
		SDL_WaitForGPUIdle(productionDevice);
		if (thermalDiffusionPipeline)
			SDL_ReleaseGPUComputePipeline(productionDevice, thermalDiffusionPipeline);
		thermalDiffusionPipeline = nullptr;
		SDL_DestroyGPUDevice(productionDevice);
		productionDevice = nullptr;
	}
}

bool ExecuteThermalDiffusion(const OmniThermalDiffusionInput &input,
	std::vector<float> &energyDelta, std::string &error)
{
	std::scoped_lock lock(productionMutex);
	if (!productionDevice || !thermalDiffusionPipeline)
	{
		error = "production_gpu_not_initialized";
		return false;
	}
	const auto cellCount = input.width * input.height;
	if (!cellCount || input.width > UINT32_MAX || input.height > UINT32_MAX || cellCount > UINT32_MAX)
	{
		error = "production_gpu_grid_too_large";
		return false;
	}
	std::vector<ThermalCell> upload(cellCount);
	for (std::size_t index = 0; index < cellCount; ++index)
	{
		upload[index].temperature = input.temperature[index];
		upload[index].conductivity = input.conductivity[index];
		upload[index].blocked = input.blocked[index] ? 1.0f : 0.0f;
	}
	const auto uploadBytes = cellCount * sizeof(ThermalCell);
	const auto outputBytes = cellCount * sizeof(ThermalOutput);
	if (uploadBytes > UINT32_MAX || outputBytes > UINT32_MAX)
	{
		error = "production_gpu_buffer_too_large";
		return false;
	}
	SDL_GPUBuffer *inputBuffer = nullptr;
	SDL_GPUBuffer *outputBuffer = nullptr;
	SDL_GPUTransferBuffer *uploadBuffer = nullptr;
	SDL_GPUTransferBuffer *downloadBuffer = nullptr;
	SDL_GPUFence *fence = nullptr;
	SDL_GPUCommandBuffer *commandBuffer = nullptr;
	bool commandActive = false;
	bool downloadMapped = false;
	auto cleanup = [&]() {
		if (downloadMapped)
			SDL_UnmapGPUTransferBuffer(productionDevice, downloadBuffer);
		if (commandActive && commandBuffer)
			SDL_CancelGPUCommandBuffer(commandBuffer);
		if (fence)
			SDL_ReleaseGPUFence(productionDevice, fence);
		if (outputBuffer)
			SDL_ReleaseGPUBuffer(productionDevice, outputBuffer);
		if (inputBuffer)
			SDL_ReleaseGPUBuffer(productionDevice, inputBuffer);
		if (downloadBuffer)
			SDL_ReleaseGPUTransferBuffer(productionDevice, downloadBuffer);
		if (uploadBuffer)
			SDL_ReleaseGPUTransferBuffer(productionDevice, uploadBuffer);
	};
	auto fail = [&](const char *stage) {
		error = std::string(stage) + ':' + SDL_GetError();
		cleanup();
		return false;
	};
	const SDL_GPUBufferCreateInfo inputInfo{
		SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ, static_cast<Uint32>(uploadBytes), 0,
	};
	const SDL_GPUBufferCreateInfo outputInfo{
		SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_WRITE, static_cast<Uint32>(outputBytes), 0,
	};
	const SDL_GPUTransferBufferCreateInfo uploadInfo{
		SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD, static_cast<Uint32>(uploadBytes), 0,
	};
	const SDL_GPUTransferBufferCreateInfo downloadInfo{
		SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD, static_cast<Uint32>(outputBytes), 0,
	};
	inputBuffer = SDL_CreateGPUBuffer(productionDevice, &inputInfo);
	if (!inputBuffer) return fail("create_thermal_input_buffer");
	outputBuffer = SDL_CreateGPUBuffer(productionDevice, &outputInfo);
	if (!outputBuffer) return fail("create_thermal_output_buffer");
	uploadBuffer = SDL_CreateGPUTransferBuffer(productionDevice, &uploadInfo);
	if (!uploadBuffer) return fail("create_thermal_upload_buffer");
	downloadBuffer = SDL_CreateGPUTransferBuffer(productionDevice, &downloadInfo);
	if (!downloadBuffer) return fail("create_thermal_download_buffer");
	void *uploadMemory = SDL_MapGPUTransferBuffer(productionDevice, uploadBuffer, false);
	if (!uploadMemory) return fail("map_thermal_upload_buffer");
	std::memcpy(uploadMemory, upload.data(), uploadBytes);
	SDL_UnmapGPUTransferBuffer(productionDevice, uploadBuffer);
	commandBuffer = SDL_AcquireGPUCommandBuffer(productionDevice);
	if (!commandBuffer) return fail("acquire_thermal_command_buffer");
	commandActive = true;
	auto *uploadPass = SDL_BeginGPUCopyPass(commandBuffer);
	if (!uploadPass) return fail("begin_thermal_upload_pass");
	const SDL_GPUTransferBufferLocation uploadLocation{ uploadBuffer, 0 };
	const SDL_GPUBufferRegion inputRegion{ inputBuffer, 0, static_cast<Uint32>(uploadBytes) };
	SDL_UploadToGPUBuffer(uploadPass, &uploadLocation, &inputRegion, false);
	SDL_EndGPUCopyPass(uploadPass);
	const ThermalParameters parameters{
		static_cast<std::uint32_t>(input.width),
		static_cast<std::uint32_t>(input.height),
		input.periodic ? 1u : 0u,
		static_cast<std::uint32_t>(cellCount),
		input.timestepOverCellLengthSquared,
	};
	SDL_PushGPUComputeUniformData(commandBuffer, 0, &parameters, sizeof(parameters));
	const SDL_GPUStorageBufferReadWriteBinding outputBinding{ outputBuffer, false, 0, 0, 0 };
	auto *computePass = SDL_BeginGPUComputePass(commandBuffer, nullptr, 0, &outputBinding, 1);
	if (!computePass) return fail("begin_thermal_compute_pass");
	SDL_BindGPUComputePipeline(computePass, thermalDiffusionPipeline);
	SDL_GPUBuffer *readonlyBuffers[] = { inputBuffer };
	SDL_BindGPUComputeStorageBuffers(computePass, 0, readonlyBuffers, 1);
	SDL_DispatchGPUCompute(computePass, static_cast<Uint32>((cellCount + 63) / 64), 1, 1);
	SDL_EndGPUComputePass(computePass);
	auto *downloadPass = SDL_BeginGPUCopyPass(commandBuffer);
	if (!downloadPass) return fail("begin_thermal_download_pass");
	const SDL_GPUBufferRegion outputRegion{ outputBuffer, 0, static_cast<Uint32>(outputBytes) };
	const SDL_GPUTransferBufferLocation downloadLocation{ downloadBuffer, 0 };
	SDL_DownloadFromGPUBuffer(downloadPass, &outputRegion, &downloadLocation);
	SDL_EndGPUCopyPass(downloadPass);
	fence = SDL_SubmitGPUCommandBufferAndAcquireFence(commandBuffer);
	commandActive = false;
	commandBuffer = nullptr;
	if (!fence) return fail("submit_thermal_command_buffer");
	SDL_GPUFence *fences[] = { fence };
	if (!SDL_WaitForGPUFences(productionDevice, true, fences, 1))
		return fail("wait_thermal_fence");
	void *downloadMemory = SDL_MapGPUTransferBuffer(productionDevice, downloadBuffer, false);
	if (!downloadMemory) return fail("map_thermal_download_buffer");
	downloadMapped = true;
	const auto *output = static_cast<const ThermalOutput *>(downloadMemory);
	energyDelta.resize(cellCount);
	for (std::size_t index = 0; index < cellCount; ++index)
		energyDelta[index] = output[index].energyDelta;
	SDL_UnmapGPUTransferBuffer(productionDevice, downloadBuffer);
	downloadMapped = false;
	cleanup();
	return true;
}

bool InitializeProductionCompute(std::string &error, bool forceFailure = false)
{
	std::scoped_lock lock(productionMutex);
	if (forceFailure)
	{
		error = "forced_initialization_failure";
		OmniCompute::ResetBackend("forced GPU initialization failure; CPU fallback");
		return false;
	}
	if (productionDevice && thermalDiffusionPipeline)
		return true;
	SDL_SetHint(SDL_HINT_GPU_DRIVER, "vulkan");
	productionDevice = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, false, "vulkan");
	if (!productionDevice)
	{
		error = std::string("create_device:") + SDL_GetError();
		return false;
	}
	const SDL_GPUComputePipelineCreateInfo pipelineInfo{
		sdlgpu_thermal_diffusion_spirv.data.size(),
		sdlgpu_thermal_diffusion_spirv.data.data(),
		"main",
		SDL_GPU_SHADERFORMAT_SPIRV,
		0, 0, 1, 0, 1, 1,
		64, 1, 1, 0,
	};
	thermalDiffusionPipeline = SDL_CreateGPUComputePipeline(productionDevice, &pipelineInfo);
	if (!thermalDiffusionPipeline)
	{
		error = std::string("create_thermal_pipeline:") + SDL_GetError();
		SDL_DestroyGPUDevice(productionDevice);
		productionDevice = nullptr;
		return false;
	}
	const auto *driver = SDL_GetGPUDeviceDriver(productionDevice);
	OmniCompute::ConfigureBackend(OmniComputeBackend::SDL_GPU_Vulkan,
		ExecuteThermalDiffusion, driver ? driver : "vulkan");
	return true;
}

bool ExecuteComputeProof(SDL_GPUDevice *device, std::string &error, std::uint64_t &resultHash)
{
	std::array<std::uint32_t, kWordCount> input{};
	std::array<std::uint32_t, kWordCount> expected{};
	for (std::size_t i = 0; i < kWordCount; ++i)
	{
		input[i] = static_cast<std::uint32_t>(i * 17u + 3u);
		expected[i] = input[i] * 3u + 7u;
	}

	SDL_GPUBuffer *inputBuffer = nullptr;
	SDL_GPUBuffer *outputBuffer = nullptr;
	SDL_GPUTransferBuffer *uploadBuffer = nullptr;
	SDL_GPUTransferBuffer *downloadBuffer = nullptr;
	SDL_GPUComputePipeline *pipeline = nullptr;
	SDL_GPUFence *fence = nullptr;
	SDL_GPUCommandBuffer *commandBuffer = nullptr;
	bool commandActive = false;
	bool downloadMapped = false;

	auto cleanup = [&]() {
		if (downloadMapped)
		{
			SDL_UnmapGPUTransferBuffer(device, downloadBuffer);
			downloadMapped = false;
		}
		if (commandActive && commandBuffer)
		{
			SDL_CancelGPUCommandBuffer(commandBuffer);
			commandActive = false;
		}
		if (fence)
		{
			SDL_ReleaseGPUFence(device, fence);
			fence = nullptr;
		}
		if (pipeline)
		{
			SDL_ReleaseGPUComputePipeline(device, pipeline);
			pipeline = nullptr;
		}
		if (outputBuffer)
		{
			SDL_ReleaseGPUBuffer(device, outputBuffer);
			outputBuffer = nullptr;
		}
		if (inputBuffer)
		{
			SDL_ReleaseGPUBuffer(device, inputBuffer);
			inputBuffer = nullptr;
		}
		if (downloadBuffer)
		{
			SDL_ReleaseGPUTransferBuffer(device, downloadBuffer);
			downloadBuffer = nullptr;
		}
		if (uploadBuffer)
		{
			SDL_ReleaseGPUTransferBuffer(device, uploadBuffer);
			uploadBuffer = nullptr;
		}
	};
	auto fail = [&](const char *stage) {
		std::ostringstream message;
		message << stage << ':' << SDL_GetError();
		error = message.str();
		cleanup();
		return false;
	};

	const SDL_GPUBufferCreateInfo inputInfo{
		SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ,
		static_cast<Uint32>(kBufferBytes),
		0,
	};
	inputBuffer = SDL_CreateGPUBuffer(device, &inputInfo);
	if (!inputBuffer)
		return fail("create_input_buffer");

	const SDL_GPUBufferCreateInfo outputInfo{
		SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_WRITE,
		static_cast<Uint32>(kBufferBytes),
		0,
	};
	outputBuffer = SDL_CreateGPUBuffer(device, &outputInfo);
	if (!outputBuffer)
		return fail("create_output_buffer");

	const SDL_GPUTransferBufferCreateInfo uploadInfo{
		SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
		static_cast<Uint32>(kBufferBytes),
		0,
	};
	uploadBuffer = SDL_CreateGPUTransferBuffer(device, &uploadInfo);
	if (!uploadBuffer)
		return fail("create_upload_buffer");

	const SDL_GPUTransferBufferCreateInfo downloadInfo{
		SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD,
		static_cast<Uint32>(kBufferBytes),
		0,
	};
	downloadBuffer = SDL_CreateGPUTransferBuffer(device, &downloadInfo);
	if (!downloadBuffer)
		return fail("create_download_buffer");

	void *uploadMemory = SDL_MapGPUTransferBuffer(device, uploadBuffer, false);
	if (!uploadMemory)
		return fail("map_upload_buffer");
	std::memcpy(uploadMemory, input.data(), kBufferBytes);
	SDL_UnmapGPUTransferBuffer(device, uploadBuffer);

	const SDL_GPUComputePipelineCreateInfo pipelineInfo{
		sdlgpu_compute_spirv.data.size(),
		sdlgpu_compute_spirv.data.data(),
		"main",
		SDL_GPU_SHADERFORMAT_SPIRV,
		0,
		0,
		1,
		0,
		1,
		0,
		64,
		1,
		1,
		0,
	};
	pipeline = SDL_CreateGPUComputePipeline(device, &pipelineInfo);
	if (!pipeline)
		return fail("create_compute_pipeline");

	commandBuffer = SDL_AcquireGPUCommandBuffer(device);
	if (!commandBuffer)
		return fail("acquire_command_buffer");
	commandActive = true;

	auto *uploadPass = SDL_BeginGPUCopyPass(commandBuffer);
	if (!uploadPass)
		return fail("begin_upload_pass");
	const SDL_GPUTransferBufferLocation uploadLocation{ uploadBuffer, 0 };
	const SDL_GPUBufferRegion inputRegion{ inputBuffer, 0, static_cast<Uint32>(kBufferBytes) };
	SDL_UploadToGPUBuffer(uploadPass, &uploadLocation, &inputRegion, false);
	SDL_EndGPUCopyPass(uploadPass);

	const SDL_GPUStorageBufferReadWriteBinding outputBinding{ outputBuffer, false, 0, 0, 0 };
	auto *computePass = SDL_BeginGPUComputePass(commandBuffer, nullptr, 0, &outputBinding, 1);
	if (!computePass)
		return fail("begin_compute_pass");
	SDL_BindGPUComputePipeline(computePass, pipeline);
	SDL_GPUBuffer *readonlyBuffers[] = { inputBuffer };
	SDL_BindGPUComputeStorageBuffers(computePass, 0, readonlyBuffers, 1);
	SDL_DispatchGPUCompute(computePass, 1, 1, 1);
	SDL_EndGPUComputePass(computePass);

	auto *downloadPass = SDL_BeginGPUCopyPass(commandBuffer);
	if (!downloadPass)
		return fail("begin_download_pass");
	const SDL_GPUBufferRegion outputRegion{ outputBuffer, 0, static_cast<Uint32>(kBufferBytes) };
	const SDL_GPUTransferBufferLocation downloadLocation{ downloadBuffer, 0 };
	SDL_DownloadFromGPUBuffer(downloadPass, &outputRegion, &downloadLocation);
	SDL_EndGPUCopyPass(downloadPass);

	fence = SDL_SubmitGPUCommandBufferAndAcquireFence(commandBuffer);
	commandActive = false;
	commandBuffer = nullptr;
	if (!fence)
		return fail("submit_command_buffer");
	SDL_GPUFence *fences[] = { fence };
	if (!SDL_WaitForGPUFences(device, true, fences, 1))
		return fail("wait_for_fence");

	void *downloadMemory = SDL_MapGPUTransferBuffer(device, downloadBuffer, false);
	if (!downloadMemory)
		return fail("map_download_buffer");
	downloadMapped = true;
	std::array<std::uint32_t, kWordCount> output{};
	std::memcpy(output.data(), downloadMemory, kBufferBytes);
	SDL_UnmapGPUTransferBuffer(device, downloadBuffer);
	downloadMapped = false;

	resultHash = HashWords(output);
	if (output != expected)
	{
		error = "cpu_gpu_value_mismatch";
		cleanup();
		return false;
	}
	cleanup();
	return true;
}
#endif
}

namespace Platform
{
void InitializeOmniCompute()
{
#if TPT_SDL3 && TPT_SDLGPU_SHADER_AVAILABLE
	std::string error;
	if (!InitializeProductionCompute(error) || !OmniCompute::RunGPUValidation(&error))
	{
		if (error.empty())
			error = "thermal diffusion numerical validation failed";
		DestroyProductionCompute();
		std::cerr << "SDL_GPU initialization failed:\n" << error << "\n\nFallback: CPU\n";
	}
	else
	{
		std::cout << "Compute backend: " << OmniCompute::BackendName(OmniCompute::GetComputeBackend()) << '\n';
	}
#else
	OmniCompute::ResetBackend("SDL_GPU production shader unavailable; CPU fallback");
	std::cout << "Compute backend: CPU\n";
#endif
}

void ShutdownOmniCompute()
{
#if TPT_SDL3 && TPT_SDLGPU_SHADER_AVAILABLE
	DestroyProductionCompute();
#else
	OmniCompute::ResetBackend("CPU shutdown");
#endif
}

int RunSDLGPUProbe()
{
	PrintValue("sdlgpu_probe_version", "1.1.0");

#if !TPT_SDL3
	PrintValue("sdl_backend", "SDL2");
	PrintBool("gpu_supported", false);
	PrintBool("compute_poc_built", false);
	PrintBool("compute_poc_executed", false);
	PrintBool("deterministic_compare", false);
	PrintBool("fallback_cpu", true);
	PrintValue("fallback_reason", "SDL3_backend_required");
	return 0;
#else
	PrintValue("sdl_backend", "SDL3");
	PrintBool("production_thermal_diffusion_built", TPT_SDLGPU_SHADER_AVAILABLE != 0);
	PrintBool("cuda_backend_available", false);
	PrintValue("cuda_backend_status", "not_implemented_optional_future_backend");
	// SDL_GPU chooses its backend before device creation.  Prefer Vulkan for
	// this probe because the checked-in artifact is SPIR-V; normal rendering
	// remains untouched and does not inherit this hint.
	SDL_SetHint(SDL_HINT_GPU_DRIVER, "vulkan");
	if (!SDL_Init(SDL_INIT_VIDEO))
	{
		PrintBool("gpu_supported", false);
		PrintBool("compute_poc_built", false);
		PrintBool("compute_poc_executed", false);
		PrintBool("deterministic_compare", false);
		PrintBool("fallback_cpu", true);
		PrintValue("fallback_reason", std::string("SDL_Init:") + SDL_GetError());
		return 0;
	}

	const int driverCount = SDL_GetNumGPUDrivers();
	PrintValue("gpu_driver_count", std::to_string(driverCount));
	for (int i = 0; i < driverCount; ++i)
	{
		const char *driver = SDL_GetGPUDriver(i);
		PrintValue((std::string("gpu_driver_") + std::to_string(i)).c_str(), driver ? driver : "");
	}
	bool hasD3D12 = false;
	bool hasVulkan = false;
	for (int i = 0; i < driverCount; ++i)
	{
		const char *driver = SDL_GetGPUDriver(i);
		hasD3D12 = hasD3D12 || (driver && std::strcmp(driver, "direct3d12") == 0);
		hasVulkan = hasVulkan || (driver && std::strcmp(driver, "vulkan") == 0);
	}
	PrintBool("sdlgpu_d3d12_driver_available", hasD3D12);
	PrintValue("sdlgpu_d3d12_compute_status", "unsupported_no_dxil_compiler_or_shader");
	PrintBool("sdlgpu_vulkan_driver_available", hasVulkan);

	const bool supportsSpirv = SDL_GPUSupportsShaderFormats(SDL_GPU_SHADERFORMAT_SPIRV, "vulkan");
	PrintBool("spirv_supported", supportsSpirv);
	SDL_GPUDevice *device = nullptr;
	// Keep the support query visible, but attempt creation independently: some
	// SDL backends defer loader/device checks until SDL_CreateGPUDevice().
	device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, false, "vulkan");
	if (!device)
	{
		PrintBool("gpu_supported", false);
		PrintBool("compute_poc_built", TPT_SDLGPU_SHADER_AVAILABLE != 0);
		PrintBool("compute_poc_executed", false);
		PrintBool("deterministic_compare", false);
		PrintBool("fallback_cpu", true);
		PrintValue("fallback_reason", std::string("create_device:") + SDL_GetError());
		SDL_Quit();
		return 0;
	}

	PrintBool("gpu_supported", true);
	const char *backend = SDL_GetGPUDeviceDriver(device);
	PrintValue("gpu_backend", backend ? backend : "unknown");
	std::ostringstream formats;
	formats << "0x" << std::hex << SDL_GetGPUShaderFormats(device);
	PrintValue("gpu_shader_formats", formats.str());
	const auto properties = SDL_GetGPUDeviceProperties(device);
	if (properties)
	{
		PrintValue("gpu_device_name", SDL_GetStringProperty(properties, SDL_PROP_GPU_DEVICE_NAME_STRING, "unknown"));
		PrintValue("gpu_driver_name", SDL_GetStringProperty(properties, SDL_PROP_GPU_DEVICE_DRIVER_NAME_STRING, "unknown"));
		PrintValue("gpu_driver_version", SDL_GetStringProperty(properties, SDL_PROP_GPU_DEVICE_DRIVER_VERSION_STRING, "unknown"));
	}

#if TPT_SDLGPU_SHADER_AVAILABLE
	std::string error;
	std::uint64_t resultHash = 0;
	const bool computeOk = ExecuteComputeProof(device, error, resultHash);
	PrintBool("compute_poc_built", true);
	PrintBool("compute_poc_executed", computeOk);
	PrintBool("deterministic_compare", computeOk);
	PrintBool("fallback_cpu", !computeOk);
	if (computeOk)
	{
		std::ostringstream hash;
		hash << "0x" << std::hex << resultHash;
		PrintValue("gpu_result_hash", hash.str());
	}
	else
		PrintValue("fallback_reason", error);
#else
	PrintBool("compute_poc_built", false);
	PrintBool("compute_poc_executed", false);
	PrintBool("deterministic_compare", false);
	PrintBool("fallback_cpu", true);
	PrintValue("fallback_reason", "shader_compiler_unavailable_at_build");
#endif
	PrintValue("device_loss_path", "not_tested");
	SDL_WaitForGPUIdle(device);
	SDL_DestroyGPUDevice(device);
	SDL_Quit();
	return 0;
#endif
}

int RunSDLGPUValidation(const char *jsonPath, bool forceInitializationFailure)
{
	PrintValue("sdlgpu_validation_version", "1.1.0-rc1");
#if !TPT_SDL3 || !TPT_SDLGPU_SHADER_AVAILABLE
	PrintBool("gpu_validation_supported", false);
	PrintBool("gpu_validation_passed", false);
	PrintValue("gpu_validation_status", "unsupported_build_without_sdlgpu_spirv");
	PrintValue("fallback", "CPU");
	WriteValidationJson(jsonPath, false, "CPU", "unsupported_build_without_sdlgpu_spirv", false, true);
	return kShaderUnavailable;
#else
	if (!SDL_Init(SDL_INIT_VIDEO))
	{
		PrintBool("gpu_validation_supported", false);
		PrintBool("gpu_validation_passed", false);
		PrintValue("gpu_validation_status", std::string("SDL_Init:") + SDL_GetError());
		PrintValue("fallback", "CPU");
		WriteValidationJson(jsonPath, false, "CPU", SDL_GetError(), false, true);
		return kSDLInitializationFailure;
	}
	std::string error;
	const bool initialized = InitializeProductionCompute(error, forceInitializationFailure);
	const bool validated = initialized && OmniCompute::RunGPUValidation(&error);
	if (!validated && error.empty())
		error = "thermal_diffusion_cpu_gpu_epsilon_validation_failed";
	const auto status = OmniCompute::GetStatus();
	PrintBool("gpu_validation_supported", initialized);
	PrintValue("compute_backend", OmniCompute::BackendName(status.backend));
	PrintBool("gpu_validation_passed", validated);
	PrintValue("gpu_validation_status", validated ? "PASS" : error);
	PrintValue("fallback", validated ? "not_used" : "CPU");
	WriteValidationJson(jsonPath, validated, OmniCompute::BackendName(status.backend),
		validated ? "PASS" : error.c_str(), initialized, !validated,
		status.maxAbsoluteError, status.maxRelativeError, status.firstMismatchIndex);
	DestroyProductionCompute();
	SDL_Quit();
	if (validated)
		return kGPUValidationPass;
	if (!initialized)
		return kGPUDeviceCreationFailure;
	return error.find("readback") != std::string::npos ? kReadbackFailure : kNumericalMismatch;
#endif
}

int RunCPUFallbackValidation(const char *jsonPath)
{
	// This is intentionally a separate gate: forced GPU initialization failure
	// must still leave the reference CPU stencil usable and finite.
	std::string initializationError;
#if TPT_SDL3 && TPT_SDLGPU_SHADER_AVAILABLE
	const bool gpuInitialized = InitializeProductionCompute(initializationError, true);
#else
	const bool gpuInitialized = false;
	initializationError = "forced_initialization_failure";
	OmniCompute::ResetBackend("forced GPU initialization failure; CPU fallback");
#endif
	const std::vector<float> temperature{ 290.0f, 292.0f, 296.0f, 301.0f };
	const std::vector<float> conductivity(temperature.size(), 0.02f);
	const std::vector<unsigned char> blocked(temperature.size(), 0);
	const OmniThermalDiffusionInput input{
		.width = 2, .height = 2, .periodic = false,
		.timestepOverCellLengthSquared = 0.25f,
		.temperature = temperature, .conductivity = conductivity, .blocked = blocked,
	};
	const auto reference = OmniCompute::ComputeThermalDiffusionReference(input);
	const bool ran = !gpuInitialized && initializationError == "forced_initialization_failure" &&
		OmniCompute::GetComputeBackend() == OmniComputeBackend::CPU && !reference.empty();
	for (float value : reference)
		if (!std::isfinite(value))
			return (WriteFallbackJson(jsonPath, false, "cpu_reference_nonfinite"), kGPUInternalFailure);
	WriteFallbackJson(jsonPath, ran, ran ? "GPU_init_failed_CPU_reference_continues" : initializationError.c_str());
	PrintBool("cpu_fallback_tested", true);
	PrintBool("cpu_fallback_passed", ran);
	PrintValue("fallback", "CPU");
	return ran ? 0 : kGPUInternalFailure;
}

int RunSDL3GUISmokeTest(const char *jsonPath)
{
#if !TPT_SDL3
		WriteSimpleJson(jsonPath, "sdl3_gui", false, "SDL3_not_built");
	return 10;
#else
	const auto artifactPath = jsonPath && *jsonPath
		? std::filesystem::path(jsonPath).parent_path() / "gui-smoke.bmp"
		: std::filesystem::path("gui-smoke.bmp");
	const auto artifact = artifactPath.string();
	if (!SDL_Init(SDL_INIT_VIDEO))
	{
		WriteSimpleJson(jsonPath, "sdl3_gui", false, SDL_GetError());
		return 11;
	}
	SDL_Window *window = SDL_CreateWindow("TPT-ZH OmniPack GUI Smoke", 320, 240, SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE);
	if (!window)
	{
		const auto error = std::string(SDL_GetError());
		SDL_Quit();
		WriteSimpleJson(jsonPath, "sdl3_gui", false, error.c_str(), nullptr, true);
		return 12;
	}
	SDL_Renderer *renderer = SDL_CreateRenderer(window, nullptr);
	if (!renderer)
	{
		const auto error = std::string(SDL_GetError());
		SDL_DestroyWindow(window);
		SDL_Quit();
		WriteSimpleJson(jsonPath, "sdl3_gui", false, error.c_str(), nullptr, true, false);
		return 13;
	}
	bool ok = SDL_SetRenderDrawColor(renderer, 0x11, 0x72, 0xA9, 0xFF) &&
		SDL_RenderClear(renderer) && SDL_RenderPresent(renderer);
	const bool resize = SDL_SetWindowSize(window, 480, 360);
	int resizedWidth = 0, resizedHeight = 0;
	const bool resizeObserved = SDL_GetWindowSize(window, &resizedWidth, &resizedHeight) &&
		resizedWidth == 480 && resizedHeight == 360;
	const bool fullscreen = SDL_SetWindowFullscreen(window, true) && SDL_SetWindowFullscreen(window, false);
	const bool textInput = SDL_StartTextInput(window) && SDL_StopTextInput(window);
	const bool clipboard = SDL_SetClipboardText("omnipack-gui-smoke") &&
		([]() { char *text = SDL_GetClipboardText(); const bool same = text && std::strcmp(text, "omnipack-gui-smoke") == 0; SDL_free(text); return same; })();
	SDL_Event keyEvent{};
	keyEvent.type = SDL_EVENT_KEY_DOWN;
	keyEvent.key.key = SDLK_A;
	keyEvent.key.down = true;
	const bool keyboard = SDL_PushEvent(&keyEvent);
	SDL_Event mouseEvent{};
	mouseEvent.type = SDL_EVENT_MOUSE_MOTION;
	mouseEvent.motion.x = 12.0f;
	mouseEvent.motion.y = 13.0f;
	const bool mouse = SDL_PushEvent(&mouseEvent);
	SDL_PumpEvents();
	const bool frameRendered = ok;
	SDL_Surface *surface = ok ? SDL_RenderReadPixels(renderer, nullptr) : nullptr;
	bool screenshotCreated = false;
	if (surface)
	{
		screenshotCreated = SDL_SaveBMP(surface, artifact.c_str());
		SDL_DestroySurface(surface);
	}
	else
		screenshotCreated = false;
	ok = frameRendered && resize && resizeObserved && fullscreen && textInput && clipboard && keyboard && mouse && screenshotCreated;
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
	bool restart = false;
	if (ok && SDL_Init(SDL_INIT_VIDEO))
	{
		SDL_Window *second = SDL_CreateWindow("TPT-ZH OmniPack GUI Restart", 160, 120, SDL_WINDOW_HIDDEN);
		restart = second != nullptr;
		if (second)
			SDL_DestroyWindow(second);
		SDL_Quit();
	}
	ok = ok && restart;
	WriteSimpleJson(jsonPath, "sdl3_gui", ok, "sdl_init_window_render_resize_fullscreen_input_clipboard_shutdown", artifact.c_str(), true, frameRendered, resize && resizeObserved, fullscreen, keyboard, mouse, textInput, clipboard, screenshotCreated, true, restart);
	return ok ? 0 : 14;
#endif
}
}
