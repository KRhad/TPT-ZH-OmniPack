# 第三方来源与许可证

> 本项目在开发中使用 OpenAI Codex / AI 辅助。AI 不改变第三方版权归属，也不能代替授权核验。

## 主项目许可证

TPT-ZH-OmniPack 是 The Powder Toy 的派生作品，整体按 GNU General Public License version 3 发布。必须随二进制提供完整对应源码、`LICENSE`、构建说明、修改记录和原项目致谢。

Phase 0 审计的九个仓库顶层均含 GPL-3.0 `LICENSE`，本次固定文件的 SHA-256：

`0B383D5A63DA644F628D99C33976EA6487ED89AAA59F0B3257992DEAC1171E6B`

## 固定源码来源

| 项目 | URL | 分支 | 精确 commit | 使用方式 |
|---|---|---|---|---|
| The Powder Toy | https://github.com/The-Powder-Toy/The-Powder-Toy | `master` | `bff38ce6959e1c1a7a4d17d0d5d44d127a0dfcbd` | 生产底座、官方 ID/保存/核心 |
| The Powder Toy Chinese | https://github.com/Dragonrster/The-Powder-Toy-Chinese | `i18n-new` | `445fab51dcf66057e645371aa9c9a556425b3d2f` | 翻译语料和本地化覆盖参考 |
| Cracker1000 TPT | https://github.com/cracker1000/The-Powder-Toy | `master` | `ebbb9aab6aef27d26517682cebbc0a07147a843a` | 元素/自动化设计参考，按当前 API 重写 |
| SpikeViper Biology | https://github.com/SpikeViper/The-Powder-Toy | `master` | `134ebf330eda42b4b300a2b7613ede71261697df` | 生物系统设计参考，重写 |
| TPT Ultimata Mod | https://github.com/Bowserinator/TPT-Ultimata-Mod | `development` | `b74971752433652c033559abea415ec3510ac433` | 特殊物理/电子/载具候选，精选重写 |
| Jacob1 Mod | https://github.com/jacob1/The-Powder-Toy | `c++` | `b492616124d2346a5ec7bc70fa881fb73345396b` | 自动化、UX 和存档来源识别参考 |
| TPT-Alchemy | https://github.com/jacob1/TPT-Alchemy | `master` | `9a593ce11536e2e683bc64a698399c0805ce77a1` | 炼金模式设计参考，进度实现重写 |
| Seppo's Metallurgy Mod SRC | https://github.com/SeppoTPT/Seppo-s-Metallurgy-Mod-SRC | `master` | `c3a8dd171a1c0fefc9a386e7e069f81d91f1514f` | 清洁室需求参考；实现源码缺失 |
| Cyens Toy | https://github.com/cbeimers113/cyens-toy | `master` | `f01d992c97432ec1c46d84ade05131da521f355a` | 烃化学/气体/特殊物理设计参考，重写 |

详细版本、元素数、风险与裁决见 `docs/SOURCE_AUDIT.md`；实际文件级来源进入 `docs/PORTING_LEDGER.md`。

## Phase 3 冶金、基础化学与 Phase 4 局部生态来源复核

以下复核使用仓库内可读源码和 Git 历史，不使用模组二进制。三份来源的固定快照顶层均提供 GNU GPL version 3 `LICENSE`；若未来采用具体代码，发布时仍须保留原版权、作者、文件路径和逐文件 commit 记录。

| 来源 | 固定快照与文件级锚点 | 可验证内容 | 本项目使用边界 |
|---|---|---|---|
| Seppo's Metallurgy Mod SRC | `c3a8dd171a1c0fefc9a386e7e069f81d91f1514f` | `ElementNumbers.h` 仅登记 32 个新增元素；32 个对应构造文件全部不存在；`WOOD/IRON/TTAN/COAL/SPRK/NEUT/OIL.cpp` 留有反应碎片 | 12 个 token/行为概念进入独立实现；不得从论坛二进制或描述反推缺失实现 |
| Cracker1000 COPR | 快照 `ebbb9aab6aef27d26517682cebbc0a07147a843a`；`src/simulation/elements/COPR.cpp` 最后修改 `eb474d385ffb5ebd545cf9f5f3cf513ffba9fe35`；blob `f531fb85ea2c70b19195ceba168abd41b3a522da` | 铜的颜色、导热/导电、熔点和氧化玩法；同时存在未做边界检查的远距读取及高频邻域扫描 | 本项目适配参数/行为并保留 GPL 来源；没有逐行复制原 `update`，改为 3×3 有界腐蚀和可回收 `MSCR` |
| Cyens Toy Hydrocarbon | 快照 `f01d992c97432ec1c46d84ade05131da521f355a`；相关提交锚点 `e60752b6cc0c31a0d323c22ee5a764c66a10033a`；当前主体历史还含 `6bec6d120605889efd9999e905341cc7d88d4e52` | 烃分类、相变估算和命名；同时改写官方 `GAS/OIL/MWAX/WAX` 行为 | `KERO/GASO/ACTY` 的分馏链只作独立需求参考；不覆盖官方语义，不把未完成离子体系包装为正式功能 |

### Seppo 可读反应碎片与已知问题

