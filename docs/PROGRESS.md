# 开发进度

## 当前分支基线

```text
branch=development/content-expansion-1.0
audit_start_head=3ee6b0a15cbd7d76f0605af3b614215a3b54a9d4
phase1_commit=cdbb87e288c4c800d23ed3834c6a07c4960daab2
mod_catalog_commit=bbb6d805
periodic_id_infrastructure_commit=21b160a5
periodic_boron_group_base_commit=5d9b99471590428e6430e9070c25a0907710c222
periodic_carbon_group_base_commit=de91bbb67b2faef85ed178444a52a7b148328136
periodic_nitrogen_group_base_commit=2eeab9f39b503e580b259ac6897dc9e177e5244c
periodic_oxygen_group_base_commit=3c3c623a12c2f17374db26c067d454787ecbd7ae
periodic_halogen_group_base_commit=8066533aea4bc4a5d6ae73c5d776d56fabf84eb6
periodic_first_transition_base_commit=6c64c0a0b39ed3e9830f1077b04779b21943c380
periodic_second_transition_base_commit=b26112f95c08d9daa252e54f65c6e26a05cd17e9
periodic_third_transition_base_commit=248c516aefcbeae187de95aadb384c22280a678c
periodic_lanthanide_base_commit=7c49278b856461f61cec1cc4974db90c709968bb
periodic_actinide_base_commit=54411e08b8215638f403c9bc181afb48d9ca5ea5
periodic_superheavy_base_commit=518dd9a4d5cbb874281635f7e7552b1ce14050ee
inorganic_batch1_worktree_base=8abe277e1cac49bac4654da89c5271c19bf1e827
inorganic_batch2_worktree_base=7eed94db5fbe81a95315cfac29cf142af1b18d6a
inorganic_batch3_worktree_base=0eb4e11dd41fe4da72b8f8a1db65fa07a77898b1
capacity_expansion_worktree_base=4f473ebefe59e436228ec7799f9363d39fe166b1
pt_num=1024
pmapbits=10
official_active_elements=195
omnipack_registered_elements=191
omnipack_playable_elements=190
engine_active_elements=386
compatibility_aliases=1
total_playable_materials=385
registered_slots=513
reserved_slots=127
unallocated_capacity_slots=511
enabled_content_modules=4
periodic_elements_placeable=118
periodic_elements_remaining=0
inorganic_batch1_elements=16
inorganic_batch2_elements=16
inorganic_batch3_elements=18
reaction_registry_entries=192
total_playable_materials_minimum=true
periodic_table_ui=true
save_format=OPS1/BZip2
release_ready=false
```

## 模组素材库与周期表 ID 基础设施

- 已审计 41 个来源：27 个固定 Git HEAD 的仓库、2 个按 SHA-256 固定的论坛 Lua 源码；
- 提取 4,248 条 C++ 与 277 条 Lua 定义，跨分叉折叠为 770 个候选概念，其中 339 个来自根许可证兼容的源码；完整逐文件/资源许可证门禁仍为 `false`；
- 建立 118 行周期表来源映射：复用 15 个官方和 11 个 OmniPack 纯元素实现，92 个缺失元素固定到 `370..461`；
- 周期表阶段只需 9 位空间；后续无机三批占满 `462..511` 后，已另行完成 10 位扩容：OPS 第二类型字节、palette identifier、旧 8/9 位携带字段重打包、Lua `255..196` 首选动态槽与高位回退均已审计；
- 周期表 ID/字体基础设施 clean build `509/509`、Meson static `20/20`、Python `136/136`（0 skip）通过；
- 周期表中文名称的 118 个字符已全部加入确定性字体，新增稀有字形仍待人工桌面可读性检查。

## Phase 2/3：周期表 UI 与完整 118 元素

