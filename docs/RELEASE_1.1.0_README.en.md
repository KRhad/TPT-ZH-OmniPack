# TPT-ZH OmniPack 1.1.0

The stable `1.1.0` package may be produced only by
`tools/release_1_1_0.ps1 -Channel stable` after every mandatory gate passes.
The current source is an RC workflow: see `RELEASE_1.1.0_RC.md` for factual
feature status and remaining validation boundaries.

Windows x64 releases use SDL 3. The optional SDL_GPU Vulkan thermal-diffusion
path validates against CPU and falls back to CPU on any failure. CUDA is not
implemented and is not required.
