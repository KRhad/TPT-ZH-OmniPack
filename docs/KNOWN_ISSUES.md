# 已知问题

## 发布阻塞

1. 用户已确认当前原生 Fusion Pixel Font 12px 方案的中文显示问题解决；100%/125%/150% DPI、中文/英文往返切换、重启持久化、长文本及全部页面矩阵仍未完成。
2. 便携候选曾因默认 `can_install=auto` 弹出文件关联“安装”提示。当前源码和 clean build 为 `CAN_INSTALL=false`、`INSTALL_CHECK=false`，当前 ZIP 解压 EXE 已从全新目录启动并正常退出；由于本会话没有可信窗口内容捕获，“没有显示安装提示”的人工视觉项仍为 `not_tested`。
3. 官方、四个单模块和四模块混合的真实 OPS stamp 双往返已经自动通过，并覆盖 `LAVA/SPRK/MSCR/CONV/VIRS` 的间接类型字段；本地保存对话框生成 `.cps` 的 GUI 路径仍未实际点击。
4. 禁用模块的正常加载、只读加载、取消、菜单/快捷键/另存/覆盖/上传拦截和退出不覆盖仍缺少可信 GUI 点击证据。
5. 十个固定压力场景和隔离采样工具已建立。最初 2 秒烟测全部能生成 FPS、1% low、工作集、粒子和 OPS 数据；第一次完整样本因 Lua 连续占用触发“脚本无响应”而失败，已在 `4f5c07f9` 改为逐 UI tick 采样并通过响应性烟测。十个 10 分钟样本和两小时长跑仍未全部完成。
6. 稳定新增元素 ID 分区、登记门禁、禁用模块警告和只读加载已实现；旧模组 ID 迁移与缺失元素兼容占位尚未实现。
7. GCC 16 对 `OurVariant/Bson` 和 `PowderToy.cpp` 的 `std::optional<ByteString>` 路径给出 `-Wmaybe-uninitialized`，并对 `Simulation::FloodParts` 给出 `-Warray-bounds` 优化警告；clean build 成功但尚未形成独立根因结论，不能写成已修复。
8. 图鉴内容目前覆盖 48 个已实现的 OmniPack 元素；官方元素继续只显示现有登记说明和热学属性，不能据此推断完整官方工艺百科。
9. 2026-07-30 脱敏扫描在当前进程环境变量发现一个 GitHub classic PAT；Git 历史和已审计 ZIP 未发现该模式，但无法证明旧凭据已经撤销或轮换。因此不得推送、创建 tag 或公开发布，直到用户在外部账户完成可验证的撤销或轮换。
10. 正式发布远端和权限尚未确认；当前 `origin` 指向旧汉化仓库，不能把本地开发分支擅自推送为 OmniPack 正式源码。
11. 当前 Windows Computer Use 运行时不可用；系统 API 可启动和聚焦 SDL 窗口，但不能替代可信的完整 GUI 交互、截图和 DPI 验收。

```text
credential_exposure_found=true
credential_present_in_git_history=false
credential_present_in_release_artifacts=false
credential_revoked=false
credential_rotated=false
secret_scan_pass=false
```

## 历史废弃基线

2026-07-30 的早期 Unifont 转换候选及其第一次结构修复试包均已废弃，只用于回归对照。当前产物以 `c743db2f` 的原生 Fusion 12px 字体实现及后续 `development/omnipack-1.0` 提交为准；历史 EXE/ZIP 哈希不得用于当前发布报告。

## 来源限制

- Seppo 公共仓库登记的 32 个新增元素实现文件全部缺失；不得从仅有二进制的版本复制实现。
- Cracker、SpikeViper、Ultimata、Seppo、Cyens 的旧自定义 ID 与官方 TPT 100 尾部元素冲突。
- Alchemy 旧进度按数组下标保存在 `powder.pref`，存在确定越界路径且不能直接复用。
- Cyens 的烃系统重定义官方 GAS/OIL/WAX 等语义，不能在兼容模式下原样覆盖。
- Ultimata 的时间、电磁、车辆和全局物理修改跨越模拟核心，需逐项重写和预算测试。

## 当前官方上游

审计日官方 master 仍有公开的崩溃/越界报告，包括 #1091、#1089、#1087、#1086、#1083。后续同步上游不能替代本项目自身的输入边界和回归测试。

## 非问题但需明确

- Phase 0 没有新增元素、可执行文件或发布包。
- Phase 0 的存档兼容表是静态判定，不是运行验证结果。
- “键数相同”不代表完全汉化。
- 字体来源和许可证已由 `docs/FONT_AUDIT.md` 解决；这不替代最终 ZIP 中的视觉与缩放检查。
- Phase 2 的六个内容模块开关只提供统一选择框架；四个计划中设置在 UI 中明确禁用，未声称功能已实现。
- 模块关闭不会删除已有粒子。加载含已关闭模块元素的存档时可选择正常或只读加载；兼容占位转换尚未实现，不能静默删除、转换或重映射粒子。