- 118 行来源映射生成编译时周期表模型；标准长式面板支持中英文名、符号、原子序数、identifier、状态、放射性和金属类别筛选，f 区可展开；
- 周期表只是直接选择入口，不读取存档进度，不增加解锁或任务状态；118 个格位均已映射到可直接放置的真实实现；
- 复用官方氢 `148`；新增 `HE=370`、`NE=375`、`AR=379`、`KR=390`、`XE=405`、`RN=431`、`OG=461`；
- 复用官方锂 `191` 和铷 `41`；新增 `NA=376`、`K=380`、`CS=406`、`FR=432`，水反应强度依族序增加，熔融态保留反应，钫以 `180..360` 游戏刻压缩衰变为钋和一个有限寿命光子；
- 复用并增强既有镁 `261`；新增 `BE=371`、`CA=381`、`SR=391`、`BA=407`、`RA=433`，区分铍钝化/毒性、镁白光燃烧、钙锶钡焰色和镭到氡衰变；
- 复用并增强既有铝 `256`；新增 `B=372`、`GA=385`、`IN=401`、`TL=428`、`NH=456`，实现硼中子俘获、镓脆铝、低熔点差异、铊毒性、酸/苛性/氧化代理和鿭到钋衰变；
- 复用官方钻石 `28`、硅 `187` 和既有锡 `259`、铅 `258`；新增 `GE=386`、`FL=457`，实现钻石惰性映射、硅氧化、锗有限放电、锡瘟、铅中子吸收/酸氧路线、熔融汽化和𫓧到钋衰变；
- 新增 `N=373`、`P=377`、`AS=387`、`SB=402`、`BI=429`、`MC=458`，连接 `N ↔ LNTG ↔ NICE`，实现氮有限放电、磷燃烧、砷升华、锑/铋属性与熔点差异、分级酸氧/汽化及 `MC → NH → POLO` 两段衰变；
- 复用官方 `O2=61`、`LO2=60` 与 `POLO=182`；新增 `S=378`、`SE=388`、`TE=403`、`LV=459`，实现氧液相往返、硫有限烟雾燃烧、硒光敏辉光、硒/碲差异化氧化与汽化、钋质子计数转钚及 `LV → FL → POLO` 两段衰变；
- 复用既有 `CHLR=360` 并保留其 11 条高级化学路径；新增 `F=374`、`BR=389`、`I=404`、`AT=430`、`TS=460`，实现氟遇水/氢成酸、分级卤化/消毒、溴/碘有限蒸气、碘熔融，以及 `AT → POLO` 与 `TS → MC → NH → POLO` 有界衰变；
- 第一过渡系复用 `TTAN=144`、`CHRM=262`、`IRON=76`、`COBT=263`、`NICL=260`、`COPR=257`、`ZINC=265`；新增 `SC=382`、`V=383`、`MN=384`，实现钪放电灯辉光、钒工具钢合金、锰钢液脱氧及分级酸蚀/氧化/汽化；
- 第二过渡系复用 `MOLY=264`；新增 `Y=392`、`ZR=393`、`NB=394`、`TC=395`、`RU=396`、`RH=397`、`PD=398`、`AG=399`、`CD=400`，实现钇放电辉光、锆蒸汽氧化、锝衰变、钌/铑催化、钯储氢、银硫化、镉中子吸收及分级酸蚀/氧化/汽化；
- 第三过渡系复用官方 `TUNG=171`、`PTNM=188`、`GOLD=170`、`MERC=152`；新增 `HF=423`、`TA=424`、`RE=425`、`OS=426`、`IR=427`，实现铪中子俘获、钽高温钝化、铼镍高温合金代理、锇有毒氧化物代理、铱过氧化氢催化及分级酸蚀/氧化/汽化；
- 镧系新增 `LA=408`、`CE=409`、`PR=410`、`ND=411`、`PM=412`、`SM=413`、`EU=414`、`GD=415`、`TB=416`、`DY=417`、`HO=418`、`ER=419`、`TM=420`、`YB=421`、`LU=422`，实现有限储氢/储氧、分级磁性激励、钷衰变、差异化中子吸收、双波段荧光、带冷却光子放大、镱温水制氢及全族分级酸氧/相变；
- 锕系复用官方 `URAN=32`、`PLUT=19`；新增 `AC=434`、`TH=435`、`PA=436`、`NP=437`、`AM=438`、`CM=439`、`BK=440`、`CF=441`、`ES=442`、`FM=443`、`MD=444`、`NO=445`、`LR=446`，实现分级寿命/衰变热、代表性后代、中子合成链、锎有限中子与裂变代理，并由铹高能俘获进入超重链；
- 104–112 号新增 `RF=447`、`DB=448`、`SG=449`、`BH=450`、`HS=451`、`MT=452`、`DS=453`、`RG=454`、`CN=455`；差异化硬度、导热、熔沸点与衰变后代，并用 `900..1700 K` 逐级中子门槛连接 `LR→RF→…→CN→NH`；
- 共享 `OmniPeriodic.cpp` 只做 `3x3` 局部检查，放电、低温换热、七类主族、三条过渡系、镧系、锕系与超重族反应和衰变共享 `1024` 次/帧预算；火焰、光子、中子和着色蒸气寿命有限；
- 真实客户端 Lua 回归：`OMNI_PERIODIC_STATUS=PASS`、92 个新元素、118 个已实现周期映射、七个主族各 6 个成员、三条过渡系、镧系/锕系/超重系各 15 个成员，压力帧事件恰为 1024；
- 周期 OPS：3 进程、2 重启、2 加载、125 粒子、134 字段断言、118 个周期元素直接类型、其中 103 个大于 255，并覆盖 `LAVA/SPRK/BRMT/CONV/VIRS` 携带字段；
- 模块/Lua 回归确认从 `HE`、`AC` 到 `RF` 的各批代表项均可直接选择且 Lua 仍优先分配 ID `255`；
- 全新 `build-periodic-superheavy-final-clean` Windows x64 Release 构建 `605/605` 通过；最终 EXE SHA-256 `EB56694250D2F8D88BFE138879FA50622BB3A1E4FD9D445BA6EC4AD3A611808F`，Meson static `21/21`、Python `159/159`（0 skip）通过；
- clean EXE 已复跑四模块、完整/简化生态、周期、模块、六类 OPS 与 mixed OPS：OPS 合计 21 个进程、14 次重启、14 次加载验证；六类为 204 粒子/254 字段断言和 203 个稳定/调色板 identifier，周期用例同图覆盖全部 118 个周期元素。

