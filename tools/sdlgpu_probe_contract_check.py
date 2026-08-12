#!/usr/bin/env python3
"""Static contract checks for the 1.1.0 SDL_GPU proof-of-concept."""

from pathlib import Path
import re
import sys


def require(text: str, needle: str, label: str) -> None:
    if needle not in text:
        raise AssertionError(f"missing {label}: {needle}")


def main() -> int:
    root = Path(__file__).resolve().parents[1]
    gpu = (root / "src/common/platform/SDLGPU.cpp").read_text(encoding="utf-8")
    shader = (root / "resources/omnicore/sdl_gpu_add.comp").read_text(encoding="utf-8")
    meson = (root / "meson.build").read_text(encoding="utf-8")
    platform = (root / "src/PowderToy.cpp").read_text(encoding="utf-8")

    for needle, label in [
        ("SDL_SetHint(SDL_HINT_GPU_DRIVER, \"vulkan\")", "isolated Vulkan probe hint"),
        ("SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, false, \"vulkan\")", "SPIR-V device creation"),
        ("SDL_CreateGPUBuffer", "storage buffer creation"),
        ("SDL_UploadToGPUBuffer", "buffer upload"),
        ("SDL_DownloadFromGPUBuffer", "buffer readback"),
        ("SDL_WaitForGPUFences", "fence wait"),
        ("SDL_ReleaseGPUFence", "fence release"),
        ("SDL_DestroyGPUDevice", "device destruction"),
        ("fallback_reason", "fallback reason output"),
        ("deterministic_compare", "deterministic CPU comparison"),
    ]:
        require(gpu, needle, label)

    require(platform, '"--gpu-probe"', "opt-in command-line entry")
    require(meson, "sdlgpu_shader_available", "optional shader build gate")
    require(meson, "--target-env=vulkan1.0", "deterministic Vulkan shader target")
    require(meson, "sdlgpu-spirv-validation", "SPIR-V validation test")
    require(shader, "layout(std140, set = 0, binding = 0)", "input std140 layout")
    require(shader, "layout(std140, set = 1, binding = 0)", "output std140 layout")

    local_size = re.search(r"local_size_x\s*=\s*(\d+)", shader)
    if not local_size or local_size.group(1) != "64":
        raise AssertionError("compute local size must remain 64")

    print("sdlgpu_probe_contract_pass=true")
    print("shader_layout=std140_input_output")
    print("fallback_contract=cpu")
    print("device_loss_path=declared_not_tested")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as error:
        print(f"sdlgpu_probe_contract_pass=false error={error}", file=sys.stderr)
        raise SystemExit(1)
