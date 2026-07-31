# TPT-ZH-OmniPack / 万象沙盘整合版

> **AI 辅助开发披露：**本项目在来源审计、代码与文档起草、构建、自动测试和翻译检查中使用 OpenAI Codex / AI 辅助。详细范围及实际测试边界见 `docs/AI_DISCLOSURE.md` 与 `docs/TEST_MATRIX.md`。

这是一个非官方、以 The Powder Toy `100.0.399` 为基础、默认简体中文并保留英文切换能力的 GPL-3.0 整合客户端。当前产品标识为 `TPT-ZH-OmniPack 0.1.0-test`；它不是 The Powder Toy 官方发布，也不是稳定版。

## 当前状态

**公开基线尚未达到发布门禁；长期开发分支已完成 0.2 跨模块、0.3 自动化和 0.4 炼金探索的本地功能证据。**

开发分支另有明确标记的本地验证包：0.2 提供 7 个示例 OPS、8 项教程和四条跨模块闭环；0.3 提供 9 个官方元件自动化场景与 6 项挑战；0.4 提供独立炼金模式、十阶段进度、OPS 持久化和统一防绕过门禁。它们不替代 `0.1.0-test` 的外部门禁，也不是公开版本；精确提交、哈希和未测试边界见 `docs/PHASE_0_2_EVIDENCE.md`、`docs/PHASE_0_3_EVIDENCE.md` 与 `docs/PHASE_0_4_EVIDENCE.md`。

当前测试版包含 48 个已实现 OmniPack 元素与四个可开关内容模块：工业冶金、局部生态、高级化学和受控核工业。所有元素均有固定 ID、英中名称和图鉴内容；其他预留分区不会显示为已经完成的玩法。

Windows x64 发布候选由 `tools/package_test_release.py` 从剥离后的 Release EXE 自动生成，独立符号包只供崩溃分析。默认 `0.1.0-test/public-test` 配置拒绝 `powder.pref`、个人存档、未登记图章、账户资料、脚本、对象文件和调试符号；显式 local-dev 配置只允许相应版本白名单中的 0.2 示例、0.3 自动化场景和 0.4 炼金图。包内可离线查看测试、来源、已知问题、AI 披露与字体许可证；构建和发布证据见源码仓库 `docs/RELEASE_HARDENING.md` 与各阶段 evidence 文档。

## 法律与来源

本项目依据 GNU GPL v3 发布，并保留 The Powder Toy 及各来源作者的版权与致谢。完整来源、精确提交和移植裁决见：

- `docs/SOURCE_AUDIT.md`
- `docs/THIRD_PARTY_SOURCES.md`
- `docs/PORTING_LEDGER.md`

不会从只有二进制、没有对应公开源码的模组复制实现。对应源码由当前 Git commit 标识，发布报告会写入完整 commit；发布 tag 仅在全部硬门禁通过后创建。发布包不会包含 `powder.pref`、登录令牌、个人存档或开发者隐私文件。

当前尚未确认经授权的 OmniPack 正式源码仓库。Git remote `origin` 仍指向历史汉化仓库 `Dragonrster/The-Powder-Toy-Chinese`，不得把它描述为当前候选已公开的对应源码，也不得擅自推送。完整候选 commit 已写入 ZIP 内 `TEST-MANIFEST.txt` 和 `dist/release-report-0.1.0-test.md`；只有在安全认证、授权远端、匿名克隆和重建全部验证后，README 才会公布正式源码地址。

## 构建

Windows x64 基线和 Phase 2 clean build 已按 Meson 与官方静态链接参数复现。环境、命令、产物哈希和警告见 `docs/BASELINE_BUILD.md` 与 `docs/PROGRESS.md`。当前仓库自带的旧 `build.bat` 含开发机绝对路径，不能作为正式构建方法。

## 测试

当前已完成 Windows x64 Release clean build、Meson/Python 静态测试、四模块运行回归、官方与模块 OPS 双往返、0.2 示例/教程、0.3 自动化场景/挑战及 0.4 炼金十阶段。炼金精通状态连续 100 次真实 OPS 序列化、解析和导入保持一致；普通沙盒、锁定模式、模块关闭和损坏进度均有独立回归。0.1 十个正式压力样本、0.2 四条跨模块链和 0.3 自动化压力已有有限时长证据。GUI 本地 `.cps` 保存路径仍未点击；最终 ZIP 的简体中文/英文切换、DPI、禁用模块三选项、只读上传拦截和炼金进度窗口仍需可信交互证据，1.0 两小时综合长跑也尚未完成。当前环境 PAT 未有撤销证据，故不会推送或公开发布；详见 `docs/FINAL_VALIDATION.md`。简体中文和英文是完整目标语言；其他语言的 OmniPack 新内容回退英文，不显示裸键或空字符串。

## 校验与签名

发布目录中的 `.sha256` 校验整个 ZIP，ZIP 内 `TEST-MANIFEST.txt` 校验每个成员。Windows 可运行 `Get-FileHash .\TPT-ZH-OmniPack-0.1.0-test-Windows-x64.zip -Algorithm SHA256` 后与 `.sha256` 对照。当前候选未使用 Authenticode 签名，Windows SmartScreen 可能提示；请只从明确的项目发布来源下载并先核对哈希。
