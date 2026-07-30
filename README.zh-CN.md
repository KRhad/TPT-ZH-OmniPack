# TPT-ZH-OmniPack / 万象沙盘整合版

> **AI 辅助开发披露：**本项目在来源审计、代码与文档起草、构建、自动测试和翻译检查中使用 OpenAI Codex / AI 辅助。详细范围及实际测试边界见 `docs/AI_DISCLOSURE.md` 与 `docs/TEST_MATRIX.md`。

这是一个以 The Powder Toy 100.0 系列为基础、默认简体中文、保留英文切换能力的独立 GPL-3.0 整合客户端。项目目标是把经过筛选的冶金、生物、化学、核能、自动化和特殊物理内容组织成互相联动的玩法体系，并保留普通沙盒模式。

## 当前状态

**测试版整合包已具备构建、静态审计和模块运行回归；尚不是正式公开发布。**

当前测试版包含 48 个已实现 OmniPack 元素与四个可开关内容模块：工业冶金、局部生态、高级化学和受控核工业。所有元素均有固定 ID、英中名称和图鉴内容；其他预留分区不会显示为已经完成的玩法。

Windows x64 测试包由 `tools/package_test_release.py` 从已验证的静态构建生成，且封包流程拒绝 `powder.pref`、个人存档、图章、账户资料与脚本。具体启动方式、测试场景、已验证范围和已知限制见 `docs/TEST_RELEASE.md` 与 `docs/TEST_MATRIX.md`。

## 法律与来源

本项目依据 GNU GPL v3 发布，并保留 The Powder Toy 及各来源作者的版权与致谢。完整来源、精确提交和移植裁决见：

- `docs/SOURCE_AUDIT.md`
- `docs/THIRD_PARTY_SOURCES.md`
- `docs/PORTING_LEDGER.md`

不会从只有二进制、没有对应公开源码的模组复制实现。发布包不会包含 `powder.pref`、登录令牌、个人存档或开发者隐私文件。

## 构建

Windows x64 基线和 Phase 2 clean build 已按 Meson 与官方静态链接参数复现。环境、命令、产物哈希和警告见 `docs/BASELINE_BUILD.md` 与 `docs/PROGRESS.md`。当前仓库自带的旧 `build.bat` 含开发机绝对路径，不能作为正式构建方法。

## 测试

当前完成 Windows x64 静态构建、10/10 Meson 静态测试、50/50 工具单元测试，以及冶金、化学、生物、核工业和 Lua 模块选择运行回归。禁用模块的存档加载提示与只读保存拦截已编译并静态审计；真实 UI 点击、OPS 往返和压力测试仍在测试范围内。详细边界见 `docs/TEST_MATRIX.md`，不得把测试版描述为正式公开发布。
