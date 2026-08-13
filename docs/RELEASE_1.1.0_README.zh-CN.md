# TPT-ZH OmniPack 1.1.0

只有 `tools/release_1_1_0.ps1 -Channel stable` 的全部强制门禁通过后，才允许
生成稳定版 `1.1.0`。当前源码走的是 RC 流程；真实功能范围与未完成门禁见
`RELEASE_1.1.0_RC.md`。

Windows x64 使用 SDL 3。可选 SDL_GPU Vulkan 温度扩散路径每次都与 CPU reference
比较，任何失败立即回退 CPU。CUDA 尚未实现，也不是运行依赖。
