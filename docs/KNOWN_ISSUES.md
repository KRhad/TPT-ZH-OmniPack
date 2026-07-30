# 已知问题

## 发布阻塞

1. 尚未完成最终 ZIP 的视觉中文布局与英文切换交互测试。
2. 尚未执行官方存档载入/重存基线和四模块 OPS 往返。
3. 稳定新增元素 ID 分区、登记门禁、禁用模块警告和只读加载已实现；旧模组 ID 迁移与缺失元素兼容占位尚未实现。
4. 最终 ZIP 的只读上传拦截仍缺少实际点击证据。
5. GCC 16 对 `PowderToy.cpp` 的 `std::optional<ByteString>` 路径给出 2 个 `-Wmaybe-uninitialized`，待定位。
6. 图鉴内容目前覆盖 48 个已实现的 OmniPack 元素；官方元素继续只显示现有登记说明和热学属性，不能据此推断完整官方工艺百科。
7. 曾发现一个开发期 Meson 原始测试日志记录明文 GitHub PAT。该日志已删除且从未提交；凭据仍必须在仓库外轮换。后续测试已清理敏感环境并通过令牌模式扫描。
8. 当前 Windows x64 ZIP 是未公开发布候选；其字体来源、许可证、白名单打包和符号分离已自动验证，但人工 UI、OPS 往返和性能验证仍是发布阻塞。
9. 2026-07-30 最终验收扫描在当前进程环境变量发现一个 GitHub classic PAT；扫描报告仅保留变量名和脱敏指纹。Git 历史和候选 ZIP 未发现该模式，但无法证明旧凭据已经撤销或轮换。因此不得推送、创建 tag 或公开发布，直到在仓库外完成可验证的撤销。
10. 当前 Windows 自动化会话无法将最终 ZIP 的 SDL 窗口置为前景，且 `PrintWindow` 只能获取黑色客户端帧。语言切换、模块 UI、OPS 三选项/只读保存/上传和压力测试均未执行，不得声称通过。

```text
credential_exposure_found=true
credential_present_in_git_history=false
credential_present_in_release_artifacts=false
credential_revoked=false
credential_rotated=false
secret_scan_pass=false
```

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