- `WOOD.cpp`：压力/温度和倒计时尝试产生 `CHRC`；
- `IRON.cpp`：腐蚀为 `BMTL`，高温邻近 `COAL` 时尝试生成 `STEL`；
- `TTAN.cpp`：高温邻近 `STEL` 时尝试生成 `TTSL`；
- `COAL.cpp`：极高温邻近 `COCH` 时尝试生成 `COCA`；
- `SPRK.cpp`：`IRON/COPR/BRNZ/STEL/TTSL` 通电后尝试电解邻水；
- `NEUT.cpp`：`OIL/DESL/KERO` 的随机燃料裂解；
- `OIL.cpp`：高温转换字段使用 `PT_GAS | PT_ETHL`。

上游公开记录的 bug 是合金比例错误、`WOOD -> CHRC` 不发生、酸元素为占位。源码还显示非法概率参数、极窄反应温区、连续随机转换互相覆盖，以及把两个元素 ID 按位或当作产物。由于自定义构造实现缺失，这些碎片只能证明需求和旧问题，不能证明 32 个新增元素的完整行为。

本阶段来源分类结论：

- **第三方更新函数逐行复制：0。** Seppo 的 32 个构造器不存在；Cracker COPR 原 `update` 未复制；Cyens 烃代码未进入冶金模块。
- **参数/行为适配：1。** `OMNI_PT_COPR`（稳定 ID 257）适配 Cracker 铜色、导热/导电定位、熔点和氧化玩法，并记录 `eb474d...`；本项目另写有界腐蚀算法。
- **Seppo 概念参考并独立实现：12。** `ALUM/LEAD/TIN/NICL/MAGN/CHRM/COBT/MOLY/CHRC/STEL/BRNZ/CRUC`。
- **OmniPack 原创：10。** `ZINC/COKE/BRAS/SSIL/NCRM/ALMG/TSTL/SLAG/FLUX/MSCR`，连同集中反应、每帧预算和回收机制。
- **Cyens 当前使用：概念参考。** 分馏燃料链用新稳定 ID 与独立 3×3 实现；未复制其烃状态机或官方元素改写。
- **禁止项：**不得从缺失源码、论坛二进制或只有行为描述的发布物补全实现。

23 项稳定 ID 固定为 `256–278`，完整逐项映射、构造文件和来源方式见
`docs/PORTING_LEDGER.md`。中央实现位于
`src/simulation/OmniMetallurgy.cpp`/`.h`；静态门禁位于
`tools/metallurgy_audit.py`，真实运行测试位于
`tools/runtime/metallurgy_regression.lua`。当前证据为：静态审计 PASS（23 元素、6 合金配方、1 炼钢配方、3×3 有界）、冶金单测 2/2、全部工具单测 30/30、Meson `static` 4/4、Lua 回归 PASS（7 配方、5 材料行为、ID `256–278`）。

### Phase 4 生物来源边界

SpikeViper 快照 `134ebf330eda42b4b300a2b7613ede71261697df` 仅作为“氧气、营养、感染联动”的设计参考。`NUTR/ALGA/MYCL/SPOR/PATH/STER/HUMS/BIOF` 均为本项目独立实现；没有复制第三方生物更新函数，也没有从发布二进制反推实现。新模块不覆盖官方 `PLNT`、`VIRS`、`WATR` 或 `LIFE` 的状态机，全部规则集中在 `OmniBiology.cpp` 的固定 `3x3` 局部查找中。完整与简化两种模式均由真实客户端 Lua 回归验证，未来使用任何第三方具体实现前仍须单独记录文件级来源和版权。

## 论坛与文档资料

- Seppo's Metallurgy Mod 公开主题：  
  https://powdertoy.co.uk/Discussions/Thread/View.html?Thread=24204
- Cyens Toy 公开主题：  
  https://powdertoy.co.uk/Discussions/Thread/View.html?Thread=22486
- Ultimata 仓库内 `Wiki/`：随固定 commit 一并审计。

这些资料只用于核对公开行为和已知问题；不会用论坛二进制补全缺失源码。

## 字体、图像、音效与二进制

- Dragonrster 分支的扩展 `font.bz2` 暂不进入发布：其名称、作者、来源和许可证不可追溯。
- Phase 0 没有引入新字体、音效、图片或第三方二进制。
- 后续字体必须记录原始字体文件、版本、作者、许可证、来源 URL、子集/转换命令及生成物哈希。
- 发布包不会包含任何只有二进制而没有对应合法源码的模组实现。

## 当前代码使用情况

截至 Phase 3 冶金与基础化学首批实现：

- 当前工作树正式登记并实现：23 个元素，稳定 ID `256–278`；
- 第三方更新函数逐行复制：0；
- Cracker 参数/行为适配：`COPR` 1 项，来源已固定到 `eb474d...`；
- Seppo 概念参考并独立实现：12 项；
- OmniPack 原创冶金元素：10 项；
- 基础化学正式登记元素：10 个，稳定 ID `360–369`；
- 化学来源：Seppo `ETHL/KERO/GASO` token、Cracker `CHLR/ACTY` token、Cyens 分馏链均为概念参考；对应更新实现独立编写；
- 冶金与化学静态审计、单元测试和 Lua 真实运行回归：PASS；
- 来源仓库只读审计：9；
- 未解决授权项：汉化分支中文字库 1 项；
- 缺失源码项：Seppo 自定义元素实现 32 项。