## Phase 4：无机化学首批

- 固定 `HCLA..CAOX=462..477` 共 16 个可直接放置材料：5 种酸、3 种碱、硝酸钾/硫酸铜/碳酸钙/碳酸氢钠、一氧化碳/二氧化硫/二氧化氮和氧化钙；官方 `SALT=26` 明确复用为氯化钠；
- `CHLR + H2` 在 450 K 以上改为生成精确盐酸 `HCLA`，不再生成泛化官方 `ACID`；旧 `ACID` 保留原 ID 和既有行为；
- 新材料接入原化学模块和 `OmniChemistry.cpp`，所有成功事件继续共享 `1536/frame` 预算，只读取固定 `3x3` 邻域；
- 反应覆盖酸碱/碳酸盐、氢氟酸腐蚀、硫酸铜与二氧化硫循环、硝酸/二氧化氮循环、磷酸肥料、碱吸收二氧化碳、石灰闭环、铁置换铜、硝酸钾有界氧化和一氧化碳燃烧；
- 登记门禁当前实测为 478 个槽、351 个活动元素、127 个保留槽；内容门禁确认 156 个 OmniPack 元素均有双语内容，反应登记为 151 条；
- 真实客户端化学回归为 `PATHS=29`、`ELEMENTS=26`、`INORGANIC_ELEMENTS=16`，压力帧事件恰为 1536；模块直选确认 `OMNI_PT_HCLA` 可用；
- 化学 OPS 已完成 3 进程、2 重启、2 加载、32 粒子、每次加载 40 字段断言和 32 个稳定/调色板 identifier；mixed OPS 为 11 粒子、20 字段断言；
- 四个内容模块增加按模拟刻缓存的运行门禁；启用进程保存、全部模块禁用进程加载的 OPS 回归保留 7 个粒子，冶金/生物/化学/核工业反应均暂停且事件为 0，周期元素不受影响；
- 全新 `build-inorganic-batch1-gated-final-clean` Release 构建 `621/621`、Meson static `21/21`、Python `160/160`（0 skip）通过；EXE 为 285,101,266 字节，SHA-256 `995317A93E2097FE0A11ADB6576C2697ED868D4593EFDEFC3A8CBDC5B919905B`；
- 最终 EXE 已从冶金开始复跑化学、完整/简化生态、核工业、周期、模块直选、六类 OPS 与 mixed OPS；首次未带 UCRT64 PATH 的启动失败保留为环境失败，正确 PATH 下所有回归通过；
- 该 EXE 使用 `release` 优化但 `debug=true`、`strip=false`，仅是本批开发证据，不是可公开分发的 1.0.0 发布 EXE。

## Phase 4：无机化学第二批

