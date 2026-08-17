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
    require(
        platform,
        "SDL_SetTextureBlendMode(sdl_texture, SDL_BLENDMODE_NONE)",
        "opaque SDL3 framebuffer presentation",
    )
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

    # Freeze Git-canonical source bytes rather than the checkout's newline
    # representation.  The previous hashes encoded a particular mixture of
    # CRLF and LF lines, so touching and restoring an otherwise byte-identical
    # source hunk could fail this contract on Windows while the Git blob was
    # unchanged.
    frozen = [
        ("src/simulation/Simulation.cpp", "AC7ABE90F0A200F1DE2180427413D919D775C02B861A6B406A246402C5459D16"),
        ("src/simulation/Simulation.h", "C7209AD8129C7C79E3CC3D91245E758570B3196E1C7E183FFD17734C2F8B40E1"),
        ("src/simulation/Air.cpp", "58DE79839CF8B0264B6BE5ECB8193D1EAD32224806C9191466F4BBD1EE627084"),
        ("src/simulation/Air.h", "31E86BDC91325A72487CAA486751BA6E66CE60CFDED3C0EAB4967AC9FF7E7953"),
        ("src/simulation/Particle.h", "BA10AE0F045790CEECB92F3DF362527017F60B490957CAFD630EED0F60F73A03"),
        ("src/client/GameSave.cpp", "7620DA07C77CC80AB5A99AED0F85DCC259C97CCD9232E1F0555C2892368572B3"),
        ("src/client/GameSave.h", "A90A34B743094CE5BE74E3725C868C08F682236FB1ED33BF119406F44B404873"),
    ]
    for relative, expected_sha256 in frozen:
        path = root / relative
        if not path.is_file():
            raise AssertionError(f"missing frozen simulation source: {relative}")
        canonical_bytes = path.read_bytes().replace(b"\r\n", b"\n")
        actual_sha256 = hashlib.sha256(canonical_bytes).hexdigest().upper()
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
