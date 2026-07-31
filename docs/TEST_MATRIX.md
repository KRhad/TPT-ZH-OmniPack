# 测试矩阵

状态只使用 `PASS`、`FAIL`、`NOT RUN`。编译、进程存活、离屏渲染或自动化断言不能替代 GUI 和人工视觉结论。

## Phase 1：纯沙盒方向清理

| 项目 | 类型 | 当前状态 | 证据 |
|---|---|---|---|
| 炼金服务、进度存档和选取锁不存在 | 源码/静态 | PASS | `rg` 审计；相关源文件和 Meson 目标已删除 |
| 玩家任务、成就、科技树入口不存在 | 源码/文档 | PASS | 没有对应 C++ 玩家系统；路线已改写 |
| 已启用模块元素可直接选择 | 源码/静态 | PASS | 统一限制仅含非法 ID、保留槽和模块关闭 |
| Lua 创建门禁 | 编译/静态 | PASS | 不再包含进度分支；保留模块关闭和保留 ID 拦截 |
| 空模块设置入口 | 源码/i18n | PASS | 只显示四个真实模块及简化生物设置 |
| 旧进度 OPS 加载与重存 | C++ 运行 | PASS | `legacy-progress-save-probe`，2 个真实旧样本、29 粒子 |
| 新 OPS 不写 `omniAlchemy` | C++ 运行 | PASS | 解压重存 BSON 并检查字段不存在 |
| 旧 OPS 粒子/间接类型保持 | C++ 运行 | PASS | 粒子数、`type/ctype/tmp/tmp2` 逐粒子比较 |
| 图鉴“元素说明”标签 | 源码/i18n | PASS | 英文、简中键与 UI 调用存在 |
| 全新 clean Release build | 编译 | PASS | `build-content-phase1-clean`，`509/509` |
| Meson static suite | 自动 | PASS | `18/18` |
| Python 工具单测 | 自动 | PASS | `124/124`，0 skip |
| 四模块及生态双模式 Lua | 实际运行 | PASS | `6/6`；冶金 8 配方、化学 11 路径、生态 8+8、核 5 |
| 0.2 反应样例 | 实际运行 | PASS | 示例 `7/7`、回归场景 `8/8` |
| 0.3 自动化回归 | 实际运行 | PASS | 场景 `9/9`、工程断言组 `6/6`、95 断言、停止增量 0 |
| 官方与四模块 OPS | 实际运行 | PASS | `5/5`、15 进程、10 重启、10 加载、79 粒子、120 字段断言 |
| 最终 GUI 双语/DPI/滚动 | 人工 | NOT RUN | 需要可信桌面视觉确认 |

## 现有内容回归

| 范围 | 自动测试 | 运行回归 | OPS | 压力 | 当前说明 |
|---|---|---|---|---|---|
| 官方元素基线 | PASS | PASS | PASS | 历史 PASS | Phase 1 clean EXE 已复跑功能/OPS |
| 工业冶金 23 | PASS | PASS | PASS | 历史 PASS | ID `256..278` |
| 局部生态 8 | PASS | PASS | PASS | 历史 PASS | ID `288..295`；完整/简化均通过 |
| 受控核工业 7 | PASS | PASS | PASS | 历史 PASS | ID `328..334` |
| 高级化学 10 | PASS | PASS | PASS | 历史 PASS | ID `360..369` |
| 周期表前四批 21 | PASS | PASS | PASS | 预算帧 PASS | 新 ID `370..461` 中 21 项启用；47/118 映射可用 |
| 多模块混合 | PASS | 历史 PASS | 历史 PASS | 历史 PASS | Phase 1 正式混合压力尚未复跑 |

### 周期表前四批证据

| 项目 | 状态 | 证据 |
|---|---|---|
| 118 行数据、长式格位、搜索/筛选契约 | PASS | `periodic-runtime-audit` 与 Python 单测 |
| 21 个新 identifier / 固定 ID / 双语图鉴 | PASS | 元素登记、i18n、内容门禁 |
| 氦/氙/放射性行为及碱金属、碱土金属、硼族各 6 个族成员 | PASS | `runtime_lua_periodic_test.ps1` |
| 硼族关键行为 | PASS | 硼中子俘获、铝两性反应、镓脆铝、硼氧化、铟汽化与鿭衰变均由真实客户端断言 |
| 单帧事件预算 | PASS | 1,200 个 `OG/FR/RA/NH(tmp=1)` 隔离样本分别验证，峰值均不超过且可达到 `1024` |
| 周期 ID OPS 与携带字段 | PASS | 3 进程、2 重启、2 加载、27 粒子、35 字段断言 |
| 周期内容无解锁直接选择 | PASS | `HE`、`NA`、`CA`、`B` 四批代表项均可直接选择 |
| 新增中文字体覆盖 | PASS | 本批“俘”`U+4FD8`、“室”`U+5BA4`、“钝”`U+949D` 由固定 Fusion BDF 补入；容器与离屏渲染通过 |
| 本批 clean Release build | PASS | `build-periodic-boron-group-final2-clean`，`534/534`；EXE `7573A6DF61A7EA20C149DAEE9D3BB04548CAD788FF1B7463475D22E119D09BA9` |
| 本批 Meson/Python 全量套件 | PASS | static `21/21`；Python `146/146`，0 skip |
| 六类与 mixed OPS | PASS | 21 进程、14 重启、14 加载；六类 106 粒子/155 字段断言，mixed 11 粒子/20 字段断言 |
| 周期表真实窗口排版/双语/DPI | NOT RUN | 仍需可信桌面视觉矩阵；编译和静态 UI 契约不替代视觉结论 |

旧文件名中的 `tutorial` 或 `challenge` 表示开发用反应样例和回归场景，不是玩家任务系统，不参与元素可用性或存档进度。

## 后续批次固定测试

每个元素或材料批次至少执行：

1. clean build；
2. 登记和 ID 冲突；
3. 英中名称、图鉴和说明标签；
4. 相变与族代表反应；
5. 模块关闭；
6. OPS 往返及间接类型；
7. 事件预算和压力场景；
8. 来源与许可证登记；
9. 发布包禁止文件审计。

重点单测强水反应、强腐蚀、高低温、放射性、超重、爆炸、无限增殖风险、高密度液体、有毒气体和复杂载体元素。

## 1.0.0 GUI 与发布矩阵

| 门禁 | 状态 |
|---|---|
| 简体中文完整窗口矩阵 | NOT RUN |
| 英文完整窗口矩阵 | NOT RUN |
| 100% / 125% / 150% DPI | NOT RUN |
| 周期表键盘、搜索、筛选、滚动 | NOT RUN |
| 禁用模块加载三选项 | NOT RUN |
| 本地 `.cps` 保存/覆盖/取消 | NOT RUN |
| 只读保存和上传拦截 | NOT RUN |
| 两小时长跑 | NOT RUN |
| ZIP、PE、路径、许可证、哈希 | NOT RUN |
| 对应源码匿名克隆重建 | NOT RUN |
| `v1.0.0` tag 与 Release | NOT RUN |
