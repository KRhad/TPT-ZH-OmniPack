# 移植账本

> Phase 0 未复制第三方实现代码。Phase 3 当前工作树已登记并实现 23 个冶金元素与 10 个基础化学元素。下文的“没有逐行复制”特指没有复制第三方元素更新函数；`COPR` 的颜色、导热/导电定位、熔点和氧化玩法由 Cracker 源码适配，不能笼统标成完全原创。

状态值：`AUDITED`、`DESIGN`、`PORTING`、`TESTING`、`ACCEPTED`、`REJECTED`、`BLOCKED`。

| 模块/候选 | 来源与固定 commit | 原作者/归属 | Phase 0 判定 | 目标方式 | 当前状态 | 理由与门禁 |
|---|---|---|---|---|---|---|
| 官方模拟、保存、ID | Official `bff38ce...` | TPT contributors | 直接底座 | 保留并同步 | AUDITED | 100.0.399 权威实现 |
| 中文/英文语料与严格加载 | Dragonrster `445fab51...`；OmniPack `457233ac...` | Dragonrster、TPT contributors、OmniPack contributors | 审核后重放并重写加载器 | JsonCpp 严格 JSON + 自动审计 | ACCEPTED | en/zh 1,252/1,252，0 阻塞错误；视觉复核仍独立跟踪 |
| 官方元素稳定锁 | Official `bff38ce...`；OmniPack `08fe8a82...` | TPT contributors、OmniPack contributors | 源码解析生成 | 逐槽固定 0–195 | ACCEPTED | 196 槽、195 活动、1 tombstone；自动门禁 PASS |
| 模块选择与图鉴框架 | OmniPack `d7312a9b...`、`f28cdcb7...` | OmniPack contributors | 项目原创实现 | 统一门禁、编译时登记目录 | ACCEPTED | clean build、28 单测、Lua 运行回归和枚举本地化门禁 PASS |
| Phase 3 工业冶金首批 | Seppo `c3a8dd17...`；Cracker COPR `eb474d38...`；OmniPack 当前实现 | SeppoTPT、Cracker contributors、OmniPack contributors | 概念筛选、参数/行为适配、独立更新实现 | 稳定 ID 256–278 + 集中反应引擎 | TESTING | 23 元素、7 配方、5 材料行为；静态门禁与 Lua 运行回归 PASS |
| Phase 3 基础化学首批 | Seppo `c3a8dd17...`；Cracker `ebbb9aab...`；Cyens `f01d992c...`；OmniPack 当前实现 | SeppoTPT、Cracker contributors、cbeimers113、OmniPack contributors | token/玩法需求参考，集中算法独立实现 | 稳定 ID 360–369 + 中央有界反应引擎 | TESTING | 10 元素、11 条反应路径、化学静态门禁与 Lua 真实运行回归 PASS |
| Phase 5 受控核工业首批 | OmniPack 当前实现；Spike/Ultimata/Cracker 固定快照仅作范围参考 | OmniPack contributors；对应来源作者 | 玩法和风险边界参考，集中算法独立实现 | 稳定 ID 328–334 + 中央有界反应器引擎 | TESTING | 7 元素、4 类受控路径；静态门禁与 Lua 真实运行回归 PASS |
| Phase 3 周期表前七批 | OmniPack `21b160a5...`、`bcf5accf...`、`546b8791...`、`5d9b9947...`、`de91bbb6...`、`2eeab9f3...`、`3c3c623a...` 基线；候选目录只核对搜索覆盖 | OmniPack contributors | 原创族逻辑；未复制外部候选实现 | 稀有气体、碱金属、碱土金属、硼族、碳族、氮族及氧族 33 个新固定 ID + 共享周期引擎 | TESTING | 33 新元素、59/118 映射；行为、预算、OPS 与直接选择回归 PASS |
| 汉化字体 | Dragonrster `445fab51...` | 未知 | 禁止发布 | 替换为可追溯字体 | BLOCKED | 名称、来源、许可证缺失 |
| Cracker 工业化学 | Cracker `ebbb9aab...` | Cracker1000 等 | token/玩法需求参考 | 中央反应表 | TESTING | `CHLR/ACTY` 以新稳定 ID 独立实现；不复制 5×5 更新，不复用旧 ID |
| 动力门户 PPTI/PPTO | Cracker `ebbb9aab...`; Jacob `b4926161...` | 各来源作者 | 合并重写 | 单一稳定实现 | DESIGN | 多来源重复、旧 API |
| CSNS/GSNS/导线/PCON | Cracker `ebbb9aab...` | Cracker1000 | 重写候选 | 当前电子 API + 预算 | DESIGN | 自动化价值明确 |
| PET/BEE 原实现 | Cracker `ebbb9aab...` | Cracker1000 | 排除原实现 | 如需概念则另行设计 | REJECTED | 大范围扫描/越界风险 |
| BFLM/CEXP/EXPL | Cracker/Jacob | 各来源作者 | 排除原实现 | 预算化灾害另设计 | REJECTED | 无受控扩散门禁 |
| MGNT | Cracker `ebbb9aab...` | Cracker1000 | 重写候选 | 半径/帧预算磁体 | DESIGN | 原实现每粒子 6,561 格扫描 |
| Spike 生物循环 | SpikeViper `134ebf33...` | SpikeViper contributors | 概念参考；独立实现 | 事件驱动局部状态 | PORTED | `288..295` 局部营养/氧气/感染/处理循环；不复制其更新函数 |
| Spike/Ultimata/Cracker 核内容 | Spike `134ebf33...`；Ultimata `b7497175...`；Cracker `ebbb9aab...` | 对应来源作者 | 只读玩法和风险范围参考 | 原创局部反应器 | PORTED | `328..334` 不复制第三方核更新；不改写官方 `URAN/PLUT/NEUT/DEUT` |
| Ultimata 传送/漏斗/力场 | Ultimata `b7497175...` | Bowserinator 等 | 精选重写 | 工程化可控元素 | DESIGN | 必须有克制与保存测试 |
| Ultimata 时间/电磁核心 | Ultimata `b7497175...` | Bowserinator 等 | 实验参考 | 默认关闭，架构先行 | DESIGN | 确定性/网络/性能风险 |
| Jacob BUTN/PWHT | Jacob `b4926161...` | jacob1 等 | 重写候选 | 当前 API + 洪泛预算 | DESIGN | UX 价值明确，旧代码越界 |
| Jacob MOVS/ANIM | Jacob `b4926161...` | jacob1 等 | 仅需求参考 | 新数据结构才可实现 | DESIGN | 旧内存/边界/冻结风险 |
| Jacob 保存来源标记 | Jacob `b4926161...` | jacob1 | 迁移参考 | 只读识别 `"Jacob1's_Mod"` | DESIGN | 可帮助可靠识别来源 |
| Alchemy 四元素开局/发现 | Alchemy `9a593ce1...` | jacob1/SopaXorzTaker 等 | 排除 | 不移植 | REJECTED | 与全部内容直接可用的产品方向冲突 |
| Alchemy 原进度代码 | Alchemy `9a593ce1...` | 原仓库作者 | 排除 | 不移植 | REJECTED | 越界、未初始化、可绕过且属于已删除系统 |
| Seppo 冶金实现 | Seppo `c3a8dd17...` | SeppoTPT | 无可移植源码 | 清洁室重写 | BLOCKED | 32 个实现文件全部缺失 |
| Seppo 元素/合金清单 | Seppo `c3a8dd17...` 与论坛 | SeppoTPT | 设计参考 | 去重后自研 | TESTING | 12 个概念条目已用独立实现进入首批冶金；未复制缺失构造器 |
| Cyens 烃网络 | Cyens `f01d992c...` | cbeimers113 | 设计参考 | 不改官方基础语义 | TESTING | 裂化链使用新 `KERO/GASO/ACTY`；未复制其官方 GAS/OIL/WAX 改写 |
| Cyens 时间/局部重力 | Cyens `f01d992c...` | cbeimers113 | 实验参考 | 默认关闭 | DESIGN | 未完成离子系统、全局风险 |

