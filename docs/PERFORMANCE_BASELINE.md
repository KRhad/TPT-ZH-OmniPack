# 性能与稳定性基线

本文件定义可重复的性能采样、固定场景和发布判定。当前已实现十场景的隔离 Lua/PowerShell 采样工具，并能生成逐秒帧/粒子序列、Windows 进程 CPU/内存序列、两份 OPS、保存加载耗时和机器可读 JSON。当前候选的十项完整 `60+600` 秒执行及独立工件评估均已完成；模块事件计数与停止/恢复断言也已由客户端 API 实际记录，十项 `performance_gate_pass=true`。候选 S09 另完成一次连续 `7200.002183` 秒有限观测并通过独立评估；这仍不是覆盖语言/模块切换等要求的 1.0.0 综合长跑证明。

## 当前快照

```text
candidate_source_commit=ff5945c4acbe15052a316771934854aa0f9281de
implementation_commit=4f5c07f9243b2ad04c8dbeb8b9c1887d9812c60a
harness_commit=ff5945c4acbe15052a316771934854aa0f9281de
candidate_version=0.1.0-test
candidate_exe_sha256=14A00CCF73D5100C43D677572529F6DDCD9A2790FC16FED70136185262B46926
candidate_zip_sha256=53E0304FF8CE932F7D836620A7599085A486B1689EAC131BC78D7B8EA6619827
stress_evidence_source_commit=ff5945c4acbe15052a316771934854aa0f9281de
stress_evidence_zip_sha256=53E0304FF8CE932F7D836620A7599085A486B1689EAC131BC78D7B8EA6619827
stress_samples_passed=10
stress_samples_total=10
s09_7200s_observation=true
s09_7200s_run_id=20260730T210719Z-d4085bc4
s09_7200s_performance_gate=true
harness_smoke_scenarios=10/10
responsive_harness_smoke_scenarios=1/10
full_sample_executions=10/10
sample_executions_passed=10/10
full_samples_assessed=10/10
event_evidence_complete=true
scenario_behavior_pass=true
stress_event_total=22529
stress_peak_event_per_frame=1024
ops_mixed_carrier_roundtrip=true
stress_test=true
long_run_test=not_tested
```

## 0.2.0-dev 针对性正式样本

本地候选 `32336e66`、EXE `08D30ED6...F340B` 和普通包 `2781B8B0...3FA8` 另行顺序完成更新后四条跨模块链对应的 S04/S05/S07/S09。四项均为实际 `60+600` 秒、独立 `assessment.json` 判定 `performance_gate_pass=true`，总事件 `30557`、最大单帧事件 `3072`；每项 `stop_event_delta=0`、恢复断言 `7`，有限观察无界增长和内存泄漏怀疑均为 `false`。精确 run ID、FPS、粒子数、哈希和边界见 `docs/PHASE_0_2_EVIDENCE.md`。该增量不改写上方 0.1 十样本基线，也不替代 1.0 两小时综合长跑。

当前记录的环境只作为未来采样的机器标识，不是低端硬件代表：

| 项目 | 当前记录 |
|---|---|
| OS | Windows 11 Pro x64 `10.0.26200` |
| 系统语言 | `zh-CN` |
| 显示 | `1920x1080`、32-bit |
| DPI | `LogPixels=192`，150% |
| GPU | NVIDIA GeForce RTX 5070 Ti Laptop GPU |
| 编译器 | MSYS2 UCRT64 GCC 16.1.0 |
| Meson / Ninja | 1.11.2 / 1.13.2 |
| SDL / JsonCpp | 2.30.9-tpt-libs / 1.9.5-tpt-libs |
| 固定依赖 | `tpt-libs v20251019131007` |
| CPU | Intel Core Ultra 9 275HX；24 logical CPUs |
| 物理内存 | 33,784,102,912 bytes |
| 电源模式 | Windows 高性能方案 `8c5e7fda-e8bf-4a96-9a85-a6e23a8c635c` |

