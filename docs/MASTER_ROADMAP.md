# TPT-ZH-OmniPack 1.0 总路线与开发总账

本文件是 `0.1.0-test` 到 `1.0.0` 的顺序执行总账。版本能否发布只由 `docs/VERSION_GATES.md` 中的硬门禁和对应 `dist/release-report-<version>.md` 决定；路线表中的“计划”不代表实现或测试完成。

## 证据口径

所有进度只使用以下六类结论：

| 中文标记 | 含义 | 可作为哪类门禁证据 |
|---|---|---|
| 源码确认 | 已从当前 Git 可达源码、登记表或固定资源核对 | 结构、ID、许可证输入和实现存在性 |
| 编译确认 | 指定提交的 clean build 成功 | 构建门禁；不能代替运行或 GUI |
| 自动测试确认 | 可重复执行的 Meson、Python、Lua 或审计工具通过 | 对应自动测试门禁 |
| 实际运行确认 | 指定 ZIP/EXE 在隔离数据目录中执行并取得进程或场景结果 | 仅覆盖实际运行过的路径 |
| 人工视觉确认 | 人在真实桌面检查指定 ZIP、语言、DPI 和页面 | 视觉与交互门禁 |
| 尚未测试 | 没有足够证据，机器可读值为 `not_tested` | 不得通过任何硬门禁 |

`PASS`、`true` 或计数仅在证据绑定到精确源码提交和产物哈希时有效。静态分析不能替代 OPS 往返，进程响应不能替代 GUI 可读性，Lua 场景不能替代压力数据。

## 2026-07-30 接管时基线

```text
audit_head=e18abad9753e61e8f6c9f8fdf671c4bd80a4ca53
implementation_commit=c743db2fcc49c01033e68023cceff897ed4c35f6
development_branch=development/omnipack-1.0
upstream_version=The Powder Toy 100.0.399
candidate_version=0.1.0-test
implemented_omnipack_elements=48
font_sha256=C13C3D0ECB9EAC6B8CB1C2785C4C3176C578C1B07506D4CA5D32F24838E566B0
private_candidate_exe_sha256=05DACBFCC31D6F1920D4437DB60629A1F9DA79CC0A14AA13138DD97393CCF6AF
private_candidate_public_zip_sha256=943DA2A60C0B371A1D3F921FEC525FB3F7B5AEBC7C5CE7775A8AEFA883C13F14
private_candidate_symbols_zip_sha256=BE14C7D53658963DF1C6B1ECFAA44AC51F8004883530CB7DF922E4282FC11031
clean_build_pass=true
meson_tests=12/12
python_tests=56
lua_runtime_tests=6/6
private_candidate_startup_runs=20
private_candidate_startup_crashes=0
font_visual_test=not_tested
ops_roundtrip_test=not_tested
stress_test=not_tested
source_commit_public=false
anonymous_clone_pass=false
release_ready=false
```

基线证据来自 `docs/FINAL_VALIDATION.md`、`docs/FONT_AUDIT.md`、`docs/TEST_MATRIX.md` 和 `dist/release-report-0.1.0-test.md`。`e18abad9` 只记录试包结果，实际字体实现绑定 `c743db2f`；后续产物必须重新绑定其真实构建提交，不能继续沿用上述私有试包哈希。

### 接管后增量证据

```text
candidate_source_commit=ff5945c4acbe15052a316771934854aa0f9281de
binary_clean_build_commit=ff5945c4acbe15052a316771934854aa0f9281de
development_gate_head=ff5945c4
candidate_package_documentation_commit=ff5945c4
portable_install_prompt_fix=a590f8b5
reaction_registry_gate=8391dbd1
ops_mixed_roundtrip_test_commit=148c4acd
ops_isolated_cases_commit=f92e12fa
stress_harness_commit=ff5945c4
stress_responsiveness_fix=4f5c07f9
stress_assessment_gate=ff5945c4
release_report_gate=ff5945c4
font_visual_test=true
ops_mixed_carrier_roundtrip_test=true
ops_official_roundtrip_test=true
ops_single_module_roundtrip_tests=4/4
clean_build=502/502
meson_tests=14/14
candidate_python_tests=87/87
public_zip_sha256=53E0304FF8CE932F7D836620A7599085A486B1689EAC131BC78D7B8EA6619827
symbols_zip_sha256=8FD702E9F9B92E34321226340F9EF3742EFA86FE8C0FA98302CD6CECAAACE48D
stress_samples=10/10
stress_event_total=22529
stress_test=true
candidate_s09_7200s_observation=true
candidate_s09_run_id=20260730T210719Z-d4085bc4
candidate_s09_performance_gate=true
cross_module_implementation_commit=98affcd76c9d3a02b136781d1bfa71fefb88302f
implementation_exe_sha256=EBB33CCEDE76DDCC4F9375F96330519A1A5751080E40E936D3BAEB70763F3D82
implementation_lua_runtime=5/5
implementation_stress_smoke=4/4
release_ready=false
```