周期表碱金属批次复用官方 `LITH=191` 与 `RBDM=41`，没有复制或覆盖其更新函数；`NA=376`、`K=380`、`CS=406`、`FR=432` 的构造器和 `OmniPeriodic.cpp` 共享逻辑均为本项目原创实现。外部 `fun_chemicals` 只作为候选搜索命中保留在来源映射中，不是代码或常量来源，因此第三方 `elements_ported` 和 `elements_rewritten` 仍为 0。

周期表碱土金属批次复用既有 `MAGN=261`，在保留其冶金、合金、碎料与旧 ID 行为的同时增加共享水/酸入口及白色焰色；`BE=371`、`CA=381`、`SR=391`、`BA=407`、`RA=433` 均为本项目原创族实现，没有复制候选模组构造器或更新函数。

周期表硼族批次复用既有 `ALUM=256` 并保留其冶金、合金、碎料与旧 ID 行为；`B=372`、`GA=385`、`IN=401`、`TL=428`、`NH=456` 的构造器和共享逻辑均为本项目原创实现。硼中子俘获、镓脆铝、两性反应代理和鿭压缩衰变没有复制候选模组代码或常量表。

周期表碳族批次复用官方 `DMND=28`、`SLCN=187` 与既有 `TIN=259`、`LEAD=258`，保持原稳定 ID、钻石不可破坏语义、硅电气行为以及锡/铅冶金路径；`GE=386`、`FL=457` 的构造器和共享逻辑为本项目原创实现。锗有限放电、锡瘟、分级酸/氧/汽化路线和𫓧压缩衰变没有复制候选模组代码或常量表。

