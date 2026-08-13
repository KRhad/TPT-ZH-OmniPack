# 1.1.0 SDL_GPU bounded compute proof-of-concept

```text
TARGET_VERSION=1.1.0
BASE_COMMIT=d73b51b0f17dfaa13e8336b0d227daada97703ec
BRANCH=integration/omnicore-vnext
SCOPE=SDL_GPU_PROBE_AND_DETERMINISTIC_COMPUTE_POC
SIMULATION_EQUATIONS_CHANGED=false
PARTICLE_ABI_CHANGED=false
SAVE_SCHEMA_CHANGED=false
LUA_ABI_CHANGED=false
CUDA=false
```

## What changed

The desktop executable now accepts `--gpu-probe` before normal singleton,
preference, window and simulation setup. The probe is intentionally isolated:

1. SDL3 requests the Vulkan SDL_GPU backend and records available drivers,
   selected backend, device and driver properties.
2. A checked-in GLSL compute shader is compiled with local `glslc` and embedded
   as SPIR-V when that tool is available. `spirv-val` validates the generated
   artifact during development; no generated binary is tracked.
3. Two std140 storage buffers carry sixteen `u32` words. The compute pass applies
   `output = input * 3 + 7`, then a copy pass downloads the result through a
   fence. The CPU computes the same reference and compares every word.
4. Every failure path releases command buffers/resources and reports a reason;
   SDL2 and shader/device-unavailable builds explicitly select CPU fallback.

The shader uses 64 local invocations and 16 active words so the proof remains
small, deterministic and independent of the simulation's particle layout.

## Direct Windows evidence

The SDL3 build used SDL 3.4.14 from MSYS2 and the UCRT64 shader tools
`glslc 2026.3` and SPIR-V Tools 2026.3, with the artifact validated against
the Vulkan 1.0 target. The probe reported:

```text
sdl_backend=SDL3
gpu_driver_count=2
gpu_driver_0=direct3d12
gpu_driver_1=vulkan
spirv_supported=true
gpu_supported=true
gpu_backend=vulkan
gpu_shader_formats=0x2
gpu_device_name=NVIDIA GeForce RTX 5070 Ti Laptop GPU
gpu_driver_name=NVIDIA
gpu_driver_version=591.86.0.0
compute_poc_built=true
compute_poc_executed=true
deterministic_compare=true
gpu_result_hash=0x6218ce997ec92e93
fallback_cpu=false
device_loss_path=not_tested
```

The explicit SDL2 build reported `gpu_supported=false`,
`compute_poc_executed=false`, `deterministic_compare=false`,
`fallback_cpu=true`, `fallback_reason=SDL3_backend_required`. This development
executable was run with the UCRT64 runtime DLL directory on `PATH`; its SHA-256
is `5E5F3D617A164055A01A5546FC78E664D28A55C9E3C78EC93EFB4987C5B43C61`
and it is not claimed as a standalone portable artifact.

The complete sanitized-environment Meson suite passed `98/98`; the log audit
found zero credential-like environment variable names. The final `auto`
release configuration selected SDL3 and produced a statically linked Windows
executable with SHA-256
`321BAE590DB5766EED9BE375B61064DC8EAAE5C5466C5E46D5893BB44798D53B`.
Its imports contain no SDL2, SDL3, GCC, libstdc++ or libwinpthread DLL. The
validated SPIR-V artifact SHA-256 is
`954FDABE5B207450C85167171A74679F88E6BE43FFDA2B771FE4E079CFA0A5CB`.

## Gate and limits

```text
BUILD_SDL3=GREEN
BUILD_SDL2=GREEN
BUILD_AUTO_STATIC=GREEN_SELECTED_SDL3
MESON_TESTS=98_OF_98_PASS
SDL3_GPU_PROBE=GREEN_WINDOWS_VULKAN_SPIRV
CPU_GPU_DETERMINISTIC_COMPARE=GREEN
RESOURCE_RELEASE_ON_SUCCESS=GREEN_CODE_REVIEWED_AND_EXERCISED
CPU_FALLBACK=GREEN
DEVICE_LOSS=NOT_TESTED
CUDA=NOT_TESTED_NVCC_UNAVAILABLE
LINUX_MACOS_RUNTIME=NOT_TESTED_WINDOWS_HOST
GPU_PERFORMANCE_CROSSOVER=NOT_TESTED
PRODUCTION_SIMULATION_GPU_MIGRATION=FALSE
GATE=GREEN_WINDOWS_SDLGPU_POC_CPU_FALLBACK_CROSS_PLATFORM_PENDING
```

The result does not claim a faster simulator, GPU-resident particles, CUDA
support, or migration of chemistry, Save, Lua or the CPU atmosphere reference.