- 固定 `CARA..ZNOX=478..493` 共 16 个可直接放置材料，覆盖碳酸、硫化氢、氨水、氢氧化钡、六种精确盐/碳酸盐、高锰酸钾和五种金属氧化物；既有 ID 不移动；
- `OmniChemistry.cpp` 加入碳酸/硫化氢的通电催化与反向路径、氨水可逆释放、精确中和盐、氯化物溶解/水解、受限硝酸铵/高锰酸钾事件、过氧化物精确氧化和铁/铜/锌氧化物分级还原；全部继续使用 3×3 邻域与共享 `1536/frame` 预算；
- 登记门禁为 494 槽、367 活动、127 保留；内容门禁确认 172 个 OmniPack 元素双语完整，i18n 为 `1546/1546`，反应登记为 166 条；
- 真实客户端化学回归为 `PATHS=58`、`ELEMENTS=42`、`INORGANIC_ELEMENTS=32`，预算压力帧事件恰为 1536；模块直选确认 `CARA=478` 可用；
- 化学 OPS 为 3 进程、2 重启、2 加载、48 粒子、每次加载 56 字段断言和 48 个稳定/调色板 identifier；六类合计 236 粒子、286 字段断言和 235 identifier；mixed OPS 为 11 粒子、20 字段断言；
- 四模块禁用两进程 OPS 回归保留 8 个粒子，其中 `CARA=478` 在模块关闭后不丢失、不更新且拒绝选择/创建；总 Omni 事件为 0；
- 新增中文字符 `抵/铵` 后按固定 Fusion 来源重建字体：14,744 字形、2,708 必需字符、Unifont 回退 0、SHA-256 `B10BE53C1678B20FD85B61E4156D2A1A45824F2FB81863FA747289808487C835`；结构和离屏渲染通过，人工新字形视觉为 `not_tested`；
- 全新 `build-inorganic-batch2-final-clean` Release 构建 `637/637`、Meson static `21/21`、Python 160 项（158 PASS、2 SKIP、0 FAIL）通过；EXE 为 290,240,495 字节，SHA-256 `C42AF0C961AA8C1D5645D32BAEC0A587CF704663C33D0427AC40B9C28A624EB7`；
- 该 EXE 使用 `release` 优化但 `debug=true`、`strip=false`，仅是开发证据；正式 600 秒压力、两小时长跑和 GUI/DPI 仍为 `not_tested`，`release_ready=false`。

## Phase 4：无机化学第三批

- 固定 `SUTR..AMCL=494..511` 共 18 个可直接放置材料，覆盖三氧化硫、一氧化氮、二氧化钛、二氧化铀、磷酸钙、两种硫化物、氰化氢、两种碳化物、两种氮化物、两种氢化物和四种精确氯化物；既有 ID 不移动；
- `OmniChemistry.cpp` 加入硫/氮氧化物循环、过氧化物钛铀氧化、铀氧化物高温还原、硫化物遇酸放气、湿植物利用磷酸盐、碳化钙/氢化物水解、氯化物水解与置换、碳化硅高温氧化，以及九条数据共享式通电催化合成；所有成功事件继续使用 3×3 邻域与共享 `1536/frame` 预算；
- `SPRK(CATA)` 在有界化学反应成功时先完成反应再返回，避免通用点火在同一帧抢先把氢化物原料变成 `FIRE`；旧化学、冶金、生态、核工业和周期运行回归均通过；
- 该批提交时登记门禁为 512 槽、385 活动、127 保留；内容门禁确认 190 个 OmniPack 元素双语完整，i18n 为 `1582/1582`，反应登记为 190 条，化合物登记对第三批公式/ID/identifier 进行 fail-closed 校验；
- 真实客户端化学回归为 `PATHS=90`、`ELEMENTS=60`、`INORGANIC_ELEMENTS=50`、`BATCH3_IDS=494-511`，预算压力帧事件恰为 1536；模块直选确认最高 ID `AMCL=511` 可用；
- 化学 OPS 为 3 进程、2 重启、2 加载、66 粒子、每次加载 74 字段断言和 66 个稳定/调色板 identifier；六类合计 254 粒子、304 字段断言和 253 identifier；mixed OPS 保持 11 粒子、20 字段断言；
- 四模块禁用两进程 OPS 回归保留 9 个粒子，其中 `AMCL=511` 在模块关闭后不丢失、不热分解且拒绝选择/创建；总 Omni 事件为 0；
- 新增中文字符 `氰/盒/矿` 后按固定 Fusion 来源重建字体：14,747 字形、2,711 必需字符、Fusion 1,957、Unifont 回退 0、SHA-256 `855A77A8C38734A1045910DC57DA56AD081D9981DC405D7FAF040AB3A1FB2F8D`；来源解码、覆盖和离屏渲染通过，人工新字形视觉为 `not_tested`；
- 全新 `build-inorganic-batch3-final-clean` Release 构建 `655/655`、Meson static `21/21`、Python `162/162`（0 skip）通过；最终 EXE 为 296,047,216 字节，SHA-256 `C89B942EB67A6A78C3E8B1E94CFAB52EBDA0CC7CD1E6932D06118D4ED338C58E`；
- 最终 EXE 已复跑冶金、化学、完整/简化生态、核工业、周期、模块直选、四模块禁用、六类与 mixed OPS；0.2/0.3 开发样例在系统临时目录隔离生成和验证，仓库内旧 `omniAlchemy` 兼容样本保持原字节；
- 该 EXE 使用 `release` 优化但 `debug=true`、`strip=false`，仅是开发证据；正式 600 秒压力、两小时长跑和 GUI/DPI 仍为 `not_tested`，`release_ready=false`。