周期表氮族批次新增 `N=373`、`P=377`、`AS=387`、`SB=402`、`BI=429`、`MC=458`，构造器与共享逻辑均为本项目原创实现；仅把官方 `LNTG=37`、`NICE=51` 的既有低温相安全连接到常温氮。氮放电、磷燃烧、分级酸/氧/汽化和 `MC → NH → POLO` 压缩衰变没有复制候选模组代码或常量表；`biological_mod` 只保留为氮候选搜索命中。

周期表氧族批次复用官方 `O2=61`、`LO2=60` 和 `POLO=182`，不覆盖其更新函数、相变或稳定 ID；`S=378`、`SE=388`、`TE=403`、`LV=459` 的构造器和共享逻辑均为本项目原创实现。外部 `chem_mod_lua` 只作为硫候选搜索命中保留在来源映射中；硫烟雾燃烧、硒光敏辉光、硒/碲氧化与汽化及 `LV → FL → POLO` 压缩衰变没有复制候选模组代码或常量表。

## Phase 3 冶金来源与实现复核

三份来源的固定快照均带 GNU GPL version 3 `LICENSE`；许可证允许在遵守 GPL 的前提下改编可获得的源码，但不允许把缺失实现或论坛二进制猜测成“已移植源码”。当前 23 个元素已进入生产源码和登记表；实现提交就是包含本账本更新的 Phase 3 模块提交，提交后由 `docs/PROGRESS.md` 记录精确 hash。

### Seppo：只能参考清单与公开行为

固定来源为 `SeppoTPT/Seppo-s-Metallurgy-Mod-SRC` `master`：
`c3a8dd171a1c0fefc9a386e7e069f81d91f1514f`。`ElementNumbers.h` 登记了旧 ID `187..218` 的 32 个新增元素：

```text
CHRC SFAC ETHL KERO GASO TIN COPR BRNZ HELI LEAD COBA STEL CROM ALUM
DURA COCH MLYB SODI CHLR COCA ANAC TTSL CRUC SALD MAGN NICK ALBR
ALNI ALNC TERN MAGX PLTU
```

固定树中不存在上述任一元素的 `src/simulation/elements/<NAME>.cpp`，因此没有可审计的构造器、属性、更新函数或图形函数。能够复核的只有对官方元素文件的零散修改：

| 原文件 | 可见反应碎片 | Phase 3 裁决 |
|---|---|---|
| `WOOD.cpp` | 压力/温度与计时逻辑试图把 `WOOD` 转为 `CHRC` | 只作为木炭生产需求；独立重写 |
| `IRON.cpp` | 铁腐蚀为 `BMTL`；约 `1670 K` 时在 5×5 邻域消耗 `COAL` 生成 `STEL` | 只采用“冶炼钢+腐蚀差异”概念；反应表重写 |
| `TTAN.cpp` | 约 `1940 K` 时在 5×5 邻域消耗 `STEL` 生成 `TTSL` | 只作为高温合金需求；配方和窗口重设计 |
| `COAL.cpp` | 高温 `COAL + COCH -> COCA` | 不移植原概率代码；按集中配方重写 |
| `SPRK.cpp` | `IRON/COPR/BRNZ/STEL/TTSL` 通电后把邻水转换为 `O2` 或 `H2` | 只采用电解联动概念；统一电解机制重写 |
| `NEUT.cpp` | `OIL/DESL` 随机裂解为 `GAS/GASO/KERO`，`KERO` 可继续变为 `GASO` | 只采用燃料裂解概念；不把中子随机覆盖链原样移入 |
| `OIL.cpp` | 高温转换字段写成 `PT_GAS \| PT_ETHL` | 拒绝原实现；按单一、可验证产物规则重写 |

公开主题明确记录了合金比例错误、`WOOD -> CHRC` 不发生、酸元素只是占位。源码复核另确认了以下风险：

