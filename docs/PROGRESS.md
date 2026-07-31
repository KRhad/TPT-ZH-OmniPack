# 开发进度

## 当前分支基线

```text
branch=research/mod-source-integration
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
pt_num=512
pmapbits=9
official_active_elements=195
omnipack_active_elements=118
total_active_elements=313
registered_slots=462
reserved_slots=149
enabled_content_modules=4
periodic_elements_placeable=96
periodic_elements_remaining=22
reaction_registry_entries=123
total_playable_materials_minimum=true
periodic_table_ui=true
save_format=OPS1/BZip2
release_ready=false
```

## 模组素材库与周期表 ID 基础设施

- 已审计 41 个来源：27 个固定 Git HEAD 的仓库、2 个按 SHA-256 固定的论坛 Lua 源码；
- 提取 4,248 条 C++ 与 277 条 Lua 定义，跨分叉折叠为 770 个候选概念，其中 339 个来自根许可证兼容的源码；完整逐文件/资源许可证门禁仍为 `false`；
- 建立 118 行周期表来源映射：复用 15 个官方和 11 个 OmniPack 纯元素实现，92 个缺失元素固定到 `370..461`；
- OPS 第二类型字节、palette identifier、`PMAPBITS=9` 携带字段和 Lua `255..196` 首选动态槽已完成源码审计，不需要扩大 `PT_NUM`；
- 周期表 ID/字体基础设施 clean build `509/509`、Meson static `20/20`、Python `136/136`（0 skip）通过；
- 周期表中文名称的 118 个字符已全部加入确定性字体，新增稀有字形仍待人工桌面可读性检查。

## Phase 2/3：周期表 UI、前八个主族批次、三条过渡系与镧系

- 118 行来源映射生成编译时周期表模型；标准长式面板支持中英文名、符号、原子序数、identifier、状态、放射性和金属类别筛选，f 区可展开；
- 周期表只是直接选择入口，不读取存档进度，不增加解锁或任务状态；未实现的 22 格保留正确位置并明确显示待实现；
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
- 共享 `OmniPeriodic.cpp` 只做 `3x3` 局部检查，放电、低温换热、七类主族、三条过渡系、镧系反应和衰变共享 `1024` 次/帧预算；火焰、高能光子和着色蒸气寿命有限；
- 真实客户端 Lua 回归：`OMNI_PERIODIC_STATUS=PASS`、70 个新元素、96 个已实现周期映射、七个主族各 6 个成员、前两条过渡系各 10 个成员、第三过渡系 9 个成员及镧系 15 个成员，压力帧事件恰为 1024；
- 周期 OPS：3 进程、2 重启、2 加载、76 粒子、84 字段断言、70 个直接大于 255 的类型与携带字段通过；
- 模块/Lua 回归确认从 `HE` 到 `LA` 的各批代表项均可直接选择且 Lua 仍优先分配 ID `255`；
- 全新 `build-periodic-lanthanide-final-clean` Windows x64 Release 构建 `583/583` 通过；最终 EXE SHA-256 `DD32638A554B00EF918D921C155126594E502D98F15A99E6597E326922BCF3E8`，Meson static `21/21`、Python `157/157`（0 skip）通过；
- clean EXE 已复跑四模块、完整/简化生态、周期、模块、六类 OPS 与 mixed OPS：OPS 合计 21 个进程、14 次重启、14 次加载验证；六类为 155 粒子/204 字段断言和 154 个稳定/调色板 identifier，周期用例含 70 个大于 255 的直接类型。

## Phase 1：纯沙盒方向清理

- 删除玩家炼金服务、十阶段进度、元素发现锁、进度窗口和通知；
- 删除 GameSave/Simulation 的 `omniAlchemy` 读取与写出；
- 删除 Lua 进度 API、stamp 锁和普通创建中的进度分支；
- 保留模块关闭、非法 ID 和保留槽门禁；
- 删除 0.4 炼金本地包 profile、文档、探针和专用测试；
- 删除五个没有实际内容的玩家设置入口，稳定 ID 区间不变；
- 新增两个真实旧 OPS 的忽略字段/重存兼容探针；
- 图鉴正文新增“元素说明 / Element description”标签；
- 当前 12px 字体：14,735 字形、2,699 个语言与周期表必需字符、SHA-256 `7C779B232E5CE7CED156AFEA3D5F1A0FD5C82629CFFB4075B35330FA055B9254`；补充平面字符 `𫓧` 的 UTF-8 往返和完整语言包离屏渲染已通过；
- 全新 Release clean build `509/509` 通过，EXE SHA-256 `CAC60B021E92E46C232B8DEF07126BE8CF2F791BDE1722550C341AA02D3A05C4`；Meson static `18/18`、Python `124/124`、0 skip。
- 当前四模块及完整/简化生态 Lua 回归 `6/6`；0.2 反应样例 `7/7`、回归场景 `8/8`；自动化场景 `9/9`、工程断言组 `6/6`、95 断言、停止增量 0。
- 官方与四模块 OPS `5/5`：15 个进程、10 次重启、10 次加载、79 粒子、每次加载合计 120 个字段断言。

## 下一步

1. 实现锕系共享行为与 13 个缺失成员，并复用官方铀/钚；
2. 连续推进 104–112 号超重元素；
3. 每批继续登记、运行回归、OPS 双往返和性能预算验证；
4. 保持 `release_ready=false`，直到 118 元素、300+ 材料和全部 1.0.0 门禁真实完成。

开发回归场景、构建脚本、测试矩阵和版本门禁继续保留；它们不属于已删除的玩家游戏任务。
