# 0.3.0-dev 本地工程自动化候选证据

本文件记录 `0.3.0-dev` 的本地可复核结果。它不是公开发布报告：可信双语 GUI/DPI、禁用模块只读交互、授权公开源码、匿名克隆、tag、GitHub Release 和正式 `v0.3.0` 均未完成，因此 `release_ready=false`。

## 来源与候选绑定

| 对象 | 提交 / 值 | 说明 |
|---|---|---|
| 官方自动化能力矩阵、场景与审计 | `52544032509f0b6e6888c6322a1c385e2f62ee3c` | 11 类能力、9 个场景、6 个挑战、17 个官方源码指纹；没有新增元素 |
| 0.3 示例生成基础设施 | `8650b3e1` | 生成、重载及运行报告 |
| 官方信号压力指标 | `ef0202bf` | 独立记录 `SPRK` 总量、峰值和停止状态 |
| 0.3 本地包配置 | `a04c1ffe` | 显式 `0.3.0-dev/local-dev`，同时封入 0.2 与 0.3 内容 |
| 双语元素自识别说明 | `c75aeaf18103ed6cb6eddc844d80b7a3a87045fc` | 48 个元素的说明统一以登记名称开头；重建语言包和字体 |
| 0.3 场景运行证据 | `a57e712291e25183b207f989817aab67468bd848` | 绑定新 EXE 的 9/9 场景、6/6 挑战 |
| 0.2 教程运行证据 | `dafec4d082fead6da49cb7e236f87262236096d6` | 同一 EXE 的 7/7 示例、8/8 教程；也是普通包清单 revision |
| S12 持续信号夹具 | `522dc3bc10c363bfd73333928f596a482b60a274` | 用官方 `BTRY` 持续激励官方导体；不修改官方电子行为 |
| 固定构建时间 | `SOURCE_DATE_EPOCH=1785456000` | ZIP 和 PE 时间输入固定 |

包内 `TEST-MANIFEST.txt` 的 revision 是 `dafec4d0`。S11 的 harness commit 同为 `dafec4d0`；S12 通过样本的 harness commit 是 `522dc3bc`。二进制来源与后续测试夹具修复分开记录，不能把夹具提交冒充为已打包二进制来源。

## 自动化设计结论

- 逐项审计传感、过滤、延迟、计数、存储、阀门、加料、排废、停机、报警和联锁，共 `11/11` 类能力。
- 复用 `TSNS/PSNS/LSNS/VSNS/DTEC/LDTC/FILT/DLAY/STOR/CRAY/PSTN/PIPE/PPIP/SWCH/WIFI/INST` 等官方能力，不新增自动化元素。
- 17 份直接复用的官方元素源码以 SHA-256 固定；任何缺失或漂移都会使 `automation_audit.py` 失败。
- 稳定 ID `392..423` 继续保持未占用预留区；现有 48 个 OmniPack 元素的 ID 未移动。
- 自动化场景只组合官方电子元件和已有模块材料；没有新的全粒子扫描、模块门禁旁路、只读写入旁路或普通保存路径旁路。

## 双语元素说明修复

48 个已实现模组元素的两份说明现使用统一结构：英文为 `Element Name: description`，简中为 `元素名：说明`。登记表和语言包逐字一致，且 `.name` 键必须等于登记名称。

`element_registry_check.py` 对以下回退 fail-closed：

1. 说明没有以对应名称和规定冒号开头；
2. `en-US.json` 或 `zh-CN.json` 的名称与登记表不同；
3. 任一语言说明与登记表说明不同。

当前核对结果为 `omni_rows=48`、`consistency_errors=0`。新增 3 个负向用例分别验证缺前缀、登记/语言说明漂移和语言名称漂移。

新增全角冒号 `U+FF1A` 后按固定来源重建 `resources/font.bz2`：SHA-256 为 `C13C3D0ECB9EAC6B8CB1C2785C4C3176C578C1B07506D4CA5D32F24838E566B0`，包含 14,626 个字形，覆盖 2,590 个语言字符；Fusion 补充 1,836 个，Unifont 回退 0 个。分离重建得到相同哈希。

## 构建与自动回归

