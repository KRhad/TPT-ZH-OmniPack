# 0.2.0-dev 本地跨模块候选证据

本文件记录 `0.2.0-dev` 的本地可复核结果。它不是公开发布报告：可信双语 GUI/DPI、禁用模块只读交互、授权公开源码、匿名克隆、tag、GitHub Release 和正式 `v0.2.0` 均未完成，因此 `release_ready=false`。

## 来源与候选绑定

| 对象 | 提交 / 值 | 说明 |
|---|---|---|
| 四条跨模块实现 | `98affcd76c9d3a02b136781d1bfa71fefb88302f` | 局部反应、预算和回收实现 |
| 教程与示例验证器 | `a81a0a5199809933f13858861e71caf5e4a24c5f` | 7 个示例规范、8 项教程及运行夹具 |
| 版本化本地包与压力来源门禁 | `c77c777410a8468bd011ca7669f05e9e748abf8e` | 显式区分 `0.1.0-test/public-test` 与 `0.2.0-dev/local-dev` |
| 示例生成清单来源 | `dcd2ae578916288f9a87ed9f860b6516856ac25e` | `examples/0.2.0/manifest.json` 的 `source_commit` |
| 本地包 / EXE / 正式压力来源 | `32336e66cecd5dda70b11d5c019a3cdfa6506670` | 包内 `TEST-MANIFEST.txt`、四份 `result.json` 的 `source_commit` 和 `harness_commit` |
| 最终教程运行报告来源 | `32336e66cecd5dda70b11d5c019a3cdfa6506670` | `tutorials-runtime-report.json` 对同一候选 EXE 的 8/8 验证 |
| 运行夹具句柄清理修复 | `c63c5672` | 测试基础设施修复，不冒充已打包二进制来源 |
| 版本标签 | `0.2.0-dev` | 仅本地开发候选；不是 `v0.2.0` |
| 固定构建时间 | `SOURCE_DATE_EPOCH=1785456000` | 使 `objcopy/strip` 的 PE 时间确定；两次分离哈希完全一致 |

构建使用 `resolve_vcs_tag=no`、`can_install=no`、静态 GCC runtime、`--no-insert-timestamp` 和独立调试符号。候选提交 `32336e66` 重新链接后仍得到与示例清单相同的 EXE 哈希；包清单的 revision、EXE 大小和哈希均由压力夹具再次核对。

## 本地产物

产物位于忽略目录 `dist/0.2.0-dev-local/`，未覆盖既有 0.1 候选。

| 产物 | 字节 | SHA-256 | 审计 |
|---|---:|---|---|
| `TPT-ZH-OmniPack-0.2.0-dev-Windows-x64.zip` | 5,765,971 | `2781B8B0AC9BF2EF53A6D799081A5C55CBB5F882BDE30226E07DB68CD96F3FA8` | `kind=local-dev`；白名单、成员哈希、7 个 OPS、教程报告与 EXE 绑定通过 |
| `TPT-ZH-OmniPack-0.2.0-dev-Symbols-Windows-x64.zip` | 67,764,596 | `F4B6159EB1574AEDB98F9E3B03BD565D29935607003FDDA2469CEB755F4C5EF0` | 仅 detached symbols 与清单，审计通过 |
| `tpt-zh-omnipack.exe` | 17,675,109 | `08D30ED6753E97EA6605E8553B0C82B06B9A8E4818C6461ABBFD0C0F499F340B` | 已剥离；PE/路径/runtime 审计通过；Authenticode `NotSigned` |
| `tpt-zh-omnipack.debug` | 241,984,637 | `4BD0BA27B6A3B6170B6519B2771FDA95144DBB5DCD46A6DB6E41896CAFD67D2E` | 与普通 EXE 分离 |
| `examples/0.2.0/manifest.json` | 3,305 | `D8A484DB6509B43C42E2B29D98027165A988E42C785C453751B816607E82424B` | 7/7 OPS 的稳定加载 ID、字节数和 SHA-256 |

普通包从 ZIP 解压到全新隔离目录后实际启动 8 秒：标题为 `TPT-ZH-OmniPack 0.2.0-dev`，`Responding=true`，主窗口句柄非零，EXE 哈希与包清单一致。该结果只证明进程级启动，不证明窗口内容、字体、语言、DPI 或交互正确。

## 示例 OPS 与教程

7 个 OPS 均为真实 `OPS1/BZip2` 存档，由候选 EXE 生成并重新加载。稳定加载 ID 为 `0200000001` 至 `0200000007`；初始粒子数依次为 `2, 3, 3, 3, 6, 5, 17`。详细文件哈希见 `examples/0.2.0/manifest.json`。

| 教程 | 示例 | 断言 | 结果 |
|---|---|---:|---|
| `T01-PERO-PATH` | `peroxide-pathogen` | 4 | PASS |
| `T02-PERO-CONTROL` | `peroxide-pathogen` | 6 | PASS |
| `T03-HUMS-FERT` | `humus-fertilizer` | 5 | PASS |
| `T04-SLAG-ACID` | `slag-acid` | 3 | PASS |
| `T05-SHIELD` | `shield-assembly` | 4 | PASS |
| `T06-WASTE-CATALYST` | `waste-missing-catalyst` | 6 | PASS |
| `T07-WASTE-FULL` | `waste-stabilization` | 7 | PASS |
| `T08-INTEGRATED` | `integrated-recovery` | 16 | PASS |