- `OIL.cpp` 用元素 ID 做按位或，不能表达多产物反应；
- `COAL.cpp` 出现 `RNG::Ref().chance(3, 1)`，概率参数不合法；
- `TTAN.cpp` 的反应温度只比熔化转换低约 `1 K`，并使用恒真的 `chance(1, 1)`；
- `NEUT.cpp` 对同一粒子连续执行互相覆盖的随机转换；
- 32 个新增元素构造实现缺失，不能从登记表、论坛描述或二进制反推出代码。

因此 Phase 3 对 Seppo 的使用方式固定为：**概念参考 + 独立实现**。当前采用其公开 token/行为概念的 12 项是 `ALUM/LEAD/TIN/NICL/MAGN/CHRM/COBT/MOLY/CHRC/STEL/BRNZ/CRUC`；对应构造器和中央更新函数均由本项目按 TPT 100.0 API 编写。不得标为逐行移植，不得从发布二进制补全；登记表继续保留 Seppo commit 和旧 ID 以提供概念来源与迁移线索。

### Cracker：COPR 参数/行为适配，更新函数独立实现

固定仓库快照为 `cracker1000/The-Powder-Toy` `master`
`ebbb9aab6aef27d26517682cebbc0a07147a843a`。该快照中的
`src/simulation/elements/COPR.cpp` 内容最后由
`eb474d385ffb5ebd545cf9f5f3cf513ffba9fe35` 修改；固定 blob 为
`f531fb85ea2c70b19195ceba168abd41b3a522da`。

本项目的 `src/simulation/elements/COPR.cpp` 明确属于 **参数/行为适配**：保留了 Cracker 文件的铜色 `0xB87333`、高导热 `255`、导电/热辉光定位、约 `1358 K` 熔点和“氧化削弱材料”的玩法目标，因此登记来源是 Cracker、旧 ID 为 `222`、文件级 commit 为 `eb474d...`。

没有逐行复制 Cracker 的 `update`：

- 原实现是可移动 `TYPE_PART`，本项目使用固定 `TYPE_SOLID`；
- 原实现用无边界检查的 `±8` 读取模拟低温超导，本项目删除该路径；
- 原实现每粒子扫描 5×5 邻域，本项目经 `OmniMetallurgyMetalUpdate` 只检查有边界保护的 3×3 邻域；
- 本项目把氧气、盐水和水造成的腐蚀累积到 `tmp`，最终生成携带 `ctype=COPR` 的 `MSCR`，支持回炉恢复；这是新的兼容/回收设计；
- 铜与锡、锌的 3:1 熔融配方由集中反应表验证，不沿用 Cracker 更新代码。

因此法律与工程标签是“GPL 来源参数/行为适配 + 本项目更新算法重写”，不是“未使用 Cracker”，也不是“逐行复制 Cracker 更新函数”。

### Cyens：烃网络仅作基础化学设计参考

固定仓库快照为 `cbeimers113/cyens-toy` `master`
`f01d992c97432ec1c46d84ade05131da521f355a`。烃系统的文件级相关提交锚点为
`e60752b6cc0c31a0d323c22ee5a764c66a10033a`（`Remove hydrocarbon decomposition`）；
当前 `Hydrocarbon.cpp` 主体的逐行历史还回溯到
`6bec6d120605889efd9999e905341cc7d88d4e52`，所以 `e60752...` 不能被误写成全部代码的唯一作者来源。

可参考内容是烷烃/烯烃/炔烃分类、碳数对应相变点及名称生成。固定快照同时重定义官方
`GAS/OIL/MWAX/WAX` 语义，而且后续离子体系仍未完成。基础化学首批只采用“分馏燃料链 + 独立稳定标识符”的需求：新 `KERO/GASO/ACTY` 由 `OmniChemistry.cpp` 的 3×3 有界规则产生，官方 `GAS/OIL/MWAX/WAX` 构造器和状态转换没有被替换或复制。

### 方式标签

| 标签 | 本阶段含义 |
|---|---|
| 第三方更新函数逐行复制 | **0**；Seppo 构造器缺失，Cracker COPR 的原 `update` 未复制，Cyens 烃代码未进入冶金模块 |
| 参数/行为适配 | **1 个元素：COPR**；保留可追溯参数和玩法定位，更新算法按当前 API 重写 |
| 概念参考 | Seppo 的 12 个冶金 token/行为；Seppo 的 `ETHL/KERO/GASO` token；Cracker 的 `CHLR/ACTY` token；Cyens 的分馏燃料链均进入独立实现 |
| 本项目原创设计 | 10 个冶金元素和 `AMON/CATA/POLY/PERO/FERT`，连同集中反应、性能预算、回收机制；来源登记为 `TPT-ZH-OmniPack/original` |