当前 ZIP 解压后 EXE 已从全新隔离目录启动并正常退出，且所有运行工具都使用独立 `ddir`。十项持续运行、FPS 和内存原始序列另见下文；隔离启动本身不替代窗口内容、语言或 DPI 的人工视觉证据。

## 采样工具与已发现失败

运行入口：

```powershell
.\tools\runtime_stress_test.ps1 `
  -Executable <从当前普通 ZIP 解压的 tpt-zh-omnipack.exe> `
  -PackageZip .\dist\TPT-ZH-OmniPack-0.1.0-test-Windows-x64.zip `
  -SampleId S01-METALLURGY-LARGE
```

- 默认参数严格使用 60 秒预热、600 秒正式采样和密度步长 3；`-Smoke` 只运行 2 秒、使用步长 12，并强制写 `gate_result=not_tested`。
- 每次运行预创建仅含 `{}` 的隔离 `powder.pref`，避免首次启动缩放确认阻塞事件循环；不读取真实用户偏好、账户、图章或存档，也不把该临时偏好复制到证据目录。
- 第一版工具在 Lua autorun 中连续循环。十个 2 秒烟测均完成，但首次 600 秒运行在约 3 秒后被客户端 `LuaHookTimeout` 以“Script not responding”终止。该结果是工具缺陷和真实失败，不计为样本结果。
- 提交 `4f5c07f9` 改为在 `event.tick` 中每次只执行一帧并立即返回。修复后 S01 响应性烟测通过，约 2.004 秒内完成 122 帧，平均约 60.9 次 `sim.updateUpTo`/秒，`Responding=true`；该数值只验证新调度方式，不是冻结性能基线。
- 当前候选通过 `sim.resetOmniEventMetrics()` / `sim.omniEventMetrics()` 暴露并记录四个 Omni 模块的成功预算消耗；每项结果同时记录总数、单帧峰值、停止后的残余事件和七项恢复字段断言。十项只读判定器均给出 `unbounded_growth=false` 和 `memory_leak_suspected=false`；这是有限观察分类，不是长期有界性的数学证明，也不会仅根据“进程未崩溃”推断结果。

## 源码中的有界机制

| 系统 | 邻域 | 每帧成功事件预算 | 同帧级联保护 | 证据类型 |
|---|---|---:|---|---|
| 工业冶金 | 固定 `3x3` | 2,048 | tick 标记 | 源码确认、自动回归 |
| 局部生态 | 固定 `3x3` | 1,024 | tick 标记 | 源码确认、自动回归 |
| 高级化学 | 固定 `3x3` | 1,536 | tick 标记 | 源码确认、自动回归 |
| 受控核工业 | 固定 `3x3` | 512 | tick 标记；发生器每次火花脉冲最多一粒中子 | 源码确认、自动回归 |

这些预算限制单帧成功事件，但没有证明满粒子图下的扫描成本、平均/最低 FPS、工作集峰值或长时间内存趋势。性能保护、自动化、特殊物理和灾害计数器目前未实现，状态为 `not_tested`。

## 固定采样协议

每个性能结果必须绑定同一套输入，禁止用不同构建、不同存档或不同硬件的数值直接比较。

1. 从待测普通 ZIP 解压到空目录，核对 ZIP 与 EXE SHA-256；不从构建目录直接运行。
2. 为每个样本创建独立空用户目录，不读取或写入真实 `powder.pref`、图章和个人存档。
3. 记录源码提交、tag、版本、EXE/OPS 哈希、OS build、CPU、GPU、物理内存、显示/DPI、电源模式、是否接通电源、语言、模块状态和性能保护状态。
4. 使用项目版本生成并保存固定 OPS；将随机种子、模拟尺寸、初始粒子数和初始粒子类型计数写入场景清单。若引擎不能固定随机种子，明确记录并至少重复 3 次取中位数。
5. 加载后预热 60 秒；正式采样至少 10 分钟。每秒记录进程 CPU、工作集、私有字节、粒子数和可用事件计数；逐帧或以引擎计数器记录 FPS。
6. 样本开始和结束各执行一次保存/加载；需要往返的样本按“保存→退出→重启→加载→再保存→再加载”执行。
7. 每个样本独立记录崩溃、无响应、用户强制终止、模块成功事件总数/单帧峰值、停止后的残余事件、恢复标记字段断言、无界增长判定、保存/加载成功和输出 OPS SHA-256。
8. 原始数据写入 `artifacts/performance/<version>/<machine-id>/<sample-id>/`；摘要写回本文件和版本报告。没有原始文件的数值不得作为门禁证据。

### 每个样本的机器可读字段

建议每次运行使用一个 JSON 对象，字段不可省略；未取得的值写字符串 `not_tested`：

```text
schema_version
sample_id
run_id
source_commit
harness_commit
release_tag
version
public_zip_sha256
exe_sha256
input_ops_sha256
output_ops_first_sha256
output_ops_second_sha256
machine_id
os_build
cpu_model
logical_cpu_count
physical_memory_bytes
gpu_model
power_mode
display_resolution
dpi_percent
language
enabled_modules
performance_protection
random_seed
warmup_seconds
sample_seconds
initial_particles
peak_particles
final_particles
average_fps
one_percent_low_fps
minimum_fps
average_cpu_percent
peak_working_set_bytes
peak_private_bytes
event_count_total
event_count_peak_per_frame
scenario_stop_pass
scenario_recovery_pass
stop_event_delta
scenario_recovery_assertions
save_time_first_ms
load_time_first_ms
save_time_second_ms
load_time_second_ms
crashed
hung
unbounded_growth
roundtrip_pass
notes
```

`notes` 只能补充解释，不能把失败改写为通过。

## 0.1.0-test 十个固定样本

十个场景均已有确定性构造器和短烟测 OPS/JSON。下表的所有当前状态均绑定同一候选 ZIP、同一 EXE 和同一采样器提交；每项均完成完整 `60+600` 秒运行、通过独立工件评估，并满足事件/停止/恢复的完整门禁。

| 样本 ID | 固定场景 | 必需活动与故障点 | 运行时长 | 当前状态 |
|---|---|---|---:|---|
| `S01-METALLURGY-LARGE` | 大型冶金工厂 | 原料熔化、合金、炼钢、炉渣与碎料回收同时运行 | 60 秒预热 + 600 秒采样 | 实际运行确认；独立评估和完整门禁通过 |
| `S02-FURNACES-PARALLEL` | 多熔炉并行 | 多个 `CRUC` 炭化/炼焦及多组合金达到事件高负载 | 60 秒预热 + 600 秒采样 | 实际运行确认；独立评估和完整门禁通过 |
| `S03-ECOLOGY-AREA` | 大面积生态循环 | 藻类、菌丝、孢子、营养和腐殖质在完整模式长期循环 | 60 秒预热 + 600 秒采样 | 实际运行确认；独立评估和完整门禁通过 |
| `S04-PATHOGEN-CONTROL` | 病原体传播与消毒 | `PATH` 扩散、`STER` 消毒和湿 `BIOF` 过滤；确认可停止 | 60 秒预热 + 600 秒采样 | 实际运行确认；独立评估和完整门禁通过 |
| `S05-CHEMISTRY-DENSE` | 高密度化学反应 | 裂化、聚合、氨、肥料、过氧化物和发酵并行 | 60 秒预热 + 600 秒采样 | 实际运行确认；独立评估和完整门禁通过 |
| `S06-NEUTRON-GENERATORS` | 多中子发生器 | 多个 `SPRK(NGEN)` 脉冲、有/无燃料和满输出槽负例 | 60 秒预热 + 600 秒采样 | 实际运行确认；独立评估和完整门禁通过 |
| `S07-REACTOR-STABLE` | 稳定反应堆 | 燃料、慢化、控制、冷却和屏蔽稳定运行 | 60 秒预热 + 600 秒采样 | 实际运行确认；独立评估和完整门禁通过 |
| `S08-REACTOR-LOCA` | 失冷反应堆 | 切断冷却、废料升温、停堆和恢复；不得不可控增长 | 60 秒预热 + 600 秒采样 | 实际运行确认；独立评估和完整门禁通过 |
| `S09-ALL-MODULES` | 四模块同时活动 | 冶金、生态、化学、核工业同时达到持续负载 | 60 秒预热 + 600 秒采样 | 实际运行确认；独立评估和完整门禁通过 |
| `S10-CARRIERS-ROUNDTRIP` | 大量间接元素引用 | `LAVA`、`SPRK`、`MSCR`、`ctype/tmp/tmp2` 大量存在并反复双往返 | 60 秒预热 + 600 秒采样 | 实际运行确认；独立评估和完整门禁通过 |

### 当前结果表

| 样本 | 初始/峰值/结束粒子 | 平均/1% low/最低 FPS | 峰值工作集 | 事件总数/单帧峰值 | 崩溃/卡死 | 增长/泄漏（有限观察） | OPS / 停止恢复 | 结果 |
|---|---|---|---:|---:|---|---|---|---|
| `S01` | `49,536 / 49,536 / 12,971` | `60.001 / 55.235 / 49.638` | `150,118,400` | `1,557 / 243` | `false / false` | `false / false` | `true / true` | 完整门禁 `true` |
| `S02` | `49,536 / 138,927 / 16,520` | `60.002 / 52.940 / 39.206` | `142,217,216` | `0 / 0` | `false / false` | `false / false` | `true / true` | 完整门禁 `true` |
| `S03` | `33,024 / 33,024 / 15,056` | `60.002 / 55.350 / 34.483` | `137,564,160` | `4,449 / 693` | `false / false` | `false / false` | `true / true` | 完整门禁 `true` |
| `S04` | `33,024 / 33,024 / 6,605` | `60.002 / 55.553 / 51.425` | `166,432,768` | `6,541 / 1,024` | `false / false` | `false / false` | `true / true` | 完整门禁 `true` |
| `S05` | `33,024 / 33,024 / 5,516` | `60.002 / 55.552 / 40.809` | `137,826,304` | `229 / 8` | `false / false` | `false / false` | `true / true` | 完整门禁 `true` |
| `S06` | `43,206 / 43,758 / 42,188` | `60.002 / 49.999 / 43.476` | `140,804,096` | `2,402 / 512` | `false / false` | `false / false` | `true / true` | 完整门禁 `true` |
| `S07` | `82,560 / 82,560 / 75,962` | `60.001 / 41.667 / 37.037` | `180,563,968` | `2,048 / 512` | `false / false` | `false / false` | `true / true` | 完整门禁 `true` |
| `S08` | `41,280 / 41,280 / 32,000` | `60.001 / 50.116 / 45.324` | `140,611,584` | `2,048 / 512` | `false / false` | `false / false` | `true / true` | 完整门禁 `true` |
| `S09` | `43,929 / 43,929 / 24,461` | `60.001 / 52.631 / 47.308` | `168,517,632` | `3,255 / 525` | `false / false` | `false / false` | `true / true` | 完整门禁 `true` |
| `S10` | `16,512 / 16,512 / 1,298` | `60.001 / 31.732 / 30.218` | `135,323,648` | `0 / 0` | `false / false` | `false / false` | `true / true` | 完整门禁 `true` |

### 当前候选的来源与评估

上述十项均由普通包 `53E0304FF8CE932F7D836620A7599085A486B1689EAC131BC78D7B8EA6619827` 中与 `14A00CCF73D5100C43D677572529F6DDCD9A2790FC16FED70136185262B46926` 匹配的 EXE 执行，`source_commit=ff5945c4acbe15052a316771934854aa0f9281de`，`harness_commit=ff5945c4acbe15052a316771934854aa0f9281de`。每个运行目录包含 `result.json`、逐秒 `frame-series.csv`、进程 `process-series.csv`、两份 OPS 和 `assessment.json`；十份评估均为 `assessment_status=PASS`、`sample_execution_pass=true`、`event_evidence_complete=true`、`scenario_behavior_pass=true`、`unbounded_growth=false`、`memory_leak_suspected=false`。

| 样本 | 当前候选 run ID |
|---|---|
| `S01` | `20260730T180440Z-784d734c` |
| `S02` | `20260730T181609Z-1519fcf4` |
| `S03` | `20260730T182740Z-1b06cb19` |
| `S04` | `20260730T183905Z-2586b0be` |
| `S05` | `20260730T185034Z-fabbdd4c` |
| `S06` | `20260730T190158Z-7a7786ba` |
| `S07` | `20260730T191326Z-f68af76a` |
| `S08` | `20260730T192453Z-9131600a` |
| `S09` | `20260730T193619Z-d9f707f9` |
| `S10` | `20260730T194745Z-2bd2d539` |

当前正式样本的最低观测为 S10 的 `30.218` FPS；本版本没有绝对 FPS 通过阈值，因此该数值只如实记录。十项的 `event_evidence_complete=true`、`scenario_behavior_pass=true`、`performance_gate_pass=true`，总事件 `22529`、最大单帧峰值 `1024`、`stress_samples_passed=10`、`stress_test=true`。每项停止后 `stop_event_delta=0`、恢复断言为 `7`。有限观察的 `false` 值不构成长期有界性的数学证明。候选 S09 长跑工件为 `artifacts/performance/0.1.0-test-long-run/.../S09-ALL-MODULES/20260730T210719Z-d4085bc4/`，采样 `7200.002183` 秒，独立评估 `performance_gate_pass=true`；它不包含 1.0.0 要求的语言/模块切换等综合步骤。

### 早期候选 S01 历史证据（不计入当前候选）

```text
run_id=20260730T135615Z-7ce0af06
source_commit=4f5c07f9243b2ad04c8dbeb8b9c1887d9812c60a
public_zip_sha256=0DF8695EE9D28D61C7F076EF199831BA953117B632043948E85AF6A3BBACB051
exe_sha256=D29E67762E3A6C592E84B2FF3D5FAB47958C7BB79336D3D2C9AE3CCDC9C5BFB2
warmup_seconds=60.008954
sample_seconds=600.001172
initial_particles=49536
sample_particle_min=13167
sample_particle_max=13177
final_particles=13167
average_fps=60.001549
one_percent_low_fps=52.631431
minimum_fps=46.490251
average_cpu_percent=0.866442
peak_working_set_bytes=152281088
peak_private_bytes=143712256
save_time_first_ms=127.002716
load_time_first_ms=17.000198
save_time_second_ms=49.002886
load_time_second_ms=8.999109
input_ops_sha256=92E589FA72F3F231AD44B26C1B2588797996C13D0571442FC5AC7147C91E805A
output_ops_second_sha256=98670969419183BCE13B9987AAFD3F0BE7AFF582F14120225F8925AA1FC09DB8
crashed=false
hung=false
roundtrip_pass=true
unbounded_growth=false
memory_leak_suspected=false
sample_execution_pass=true
gate_result=incomplete_event_and_growth_evidence
```

原始数据位于 `artifacts/performance/0.1.0-test/DESKTOP-14BQH2Q-276049E7945C/S01-METALLURGY-LARGE/20260730T135615Z-7ce0af06/`：595 条逐秒帧/粒子记录、1,289 条进程记录以及两份 OPS。`tools/analyze_stress_result.py` 已核对原始 JSON、CSV、OPS1/BZip2 和两份 SHA-256；后半段粒子观测不是“非递减且至少一次增加”，工作集与私有字节最后四分之一也未同时满足该有限观察规则，因此输出 `unbounded_growth=false`、`memory_leak_suspected=false`、`sample_execution_pass=true`。该规则不是长期有界性的数学证明；事件计数和场景停止/恢复仍缺失，所以 `performance_gate_pass=false`。

### 早期候选 S02 历史证据（不计入当前候选）

```text
run_id=20260730T140824Z-4564e521
source_commit=4f5c07f9243b2ad04c8dbeb8b9c1887d9812c60a
public_zip_sha256=0DF8695EE9D28D61C7F076EF199831BA953117B632043948E85AF6A3BBACB051
exe_sha256=D29E67762E3A6C592E84B2FF3D5FAB47958C7BB79336D3D2C9AE3CCDC9C5BFB2
warmup_seconds=60.013020
sample_seconds=600.000381
initial_particles=49536
peak_particles=139323
final_particles=16520
average_fps=60.001629
one_percent_low_fps=52.632752
minimum_fps=47.176838
average_cpu_percent=0.567436
peak_working_set_bytes=162676736
peak_private_bytes=157003776
save_time_first_ms=84.499121
load_time_first_ms=10.999203
save_time_second_ms=55.009127
load_time_second_ms=8.121014
input_ops_sha256=4B24731BC81F02E9EBD9B9B1F1C247E6B59A9A9CBF42A7A6AD380BA1256F1528
output_ops_second_sha256=3FE68AB6CA58D4DE9F0CDB1F3D590E4B6CD4765E9517AEA3EB8D5D26930D144D
crashed=false
hung=false
roundtrip_pass=true
unbounded_growth=false
memory_leak_suspected=false
sample_execution_pass=true
performance_gate_pass=false
```

原始数据位于 `artifacts/performance/0.1.0-test/DESKTOP-14BQH2Q-276049E7945C/S02-FURNACES-PARALLEL/20260730T140824Z-4564e521/`。正式采样的 595 条粒子记录全部为 16,520；预热反应期曾达到 139,323。结束保存/加载阶段提高了进程内存峰值，但最后四分之一的工作集和私有字节并非同时逐样本单调增加，有限观察分类为 `memory_leak_suspected=false`。

## 0.3.0-dev 自动化正式样本

候选普通 ZIP `BB57D87F...07EA`、EXE `FBCA1387...0FF2`。两项通过样本均为串行单客户端、60 秒预热加 600 秒采样，并由 `analyze_stress_result.py` 独立评估。

| 样本 / Run ID | 实际秒数 | 平均 / 1% low / 最低 FPS | 初始 / 峰值 / 最终粒子 | 模块事件 / 峰值 | 官方信号 / 峰值 | 门禁 |
|---|---:|---:|---:|---:|---:|---|
| S11 `20260731T080609Z-91b43bef` | `60.001 + 600.005` | `60.001 / 47.178 / 39.853` | `74304 / 74304 / 54535` | `5233 / 4096` | `74380132 / 4132` | true |
| S12 `20260731T083137Z-80a982fe` | `60.016 + 600.017` | `52.809 / 29.972 / 22.771` | `148608 / 150220 / 150220` | `0 / 0` | `978876949 / 56421` | true |

两项的崩溃、挂起、有限观察无界增长和内存泄漏怀疑均为 false；停止后事件增量为 0，官方信号停止、场景恢复和双 OPS 往返均通过。S12 的正式粒子序列稳定在 150,220。

S12 首次正式 run `20260731T081741Z-ddd919fe` 明确为失败：一次性火花在预热期耗尽，正式窗口官方信号为 0，`performance_gate_pass=false`。`522dc3bc` 改用官方 BTRY 持续激励后才取得上表通过样本。失败 run 不计入 `2/2`。

精确哈希、提交、尾段与边界见 `docs/PHASE_0_3_EVIDENCE.md`。

## 后续版本场景扩展

固定样本一旦进入已发布基线，不删除、不改义；需要调整时新增场景版本并保留旧哈希。

| 版本 | 新增必需场景 |
|---|---|
| 0.2.0 | 四条跨模块生产链各一组正常、事故、停止和回收；7 个示例 OPS |
| 0.3.0 | 自动恒温、自动合金、燃料处理、营养、消毒、冷却反应堆、紧停、废运和综合自动工厂；信号环高负载 |
| 0.4.0 | 最大配方图搜索、长期炼金进度、反复解锁/保存/迁移 |
| 0.5.0 | 每类灾害的受控、失控、隔离和清理极限样本 |
| 0.6.0 | 最大图鉴索引、冷/热索引构建、混合语言查询和模块切换重建 |
| 0.7.0 | 大型 OPS、每类旧存档迁移、取消/失败/备份及载体映射 |
| 0.8.0 | 低端配置、中型/大型综合工厂、大型生态、多反应堆、灾害失控、自动化高负载、最大索引、大型 OPS 和旧存档迁移 |
| 0.9.0 | 冻结内容的全量回归和崩溃恢复 |
| 1.0.0 | 两小时综合长跑，覆盖自动化、生态、多反应堆、多灾害恢复、反复存取、语言/模块切换、索引和炼金进度 |

“低端配置”必须是真实记录的独立硬件/虚拟机配置；当前 RTX 5070 Ti Laptop 环境不能冒充低端样本。

## 通过与回归规则

### 单样本硬条件

每个样本只有同时满足以下条件才可标为通过：

- 所有必需字段为真实数值或布尔值，不是 `not_tested`；
- `crashed=false`、`hung=false`、`unbounded_growth=false`；
- 场景规定的停止、隔离或恢复操作成功；
- 需要存档的场景 `roundtrip_pass=true`，两次加载后 identifier、稳定 ID 和间接引用一致；
- 粒子数、FPS、CPU、工作集和保存/加载时间原始序列完整；
- 未超过引擎容量且没有因测试脚本失效、窗口暂停或后台限帧产生无效数据。

0.1.0-test 首次建立基线时不凭空设定绝对 FPS 或内存目标，但缺少数据、发生崩溃/卡死、无界增长或 OPS 失败必定不通过。

### 后续同硬件回归

首次有效基线按每个样本至少 3 次运行的中位数冻结。相同机器、电源模式、分辨率、DPI、模拟尺寸和样本 OPS 下，默认阻塞阈值为：

- 平均 FPS 低于基线 90%；
- 1% low FPS 低于基线 85%；
- 峰值工作集或私有字节高于基线 115%；
- 保存或加载中位耗时高于基线 125%；
- 稳态后内存或粒子数呈无上限单调增长；
- 事件计数超过登记的每帧预算。

任何超限必须关联可复现缺陷和修复；不能只在报告中注明“可接受”后继续发布。若场景功能本身合法增加了固定成本，应新增带版本的基线和变更说明，保留旧结果供比较。

## 两小时长时间测试

1.0.0 至少执行一次连续 7,200 秒的综合长跑，并在相同进程中完成：大型自动化、生态、多反应堆、多灾害恢复、至少 10 次保存/加载、至少 10 次中英文切换、至少 10 次四模块关闭/开启、最大图鉴搜索和炼金进度推进。

除通用字段外还记录：

```text
memory_at_warmup_end_bytes=
memory_at_30m_bytes=
memory_at_60m_bytes=
memory_at_90m_bytes=
memory_at_120m_bytes=
particle_count_at_30m=
particle_count_at_60m=
particle_count_at_90m=
particle_count_at_120m=
ops_size_min_bytes=
ops_size_max_bytes=
save_operations=
load_operations=
language_switches=
module_toggle_cycles=
data_consistency_pass=
```

长跑只有在进程无崩溃/卡死、无未解释的持续内存或粒子增长、所有保存/加载成功且最终数据一致时通过。

## 当前门禁结论

```text
performance_measurements_complete=true
stress_samples_executed=10
stress_sample_executions_passed=10
stress_samples_passed=10
stress_samples_total=10
stress_test=true
memory_leak_suspected=false
unbounded_growth_detected=false
event_evidence_complete=true
scenario_behavior_pass=true
stress_event_total=22529
stress_peak_event_per_frame=1024
s09_7200s_observation=true
long_run_test=not_tested
performance_gate_pass=true
```