用户已确认当前原生 Fusion 12px 字体的中文实际可读性；DPI 和完整页面矩阵仍未测试。官方、四个单模块和四模块混合 OPS 已分别完成真实三进程双往返，验证稳定 identifier/ID 与 `LAVA/SPRK/MSCR/CONV/VIRS` 的 `ctype/tmp/tmp2`。本地 `.cps` 保存对话框、禁用模块三选项和只读写入拦截仍需 GUI 交互。当前候选的十项 `60+600` 秒样本均完成、通过独立 JSON/CSV/OPS 评估，并实际记录模块事件计数和停止/恢复断言；十项 `performance_gate_pass=true`，总事件 `22529`。另有 S09 连续 `7200.002183` 秒有限观测通过独立评估，但不覆盖 1.0.0 所需的语言/模块切换等综合长跑，故 `long_run_test=not_tested`。

### 已明确废弃的候选

| 实现提交 | 普通 ZIP SHA-256 | 结论 |
|---|---|---|
| `5828a97fc39129547354956dde84d7b6cfb818c2` | `DC8211AC5F4590DA74231D922FCCC0168933D6DCF7AB82AA1D369CE2E97B0189` | 中文字形损坏，已废弃 |
| `ca3cccbee13a41c37ee0b7975c4b5f060cb34a95` | `E52E746BF925B2096ED93D659E54A52876179230D29D9534D4E579EB755E4081` | 人工可读性/字形质量失败，已废弃 |

这些提交和哈希只保留作失败回归基线，不得再作为当前候选、tag 或公开发布物。

## 不可破坏的全局约束

- 官方元素 ID `0..195` 保持锁定；ID 146 tombstone 不复用。
- 现有 48 个 OmniPack 元素保持 identifier 与稳定 ID：冶金 `256..278`、生态 `288..295`、核工业 `328..334`、化学 `360..369`。
- 关闭模块只隐藏选择入口；加载时不得静默删除、替换或重排粒子，间接类型引用必须同步检测。
- 测试偏好、OPS、图章、账户资料和令牌只能写入隔离目录，不能进入普通发布包。
- 新元素和新反应先登记、后实现；没有生产、用途、控制/清理或测试的内容不得成为正式内容。
- 所有传播、复制、信号和高能路径必须有局部范围、每帧事件预算及循环保护。
- 各版本依次完成；前一版本硬门禁未通过时，不创建其 tag，也不把后续开发称为已发布版本。

## 分支与提交纪律

总开发分支为 `development/omnipack-1.0`。版本准备分支依次为：

```text
release/0.1.0-test
release/0.2.0
release/0.3.0
release/0.4.0
release/0.5.0
release/0.6.0
release/0.7.0
release/0.8.0
release/0.9.0
release/1.0.0
```

分支名是工作约定，不是完成证据。每个提交只覆盖一类实现、测试、性能、文档或发布工作；版本 tag 只能在该版本全部硬门禁通过、报告为 `release_ready=true` 后创建。

## 顺序版本总账

