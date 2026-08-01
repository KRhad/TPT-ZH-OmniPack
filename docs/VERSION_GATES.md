# 版本硬门禁

## 通用规则

- 每个状态必须为 `true`、`false`、`not_tested` 或精确计数。
- 旧版本证据不能自动继承给新二进制；所有报告必须绑定 source commit 和 EXE/ZIP 哈希。
- 玩家任务、成就、科技树、炼金进度或强制解锁重新出现时，任何版本均不得发布。

## `0.1.x`

```text
game_tasks_removed=true
achievements_removed=true
challenge_system_removed=true
technology_tree_removed=true
alchemy_progression_removed=true
forced_unlocks_removed=true
omnipack_elements=48
enabled_modules=4
stable_ids_reassigned=false
legacy_progress_save_compatibility=true
```

本阶段还需完成 Phase 1 clean build、运行回归和 GUI 核验后才能更新测试候选。

## `0.2.0` 周期表

```text
periodic_table_elements=118
periodic_table_ui=true
all_periodic_elements_placeable=true
periodic_source_map_complete=true
periodic_family_tests_pass=true
```

只有 UI 或百科页面、没有 118 个可放置元素时不得通过。

当前开发树真实状态：

```text
periodic_table_elements=118
periodic_table_ui=true
all_periodic_elements_placeable=true
periodic_source_map_complete=true
periodic_family_tests_pass=true
periodic_clean_build_pass=true
periodic_ops_roundtrip_test=true
periodic_table_gui_visual_test=not_tested
periodic_stress_test=not_tested
periodic_0_2_content_gate=true
periodic_0_2_release_gate=false
```

`periodic_0_2_release_gate=false` 只表示本批全新 clean build、正式压力矩阵和可信桌面人工视觉尚未全部形成证据；不否定 118 种元素已实现、可直接放置且自动化族行为与 OPS 往返已通过。

## `0.3.0`–`0.8.0` 内容批次

每版必须满足登记、ID、双语、图鉴、来源、反应、相变、模块、OPS、性能和许可证测试。纯换色或无玩法差异材料不计入数量。

当前 0.3.0 无机化学两批状态：

```text
inorganic_batch1_elements=16
inorganic_batch2_elements=16
inorganic_elements_total=32
total_playable_materials=367
element_registry_valid=true
reaction_registry_entries=166
reaction_registry_valid=true
inorganic_runtime_test=true
inorganic_event_budget_test=true
inorganic_ops_roundtrip_test=true
disabled_module_test=true
inorganic_final_clean_build=true
inorganic_gui_visual_test=not_tested
inorganic_0_3_release_gate=false
```

两批通过不等于 0.3.0 内容完成；剩余酸、碱、盐、氧化物和工业无机物仍需继续实现、去重和验证。

## `0.9.0` 整理

```text
duplicate_content_removed=true
menu_categories_complete=true
zh_en_symbol_identifier_search=true
reaction_registry_valid=true
stable_ids_valid=true
ops_roundtrip_test=true
stress_matrix_pass=true
```

## `1.0.0`

```text
game_tasks_removed=true
achievements_removed=true
challenge_system_removed=true
technology_tree_removed=true
alchemy_progression_removed=true
forced_unlocks_removed=true

periodic_table_elements=118
periodic_table_ui=true
all_periodic_elements_placeable=true

total_playable_materials>=300
element_registry_valid=true
reaction_registry_valid=true
stable_ids_valid=true

zh_localization_complete=true
en_localization_complete=true
font_visual_test=true

ops_roundtrip_test=true
disabled_module_test=true
save_compatibility_test=true
stress_test=true
long_run_test=true

clean_build_pass=true
all_automated_tests_pass=true
release_exe_stripped=true
developer_paths_removed=true
pe_security_flags_preserved=true
zip_audit_pass=true

font_license_resolved=true
third_party_license_audit=true
source_public=true
release_tag=v1.0.0
release_ready=true
```

当前 `release_ready=false`。不得为赶版本把 `not_tested` 改为 `true`，也不得用重复空壳凑到 300。