## Phase 10：元素行为级去重与官方 BRMT 增强

- 行为审计确认 `FERT/NUTR`、`PATH/VIRS`、精确酸碱/官方泛化材料、精确纯元素/官方泛化材料等仍有明确玩法差异；当前唯一合并项为 `OMNI_PT_MSCR=278` 与官方 `DEFAULT_PT_BRMT=30`；
- 可回收来源金属行为已迁入官方 BRMT，使用 `ctype` 保存来源类型、`tmp4=0x4F4D5343` 隔离增强状态；普通官方 BRMT 的 1273 K 相变和 `BRMT+BREC` 状态机保持；
- `MSCR=278` 继续注册为隐藏兼容别名：菜单/搜索/Lua 直接创建均拒绝，旧 OPS 和间接载体仍能读取，模块启用后一个模拟刻迁移，模块关闭时保持不动，ID 永不复用；
- 合并原则已扩展为“主元素增强”：有兼容许可证且存在玩法增量的同概念候选重写到官方或当前 OmniPack 主元素，不新增第二个 ID；完全相同且无增量的定义无需改代码；
- 当前计数为 513 个已登记槽、386 个引擎活动项、1 个兼容别名、385 个可玩材料和 127 个显式保留槽；OmniPack 为 191 个登记项 / 190 个可玩元素；
- 三进程迁移回归通过：旧 278 stamp 为 603 字节、SHA-256 `C057B705FCDE0CF8BD90B8F96139022EDB0606EA92D7A4F8BC2963F7E83E0C5F`；canonical BRMT stamp 为 645 字节、SHA-256 `D01A2DB444980110AFE62217D4B189BF33BB28B7631AF3F97FBBCCE4C25168B3`；
- 六类 OPS 为 18 进程、12 次重启、12 次加载、253 粒子和每次加载 305 个字段断言；mixed OPS 为 3 进程、11 粒子和 21 个字段断言；周期 OPS 的可回收 BRMT 同时覆盖 `ctype/tmp4`；
- 全新 `build-element-dedup-final-clean` 构建 `655/655`、Meson static `22/22`、Python `166/166`（0 skip）通过；冶金、周期、模块、四模块禁用、化学、生态双模式、核工业、0.2 示例/教程和临时 0.3 自动化开发探针均通过；
- 当前 EXE 为 296,063,798 字节，SHA-256 `39EDECDC992A48747E73F582ED340706A5B75A13DB521A593FB2F8946828C352`；S09/S10 两个 2 秒 smoke harness 通过，正式压力门禁仍为 `not_tested`；
- 该 EXE 为 `release` 优化但 `debug=true`、`strip=false` 的开发证据，不是可发布 1.0.0 二进制；GUI/DPI、正式 600 秒压力和两小时长跑仍未完成，`release_ready=false`。

## Phase 5 起步：10 位容量与锡铅工程合金

