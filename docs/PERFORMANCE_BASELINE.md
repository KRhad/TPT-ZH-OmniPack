# 性能基线与预算

## 规则

- 元素更新只使用局部邻域或显式分帧游标，不允许普通更新函数全图扫描。
- 化学、冶金、生态、核衰变、爆炸和超重元素共享统一的单帧事件预算。
- 禁止递归无界创建、无寿命高能粒子、无营养或寿命限制的生态增殖。
- 模块关闭时不执行该模块的扩展更新。
- 性能通过必须同时包含场景行为断言；仅 FPS、进程存活或粒子数稳定不足以证明通过。

## 已有历史基线

`0.1.0-test` 曾对十个固定场景完成 `60` 秒预热和 `600` 秒采样，记录 FPS、1% low、工作集、粒子、事件、停止/恢复和 OPS 往返。该证据绑定旧候选，不自动证明 Phase 1 或未来 300+ 内容版本通过。

当前模块预算：

| 系统 | 既有预算/边界 |
|---|---|
| 工业冶金 | `2048` 事件/帧，`3x3` 局部配方 |
| 局部生态 | `1024` 事件/帧，局部传播、营养/寿命约束 |
| 化学与无机物 | `1536` 事件/帧，`3x3` 局部反应；新建气体前先检查空槽 |
| 受控核工业 | 有界中子/控制更新，不允许无界粒子链 |
| 周期元素族 | `1024` 事件/帧，`3x3` 放电/换热，衰变每次至多一个有限寿命光子 |

## 内容扩展压力场景

| 场景 ID | 内容 | 必要断言 |
|---|---|---|
| `P01-PERIODIC-118` | 118 种周期元素同时存在 | 全部可放置、无非法 ID、无空壳替代 |
| `P02-MOLTEN-METALS` | 大量熔融金属 | 相变与合金预算有界 |
| `P03-ACID-BASE` | 大量酸碱盐 | 中和产物和连锁事件有界 |
| `P04-ORGANIC-FIRE` | 大量有机燃烧 | 烟气/火焰有寿命、无递归爆量 |
| `P05-RADIOACTIVE` | 大量放射性和核素 | 衰变粒子、热和中子受预算限制 |
| `P06-ALLOY-FACTORY` | 大规模合金反应 | 成分消耗守恒的游戏化断言 |
| `P07-POLLUTION` | 大规模污染与生态 | 增殖、营养和寿命上限有效 |
| `P08-EXPLOSIVES` | 大规模爆炸 | 碎片和高能粒子数量/寿命有界 |
| `P09-MIXED-MODULES` | 多模块混合 | 各预算公平且模块关闭立即停止更新 |
| `P10-LARGE-SAVE` | 大型 OPS 反复加载 | 粒子、identifier、间接类型稳定 |
| `P11-SUPERHEAVY` | 104–118 衰变链 | 不出现永久稳定或无限衰变链 |
| `P12-PERIODIC-UI` | 最大搜索与筛选 | UI 响应时间和内存有界 |

## 采样要求

- 开发批次：至少 60 秒预热 + 600 秒采样；
- `0.9.0`：上述场景全部复跑；
- `1.0.0`：至少 7,200 秒综合长跑，同进程完成至少 10 次保存/加载、10 次语言切换和 10 次模块关闭/开启；
- 记录 source commit、EXE SHA-256、运行 ID、系统信息、FPS、1% low、最小 FPS、峰值工作集、粒子和事件计数；
- crash、hang、场景断言失败、无界增长、疑似泄漏或证据缺失任一出现，`performance_gate_pass=false`。

