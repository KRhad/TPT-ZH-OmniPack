# TPT-ZH-OmniPack 0.1.0-test 最终验收报告

本报告记录已拒绝的候选，不再代表可发布版本。用户在真实 Windows 桌面确认中文界面严重异常后，候选源码提交 `5828a97fc39129547354956dde84d7b6cfb818c2` 及其 ZIP 已于 2026-07-30 废弃。旧 ZIP 保留为失败基线，禁止作为最终候选、tag 或公开发布依据。

## 交付物

| 文件 | SHA-256 | 状态 |
|---|---|---|
| `TPT-ZH-OmniPack-0.1.0-test-Windows-x64.zip` | `DC8211AC5F4590DA74231D922FCCC0168933D6DCF7AB82AA1D369CE2E97B0189` | 已拒绝失败基线，`zh_ui_failure` |
| `TPT-ZH-OmniPack-0.1.0-test-Symbols-Windows-x64.zip` | `3EDD20947C0D96BFD4675938B7FAE599D5F9DBA8E3F99D388C33854B329B33F9` | 已拒绝失败基线，`zh_ui_failure` |

此前候选的普通 ZIP `8266231A3DE12D504706B3174D01570456DAC3948C3C7AB0187949EAADE05DBD` 与符号 ZIP `296794B51D11ACD73198CE96EE300C61320F8E282B9307F2EF03AF4A2D31CF1B` 仅作为基线归档，不得作为本候选引用。

## 拒绝原因

- 中文字体转换器把 2bpp 像素按高位优先写入，而 `FontReader::NextPixel()` 按低位优先读取，导致四像素组内横向顺序错误。
- 16x16 到 12 高度的整数点采样跳过部分源行，会把单像素笔画转换为空白，例如 `U+4E00`。
- 修复工作位于 `fix/zh-ui-crash`；旧 ZIP 的来源提交和哈希不得继续被称为候选。

## 历史证据摘要

- clean Release build、Meson `10/10`、Python `50`、最终 ZIP EXE 的 Lua 运行回归 `6/6` 均已通过。
- ZIP 白名单、内部清单、分离调试符号、开发路径清理、静态运行库和 PE ASLR/DEP/high-entropy 标志均已审计通过。
- 最终 ZIP 解压后 EXE 的路径、SHA-256、窗口标题、非零句柄和 `Responding=True` 已取得；并使用独立用户数据目录。
- 脱敏扫描未在 Git 可达历史或当前发布 ZIP 找到 GitHub PAT 模式。初次本轮 Meson 测试继承 PAT 后生成的两份忽略测试日志已在复扫发现后删除，并在移除 PAT 的环境重跑 Meson `10/10`；当前环境仍存在 `GITHUB_PAT_TOKEN`，且无撤销或轮换的可验证证据。
- `origin` 不存在两个发布分支或目标 tag；凭据门禁失败前未推送，未进行匿名克隆。
- SDL 窗口无法获得前景，`PrintWindow` 仅获黑帧，故没有可信真实 GUI、OPS、只读门禁或压力测试证据。

## 私有修复候选

修复源码提交为 `ca3cccbee13a41c37ee0b7975c4b5f060cb34a95`。该候选仅生成在 `artifacts/zh-ui-fix/candidate/` 供中文人工验收，未公开、未推送、未创建 tag。普通 ZIP SHA-256 为 `E52E746BF925B2096ED93D659E54A52876179230D29D9534D4E579EB755E4081`，符号 ZIP SHA-256 为 `AAEDCAC3F3EBF16A967D29C110C4935C7A4A6396C46403C67A5C1E53D40CE0E0`。两包 ZIP 内容审计通过，但人工视觉和语言切换门禁未完成，`release_ready=false`。

完整证据、环境信息、失败模式和最小下一步见 `docs/FINAL_VALIDATION.md` 与 `artifacts/final-validation/`。

## 机器可读结论

```text
source_commit=ca3cccbee13a41c37ee0b7975c4b5f060cb34a95
candidate_version=0.1.0-test
rejected_candidate_commit=5828a97fc39129547354956dde84d7b6cfb818c2
rejected_public_zip_sha256=DC8211AC5F4590DA74231D922FCCC0168933D6DCF7AB82AA1D369CE2E97B0189
rejected_symbols_zip_sha256=3EDD20947C0D96BFD4675938B7FAE599D5F9DBA8E3F99D388C33854B329B33F9
rejection_reason=zh_ui_failure
release_tag=not_tested
version=0.1.0-test

credential_exposure_found=true
credential_present_in_git_history=false
credential_present_in_release_artifacts=false
credential_revoked=false
credential_rotated=false
secret_scan_pass=false

source_branch_public=false
source_commit_public=false
anonymous_clone_pass=false
public_clone_commit=not_tested
public_clone_build_pass=not_tested

clean_build_pass=true
meson_tests=10/10
python_tests=50
lua_runtime_tests=6/6

gui_launch_test=true
zh_en_switch_test=false
ui_text_overflow_test=not_tested
font_visual_test=false
module_ui_test=not_tested
representative_element_test=not_tested

official_ops_roundtrip_test=not_tested
metallurgy_ops_roundtrip_test=not_tested
ecology_ops_roundtrip_test=not_tested
chemistry_ops_roundtrip_test=not_tested
nuclear_ops_roundtrip_test=not_tested
mixed_ops_roundtrip_test=not_tested
ops_roundtrip_test=not_tested

disabled_module_dialog_test=not_tested
normal_load_test=not_tested
readonly_load_test=not_tested
cancel_load_test=not_tested
readonly_save_block_test=not_tested
readonly_upload_block_test=not_tested
indirect_element_detection_test=not_tested

representative_gameplay_test=not_tested
stress_samples_passed=0
stress_samples_total=10
unbounded_growth_detected=not_tested
memory_leak_suspected=not_tested
stress_test=not_tested

font_license_resolved=true
release_exe_stripped=true
debug_symbols_separated=true
developer_paths_removed=true
pe_security_flags_preserved=true
authenticode_signed=false

public_zip_sha256=E52E746BF925B2096ED93D659E54A52876179230D29D9534D4E579EB755E4081
symbols_zip_sha256=AAEDCAC3F3EBF16A967D29C110C4935C7A4A6396C46403C67A5C1E53D40CE0E0
zip_audit_pass=true

tag_public=false
github_release_created=false
release_ready=false
```

## 阻塞项

1. **安全/外部账户：**PAT 尚未取得撤销或轮换证据；停止推送、tag 和 Release。下一步是在 GitHub 外部安全设置撤销旧凭据并安全重新认证。
2. **权限/外部服务：**发布分支未公开，不能进行匿名克隆。下一步是在安全门禁通过后推送并用无凭据 HTTPS 克隆验证。
3. **环境：**当前会话无法安全控制或捕获 SDL GUI 内容。下一步是在交互式 Windows 桌面使用最终 ZIP 完成语言、模块、OPS、只读门禁和十个固定压力样本。
