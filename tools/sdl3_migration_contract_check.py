#!/usr/bin/env python3
from pathlib import Path
import hashlib
import sys


def require(text: str, needle: str, label: str) -> None:
    if needle not in text:
        raise AssertionError(f"missing {label}: {needle}")


def main() -> int:
    root = Path(__file__).resolve().parents[1]
    meson = (root / "meson.build").read_text(encoding="utf-8")
    options = (root / "meson_options.txt").read_text(encoding="utf-8")
    compat = (root / "src/common/platform/SDLCompat.h").read_text(encoding="utf-8")
    platform = (root / "src/PowderToySDL.cpp").read_text(encoding="utf-8")
    clipboard = (root / "src/common/clipboard/Dynamic.cpp").read_text(encoding="utf-8")
    windows_clipboard = (root / "src/common/clipboard/Windows.cpp").read_text(encoding="utf-8")
    gpu = (root / "src/common/platform/SDLGPU.cpp").read_text(encoding="utf-8")
    gpu_shader = (root / "resources/omnicore/sdl_gpu_add.comp").read_text(encoding="utf-8")
    ci = (root / ".github/build.sh").read_text(encoding="utf-8")

    require(options, "choices: [ 'auto', 'sdl2', 'sdl3' ]", "dual backend option")
    require(meson, "sdl_backend = 'sdl3'", "desktop SDL3 auto selection")
    require(meson, "sdl_backend = 'sdl2'", "legacy fallback selection")
    require(meson, "dependency(use_sdl3 ? 'sdl3' : 'sdl2'", "backend dependency")
    # Static MinGW SDL3 archives require the package's private iconv dependency.
    require(meson, "cpp_compiler.find_library('iconv', required: true)", "static MinGW SDL3 iconv dependency")
    require(compat, "# include <SDL3/SDL.h>", "SDL3 header")
    require(platform, "SDL_EVENT_WINDOW_DISPLAY_CHANGED", "SDL3 window event")
    require(platform, "SDL_SetRenderLogicalPresentation", "SDL3 logical rendering")
    require(platform, "SDL_ConvertEventToRenderCoordinates", "SDL3 logical input coordinates")
    require(platform, "SDL_SetWindowFullscreenMode", "SDL3 fullscreen mode")
    require(platform, "SDL_free(text);", "SDL3 clipboard text ownership")
    require(clipboard, "SDL_GetCurrentVideoDriver", "SDL3 clipboard subsystem routing")
    require(windows_clipboard, "SDL_PROP_WINDOW_WIN32_HWND_POINTER", "SDL3 HWND property")
    require(meson, "find_program('glslc', required: false)", "optional SDL_GPU shader compiler")
    require(meson, "sdlgpu_compute_spirv", "embedded SDL_GPU shader target")
    require(gpu, "SDL_CreateGPUDevice", "SDL_GPU device creation")
    require(gpu, "SDL_BeginGPUComputePass", "SDL_GPU compute pass")
    require(gpu, "SDL_SubmitGPUCommandBufferAndAcquireFence", "SDL_GPU fence submission")
    require(gpu, "deterministic_compare", "CPU/GPU deterministic comparison")
    require(gpu, "fallback_cpu", "CPU fallback contract")
    require(gpu_shader, "layout(local_size_x = 64", "deterministic compute local size")
    require(gpu_shader, "layout(std140, set = 0, binding = 0)", "read-only storage layout")
    require(gpu_shader, "layout(std140, set = 1, binding = 0)", "read-write storage layout")
    require(ci, "SDL3_RELEASE=release-3.4.14", "pinned Linux SDL3 release")
    require(
        ci,
        "SDL3_COMMIT=147a8ee32dbf9ac02f3794964490687b6bbda1bc5",
        "pinned Linux SDL3 commit",
    )
    require(ci, 'git -C "$sdl3_source" rev-parse HEAD', "Linux SDL3 commit verification")
    require(ci, 'sudo cmake --install "$sdl3_build"', "Linux SDL3 source install")
    if "libsdl3-dev" in ci:
        raise AssertionError("Ubuntu 22.04 must not depend on unavailable libsdl3-dev")
    require(ci, "brew install binutils sdl3", "macOS SDL3 package")
    require(ci, 'mingw-w64-"$variant"-sdl3', "Windows SDL3 package")

    frozen = [
        ("src/simulation/Simulation.cpp", "0FC5B57B6E0DD3798749D610A39848B8D35C3998A3A6E68CD07FA2E26158F102"),
        ("src/simulation/Simulation.h", "C2F808058EE93F4E04A8151CA276E607D40384592AC0EB2D21823D625525792A"),
        ("src/simulation/Air.cpp", "E831BC481268415052B9258832AA73013A9E010EEB963A75B93FBF4F28248E7E"),
        ("src/simulation/Air.h", "4A9162225067573956FAE5557F9B3FC27AC50F1A5B7E7D0183D9499D50364AF2"),
        ("src/simulation/Particle.h", "F04F77B2711E99C3CDF82F56682A5DC7BD248157040D4CC142DBFE6449C0297F"),
        ("src/client/GameSave.cpp", "AC7B6C348B38972E5727A0841C484A60AC000AC261FF4943F9D713328188D25E"),
        ("src/client/GameSave.h", "3F1C099331EBA03A7E41013C21432746B4AECD16F13E40660374458CAF0465BB"),
    ]
    for relative, expected_sha256 in frozen:
        path = root / relative
        if not path.is_file():
            raise AssertionError(f"missing frozen simulation source: {relative}")
        actual_sha256 = hashlib.sha256(path.read_bytes()).hexdigest().upper()
        if actual_sha256 != expected_sha256:
            raise AssertionError(
                f"frozen simulation source changed: {relative} "
                f"expected={expected_sha256} actual={actual_sha256}"
            )

    print("sdl3_migration_contract_pass=true")
    print("desktop_default=sdl3")
    print("legacy_fallback=sdl2")
    print("sdlgpu_probe_contract=pass")
    print("frozen_simulation_files=7")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as error:
        print(f"sdl3_migration_contract_pass=false error={error}", file=sys.stderr)
        raise SystemExit(1)
