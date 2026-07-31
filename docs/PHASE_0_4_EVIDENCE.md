# 0.4.0-dev 本地炼金探索候选证据

本文件记录 `0.4.0-dev` 的本地可复核结果。它不是公开发布报告：炼金进度窗口的可信桌面检查、完整简中/英文与 DPI 矩阵、授权公开源码、匿名克隆、tag 和 GitHub Release 均未完成，因此 `GATE-040-RELEASE=false`、`release_ready=false`。

## 来源与候选绑定

| 对象 | 提交 / 值 | 说明 |
|---|---|---|
| 炼金进度实现 | `00785c133520d1848635d9a9407d9670a2bb2d6c` | 独立模式、十阶段、OPS 进度、统一选择门禁和进度窗口 |
| 连续 OPS 往返门禁 | `1aefae4efb11d6ed1fea7eb66265eedf92eb7afc` | 每轮都执行 `Serialise -> Parse -> Import`，连续 100 次逐字段一致 |
| 0.4 本地包配置 | `3a97b4bf0e99e891490130f5fb5d24cb1e57d911` | 普通包继承 0.2/0.3 内容并封入版本化炼金图 |
| 最终 0.3 自动化内容 | `49f42be8654e62f5180bc7ad848c7435e46f72a6` | 同一 0.4 EXE 的 9/9 场景、6/6 挑战 |
| 包清单 / 最终 0.2 教程内容 | `f42deded7f537c343071854068c2e2c453d56cd4` | 普通包 `TEST-MANIFEST.txt` revision；7/7 示例、8/8 教程 |
| 双语元素自识别说明 | `c75aeaf18103ed6cb6eddc844d80b7a3a87045fc` | 48 个模组元素说明均先显示对应名称 |
| 固定构建时间 | `SOURCE_DATE_EPOCH=1785491929` | clean build、符号分离和 ZIP 两次封包使用同一输入 |

## 模式、图与持久化结论

- `docs/ALCHEMY_PROGRESSION.json` 使用 schema 1，初始集合严格为 `FIRE/WATR/STNE/O2`，十个阶段全部使用稳定 identifier，不保存数字元素 ID。
- 图审计证明所有阶段输入按顺序可达、无前向依赖或循环死锁，并精确覆盖温度、压力、电流、催化、时间、结构、冷却、过滤和多阶段九类条件。
- 普通沙盒默认完全自由；只有显式启用炼金模式才限制选择。模块关闭与炼金未解锁使用不同限制类型。
- 进度保存在 OPS 的严格 `omniAlchemy` 对象中；未知 schema、损坏字段、额外键、超量或不一致记录均 fail-closed 回到四初始元素，且不改写来源存档。
- 搜索、收藏、活动工具、UI 粘贴、普通 Lua 粒子/工具接口和 stamp 加载全部经过同一门禁。实际锁定模式 11 条断言、自由模式 10 条断言、stamp 夹具 1 条断言均通过。
- 模块开启与关闭各完成十阶段真实客户端运行；每种模式 2,310 帧，阶段帧数为 `120,60,180,120,240,300,30,270,360,630`。模块关闭时进度仍完成并保存隐藏解锁，重新启用模块后才允许选择。
- 状态探针先验证多存档隔离、损坏状态和未知 schema，再对精通状态连续执行 100 次真实 `Serialise -> Parse -> Import`。每轮逐字段比较解锁集合、阶段、记录、dwell 和 cooling 状态，无丢失、重复或死局。该结果满足 `GATE-040-LONGRUN` 的反复存取要求，但不等于 `GATE-100-LONGRUN` 的两小时综合运行。

## 构建与自动回归

