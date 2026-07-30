# TPT-ZH-OmniPack 0.1.0-test 候选报告

本报告绑定当前本地候选 `4f5c07f9243b2ad04c8dbeb8b9c1887d9812c60a`。它不是公开发布报告：人工 GUI、完整压力、凭据撤销、公开源码、匿名克隆、tag 和 GitHub prerelease 尚未通过，因此 `release_ready=false`。

## 当前产物

| 产物 | SHA-256 | 状态 |
|---|---|---|
| `TPT-ZH-OmniPack-0.1.0-test-Windows-x64.zip` | `0DF8695EE9D28D61C7F076EF199831BA953117B632043948E85AF6A3BBACB051` | 本地候选；白名单、清单、哈希和解压二审通过 |
| `TPT-ZH-OmniPack-0.1.0-test-Symbols-Windows-x64.zip` | `4F3645DFD664B3DE2BB0ADDD0FE107037607F4DEE7BBF4DAF5C97D6521A400A1` | 本地符号候选；审计通过 |
| `tpt-zh-omnipack.exe` | `D29E67762E3A6C592E84B2FF3D5FAB47958C7BB79336D3D2C9AE3CCDC9C5BFB2` | 已剥离；未签名 |
| `tpt-zh-omnipack.debug` | `42D96F23C3702EE96FBEE5CF961582B7A1423FA1ABC6A73D617066A8A5E66364` | 与普通包分离 |
| `resources/font.bz2` | `47F4EB851ABFC4CABDFC780E3D427ECBA39077418324A4E291CCDE552F0C139D` | Fusion Pixel Font 原生 12px；许可证与覆盖审计通过 |

普通 ZIP 有 15 个白名单成员，未压缩总大小 17,774,104 bytes；符号 ZIP 有 2 个成员。普通包不包含 `.cps`、`.stm`、`.pref`、Lua、账户、图章、个人存档或调试符号。

## 当前证据

- 空目录 Windows x64 Release build：`502/502`，0 error；二进制构建提交为仅早于压力工具修复的 `e2e1b3fe81082350b5e4919a2b8e43dac29ef090`，`4f5c07f9` 不改变二进制源码。
- Meson：`13/13`；Python：`77/77`，0 skip。
- 已剥离 EXE 的模块、冶金、生态、化学、核工业和混合 OPS Lua 运行回归：`6/6`。
- 官方、冶金、生态、化学、核工业独立 OPS：`5/5`，15 个进程、10 次重启、10 次加载验证、79 粒子、每次加载合计 120 字段断言。四模块混合 OPS 另用 3 个进程完成双往返。
- OPS 覆盖 `LAVA.ctype`、`SPRK.ctype`、`MSCR.ctype`、`CONV.ctype/tmp`、`VIRS.tmp2`，并检查 OPS1、BZip2 和 palette identifier。
- ZIP 解压 EXE 在全新隔离 `ddir` 实际启动，窗口标题正确、句柄非零、`Responding=true`，正常退出；这不是窗口内容或安装提示的人工视觉证据。
- 发布 EXE 无 `.debug*` 段、无开发路径标记、无动态 GCC 开发运行库，保留 `DYNAMIC_BASE`、`NX_COMPAT` 和 `HIGH_ENTROPY_VA`；Authenticode 状态为 `NotSigned`。
- 用户确认当前原生 Fusion 12px 中文显示问题已解决；英文界面、双向语言切换、重启持久化和 100%/125%/150% DPI 尚未完整验收。
- 十个固定压力场景和原始数据格式已实现。第一次完整运行暴露阻塞式 Lua 的脚本无响应并失败；`4f5c07f9` 已改为逐 UI tick 返回。S01/S02 随后各完成 60+600 秒；平均 FPS 均约 60，峰值工作集分别为 152,281,088/162,676,736 bytes，无崩溃/卡死且 OPS 往返通过。只读判定器对两项均报告有限观察期内 `unbounded_growth=false`、`memory_leak_suspected=false`、`sample_execution_pass=true`；事件计数、场景行为和其余 8 个样本尚未齐全。

