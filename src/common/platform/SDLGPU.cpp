#include "Platform.h"
#include "SDLCompat.h"

#include <array>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>

#ifndef TPT_SDLGPU_SHADER_AVAILABLE
# define TPT_SDLGPU_SHADER_AVAILABLE 0
#endif

#if TPT_SDL3
# include <SDL3/SDL_gpu.h>
# if TPT_SDLGPU_SHADER_AVAILABLE
#  include "sdlgpu_compute_spirv.h"
# endif
#endif

namespace
{
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
}
