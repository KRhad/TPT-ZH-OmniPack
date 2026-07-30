# 0.1.0-test 最终公开测试验收

验收分支原为 `release/test-public-final-validation`；中文修复工作从 2026-07-30 起在 `fix/zh-ui-crash` 进行。旧候选已经废弃。本文件只记录实际取得的证据；未取得可信 GUI、OPS 或性能数据的项目不得由静态分析、编译或 Lua 回归替代。

## 冻结与基线

- 初始工作树干净；请求指定的 `0e9ff54c65c64f01e3af567366d2eed1de2c5dd9` 是当前分支的祖先。
- 开始验收时 HEAD 已包含后续干净提交 `e2e965ad` 和 `1d490072`，没有回退或覆盖它们。
- 被拒绝候选构建提交为 `5828a97fc39129547354956dde84d7b6cfb818c2`；旧候选 ZIP 已保存在 `artifacts/final-validation/baseline-dist/`，原始 `dist/` 文件未删除。用户报告真实 Windows 中文界面严重异常后，该候选不再有效。失败证据、基线文件和修复日志位于 `artifacts/zh-ui-fix/`。

## 中文故障处置

- `zh_failure_type=glyph_corruption`
- `zh_failure_reproduced=true`
- `zh_failure_root_cause=Unifont converter packed 2bpp pixels MSB-first while FontReader::NextPixel consumes LSB-first; integer 16-to-12 point sampling also skipped source stroke rows`
- `zh_process_crash=false`：修复候选在全新隔离用户目录默认中文启动 20 次，均存活并响应；旧候选没有可用 WER、转储或异常代码，不能补造崩溃记录。
- `baseline_zh_glyph_corruption=true`：旧候选字体确定存在上述位序和截笔错误。
- `rejected_fix_zh_glyph_corruption=true`：转换器修复试包的离屏引擎输出和结构检查通过，但用户从 ZIP 解压运行后明确判定其中文显示仍不如既有出版中文版本；该结论记录为中文可读性/字形质量失败，不推断进程崩溃。
- `zh_glyph_corruption=not_tested`：当前原生 Fusion 12px BDF 方案已通过固定矩阵、全目录覆盖和引擎离屏渲染，但新的私有 ZIP 尚未由用户在真实桌面查看。
- `zh_layout_failure=not_tested`
- `zh_locale_loading_failure=false`：严格 JSON/键/占位符审计及引擎对 `zh-CN.json` 1,262 条文本的测量/绘制通过，替换字形 0；真实 GUI 切换生命周期仍待验证。
- `rejected_fix_font_visual_readability_valid=false`：提交 `ca3cccbee13a41c37ee0b7975c4b5f060cb34a95`、ZIP `E52E746BF925B2096ED93D659E54A52876179230D29D9534D4E579EB755E4081` 已由用户解压运行并人工查看，中文显示质量未达到既有出版中文基线。该私有试包已拒绝。
- `font_visual_readability_valid=not_tested`：新的原生 12px 方案仍需从私有 ZIP 解压运行后人工验收。

## 凭据检查

2026-07-30 的脱敏扫描覆盖工作区、候选 ZIP、所有分支/tag/reflog 可达 Git 对象、PowerShell 历史、CI 配置、临时目录和环境变量。报告位于 `artifacts/final-validation/logs/secret-scan-results.txt`，仅包含位置、类型和不可逆 SHA-256 截断指纹。

- 发现一个当前进程环境变量 `GITHUB_PAT_TOKEN` 中的 GitHub classic PAT；报告不包含其原文。
- Git 可达历史和候选发布包未发现匹配的 GitHub PAT 模式。
- 初次本轮 Meson 测试曾在继承该环境变量后，把同一 PAT 写入两个忽略的 `build-final-validation-release/meson-logs/testlog.*` 文件。复扫发现后立即删除两份日志，并在移除该变量的子进程中重跑 Meson `10/10`；最终复扫确认工作区、可达 Git 对象和候选 ZIP 均无匹配模式。
- 本机没有 `gh`，且没有 GitHub 账户安全管理操作的可验证权限。因此无法证明旧凭据已撤销或已轮换。
- 在旧凭据失效获得可验证证据前，不推送分支、不创建 tag、不创建 GitHub Release。

## 源码公开性

`origin` 是 `https://github.com/Dragonrster/The-Powder-Toy-Chinese.git`。远端引用检查记录于 `artifacts/final-validation/logs/remote-release-ref-check.txt`：

