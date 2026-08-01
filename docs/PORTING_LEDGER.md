# 移植账本

> Phase 0 未复制第三方实现代码。当前工作树保留 Cracker `COPR` 的参数/行为适配，并把 Cyens 的 `ACET/UREA` 两个 GPL 材料概念按现有架构重写；第三方元素更新函数逐行复制仍为 0。下文的“没有逐行复制”不等于没有第三方来源，所有适配与重写都必须保留仓库、commit、文件和作者追踪。

状态值：`AUDITED`、`DESIGN`、`PORTING`、`TESTING`、`ACCEPTED`、`REJECTED`、`BLOCKED`。

| 模块/候选 | 来源与固定 commit | 原作者/归属 | Phase 0 判定 | 目标方式 | 当前状态 | 理由与门禁 |
|---|---|---|---|---|---|---|
| 官方模拟、保存、ID | Official `bff38ce...` | TPT contributors | 直接底座 | 保留并同步 | AUDITED | 100.0.399 权威实现 |
| 中文/英文语料与严格加载 | Dragonrster `445fab51...`；OmniPack `457233ac...` | Dragonrster、TPT contributors、OmniPack contributors | 审核后重放并重写加载器 | JsonCpp 严格 JSON + 自动审计 | ACCEPTED | en/zh 1,252/1,252，0 阻塞错误；视觉复核仍独立跟踪 |
| 官方元素稳定锁 | Official `bff38ce...`；OmniPack `08fe8a82...` | TPT contributors、OmniPack contributors | 源码解析生成 | 逐槽固定 0–195 | ACCEPTED | 196 槽、195 活动、1 tombstone；自动门禁 PASS |
| 模块选择与图鉴框架 | OmniPack `d7312a9b...`、`f28cdcb7...` | OmniPack contributors | 项目原创实现 | 统一门禁、编译时登记目录 | ACCEPTED | clean build、28 单测、Lua 运行回归和枚举本地化门禁 PASS |
| Phase 3 工业冶金首批 | Seppo `c3a8dd17...`；Cracker COPR `eb474d38...`；OmniPack 当前实现 | SeppoTPT、Cracker contributors、OmniPack contributors | 概念筛选、参数/行为适配、独立更新实现 | 稳定 ID 256–278 + 集中反应引擎 | TESTING | 23 元素、7 配方、5 材料行为；静态门禁与 Lua 运行回归 PASS |
| Phase 3 基础化学首批 | Seppo `c3a8dd17...`；Cracker `ebbb9aab...`；Cyens `f01d992c...`；OmniPack 当前实现 | SeppoTPT、Cracker contributors、cbeimers113、OmniPack contributors | token/玩法需求参考，集中算法独立实现 | 稳定 ID 360–369 + 中央有界反应引擎 | TESTING | 10 元素、11 条反应路径、化学静态门禁与 Lua 真实运行回归 PASS |
| 有机化学首批 | Cyens Source `1b74504e...`；OmniPack 当前实现 | DaveYognaught / cbeimers113、OmniPack contributors | `ACET/UREA` 为 GPL 概念重写，其余元素独立实现 | `CH4M..FATS=589..601` + `POLY=367` 原位升级 + `OmniOrganics` | TESTING | 13 新元素、15 反应、5 相变、1536 峰值、模块与化学 OPS 运行回归 PASS |
| Phase 5 受控核工业首批 | OmniPack 当前实现；Spike/Ultimata/Cracker 固定快照仅作范围参考 | OmniPack contributors；对应来源作者 | 玩法和风险边界参考，集中算法独立实现 | 稳定 ID 328–334 + 中央有界反应器引擎 | TESTING | 7 元素、4 类受控路径；静态门禁与 Lua 真实运行回归 PASS |
| Phase 3 完整周期表十四批 | OmniPack `21b160a5...`、`bcf5accf...`、`546b8791...`、`5d9b9947...`、`de91bbb6...`、`2eeab9f3...`、`3c3c623a...`、`8066533a...`、`6c64c0a0...`、`b26112f9...`、`248c516a...`、`7c49278b...`、`54411e08...`、`518dd9a4...` 基线；候选目录只核对搜索覆盖 | OmniPack contributors | 原创族逻辑；未复制外部候选实现 | 前八个主族批次、三条过渡系、镧系、锕系及超重系共 92 个新固定 ID + 共享周期引擎 | TESTING | 92 新元素、118/118 映射；行为、预算、118 元素同图 OPS 与直接选择回归 PASS |
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

