# 已知问题

## 当前发布阻塞

1. 周期表 UI、118 行元数据和 118/118 个可放置映射已实现；正式 60 秒预热/600 秒压力采样、两小时长跑和 GUI 周期表视觉矩阵仍未执行。
2. 当前引擎活动槽为 419，其中含 1 个隐藏兼容别名；实际可玩材料为 418，`total_playable_materials>=300=true`。这只满足数量下限，不代表后续材料族、质量、性能或发布门禁完成。
3. 模组来源目录、自动提取和去重报告已建立；逐文件、子模块与资源许可证总审计仍未完成，因此 `license_audit_pass=false`。
4. Phase 1、完整周期表十四批、无机三批、元素去重、10 位容量、工程合金首批、矿物/陶瓷/玻璃首批及代表性核素首批的 clean build/静态/Python/真实客户端/模块/OPS 证据已形成；核素独立 clean EXE 已完成复跑并固定哈希。S09/S10 仍只有 2 秒 smoke；正式 600 秒压力矩阵和两小时长跑尚未执行。
5. 用户已确认原生 Fusion Pixel Font 的中文可读性；既有新增字形和本批新增 `塔/氚/育` 原生字形仍需人工逐字复核，内容界面、双语往返、100%/125%/150% DPI 和所有页面仍需最终 GUI 人工矩阵。
6. 图鉴已有 224 个 OmniPack 登记项的完整双语内容，其中 223 个是可玩元素、1 个是旧 `MSCR` 兼容别名；所有模组说明先显示“元素说明”，且正文以对应中英文材料名称开头。后续有机物、电子材料和生态污染扩展说明尚未完成。
7. 当前开发 EXE 仍为 `debug=true`、`strip=false`，并动态依赖 MSYS2 的 GCC 运行库；本批自动运行通过依赖 UCRT64 PATH，不是可直接分发的剥离发布 EXE。相同源码/选项在两个不同构建目录产生不同字节数和哈希，`strings` 确认二进制嵌入各自绝对构建路径，因此 `reproducible_build=false`、`developer_paths_removed=false`。
8. 当前 `origin` 是旧汉化仓库，不是授权的 OmniPack 正式远端；不能擅自推送或发布。
9. 既有环境曾发现 GitHub classic PAT；Git 历史和已审计包未发现该模式，但撤销/轮换没有外部证据。
10. GCC 16 仍对 `OurVariant/Bson`、`PowderToy.cpp` 和 `Simulation::FloodParts` 给出既有优化警告，尚无独立根因结论。
11. 外部模组只有明确兼容许可证的源码才可直接移植；二进制、许可证不明或不兼容来源只能拒绝或独立设计参考。
12. 四模块关闭后的选择、Lua 创建、OPS 粒子保留和更新暂停已经自动验证；实际 GUI 的“正常加载/只读加载/取消”三选项仍为 `not_tested`。

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
engine_active_elements=419
compatibility_aliases=1
total_playable_materials=418
total_playable_materials_minimum=true
representative_isotope_elements=13
isotope_runtime_test=true
isotope_formal_600s_stress_test=not_tested
clean_build_pass=true
reproducible_build=false
developer_paths_removed=false
stress_test=not_tested
long_run_test=not_tested
source_public=false
release_tag=not_tested
```
