# 版本硬门禁

本文件定义从 `0.1.0-test` 到 `1.0.0` 的发布判定。任何版本只有在其全部必需门禁都有精确证据、机器可读报告写入 `release_ready=true` 后才可创建对应 tag。后续版本的代码可以在开发分支继续，但不能绕过尚未通过的前序发布门禁。

## 判定规则

- 门禁状态只能是 `true`、`false`、`not_tested`、精确计数、精确提交或精确 SHA-256。
- 表格结果使用“源码确认、编译确认、自动测试确认、实际运行确认、人工视觉确认、尚未测试”六类证据标记；已执行的失败在标记后写精确 `false`，外部阻塞也单独注明。任何 `false`、尚未测试或外部阻塞均使版本不能发布。
- 当前候选源码为 `ff5945c4acbe15052a316771934854aa0f9281de`，字体实现为 `c743db2fcc49c01033e68023cceff897ed4c35f6`，本次 binary/压力证据均绑定 `ff5945c4acbe15052a316771934854aa0f9281de`；普通包和符号包清单也绑定该提交。一旦影响二进制、包内文档或测试语义的源码变化，相关构建、ZIP、运行、GUI、OPS 和压力证据必须重跑。
- 每个证据至少记录 `source_commit`、命令或操作步骤、环境、开始/结束时间、结果文件和产物 SHA-256。GUI 证据还需记录语言、DPI 和页面；OPS 证据还需记录输入/两次输出哈希与粒子引用核对。
- `release_ready` 是所有适用硬门禁的逻辑与，不允许人工覆写。

## 全版本不变量

| 门禁 ID | 条件 | 核查依据 |
|---|---|---|
| `GATE-ALL-ID` | 官方 ID `0..195` 和已发布 OmniPack identifier/稳定 ID 不漂移；删除内容只保留 tombstone/迁移映射 | `docs/ELEMENT_REGISTRY.csv`、登记审计 |
| `GATE-ALL-REG` | 新元素和反应在实现前完成元素/反应登记，字段完整且审计通过 | 两个 registry 及构建门禁 |
| `GATE-ALL-SAVE` | 禁用、缺失或迁移失败的元素不被静默删除或错误替换；取消/失败不改变沙盘 | OPS 语料与运行测试 |
| `GATE-ALL-LOCAL` | 反应、传播、信号和特殊物理是有界局部计算，具有事件预算和循环保护 | 源码审计、单测、压力测试 |
| `GATE-ALL-PRIVACY` | 普通包不含偏好、账户、令牌、图章、个人存档、测试数据或调试符号 | ZIP 白名单与解压审计 |
| `GATE-ALL-LICENSE` | GPL、字体和所有第三方来源/许可证/修改记录完整 | 许可证清单与来源账本 |
| `GATE-ALL-REPORT` | 报告字段齐全，SHA-256 与实际文件匹配，未执行项不伪装为通过 | `dist/release-report-<version>.md` 解析检查 |

## 0.1.0-test 当前门禁

### 源码、构建与自动测试

| 门禁 ID | 必需结果 | 当前状态 | 当前证据 |
|---|---|---|---|
| `GATE-010-SOURCE` | HEAD、分支、上游和 48 元素登记可重现 | 源码确认 | 候选源码 `ff5945c4`；开发分支 `development/omnipack-1.0`；字体 `c743db2f`；48 个元素固定于 `256..278`、`288..295`、`328..334`、`360..369`；登记表 370 行、243 active、127 reserved |
| `GATE-010-FONT-SOURCE` | 字体来源、许可证、固定哈希、容器和全部语言字符覆盖通过 | 自动测试确认 | `resources/font.bz2` SHA-256 `C13C3D0E...`；Fusion 原生 12px，14,626 字形、2,590 字符覆盖 |
| `GATE-010-BUILD` | Windows x64 clean Release build 成功 | 编译确认 | `build-0.1.0-test-metrics` 的 clean Release 构建通过，0 error；GCC 警告另列已知问题 |
| `GATE-010-MESON` | 全部 Meson 测试通过 | 自动测试确认 | 当前候选 `14/14` |
| `GATE-010-PYTHON` | 全部 Python 工具测试通过且无未说明跳过 | 自动测试确认 | 当前候选 `87/87`，0 skip |
| `GATE-010-LUA` | 最终 ZIP EXE 执行模块及四反应引擎回归 | 实际运行确认 | 已剥离 EXE `6/6`；另有五类 OPS 双往返运行 |
| `GATE-010-STARTUP` | 从 ZIP 解压、全新隔离目录重复启动，无崩溃且进程响应 | 实际运行确认 | 当前 ZIP 解压 EXE 在全新隔离 `ddir` 启动，标题正确、句柄非零、`Responding=true`、正常退出；重复次数 1 |