周期表碱金属批次复用官方 `LITH=191` 与 `RBDM=41`，没有复制或覆盖其更新函数；`NA=376`、`K=380`、`CS=406`、`FR=432` 的构造器和 `OmniPeriodic.cpp` 共享逻辑均为本项目原创实现。外部 `fun_chemicals` 只作为候选搜索命中保留在来源映射中，不是代码或常量来源；周期表批次本身的第三方移植/重写数为 0。项目累计 `elements_rewritten=2` 只来自后续有机批的 `ACET/UREA`。

周期表碱土金属批次复用既有 `MAGN=261`，在保留其冶金、合金、碎料与旧 ID 行为的同时增加共享水/酸入口及白色焰色；`BE=371`、`CA=381`、`SR=391`、`BA=407`、`RA=433` 均为本项目原创族实现，没有复制候选模组构造器或更新函数。

周期表硼族批次复用既有 `ALUM=256` 并保留其冶金、合金、碎料与旧 ID 行为；`B=372`、`GA=385`、`IN=401`、`TL=428`、`NH=456` 的构造器和共享逻辑均为本项目原创实现。硼中子俘获、镓脆铝、两性反应代理和鿭压缩衰变没有复制候选模组代码或常量表。

周期表碳族批次复用官方 `DMND=28`、`SLCN=187` 与既有 `TIN=259`、`LEAD=258`，保持原稳定 ID、钻石不可破坏语义、硅电气行为以及锡/铅冶金路径；`GE=386`、`FL=457` 的构造器和共享逻辑为本项目原创实现。锗有限放电、锡瘟、分级酸/氧/汽化路线和𫓧压缩衰变没有复制候选模组代码或常量表。

周期表氮族批次新增 `N=373`、`P=377`、`AS=387`、`SB=402`、`BI=429`、`MC=458`，构造器与共享逻辑均为本项目原创实现；仅把官方 `LNTG=37`、`NICE=51` 的既有低温相安全连接到常温氮。氮放电、磷燃烧、分级酸/氧/汽化和 `MC → NH → POLO` 压缩衰变没有复制候选模组代码或常量表；`biological_mod` 只保留为氮候选搜索命中。

周期表氧族批次复用官方 `O2=61`、`LO2=60` 和 `POLO=182`，不覆盖其更新函数、相变或稳定 ID；`S=378`、`SE=388`、`TE=403`、`LV=459` 的构造器和共享逻辑均为本项目原创实现。外部 `chem_mod_lua` 只作为硫候选搜索命中保留在来源映射中；硫烟雾燃烧、硒光敏辉光、硒/碲氧化与汽化及 `LV → FL → POLO` 压缩衰变没有复制候选模组代码或常量表。

周期表卤素批次复用既有 `CHLR=360`，保留其稳定 ID、模块归属和 11 条高级化学路径；`F=374`、`BR=389`、`I=404`、`AT=430`、`TS=460` 的构造器与共享逻辑均为本项目原创实现。氟遇水/氢代理、分级卤化/消毒、有限着色蒸气和 `AT → POLO`、`TS → MC → NH → POLO` 压缩衰变没有复制候选模组代码或常量表。