| 版本 | 主要交付 | 当前证据状态 | 进入下一版本的条件 |
|---|---|---|---|
| `0.1.0-test` | 冻结 48 元素与四模块，完成双语、OPS 门禁、十样本压力、法律与可公开测试包 | 中文可读性、官方/单模块/混合 OPS、源码/构建/自动测试、本地 ZIP 和十样本性能门禁已有证据；DPI/语言切换、GUI 只读门禁、凭据撤销、公开源码未通过 | `GATE-010-*` 全部通过并创建 `v0.1.0-test` |
| `0.2.0` | 四模块用途审计、四条跨模块闭环、7 个示例存档、8 项教程/挑战 | 本地功能证据已完成：48/48 用途矩阵、四条链、OPS `7/7`、教程 `8/8`、正式 S04/S05/S07/S09 `4/4` 与 `0.2.0-dev` 包审计通过；可信 GUI、公开源码/tag/Release 尚未完成 | `GATE-020-*` 全部通过并创建 `v0.2.0` |
| `0.3.0` | 复用官方电子系统的传感、阀门、联锁和自动化场景 | 本地功能证据已完成：11 类官方能力、9/9 场景、6/6 挑战、S11/S12 `2/2 gate=true` 与 `0.3.0-dev` 包审计通过；可信 GUI、公开源码/tag/Release 尚未完成 | `GATE-030-*` 全部通过并创建 `v0.3.0` |
| `0.4.0` | 独立炼金探索模式、十阶段解锁、进度持久化与防死局图搜索 | 本地功能证据已完成：图/九类条件、两种模块模式十阶段、22 条防绕过、100 次连续 OPS 往返、全量回归与 `0.4.0-dev` 两包二审通过；可信 GUI、公开源码/tag/Release 尚未完成 | `GATE-040-*` 全部通过并创建 `v0.4.0` |
| `0.5.0` | 有预算、可隔离、可清理的灾害与特殊物理，以及恢复挑战 | 尚未测试 | `GATE-050-*` 全部通过并创建 `v0.5.0` |
| `0.6.0` | 多维搜索、图鉴关系、生产链、示例/教程入口和高 DPI 操作 | 尚未测试 | `GATE-060-*` 全部通过并创建 `v0.6.0` |
| `0.7.0` | 官方及合法来源存档语料、兼容等级、备份/迁移/取消/日志 | 尚未测试 | `GATE-070-*` 全部通过并创建 `v0.7.0` |
| `0.8.0` | 性能保护、基线回归、模糊测试、崩溃恢复和高负载稳定性 | 尚未测试 | `GATE-080-*` 全部通过并创建 `v0.8.0` |
| `0.9.0` | 功能冻结，只处理缺陷、兼容、平衡、性能、翻译、教程和 UI | 尚未测试 | `GATE-090-*` 全部通过，创建 `v0.9.0` 与 `v1.0.0-rc.1` |
| `1.0.0` | 正式双语整合包、全链路功能、两小时稳定性、公开源码和三类发布包 | 尚未测试 | `GATE-100-*` 全部通过，报告精确写入 `release_tag=v1.0.0`、`version=1.0.0`、`release_ready=true` |

## 各阶段实施边界

### 0.1.0-test：稳定公开测试基线

不增加大型玩法。只允许修复中文/英文 GUI、四模块 UI、OPS 往返和只读门禁、固定压力样本、凭据/许可证/打包问题。当前最短关键路径是：

1. 从绑定 `ff5945c4` 的 ZIP 做简中/英文、100%/125%/150% DPI 人工矩阵；
2. 实际点击正常加载、只读加载、取消及全部保存/上传拦截路径；
3. 完成 1.0.0 定义的综合两小时长跑（当前候选 S09 已有 `7200.002183` 秒有限观测，但不包含语言/模块切换等全部要求）；十个固定 `60+600` 秒样本已通过；
4. 在外部账户撤销或轮换已暴露 PAT，再进行推送、匿名克隆、tag 和 prerelease。

官方、四模块、混合及 `LAVA`/`SPRK`/`MSCR`/`ctype/tmp/tmp2` OPS 双往返已经通过，不再列为未完成关键路径。

### 0.2.0：跨模块闭环

48 元素用途矩阵与审计门禁已经完成；四条最小跨模块反应由提交 `98affcd7` 登记、实现：炉渣酸处理、过氧化氢病原处理与腐殖质回收、不锈钢—熔融铅屏蔽组装、冷却四模块废物稳定化。本地候选 `32336e66` 的 7 个示例 OPS、8 项教程、Lua/OPS 全量回归及 S04/S05/S07/S09 四项正式压力均通过，普通包和符号包二审通过。`GATE-020-SAVES`、`GATE-020-CHALLENGES` 和针对四条链的测试子门禁已有证据；`GATE-020-RELEASE` 仍因可信双语 GUI、授权公开源码、匿名克隆、tag 和 GitHub Release 未完成而为 `false`，因此不能创建或宣称公开发布 `v0.2.0`。后续本地开发不替代此前序发布门禁。详细证据见 `docs/PHASE_0_2_EVIDENCE.md`。

### 0.3.0：工程自动化

