# TPT-ZH-OmniPack 开发进度

> 本文件只记录有证据支持的结果。未执行的测试明确标为“未执行”，不以编译成功替代运行验证。

## 当前阶段

Phase 0：来源审计（文档收尾中）

## 已完成

- 在 `C:\Users\KR\TPT-ZH-OmniPack` 建立独立、干净的 `Dragonrster/The-Powder-Toy-Chinese:i18n-new` 工作副本。
- 在 `C:\Users\KR\tpt-omnipack-sources` 建立九个固定提交的只读审计副本，未覆盖任何仓库的 `src`。
- 固定并核验官方、汉化主干、Cracker、SpikeViper、Ultimata、Jacob1、Alchemy、Seppo、Cyens 的仓库、分支、提交、时间和许可证。
- 核对各来源的元素登记数量、旧 ID 区间、当前官方 ID 冲突和主要核心系统差异。
- 对公开源码执行静态审计，记录确定的越界、除零、无限增长或高复杂度风险。
- 确认九个来源顶层均带 GPL-3.0 许可证；相同 `LICENSE` 文件的 SHA-256 为 `0B383D5A63DA644F628D99C33976EA6487ED89AAA59F0B3257992DEAC1171E6B`。
- 确认 Seppo 公共源码登记了 32 个新增元素，但缺少全部 32 个实现文件，不能编译、链接或作为可发布实现直接移植。
- 确认当前汉化分支内嵌中文字库没有可追溯的字体名称、来源或许可证，正式发布前必须替换。

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

Phase 0 未修改生产代码。

## 新增元素

0。Phase 0 只审计候选元素，不分配或注册正式 ID。

## 汉化状态

固定提交 `445fab51dcf66057e645371aa9c9a556425b3d2f` 的严格 JSON 基线：

- 英文键：878
- 中文键：878
- 缺失键：0
- 多余键：0
- 中文空值：1
- 已确认玩家可见硬编码英文：至少 4 个明确 UI 实例，另有错误路径待系统扫描
- 默认语言：英文
- 当前设置页语言切换控件：缺失
- 中文正式元素名称：未实现

这不是发布合格状态。

## 编译命令

Phase 0 不编译。Phase 1 将先按仓库 Meson 与 `.github/build.sh` 的实际配置执行未经生产代码修改的 Windows x64 基线构建。

## 编译结果

未执行（Phase 1 门禁）。

## 测试结果

- Git 工作树隔离检查：通过。
- 九来源提交固定检查：通过。
- 顶层许可证存在性与哈希检查：通过。
- 元素登记静态计数：通过。
- ID 冲突静态分析：完成。
- 本地运行、存档和 UI 测试：未执行（Phase 1 起执行）。

## 性能结果

未执行运行压力测试。静态审计已确认若干必须重写或加预算的路径，例如 Cracker `PET`/`MGNT` 大范围逐粒子搜索、Cracker `BFLM` 扩散、Ultimata 全局物理扩展及旧 Alchemy 的逐粒子配方检查。

## 已知问题

- 汉化主干落后当前官方 master 46 个提交，其中包含多项崩溃、越界、除零和 BSON 边界修复。
- 汉化主干默认仍为英文，且当前 Options UI 没有语言切换控件。
- `BASE` 描述硬编码中文，破坏英文切换。
- 中文字体来源与授权不可追溯，是发布阻塞项。
- Seppo 公共源码缺失新增元素实现，只能作为设计和登记表参考。
- 所有旧模组自定义 ID 均需统一迁移；不得直接信任旧数字 ID。
- Phase 1 之前尚无本机基线构建和运行证据。

## 下一阶段

1. 完成并提交 Phase 0 审计文档。
2. 在生产源码未经修改状态完成 Windows x64 基线构建并保留完整日志。
3. 进行启动、中文显示、英文切换能力和官方存档基线验证。
4. 建立 `integration/zh-omnipack` 分支。
5. 以当前官方 100.0.399 安全修复为底座重放本地化，而不是目录覆盖。

## 当前 commit hash

Phase 0 文档尚未提交时的源码基线：

`445fab51dcf66057e645371aa9c9a556425b3d2f`