- `PMAPBITS` 从 9 安全扩展到 10，`PT_NUM` 从 512 扩展到 1024；旧 `0..511` ID 与 identifier 全部不变，`512..575` 固定为工程材料区，`576..1023` 为后续内容区；
- OPS 只接受来源 `pmapbits=8..16`，palette 表按来源位宽建立，直接类型先经 identifier 映射再按当前范围过滤，携带字段按来源位宽无符号拆包并按当前 10 位重打包；原生探针覆盖直接 `SOLD=512`、`LAVA/SPRK/BRMT/CONV/VIRS` 携带字段、来源槽 `1536` 到当前 `512` 的映射、缺失高位 identifier 报告/中和以及损坏位宽拒绝；
- Lua 分配器不再误占官方保留空洞 146，只先使用 `255..196`，耗尽后从 1023 向下；实际客户端确认边界 ID 255、196、1023 均可用；
- 新增 `SOLD=512` 锡铅合金：三份熔融锡与两份熔融铅在 700 K 以上形成五份熔融合金，460 K 凝固，反复火花每刻升温 45 K，压力阈值 16 并可通过带类型官方 `BRMT` 回炉；
- 登记门禁为 513 行、386 活动、1 兼容别名、385 可玩、127 显式保留和 511 未登记容量槽；反应登记为 192 条；
- 容量、冶金、模块/Lua、禁用模块和六类/mixed OPS 已有自动与真实运行证据；正式 600 秒压力、7,200 秒长跑和 GUI/DPI 仍为 `not_tested`，`release_ready=false`。
- 全新 `build-capacity-1024-solder-final3-clean` 构建 `663/663`、Meson static `24/24`、Python `175/175`（0 skip）通过；最终开发 EXE 为 296,453,527 字节，SHA-256 `836BCA8AA19BEF2CAFE134DDFB4C1DDDA2E2A31874211B9A0487887B9C4C0699`；
- 最终 EXE 复跑冶金以及六类与 mixed OPS；六类为 18 进程、254 粒子、306 字段断言、258 稳定 identifier 和 254 palette identifier，mixed 为 3 进程、11 粒子、21 字段断言；周期、化学、生态双模式、核工业、模块直选、四模块禁用和旧别名迁移保留此前 clean EXE 证据，本次未将其误记为新 EXE 复跑；
- 自动化与示例在系统临时目录隔离副本中以开发探针运行通过，未覆盖仓库内两个含 `omniAlchemy` 的旧字段兼容样本；该结果不替代正式 clean-source 发布证据。
- 两个不同绝对目录下的同配置 debug 构建哈希不同，且 `strings` 直接检出各自开发路径；这是 `debug=true/strip=false` 开发证据的已确认边界，正式可复现、去路径、剥离发布构建仍未完成。

## Phase 1：纯沙盒方向清理

- 删除玩家炼金服务、十阶段进度、元素发现锁、进度窗口和通知；
- 删除 GameSave/Simulation 的 `omniAlchemy` 读取与写出；
- 删除 Lua 进度 API、stamp 锁和普通创建中的进度分支；
- 保留模块关闭、非法 ID 和保留槽门禁；
- 删除 0.4 炼金本地包 profile、文档、探针和专用测试；
- 删除五个没有实际内容的玩家设置入口，稳定 ID 区间不变；
- 新增两个真实旧 OPS 的忽略字段/重存兼容探针；
- 图鉴正文新增“元素说明 / Element description”标签；
- 当前 12px 字体：14,747 字形、2,711 个语言与周期表必需字符、SHA-256 `855A77A8C38734A1045910DC57DA56AD081D9981DC405D7FAF040AB3A1FB2F8D`；补充平面字符的 UTF-8 往返和完整语言包离屏渲染已通过；
- 全新 Release clean build `509/509` 通过，EXE SHA-256 `CAC60B021E92E46C232B8DEF07126BE8CF2F791BDE1722550C341AA02D3A05C4`；Meson static `18/18`、Python `124/124`、0 skip。
- 当前四模块及完整/简化生态 Lua 回归 `6/6`；0.2 反应样例 `7/7`、回归场景 `8/8`；自动化场景 `9/9`、工程断言组 `6/6`、95 断言、停止增量 0。
- 官方与四模块 OPS `5/5`：15 个进程、10 次重启、10 次加载、79 粒子、每次加载合计 120 个字段断言。

## 下一步

1. 在固定工程材料区 `513..575` 继续下一批合金、矿物、陶瓷和玻璃；不得覆盖旧槽，也不得用重复空壳填充 1024 容量；
2. 同概念模组候选只把许可证兼容的玩法增量合并到主元素，并登记来源与主元素回归；
3. 为完整 118 元素与高位内容执行正式 60 秒预热/600 秒压力采样及后续两小时综合长跑；
4. 每批继续登记、运行回归、OPS 双往返和性能预算验证；
5. 保持 `release_ready=false`，直到后续材料族、人工 GUI、长跑、发布包和公开发布门禁全部真实完成。

开发回归场景、构建脚本、测试矩阵和版本门禁继续保留；它们不属于已删除的玩家游戏任务。
