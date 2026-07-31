# Phase 0 来源审计

审计日期：2026-07-29（Asia/Shanghai）

审计原则：固定公开源码 commit、只读检查、禁止目录覆盖、禁止从只有二进制的模组复制实现。Phase 0 未修改生产代码，也未把任何第三方实现加入主干。

## 审计基准与计数口径

- 优先主干：`Dragonrster/The-Powder-Toy-Chinese:i18n-new`
- 主干固定提交：`445fab51dcf66057e645371aa9c9a556425b3d2f`
- 精确官方共同基线：`606bbc98bd949e69345e8e51cf3d571083a2a91c`，标签 `v100.0.398b`
- 当前官方 master：`bff38ce6959e1c1a7a4d17d0d5d44d127a0dfcbd`
- 当前正式标签：`v100.0.399`，提交 `9c94feba3ed5eaa75a819ac000c0d29e4ce92570`
- “登记元素数”包含 `NONE` 擦除项；需要时另列实际玩家元素和菜单可见元素。
- 所有仓库顶层 `LICENSE` 均为 GPL-3.0；本次取得的文件 SHA-256 均为  
  `0B383D5A63DA644F628D99C33976EA6487ED89AAA59F0B3257992DEAC1171E6B`。

## 来源总表

| 来源 | 审计分支 / commit | 最后提交时间 | 源码版本 | 元素登记 | 核心裁决 |
|---|---|---|---|---:|---|
| [Dragonrster 中文分支](https://github.com/Dragonrster/The-Powder-Toy-Chinese) | `i18n-new` / `445fab51dcf66057e645371aa9c9a556425b3d2f` | 2026-06-04 14:25:47 +08:00 | 100.0.398 | 195，含 NONE | 翻译语料审核后重放；本地化核心与字体重写/替换 |
| [官方 TPT](https://github.com/The-Powder-Toy/The-Powder-Toy) | `master` / `bff38ce6959e1c1a7a4d17d0d5d44d127a0dfcbd` | 2026-07-29 07:07:40 +02:00 | 100.0.399 | 195，含 NONE | 模拟、存档、ID 和安全修复的权威底座 |
| [Cracker1000](https://github.com/cracker1000/The-Powder-Toy) | `master` / `ebbb9aab6aef27d26517682cebbc0a07147a843a` | 2023-09-27 21:31:36 +05:30 | 97.0.352 | 243，含 NONE；51 新增 | 仅逐项重写候选；原 ID 和高开销实现不可用 |
| [SpikeViper Biology](https://github.com/SpikeViper/The-Powder-Toy) | `master` / `134ebf330eda42b4b300a2b7613ede71261697df` | 2023-11-26 20:38:00 -06:00 | 97.0.352 | 222，含 NONE；30 新增 | 生物循环概念按事件/预算模型重写 |
| [TPT Ultimata](https://github.com/Bowserinator/TPT-Ultimata-Mod) | `development` / `b74971752433652c033559abea415ec3510ac433` | 2023-10-08 16:42:48 -04:00 | 97.0.352 | 334，含 NONE；142 新增 | 只精选有联动内容，按当前核心重写 |
| [Jacob1 Mod](https://github.com/jacob1/The-Powder-Toy) | `c++` / `b492616124d2346a5ec7bc70fa881fb73345396b` | 2026-06-29 18:03:41 -04:00 | 宏为 100.0.399；架构源于 83 | 206 槽；10 新增 | 保存标记和 UX 仅参考；少量自动化元素重写 |
| [TPT-Alchemy](https://github.com/jacob1/TPT-Alchemy) | `master` / `9a593ce11536e2e683bc64a698399c0805ce77a1` | 2016-08-17 14:54:42 +03:00 | 91.3.328 | 180 槽；无新增 | 进度、发现、成就和限制系统全部拒绝 |
| [Seppo Metallurgy](https://github.com/SeppoTPT/Seppo-s-Metallurgy-Mod-SRC) | `master` / `c3a8dd171a1c0fefc9a386e7e069f81d91f1514f` | 2020-05-28 23:41:02 -04:00 | 95.0.345 | 219，含 NONE；32 新增 | 32 个实现文件全缺失，只能清洁室设计参考 |
| [Cyens Toy](https://github.com/cbeimers113/cyens-toy) | `master` / `f01d992c97432ec1c46d84ade05131da521f355a` | 2022-07-21 14:22:31 -04:00 | 96.2.350 | 194，含 NONE；2 新增 | 烃链/可压缩气体思路重写；不覆盖官方元素语义 |

## 1. Dragonrster/The-Powder-Toy-Chinese

### 内容与代码差距

汉化分支与当前官方 master 的共同祖先是官方 `v100.0.398b`。从共同祖先计算：

- 汉化独有 27 个提交；
- 官方独有 46 个提交；
- 汉化差异：291 files changed，`+12292/-1224`；
- 官方后续差异：48 files changed，`+713/-259`。

元素注册表与其官方 398b 基线字节级一致：

- 195 个构造器/标识符，含 `NONE`；
- 194 个实际玩法元素；
- 175 个菜单可见元素；
- ID `146` 是官方保留空位；
- 最高已用 ID 为 `SEED=195`；
- `PT_NUM=512`。

### 适合保留/重放

- `en-US.json`、`zh-CN.json` 的现有翻译语料；
- 元素说明、墙、菜单、工具和 GOL 的键命名及调用点清单；
- 将显示文本与 GOL 规则 ID 分离的设计。

### 必须修复或重写

- 初次启动用 `prefs.Get("Language", 0)`，默认是英文；
- 当前 Options 页面已丢失语言下拉框，只剩 Model/Controller 的未使用接口；
- `BASE.cpp` 硬编码中文说明，两个语言文件均缺少它的说明键；
- 至少 `Save from newer version`、`Next`、`X / Y / Total`、`Copy / Paste` 仍硬编码英文；
- 没有中文正式元素名称或中文搜索；
- 手写 JSON 解析器遇错静默停止，不能严格报告重复键、尾部垃圾和位置；
- `build.bat` 硬编码 Visual Studio、Microsoft Store Python 与 `C:\tools\sccache`；
- 提交了陈旧 `src/VcsTag.h`；
- `mod_id=16` 属于原汉化模组，不应由 OmniPack 冒用。

### 发布阻塞

官方 `font.bz2` 为 57,718 字节，而汉化版本为 476,825 字节。修改没有字体名称、作者、原始字形源码、来源 URL 或许可证。该二进制在补齐授权前不得发布；应替换为明确授权的中文字库并保留字体许可证和生成步骤。

### 已知崩溃差距

汉化 HEAD 缺少的官方提交包括 PROP 垃圾输入崩溃、BOMB 越界读取、`!bubble` 越界写、MIX/MERC 除零、BSON 与风扇数据边界检查和未定义行为修复。因此 `445fab51` 不能直接作为安全发布底座。

**裁决：**以当前官方模拟/存档核心为底座，逐文件重放并校订翻译；禁止用汉化分支整个 `src` 覆盖官方树。

## 2. The-Powder-Toy/The-Powder-Toy

当前 master 是 TPT 100.0.399 系列的权威开发头，包含正式 `v100.0.399` 后续 22 个提交。必须保留：

- 官方元素 ID、identifier 和保留槽位；
- OPS/PSv 保存、BSON 边界和 palette 语义；
- Meson 构建体系；
- 当前模拟循环、Lua API、UI 和网络安全修复；
- 原始作者版权、GPL 和致谢。

当前 master 也不能被描述为“无已知崩溃”：审计时公开未关闭报告包括 #1091、#1089、#1087、#1086 和 #1083，分别涉及粒子/Lua/印章边界或未定义行为。OmniPack 需保留上游同步路径并增加自己的回归测试。

**裁决：**直接作为生产底座，不从旧模组反向覆盖核心。

## 3. cracker1000/The-Powder-Toy

### 新增元素

旧基线后连续登记 51 个 ID `192..242`：

```text
FNTC CLNT LED TIMC FUEL FPTC DMRN PINV WALL COND QGPP UVRD SUN TMPS
PHOS CMNT NTRG PRMT CLUD BEE ECLR CEXP LITH2 PROJ PPTI PPTO SEED CSNS
CWIR CLRC COPR PCON STRC TURB BFLM PET MISL AMBE CHLR ACTY ELEX RADN
GRPH BASE WHEL NAPM GSNS MGNT SODM BALL RUBR
```

`LASR.cpp` 存在但未登记，并错误定义第二个 `Element_BALL()`，更新函数基本为空，是明确的遗留未完成文件。

### 冲突与风险

- `192 FNTC / 193 CLNT / 194 LED / 195 TIMC` 覆盖当前官方 `RSST/RSSS/BASE/SEED`；
- 模组自己的 `SEED=218`、`BASE=235` 又与官方 identifier 重名；
- `BEE` 用像素半径直接索引缩小后的压力网格，存在越界写风险；
- `PET` 每粒子每帧可执行 4,900 格加 450 格搜索；
- `MGNT` 每粒子可扫描 6,561 格并触发连通洪泛；
- `BFLM` 无全局数量预算地传播；
- `QGPP` 描述明确为 “under investigation”；
- 提交历史含 “unfinished changes” 和 “severely broken”。

### 核心修改与差距

涉及元素注册、传感器/导线/动力门户、磁场、轮子、导弹、Lua、脚本管理器、HUD、Options、存档预览、客户端和模拟工具。与当前主干源码快照直接比较约 835 files，`+61833/-55482`；这不是可合并的局部补丁规模。

**裁决：**动力门户、受限传感器、导线、工业化学与磁场概念可按当前 API 重写；PET/BEE/BFLM/QGPP/LASR 和近重复元素不直接收录。

## 4. SpikeViper/The-Powder-Toy

新增 ID `192..221`：

```text
THOR NIH NIHM GGOO RGOO BLD DT LUNG MEAT BVES SKINE SKIND SKINS TUMOR
PSN NEUR WACK WACKY BACT WBLD GLUC DIGE SACID STOM MUCO SVLV POOP
INTE PLAT SCAR
```

这些 ID 与当前官方 `192..195` 及其他模组重叠。核心修改围绕血液、血管、肺、组织、皮肤、神经、葡萄糖、消化、细菌、白细胞、毒素、肿瘤和死亡/疤痕组织，建立了有价值的生命循环概念，但基于 97.0 模拟 API。

与主干源码快照直接比较约 813 files，`+42070/-55306`。该规模及 ID 冲突排除整树合并。正式设计还必须补充：

- 氧气、二氧化碳和营养的明确运输状态；
- 感染/免疫的局部事件和每帧预算；
- 大结构的简化生物开关；
- 不依赖全地图扫描的死亡原因和性能保护；
- 中文状态、危险信息和图鉴。

**裁决：**生命网络和元素关系作为首要设计参考；按局部邻域、事件队列和模块预算重写，不直接复制旧模拟核心。

## 5. Bowserinator/TPT-Ultimata-Mod

在 192 个旧基线登记项后加入 142 个元素，ID `192..333`。改动覆盖：

- 传送凝胶、漏斗、力场、单向结构和特殊墙；
- 电子、电路、逻辑与传感；
- 电磁、时间和特殊物理；
- 有机化学与材料；
- 载具、角色装备和爆炸物；
- UI、工具、渲染和模拟核心。

仓库 Wiki 含大量元素/工具文档，也专门记录电路已知 bug。与主干快照比较约 1,330 files，`+195830/-56186`，是本次差距最大的来源。连续 ID 全部与其他旧模组重叠，不能保留。

强力扩散、时间、电磁和全局场系统若原样进入会影响随机数、保存、网络逻辑和性能；未完成的电磁/应力设想不能作为正式功能宣传。

**裁决：**只从 Wiki 和固定源码挑选有明确克制、预算和工程用途的概念；传送/漏斗/力场可列重写候选，时间和全局场必须先通过架构与确定性测试。

## 6. jacob1/The-Powder-Toy

README 明确说明代码基于旧 TPT 83 架构；`SAVE_VERSION=100` 是长期手工回移字段，不代表已升级到当前 C++/Meson 核心。新增：

```text
196 MOVS  197 ANIM  198 INDI  199 PPTI  200 PPTO
201 BUTN  202 PINV  203 RAZR  204 PWHT  205 EXPL
```

保存会写入 `"Jacob1's_Mod"=27` 和元素 palette，适合作为来源识别/迁移参考。

确定风险包括：

- `MOVS` 在边界判断前读取四个正交邻格；
- `EXPL` 邻域访问无边界判断；
- `PWHT` 无条件读取上方格；
- legacy save 用 `type > PT_NUM`，错误放过 `type == PT_NUM`；
- 某处类型条件写成不可能同时成立的 `type < 0 && type >= PT_NUM`；
- README 承认大量删除 ANIM 会冻结；
- 保存浏览器仍用七年以上的旧接口，多个 UI/保存 TODO 未完成。

与主干共同祖先可追溯到 2012 年；快照比较约 932 files，`+119924/-92607`。

**裁决：**`PPTI/PPTO`、`BUTN` 和带预算的 `PWHT` 重写候选；保存模组字段、标签页/恢复 UX 仅参考；MOVS/ANIM 需全新数据设计；RAZR/INDI/EXPL 默认排除。

## 7. jacob1/TPT-Alchemy

没有新元素；初始菜单恰好只开放 `FIRE/WATR/STNE/O2`。旧状态是：

```cpp
bool elementsAcquired[PT_NUM];
```

反应散落在约 160 个元素文件，43 个文件引用八邻域 `craft_with`，没有结构化配方表或版本化解锁树。17 个成就均硬编码英文。

确定缺陷：

- 进度只写 `powder.pref`，无 schema/版本，且仅在 `GameModel` 析构时保存；
- 读取数组未限制到 `PT_NUM`；
- `ResetProgress()` 无 UI 调用点，并会清除四个初始元素后直接退出；
- 本地存档可绕过发现限制；
- `SPC_AIR=256` 会先进入无范围检查的 `ElementAcquired(256)`，形成确定越界；
- `ignoreElementAcquistion` 未初始化；
- “全部发现”成就错误包含 `PT_NONE`。

快照比较约 870 files，`+62095/-73583`。

**裁决：**进度、发现限制、通知、成就和四元素开局全部拒绝移植。公开源码仅用于证明该旧系统存在的缺陷；不再作为当前产品设计来源。可独立评估其中具体化学反应概念，但必须进入自由沙盒统一反应表，且不得附带解锁条件。

## 8. SeppoTPT/Seppo-s-Metallurgy-Mod-SRC

登记 32 个新元素，旧 ID `187..218`：

```text
CHRC SFAC ETHL KERO GASO TIN COPR BRNZ HELI LEAD COBA STEL CROM ALUM
DURA COCH MLYB SODI CHLR COCA ANAC TTSL CRUC SALD MAGN NICK ALBR
ALNI ALNC TERN MAGX PLTU
```

**关键事实：公开仓库缺少上述全部 32 个自定义元素的实现 `.cpp` 文件。** Meson 登记存在但无法链接，公开 forks 也没有补齐。因此不能从该仓库取得可发布实现，更不能从论坛二进制反编译或复制。

论坛公开说明还记录：

- 合金比例行为错误；
- `WOOD -> CHRC` 永不发生；
- 酸元素只是占位。

快照比较约 843 files，`+51785/-60658`，但这主要是旧 95.0 树差距，不能弥补缺失实现。

**裁决：**只把元素清单、合金生产线与论坛公开行为当清洁室需求参考；所有冶金实现、数值和反应网络由本项目重新设计。若未来取得合法完整源码，必须重新审计精确 commit 和作者。

## 9. cbeimers113/cyens-toy

在旧 192 项基线上加入 `CRBN=192` 与 `N2=193`，直接覆盖当前官方 `RSST/RSSS`。核心实验包括：

- 时间膨胀；
- 局部重力；
- 可压缩气体；
- 烃类/有机反应网络；
- 离子键实验。

最后提交标题是 `Start ionic bonding system`，表明离子体系未完成。烃系统还大幅重定义官方 `GAS/OIL/MWAX/WAX` 等既有元素行为；原样移植会让官方存档语义改变。快照比较约 808 files，`+55124/-57100`。

**裁决：**烃链、气相反应和可压缩气体可作为算法/设计参考；使用新稳定标识符或向后兼容扩展，不能覆盖官方元素基础语义。时间和局部重力必须通过确定性、保存和性能测试后才可进入实验模块。

## ID 冲突矩阵

| 旧 ID | 官方 TPT 100 | Cracker | SpikeViper | Ultimata | Seppo | Cyens |
|---:|---|---|---|---|---|---|
| 187–191 | 官方已用 | — | — | — | 自定义 | — |
| 192 | RSST | FNTC | THOR | 自定义 | 自定义 | CRBN |
| 193 | RSSS | CLNT | NIH | 自定义 | 自定义 | N2 |
| 194 | BASE | LED | NIHM | 自定义 | 自定义 | — |
| 195 | SEED | TIMC | GGOO | 自定义 | 自定义 | — |
| 196–218 | 当前空闲 | 自定义 | 自定义至 221 | 自定义 | 自定义至 218 | — |
| 219–242 | 当前空闲 | 自定义 | 自定义至 221 | 自定义 | — | — |
| 243–333 | 当前空闲 | — | — | 自定义 | — | — |

结论：旧数字 ID 在没有来源证据时没有唯一语义。所有迁移必须先识别来源，再映射到稳定 identifier；不能按移植顺序分配新 ID。

## 重复内容与系统冲突

| 冲突组 | 来源 | 决策 |
|---|---|---|
| 动力门户 `PPTI/PPTO` | Cracker、Jacob、Ultimata | 选一个现代重写实现，共用稳定 ID |
| 铜/氯/钠/燃料/合金 | Cracker、Seppo、Cyens | 合并为单一工业化学网络，不保留近重复元素 |
| 组织/毒素/感染 | SpikeViper、Ultimata | Spike 生命循环为主需求，其他来源只补机制 |
| 磁场/力场/局部重力 | Cracker、Ultimata、Cyens | 分离工程磁体、可控力场和实验全局物理 |
| 灰蛊/异常复制 | Cracker、Ultimata | 统一粒子/帧预算和克制机制 |
| 时间控制 | Cracker、Ultimata、Cyens | 不改全局时钟；仅实验局部更新调度，默认关闭 |
| 旧发现/进度系统 | Alchemy | 拒绝；只保留无解锁条件的独立反应概念 |

## 总体移植判定

### 可直接保留

- 当前官方 TPT 100.0.399 的元素 ID、保存、模拟和安全修复；
- 各仓库 GPL、版权和原项目致谢；
- 已验证不含第三方实现的文档性事实。

### 审核后重放

- Dragonrster 的英文/中文翻译语料和本地化调用点；
- 各模组公开 Wiki/README 中的玩法说明，按引用和作者登记。

### 必须按当前代码重写

- 所有旧模组新增元素实现；
- 生物循环、冶金反应、核工业、自动化门户/传感器；
- 旧模组中值得保留但依赖过时核心的材料与反应；
- 时间、磁场、力场、灰蛊等强力系统及其预算；
- 旧存档来源识别、稳定 ID 迁移和缺失模块占位。

### 排除

- 没有源码的 Seppo 二进制实现；
- 未登记/未完成的 Cracker `LASR` 和调查态 `QGPP`；
- 只有颜色或小参数差异的重复元素；
- 没有数量/寿命/每帧预算的无限复制或传播实现；
- 会覆盖当前官方元素语义而没有兼容层的 Cyens 实现；
- 未经验证的全局时间、电磁或应力实验。

## 复核位置与命令

固定审计副本位于：

`C:\Users\KR\tpt-omnipack-sources`

主要复核命令：

```powershell
git show -s --format="%H %cI %s" HEAD
git branch --show-current
git describe --tags --always
git merge-base <source> <official>
git rev-list --left-right --count <base>...<head>
git diff --shortstat --no-index <source-a> <source-b>
rg -n "Identifier\s*=|PT_[A-Z0-9_]+\s*=" src
rg -n "MenuVisible\s*=\s*1" src/simulation/elements
rg -n "TODO|unfinished|under investigation|known bug" .
Get-FileHash LICENSE -Algorithm SHA256
```

本报告中的“已知崩溃/风险”来自具体源码边界、公开问题或仓库明确说明；没有用“issue 为零”推断“没有 bug”。