### 双语与四模块 GUI

| 门禁 ID | 必需结果 | 当前状态 | 完成证据 |
|---|---|---|---|
| `GATE-010-ZH-DEFAULT` | 无偏好首次启动实际显示简体中文 | 尚未测试 | 最终 ZIP 的真实桌面截图/记录；进程启动不替代视觉确认 |
| `GATE-010-LANG-SWITCH` | 英→中、中→英均成功，重启后各自持久化 | 尚未测试 | 两方向操作和两次重启记录 |
| `GATE-010-ZH-FONT` | 中文无乱码、破碎、方框，图鉴和化学文本可读 | 人工视觉确认 | 用户确认当前原生 Fusion 12px 方案的中文显示问题已解决；DPI/完整页面另行验收 |
| `GATE-010-DPI` | 100%、125%、150% 下主界面、设置、图鉴、对话框无严重溢出 | 尚未测试 | 三档 DPI 的中英文页面矩阵 |
| `GATE-010-MODULE-UI` | 四模块开关、搜索、放置、图鉴和重启持久化 | 尚未测试 | 分别验证工业冶金/局部生态/高级化学/受控核工业及 `ALUM/NUTR/CHLR/NFUL` |

### OPS 往返与只读门禁

| 门禁 ID | 必需结果 | 当前状态 | 完成证据 |
|---|---|---|---|
| `GATE-010-OPS-OFFICIAL` | 官方基础 OPS 执行保存→退出→重启→加载→再保存→再加载 | 实际运行确认 | `official` 场景 3 进程、2 重启、2 加载；6 粒子、每次 14 字段断言，官方 identifier/ID 逐项一致 |
| `GATE-010-OPS-MODULES` | 四个单模块与四模块混合 OPS 完成双往返 | 实际运行确认 | `metallurgy/biology/chemistry/nuclear` 单模块 `4/4`，共 12 进程、8 重启、8 加载验证、73 粒子和 106 字段断言；混合场景另用 3 进程双往返通过 |
| `GATE-010-OPS-CARRIERS` | `LAVA`、`SPRK`、`MSCR` 及 `ctype/tmp/tmp2` 间接引用往返不漂移 | 实际运行确认 | 提交 `f92e12fa`：官方、四单模块和混合场景覆盖 `LAVA/SPRK/MSCR/CONV/VIRS` 的 `ctype/tmp/tmp2`；官方加四单模块每次加载合计 120 字段断言，其中四单模块为 106 |
| `GATE-010-LOAD-CHOICE` | 禁用模块提示的正常加载、只读加载、取消均正确 | 尚未测试 | 三项真实 GUI 操作；取消后沙盘哈希/状态不变 |
| `GATE-010-READONLY-SAVE` | 菜单、快捷键、另存、覆盖和退出路径均不覆盖只读源 | 尚未测试 | 五条写入路径及磁盘哈希前后对比 |
| `GATE-010-READONLY-UPLOAD` | 新建上传和在线更新上传均被拦截 | 尚未测试 | 登录/测试环境中的两条控制器路径 |

### 压力、法律与发布

