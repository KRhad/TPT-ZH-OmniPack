# 性能与稳定性基线

本文件定义可重复的性能采样、固定场景和发布判定。当前已实现十场景的隔离 Lua/PowerShell 采样工具，并能生成逐秒帧/粒子序列、Windows 进程 CPU/内存序列、两份 OPS、保存加载耗时和机器可读 JSON。短烟测只验证工具链；S01/S02 已完成 60 秒预热和 600 秒采样，但模块事件计数、场景停止/恢复断言及其余 8 个样本尚未完成，因此正式性能基线仍为 `not_tested`。

## 当前快照

```text
audit_head=4f5c07f9243b2ad04c8dbeb8b9c1887d9812c60a
implementation_commit=4f5c07f9243b2ad04c8dbeb8b9c1887d9812c60a
candidate_version=0.1.0-test
candidate_exe_sha256=D29E67762E3A6C592E84B2FF3D5FAB47958C7BB79336D3D2C9AE3CCDC9C5BFB2
candidate_zip_sha256=0DF8695EE9D28D61C7F076EF199831BA953117B632043948E85AF6A3BBACB051
stress_samples_passed=0
stress_samples_total=10
harness_smoke_scenarios=10/10
responsive_harness_smoke_scenarios=1/10
full_sample_executions=2/10
sample_executions_passed=2/10
full_samples_assessed=2/10
ops_mixed_carrier_roundtrip=true
stress_test=not_tested
long_run_test=not_tested
```

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

当前 ZIP 解压后 EXE 已从全新隔离目录启动并正常退出，且所有运行工具都使用独立 `ddir`。这证明当前运行路径和隔离策略可用，不提供持续运行、FPS 或内存基线。

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
- 当前没有引擎暴露的模块事件计数器，对应 JSON 字段保持 `not_tested`。S01/S02 的只读判定器基于 600 秒序列给出 `unbounded_growth=false` 和 `memory_leak_suspected=false`；这是有限观察分类，不是长期有界性的数学证明，也不会仅根据“进程未崩溃”推断结果。

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
7. 每个样本独立记录崩溃、无响应、用户强制终止、无界增长判定、保存/加载成功和输出 OPS SHA-256。
8. 原始数据写入 `artifacts/performance/<version>/<machine-id>/<sample-id>/`；摘要写回本文件和版本报告。没有原始文件的数值不得作为门禁证据。

### 每个样本的机器可读字段

建议每次运行使用一个 JSON 对象，字段不可省略；未取得的值写字符串 `not_tested`：

```text
schema_version
sample_id
run_id
source_commit
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

十个场景均已有确定性构造器和短烟测 OPS/JSON。正式表只接受完整 60+600 秒运行，因此在完整样本结束并复核前仍全部保持尚未测试。

| 样本 ID | 固定场景 | 必需活动与故障点 | 运行时长 | 当前状态 |
|---|---|---|---:|---|
| `S01-METALLURGY-LARGE` | 大型冶金工厂 | 原料熔化、合金、炼钢、炉渣与碎料回收同时运行 | 60 秒预热 + 600 秒采样 | 实际运行确认；完整门禁未通过 |
| `S02-FURNACES-PARALLEL` | 多熔炉并行 | 多个 `CRUC` 炭化/炼焦及多组合金达到事件高负载 | 60 秒预热 + 600 秒采样 | 实际运行确认；完整门禁未通过 |
| `S03-ECOLOGY-AREA` | 大面积生态循环 | 藻类、菌丝、孢子、营养和腐殖质在完整模式长期循环 | ≥10 分钟 | 尚未测试 |
| `S04-PATHOGEN-CONTROL` | 病原体传播与消毒 | `PATH` 扩散、`STER` 消毒和湿 `BIOF` 过滤；确认可停止 | ≥10 分钟 | 尚未测试 |
| `S05-CHEMISTRY-DENSE` | 高密度化学反应 | 裂化、聚合、氨、肥料、过氧化物和发酵并行 | ≥10 分钟 | 尚未测试 |
| `S06-NEUTRON-GENERATORS` | 多中子发生器 | 多个 `SPRK(NGEN)` 脉冲、有/无燃料和满输出槽负例 | ≥10 分钟 | 尚未测试 |
| `S07-REACTOR-STABLE` | 稳定反应堆 | 燃料、慢化、控制、冷却和屏蔽稳定运行 | ≥10 分钟 | 尚未测试 |
| `S08-REACTOR-LOCA` | 失冷反应堆 | 切断冷却、废料升温、停堆和恢复；不得不可控增长 | ≥10 分钟 | 尚未测试 |
| `S09-ALL-MODULES` | 四模块同时活动 | 冶金、生态、化学、核工业同时达到持续负载 | ≥10 分钟 | 尚未测试 |
| `S10-CARRIERS-ROUNDTRIP` | 大量间接元素引用 | `LAVA`、`SPRK`、`MSCR`、`ctype/tmp/tmp2` 大量存在并反复双往返 | ≥10 分钟 | 尚未测试 |

### 当前结果表

| 样本 | 初始/峰值/结束粒子 | 平均/1% low/最低 FPS | 峰值工作集 | 崩溃/卡死 | 无界增长 | OPS 往返 | 结果 |
|---|---|---|---|---|---|---|---|
| `S01` | `49,536 / 49,536 / 13,167` | `60.002 / 52.631 / 46.490` | `152,281,088 bytes` | `false / false` | `false` | `true` | 样本执行通过；事件计数/场景行为未完成 |
| `S02` | `49,536 / 139,323 / 16,520` | `60.002 / 52.633 / 47.177` | `162,676,736 bytes` | `false / false` | `false` | `true` | 样本执行通过；事件计数/场景行为未完成 |
| `S03` | `not_tested` | `not_tested` | `not_tested` | `not_tested` | `not_tested` | `not_tested` | 尚未测试 |
| `S04` | `not_tested` | `not_tested` | `not_tested` | `not_tested` | `not_tested` | `not_tested` | 尚未测试 |
| `S05` | `not_tested` | `not_tested` | `not_tested` | `not_tested` | `not_tested` | `not_tested` | 尚未测试 |
| `S06` | `not_tested` | `not_tested` | `not_tested` | `not_tested` | `not_tested` | `not_tested` | 尚未测试 |
| `S07` | `not_tested` | `not_tested` | `not_tested` | `not_tested` | `not_tested` | `not_tested` | 尚未测试 |
| `S08` | `not_tested` | `not_tested` | `not_tested` | `not_tested` | `not_tested` | `not_tested` | 尚未测试 |
| `S09` | `not_tested` | `not_tested` | `not_tested` | `not_tested` | `not_tested` | `not_tested` | 尚未测试 |
| `S10` | `not_tested` | `not_tested` | `not_tested` | `not_tested` | `not_tested` | `not_tested` | 尚未测试 |

### S01 完整执行证据

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

### S02 完整执行证据

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
performance_measurements_complete=false
stress_samples_passed=0
stress_samples_total=10
stress_test=not_tested
memory_leak_suspected=not_tested
unbounded_growth_detected=not_tested
long_run_test=not_tested
performance_gate_pass=false
```
