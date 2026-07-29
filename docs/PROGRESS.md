# TPT-ZH-OmniPack 开发进度

> 本文件只记录有证据支持的结果。未执行的测试明确标为“未执行”，不以编译成功替代运行验证。

## 当前阶段

Phase 1：稳定基线（完成，进入 Phase 2）

## 已完成

- 在 `C:\Users\KR\TPT-ZH-OmniPack` 建立独立、干净的 `Dragonrster/The-Powder-Toy-Chinese:i18n-new` 工作副本。
- 在 `C:\Users\KR\tpt-omnipack-sources` 建立九个固定提交的只读审计副本，未覆盖任何仓库的 `src`。
- 固定并核验官方、汉化主干、Cracker、SpikeViper、Ultimata、Jacob1、Alchemy、Seppo、Cyens 的仓库、分支、提交、时间和许可证。
- 核对各来源的元素登记数量、旧 ID 区间、当前官方 ID 冲突和主要核心系统差异。
- 对公开源码执行静态审计，记录确定的越界、除零、无限增长或高复杂度风险。
- 确认九个来源顶层均带 GPL-3.0 许可证；相同 `LICENSE` 文件的 SHA-256 为 `0B383D5A63DA644F628D99C33976EA6487ED89AAA59F0B3257992DEAC1171E6B`。
- 确认 Seppo 公共源码登记了 32 个新增元素，但缺少全部 32 个实现文件，不能编译、链接或作为可发布实现直接移植。
- 确认当前汉化分支内嵌中文字库没有可追溯的字体名称、来源或许可证，正式发布前必须替换。
- 在固定 `445fab51` 干净副本完成未经生产代码修改的 Windows x64 静态基线构建。
- 建立 `integration/zh-omnipack` 与 `backup/phase0-097cb71e` 分支。
- 合并官方 `bff38ce6959e1c1a7a4d17d0d5d44d127a0dfcbd`，保留 100.0.399 的保存与模拟安全修复。
- 人工融合 7 个冲突文件，保留 HEAC 热容量加权、MIX 除零保护和新版 FPS/Options 行为。
- 默认语言改为简体中文，设置页恢复 12 语言下拉框，`BASE` 说明进入本地化系统。
- 项目名、可执行文件名、App ID 和数据目录改为 OmniPack 独立命名，并将 `mod_id` 归零。
- 删除陈旧 `src/VcsTag.h`，修复 clean parallel build 的生成依赖竞态。

## 修改文件

- `docs/SOURCE_AUDIT.md`
- `docs/PORTING_LEDGER.md`
- `docs/ELEMENT_REGISTRY.csv`
- `docs/ELEMENT_DESIGN.md`
- `docs/I18N_AUDIT.md`
- `docs/SAVE_COMPATIBILITY.md`
- `docs/TEST_MATRIX.md`
- `docs/KNOWN_ISSUES.md`
- `docs/THIRD_PARTY_SOURCES.md`
- `docs/AI_DISCLOSURE.md`
- `docs/PROGRESS.md`
- `README.zh-CN.md`
- `CHANGELOG.zh-CN.md`

Phase 0 未修改生产代码；Phase 1 的生产修改为官方同步、冲突融合、中文基线和构建修复。

## 新增元素

0。Phase 1 没有改变官方元素登记表或 ID。

## 汉化状态

当前中文基线的严格 JSON 统计：

- 英文键：893
- 中文键：893
- 缺失键：0
- 多余键：0
- 中文空值：1
- 已确认玩家可见硬编码英文：至少 4 个明确 UI 实例，另有错误路径待系统扫描
- 默认语言：简体中文（索引 1）
- 当前设置页语言切换控件：已恢复，包含 12 种语言
- 中文正式元素名称：未实现

键集合、默认索引和编译已验证；视觉中文布局和实际下拉交互仍未执行，不能据此声称发布合格。

## 编译命令

详见 `docs/BASELINE_BUILD.md`。核心配置为 `debugoptimized`、`static=prebuilt`、静态 GCC runtime 与 gc-sections。

## 编译结果

- 未修改 100.0.398 基线：PASS，445/445，SHA-256 `A15B5D25C5552B9954040F94001C96B4289072D88B9820DCEC3FA5EDDFA69AC7`
- 官方 100.0.399 合并态：PASS，445/445，SHA-256 `433F7815624E77F2BF114FC6A923F2D61BC30AD681DF7445E3537C73A1898D44`
- 中文基线 clean build：PASS，445/445，SHA-256 `8DCC6EB3FF86200188378D273308FF94B5FDB949E2F11E36E0DA55B4B3D1CF3B`
- 错误：0；警告：2，均为 GCC 16 对 `PowderToy.cpp` 的 `std::optional<ByteString>` 路径报告

## 测试结果

- Git 工作树隔离检查：通过。
- 九来源提交固定检查：通过。
- 顶层许可证存在性与哈希检查：通过。
- 元素登记静态计数：通过。
- ID 冲突静态分析：完成。
- 未修改基线启动/响应：PASS。
- 中文基线启动/响应与独立窗口标题：PASS。
- 项目注册的 Meson 测试：0 项；退出码 0 不代表覆盖。
- 默认语言索引、语言下拉和英中键集合：静态验证 PASS。
- 视觉中文、英文切换交互、官方存档：未执行。

## 性能结果

未执行运行压力测试。静态审计已确认若干必须重写或加预算的路径，例如 Cracker `PET`/`MGNT` 大范围逐粒子搜索、Cracker `BFLM` 扩散、Ultimata 全局物理扩展及旧 Alchemy 的逐粒子配方检查。

## 已知问题

- 当前分支已合并审计时的 46 个官方后续提交；仍需持续跟踪新的上游安全修复。
- 中文字体来源与授权不可追溯，是发布阻塞项。
- Seppo 公共源码缺失新增元素实现，只能作为设计和登记表参考。
- 所有旧模组自定义 ID 均需统一迁移；不得直接信任旧数字 ID。
- Windows Computer Use 的受信任 `node_repl`/native pipe 本会话不可用，视觉 UI 自动化尚未执行。
- 官方存档样本载入/往返尚未执行。

## 下一阶段

1. 实现 `tools/i18n_audit.py` 与 `tools/element_registry_check.py`。
2. 生成不可漂移的官方 100.0 元素锁表和完整 `ELEMENT_REGISTRY.csv`。
3. 用 JsonCpp 替换宽松本地化解析器，并增加 Meson 静态测试。
4. 建立模块元数据、统一选择权限和图鉴数据框架。
5. 建立官方存档固定样本与往返测试。

## 当前 commit hash

最近已提交的官方合并：

`5ff2bccd99169bc54d61e9485ba2013b9e6adb8c`

本文件随 Phase 1 中文基线提交更新。