周期表第一过渡系复用官方 `TTAN=144`、`IRON=76` 和既有 `CHRM=262`、`COBT=263`、`NICL=260`、`COPR=257`、`ZINC=265`，不覆盖其稳定 ID 或原更新状态机；`SC=382`、`V=383`、`MN=384` 的构造器及共享逻辑均为本项目原创实现。钪放电灯、钒工具钢、锰钢液脱氧和三者分级酸蚀/氧化/汽化没有复制候选模组代码或常量表。

周期表第二过渡系复用既有 `MOLY=264`，不覆盖其稳定 ID 或冶金更新状态机；`Y=392`、`ZR=393`、`NB=394`、`TC=395`、`RU=396`、`RH=397`、`PD=398`、`AG=399`、`CD=400` 的构造器和共享反应均由本项目独立实现。来源目录中的 `chem_mod_lua` 银候选只保留搜索命中与去重线索，未复制其代码或常量；钇放电、锆蒸汽氧化、锝衰变、铂族催化、钯储氢、银硫化和分级通用化学均使用当前本地预算框架重新设计。

周期表第三过渡系复用官方 `TUNG=171`、`PTNM=188`、`GOLD=170`、`MERC=152`，不覆盖其稳定 ID 或更新函数；`HF=423`、`TA=424`、`RE=425`、`OS=426`、`IR=427` 的构造器和共享反应均由本项目独立实现。铪中子俘获、钽钝化、铼镍高温合金代理、锇有毒氧化物代理、铱过氧化物催化和分级通用化学没有复制候选模组代码、参数表或更新函数；外部目录仅用于来源搜索和重复性核对。

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
- 本项目把氧气、盐水和水造成的腐蚀累积到 `tmp`，最终生成携带 `ctype=COPR` 与专用 `tmp4` 标记的官方 `BRMT`，支持回炉恢复；这是新的兼容/回收设计；
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
| 278 | `OMNI_PT_MSCR` / `MSCR` | OmniPack 原创旧碎料的隐藏兼容别名；迁移到官方 `BRMT=30`，ID 永不复用 | `src/simulation/elements/MSCR.cpp`、`src/simulation/elements/BRMT.cpp` |

### 官方 BRMT 合并与来源边界

这次合并属于 OmniPack 自身重复内容整理，不是从外部模组复制代码。官方 `BRMT.cpp` 继续使用上游 GPL-3.0 实现及稳定 ID 30；本项目只增加带专用 `tmp4` 标记的来源金属回收分支，并把既有 OmniPack `MSCR` 生产路径改为该 canonical 类型。普通官方 BRMT 不带标记，因此仍按上游 1273 K 相变和 `BRMT+BREC` 逻辑运行。

`MSCR=278` 继续由原文件构造，以便旧 OPS、`CONV/CLNE` 等间接载体和 identifier palette 安全解析；它不出现在菜单和搜索中，也不能由 Lua 直接创建。三进程回归已证明旧粒子迁移后新 OPS 只保存 `DEFAULT_PT_BRMT=30`，同时保留 `ctype/tmp4`。该别名不计入可玩材料或第三方移植数量，稳定 ID 278 永不复用。

## 化学核心与无机三批来源和实现复核

