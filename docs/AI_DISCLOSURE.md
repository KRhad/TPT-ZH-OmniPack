# AI 辅助开发披露

TPT-ZH-OmniPack / 万象沙盘整合版在开发过程中使用了 OpenAI Codex / AI 辅助。

## AI 辅助范围

- 仓库和许可证清点；
- Git 提交、版本、元素登记与源码差异的静态审计；
- 风险路径、ID 冲突和移植策略分析；
- 文档、构建脚本、检查脚本、测试和部分实现代码的起草与修改；
- 编译日志分析、自动测试执行和缺陷修复建议；
- 中文翻译初稿、术语统一和疑似未翻译文本扫描。

## 不代表的内容

- AI 输出不构成第三方授权证明；
- 编译成功不构成运行、性能或存档兼容证明；
- 未执行的人工可用性、翻译质量或长时间压力测试不会被描述为已通过；
- 第三方旧模组代码不会因为使用 AI 处理而失去原作者归属或 GPL 义务。

## 审核与测试记录

每次发布必须在 `docs/TEST_MATRIX.md`、`docs/PROGRESS.md` 和构建日志中列出：

- 自动执行的检查；
- 实际运行的场景；
- 人工复核范围；
- 失败、跳过和外部阻塞；
- 对应源码 commit。

`1.1.0` 发布加固包含自动 Release 构建、结构化证据门禁、DWARF 独立 symbols、GNU debug link、PE 路径/安全审计和确定性 ZIP/哈希；SDL_GPU Vulkan 用于 Enhanced OmniAtmosphere 温度扩散，并由 CPU reference 多尺寸逐次校验，失败自动回退 CPU。D3D12 只在有真实实现与证据时报告 compute，CUDA 尚未实现（未来可选后端）。SDL3 GUI、便携包、官方 upstream 存档兼容和 7,200 秒长跑的完成状态不写死在本文中，只以对应发布运行的 `RELEASE-VALIDATION.json` 与 Validation Evidence 包为准。