11 类能力均已证明可复用官方电子元件，新增自动化元素为 0，稳定 ID `392..423` 保持预留。候选 `dafec4d0` 的 9 个真实 OPS 场景、6 项挑战、同 EXE 的 0.2 内容、普通/符号包二审和 S11/S12 正式压力均通过；S12 的一次性火花失败样本已保留并由 `522dc3bc` 的官方 BTRY 持续激励夹具修复。`GATE-030-DESIGN/SCENARIOS/BOUNDS/INTERLOCK/CHALLENGES/STRESS` 已有本地证据，`GATE-030-RELEASE` 仍为 `false`。详细证据见 `docs/PHASE_0_3_EVIDENCE.md`。

### 0.4.0：炼金探索

普通沙盒保持自由。独立进度必须按存档隔离，搜索、收藏和普通 Lua 接口不得绕过锁定；配方图工具必须证明关键节点可达、无循环死锁，并覆盖模块关闭与旧进度迁移。

当前本地候选 `f42deded` 已完成上述实现与验证：schema 1 的四初始元素/十阶段图覆盖九类条件；模块开/关各十阶段均达到精通；锁定、自由和 stamp 共 22 条创建门禁断言通过；精通状态连续 100 次真实 `Serialise -> Parse -> Import` 无丢失或重复。`build-0.4.0-dev-repro-clean` 为 `510/510`，static `18/18`、Python `133/133`；普通包 `CC5FD5BB...F7BD` 与 Symbols 包 `D6C61673...7628` 两次封包复现并通过解压二审。`GATE-040-MODE/GRAPH/CONDITIONS/PERSIST/NO-BYPASS/MODULES/LONGRUN` 已有本地证据，`GATE-040-RELEASE=false`。详细边界见 `docs/PHASE_0_4_EVIDENCE.md`。

### 0.5.0：灾害与特殊物理

每种危险必须登记触发、扩散、寿命/预算、隔离、清理、防护和性能上限。没有停止或清理路径的系统不得合入正式内容。

### 0.6.0：内容发现

索引支持中英文名、四字符代号、identifier、用途、来源、模块、状态和危险等级；模块/解锁状态改变后索引必须一致。长说明、高 DPI、键盘操作和搜索时延均为硬门禁。

### 0.7.0：兼容与迁移

只分析可合法取得且来源明确的存档/源码。迁移前自动备份，失败或取消不得改变当前沙盘；直接类型与所有载体字段必须使用同一映射。每类来源显式标记完全兼容、自动迁移、部分迁移、只读或无法兼容。

### 0.8.0：性能与安全

对模拟、自动化、灾害、索引、迁移和 Lua 做同硬件基线回归；对字体、JSON、OPS 元数据、模块状态、元素 ID、配方图、进度和索引做模糊测试。任何无界增长、数据损坏或复现崩溃均阻塞发布。

### 0.9.0 与 RC：冻结

从 `0.9.0` 起禁止大型新系统、大规模元素扩充和无必要架构重写。RC 只接受阻塞缺陷修复及相应回归；每次 RC 都重新生成、审计和人工检查发布包。

### 1.0.0：正式发布

必须同时满足功能、双语、兼容、性能、两小时运行、法律、安全、对应源码、匿名克隆、ZIP/符号/源码包和实际 Windows x64 运行门禁。未签名可以如实发布，但 `authenticode_signed=false` 和 SmartScreen 说明必须同时存在。

## 每版闭环

每个版本按以下顺序执行，不因局部成功中止：

1. 更新版本号、CHANGELOG、README、登记表和测试/兼容/来源/AI 文档；
2. clean build，执行全部 Meson、Python 和 Lua 回归；
3. 执行该版本 GUI、OPS、场景、压力与迁移测试；
4. 生成普通包、符号包及要求的源码包，写入 SHA-256；
5. 从 ZIP 解压，在隔离数据目录二次运行和内容审计；
6. 生成 `dist/release-report-<version>.md`，所有未执行项保留 `not_tested`；
7. 将独立类型的工作拆分提交；
8. 只有所有硬门禁通过才创建 tag、推送、匿名克隆验证并创建 prerelease/release；
9. 合入总开发分支并进入下一版本。

## 当前总账结论

```text
current_version=0.1.0-test
current_gate_set=GATE-010
current_gate_result=false
next_version_allowed=false
release_ready=false
```

当前可以继续完成本地修复、测试、文档和打包，但在 PAT 撤销/轮换、真实 GUI、公开源码和匿名克隆证据齐全前，不得创建 `v0.1.0-test` 或宣称公开发布 0.1.0-test；十样本性能门禁本身已通过。
