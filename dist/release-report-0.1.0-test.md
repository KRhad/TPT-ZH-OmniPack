# TPT-ZH-OmniPack 0.1.0-test 最终验收报告

本报告对应最终候选源码提交 `784525867567a801fc0d37224eb36041ba78d553`，其 ZIP 内 `TEST-MANIFEST.txt` 已绑定相同提交。此候选未公开发布；所有未完成实际交互门禁均明确保持未测试。

## 交付物

| 文件 | SHA-256 | 状态 |
|---|---|---|
| `TPT-ZH-OmniPack-0.1.0-test-Windows-x64.zip` | `E697338844474CC90C89863B8C93165F5CC39D3CEADE41C8D336EF67118F861C` | ZIP 内容审计通过，未公开 |
| `TPT-ZH-OmniPack-0.1.0-test-Symbols-Windows-x64.zip` | `602032D5739B38ECB4B2499CAD0F0066F9F253B090A3F4D69AD02545343E9D55` | ZIP 内容审计通过，未公开 |

此前候选的普通 ZIP `8266231A3DE12D504706B3174D01570456DAC3948C3C7AB0187949EAADE05DBD` 与符号 ZIP `296794B51D11ACD73198CE96EE300C61320F8E282B9307F2EF03AF4A2D31CF1B` 仅作为基线归档，不得作为本候选引用。

## 证据摘要

- clean Release build、Meson `10/10`、Python `50`、最终 ZIP EXE 的 Lua 运行回归 `6/6` 均已通过。
- ZIP 白名单、内部清单、分离调试符号、开发路径清理、静态运行库和 PE ASLR/DEP/high-entropy 标志均已审计通过。
- 最终 ZIP 解压后 EXE 的路径、SHA-256、窗口标题、非零句柄和 `Responding=True` 已取得；并使用独立用户数据目录。
- 脱敏扫描未在 Git 可达历史或当前发布 ZIP 找到 GitHub PAT 模式。初次本轮 Meson 测试继承 PAT 后生成的两份忽略测试日志已在复扫发现后删除，并在移除 PAT 的环境重跑 Meson `10/10`；当前环境仍存在 `GITHUB_PAT_TOKEN`，且无撤销或轮换的可验证证据。
- `origin` 不存在两个发布分支或目标 tag；凭据门禁失败前未推送，未进行匿名克隆。
- SDL 窗口无法获得前景，`PrintWindow` 仅获黑帧，故没有可信真实 GUI、OPS、只读门禁或压力测试证据。

完整证据、环境信息、失败模式和最小下一步见 `docs/FINAL_VALIDATION.md` 与 `artifacts/final-validation/`。

## 机器可读结论

```text
source_commit=784525867567a801fc0d37224eb36041ba78d553
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
zh_en_switch_test=not_tested
ui_text_overflow_test=not_tested
font_visual_test=not_tested
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

public_zip_sha256=E697338844474CC90C89863B8C93165F5CC39D3CEADE41C8D336EF67118F861C
symbols_zip_sha256=602032D5739B38ECB4B2499CAD0F0066F9F253B090A3F4D69AD02545343E9D55
zip_audit_pass=true

tag_public=false
github_release_created=false
release_ready=false
```

## 阻塞项

1. **安全/外部账户：**PAT 尚未取得撤销或轮换证据；停止推送、tag 和 Release。下一步是在 GitHub 外部安全设置撤销旧凭据并安全重新认证。
2. **权限/外部服务：**发布分支未公开，不能进行匿名克隆。下一步是在安全门禁通过后推送并用无凭据 HTTPS 克隆验证。
3. **环境：**当前会话无法安全控制或捕获 SDL GUI 内容。下一步是在交互式 Windows 桌面使用最终 ZIP 完成语言、模块、OPS、只读门禁和十个固定压力样本。