| 门禁 ID | 必需结果 | 当前状态 | 完成证据 |
|---|---|---|---|
| `GATE-010-STRESS` | 十个固定样本全部有时长、粒子数、FPS、内存、崩溃/卡死/增长、OPS 结果 | 实际运行确认 | 当前候选十项均完成 `60+600` 秒，JSON/CSV/OPS 与哈希评估通过；`crashed=false`、`hung=false`、`roundtrip_pass=true`、`unbounded_growth=false`、`memory_leak_suspected=false`；事件总数/单帧峰值、停止残余事件和七项恢复断言均实际记录，十项 `performance_gate_pass=true`（总事件 `22529`，峰值 `1024`）。另有候选 S09 run `20260730T210719Z-d4085bc4` 完成 `7200.002183` 秒有限观测并独立评估通过；不覆盖 1.0.0 综合长跑。 |
| `GATE-010-PAT` | 暴露 PAT 已撤销或轮换，重新扫描无凭据泄露 | 自动测试确认（`false`；外部账户阻塞） | 当前 `secret_scan_pass=false`、`credential_revoked=false`、`credential_rotated=false` |
| `GATE-010-SOURCE-PUBLIC` | 对应源码和 tag 可匿名 HTTPS 克隆并重建 | 源码确认（`false`；外部权限阻塞） | 当前 `source_commit_public=false`、`anonymous_clone_pass=false` |
| `GATE-010-LICENSES` | GPL、字体、第三方来源和 AI 披露随源码/包完整 | 自动测试确认 | 当前普通 ZIP 白名单含 GPL、字体许可证、第三方来源和 AI 披露；ZIP 清单/哈希二审通过；公开对应源码仍受外部门禁阻塞 |
| `GATE-010-BINARY` | 普通 EXE 已剥离、符号分离、无开发路径，PE 安全标志保留 | 自动测试确认 | EXE `14A00CCF73D5100C43D677572529F6DDCD9A2790FC16FED70136185262B46926`；符号 `17CE34385D9F27A610A591E3F76D6E61D9044B02D5791784563F4B5FDEBC7871`；路径、调试段、动态开发运行库及 PE 标志审计通过；未签名 |
| `GATE-010-PACKAGES` | 普通包、符号包及 `.sha256` 生成并解压二审，普通包无用户数据 | 自动测试确认 | 候选 `ff5945c4`：普通 ZIP `53E0304FF8CE932F7D836620A7599085A486B1689EAC131BC78D7B8EA6619827`；符号 ZIP `8FD702E9F9B92E34321226340F9EF3742EFA86FE8C0FA98302CD6CECAAACE48D`；两包清单绑定相同提交且审计通过 |
| `GATE-010-TAG` | tag `v0.1.0-test` 指向报告中的精确提交 | 源码确认（`false`；外部权限阻塞） | 当前无发布 tag，不得提前创建 |
| `GATE-010-RELEASE` | prerelease 已创建且下载物哈希与报告一致 | 源码确认（`false`；外部权限阻塞） | 当前 `github_release_created=false`、`release_ready=false` |

0.1.0-test 当前总判定：

```text
gate_set=GATE-010
gate_pass=false
release_tag=not_tested
release_ready=false
```

## 0.2.0：四模块跨系统联动