化学核心和无机三批包含 `src/simulation/OmniChemistry.cpp`/`.h`、60 个构造器、模块选择门禁、本地化、登记、审计和真实客户端 Lua 回归。所有反应实现由本项目按 TPT 100.0 API 独立编写；没有复制 Cracker、Seppo、Cyens 或其他第三方模组的元素更新函数。

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
| 462 | `OMNI_PT_HCLA` / `HCLA` | OmniPack 原创精确盐酸与通用酸网络 | `src/simulation/elements/HCLA.cpp` |
| 463 | `OMNI_PT_SULA` / `SULA` | OmniPack 原创硫酸/硫氧化物循环 | `src/simulation/elements/SULA.cpp` |
| 464 | `OMNI_PT_NITA` / `NITA` | OmniPack 原创硝酸/氮氧化物循环 | `src/simulation/elements/NITA.cpp` |
| 465 | `OMNI_PT_PHOA` / `PHOA` | OmniPack 原创磷酸肥料路径 | `src/simulation/elements/PHOA.cpp` |
| 466 | `OMNI_PT_HYFA` / `HYFA` | OmniPack 原创有界硅质腐蚀代理 | `src/simulation/elements/HYFA.cpp` |
| 467 | `OMNI_PT_NAOH` / `NAOH` | OmniPack 原创强碱/碳酸化规则 | `src/simulation/elements/NAOH.cpp` |
| 468 | `OMNI_PT_KOH` / `KOH` | OmniPack 原创强碱/硝酸钾规则 | `src/simulation/elements/KOH.cpp` |
| 469 | `OMNI_PT_CAOH` / `CAOH` | OmniPack 原创石灰循环 | `src/simulation/elements/CAOH.cpp` |
| 470 | `OMNI_PT_KNIT` / `KNIT` | OmniPack 原创有界氧化剂玩法 | `src/simulation/elements/KNIT.cpp` |
| 471 | `OMNI_PT_CUSF` / `CUSF` | OmniPack 原创铜置换与硫酸循环 | `src/simulation/elements/CUSF.cpp` |
| 472 | `OMNI_PT_CACO` / `CACO` | OmniPack 原创碳酸盐/煅烧规则 | `src/simulation/elements/CACO.cpp` |
| 473 | `OMNI_PT_NABC` / `NABC` | OmniPack 原创碳酸氢盐分解规则 | `src/simulation/elements/NABC.cpp` |
| 474 | `OMNI_PT_COMO` / `COMO` | OmniPack 原创有界一氧化碳氧化 | `src/simulation/elements/COMO.cpp` |
| 475 | `OMNI_PT_SODI` / `SODI` | OmniPack 原创二氧化硫回收路径 | `src/simulation/elements/SODI.cpp` |
| 476 | `OMNI_PT_NODI` / `NODI` | OmniPack 原创二氧化氮回收路径 | `src/simulation/elements/NODI.cpp` |
| 477 | `OMNI_PT_CAOX` / `CAOX` | OmniPack 原创氧化钙水合规则 | `src/simulation/elements/CAOX.cpp` |
| 478 | `OMNI_PT_CARA` / `CARA` | OmniPack 原创碳酸可逆水合规则 | `src/simulation/elements/CARA.cpp` |
| 479 | `OMNI_PT_H2SG` / `H2SG` | OmniPack 原创硫化氢有界合成与氧化规则 | `src/simulation/elements/H2SG.cpp` |
| 480 | `OMNI_PT_AMWA` / `AMWA` | OmniPack 原创氨水溶解与释放规则 | `src/simulation/elements/AMWA.cpp` |
| 481 | `OMNI_PT_BAOH` / `BAOH` | OmniPack 原创重质强碱规则 | `src/simulation/elements/BAOH.cpp` |
| 482 | `OMNI_PT_KCL` / `KCL` | OmniPack 原创精确氯化物与溶解规则 | `src/simulation/elements/KCL.cpp` |
| 483 | `OMNI_PT_CACL` / `CACL` | OmniPack 原创放热氯化物溶解规则 | `src/simulation/elements/CACL.cpp` |
| 484 | `OMNI_PT_FECL` / `FECL` | OmniPack 原创氯化铁腐蚀与水解循环 | `src/simulation/elements/FECL.cpp` |
| 485 | `OMNI_PT_NASF` / `NASF` | OmniPack 原创精确硫酸盐中和产物 | `src/simulation/elements/NASF.cpp` |
| 486 | `OMNI_PT_AMNT` / `AMNT` | OmniPack 原创受预算热分解代理 | `src/simulation/elements/AMNT.cpp` |
| 487 | `OMNI_PT_NACO` / `NACO` | OmniPack 原创碳酸盐捕集与酸解规则 | `src/simulation/elements/NACO.cpp` |
| 488 | `OMNI_PT_KPER` / `KPER` | OmniPack 原创有界强氧化剂代理且不含现实制造指导 | `src/simulation/elements/KPER.cpp` |
| 489 | `OMNI_PT_ALOX` / `ALOX` | OmniPack 原创耐火氧化物与过氧化物路径 | `src/simulation/elements/ALOX.cpp` |
| 490 | `OMNI_PT_MGOX` / `MGOX` | OmniPack 原创耐火碱性氧化物路径 | `src/simulation/elements/MGOX.cpp` |
| 491 | `OMNI_PT_FEOX` / `FEOX` | OmniPack 原创分级铁氧化物还原规则 | `src/simulation/elements/FEOX.cpp` |
| 492 | `OMNI_PT_CUOX` / `CUOX` | OmniPack 原创铜氧化物还原与硫酸盐规则 | `src/simulation/elements/CUOX.cpp` |
| 493 | `OMNI_PT_ZNOX` / `ZNOX` | OmniPack 原创高阈值锌氧化物还原规则 | `src/simulation/elements/ZNOX.cpp` |
| 494 | `OMNI_PT_SUTR` / `SUTR` | OmniPack 原创硫氧化物与硫酸循环 | `src/simulation/elements/SUTR.cpp` |
| 495 | `OMNI_PT_NIMO` / `NIMO` | OmniPack 原创有界氮氧合成与氧化循环 | `src/simulation/elements/NIMO.cpp` |
| 496 | `OMNI_PT_TIOX` / `TIOX` | OmniPack 原创耐火钛氧化物路径 | `src/simulation/elements/TIOX.cpp` |
| 497 | `OMNI_PT_UROX` / `UROX` | OmniPack 原创放射性燃料陶瓷代理与还原规则 | `src/simulation/elements/UROX.cpp` |
| 498 | `OMNI_PT_CAPH` / `CAPH` | OmniPack 原创磷酸盐与湿植物肥料联动 | `src/simulation/elements/CAPH.cpp` |
| 499 | `OMNI_PT_FESF` / `FESF` | OmniPack 原创硫化物合成和遇酸放气循环 | `src/simulation/elements/FESF.cpp` |
| 500 | `OMNI_PT_NASD` / `NASD` | OmniPack 原创碱金属硫化物路径 | `src/simulation/elements/NASD.cpp` |
| 501 | `OMNI_PT_HYCN` / `HYCN` | OmniPack 原创仅直接放置有毒气体代理与有界清理 | `src/simulation/elements/HYCN.cpp` |
| 502 | `OMNI_PT_CACB` / `CACB` | OmniPack 原创游戏化碳化物与乙炔路径 | `src/simulation/elements/CACB.cpp` |
| 503 | `OMNI_PT_SICB` / `SICB` | OmniPack 原创耐火碳化物合成与氧化 | `src/simulation/elements/SICB.cpp` |
| 504 | `OMNI_PT_BORN` / `BORN` | OmniPack 原创绝缘中子吸收氮化物 | `src/simulation/elements/BORN.cpp` |
| 505 | `OMNI_PT_SINT` / `SINT` | OmniPack 原创高强绝缘氮化物 | `src/simulation/elements/SINT.cpp` |
| 506 | `OMNI_PT_NAHY` / `NAHY` | OmniPack 原创加压氢化物与水解规则 | `src/simulation/elements/NAHY.cpp` |
| 507 | `OMNI_PT_CAHY` / `CAHY` | OmniPack 原创碱土氢化物与水解规则 | `src/simulation/elements/CAHY.cpp` |
| 508 | `OMNI_PT_ALCL` / `ALCL` | OmniPack 原创精确氯化铝与水解循环 | `src/simulation/elements/ALCL.cpp` |
| 509 | `OMNI_PT_MGCL` / `MGCL` | OmniPack 原创精确氯化镁与水解循环 | `src/simulation/elements/MGCL.cpp` |
| 510 | `OMNI_PT_CUCL` / `CUCL` | OmniPack 原创精确氯化铜与铁置换循环 | `src/simulation/elements/CUCL.cpp` |
| 511 | `OMNI_PT_AMCL` / `AMCL` | OmniPack 原创精确氯化铵与有界热释放 | `src/simulation/elements/AMCL.cpp` |