| 门禁 | 结果 | 说明 |
|---|---|---|
| 全新 Windows x64 Release 构建 | `510/510` | `build-0.4.0-dev-repro-clean`；GCC 16.1.0、Meson 1.11.2、Ninja 1.13.2；0 error |
| Meson static | `18/18` | 0 fail；包含炼金图、状态探针、包审计和 Python 聚合 |
| Python | `133/133` | 0 fail、0 skip |
| i18n | PASS | `en-US=1307`、`zh-CN=1307`、missing 0、extra 0、errors 0；38 个需人工分类的警告 |
| 字体结构与渲染 | PASS | 14,633 字形、2,597 必需字符；中文目录离屏测量/渲染 `1307/1307` |
| 元素登记 | PASS | 370 槽、243 active、127 reserved、`PT_NUM=512` |
| 炼金状态探针 | PASS | `initial=4`、`stage_roundtrip=10`、`serialise_parse_import=100`、多存档隔离和 fail-closed 均通过 |
| Lua 防绕过 | PASS | 2 种门禁模式、1 次 stamp 夹具、22 条断言 |
| 十阶段实际运行 | PASS | 2 种模块模式、20 个阶段、4,620 帧、两次 `mastery=true` |
| 四模块基础 Lua | PASS | 模块选择、冶金、化学、生态完整/简化、核工业均通过 |
| OPS 五类双往返 | PASS | 15 进程、10 重启、10 加载、79 粒子、每次加载合计 120 字段断言 |
| 0.2 示例 / 教程 | PASS | 同一候选 EXE 生成并验证 7/7 示例、8/8 教程 |
| 0.3 场景 / 挑战 | PASS | 同一候选 EXE 生成并验证 9/9 场景、6/6 挑战、95 断言、停止增量 0 |

`i18n_audit.py` 对 `alchemy.progress.button_info` 保留一个窄布局宽度警告。`font_render_probe` 已证明字符串可测量、可渲染且无替换字形，但这些自动结果不能证明进度窗口、滚动、窄窗口或高 DPI 的实际桌面布局。

## 可复现二进制与本地产物

第一次比较误用了不同提交时间作为 `SOURCE_DATE_EPOCH`，该比较被拒绝。随后把 clean build、`objcopy`、`strip` 和封包全部固定为 `1785491929`：两个独立 clean 目录的剥离 EXE 均得到同一 SHA-256 `04398AE78304FCAAC42011FB0F6665B32BA9BF558F5FCA8D05FD30A70F3DF4B7`。detached symbols 因构建目录不同而不复现，明确记为 `false`；最终包使用与最终 clean EXE 同次分离的符号。

产物位于忽略目录 `dist/0.4.0-dev-local/`：

| 产物 | 字节 | SHA-256 | 审计 |
|---|---:|---|---|
| `TPT-ZH-OmniPack-0.4.0-dev-Windows-x64.zip` | 5,818,469 | `CC5FD5BB099C12DD4B215C5C592C4794ABAAD482D8CB1A38D41B632586DFF7BD` | local-dev 白名单、清单、三代内容、炼金图、PE 和路径审计通过 |
| `TPT-ZH-OmniPack-0.4.0-dev-Symbols-Windows-x64.zip` | 68,324,563 | `D6C61673FFCF3D2D2695D9F838EF40FAA52370D7A4BE9B9728C4215A5A197628` | 仅 detached symbols 与清单；审计通过 |
| `tpt-zh-omnipack.exe` | 17,791,785 | `04398AE78304FCAAC42011FB0F6665B32BA9BF558F5FCA8D05FD30A70F3DF4B7` | 已剥离；PE、开发路径和动态 GCC runtime 审计通过 |
| `tpt-zh-omnipack.debug` | 243,876,014 | `0769CE456B95B28629796BF79FDEA75F17A57E9465502503F329ADC5C48F165C` | 与最终候选 EXE 同次分离 |

普通包和符号包均以相同输入重新封装两次，两个 ZIP 分别逐字节复现。二次解压后，EXE 和 symbols 再次通过二进制审计，解压 EXE 哈希与包输入一致，清单 revision 为 `f42deded`。隐藏启动得到进程存活且 `Responding=true`，无残留进程；隐藏窗口没有可用句柄，因此 `extracted_gui_visual_test=not_tested`。Authenticode 状态为 `NotSigned`。