- `release/test-public-hardening` 不存在于远端。
- `release/test-public-final-validation` 不存在于远端。
- `v0.1.0-test` 不存在于远端。

由于凭据门禁失败，未尝试推送，也未进行匿名克隆。公开源码和匿名克隆门禁均不通过。

## 最终 ZIP 运行环境

- OS：Windows 11 Pro x64 `10.0.26200`；系统语言：`zh-CN`。
- 显示器：NVIDIA GeForce RTX 5070 Ti Laptop GPU，`1920x1080`，32-bit。
- DPI：`LogPixels=192`，等价 150% 缩放。
- 最终 ZIP 从 `dist/` 解压到 `artifacts/final-validation/runtime/final-zip/`；EXE SHA-256 与 ZIP 内清单一致。
- 独立数据目录：`artifacts/final-validation/runtime/user-data/`；启动前没有 `powder.pref`，首次启动后才创建该文件。
- EXE 进程路径、命令行、窗口标题、句柄和 `Responding=True` 已记录在 `artifacts/final-validation/logs/process-identity.json` 与 `gui-launch-dpi-fixed.json`。

## GUI 限制

最终 ZIP 的 EXE 可启动，窗口标题为 `TPT-ZH-OmniPack 0.1.0-test` 并保持响应。当前自动化会话无法把 SDL 窗口置为前景：`SetForegroundWindow` 返回 `false`，且目标窗口句柄不等于当前前景句柄，见 `gui-foreground-attempt.json`。`PrintWindow` 返回成功但 SDL 客户端区为黑帧，见 `04-zh-main-printwindow.png`。屏幕区域抓取会包含其他桌面窗口，因此不能作为 TPT UI 证据。

因此，本自动化环境不能安全或可验证地执行所要求的设置点击、语言切换、模块开关、保存加载、只读门禁和十个压力样本。用户已在真实 Windows 桌面对私有修复试包完成中文可读性检查并给出失败结论，但这不等于其余交互、高 DPI 和页面矩阵已执行。没有发送会影响其他前景应用的键鼠输入，也没有伪造 OPS 或性能记录。

## 已完成的非 GUI 验证

- clean Release build：通过。
- Meson 静态测试：`10/10`。
- Python 工具测试：`50`。
- 最终 ZIP EXE 的真实客户端 Lua 回归：`6/6`（模块选择、工业冶金、高级化学、局部生态完整/简化、受控核工业）。
- 本地化审计、存档兼容静态审计和 ZIP 内容审计：通过。
- EXE 调试段拆分、开发路径清理、静态运行库检查和 PE ASLR/DEP/high-entropy 标志：通过。

## 术语审计

- 玩家可见正式模块名称已统一为工业冶金、局部生态、高级化学和受控核工业，分别出现在简中/英文设置标签和发布测试说明。
- `options.omni.biology`、`options.omni.metallurgy` 和 `options.omni.advanced_nuclear` 是内部稳定本地化键，不因显示名称变化而改名。
- `基础化学`、`生物扩展` 等出现在部分 Phase 历史、来源账本或旧来源名称中时属于历史记录，不作为当前玩家可见的模块名称。

## 机器可读结论