| 门禁 | 结果 | 说明 |
|---|---|---|
| 全新 Windows x64 Release 构建 | `502/502` | `build-0.3.0-dev-final-clean`，GCC 16.1.0、Meson 1.11.2、Ninja 1.13.2 |
| Meson static | `16/16` | 0 fail；含 i18n、字体、元素/反应/自动化/存档/包审计和 Python 聚合 |
| Python | `121/121` | 0 fail、0 skip |
| i18n | PASS | `en-US=1262`、`zh-CN=1262`、missing 0、extra 0、errors 0；37 个既有人工分类警告 |
| 字体容器/覆盖/矩阵/渲染探针 | PASS | 14,626 字形、2,590 必需字符，pack/unpack 和 Fusion 固定矩阵通过 |
| 元素登记 | PASS | 370 行、243 active、127 reserved、`PT_NUM=512` |
| 自动化审计 | PASS | 11 能力、9 场景、6 挑战、17 固定官方源码、0 新元素 |
| 0.3 场景/挑战 | `9/9`、`6/6` | 总断言 95，停止后事件增量 0 |
| 0.2 示例/教程 | `7/7`、`8/8` | 与 0.3 内容绑定同一 EXE |

全新构建重新剥离的普通 EXE SHA-256 仍为 `FBCA1387...0FF2`，即 `clean_rebuild_exe_reproducible=true`。全新构建目录的 detached symbols 为 `D14F315D...E26A`，与原候选的 `128310B6...9A65` 不同，因此 `debug_symbols_cross_build_reproducible=false`；两者均通过符号分离和二进制审计，不能把“使用相同符号输入重复封包”写成“跨 clean build 符号可复现”。

## 本地产物

产物位于忽略目录 `dist/0.3.0-dev-local/`，没有覆盖 0.1 或 0.2 候选。

| 产物 | 字节 | SHA-256 | 审计 |
|---|---:|---|---|
| `TPT-ZH-OmniPack-0.3.0-dev-Windows-x64.zip` | 5,786,269 | `BB57D87F2712ABAC45E0BFAE2743D1E168BD27278404CCB619B867D32F3607EA` | local-dev 白名单、清单、两代示例、运行报告、PE 和路径审计通过 |
| `TPT-ZH-OmniPack-0.3.0-dev-Symbols-Windows-x64.zip` | 67,764,532 | `7AAD0AB5FE3F20E9B5624E17F6280CDFF3B0B3FA1655F9BEA998007BEE74ACAE` | 仅 detached symbols 与清单；审计通过 |
| `tpt-zh-omnipack.exe` | 17,677,669 | `FBCA138710EDB964ECCA7BA14FCB276BE24C8E616F059BF6A18682A8A1380FF2` | 已剥离；PE、开发路径和动态 GCC runtime 审计通过 |
| `tpt-zh-omnipack.debug` | 241,984,637 | `128310B6D37BA63FEE739B46F39A9717FEB3E0F22D026D460E9AEF158AF79A65` | 与该候选 EXE 同次分离 |

普通包和符号包分别由相同固定输入再次封包，两个 ZIP 均逐字节复现。这个结论只覆盖封包器确定性，不覆盖上表已明确为 false 的跨 clean build 符号文件可复现性。

## 示例与挑战

0.3 提供 9 个真实 `OPS1/BZip2` 场景，稳定加载 ID 为 `0300000001..9`：自动恒温熔炉、自动合金、燃料控制、营养加料、病原体消毒、反应堆冷却、紧急停机、废料转运和综合自动工厂。

运行报告绑定 EXE `FBCA1387...0FF2`：场景 `9/9`、挑战 `6/6`、断言 `95`、停止后事件增量 `0`。六项挑战覆盖控温、加料、分拣、消毒、紧急停机和多模块自动工厂。

包还封入同一 EXE 刷新的 0.2 内容：7 个示例和 8 项教程均实际加载/完成。清单与报告哈希如下：

| 文件 | SHA-256 |
|---|---|
| `examples/0.2.0/manifest.json` | `08C70245818221E2654A7F18F600EE3E0C55BA8A2C6349CBD3CB65F3A0331AF0` |
| `examples/0.2.0/tutorials-runtime-report.json` | `CF79B3793C9F830AF6615B8BE779F9E27C1598E43D92196EE926817CF419268B` |
| `examples/0.3.0/manifest.json` | `6772A067FDC8C5E8BC7E531B3C5B2289EC5FB97094DEE06F810B6531A7E06ADC` |
| `examples/0.3.0/runtime-report.json` | `1C3C83E6B33E37EDAC43819196C654FA92985ADCB93E7F7EFB702E31F9DBCA55` |

## 0.3 正式压力

两项通过样本均绑定普通 ZIP `BB57D87...07EA` 和 EXE `FBCA1387...0FF2`，串行单客户端运行。每项为至少 60 秒预热加 600 秒采样，随后执行停止、恢复、保存、加载和独立 `assessment.json` 判定。