来源限制与裁决：

- Seppo 的自定义构造器仍缺失，`ETHL/KERO/GASO` 只能作为公开 token/需求参考；
- Cracker `CHLR` 使用 5×5 邻域并混合多个概率反应，本项目只保留“氯气与氢气在受控温度下生成酸”的玩法目标，改为确定的 3×3 规则；
- Cyens 的 `Hydrocarbon.cpp` 改写官方 `GAS/OIL/MWAX/WAX` 且离子体系未完成，本项目没有采用这些状态机或粒子字段；
- 反应配方集中在 `OmniChemistry.cpp`，成功反应受每帧 1,536 次预算限制，使用 `tmp3` 防止同帧重复输入；
- 无机三批没有选取可直接移植的第三方源码；元素名称和常见化学概念不构成第三方代码来源，构造参数、反应表、预算、测试和图鉴均由本项目独立设计；
- 官方 `DEFAULT_PT_SALT=26` 经行为与 identifier 审计后仅映射为氯化钠，不复制、不改号，也不新增重复 `NaCl` 元素；
- 化学 OPS、模块直选和 1,800 粒子预算帧已经实测，但正式 600 秒压力采样、GUI 视觉和两小时长跑仍为 `not_tested`。

## 有机化学首批来源与重写记录

固定来源为 `https://github.com/cbeimers113/cyens-toy-src.git`，分支 `master`，commit `1b74504e4642cd967c0079499b57faa7f37d9668`（提交作者 DaveYognaught）。根 `LICENSE` 是 GPL-3.0，SHA-256 为 `0B383D5A63DA644F628D99C33976EA6487ED89AAA59F0B3257992DEAC1171E6B`。只读源码确认旧 `ElementNumbers.h` 使用 `ACET=210`、`UREA=211`；对应文件 blob 分别为 `c17e871fde5ea8e769a48609ff25f616bbfa98ac` 与 `6f324d1fe80dcc759962e7fbfeedc839b79efb76`。

