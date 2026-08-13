# 1.1.0-rc1 Windows x64 SDL3 Release Candidate

This is an unsigned local release candidate, not a stable release (不是稳定版).

## Implemented and verified automatically

- SDL 3.4.14 desktop runtime; normal renderer remains SDL-based.
- Optional SDL_GPU Vulkan compute for OmniAtmosphere thermal-conduction stencil.
- Every GPU stencil dispatch is checked against the CPU reference with an
  absolute-plus-relative floating point tolerance. Device, pipeline, fence,
  readback, non-finite, or comparison failure immediately selects CPU.
- D3D12 driver presence is reported, but a D3D12 compute backend is not shipped
  because this build does not produce DXIL shaders.
- CUDA is not implemented and is not a runtime dependency; `nvcc` was absent
  during the RC audit. The roadmap calls it an optional future backend only.
- The Windows release workflow extracts DWARF symbols into a separate symbols
  archive, strips DWARF from the user executable, adds a GNU debug link, audits
  PE imports/security flags/developer-path markers, and generates manifest and
  SHA-256 sidecars.

## Stable-release blockers

- `OfficialTPTSaveCompatibility=NOT TESTED`: no curated official save fixture
  corpus has been supplied to run load -> simulate -> save -> reload.
- `WindowsCleanMachine=NOT TESTED`: the current host is a development machine;
  an isolated extraction must be verified on a Windows x64 machine without
  MSYS2/MinGW/source-tree dependencies.
- `SDL3GUI=NOT TESTED`: process liveness does not replace visible validation of
  window, resize, fullscreen, input, clipboard, screenshot, shutdown/restart.
- `Soak2Hours=NOT TESTED`: no 7,200-second complex-scene run with periodic
  finite/conservation/memory checks has completed.

`release_ready=false`
