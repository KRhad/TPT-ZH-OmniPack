# 1.1.0

- 新增 SDL_GPU Vulkan 真实温度扩散 stencil，含 CPU reference 数值比较与自动回退。
- 新增 Windows DWARF 独立 symbols、PE 审计、确定性 manifest 与 SHA-256 发布流程。
- CUDA 仍是未实现的未来可选后端。
- 新增 run-bound 证据 schema、upstream Git object provenance、不可变 staging candidate、独立 runtime 验证与 fail-closed 最终提升。
- OmniAtmosphere OPS 状态升至 v3；历史 v2 按原语义校验后确定性迁移到当前物理下限，并且只会重新写出为 v3。
- 水/溶液粒子到气体的质量与焓耦合已有覆盖；跨相 parcel 动量与动能转移仍为未实现/未测试，不宣称完整守恒。