| 门禁 ID | 必需结果 | 当前状态 |
|---|---|---|
| `GATE-020-AUDIT` | 48 元素逐项填写生产、主要/次要用途、消耗、副产物、危险、控制、回收、模块、教程、压力风险；孤立项有保留 ID 的处置 | 自动测试确认；`ELEMENT_USAGE_MATRIX.csv` 48/48，当前 21 项仅直接放置、0 项缺回收、1 项未验证声明均有 disposition |
| `GATE-020-REGISTRY` | 新增或修改元素/反应先通过两个 registry 的完整性、ID、来源、许可证、预算和测试审计 | 自动测试确认；43 条反应及元素、内容、用途四项 fail-closed 审计通过；四条最小跨模块路径均已登记 |
| `GATE-020-MET-CHEM` | 冶金—化学链具备正常、误操作、事故、停止和回收路径 | 实际运行确认；`SLAG/ACID/CATA` 正负例、示例 `03`、教程 T04、OPS 往返及包绑定 S05 `60+600` 秒压力均通过 |
| `GATE-020-BIO-CHEM` | 生态—化学链具备正常、误操作、事故、停止和回收路径 | 实际运行确认；完整/简化生物、`PERO/PATH` 与湿 `HUMS/FERT/WATR` 正负例、示例 `01/02`、教程 T01–T03、OPS 及 S04 正式压力通过 |
| `GATE-020-MET-NUCLEAR` | 冶金—核工业链具备结构、屏蔽、控制、冷却、燃料/废料处理闭环 | 实际运行确认；稳定保存 NCRM、加载后 `SPRK(NCRM)` 组装、无效火花负例、示例 `04`、教程 T05、OPS 及 S07 正式压力通过 |
| `GATE-020-WASTE` | 冶金、化学、生态、核废料及污染水/结构均有受限处理或封装路径 | 实际运行确认；完整/缺催化剂/温区负例、示例 `05–07`、教程 T06–T08、OPS 及 S09 正式压力通过 |
| `GATE-020-SAVES` | 7 个项目版本生成的示例 OPS 可加载，清单记录生成版本和 SHA-256 | 实际运行确认；`7/7` 为 OPS1/BZip2，稳定加载 ID `0200000001..7`，字节数与 SHA-256 审计和重载通过 |
| `GATE-020-CHALLENGES` | 8 项教程/挑战均有目标、初始存档、提示、成功/失败、结果和下一项，且可实际完成 | 实际运行确认；双语结构审计与候选 EXE 解题 `8/8`，总 51 条断言通过 |
| `GATE-020-TESTS` | 每条链的自动反应测试、OPS 往返、错误/事故/回收和压力场景通过 | 自动/实际运行确认；Meson `15/15`、Python `109/109`、Lua 6 次、OPS 场景 `6/6`、教程 `8/8`；S04/S05/S07/S09 正式样本 `4/4 gate=true`，总事件 `30557` |
| `GATE-020-RELEASE` | 全量回归、双语 GUI、包审计、源码/tag/匿名克隆及 `v0.2.0` 发布通过 | `false`；`0.2.0-dev/local-dev` 包审计和解压启动通过，但可信双语 GUI、授权公开源码、匿名克隆、tag 和 GitHub Release 未测试/未执行 |

0.2 本地功能子门禁已有证据，但总判定仍由 `GATE-020-RELEASE=false` 拉低：`release_ready=false`。精确候选、哈希、教程报告和四项压力 run ID 见 `docs/PHASE_0_2_EVIDENCE.md`。

## 0.3.0：自动化和工程控制

| 门禁 ID | 必需结果 | 当前状态 |
|---|---|---|
| `GATE-030-DESIGN` | 对传感、过滤、延迟、计数、存储、阀门、加料、排废、停机、报警和联锁逐项证明复用官方元件或登记必要新增 | 源码/自动测试确认；11/11 能力复用官方元件，17 个官方源码指纹，新增元素 0，`392..423` 保留 |
| `GATE-030-SCENARIOS` | 自动恒温熔炉、合金、燃料、营养、消毒、冷却反应堆、紧停、废料转运和综合工厂可运行 | 实际运行确认；9/9 场景绑定候选 EXE 生成、重载并通过 |
| `GATE-030-BOUNDS` | 无每帧全图扫描；扫描半径、事件预算、循环保护、故障状态和性能计数器均有自动测试 | 源码/自动测试确认；只组合官方局部电子行为，模块事件与官方信号指标、停止和恢复字段 fail-closed |
| `GATE-030-INTERLOCK` | 自动化不能绕过模块禁用、只读存档或改变官方电子行为 | 源码/自动测试确认；未新增选择/保存入口，未修改官方电子状态机，现有模块和只读门禁保持统一入口 |
| `GATE-030-CHALLENGES` | 自动控温、加料、分拣、消毒、紧急停机和多模块自动工厂挑战均可完成 | 实际运行确认；6/6 挑战、总断言 95、停止后事件增量 0 |
| `GATE-030-STRESS` | 自动化高负载和信号环压力样本无崩溃、卡死、无界事件或不可停止回路 | 实际运行确认；S11/S12 正式 `60+600` 秒 `2/2 gate=true`，停止增量 0，有限观察增长/泄漏均为 false |
| `GATE-030-RELEASE` | 全量回归及 `v0.3.0` 公开发布门禁通过 | `false`；本地 clean build、自动回归、包审计和功能压力通过，但可信 GUI、授权公开源码、匿名克隆、tag 和 GitHub Release 未完成 |