| 原始模组/文件 | 原始概念 | 当前 identifier / ID | 移植类型 | 主要改动 | 当前测试 |
|---|---|---|---|---|---|
| Cyens Source `src/simulation/elements/ACET.cpp` | `DEFAULT_PT_ACET=210`，无更新函数的简单液体 | `OMNI_PT_ACET=595` | `rewrite_existing` | 不复制旧属性表；按当前化学网络重做挥发、毒性、乙酸酮化、双语图鉴、模块和预算 | `organic_audit.py`；`organic_regression.lua`；化学 OPS |
| Cyens Source `src/simulation/elements/UREA.cpp` | `DEFAULT_PT_UREA=211`，旧硝酸邻接会转 `UNTR` | `OMNI_PT_UREA=600` | `rewrite_existing` | 明确不采用旧爆炸物路径；重做加压氨/二氧化碳合成、水解、热分解、粉末物态、双语图鉴、模块和预算 | `organic_audit.py`；`organic_regression.lua`；禁用模块 OPS |

`CH4M/ETHA/PROP/BUTA/ETHE/METH/BENZ/TOLU/GLYC/ACTA/FATS` 与共享 `OmniOrganics.cpp` 为本项目实现，不计入第三方重写数。`POLY=367` 只在原 identifier 和 ID 上改为聚乙烯并切换到乙烯聚合，不新增重复元素。官方 `OIL/GAS/DESL` 构造器与状态机保持；本批没有整库合并 Cyens 的保存、网络、协议、UI、Lua 或模拟核心改动。