### 当前稳定 ID 与目标文件

稳定 ID `196–255` 仍是兼容保留区；冶金元素只占用固定区间 `256–278`，没有改变官方 `0–195`。

| 稳定 ID | identifier / 代号 | 来源方式 | 本项目构造文件 |
|---:|---|---|---|
| 256 | `OMNI_PT_ALUM` / `ALUM` | Seppo token/概念；独立实现 | `src/simulation/elements/ALUM.cpp` |
| 257 | `OMNI_PT_COPR` / `COPR` | Cracker 参数/行为适配；更新算法重写 | `src/simulation/elements/COPR.cpp` |
| 258 | `OMNI_PT_LEAD` / `LEAD` | Seppo token/概念；独立实现 | `src/simulation/elements/LEAD.cpp` |
| 259 | `OMNI_PT_TIN` / `TIN` | Seppo token/概念；独立实现 | `src/simulation/elements/TIN.cpp` |
| 260 | `OMNI_PT_NICL` / `NICL` | Seppo `NICK` 概念重命名；独立实现 | `src/simulation/elements/NICL.cpp` |
| 261 | `OMNI_PT_MAGN` / `MAGN` | Seppo token/概念；独立实现 | `src/simulation/elements/MAGN.cpp` |
| 262 | `OMNI_PT_CHRM` / `CHRM` | Seppo `CROM` 概念重命名；独立实现 | `src/simulation/elements/CHRM.cpp` |
| 263 | `OMNI_PT_COBT` / `COBT` | Seppo `COBA` 概念重命名；独立实现 | `src/simulation/elements/COBT.cpp` |
| 264 | `OMNI_PT_MOLY` / `MOLY` | Seppo `MLYB` 概念重命名；独立实现 | `src/simulation/elements/MOLY.cpp` |
| 265 | `OMNI_PT_ZINC` / `ZINC` | OmniPack 原创 | `src/simulation/elements/ZINC.cpp` |
| 266 | `OMNI_PT_CHRC` / `CHRC` | Seppo 木炭概念；有界炭化重写 | `src/simulation/elements/CHRC.cpp` |
| 267 | `OMNI_PT_COKE` / `COKE` | OmniPack 原创；不采用含混的 `COCH/COCA` | `src/simulation/elements/COKE.cpp` |
| 268 | `OMNI_PT_STEL` / `STEL` | Seppo 炼钢概念；配方重写 | `src/simulation/elements/STEL.cpp` |
| 269 | `OMNI_PT_BRNZ` / `BRNZ` | Seppo token/概念；3:1 配方重写 | `src/simulation/elements/BRNZ.cpp` |
| 270 | `OMNI_PT_BRAS` / `BRAS` | OmniPack 原创 | `src/simulation/elements/BRAS.cpp` |
| 271 | `OMNI_PT_SSIL` / `SSIL` | OmniPack 原创；不复用 Seppo 的钛钢 `TTSL` | `src/simulation/elements/SSIL.cpp` |
| 272 | `OMNI_PT_NCRM` / `NCRM` | OmniPack 原创 | `src/simulation/elements/NCRM.cpp` |
| 273 | `OMNI_PT_ALMG` / `ALMG` | OmniPack 原创；不复制未定义的 Seppo `MAGX` | `src/simulation/elements/ALMG.cpp` |
| 274 | `OMNI_PT_TSTL` / `TSTL` | OmniPack 原创钴钼工具钢 | `src/simulation/elements/TSTL.cpp` |
| 275 | `OMNI_PT_SLAG` / `SLAG` | OmniPack 原创工艺副产物 | `src/simulation/elements/SLAG.cpp` |
| 276 | `OMNI_PT_FLUX` / `FLUX` | OmniPack 原创工艺材料 | `src/simulation/elements/FLUX.cpp` |
| 277 | `OMNI_PT_CRUC` / `CRUC` | Seppo token/坩埚概念；独立实现 | `src/simulation/elements/CRUC.cpp` |
| 278 | `OMNI_PT_MSCR` / `MSCR` | OmniPack 原创、携带 `ctype` 的可回收碎料 | `src/simulation/elements/MSCR.cpp` |

## Phase 3 基础化学来源与实现复核

