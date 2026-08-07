# 已知问题

## 当前发布阻塞

1. 周期表 UI、118 行元数据和 118/118 个可放置映射已实现；最右侧入口已从 `P` 改为专属像素图标，位置、点击回调、构建和运行回归通过。旧 `P` 入口的 96 DPI/2× 实屏点击证据不能替代新图标视觉；专属图标、完整周期表、125%/150% DPI 和两小时长跑仍未完成人工矩阵。
2. 当前引擎活动槽为 488，其中含 1 个隐藏兼容别名；活动非别名类型为 487，普通菜单可直接选择的真实材料为 466，`total_playable_materials>=300=true`。其余类型是擦除工具及官方隐藏过渡/辅助类型；这只满足数量下限，不代表质量、性能或发布门禁完成。
3. 模组来源、选用文件、字体、跟踪资源和静态库许可证总审计已完成，`license_audit_pass=true`；旧 `0.7.0-dev` 私测 ZIP 没有携带新增的静态库许可证清单，因此只保留为历史包。`f970a534` rc9 候选已通过成员级许可证与 ZIP 审计；未来若重新打包最终 1.0.0，仍须对新 ZIP 重新审计。
4. Phase 1、完整周期表十四批、无机三批、元素去重、10 位容量、工程合金首批、矿物/陶瓷/玻璃首批、代表性核素首批、两批有机内容、电子材料首批及生态污染首批的静态、真实客户端、模块和 OPS 证据已形成。电子 S13 与环境 S14 的 600 秒结果保留为对应旧候选的历史证据；rc9 的 S15-S20 各 30 秒稳定性门禁已 `6/6` 通过，独立两小时长跑仍为 `not_tested`。
5. 用户已确认原生 Fusion Pixel Font 的中文可读性；既有新增字形和第二批新增 `橡/沥/淀/糖/纤/胺/萄/葡/酯/韧/龙` 原生字形仍需人工逐字复核，内容界面、双语往返、100%/125%/150% DPI 和所有页面仍需最终 GUI 人工矩阵。
6. 图鉴已有 293 个 OmniPack 登记项的完整双语内容，其中 292 个是可玩元素、1 个是旧 `MSCR` 兼容别名；所有模组说明先显示“元素说明”，正文以对应中英文材料名称开头，且玩家窗口不再显示稳定 ID、源码 commit、实现/测试状态等开发门禁字段。内容数量现已冻结，不再以新增重复材料作为发布前工作。
7. 环境开发证据 EXE 仍为 `debug=true`、`strip=false` 并动态依赖 MSYS2 GCC 运行库；它不是分发文件。另行生成的 `0.7.0-dev` 私测 EXE 已静态链接、剥离并通过开发路径与 PE 审计，但只证明该私测包的便携二进制门禁；完整可复现构建、签名和正式发布门禁仍未完成。
8. 当前 `origin` 是旧汉化仓库，不是授权的 OmniPack 正式远端；不能擅自推送或发布。
9. 既有环境曾发现 GitHub classic PAT；Git 历史和已审计包未发现该模式，但撤销/轮换没有外部证据。
10. GCC 16 仍对 `OurVariant/Bson`、`PowderToy.cpp` 和 `Simulation::FloodParts` 给出既有优化警告，尚无独立根因结论。
11. 外部模组只有明确兼容许可证的源码才可直接移植；二进制、许可证不明或不兼容来源只能拒绝或独立设计参考。
12. 五模块关闭后的选择、Lua 创建、OPS 粒子保留和更新暂停已经自动验证；实际 GUI 的“正常加载/只读加载/取消”三选项仍为 `not_tested`。

## Phase 1 边界

- 炼金进度、元素发现、选择锁、进度窗口和 OPS 写入已删除。
- 旧 OPS `omniAlchemy` 兼容探针通过，但最终大型旧存档矩阵仍未完成。
- 空的特殊物理、灾害、实验、性能保护和详细 HUD 设置入口已从玩家界面移除；相应稳定 ID 分区继续保留。
- 图鉴现在明确标注“元素说明”，但实际窗口滚动与 DPI 视觉仍为 `not_tested`。

```text
release_ready=false
periodic_table_elements=118
periodic_table_ui=true
periodic_table_gui_visual_test=not_tested
periodic_icon_static_test=true
periodic_icon_visual_test=not_tested
periodic_button_scale2_visual_test=not_tested
periodic_button_click_open_test=not_tested
engine_active_elements=488
compatibility_aliases=1
active_non_alias_types=487
directly_selectable_materials=466
total_playable_materials=466
total_playable_materials_minimum=true
representative_isotope_elements=13
isotope_runtime_test=true
isotope_formal_600s_stress_test=not_tested
organic_batch1_elements=13
organic_runtime_test=true
organic_formal_600s_stress_test=not_tested
organic_batch2_elements=20
organic_batch2_runtime_test=true
organic_batch2_gui_visual_test=not_tested
electronics_batch1_elements=20
electronics_runtime_test=true
electronics_ops_roundtrip_test=true
environment_batch1_elements=16
environment_runtime_test=true
environment_ops_roundtrip_test=true
environment_s14_smoke_test=true
environment_s14_formal_test=true
environment_s14_performance_gate=true
environment_clean_build_pass=true
environment_clean_build_commit=031c36ff7f2838e7f5d9e76bb2c8f1ec1e6fd424
environment_clean_exe_sha256=154A8D24FA52B54A3F3038C14E7B96728E09B1C6415503D7AB61DE90F634C709
environment_formal_600s_stress_test=true
electronics_formal_600s_stress_test=true
electronics_s13_performance_gate=true
clean_build_pass=true
reproducible_build=false
developer_paths_removed=false
private_test_0_7_package_audit=true
private_test_0_7_release_exe_stripped=true
private_test_0_7_developer_paths_removed=true
private_test_0_7_pe_security_flags_preserved=true
rc9_candidate_manifest_revision=f970a5342a96fa69121902368196acdf1a71aa83
rc9_candidate_zip_audit=true
content_freeze_stability_s15_s20=true
stress_test=true
long_run_test=not_tested
source_public=false
third_party_license_audit=true
release_tag=not_tested
```