0.3 本地功能子门禁已有证据，但总判定仍由 `GATE-030-RELEASE=false` 拉低：`release_ready=false`。精确候选、哈希、失败样本、S11/S12 run ID 和边界见 `docs/PHASE_0_3_EVIDENCE.md`。

## 0.4.0：炼金探索与科技解锁

| 门禁 ID | 必需结果 | 当前状态 |
|---|---|---|
| `GATE-040-MODE` | 炼金探索是独立模式；普通沙盒默认自由且回归通过 | 源码/实际运行确认；默认关闭，free 模式 10 条创建断言通过，炼金锁定不泄漏到普通沙盒 |
| `GATE-040-GRAPH` | 初始元素和十阶段配方图有版本化数据，所有必需节点可达且无循环依赖死锁 | 自动确认；schema 1、四初始 identifier、十阶段，`graph_acyclic=true`，拒绝数字 ID、前向依赖和不可达输入 |
| `GATE-040-CONDITIONS` | 解锁条件实际覆盖温度、压力、电流、催化、时间、结构、冷却、过滤和多阶段过程 | 自动/实际运行确认；九类条件完整，模块开/关各十阶段共 4,620 帧均完成 |
| `GATE-040-PERSIST` | 发现、通知、提示、解锁树、记录、重置及多存档进度保存/加载/迁移通过 | 源码/自动/OPS 确认；严格 `omniAlchemy`、identifier-only、多存档隔离、记录/提示/树契约及损坏状态 fail-closed 通过；窗口视觉另列未测试 |
| `GATE-040-NO-BYPASS` | 搜索、收藏和普通 Lua 接口不能提前使用未解锁元素 | 实际运行确认；锁定 11、自由 10、stamp 1，共 22 条断言覆盖创建、改型、属性、刷/线/框/填充、工具和 stamp |
| `GATE-040-MODULES` | 关闭模块的行为明确，旧进度可迁移，失败不损坏进度 | 实际运行确认；modules-on/off 各十阶段；隐藏解锁保留；损坏/未知 schema 回到四初始元素且不改来源存档 |
| `GATE-040-LONGRUN` | 长期进度反复保存加载无丢失、重复解锁或死局 | 自动确认；精通状态连续 100 次真实 `Serialise -> Parse -> Import` 逐字段一致；仅满足 0.4 进度往返，不替代 1.0 两小时综合长跑 |
| `GATE-040-RELEASE` | 全量回归及 `v0.4.0` 公开发布门禁通过 | `false`；clean build、全量自动回归、本地包和二审通过，但可信 GUI/DPI、授权公开源码、匿名克隆、tag 与 GitHub Release 未完成 |

0.4 本地功能子门禁已有证据，但总判定仍由 `GATE-040-RELEASE=false` 拉低：`release_ready=false`。精确提交、二进制/ZIP 哈希和证据边界见 `docs/PHASE_0_4_EVIDENCE.md`。

## 0.5.0：灾害和特殊物理

| 门禁 ID | 必需结果 | 当前状态 |
|---|---|---|
| `GATE-050-REGISTRY` | 每种危险登记触发、扩散、寿命/预算、控制、隔离、清理、防护、性能上限和测试 | 尚未测试 |
| `GATE-050-BOUNDED` | 不存在单粒无限复制、全图无界传播、永久不可清理污染或无预算高能粒子 | 尚未测试 |
| `GATE-050-CONTROL` | 每种危险的停止、隔离和清理路径都通过正常/失败/资源受限测试 | 尚未测试 |
| `GATE-050-CHALLENGES` | 火灾、病原体、冷却、核事故、化学泄漏、灰蛊及设施恢复挑战可完成 | 尚未测试 |
| `GATE-050-SAVE` | 灾害不会损坏 OPS 格式或把强制规则泄漏到普通沙盒 | 尚未测试 |
| `GATE-050-STRESS` | 每类灾害极限样本在预算上限内运行且可恢复 | 尚未测试 |
| `GATE-050-RELEASE` | 全量回归及 `v0.5.0` 公开发布门禁通过 | 尚未测试 |

## 0.6.0：界面、图鉴与内容管理