化学首批的提交包含 `src/simulation/OmniChemistry.cpp`/`.h`、十个构造器、模块选择门禁、本地化、登记、审计和真实客户端 Lua 回归。所有反应实现由本项目按 TPT 100.0 API 独立编写；没有复制 Cracker、Seppo 或 Cyens 的元素更新函数。

| 稳定 ID | identifier / 代号 | 来源方式 | 本项目构造文件 |
|---:|---|---|---|
| 360 | `OMNI_PT_CHLR` / `CHLR` | Cracker 氯气 token/玩法目标；3×3 反应独立实现 | `src/simulation/elements/CHLR.cpp` |
| 361 | `OMNI_PT_AMON` / `AMON` | OmniPack 原创的受压火花催化近似 | `src/simulation/elements/AMON.cpp` |
| 362 | `OMNI_PT_ETHL` / `ETHL` | Seppo token；发酵规则独立实现 | `src/simulation/elements/ETHL.cpp` |
| 363 | `OMNI_PT_KERO` / `KERO` | Seppo token + Cyens 分馏链需求；集中裂化独立实现 | `src/simulation/elements/KERO.cpp` |
| 364 | `OMNI_PT_GASO` / `GASO` | Seppo token + Cyens 分馏链需求；集中裂化独立实现 | `src/simulation/elements/GASO.cpp` |
| 365 | `OMNI_PT_ACTY` / `ACTY` | Cracker token；裂化/聚合规则独立实现 | `src/simulation/elements/ACTY.cpp` |
| 366 | `OMNI_PT_CATA` / `CATA` | OmniPack 原创工艺条件材料 | `src/simulation/elements/CATA.cpp` |
| 367 | `OMNI_PT_POLY` / `POLY` | OmniPack 原创聚合物 | `src/simulation/elements/POLY.cpp` |
| 368 | `OMNI_PT_PERO` / `PERO` | OmniPack 原创电化学近似 | `src/simulation/elements/PERO.cpp` |
| 369 | `OMNI_PT_FERT` / `FERT` | OmniPack 原创植物支持循环 | `src/simulation/elements/FERT.cpp` |

来源限制与裁决：

- Seppo 的自定义构造器仍缺失，`ETHL/KERO/GASO` 只能作为公开 token/需求参考；
- Cracker `CHLR` 使用 5×5 邻域并混合多个概率反应，本项目只保留“氯气与氢气在受控温度下生成酸”的玩法目标，改为确定的 3×3 规则；
- Cyens 的 `Hydrocarbon.cpp` 改写官方 `GAS/OIL/MWAX/WAX` 且离子体系未完成，本项目没有采用这些状态机或粒子字段；
- 反应配方集中在 `OmniChemistry.cpp`，成功反应受每帧 1,536 次预算限制，使用 `tmp3` 防止同帧重复输入；
- 化学 OPS 往返、禁用模块载入提示与压力样本仍未运行，不能据此声称完整存档兼容或大型生产线性能通过。

### 当前测试证据（2026-07-30）

| 测试 | 结果 | 可核对证据 |
|---|---|---|
| 化学静态门禁 | PASS | `py tools/chemistry_audit.py`：10 元素、7 类受限工艺、3×3 有界 |
| 化学审计单元测试 | PASS，2/2 | 正常仓库通过；删除反应预算常量时门禁拒绝 |
| 全部 Python 工具测试 | PASS，32/32 | 两项生成 C++ 语法验证因该 Python 环境未发现编译器而跳过 |
| Meson `static` suite | PASS，5/5 | registry、i18n、冶金、化学和工具测试 |
| Windows x64 增量编译 | PASS | GCC 16.1.0、Ninja；0 error |
| Lua 真实客户端回归 | PASS | `OMNI_CHEMISTRY_IDS=360-369`、`PATHS=9`；客户端保持响应 |
| 回归路径 | PASS | 原油/煤油裂化、聚合、过氧化物制备与分解、氨合成、氯氢反应、肥料、中温发酵及冷催化剂负例 |

### 本项目实现文件

- `src/simulation/OmniMetallurgy.cpp` 与 `.h`：集中反应、3×3 局部收集、每帧最多 2,048 次反应、同帧级联保护、压力碎料与回炉恢复；
- `src/simulation/elements/FIRE.cpp`：在官方熔融元素更新中接入合金/炼钢反应；
- `src/simulation/elements/WOOD.cpp`、`COAL.cpp`：接入坩埚旁缺氧保温的木炭/焦炭生产；
- `src/simulation/elements/SPRK.cpp`：接入 `NCRM` 电阻发热；
- `src/simulation/elements/BASE.cpp`：碱液腐蚀自定义金属时改生成 `MSCR` 并保留原 `ctype`；
- `docs/ELEMENT_REGISTRY.csv`：23 项稳定 ID、双语说明、来源 commit 与兼容状态；
- `src/lang/en-US.json`、`zh-CN.json`：23 项名称和短说明；
- `tools/metallurgy_audit.py`、`tools/tests/test_metallurgy_audit.py`：登记、配方、边界和性能门禁；
- `tools/runtime/metallurgy_regression.lua`、`tools/runtime_lua_metallurgy_test.ps1`：真实客户端运行回归。

