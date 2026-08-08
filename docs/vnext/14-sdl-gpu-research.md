# SDL3 and SDL_GPU research

## Current and upstream toolchain

```text
LOCAL_SDL=SDL2 2.30.9-tpt-libs
LOCAL_SDL3=false
LOCAL_SDL_GPU_PRODUCTION_CODE=false
SDL3_STABLE=3.4.14
SDL3_TAG_COMMIT=147a8ee32dbf9ac02f3794964490687b6bbda1bc
SDL_SHADERCROSS_HEAD=e55cf5e31ced6f3d1be5cc6d0c50e99384f9f4ba
DXC_AVAILABLE=false
GLSLC_AVAILABLE=false
SPIRV_VAL_AVAILABLE=false
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
SDL3_GATE=RED
GPU_POC_GATE=RED
GPU_ATMOSPHERE_GATE=RED
```

The red result reflects missing prerequisite phases and toolchain, not missing
hardware.