## 门禁判定

| 门禁 | 判定 |
|---|---|
| `GATE-040-MODE` | PASS |
| `GATE-040-GRAPH` | PASS |
| `GATE-040-CONDITIONS` | PASS |
| `GATE-040-PERSIST` | PASS（源码、静态、OPS 与实际阶段运行；视觉布局另列） |
| `GATE-040-NO-BYPASS` | PASS |
| `GATE-040-MODULES` | PASS |
| `GATE-040-LONGRUN` | PASS（100 次连续 OPS 往返；不替代 1.0 两小时综合长跑） |
| `GATE-040-RELEASE` | `false` |

## 机器可读结论

```text
package_source_commit=f42deded7f537c343071854068c2e2c453d56cd4
alchemy_implementation_commit=00785c133520d1848635d9a9407d9670a2bb2d6c
alchemy_roundtrip_probe_commit=1aefae4efb11d6ed1fea7eb66265eedf92eb7afc
description_fix_commit=c75aeaf18103ed6cb6eddc844d80b7a3a87045fc
version=0.4.0-dev
release_tag=not_tested
source_date_epoch=1785491929
clean_build_pass=true
clean_build_targets=510/510
clean_rebuild_exe_reproducible=true
debug_symbols_cross_build_reproducible=false
meson_tests=18/18
python_tests=133/133
python_test_skips=0
i18n_keys_en=1307
i18n_keys_zh=1307
i18n_errors=0
font_sha256=91AA3E913051E73B1CE412D2E84487BD459AB78BE606D062716608715479B7FA
font_glyphs=14633
font_required_codepoints=2597
font_fusion_glyphs=1843
font_unifont_fallback_glyphs=0
alchemy_schema=1
alchemy_initial_identifiers=4
alchemy_stages=10
alchemy_condition_types=9
alchemy_graph_acyclic=true
alchemy_serialise_parse_import_cycles=100
alchemy_multi_save_isolation=true
alchemy_corrupt_fail_closed=true
alchemy_unknown_schema_rejected=true
alchemy_gate_assertions=22
alchemy_runtime_modes=2
alchemy_runtime_stage_completions=20
alchemy_runtime_total_frames=4620
alchemy_mastery=true
example_ops_0_2=7/7
tutorial_challenges_0_2=8/8
automation_scenarios_0_3=9/9
automation_challenges_0_3=6/6
release_exe_sha256=04398AE78304FCAAC42011FB0F6665B32BA9BF558F5FCA8D05FD30A70F3DF4B7
debug_symbols_sha256=0769CE456B95B28629796BF79FDEA75F17A57E9465502503F329ADC5C48F165C
local_dev_zip_sha256=CC5FD5BB099C12DD4B215C5C592C4794ABAAD482D8CB1A38D41B632586DFF7BD
symbols_zip_sha256=D6C61673FFCF3D2D2695D9F838EF40FAA52370D7A4BE9B9728C4215A5A197628
package_reproducible=true
zip_audit_pass=true
secondary_extract_audit=true
extracted_process_liveness=true
extracted_gui_visual_test=not_tested
authenticode_signed=false
gate_040_mode=true
gate_040_graph=true
gate_040_conditions=true
gate_040_persist=true
gate_040_no_bypass=true
gate_040_modules=true
gate_040_longrun=true
gate_040_release=false
trusted_gui_matrix=not_tested
source_commit_public=false
anonymous_clone_pass=false
github_release_created=false
release_ready=false
```

未创建 tag、未推送、未发布、未改远端。0.4 的七个本地功能子门禁已有源码、自动或实际运行证据；公开发布门禁仍为 `false`，所以不能宣称 `v0.4.0` 已发布或整体 `release_ready=true`。
