# TPT-ZH-OmniPack 0.1.0-test 候选报告

本报告绑定当前本地候选 `ff5945c4acbe15052a316771934854aa0f9281de` 及其重新封装的普通包。十个固定压力样本的事件、停止/恢复和有限观察门禁现已全部通过；报告仍不是公开发布报告，因为可信 GUI 矩阵、凭据处置、授权公开源码、匿名克隆、发布 tag/Release 和两小时长跑尚未完成，因此 `release_ready=false`。

## 当前产物

| 产物 | SHA-256 | 状态 |
|---|---|---|
| `TPT-ZH-OmniPack-0.1.0-test-Windows-x64.zip` | `53E0304FF8CE932F7D836620A7599085A486B1689EAC131BC78D7B8EA6619827` | 当前本地候选；白名单、清单、成员哈希、ZIP 哈希和解压二审通过 |
| `TPT-ZH-OmniPack-0.1.0-test-Symbols-Windows-x64.zip` | `8FD702E9F9B92E34321226340F9EF3742EFA86FE8C0FA98302CD6CECAAACE48D` | 当前本地符号包；与普通包分离，审计通过 |
| `tpt-zh-omnipack.exe` | `14A00CCF73D5100C43D677572529F6DDCD9A2790FC16FED70136185262B46926` | 已剥离；未签名 |
| `tpt-zh-omnipack.debug` | `17CE34385D9F27A610A591E3F76D6E61D9044B02D5791784563F4B5FDEBC7871` | detached symbols；不在普通包 |
| `resources/font.bz2` | `47F4EB851ABFC4CABDFC780E3D427ECBA39077418324A4E291CCDE552F0C139D` | Fusion Pixel Font 原生 12px；许可证和覆盖审计通过 |

普通 ZIP 有 15 个白名单成员，未包含 `.cps`、`.stm`、`.pref`、Lua、账户、图章、个人存档或调试符号；符号 ZIP 仅用于崩溃分析。包内 `TEST-MANIFEST.txt` 的 `revision`、EXE 大小和成员哈希均与上述候选一致。

## 当前证据

- `build-0.1.0-test-metrics` 的 Windows x64 Release clean build 已完成；当前候选 Meson `14/14`、Python `87/87`（0 skip），最终 ZIP Lua 模块/反应回归 `6/6`。
- 官方、四个单模块和四模块混合 OPS 已完成隔离双往返；覆盖 `LAVA.ctype`、`SPRK.ctype`、`MSCR.ctype`、`CONV.ctype/tmp`、`VIRS.tmp2` 等间接字段。
- 当前普通 ZIP 解压到 `artifacts/development-1.0/extracted-ff5945c4-53e0304f` 后可启动：标题正确、窗口句柄非零、进程响应、正常退出码 `0`。这只是进程证据，不是窗口内容、安装提示或 DPI 的人工视觉证据。
- EXE 无 `.debug*` 段、无开发路径标记、无动态 GCC 开发运行库，保留 `DYNAMIC_BASE`、`NX_COMPAT` 和 `HIGH_ENTROPY_VA`；Authenticode 状态为 `NotSigned`。
- 用户已确认当前原生 Fusion 12px 方案的中文可读性；英文切换、双向语言持久化、100%/125%/150% DPI、四模块页面和只读/上传点击矩阵仍为 `not_tested`。

### 十个正式压力样本

每项均使用 60 秒预热 + 600 秒采样，且 `source_commit`、`harness_commit`、普通 ZIP 和 EXE 完全一致。每份目录保留 `result.json`、逐秒帧/进程序列、两份 OPS、`assessment.json` 和 SHA-256。

