# 已知问题

## 当前发布阻塞

1. 完整 118 元素周期表和周期表 UI 尚未实现。
2. 当前总活动材料为 243，尚未达到 `total_playable_materials>=300`。
3. 新内容路线所需的模组来源目录、许可证总审计、自动提取与去重报告尚未完成。
4. Phase 1 全新 clean build、静态测试、四模块 Lua、旧样例、自动化和五类 OPS 已通过；正式压力矩阵和两小时长跑尚未执行。
5. 用户已确认原生 Fusion Pixel Font 的中文可读性；新的内容界面、双语往返、100%/125%/150% DPI 和所有页面仍需最终 GUI 人工矩阵。
6. 图鉴仅有现有 48 个 OmniPack 元素的完整工艺内容；完整周期表与未来材料说明尚未完成。
7. 当前 `origin` 是旧汉化仓库，不是授权的 OmniPack 正式远端；不能擅自推送或发布。
8. 既有环境曾发现 GitHub classic PAT；Git 历史和已审计包未发现该模式，但撤销/轮换没有外部证据。
9. GCC 16 仍对 `OurVariant/Bson`、`PowderToy.cpp` 和 `Simulation::FloodParts` 给出既有优化警告，尚无独立根因结论。
10. 外部模组只有明确兼容许可证的源码才可直接移植；二进制、许可证不明或不兼容来源只能拒绝或独立设计参考。

## Phase 1 边界

- 炼金进度、元素发现、选择锁、进度窗口和 OPS 写入已删除。
- 旧 OPS `omniAlchemy` 兼容探针通过，但最终大型旧存档矩阵仍未完成。
- 空的特殊物理、灾害、实验、性能保护和详细 HUD 设置入口已从玩家界面移除；相应稳定 ID 分区继续保留。
- 图鉴现在明确标注“元素说明”，但实际窗口滚动与 DPI 视觉仍为 `not_tested`。

```text
release_ready=false
periodic_table_elements=not_tested
total_playable_materials=243
clean_build_pass=true
stress_test=not_tested
long_run_test=not_tested
source_public=false
release_tag=not_tested
```