| 样本 / Run ID | 实际秒数 | 平均 / 1% low / 最低 FPS | 初始 / 峰值 / 最终粒子 | 模块事件总数 / 峰值 | 官方信号总数 / 峰值 | 门禁 |
|---|---:|---:|---:|---:|---:|---|
| S11 `20260731T080609Z-91b43bef` | `60.001 + 600.005` | `60.001 / 47.178 / 39.853` | `74304 / 74304 / 54535` | `5233 / 4096` | `74380132 / 4132` | true |
| S12 `20260731T083137Z-80a982fe` | `60.016 + 600.017` | `52.809 / 29.972 / 22.771` | `148608 / 150220 / 150220` | `0 / 0` | `978876949 / 56421` | true |

两项均有 `crashed=false`、`hung=false`、`roundtrip_pass=true`、`signal_stop_pass=true`、`stop_event_delta=0`、`scenario_recovery_assertions=7`、`unbounded_growth=false`、`memory_leak_suspected=false`、`signal_behavior_pass=true` 和 `performance_gate_pass=true`。S12 的 595 个粒子采样全部为 `150220`；尾段首尾同为 `150220`。

S12 首次正式样本 `20260731T081741Z-ddd919fe` 被明确拒绝：运行时长、粒子稳定、内存、停止、恢复和 OPS 均通过，但一次性初始化火花在预热期耗尽，正式窗口 `signal_count_total=0`、`signal_behavior_pass=false`、`performance_gate_pass=false`。提交 `522dc3bc` 改用官方 BTRY 持续激励后，先以 smoke 确认信号路径，再执行上表新的完整样本；失败样本不计入 `2/2`。

`unbounded_growth=false` 与 `memory_leak_suspected=false` 只表示本次有限采样未满足判定器定义的单调增长规则，不是长期有界性的数学证明，也不替代 1.0.0 要求的两小时综合长跑。

## 机器可读结论

```text
package_source_commit=dafec4d082fead6da49cb7e236f87262236096d6
latest_harness_commit=522dc3bc10c363bfd73333928f596a482b60a274
description_fix_commit=c75aeaf18103ed6cb6eddc844d80b7a3a87045fc
version=0.3.0-dev
release_tag=not_tested
clean_build_pass=true
clean_build_targets=502/502
clean_rebuild_exe_reproducible=true
debug_symbols_cross_build_reproducible=false
meson_tests=16/16
python_tests=121/121
python_test_skips=0
i18n_keys_en=1262
i18n_keys_zh=1262
i18n_errors=0
omnipack_description_rows=48/48
omnipack_description_consistency_errors=0
automation_capabilities=11/11
official_automation_source_pins=17/17
new_automation_elements=0
reserved_automation_ids=392..423
automation_scenarios=9/9
automation_challenges=6/6
automation_assertions=95
automation_stop_event_delta=0
example_ops_0_2=7/7
tutorial_challenges_0_2=8/8
formal_stress_samples=2/2
formal_stress_sample_ids=S11,S12
formal_stress_event_total=5233
formal_stress_signal_total=1053257081
formal_stress_signal_peak_per_frame=56421
formal_stress_gate=true
rejected_formal_sample=20260731T081741Z-ddd919fe
font_sha256=C13C3D0ECB9EAC6B8CB1C2785C4C3176C578C1B07506D4CA5D32F24838E566B0
release_exe_sha256=FBCA138710EDB964ECCA7BA14FCB276BE24C8E616F059BF6A18682A8A1380FF2
debug_symbols_sha256=128310B6D37BA63FEE739B46F39A9717FEB3E0F22D026D460E9AEF158AF79A65
local_dev_zip_sha256=BB57D87F2712ABAC45E0BFAE2743D1E168BD27278404CCB619B867D32F3607EA
symbols_zip_sha256=7AAD0AB5FE3F20E9B5624E17F6280CDFF3B0B3FA1655F9BEA998007BEE74ACAE
zip_audit_pass=true
authenticode_signed=false
trusted_gui_matrix=not_tested
source_commit_public=false
anonymous_clone_pass=false
github_release_created=false
release_ready=false
```

未创建 tag、未推送、未发布、未改远端。`GATE-030-DESIGN`、`GATE-030-SCENARIOS`、`GATE-030-BOUNDS`、`GATE-030-INTERLOCK`、`GATE-030-CHALLENGES` 和 `GATE-030-STRESS` 已有本地源码、自动或实际运行证据；`GATE-030-RELEASE` 仍为 `false`，所以不能宣称 `0.3.0` 已公开发布或整体 `release_ready=true`。
