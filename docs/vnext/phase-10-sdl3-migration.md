# 1.0.10 SDL3 stable migration

```text
TARGET_VERSION=1.0.10
BASE_COMMIT=6977518762ec725038a9c3805de09ece15a57918
BRANCH=integration/omnicore-vnext
ROLLBACK_COMMIT=6977518762ec725038a9c3805de09ece15a57918
GATE=GREEN_WINDOWS_VALIDATED_CROSS_PLATFORM_CI_PENDING
SIMULATION_EQUATIONS_CHANGED=false
SAVE_SCHEMA_CHANGED=false
SDL_GPU=false
CUDA=false
```

## Scope and architecture

1.0.10 is an isolated platform migration. It adds a Meson `sdl_backend`
selection with `auto`, `sdl2` and `sdl3`; supported desktop targets select SDL3
by default while Android, Emscripten, MSVC and non-x86_64 Windows retain the
SDL2 fallback. `SDLCompat.h` centralises the include boundary and preserves the
numeric ABI of SDL2 key/scancode constants exported to Lua where SDL3 removed
the names.

The desktop migration covers window/display discovery, renderer creation,
logical presentation, texture scaling, fullscreen, refresh rate, mouse and
keyboard events, text input, file drops, screenshots, icons and clipboard text
ownership. The application continues to own `Platform::InvokeMain`; SDL3 is
built with `SDL_MAIN_HANDLED` rather than transferring lifecycle ownership.

Native save clipboard routing no longer depends on SDL2 `SDL_syswm`. SDL3 uses
the active video driver for platform selection and the Win32 HWND property on
Windows. SDL3 Windows serialises save data immediately because the removed
`SDL_SYSWMEVENT` delayed-render path is unavailable; SDL2 keeps the historical
event-watch implementation.

## Dependency and CI policy

Official SDL3 reference is `release-3.4.14` at
`147a8ee32dbf9ac02f3794964490687b6bbda1bc5`, Zlib licensed. The existing TPT
prebuilt dependency bundle still contains SDL2 only. Windows x86_64 therefore
uses the MSYS2 SDL3 `3.4.14-1` package while other libraries remain on the pinned
TPT bundle. macOS uses Homebrew SDL3. Ubuntu 22.04 does not provide `libsdl3-dev`,
so Linux CI builds and installs the exact official SDL3 tag and verifies its
commit before the project build. Older platform jobs are forced to SDL2.

The generated CI matrix and shell/Python syntax were validated locally. Linux
and macOS CI execution are `not_tested_on_this_windows_host`; the pinned source
bootstrap is not represented as a completed remote CI run.

## Frozen simulation contract

The following SHA-256 values match 1.0.9 exactly after migration:

```text
Simulation.cpp=0FC5B57B6E0DD3798749D610A39848B8D35C3998A3A6E68CD07FA2E26158F102
Simulation.h=C2F808058EE93F4E04A8151CA276E607D40384592AC0EB2D21823D625525792A
Air.cpp=E831BC481268415052B9258832AA73013A9E010EEB963A75B93FBF4F28248E7E
Air.h=4A9162225067573956FAE5557F9B3FC27AC50F1A5B7E7D0183D9499D50364AF2
Particle.h=F04F77B2711E99C3CDF82F56682A5DC7BD248157040D4CC142DBFE6449C0297F
GameSave.cpp=AC7B6C348B38972E5727A0841C484A60AC000AC261FF4943F9D713328188D25E
GameSave.h=3F1C099331EBA03A7E41013C21432746B4AECD16F13E40660374458CAF0465BB
```

## Build and test evidence

```text
windows_sdl3_explicit_build=PASS
windows_auto_build=PASS_auto_selected_sdl3
windows_sdl2_fallback_build=PASS_selected_sdl2
windows_sdl3_package=3.4.14-1
sdl3_validated_executable_sha256=9AE89ED8A782E16A59067DD8DB5DEDEA0A45C8B35D9D53E05ADA30FFBBC093EF
meson_suite=96/96_PASS
sdl3_migration_contract=PASS
python_ci_scripts=PASS_compile
ci_matrix_generation=PASS
git_diff_check=PASS
sdl3_binary_imports_sdl2=false
windows_mingw_static_sdl3_iconv_link=PASS
```

The SDL3 executable launched from an isolated data directory with title
`TPT-ZH-OmniPack 1.0.10-dev`, a nonzero window handle and `Responding=true`.
With `NativeClipboard.Enabled=true`, stderr contained
`save clipboard format registered`. No user preference directory, account
state, clipboard payload or private save was imported.

Runtime regression on the SDL3 executable:

```text
upstream_100_1_lua=PASS_lua_bounds_11
omni_solution=PASS_total_residual_8.470329472543e-22kg
omni_corrosion=PASS
omni_chemistry=PASS
profiler=PASS
rendering_profiler_concurrency=PASS
correction_ledger=PASS
lifecycle_ledger=PASS
ops_roundtrip=PASS_8_scenarios_24_processes_16_restarts_16_load_verifications
```

Direct visual interaction, input gestures, audio output, clipboard payload
roundtrip, multi-monitor switching and fullscreen mode changes are
`not_tested_computer_use_unavailable`. Process/window liveness is not claimed as
visual QA.

## Benchmark and memory

Fixed-step `mixed-medium`, 30 warm-up steps, 120 measured steps per pass and
five passes:

```text
profiler_off_steps_per_second=121.207975
profiler_on_steps_per_second=132.741312
final_state_hash_off=798083730
final_state_hash_on=798083730
peak_working_set_off_bytes=186925056
peak_working_set_on_bytes=187654144
performance_gate=not_evaluated
```

The profiler-on run being faster is treated as measurement noise/order effect,
not an optimisation claim. 1.0.10 adds no persistent simulation-side memory and
no GPU/VRAM allocation.

## Gate and rollback

```text
BUILD_WINDOWS=GREEN
SDL2_FALLBACK=GREEN
MESON_TESTS=GREEN
SAVE_LUA_RUNTIME=GREEN
SIMULATION_FREEZE=GREEN
WINDOW_LIVENESS=GREEN
NATIVE_CLIPBOARD_REGISTRATION=GREEN
CROSS_PLATFORM_CI_SCRIPT=GREEN_STATIC_PINNED
CROSS_PLATFORM_RUNTIME=NOT_TESTED_PENDING_CI
VISUAL_INTERACTION=NOT_TESTED_COMPUTER_USE_UNAVAILABLE
BENCHMARK=RECORDED_PERFORMANCE_THRESHOLD_NOT_EVALUATED
GATE=GREEN_WINDOWS_VALIDATED_CROSS_PLATFORM_CI_PENDING
```

Rollback is the parent commit `697751876`. The next version may begin only after
the focused 1.0.10 commit/tag exists. SDL_GPU and CUDA remain 1.0.11 work.
