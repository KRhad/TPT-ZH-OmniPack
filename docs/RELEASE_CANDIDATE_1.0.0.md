# TPT-ZH-OmniPack 1.0.0-rc9 本地发布候选说明

这是 Windows x64 的 1.0.0-rc9 本地发布候选，不是正式发布。它用于验证新的内容入口、周期表材料选择器、静态剥离二进制、许可证、存档、压力和长稳门禁；在全部门禁完成前保持：

```text
release_ready=false
```

本项目是以大量元素、化合物和材料为核心的自由沙盒整合版。项目不包含玩家任务、成就、挑战、科技树、炼金进度或强制元素解锁，所有已启用内容均可直接使用。

## 冻结内容口径

```text
periodic_table_elements=118/118
active_non_alias_types=487
creatable_types=484
selectable_materials_all_entries=466
standard_menu_materials=301
periodic_only_omnipack_materials=165
reaction_registry_entries=328
highest_registered_stable_id=685
element_capacity=1024
```

这些数量含义不同：487 是活动非别名类型，其中 484 个可由引擎成功创建；466 个真实材料可从全部入口选择。标准菜单展示 301 个，另有 165 个 OmniPack 周期单质、核素和无机物仅在周期表详情中显示。其余是擦除工具或官方隐藏过渡/辅助类型，不能冒充材料数量。

## 运行方式

1. 解压完整 ZIP，不要只复制 EXE。
2. 运行 `tpt-zh-omnipack.exe`。
3. 默认简体中文，可在设置中切换英文。
4. 周期表图标和元素搜索按钮位于最右侧竖列；周期表也可按 `T` 打开。
5. 单击元素格后，先阅读“元素说明”，再从单质、同位素和无机化合物分组中选择；有机材料和合金/工程材料使用右侧独立菜单。

## 已完成的候选验证

- 候选 manifest revision：`f970a5342a96fa69121902368196acdf1a71aa83`；普通 ZIP SHA-256：`3BB794F707B267FD6E5D32ED19F91E994814A38B5AA2E3B6CD49E43AD67EC4EC`；
- 普通/符号 ZIP、PE、路径、许可证、成员和解压 EXE 哈希审计通过；
- S15-S20 各自无预热的 30 秒稳定性门禁经独立 assessment `6/6` 通过；这不替代 7,200 秒长跑。

## 本候选仍未关闭的正式发布门禁

- 125%/150% DPI 与完整中英文 GUI 人工矩阵；
- 7,200 秒综合长稳；
- 对候选 ZIP 的匿名源码克隆重建；
- OmniPack 授权公开远端、`v1.0.0` tag 和正式 Release；
- Authenticode 签名。当前候选未签名。

发现问题时请记录候选 ZIP SHA-256、`TEST-MANIFEST.txt` 中的 revision、复现步骤、所用语言/缩放和相关 OPS。不要在报告中附带账号令牌、私人存档或用户目录内容。
