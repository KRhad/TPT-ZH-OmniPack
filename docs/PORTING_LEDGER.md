# 移植账本

> Phase 0 状态：尚未复制任何第三方实现代码。账本中的“重写候选”只是设计准入，不等于正式收录。

状态值：`AUDITED`、`DESIGN`、`PORTING`、`TESTING`、`ACCEPTED`、`REJECTED`、`BLOCKED`。

| 模块/候选 | 来源与固定 commit | 原作者/归属 | Phase 0 判定 | 目标方式 | 当前状态 | 理由与门禁 |
|---|---|---|---|---|---|---|
| 官方模拟、保存、ID | Official `bff38ce...` | TPT contributors | 直接底座 | 保留并同步 | AUDITED | 100.0.399 权威实现 |
| 中文/英文语料与严格加载 | Dragonrster `445fab51...`；OmniPack `457233ac...` | Dragonrster、TPT contributors、OmniPack contributors | 审核后重放并重写加载器 | JsonCpp 严格 JSON + 自动审计 | ACCEPTED | en/zh 1,154/1,154，0 阻塞错误；视觉复核仍独立跟踪 |
| 官方元素稳定锁 | Official `bff38ce...`；OmniPack `08fe8a82...` | TPT contributors、OmniPack contributors | 源码解析生成 | 逐槽固定 0–195 | ACCEPTED | 196 槽、195 活动、1 tombstone；自动门禁 PASS |
| 模块选择与图鉴框架 | OmniPack `d7312a9b...`、`f28cdcb7...` | OmniPack contributors | 项目原创实现 | 统一门禁、编译时登记目录 | ACCEPTED | clean build、28 单测、Lua 运行回归和枚举本地化门禁 PASS |
| 汉化字体 | Dragonrster `445fab51...` | 未知 | 禁止发布 | 替换为可追溯字体 | BLOCKED | 名称、来源、许可证缺失 |
| Cracker 工业化学 | Cracker `ebbb9aab...` | Cracker1000 等 | 重写候选 | 中央反应表 | DESIGN | 与冶金/Cyens 去重 |
| 动力门户 PPTI/PPTO | Cracker `ebbb9aab...`; Jacob `b4926161...` | 各来源作者 | 合并重写 | 单一稳定实现 | DESIGN | 多来源重复、旧 API |
| CSNS/GSNS/导线/PCON | Cracker `ebbb9aab...` | Cracker1000 | 重写候选 | 当前电子 API + 预算 | DESIGN | 自动化价值明确 |
| PET/BEE 原实现 | Cracker `ebbb9aab...` | Cracker1000 | 排除原实现 | 如需概念则另行设计 | REJECTED | 大范围扫描/越界风险 |
| BFLM/CEXP/EXPL | Cracker/Jacob | 各来源作者 | 排除原实现 | 预算化灾害另设计 | REJECTED | 无受控扩散门禁 |
| MGNT | Cracker `ebbb9aab...` | Cracker1000 | 重写候选 | 半径/帧预算磁体 | DESIGN | 原实现每粒子 6,561 格扫描 |
| Spike 生物循环 | SpikeViper `134ebf33...` | SpikeViper contributors | 重写候选 | 事件驱动局部状态 | DESIGN | 氧/营养/感染联动价值高 |
| Ultimata 传送/漏斗/力场 | Ultimata `b7497175...` | Bowserinator 等 | 精选重写 | 工程化可控元素 | DESIGN | 必须有克制与保存测试 |
| Ultimata 时间/电磁核心 | Ultimata `b7497175...` | Bowserinator 等 | 实验参考 | 默认关闭，架构先行 | DESIGN | 确定性/网络/性能风险 |
| Jacob BUTN/PWHT | Jacob `b4926161...` | jacob1 等 | 重写候选 | 当前 API + 洪泛预算 | DESIGN | UX 价值明确，旧代码越界 |
| Jacob MOVS/ANIM | Jacob `b4926161...` | jacob1 等 | 仅需求参考 | 新数据结构才可实现 | DESIGN | 旧内存/边界/冻结风险 |
| Jacob 保存来源标记 | Jacob `b4926161...` | jacob1 | 迁移参考 | 只读识别 `"Jacob1's_Mod"` | DESIGN | 可帮助可靠识别来源 |
| Alchemy 四元素开局 | Alchemy `9a593ce1...` | jacob1/SopaXorzTaker 等 | 设计采用 | 全新进度服务 | DESIGN | 与总任务初始集合一致 |
| Alchemy 原进度代码 | Alchemy `9a593ce1...` | 原仓库作者 | 排除 | identifier/schema 重写 | REJECTED | 越界、未初始化、可绕过 |
| Seppo 冶金实现 | Seppo `c3a8dd17...` | SeppoTPT | 无可移植源码 | 清洁室重写 | BLOCKED | 32 个实现文件全部缺失 |
| Seppo 元素/合金清单 | Seppo `c3a8dd17...` 与论坛 | SeppoTPT | 设计参考 | 去重后自研 | DESIGN | 需修复公开旧 bug |
| Cyens 烃网络 | Cyens `f01d992c...` | cbeimers113 | 重写候选 | 不改官方基础语义 | DESIGN | 有机链有价值 |
| Cyens 时间/局部重力 | Cyens `f01d992c...` | cbeimers113 | 实验参考 | 默认关闭 | DESIGN | 未完成离子系统、全局风险 |

## 每次实际移植必须补记

当状态进入 `PORTING`，新增一条详细记录，至少包含：

- 原仓库 URL、分支、commit、原文件路径；
- 原作者及文件内版权；
- 目标文件路径与提交；
- 是逐行改编、算法重写还是仅依据公开行为清洁室实现；
- ID、identifier、存档迁移；
- 本地化键和图鉴条目；
- 单元/集成/运行/压力测试；
- 性能预算与已知偏差。

任何 `ACCEPTED` 项都必须能从本账本追到固定来源和测试证据。