| 门禁 ID | 必需结果 | 当前状态 |
|---|---|---|
| `GATE-060-SEARCH` | 中英文名、代号、identifier、用途、来源、模块、状态、危险、最近和收藏检索准确 | 尚未测试 |
| `GATE-060-RECIPES` | 正向/反向配方、“如何获得”“能做什么”、关联跳转和生产链可视化与 registry 一致 | 尚未测试 |
| `GATE-060-STATE` | 未解锁和禁用模块状态准确，切换后索引立即一致 | 尚未测试 |
| `GATE-060-ENTRY` | 示例存档、教程、挑战和性能风险入口可达且不显示裸内部枚举 | 尚未测试 |
| `GATE-060-ACCESS` | 中英文、高 DPI、键盘、滚动长说明和窄窗口布局通过人工矩阵 | 尚未测试 |
| `GATE-060-PERF` | 最大索引构建与查询有数值基线，无交互卡死 | 尚未测试 |
| `GATE-060-RELEASE` | 全量回归及 `v0.6.0` 公开发布门禁通过 | 尚未测试 |

## 0.7.0：存档迁移和兼容性

| 门禁 ID | 必需结果 | 当前状态 |
|---|---|---|
| `GATE-070-CORPUS` | 官方 100.0、历版 OmniPack、汉化分支及可合法分析来源形成带来源/许可证/哈希的语料库 | 尚未测试 |
| `GATE-070-LEVELS` | 每个来源明确标记完全兼容、自动迁移、部分迁移、只读或无法兼容 | 尚未测试 |
| `GATE-070-MAPPING` | 稳定 identifier、旧 ID 和所有间接引用使用同一版本化映射 | 尚未测试 |
| `GATE-070-SAFETY` | 迁移前备份原文件；取消或失败不改变原 OPS/当前沙盘；不静默替换 | 尚未测试 |
| `GATE-070-LOG` | 缺失元素提示、迁移日志及失败原因可查看 | 尚未测试 |
| `GATE-070-ROUNDTRIP` | 每个兼容等级都有成功、失败、取消、备份和二次加载测试 | 尚未测试 |
| `GATE-070-RELEASE` | 全量回归及 `v0.7.0` 公开发布门禁通过 | 尚未测试 |

## 0.8.0：性能、安全与稳定性

| 门禁 ID | 必需结果 | 当前状态 |
|---|---|---|
| `GATE-080-BASELINE` | `docs/PERFORMANCE_BASELINE.md` 的低端、工厂、生态、反应堆、灾害、自动化、索引、OPS 和迁移样本有完整指标 | 尚未测试 |
| `GATE-080-REGRESSION` | 同硬件同样本的 FPS、1% low、内存、保存/加载和索引回归在文档阈值内或有明确阻塞缺陷处置 | 尚未测试 |
| `GATE-080-BOUND-AUDIT` | 全图/大邻域扫描、创建链、信号、灰蛊、病原体、生态、核、Lua 和标签页路径完成审计 | 尚未测试 |
| `GATE-080-FUZZ` | 字体、JSON、OPS 元数据、模块状态、元素 ID、配方图、进度和索引模糊测试通过 | 尚未测试 |
| `GATE-080-RECOVERY` | 在线/离线、崩溃恢复、坏数据和保存中断不损坏用户数据 | 尚未测试 |
| `GATE-080-STABILITY` | 高负载长时间运行无崩溃、泄漏、卡死或无界增长 | 尚未测试 |
| `GATE-080-RELEASE` | 全量回归及 `v0.8.0` 公开发布门禁通过 | 尚未测试 |

## 0.9.0 与发布候选