## 明确废弃的历史候选

以下仅为失败回归基线，不是当前产物，不得用于当前下载或 tag：

| 源码提交 | 普通 ZIP SHA-256 | 废弃原因 |
|---|---|---|
| `5828a97fc39129547354956dde84d7b6cfb818c2` | `DC8211AC5F4590DA74231D922FCCC0168933D6DCF7AB82AA1D369CE2E97B0189` | 中文字形损坏 |
| `ca3cccbee13a41c37ee0b7975c4b5f060cb34a95` | `E52E746BF925B2096ED93D659E54A52876179230D29D9534D4E579EB755E4081` | 中文可读性/字形质量失败 |

## 机器可读结论

```text
source_commit=4f5c07f9243b2ad04c8dbeb8b9c1887d9812c60a
binary_build_commit=e2e1b3fe81082350b5e4919a2b8e43dac29ef090
release_tag=not_tested
version=0.1.0-test
upstream_version=100.0.399

clean_build_pass=true
clean_build_targets=502/502
meson_tests=13/13
python_tests=77/77
python_test_skips=0
lua_runtime_tests=6/6

zh_gui_test=true
en_gui_test=not_tested
font_visual_test=true
language_switch_test=not_tested
language_persistence_test=not_tested
dpi_100_test=not_tested
dpi_125_test=not_tested
dpi_150_test=not_tested
module_ui_test=not_tested

ops_roundtrip_test=true
official_ops_roundtrip_test=true
metallurgy_ops_roundtrip_test=true
ecology_ops_roundtrip_test=true
chemistry_ops_roundtrip_test=true
nuclear_ops_roundtrip_test=true
mixed_ops_roundtrip_test=true
ops_case_processes=18
ops_case_restarts=12
ops_case_load_verifications=12
disabled_module_dialog_test=not_tested
readonly_save_block_test=not_tested
readonly_upload_block_test=not_tested
save_migration_test=not_tested
gui_cps_save_test=not_tested

reaction_tests=38/38
automation_tests=not_tested
alchemy_progression_tests=not_tested
challenge_tests=not_tested
stress_harness_test=true
stress_samples_executed=2
stress_sample_executions_passed=2
stress_samples_passed=0
stress_samples_total=10
stress_test=not_tested
long_run_test=not_tested

font_license_resolved=true
third_party_license_audit=true
secret_scan_pass=false
credential_exposure_found=true
credential_revoked=false
credential_rotated=false
source_commit_public=false
anonymous_clone_pass=false

release_exe_sha256=D29E67762E3A6C592E84B2FF3D5FAB47958C7BB79336D3D2C9AE3CCDC9C5BFB2
debug_symbols_sha256=42D96F23C3702EE96FBEE5CF961582B7A1423FA1ABC6A73D617066A8A5E66364
release_exe_stripped=true
debug_symbols_separated=true
developer_paths_removed=true
pe_security_flags_preserved=true
authenticode_signed=false

public_zip_sha256=0DF8695EE9D28D61C7F076EF199831BA953117B632043948E85AF6A3BBACB051
symbols_zip_sha256=4F3645DFD664B3DE2BB0ADDD0FE107037607F4DEE7BBF4DAF5C97D6521A400A1
source_zip_sha256=not_tested
zip_audit_pass=true

github_release_created=false
release_ready=false
```

## 当前硬阻塞

1. 外部账户：已暴露 PAT 没有撤销或轮换证据，`secret_scan_pass=false`。
2. 外部权限：没有经授权的 OmniPack 发布远端，源码未公开，匿名克隆、tag 和 GitHub prerelease 未执行。
3. GUI：本会话没有可用的可信 Windows Computer Use 会话，不能完成语言、DPI、四模块、禁用模块三选项与保存/上传拦截的点击证据。
4. 稳定性：十个 10 分钟压力样本与后续两小时长跑未全部完成。