T05 保存固态 `NCRM`，加载后再转为 `SPRK(NCRM)`，避免瞬态火花在 OPS 中丢失。T06 的缺催化剂观察阶段关闭热相变，防止 550 K 的官方 `WATR` 在补入催化剂前先变为蒸汽；恢复热模拟后再验证完整稳定化。

## 自动与运行回归

| 门禁 | 结果 | 说明 |
|---|---|---|
| Windows x64 Release 构建 | PASS | 初次完整构建 `502/502`；候选提交重链和可复现分离通过 |
| Meson | `15/15` | 0 fail |
| Python | `109/109` | UCRT64 编译器在 PATH，0 skip |
| Lua 运行 | `6/6` | 生物 full/simplified、化学、冶金、核工业、Lua 模块选择 |
| 单类 OPS 双往返 | `5/5` | 15 进程、10 次重启、10 次加载、79 粒子、每次加载合计 120 字段断言 |
| 混合 OPS 双往返 | `1/1` | 3 进程、2 次重启、2 次加载、11 粒子、每次加载 20 字段断言 |
| 示例 / 教程 | `7/7`、`8/8` | 真实生成、加载、解题和断言 |
| 普通包 / 符号包二审 | `2/2` | ZIP 哈希、清单、成员、PE、示例和教程绑定通过 |

## 0.2 正式压力

四项均使用相同的本地包、EXE 和提交，按顺序单客户端运行。每项为至少 60 秒预热加 600 秒采样，随后执行停止、恢复、保存、加载和独立 `assessment.json` 判定。

| 样本 / Run ID | 实际秒数 | 平均 / 1% low / 最低 FPS | 初始 / 峰值 / 最终粒子 | 事件总数 / 单帧峰值 | 门禁 |
|---|---:|---:|---:|---:|---|
| S04 `20260731T001643Z-4c79f478` | `60.004 + 600.002` | `60.001 / 55.346 / 49.998` | `46784 / 46784 / 6605` | `12044 / 2560` | true |
| S05 `20260731T002813Z-62d88823` | `60.005 + 600.001` | `60.002 / 53.987 / 48.858` | `51600 / 51600 / 9566` | `3865 / 1473` | true |
| S07 `20260731T061510Z-ca26e6d5` | `60.007 + 600.003` | `60.001 / 41.175 / 29.807` | `85312 / 85312 / 84782` | `2049 / 512` | true |
| S09 `20260731T062638Z-963e6452` | `60.010 + 600.001` | `60.002 / 50.810 / 43.828` | `57865 / 57865 / 28661` | `12599 / 3072` | true |

聚合为 `4/4`、事件 `30,557`、最大单帧事件 `3,072`。每项均有 `crashed=false`、`hung=false`、`roundtrip_pass=true`、`stop_event_delta=0`、`scenario_recovery_assertions=7`、`unbounded_growth=false`、`memory_leak_suspected=false` 和 `performance_gate_pass=true`。

`unbounded_growth=false` 与 `memory_leak_suspected=false` 只表示本次有限采样未满足判定器定义的单调增长规则，不是长期有界性的数学证明，也不替代两小时综合长跑。

## 机器可读结论

```text
candidate_source_commit=32336e66cecd5dda70b11d5c019a3cdfa6506670
candidate_harness_commit=32336e66cecd5dda70b11d5c019a3cdfa6506670
example_generation_source_commit=dcd2ae578916288f9a87ed9f860b6516856ac25e
tutorial_verification_source_commit=32336e66cecd5dda70b11d5c019a3cdfa6506670
version=0.2.0-dev
release_tag=not_tested
clean_build_pass=true
clean_build_targets=502/502
meson_tests=15/15
python_tests=109/109
python_test_skips=0
lua_runtime_invocations=6/6
ops_roundtrip_scenarios=6/6
example_ops=7/7
tutorial_challenges=8/8
formal_stress_samples=4/4
formal_stress_sample_ids=S04,S05,S07,S09
formal_stress_event_total=30557
formal_stress_peak_event_per_frame=3072
formal_stress_gate=true
release_exe_sha256=08D30ED6753E97EA6605E8553B0C82B06B9A8E4818C6461ABBFD0C0F499F340B
debug_symbols_sha256=4BD0BA27B6A3B6170B6519B2771FDA95144DBB5DCD46A6DB6E41896CAFD67D2E
local_dev_zip_sha256=2781B8B0AC9BF2EF53A6D799081A5C55CBB5F882BDE30226E07DB68CD96F3FA8
symbols_zip_sha256=F4B6159EB1574AEDB98F9E3B03BD565D29935607003FDDA2469CEB755F4C5EF0
zip_audit_pass=true
authenticode_signed=false
trusted_gui_matrix=not_tested
source_commit_public=false
anonymous_clone_pass=false
github_release_created=false
release_ready=false
```

未创建 tag、未推送、未发布、未改远端。`GATE-020-SAVES`、`GATE-020-CHALLENGES` 和本轮针对四条链的运行/压力子门禁已有通过证据，但 `GATE-020-RELEASE` 仍不通过，所以不能宣称 `0.2.0` 已公开发布或整体 `release_ready=true`。
