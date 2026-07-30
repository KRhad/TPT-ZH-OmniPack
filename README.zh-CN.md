# TPT-ZH-OmniPack / 万象沙盘整合版

> **AI 辅助开发披露：**本项目在来源审计、代码与文档起草、构建、自动测试和翻译检查中使用 OpenAI Codex / AI 辅助。详细范围及实际测试边界见 `docs/AI_DISCLOSURE.md` 与 `docs/TEST_MATRIX.md`。

这是一个非官方、以 The Powder Toy `100.0.399` 为基础、默认简体中文并保留英文切换能力的 GPL-3.0 整合客户端。当前产品标识为 `TPT-ZH-OmniPack 0.1.0-test`；它不是 The Powder Toy 官方发布，也不是稳定版。

## 当前状态

**发布候选已具备 Release 构建、静态审计和模块运行回归；尚未达到可公开提供测试下载的门禁。**

当前测试版包含 48 个已实现 OmniPack 元素与四个可开关内容模块：工业冶金、局部生态、高级化学和受控核工业。所有元素均有固定 ID、英中名称和图鉴内容；其他预留分区不会显示为已经完成的玩法。

Windows x64 发布候选由 `tools/package_test_release.py` 从剥离后的 Release EXE 自动生成，独立符号包只供崩溃分析。封包流程拒绝 `powder.pref`、个人存档、图章、账户资料、脚本、对象文件和调试符号。包内可离线查看 `TESTING.zh-CN.md`、`SOURCE-AND-LICENSES.zh-CN.md`、`KNOWN-ISSUES.zh-CN.md`、`AI-DISCLOSURE.zh-CN.md` 与字体许可证；构建和发布证据见源码仓库 `docs/RELEASE_HARDENING.md`。

## 法律与来源

本项目依据 GNU GPL v3 发布，并保留 The Powder Toy 及各来源作者的版权与致谢。完整来源、精确提交和移植裁决见：

- `docs/SOURCE_AUDIT.md`
- `docs/THIRD_PARTY_SOURCES.md`
- `docs/PORTING_LEDGER.md`

不会从只有二进制、没有对应公开源码的模组复制实现。对应源码由当前 Git commit 标识，发布报告会写入完整 commit；发布 tag 仅在全部硬门禁通过后创建。发布包不会包含 `powder.pref`、登录令牌、个人存档或开发者隐私文件。

对应源码仓库：`https://github.com/Dragonrster/The-Powder-Toy-Chinese`。当前公共测试候选对应分支 `release/test-public-hardening` 的完整 commit 会写入 ZIP 内 `TEST-MANIFEST.txt` 和 `dist/release-report-0.1.0-test.md`；在该 commit 已推送并可从该地址取得前，不得公开分发二进制。

## 构建

Windows x64 基线和 Phase 2 clean build 已按 Meson 与官方静态链接参数复现。环境、命令、产物哈希和警告见 `docs/BASELINE_BUILD.md` 与 `docs/PROGRESS.md`。当前仓库自带的旧 `build.bat` 含开发机绝对路径，不能作为正式构建方法。

## 测试

当前已完成 Windows x64 静态构建、Meson 静态测试、工具单元测试，以及冶金、化学、生态、核工业和 Lua 模块选择运行回归。禁用模块的存档加载提示与只读保存拦截已编译并静态审计；最终 ZIP 的真实 UI 点击、简体中文/英文切换、OPS 往返、只读上传拦截和压力测试仍必须单独记录后才可公开发布。简体中文和英文是完整目标语言；其他语言的 OmniPack 新内容回退英文，不显示裸键或空字符串。

## 校验与签名

发布目录中的 `.sha256` 校验整个 ZIP，ZIP 内 `TEST-MANIFEST.txt` 校验每个成员。Windows 可运行 `Get-FileHash .\TPT-ZH-OmniPack-0.1.0-test-Windows-x64.zip -Algorithm SHA256` 后与 `.sha256` 对照。当前候选未使用 Authenticode 签名，Windows SmartScreen 可能提示；请只从明确的项目发布来源下载并先核对哈希。
