# SDL3 and SDL_GPU research

## Current and upstream toolchain

```text
LOCAL_SDL=SDL2 2.30.9-tpt-libs
LOCAL_SDL3=true
LOCAL_SDL_GPU_PRODUCTION_CODE=false
LOCAL_SDL_GPU_POC_CODE=true
SDL3_STABLE=3.4.14
SDL3_TAG_COMMIT=147a8ee32dbf9ac02f3794964490687b6bbda1bc
SDL_SHADERCROSS_HEAD=e55cf5e31ced6f3d1be5cc6d0c50e99384f9f4ba
DXC_AVAILABLE=false
GLSLC_AVAILABLE=true
SPIRV_VAL_AVAILABLE=true
SHADERCROSS_AVAILABLE=false
SDL3_CONFIG_AVAILABLE=false
```

SDL3 `3.4.14` was the latest non-prerelease GitHub release on 2026-08-09. SDL and
SDL_shadercross use the zlib license. Current hardware includes an RTX 5070 Ti Laptop
GPU, but enumeration is not a compute-pipeline test.

## Migration boundary

SDL2-to-SDL3 is an independent phase after particle/Lua/Save characterization and
before SDL_GPU work. Simulation results must remain unchanged during migration.
Window, input, mouse, keyboard, audio, clipboard, renderer, save/load, Lua, UI,
sandbox and benchmark behavior all require dedicated acceptance.

The Lua SDL header currently registers 505 SDL2 constants. SDL3 renamed or removed
APIs and constants must be handled through an explicit compatibility table; changing
Lua numbers silently is not acceptable.

## SDL_GPU PoC scope

Only after the CPU OmniAtmosphere reference is correct, the first PoC verifies:

- GPU device selection and failure fallback;
- storage buffers and layout validation;
- offline shader artifacts and development-time compile path;
- compute pipeline/pass/dispatch;
- upload, readback, fence and error handling;
- deterministic small kernels and CPU comparison;
- resource destruction/device-loss behavior.

No particle allocator, chemistry, Save or Lua core moves to GPU in this PoC.

## Proposed atmosphere passes

Candidate passes are Primitive/EOS, Flux X, Flux Y, Conservative Update, Diffusion,
Source Terms, Boundary, Chemistry Coupling and Commit. The profiler may merge/split
them later. Ping-pong state and separate proposal/resolution passes prevent reliance
on invocation order.

Particle movement, if eventually attempted, requires proposal, target arbitration
and apply phases plus bounded Spawn/Delete request queues. Multiple invocations must
not directly mutate a shared allocator or assume a fixed write order.

## Residency and crossover

The GPU design must record upload, readback, fence/synchronization stall and resident
memory. A Hybrid CPU-special/GPU-generic backend is rejected if it requires full
state readback each frame. Small, medium, large and very-large scenes determine the
CPU/GPU crossover; GPU is not assumed faster.

## Gate

```text
SDL3_GATE=GREEN_WINDOWS_VALIDATED_CROSS_PLATFORM_CI_PENDING
GPU_POC_GATE=GREEN_WINDOWS_VULKAN_SPIRV_DETERMINISTIC_CPU_COMPARE
GPU_ATMOSPHERE_GATE=RED
```

## 1.1.0 bounded implementation

The 1.1.0 milestone adds an opt-in `--gpu-probe` command to the SDL3 desktop
executable. It requests the Vulkan SDL_GPU backend, reports the selected device,
driver and shader formats, and executes a 16-word `u32` storage-buffer kernel
when the locally available SPIR-V artifact is present. Upload, dispatch, fence,
readback and exact CPU comparison are part of the probe. The observed Windows
result hash is `0x6218ce997ec92e93` on an NVIDIA GeForce RTX 5070 Ti Laptop GPU
with driver 591.86.

SDL2 builds report `gpu_supported=false`, `compute_poc_executed=false` and
`fallback_cpu=true`. SDL3 builds without `glslc`, without a usable Vulkan
device, or with a resource/pipeline/fence failure also return a successful
diagnostic exit with an explicit `fallback_reason`; normal client startup is
unchanged. The shader is compiled at build time and embedded in the executable;
generated binaries and logs remain local build outputs.

This is a capability and synchronization proof, not a production physics port.
Particle allocation/movement, chemistry, Save, Lua, CUDA, device-loss recovery,
Linux/macOS runtime, performance crossover and full atmosphere migration remain
outside the 1.1.0 claim.

The remaining red atmosphere gate reflects missing production migration,
residency/crossover and device-loss evidence, not missing hardware.
