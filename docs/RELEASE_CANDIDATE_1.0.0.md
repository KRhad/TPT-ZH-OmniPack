# TPT-ZH-OmniPack 1.0.0-rc6 本地发布候选说明

这是 Windows x64 的 1.0.0-rc6 本地发布候选，不是正式发布。它用于把冻结内容、静态剥离二进制、许可证、存档、压力和长稳门禁绑定到同一个可审计 ZIP；在全部门禁完成前保持：

```text
release_ready=false
```

本项目是以大量元素、化合物和材料为核心的自由沙盒整合版。项目不包含玩家任务、成就、挑战、科技树、炼金进度或强制元素解锁，所有已启用内容均可直接使用。

## 冻结内容口径

```text
periodic_table_elements=118/118
active_non_alias_types=487
creatable_types=484
directly_selectable_menu_materials=466
reaction_registry_entries=328
highest_registered_stable_id=685
element_capacity=1024
```

三个数量含义不同：487 是活动非别名类型；其中 484 个可由引擎成功创建；其中 466 个是普通菜单可直接选择的材料。其余是擦除工具或官方隐藏过渡/辅助类型，不能冒充菜单材料。

## 运行方式

1. 解压完整 ZIP，不要只复制 EXE。
2. 运行 `tpt-zh-omnipack.exe`。
3. 默认简体中文，可在设置中切换英文。
4. 周期表图标和元素搜索按钮位于最右侧竖列；周期表也可按 `T` 打开。

## 本候选仍未关闭的正式发布门禁

- 125%/150% DPI 与完整中英文 GUI 人工矩阵；
- 全内容 S15-S20 的正式 60 秒预热和 600 秒采样；
- 7,200 秒综合长稳；
- 对候选 ZIP 的匿名源码克隆重建；
- OmniPack 授权公开远端、`v1.0.0` tag 和正式 Release；
- Authenticode 签名。当前候选未签名。

发现问题时请记录候选 ZIP SHA-256、`TEST-MANIFEST.txt` 中的 revision、复现步骤、所用语言/缩放和相关 OPS。不要在报告中附带账号令牌、私人存档或用户目录内容。