机器可读来源统计因此为：第三方代码直接移植 `elements_ported=0`，有来源概念和文件追踪的重写 `elements_rewritten=2`，第三方更新函数逐行复制 `0`。这两个重写不满足用户要求的 50 元素“第一轮移植”数量，所以 `first_port_batch_complete=false` 继续保持。

有机首批最终证据绑定 `build-organic-batch1-final2-clean`：从零构建 `712/712`、Meson static `27/27`、Python `184/184`，真实客户端有机专项、旧化学及全部既有内容回归通过；化学 OPS 覆盖 79 粒子/87 字段断言，六类合计 300 粒子/352 字段断言。开发 EXE SHA-256 为 `499F30834C8C3655AE14506C6BA936BB23BE7A3BE94699E38BC9BCC789CEFB25`。正式 600 秒压力、长跑和人工 GUI 仍为 `not_tested`。

### 当前测试证据（2026-08-01）

| 测试 | 结果 | 可核对证据 |
|---|---|---|
| 化学静态门禁 | PASS | `chemistry_audit.py`：60 元素、55 个源码门禁标记、3×3 有界；第三批化合物公式/ID/identifier 校验通过 |
| 登记与内容门禁 | PASS | 512 槽、385 活动、127 保留；190 个 OmniPack 元素双语内容完整；190 条反应登记有效 |
| Meson `static` suite | PASS，21/21 | 最终源码树全量静态门禁通过 |
| 全部 Python 工具测试 | PASS | 162/162，0 skip；含模块、化合物、用法与登记变异测试 |
| Windows x64 clean 编译 | PASS，655/655 | `build-inorganic-batch3-final-clean`，GCC 16.1.0、Ninja；EXE SHA-256 `C89B942EB67A6A78C3E8B1E94CFAB52EBDA0CC7CD1E6932D06118D4ED338C58E` |
| Lua 真实客户端回归 | PASS | `PATHS=90`、`ELEMENTS=60`、`INORGANIC_ELEMENTS=50`、`BATCH3_IDS=494-511` |
| 预算帧 | PASS | 1,800 个碳酸钙样本，事件峰值恰为 `1536`，剩余反应延后 |
| 模块直选 | PASS | 化学模块启用时 `HCLA=462`、`CARA=478` 与 `AMCL=511` 均可直接选取 |
| 四模块禁用运行 | PASS | 两进程 OPS1；9 粒子保留，冶金/生物/化学/核工业更新事件为 0，周期 `HE` 仍可选取/创建 |
| 化学 OPS | PASS | 3 进程、2 重启、2 加载、66 粒子、74 字段断言和 66 个稳定/调色板 identifier |
| 回归路径 | PASS | 三批路径覆盖酸碱盐、氧化还原、硫/氮氧化物、碳化物/氮化物/氢化物、氯化物循环和九条共享催化合成 |

### 本项目实现文件

- `src/simulation/OmniMetallurgy.cpp` 与 `.h`：集中反应、3×3 局部收集、每帧最多 2,048 次反应、同帧级联保护、压力碎料与回炉恢复；
- `src/simulation/elements/FIRE.cpp`：在官方熔融元素更新中接入合金/炼钢反应；
- `src/simulation/elements/WOOD.cpp`、`COAL.cpp`：接入坩埚旁缺氧保温的木炭/焦炭生产；
- `src/simulation/elements/SPRK.cpp`：接入 `NCRM` 电阻发热；
- `src/simulation/elements/BASE.cpp`：碱液腐蚀自定义金属时生成带可回收标记的官方 `BRMT` 并保留原 `ctype`；
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
- **未覆盖范围：**生态 OPS 往返和四模块关闭后的更新暂停已运行；GUI 三选项、视觉设置交互以及高粒子数正式压力样本仍未运行。

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
- **未覆盖范围：**核工业 OPS 往返和四模块关闭后的更新暂停已运行；GUI 三选项、视觉设置交互和高粒子数反应堆正式压力样本仍未运行。

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