```text
phase1_clean_build_pass=true
phase1_stress_test=not_tested
periodic_noble_gas_clean_build_pass=true
periodic_noble_gas_clean_build_targets=520
periodic_noble_gas_exe_sha256=416A8661228DFD292AECDD344681CAEF88EDB034F4BAF18A9E8B8E910489AC60
periodic_noble_gas_budget_runtime=true
periodic_noble_gas_peak_events_per_frame=1024
periodic_carbon_group_clean_build_pass=true
periodic_carbon_group_clean_build_targets=536
periodic_carbon_group_exe_sha256=69ADB4214D150697E7478B3FD57C8C082D5C0836E29AA5CF3EE0E7159491640D
periodic_carbon_group_budget_runtime=true
periodic_carbon_group_peak_events_per_frame=1024
periodic_nitrogen_group_clean_build_pass=true
periodic_nitrogen_group_clean_build_targets=542
periodic_nitrogen_group_exe_sha256=A58CAF3CE3B337E11B224E70CE1B16BA4D587F089D4A7D2CE4C45B2EE3F9EC4C
periodic_nitrogen_group_budget_runtime=true
periodic_nitrogen_group_peak_events_per_frame=1024
periodic_oxygen_group_clean_build_pass=true
periodic_oxygen_group_clean_build_targets=546
periodic_oxygen_group_exe_sha256=F074CED19649A849A8DF919D4E33B11D5C5936A5551BBE10C7A52DFF80841763
periodic_oxygen_group_budget_runtime=true
periodic_oxygen_group_peak_events_per_frame=1024
periodic_halogen_group_clean_build_pass=true
periodic_halogen_group_clean_build_targets=551
periodic_halogen_group_exe_sha256=2D70ADE599F15718A415B813E4D918C62C3C641852F6AA6CBF0F05321C868F08
periodic_halogen_group_budget_runtime=true
periodic_halogen_group_peak_events_per_frame=1024
periodic_first_transition_clean_build_pass=true
periodic_first_transition_clean_build_targets=554
periodic_first_transition_exe_sha256=034592B42F21416D5D9DF59E78E092D7E72F12A5BE2F1E90E73C8D35030500EF
periodic_first_transition_budget_runtime=true
periodic_first_transition_peak_events_per_frame=1024
periodic_second_transition_clean_build_pass=true
periodic_second_transition_clean_build_targets=563
periodic_second_transition_exe_sha256=C3FA844BBA6F91286CB5FE7B3D1B54DFF692138BAB19F09E4C7B05107B39D68B
periodic_second_transition_budget_runtime=true
periodic_second_transition_peak_events_per_frame=1024
periodic_third_transition_clean_build_pass=true
periodic_third_transition_clean_build_targets=568
periodic_third_transition_exe_sha256=21682B0978F23535D68E456EF419439161DD9D4A3238934817B0DE7C773C7CAB
periodic_third_transition_budget_runtime=true
periodic_third_transition_peak_events_per_frame=1024
periodic_lanthanide_clean_build_pass=true
periodic_lanthanide_clean_build_targets=583
periodic_lanthanide_exe_sha256=DD32638A554B00EF918D921C155126594E502D98F15A99E6597E326922BCF3E8
periodic_lanthanide_budget_runtime=true
periodic_lanthanide_peak_events_per_frame=1024
periodic_actinide_clean_build_pass=true
periodic_actinide_clean_build_targets=596
periodic_actinide_exe_sha256=ADB2C2C9AFE6D4C6267F153D6EBDF6967CBE9FC981CBE31F4EDE2A5EAFAFBEC0
periodic_actinide_budget_runtime=true
periodic_actinide_peak_events_per_frame=1024
periodic_superheavy_clean_build_pass=true
periodic_superheavy_clean_build_targets=605
periodic_superheavy_exe_sha256=EB56694250D2F8D88BFE138879FA50622BB3A1E4FD9D445BA6EC4AD3A611808F
periodic_superheavy_budget_runtime=true
periodic_superheavy_peak_events_per_frame=1024
inorganic_batch1_clean_build_pass=true
inorganic_batch1_clean_build_targets=621
inorganic_batch1_exe_sha256=995317A93E2097FE0A11ADB6576C2697ED868D4593EFDEFC3A8CBDC5B919905B
inorganic_batch1_budget_runtime=true
inorganic_batch1_budget_samples=1800
inorganic_batch1_peak_events_per_frame=1536
inorganic_batch1_formal_600s_stress_test=not_tested
disabled_module_runtime_test=true
disabled_module_loaded_particles=7
disabled_module_update_events=0
periodic_118_stress_test=not_tested
mixed_300_material_stress_test=not_tested
long_run_7200s=not_tested
```
