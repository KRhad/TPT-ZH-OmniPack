# 已知问题

## 当前发布阻塞

1. 周期表 UI 和 118 行元数据基础已实现，但当前只有 109/118 个纯元素映射可放置，仍缺 104–112 号共 9 个实际元素行为。
2. 当前总活动材料为 326，`total_playable_materials>=300=true`；这只满足数量下限，不代表内容族、质量、性能或发布门禁完成。
3. 模组来源目录、自动提取和去重报告已建立；逐文件、子模块与资源许可证总审计仍未完成，因此 `license_audit_pass=false`。
4. Phase 1 与周期表前十三批的全新 clean build、静态/Python 套件、模块/周期 Lua、六类与 mixed OPS 已通过；完整 118 元素压力矩阵和两小时长跑尚未执行。
5. 用户已确认原生 Fusion Pixel Font 的中文可读性；锕系新增字形、内容界面、双语往返、100%/125%/150% DPI 和所有页面仍需最终 GUI 人工矩阵。
6. 图鉴已有 131 个 OmniPack 元素的完整双语内容；余下周期元素与未来材料说明尚未完成。
7. 当前 clean EXE 仍动态依赖 MSYS2 的 GCC 运行库；本批自动运行通过依赖 UCRT64 PATH，不是可直接分发的剥离发布 EXE。
8. 当前 `origin` 是旧汉化仓库，不是授权的 OmniPack 正式远端；不能擅自推送或发布。
9. 既有环境曾发现 GitHub classic PAT；Git 历史和已审计包未发现该模式，但撤销/轮换没有外部证据。
10. GCC 16 仍对 `OurVariant/Bson`、`PowderToy.cpp` 和 `Simulation::FloodParts` 给出既有优化警告，尚无独立根因结论。
11. 外部模组只有明确兼容许可证的源码才可直接移植；二进制、许可证不明或不兼容来源只能拒绝或独立设计参考。

## Phase 1 边界

- 炼金进度、元素发现、选择锁、进度窗口和 OPS 写入已删除。
- 旧 OPS `omniAlchemy` 兼容探针通过，但最终大型旧存档矩阵仍未完成。
- 空的特殊物理、灾害、实验、性能保护和详细 HUD 设置入口已从玩家界面移除；相应稳定 ID 分区继续保留。
- 图鉴现在明确标注“元素说明”，但实际窗口滚动与 DPI 视觉仍为 `not_tested`。

```text
release_ready=false
periodic_table_elements=109
periodic_table_ui=true
periodic_table_gui_visual_test=not_tested
total_playable_materials=326
total_playable_materials_minimum=true
clean_build_pass=true
stress_test=not_tested
long_run_test=not_tested
source_public=false
release_tag=not_tested
```