| 门禁 ID | 必需结果 | 当前状态 |
|---|---|---|
| `GATE-090-FREEZE` | 功能冻结后无大型系统、批量元素或无必要架构改写 | 尚未测试 |
| `GATE-090-CONTENT` | 所有正式元素有双语名、图鉴、用途；所有链有示例；所有危险有控制 | 尚未测试 |
| `GATE-090-GAMEPLAY` | 全部教程/挑战可完成，炼金无死局，自动化无明显无限循环 | 尚未测试 |
| `GATE-090-COMPAT` | 旧存档迁移、OPS 双往返、只读三选项与上传拦截全部通过 | 尚未测试 |
| `GATE-090-GUI` | 简中与英文完整人工检查；其他语言回退明确 | 尚未测试 |
| `GATE-090-STABILITY` | 压力、崩溃恢复、性能和长时间测试通过 | 尚未测试 |
| `GATE-090-LEGAL` | 发布包、对应源码、许可证、第三方来源、AI 披露、升级说明和已知问题完整 | 尚未测试 |
| `GATE-090-RC1` | 创建 `v0.9.0` 后，由同一冻结树或明确修复提交生成并验证 `v1.0.0-rc.1` | 尚未测试 |

阻塞缺陷每次修复都生成新的 `v1.0.0-rc.N` 并重跑受影响门禁；RC 不能改名冒充 1.0.0。

## 1.0.0 正式门禁

| 门禁 ID | 必需结果 | 当前状态 |
|---|---|---|
| `GATE-100-FUNCTION` | 普通沙盒、四模块闭环、跨模块、自动化、炼金、教程/挑战、灾害、图鉴、模块和示例完整可用 | 尚未测试 |
| `GATE-100-LANGUAGE` | 简中、英文完整人工通过；其他语言正确回退 | 尚未测试 |
| `GATE-100-SAVE` | 官方 OPS、旧 OmniPack、进度、迁移、禁用模块、间接引用及只读/上传门禁通过 | 尚未测试 |
| `GATE-100-AUTO` | clean Release build、全部 Meson/Python/Lua/反应/自动化/配方图/挑战测试通过 | 尚未测试 |
| `GATE-100-STRESS` | 十类以上压力测试通过，无已知无界增长或数据损坏 | 尚未测试 |
| `GATE-100-LONGRUN` | 两小时连续运行覆盖大型自动化、生态、多反应堆、多灾害恢复、反复存取、语言/模块切换、索引和炼金进度 | 尚未测试 |
| `GATE-100-BINARY` | 普通 EXE 已剥离、符号分离、无开发路径、PE 安全标志保留；签名状态如实记录 | 尚未测试 |
| `GATE-100-LEGAL` | GPL、字体、第三方来源、AI 披露和对应源码完整，无已知凭据泄露 | 尚未测试 |
| `GATE-100-PACKAGES` | Windows x64、Symbols、Source 三个 ZIP 及三个 `.sha256` 与解压清单审计通过，无用户数据 | 尚未测试 |
| `GATE-100-PUBLIC` | `v1.0.0`、公开源码、匿名克隆/构建和 GitHub Release 下载复核通过 | 尚未测试 |

最终报告必须同时精确包含：

```text
version=1.0.0
release_tag=v1.0.0
release_ready=true
```

任一 `GATE-100-*` 为 `false`、`not_tested`、失败计数或缺失时，必须写 `release_ready=false`。

## 机器可读报告最低字段

每版 `dist/release-report-<version>.md` 的 fenced `text` 块必须包含且只用允许值：

```text
source_commit=
release_tag=
version=
upstream_version=
clean_build_pass=
meson_tests=
python_tests=
lua_runtime_tests=
zh_gui_test=
en_gui_test=
font_visual_test=
module_ui_test=
ops_roundtrip_test=
disabled_module_dialog_test=
readonly_save_block_test=
readonly_upload_block_test=
save_migration_test=
reaction_tests=
automation_tests=
alchemy_progression_tests=
challenge_tests=
stress_test=
long_run_test=
font_license_resolved=
third_party_license_audit=
secret_scan_pass=
source_commit_public=
anonymous_clone_pass=
release_exe_stripped=
debug_symbols_separated=
developer_paths_removed=
pe_security_flags_preserved=
authenticode_signed=
public_zip_sha256=
symbols_zip_sha256=
source_zip_sha256=
zip_audit_pass=
github_release_created=
release_ready=
```

不适用于早期版本的字段使用 `not_tested`，不能省略；`source_zip_sha256` 在不要求源码 ZIP 的版本仍保持 `not_tested`。发布脚本必须验证报告的哈希和 tag/commit 关系，不能只检查字符串是否存在。
