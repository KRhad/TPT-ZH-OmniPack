# TPT-ZH-OmniPack / 万象沙盘整合版

> **AI 辅助开发披露：**本项目在来源审计、代码与文档起草、构建、自动测试和翻译检查中使用 OpenAI Codex / AI 辅助。详细范围及实际测试边界见 `docs/AI_DISCLOSURE.md` 与 `docs/TEST_MATRIX.md`。

这是一个以 The Powder Toy 100.0 系列为基础、默认简体中文、保留英文切换能力的独立 GPL-3.0 整合客户端。项目目标是把经过筛选的冶金、生物、化学、核能、自动化和特殊物理内容组织成互相联动的玩法体系，并保留普通沙盒模式。

## 当前状态

**Phase 1 稳定基线完成，Phase 2 基础设施开发中；尚未发布正式整合版。**

现阶段没有可供玩家使用的 `dist/TPT-ZH-OmniPack-Windows-x64.zip`。任何第三方元素进入正式菜单前都必须完成稳定 ID、来源、中文图鉴、实际用途、性能预算和存档迁移审查。

## 法律与来源

本项目依据 GNU GPL v3 发布，并保留 The Powder Toy 及各来源作者的版权与致谢。完整来源、精确提交和移植裁决见：

- `docs/SOURCE_AUDIT.md`
- `docs/THIRD_PARTY_SOURCES.md`
- `docs/PORTING_LEDGER.md`

不会从只有二进制、没有对应公开源码的模组复制实现。发布包不会包含 `powder.pref`、登录令牌、个人存档或开发者隐私文件。

## 构建

Windows x64 基线已按 Meson 与官方静态链接参数复现，环境、命令、产物哈希和警告见 `docs/BASELINE_BUILD.md`。当前仓库自带的旧 `build.bat` 含开发机绝对路径，不能作为正式构建方法。

## 测试

当前已完成固定源码快照、许可证、元素 ID、静态风险审计、Windows x64 静态构建和进程级启动测试；视觉 UI、存档和压力测试仍未完成。不得把当前状态描述为可发布。