| 样本 | Run ID | 事件总数 / 单帧峰值 | 停止/恢复 | 有限观察 | 门禁 |
|---|---|---:|---|---|---|
| S01 | `20260730T180440Z-784d734c` | 1557 / 243 | true / true | 无界增长=false；泄漏嫌疑=false | true |
| S02 | `20260730T181609Z-1519fcf4` | 0 / 0 | true / true | 无界增长=false；泄漏嫌疑=false | true |
| S03 | `20260730T182740Z-1b06cb19` | 4449 / 693 | true / true | 无界增长=false；泄漏嫌疑=false | true |
| S04 | `20260730T183905Z-2586b0be` | 6541 / 1024 | true / true | 无界增长=false；泄漏嫌疑=false | true |
| S05 | `20260730T185034Z-fabbdd4c` | 229 / 8 | true / true | 无界增长=false；泄漏嫌疑=false | true |
| S06 | `20260730T190158Z-7a7786ba` | 2402 / 512 | true / true | 无界增长=false；泄漏嫌疑=false | true |
| S07 | `20260730T191326Z-f68af76a` | 2048 / 512 | true / true | 无界增长=false；泄漏嫌疑=false | true |
| S08 | `20260730T192453Z-9131600a` | 2048 / 512 | true / true | 无界增长=false；泄漏嫌疑=false | true |
| S09 | `20260730T193619Z-d9f707f9` | 3255 / 525 | true / true | 无界增长=false；泄漏嫌疑=false | true |
| S10 | `20260730T194745Z-2bd2d539` | 0 / 0 | true / true | 无界增长=false；泄漏嫌疑=false | true |

聚合断言：`10/10` 样本通过，事件总数 `22529`，最大单帧事件峰值 `1024`，每项 `scenario_recovery_assertions=7`、`stop_event_delta=0`。S02/S10 的零事件是被记录的数值，不是缺失字段。有限观察规则不构成长期有界性的数学证明；两小时长跑仍未执行。

## 明确废弃的历史候选

以下仅为失败回归基线，不是当前产物，不得用于当前下载或 tag：

| 源码提交 | 普通 ZIP SHA-256 | 废弃原因 |
|---|---|---|
| `5828a97fc39129547354956dde84d7b6cfb818c2` | `DC8211AC5F4590DA74231D922FCCC0168933D6DCF7AB82AA1D369CE2E97B0189` | 中文字形损坏 |
| `ca3cccbee13a41c37ee0b7975c4b5f060cb34a95` | `E52E746BF925B2096ED93D659E54A52876179230D29D9534D4E579EB755E4081` | 中文可读性/字形质量失败 |

## 机器可读结论

```text
source_commit=ff5945c4acbe15052a316771934854aa0f9281de
harness_commit=ff5945c4acbe15052a316771934854aa0f9281de
binary_build_commit=ff5945c4acbe15052a316771934854aa0f9281de
release_tag=not_tested
version=0.1.0-test
upstream_version=100.0.399

clean_build_pass=true
clean_build_targets=not_tested
meson_tests=14/14
python_tests=87/87
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
stress_samples_executed=10
stress_sample_executions_passed=10
stress_samples_passed=10
stress_samples_total=10
stress_event_total=22529
stress_peak_event_per_frame=1024
stress_test=true
long_run_test=not_tested

font_license_resolved=true
third_party_license_audit=true
secret_scan_pass=false
credential_exposure_found=true
credential_revoked=false
credential_rotated=false
source_commit_public=false
anonymous_clone_pass=false

release_exe_sha256=14A00CCF73D5100C43D677572529F6DDCD9A2790FC16FED70136185262B46926
debug_symbols_sha256=17CE34385D9F27A610A591E3F76D6E61D9044B02D5791784563F4B5FDEBC7871
release_exe_stripped=true
debug_symbols_separated=true
developer_paths_removed=true
pe_security_flags_preserved=true
authenticode_signed=false

public_zip_sha256=53E0304FF8CE932F7D836620A7599085A486B1689EAC131BC78D7B8EA6619827
symbols_zip_sha256=8FD702E9F9B92E34321226340F9EF3742EFA86FE8C0FA98302CD6CECAAACE48D
source_zip_sha256=not_tested
zip_audit_pass=true

github_release_created=false
release_ready=false
```

## 当前硬阻塞

1. 外部账户：当前环境仍有 PAT 暴露证据，未取得撤销或轮换证明，`secret_scan_pass=false`。
2. 外部权限：没有经授权的 OmniPack 发布远端；源码未公开，匿名克隆、tag 和 GitHub prerelease 未执行。
3. GUI：没有可信 Windows Computer Use 会话，不能把进程响应当作语言、DPI、模块、禁用模块或上传拦截的视觉/交互证据。
4. 稳定性：十个正式 10 分钟样本已通过本版本性能门禁，但两小时长跑和更高版本的长期稳定性仍未测试。
