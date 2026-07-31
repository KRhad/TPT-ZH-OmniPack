# 开发进度

## 当前分支基线

```text
branch=research/mod-source-integration
audit_start_head=3ee6b0a15cbd7d76f0605af3b614215a3b54a9d4
phase1_commit=cdbb87e288c4c800d23ed3834c6a07c4960daab2
mod_catalog_commit=bbb6d805
periodic_id_infrastructure_commit=21b160a5
pt_num=512
pmapbits=9
official_active_elements=195
omnipack_active_elements=59
total_active_elements=254
registered_slots=462
reserved_slots=208
enabled_content_modules=4
periodic_elements_placeable=37
periodic_elements_remaining=81
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

## Phase 2/3：周期表 UI、稀有气体与碱金属批次

- 118 行来源映射生成编译时周期表模型；标准长式面板支持中英文名、符号、原子序数、identifier、状态、放射性和金属类别筛选，f 区可展开；
- 周期表只是直接选择入口，不读取存档进度，不增加解锁或任务状态；未实现的 81 格保留正确位置并明确显示待实现；
- 复用官方氢 `148`；新增 `HE=370`、`NE=375`、`AR=379`、`KR=390`、`XE=405`、`RN=431`、`OG=461`；
- 复用官方锂 `191` 和铷 `41`；新增 `NA=376`、`K=380`、`CS=406`、`FR=432`，水反应强度依族序增加，熔融态保留反应，钫以 `180..360` 游戏刻压缩衰变为钋和一个有限寿命光子；
- 共享 `OmniPeriodic.cpp` 只做 `3x3` 局部检查，放电、低温换热、碱金属反应和衰变共享 `1024` 次/帧预算；火焰与高能光子寿命有限；
- 真实客户端 Lua 回归：`OMNI_PERIODIC_STATUS=PASS`、11 个新元素、37 个已实现周期映射、6 个碱金属族成员，压力帧事件恰为 1024；
- 周期 OPS：3 进程、2 重启、2 加载、17 粒子、25 字段断言、11 个直接大于 255 的类型与携带字段通过；
- 模块/Lua 回归确认 `OMNI_PT_HE` 与 `OMNI_PT_NA` 可直接选择且 Lua 仍优先分配 ID `255`；
- 全新 `build-periodic-alkali-final-clean` Windows x64 Release 构建 `524/524` 通过；EXE SHA-256 `081D54682C8904DC36591FCEA757AB7A631613D8DEB60BD3563EC07CF0F4A325`，Meson static `21/21`、Python `144/144`（0 skip）通过；
- clean EXE 已复跑周期、模块、六类 OPS 与 mixed OPS：合计 21 个进程、14 次重启、14 次加载验证；六类为 96 粒子/145 字段断言，周期用例含 11 个大于 255 的直接类型。

## Phase 1：纯沙盒方向清理

- 删除玩家炼金服务、十阶段进度、元素发现锁、进度窗口和通知；
- 删除 GameSave/Simulation 的 `omniAlchemy` 读取与写出；
- 删除 Lua 进度 API、stamp 锁和普通创建中的进度分支；
- 保留模块关闭、非法 ID 和保留槽门禁；
- 删除 0.4 炼金本地包 profile、文档、探针和专用测试；
- 删除五个没有实际内容的玩家设置入口，稳定 ID 区间不变；
- 新增两个真实旧 OPS 的忽略字段/重存兼容探针；
- 图鉴正文新增“元素说明 / Element description”标签；
- 重建 12px 字体：14,721 字形、2,685 个语言与周期表必需字符、SHA-256 `B31C93BBA0A967386D827F24DA7104F97C37FAEAC9A897BA8D55F23F32916C41`；
- 全新 Release clean build `509/509` 通过，EXE SHA-256 `CAC60B021E92E46C232B8DEF07126BE8CF2F791BDE1722550C341AA02D3A05C4`；Meson static `18/18`、Python `124/124`、0 skip。
- 当前四模块及完整/简化生态 Lua 回归 `6/6`；0.2 反应样例 `7/7`、回归场景 `8/8`；自动化场景 `9/9`、工程断言组 `6/6`、95 断言、停止增量 0。
- 官方与四模块 OPS `5/5`：15 个进程、10 次重启、10 次加载、79 粒子、每次加载合计 120 个字段断言。

## 下一步

1. 实现碱土金属共享水/酸/氧反应与缺失成员；
2. 连续推进硼族和后续主族元素；
3. 每批继续登记、运行回归、OPS 双往返和性能预算验证；
4. 保持 `release_ready=false`，直到 118 元素、300+ 材料和全部 1.0.0 门禁真实完成。

开发回归场景、构建脚本、测试矩阵和版本门禁继续保留；它们不属于已删除的玩家游戏任务。