```text
source_commit=c743db2fcc49c01033e68023cceff897ed4c35f6
candidate_version=0.1.0-test
private_candidate_public_zip_sha256=943DA2A60C0B371A1D3F921FEC525FB3F7B5AEBC7C5CE7775A8AEFA883C13F14
private_candidate_symbols_zip_sha256=BE14C7D53658963DF1C6B1ECFAA44AC51F8004883530CB7DF922E4282FC11031
private_candidate_exe_sha256=05DACBFCC31D6F1920D4437DB60629A1F9DA79CC0A14AA13138DD97393CCF6AF
private_candidate_zip_audit_pass=true
private_candidate_startup_runs=20
private_candidate_startup_crashes=0
rejected_candidate_commit=5828a97fc39129547354956dde84d7b6cfb818c2
rejected_public_zip_sha256=DC8211AC5F4590DA74231D922FCCC0168933D6DCF7AB82AA1D369CE2E97B0189
rejected_symbols_zip_sha256=3EDD20947C0D96BFD4675938B7FAE599D5F9DBA8E3F99D388C33854B329B33F9
rejection_reason=zh_ui_failure
rejected_fix_candidate_commit=ca3cccbee13a41c37ee0b7975c4b5f060cb34a95
rejected_fix_public_zip_sha256=E52E746BF925B2096ED93D659E54A52876179230D29D9534D4E579EB755E4081
rejected_fix_symbols_zip_sha256=AAEDCAC3F3EBF16A967D29C110C4935C7A4A6396C46403C67A5C1E53D40CE0E0
rejected_fix_reason=zh_font_visual_quality_failure
rejected_fix_zip_audit_pass=true

zh_failure_type=glyph_corruption
zh_failure_reproduced=true
zh_failure_root_cause=Unifont converter packed 2bpp pixels MSB-first while FontReader::NextPixel consumes LSB-first; integer 16-to-12 point sampling skipped source stroke rows
zh_process_crash=false
baseline_zh_glyph_corruption=true
rejected_fix_zh_glyph_corruption=true
zh_glyph_corruption=not_tested
zh_layout_failure=not_tested
zh_locale_loading_failure=false

font_source_verified=true
font_license_verified=true
font_container_valid=true
font_glyph_coverage_valid=true
font_pack_roundtrip_test=true
unifont_source_decode_test=true
font_pixel_conversion_valid=true
font_engine_render_valid=true
zh_known_glyph_test=true
zh_golden_image_test=not_tested
rejected_fix_font_visual_readability_valid=false
font_visual_readability_valid=not_tested
font_dpi_layout_valid=not_tested

zh_first_start_test=true
en_to_zh_switch_test=not_tested
zh_to_en_switch_test=not_tested
zh_persistence_test=not_tested
zh_startup_runs=20
zh_startup_crashes=0

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
meson_tests=12/12
python_tests=56
lua_runtime_tests=6/6

gui_launch_test=true
zh_default_test=not_tested
zh_to_en_switch_test=not_tested
en_persistence_test=not_tested
en_to_zh_switch_test=not_tested
zh_persistence_test=not_tested
zh_en_switch_test=false
ui_text_overflow_test=not_tested
font_visual_test=not_tested
module_ui_test=not_tested
metallurgy_ui_test=not_tested
ecology_ui_test=not_tested
chemistry_ui_test=not_tested
nuclear_ui_test=not_tested
representative_element_test=not_tested
module_state_persistence_test=not_tested

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
readonly_menu_save_block_test=not_tested
readonly_ctrl_s_block_test=not_tested
readonly_save_as_block_test=not_tested
readonly_overwrite_block_test=not_tested
readonly_exit_no_overwrite_test=not_tested
indirect_element_detection_test=not_tested
lava_carrier_detection_test=not_tested
sprk_carrier_detection_test=not_tested
mscr_carrier_detection_test=not_tested

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

public_zip_sha256=not_tested
symbols_zip_sha256=not_tested
zip_audit_pass=not_tested

tag_public=false
tag_anonymous_clone_pass=false
github_release_created=false
release_permission_blocked=true
release_ready=false
```

## 发布阻塞与最小下一步

| 阻塞项 | 类型 | 证据 | 已采取动作 | 最小下一步 |
|---|---|---|---|---|
| PAT 未证明撤销或轮换 | 外部账户/安全 | `secret-scan-results.txt` 发现当前环境 PAT；无撤销证据 | 停止推送、tag 和 Release | 在 GitHub 安全设置撤销旧 PAT，使用最小权限凭据和凭据库重新认证，再记录不含原文的证据 |
| 源码尚未公开 | 权限/外部服务 | 两个发布分支和 tag 在 `origin` 均不存在 | 未推送 | 撤销确认后推送分支，匿名 HTTPS 克隆并构建验证 |
| 私有修复试包中文可读性失败 | 字体/实际 GUI | 用户解压运行 `E52E746B...` 后确认其中文显示仍不如既有出版中文版本 | 拒绝该试包，保留为失败对照，`rejected_fix_font_visual_readability_valid=false` | 已实现原生 Fusion 12px BDF 方案；生成新的私有试包供人工检查 |
| 自动化 SDL GUI 无可验证前景/画面 | 环境 | 前景 API 失败，`PrintWindow` 黑帧 | 保存窗口、截图和失败证据；未盲目发送输入 | 在可交互 Windows 桌面执行其余语言切换、页面、高 DPI、OPS 和压力矩阵 |
| OPS 与压力样本未创建 | 环境 | `saves/` 和 `stress/` 的结果文件明确为未测试 | 未构造伪 OPS 或伪性能数据 | 使用最终 ZIP 的 GUI 保存十个 OPS 样本，完成往返、门禁和十个固定压力运行 |
