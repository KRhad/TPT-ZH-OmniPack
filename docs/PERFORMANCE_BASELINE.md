# 性能与稳定性基线

本文件定义可重复的性能采样、固定场景和发布判定。当前已有局部事件预算、启动稳定性以及一次混合/载体 OPS 双往返证据，但没有 FPS、CPU、内存、粒子增长、保存或加载耗时数据；因此当前性能基线仍为 `not_tested`，不能由 clean build、字段往返或 Lua 反应回归替代。

## 当前快照

```text
audit_head=e18abad9753e61e8f6c9f8fdf671c4bd80a4ca53
implementation_commit=c743db2fcc49c01033e68023cceff897ed4c35f6
candidate_version=0.1.0-test
candidate_exe_sha256=05DACBFCC31D6F1920D4437DB60629A1F9DA79CC0A14AA13138DD97393CCF6AF
candidate_zip_sha256=943DA2A60C0B371A1D3F921FEC525FB3F7B5AEBC7C5CE7775A8AEFA883C13F14
stress_samples_passed=0
stress_samples_total=10
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
| CPU、物理内存、电源模式 | `not_tested`；正式采样前必须记录 |

私有候选从 ZIP 解压后使用 20 个全新隔离数据目录启动均响应、崩溃 0。这证明启动稳定性，不提供持续运行、FPS 或内存基线。

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

所有样本当前均未生成可复核 OPS 或性能结果。

| 样本 ID | 固定场景 | 必需活动与故障点 | 运行时长 | 当前状态 |
|---|---|---|---:|---|
| `S01-METALLURGY-LARGE` | 大型冶金工厂 | 原料熔化、合金、炼钢、炉渣与碎料回收同时运行 | ≥10 分钟 | 尚未测试 |
| `S02-FURNACES-PARALLEL` | 多熔炉并行 | 多个 `CRUC` 炭化/炼焦及多组合金达到事件高负载 | ≥10 分钟 | 尚未测试 |
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
| `S01` | `not_tested` | `not_tested` | `not_tested` | `not_tested` | `not_tested` | `not_tested` | 尚未测试 |
| `S02` | `not_tested` | `not_tested` | `not_tested` | `not_tested` | `not_tested` | `not_tested` | 尚未测试 |
| `S03` | `not_tested` | `not_tested` | `not_tested` | `not_tested` | `not_tested` | `not_tested` | 尚未测试 |
| `S04` | `not_tested` | `not_tested` | `not_tested` | `not_tested` | `not_tested` | `not_tested` | 尚未测试 |
| `S05` | `not_tested` | `not_tested` | `not_tested` | `not_tested` | `not_tested` | `not_tested` | 尚未测试 |
| `S06` | `not_tested` | `not_tested` | `not_tested` | `not_tested` | `not_tested` | `not_tested` | 尚未测试 |
| `S07` | `not_tested` | `not_tested` | `not_tested` | `not_tested` | `not_tested` | `not_tested` | 尚未测试 |
| `S08` | `not_tested` | `not_tested` | `not_tested` | `not_tested` | `not_tested` | `not_tested` | 尚未测试 |
| `S09` | `not_tested` | `not_tested` | `not_tested` | `not_tested` | `not_tested` | `not_tested` | 尚未测试 |
| `S10` | `not_tested` | `not_tested` | `not_tested` | `not_tested` | `not_tested` | `not_tested` | 尚未测试 |

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