### 当前测试证据（2026-07-29）

| 测试 | 结果 | 可核对证据 |
|---|---|---|
| 冶金静态门禁 | PASS | `py tools/metallurgy_audit.py`：23 元素、6 合金配方、1 炼钢配方、3×3 有界 |
| 冶金审计单元测试 | PASS，2/2 | 正常仓库通过；把青铜比例从 3:1 改错时门禁会拒绝 |
| 全部 Python 工具测试 | PASS，30/30 | Meson 测试记录；新增 2 项冶金审计测试 |
| Meson `static` suite | PASS，4/4 | 包含 registry、i18n、冶金和工具测试 |
| Lua 真实客户端回归 | PASS | `OMNI_METALLURGY_IDS=256-278`、`RECIPES=7`、`BEHAVIORS=5` |
| 七组配方运行帧数 | PASS | `BRNZ/BRAS/NCRM/ALMG/SSIL/TSTL/STEL` 均在第 1 帧完成并验证冷却固化 |
| 额外运行行为 | PASS | 炭化、焦化、冷固体不合金、镍铬发热、压损差异、碎料回炉、镁燃烧、锌牺牲保护 |

## Phase 4 局部生态来源与实现复核

Phase 4 首批的提交包含 `src/simulation/OmniBiology.cpp`/`.h`、八个构造器、既有生物模块门禁、本地化、登记、审计和真实客户端 Lua 回归。SpikeViper 快照仅用于确认“氧气、营养、感染联动”的玩法方向；本项目没有复制该来源的元素更新函数，也没有改动官方 `PLNT`、`VIRS`、`WATR` 或 `LIFE` 状态机。

| 稳定 ID | identifier / 代号 | 来源方式 | 本项目构造文件 |
|---:|---|---|---|
| 288 | `OMNI_PT_NUTR` / `NUTR` | OmniPack 原创局部营养输入 | `src/simulation/elements/NUTR.cpp` |
| 289 | `OMNI_PT_ALGA` / `ALGA` | Spike 生态联动需求参考；光合规则独立实现 | `src/simulation/elements/ALGA.cpp` |
| 290 | `OMNI_PT_MYCL` / `MYCL` | OmniPack 原创局部分解者 | `src/simulation/elements/MYCL.cpp` |
| 291 | `OMNI_PT_SPOR` / `SPOR` | OmniPack 原创局部萌发输入 | `src/simulation/elements/SPOR.cpp` |
| 292 | `OMNI_PT_PATH` / `PATH` | Spike 感染联动需求参考；规则独立实现 | `src/simulation/elements/PATH.cpp` |
| 293 | `OMNI_PT_STER` / `STER` | OmniPack 原创局部处理材料 | `src/simulation/elements/STER.cpp` |
| 294 | `OMNI_PT_HUMS` / `HUMS` | OmniPack 原创稳定生物副产物 | `src/simulation/elements/HUMS.cpp` |
| 295 | `OMNI_PT_BIOF` / `BIOF` | OmniPack 原创局部过滤介质 | `src/simulation/elements/BIOF.cpp` |

来源限制与裁决：

- **第三方更新函数逐行复制：0。** SpikeViper 的生物循环没有进入本项目源码；只保留氧气、营养、感染之间存在可控联动的公开玩法目标。
- **官方状态机改写：0。** `PLNT`、`VIRS`、`WATR`、`LIFE` 保持官方更新函数；新元素仅把这些官方元素作为局部输入或输出。
- **性能限制：**所有成功事件共享 1,024 次/帧预算，局部搜寻固定为 `3x3`，`tmp3` 防止同帧级联；简化模式不增加粒子数。
- **未覆盖范围：**生态存档往返、关闭模块后的载入提示、视觉设置交互以及高粒子数压力样本仍未运行。

### 当前测试证据（2026-07-30）

