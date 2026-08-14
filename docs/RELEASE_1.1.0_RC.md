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

## Gate status and release boundary

Gate status is maintained in the machine-generated `RELEASE-VALIDATION.json`
and its derived text report. This document intentionally contains no independent
PASS/FAIL/NOT_TESTED table. The RC is not a stable release; the stable script
remains fail-closed until every mandatory gate has current evidence, including a
provenance-recorded official TPT corpus, a 7,200-second soak, and a true clean
Windows validation environment.

The candidate ZIP is generated once and then treated as immutable. Soak,
portable extraction, clean-machine, and SHA-256 evidence must all bind that
same archive hash. The validation snapshot embedded in the ZIP is explicitly
`pre_package`; the complete post-package validation is distributed beside the
ZIP as external evidence.

The fail-closed official-save runner is `tools/official_save_compatibility.py`.
CUDA is optional future work and is not included in 1.1.0.