| 测试 | 结果 | 可核对证据 |
|---|---|---|
| 生物静态门禁 | PASS | `py tools/biology_audit.py`：8 元素、6 条局部路径、3×3 有界 |
| 生物审计单元测试 | PASS，3/3 | 正常仓库通过；删除事件预算或禁用简化模式时门禁拒绝 |
| Windows x64 增量编译 | PASS | GCC 16.1.0、Ninja；0 error |
| Lua 真实客户端回归 | PASS | 完整与简化模式均为 `PATHS=6`、`IDS=288-295`；客户端保持响应 |
| 回归路径 | PASS | 藻类光合与冷温负例、菌丝分解、孢子萌发、感染、消毒和生物膜过滤 |

## Phase 5 受控核工业来源与实现复核

Phase 5 首批包含 `src/simulation/OmniNuclear.cpp`/`.h`、七个构造器、`SPRK` 的窄触发钩子、本地化、登记、静态审计和真实客户端 Lua 回归。Spike、Ultimata 与 Cracker 的固定快照只用于界定可玩反应器需要具备控制、冷却、屏蔽和性能限制；本项目没有复制任何第三方核更新函数，也没有修改官方 `URAN`、`PLUT`、`NEUT` 或 `DEUT` 的状态机。

| 稳定 ID | identifier / 代号 | 来源方式 | 本项目构造文件 |
|---:|---|---|---|
| 328 | `OMNI_PT_NFUL` / `NFUL` | OmniPack 原创受控燃料规则 | `src/simulation/elements/NFUL.cpp` |
| 329 | `OMNI_PT_MODR` / `MODR` | OmniPack 原创局部慢化条件 | `src/simulation/elements/MODR.cpp` |
| 330 | `OMNI_PT_CROD` / `CROD` | OmniPack 原创局部控制条件 | `src/simulation/elements/CROD.cpp` |
| 331 | `OMNI_PT_NCLT` / `NCLT` | OmniPack 原创冷却到官方蒸汽的路径 | `src/simulation/elements/NCLT.cpp` |
| 332 | `OMNI_PT_NWST` / `NWST` | OmniPack 原创受控反应副产物 | `src/simulation/elements/NWST.cpp` |
| 333 | `OMNI_PT_NGEN` / `NGEN` | OmniPack 原创的有燃料前提中子源 | `src/simulation/elements/NGEN.cpp` |
| 334 | `OMNI_PT_RSHD` / `RSHD` | OmniPack 原创局部中子汇 | `src/simulation/elements/RSHD.cpp` |

来源限制与裁决：

- **第三方核更新函数逐行复制：0。** 所有反应器路径由本项目根据当前 TPT API 独立编写；没有从二进制、论坛描述或第三方存档反推实现。
- **官方核状态机改写：0。** 新模块只在 `SPRK(NGEN)` 时创建一粒官方 `NEUT`，并在局部路径中消费已经存在的官方 `NEUT`；不修改 `URAN`、`PLUT`、`NEUT` 或 `DEUT` 的源码。
- **性能限制：**所有成功路径只读取固定 `3x3` 邻域，使用每帧 512 次共享预算和 `tmp3` 同帧级联保护。中子发生器必须先确认局部 `NFUL` 与空槽。
- **未覆盖范围：**核工业 OPS 往返、关闭模块后的载入提示、视觉设置交互和高粒子数反应堆压力样本仍未运行。

### 当前测试证据（2026-07-30）

| 测试 | 结果 | 可核对证据 |
|---|---|---|
| 核工业静态门禁 | PASS | `py tools/nuclear_audit.py`：7 元素、4 类受限路径、`3x3` 有界 |
| 核工业审计单元测试 | PASS，4/4 | 正常仓库通过；删除事件预算、发生器燃料前提或单次发射门禁时均被拒绝 |
| Windows x64 增量编译 | PASS | GCC 16.1.0、Ninja；0 error |
| Lua 真实客户端回归 | PASS | `PATHS=4`、`IDS=328-334`；客户端保持响应 |
| 回归路径 | PASS | 有慢化受控转换、控制棒抑制、无燃料发生器负例、冷却剂排热与屏蔽吸收 |

## 每次实际移植必须补记

当状态进入 `PORTING`，新增一条详细记录，至少包含：

- 原仓库 URL、分支、commit、原文件路径；
- 原作者及文件内版权；
- 目标文件路径与提交；
- 是逐行改编、算法重写还是仅依据公开行为清洁室实现；
- ID、identifier、存档迁移；
- 本地化键和图鉴条目；
- 单元/集成/运行/压力测试；
- 性能预算与已知偏差。

任何 `ACCEPTED` 项都必须能从本账本追到固定来源和测试证据。
