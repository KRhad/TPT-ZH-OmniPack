# 测试矩阵

状态：`PASS`、`FAIL`、`BLOCKED`、`NOT RUN`。编译通过不替代运行、视觉、存档或压力测试。

## 0.1.0-test 当前候选（`ff5945c4`）

本节是当前候选的权威摘要；后续 Phase 表保留历史增量证据，不能用旧计数或旧哈希覆盖本节。

| 测试 | 类型 | 状态 | 证据/备注 |
|---|---|---|---|
| 空目录 Release 构建 | 构建 | PASS | `build-0.1.0-test-metrics`，clean Release 通过、0 error；候选清单和 EXE 均绑定 `ff5945c4` |
| 当前 Meson 全量 | 自动 | PASS | `14/14` |
| 当前 Python 全量 | 自动 | PASS | `87/87`，0 skip |
| 元素/反应 registry | 自动 | PASS | 370 元素槽、243 active、127 reserved；48 个玩法字段完整；38/38 反应规则 |
| 字体资源与渲染探针 | 自动/引擎 | PASS | `font.bz2` SHA-256 `47F4EB85...`；2,593 语言字符覆盖；中文矩阵和引擎探针通过 |
| 已剥离 EXE Lua 回归 | 实际运行 | PASS | 模块、冶金、生态、化学、核工业和混合 OPS 共 `6/6` |
| 官方 OPS 双往返 | 实际运行 | PASS | 3 进程、2 重启、2 加载；6 粒子、每次 14 字段断言 |
| 四个单模块 OPS 双往返 | 实际运行 | PASS | `4/4`；总计 12 进程、8 重启、8 加载、73 粒子、每次加载合计 106 字段断言；加上官方场景后为 `5/5`、15 进程、10 重启、10 加载、79 粒子和 120 字段断言 |
| 四模块混合及载体 OPS | 实际运行 | PASS | 11 粒子；`ALUM/NUTR/NFUL/CHLR` 与 `LAVA/SPRK/MSCR/CONV/VIRS` 的 `ctype/tmp/tmp2` 两次重载一致 |
| 当前普通/符号 ZIP | 自动 | PASS | 普通 `53E0304F...9827`；符号 `8FD702E9...E48D`；清单绑定 `ff5945c4`，白名单、哈希和解压二审通过 |
| 当前发布 EXE | 自动 | PASS | `14A00CCF...6926`；调试段/开发路径/动态 GCC runtime 0；ASLR、DEP/NX、高熵地址标志存在；未签名 |
| 当前 ZIP 解压启动 | 实际运行 | PASS | 全新隔离 `ddir`；窗口标题正确、句柄非零、`Responding=true`、退出码 0；不等于窗口内容视觉通过 |
| 首次安装提示消失 | 人工视觉 | NOT RUN | 源码/生成配置为 `CAN_INSTALL=false`、`INSTALL_CHECK=false`；当前环境不能可信读取 SDL 内部对话框 |
| 中文实际可读性 | 人工视觉 | PASS | 用户确认当前原生 Fusion 12px 方案解决中文显示问题；语言切换和 DPI 另列 |
| 中英切换/持久化和 DPI | 人工视觉 | NOT RUN | 100%/125%/150%、双向切换、重启及长文本页面矩阵未完成 |
| 四模块 UI 与代表元素 | 人工视觉 | NOT RUN | `ALUM/NUTR/CHLR/NFUL` 的搜索、放置、图鉴、开关和重启未完成 |
| 禁用模块三选项/只读写入拦截 | 实际 GUI | NOT RUN | 静态门禁通过；菜单、快捷键、另存、覆盖、上传和取消不改变沙盘仍需点击证据 |
| 十场景压力烟测工具 | 实际运行 | PASS | 十个场景均能生成帧、CPU、内存、粒子、事件计数、保存加载与 OPS 原始文件 |
| 十场景各 10 分钟门禁 | 持续运行 | PASS | 当前候选 `ff5945c4` 下 `10/10` 完整 `60+600` 秒样本通过；事件总数 `22529`、单帧峰值 `1024`，十项 `performance_gate_pass=true`，有限观察增长/泄漏均为 false |
| PAT 撤销、公开源码和匿名克隆 | 外部 | BLOCKED | `credential_revoked=false`、`source_commit_public=false`、`anonymous_clone_pass=false` |
| `v0.1.0-test` / GitHub prerelease | 外部 | BLOCKED | 未创建 tag/Release；`release_ready=false` |

### 候选冻结后的本地门禁

| 测试 | 类型 | 状态 | 证据/备注 |
|---|---|---|---|
| 压力原始证据判定器 | 自动 | PASS | `ff5945c4`；核对 JSON/CSV/OPS 哈希、事件/停止/恢复字段，有限观察增长分类不替代长期证明 |
| 发布报告结构门禁 | 自动 | PASS | `d3419e7b`；67 个机器字段齐全，拒绝非法值、缺字段、哈希不符及 `release_ready` 覆盖失败门禁 |
| 开发 HEAD Meson | 自动 | PASS | `14/14`，新增 `release-report` 门禁 |
| 开发 HEAD Python | 自动 | PASS | `87/87`，0 skip |

这些门禁工具和事件指标已绑定候选 EXE `14A00CCF...6926`；当前重新封装的 ZIP 已写入真实源码提交 `ff5945c4` 和新哈希。

## Phase 0 / Phase 1 基线

| 测试 | 类型 | 状态 | 证据/备注 |
|---|---|---|---|
| 工作树隔离 | 静态 | PASS | 独立目录；未覆盖任何来源仓库的 `src` |
| 九来源提交固定 | 静态 | PASS | `docs/SOURCE_AUDIT.md` |
| GPL 顶层许可证 | 静态 | PASS | 九来源顶层许可证及哈希已记录 |
| Windows x64 未修改基线 | 构建 | PASS | 445/445；`docs/BASELINE_BUILD.md` |
| 基线启动 | 运行 | PASS | 进程存活、Responding=True、非零窗口句柄 |
| 默认简体中文选择 | 静态 | PASS | fresh preference fallback 为索引 1 |
| 12 语言切换控件 | 静态/构建 | PASS | Options 下拉已编译 |
| 中文基础可读性 | 人工视觉 | PASS | 用户确认当前原生 Fusion 12px 方案已解决中文显示问题 |
| 中文 DPI 与完整页面布局 | 人工视觉 | NOT RUN | 仍需 100%/125%/150%、长文本、按钮、对话框和图鉴矩阵 |
| 英文切换实际交互 | 运行 | NOT RUN | 需 UI 自动化或人工复核 |
| 官方 100.0 存档载入/重存 | 运行 | NOT RUN | 尚未建立固定样本 |

## Phase 2 静态门禁

| 测试 | 状态 | 证据/备注 |
|---|---|---|
| 元素稳定 ID 冲突 | PASS | `tools/element_registry_check.py` |
| 元素 identifier 冲突 | PASS | 大小写不敏感检查 |
| ID 超出 `PT_NUM` | PASS | 196 行登记；`PT_NUM=512` |
| 官方 ID 漂移 | PASS | 0–195 与固定锁表逐槽比较 |
| Meson 槽位漂移 | PASS | 196 个官方槽与源码逐槽比较 |
| 菜单分类非法 | PASS | `MenuSection.h` 枚举域 |
| 中英文名称/说明缺失 | PASS | 195 个活动官方元素 |
| 来源、commit、许可证缺失 | PASS | 语义登记门禁 |
| 布尔/枚举字段非法 | PASS | 登记检查器；图鉴生成前强制执行 |
| JSON 严格校验 | PASS | 重复键、注释、尾逗号、根类型、尾随垃圾 |
| i18n 键差 | PASS | en-US/zh-CN 1,154/1,154；missing=0；extra=0 |
| 格式占位符差异 | PASS | 0 |
| 换行/颜色/链接控制符差异 | PASS | 0 |
| 空字符串 | PASS | 0 |
| 元素名称登记 | PASS | 195/195；ID 146 tombstone 排除 |
| 菜单键登记 | PASS | 16/16 |
| 图鉴动态枚举键 | PASS | 196 行派生 27 键；英中缺失均为 0 |
| Python 工具单元测试 | PASS | 28/28 |
| Meson `static` suite | PASS | 3/3 |
| 玩家可见硬编码英文 | NOT RUN | 自动启发式已运行；完整 Phase 6 人工分类尚未完成 |
| 非法温度/数值 | PASS（官方） | 官方 195 元素构造参数静态解析 |
| 明显无限复制/循环模式 | PASS（官方基线） | Phase 0 静态复核；自定义内容尚未加入 |
| 发布包隐私文件 | PASS（自动） | 白名单打包和解压后二次审计拒绝偏好、存档、脚本、对象和调试文件 |

## Phase 2 构建与运行

| 测试 | 类型 | 状态 | 证据/备注 |
|---|---|---|---|
| clean Meson 配置 | 构建 | PASS | Windows 11、Meson 1.11.2 |
| clean Windows x64 编译 | 构建 | PASS | 448/448，0 error，2 基线 warning |
| PE 架构 | 静态 | PASS | PE32+、Windows GUI、x86-64 |
| 静态 GCC runtime | 静态 | PASS | 无 `libgcc_s`、`libstdc++`、`libwinpthread` 导入 |
| Lua 动态元素分配 | 运行 | PASS | ID 255 |
| Lua 动态元素选择 | 运行 | PASS | `OMNITEST_PT_LUA1` |
| 运行进程响应 | 运行 | PASS | Lua 回归时 Responding=True |
| 含空格临时路径 | 运行 | PASS | `.NET ProcessStartInfo.ArgumentList` 精确传参 |
| 原始构建日志令牌扫描 | 安全 | PASS | GitHub token 模式命中 0 |
| 构建目录隐私文件 | 安全 | PASS | `powder.pref`/账户/凭据候选 0 |
| 图鉴打开与视觉排版 | 运行 | BLOCKED | 需要可信 UI 控制 |
| 模块开关点击交互 | 运行 | BLOCKED | 需要可信 UI 控制 |
| 禁用模块的存档警告 | 静态/构建 | PASS | Phase 7 统一加载门禁、中文三选项提示和只读保存拦截已编译；实际 UI 交互仍待执行 |

## Phase 3 工业冶金

| 测试 | 类型 | 状态 | 证据/备注 |
|---|---|---|---|
| 冶金稳定 ID `256..278` | 静态 | PASS | 23 个连续显式槽位；登记、Meson 与构造器三方一致 |
| 配方计量与输入相态 | 静态 | PASS | 6 条合金配方、1 条炼钢配方；固定整数计量 |
| 局部扫描边界 | 静态 | PASS | 单次 3×3 收集；无全粒子表扫描 |
| 逐帧反应预算 | 静态 | PASS | 全模块每帧最多 2,048 次反应 |
| 同帧级联保护 | 静态 | PASS | `tmp3=currentTick+1` touched 标记 |
| 合金粒子守恒 | 静态/运行 | PASS | 6 条配方输出粒子数等于输入粒子数 |
| 炼钢物料闭合 | 静态/运行 | PASS | `4 IRON + COKE + FLUX -> 4 STEL + CO2 + SLAG` |
| 高 ID 熔体凝固 | 运行 | PASS | 7 条配方产物均由 `LAVA(ctype)` 冷却恢复 |
| 木炭/焦炭保温时间 | 运行 | PASS | 分别 60/90 帧；含冷料负例 |
| 镍铬电热 | 运行 | PASS | SPRK 脉冲升温且受熔点上限约束 |
| 压力破坏与碎料回收 | 运行 | PASS | 不同阈值；`MSCR.ctype` 保留并重熔恢复来源 |
| 镁高温燃烧 | 运行 | PASS | 热镁与邻近 O2 的有界反应 |
| 锌牺牲保护 | 运行 | PASS | 水环境中锌优先腐蚀并保护邻铁 |
| Lua 动态元素回归 | 运行 | PASS | 保留槽不阻断 Lua ID 255 分配 |
| Python 工具单元测试 | 静态 | PASS | 30/30 |
| Meson `static` suite | 静态 | PASS | 4/4 |
| Windows x64 增量编译 | 构建 | PASS | 28/28，0 error |
| 大型冶金工厂压力样本 | 压力 | PASS | 当前候选 S01/S02 已完成 `60+600` 秒、FPS/内存/OPS/事件原始序列及停止恢复断言；完整门禁通过 |
| 冶金 OPS 往返存档 | 运行 | PASS | 当前候选独立冶金 OPS 双往返，覆盖 `>255`、`LAVA/SPRK/MSCR` 载体字段 |

## Phase 3 基础化学

| 测试 | 类型 | 状态 | 证据/备注 |
|---|---|---|---|
| 化学稳定 ID `360..369` | 静态 | PASS | 10 个显式槽位；登记、Meson、构造器和双语键一致 |
| 未来 ID 分区规划 | 静态 | PASS | `279..287` 与 `296..359` 已在登记表与 Meson 中显式保留；`370..511` 目前仅在内容规划中预留，后续模块启用时须按序登记，避免重编号 |
| 中央模块选择门禁 | 静态/运行 | PASS | “高级化学”设置接入统一选择路径；Lua 可按 identifier 解析 360–369 |
| 局部扫描边界 | 静态 | PASS | 所有反应只检查固定 3×3 邻域；无 `NPART`/`parts.active` 扫描 |
| 逐帧反应预算 | 静态 | PASS | 全模块每帧最多 1,536 次成功反应 |
| 同帧级联与重复输入 | 静态/运行 | PASS | `tmp3=currentTick+1`；氨合成显式排除已选的 3 个氢输入 |
| 燃料裂化 | 运行 | PASS | `OIL -> KERO -> GASO/ACTY` 受催化剂温度窗口控制；催化剂不被消耗 |
| 乙炔聚合 | 运行 | PASS | 2×`ACTY` 在中温 `CATA` 邻域变为 2×`POLY` |
| 过氧化氢制备与分解 | 运行 | PASS | 火花催化制备；热催化按 `2 PERO -> 2 WATR + O2` 分解 |
| 氨合成与中和肥料 | 运行 | PASS | `LNTG + 3 H2` 的压力/温度/火花条件；`AMON + ACID -> FERT` |
| 肥料植物支持 | 运行 | PASS | 湿润植物旁肥料转尘埃并触发官方植物产氧路径 |
| 乙醇发酵 | 运行 | PASS | `YEST + PLNT + WATR` 在 `295..310 K` 变为 `ETHL + CO2` |
| 冷催化剂负例 | 运行 | PASS | 300 K 催化剂邻近原油 3 帧不裂化 |
| 化学静态审计 | 静态 | PASS | 10 元素、7 类工艺、3×3 有界 |
| Python 工具单元测试 | 静态 | PASS | 32/32，2 项本机未发现 C++ 编译器的测试跳过 |
| Meson `static` suite | 静态 | PASS | 5/5 |
| Windows x64 增量编译 | 构建 | PASS | 0 error；GCC 16.1.0 / Meson 1.11.2 / Ninja 1.13.2 |
| 化学 OPS 往返存档 | 运行 | PASS | 当前候选独立化学 OPS 双往返，覆盖 `360..369`、`SPRK(CATA)` 与 `PERO` |
| 化学生产线压力样本 | 压力 | PASS | 当前候选 S05 已完成 `60+600` 秒、FPS/内存/OPS/事件原始序列及停止恢复断言；完整门禁通过 |

## Phase 4 局部生态

| 测试 | 类型 | 状态 | 证据/备注 |
|---|---|---|---|
| 生物稳定 ID `288..295` | 静态 | PASS | 8 个显式槽位；登记、Meson、构造器和双语键一致 |
| 未来生物 ID 保留 | 静态 | PASS | `296..327` 在登记表与 Meson 中显式保留；后续模块不得重编号 |
| 中央模块选择门禁 | 静态/运行 | PASS | “局部生态”门禁按 identifier 解析 `288..295` |
| 局部扫描边界 | 静态 | PASS | 所有路径只检查固定 3×3 邻域；无 `NPART`/`parts.active` 扫描 |
| 逐帧事件预算 | 静态 | PASS | 全模块每帧最多 1,024 次成功事件 |
| 同帧级联保护 | 静态/运行 | PASS | `tmp3=currentTick+1` 防止输入在同一帧被复用 |
| 官方生物状态机隔离 | 静态 | PASS | 不修改 `PLNT`、`VIRS`、`WATR` 或 `LIFE` 更新函数 |
| 藻类光合与冷温负例 | 运行 | PASS | `ALGA + NUTR + WATR + CO2 -> O2`；280 K 下不反应 |
| 分解与萌发 | 运行 | PASS | 湿菌丝分解木材；孢子在营养盐和水旁萌发 |
| 感染、消毒与生物膜 | 运行 | PASS | 病原感染宿主；消毒剂与湿生物膜均把病原转为腐殖质 |
| 简化生物模拟 | 运行 | PASS | 独立偏好文件启动客户端；扩增输入转为 `HUMS` 而非第二粒生物 |
| 生物静态审计 | 静态 | PASS | 8 元素、6 条局部路径、3×3 有界 |
| Python 工具单元测试 | 静态 | PASS | 35/35；2 项本机未发现 C++ 编译器的测试跳过 |
| Meson `static` suite | 静态 | PASS | 6/6，含 registry、i18n、冶金、化学、生物和工具测试 |
| Windows x64 增量编译 | 构建 | PASS | GCC 16.1.0 / Ninja；0 error |
| 生物 OPS 往返存档 | 运行 | PASS | 当前候选独立生态 OPS 双往返，覆盖 `288..295` 与两种模式字段 |
| 高粒子数生态压力样本 | 压力 | PASS | 当前候选 S03/S04 已完成 `60+600` 秒、FPS/内存/OPS/事件原始序列及停止恢复断言；完整门禁通过 |

## Phase 5 受控核工业

| 测试 | 类型 | 状态 | 证据/备注 |
|---|---|---|---|
| 核工业稳定 ID `328..334` | 静态 | PASS | 7 个显式槽位；登记、Meson、构造器和双语键一致 |
| 未来核工业 ID 保留 | 静态 | PASS | `335..359` 在登记表与 Meson 中显式保留；后续模块不得重编号 |
| 局部扫描边界 | 静态 | PASS | 所有路径只检查固定 `3x3` 邻域；无 `NPART`/`parts.active` 扫描 |
| 逐帧事件预算 | 静态 | PASS | 燃料、冷却、屏蔽和中子发生共享每帧最多 512 次成功事件 |
| 同帧级联保护 | 静态/运行 | PASS | `tmp3=currentTick+1` 防止输入在同一帧被复用 |
| 官方核状态机隔离 | 静态 | PASS | 不修改 `URAN`、`PLUT`、`NEUT` 或 `DEUT` 更新函数 |
| 有慢化受控转换 | 运行 | PASS | `SPRK(NGEN) + NFUL + MODR -> NWST`；废料初始热量达到测试阈值 |
| 控制棒、单脉冲与无燃料负例 | 运行 | PASS | `CROD` 抑制燃料转换；同一火花四帧内只留下一个 `NEUT`；无燃料时局部扫描不到新中子 |
| 冷却剂排热 | 运行 | PASS | 900 K `NCLT` 邻近 1200 K `NWST` 转为 `WTRV` 并将废料冷至 900 K |
| 辐射屏蔽 | 运行 | PASS | `RSHD` 吸收相邻静止 `NEUT` 并升温 |
| 核工业静态审计 | 静态 | PASS | 7 元素、4 类受限反应器路径、`3x3` 有界 |
| Python 工具单元测试 | 静态 | PASS | 39/39；2 项本机未发现 C++ 编译器的测试跳过 |
| Meson `static` suite | 静态 | PASS | 7/7，含 registry、i18n、冶金、化学、生物、核工业和工具测试 |
| Windows x64 增量编译 | 构建 | PASS | GCC 16.1.0 / Ninja；0 error，2 条既有 `PowderToy.cpp` warning |
| 核工业 OPS 往返存档 | 运行 | PASS | 当前候选独立核工业 OPS 双往返，覆盖 `328..334`、`SPRK(NGEN)` 与 `NEUT` |
| 高粒子数反应堆压力样本 | 压力 | PASS | 当前候选 S06/S07/S08 已完成 `60+600` 秒、FPS/内存/OPS/事件原始序列及停止恢复断言；完整门禁通过 |

## Phase 6 扩展元素图鉴内容

| 测试 | 类型 | 状态 | 证据/备注 |
|---|---|---|---|
| 双语内容登记完整性 | 静态 | PASS | `ELEMENT_CONTENT.csv` 覆盖全部 48 个 `implementation_status=implemented` 的 `OMNI_PT_*` 元素 |
| 内容 identifier 契约 | 静态 | PASS | 缺失、未知或大小写漂移 identifier 均被内容审计拒绝 |
| 内容字段结构 | 静态 | PASS | 配方、生产、用途、危险均要求英中非空文本且拒绝非法控制符 |
| 编译目录整合 | 构建 | PASS | `generate_element_catalog.py` 将八个内容字段写入 `ElementCatalogRecord` |
| 图鉴搜索内容 | 构建 | PASS | 搜索索引配方、生产、用途和危险的中英文文本 |
| 内容静态审计 | 静态 | PASS | `py tools/element_content_audit.py`：48 条双语扩展元素内容 |
| Python 工具单元测试 | 静态 | PASS | 44/44；2 项本机未发现 C++ 编译器的测试跳过 |
| Meson `static` suite | 静态 | PASS | 8/8，含内容审计 |
| Windows x64 增量编译 | 构建 | PASS | GCC 16.1.0 / Ninja；0 error，2 条既有 `PowderToy.cpp` warning |
| 图鉴内容视觉布局 | 运行 | NOT RUN | 需要可信 UI 控制或人工复核 |

## Phase 7 禁用模块存档加载兼容

| 测试 | 类型 | 状态 | 证据/备注 |
|---|---|---|---|
| 直接模块元素检测 | 静态 | PASS | 解析后的 `Particle.type` 在切换沙盘前按稳定目录和模块开关检查 |
| 携带元素检测 | 静态 | PASS | 读取 `CarriesTypeIn` 标记的 `life`、`ctype`、`tmp..tmp4`；`LAVA`、`SPRK`、`MSCR` 等载体均覆盖 |
| OPS 打包值解码 | 静态 | PASS | 所有携带类型经 `TYP()` 解码，保留粒子索引等高位元数据 |
| 统一加载入口 | 静态/构建 | PASS | 本地文件、搜索、预览和 URL 加载都经 `LoadSaveFile` 或 `LoadSave` 门禁 |
| 中文/英文加载选择 | 静态/构建 | PASS | 三选项为正常加载、只读加载、取消；提示列出触发的已关闭模块 |
| 正常加载数据保留 | 静态/构建 | PASS | 检测仅读取已解析的存档，不删除、映射或替换粒子 |
| 只读保存拦截 | 静态/构建 | PASS | 本地另存/覆盖、在线新建/更新保存均在控制器入口阻止 |
| 存档兼容静态审计 | 静态 | PASS | `py tools/save_compatibility_audit.py` 覆盖检测、入口和保存保护契约 |
| Python 工具单元测试 | 静态 | PASS | 47/47；2 项本机未发现 C++ 编译器的测试跳过 |
| Meson `static` suite | 静态 | PASS | 9/9，含存档兼容审计 |
| Windows x64 增量编译 | 构建 | PASS | GCC 16.1.0 / Ninja；0 error，2 条既有 `PowderToy.cpp` warning |
| 三选项 UI 与只读保存行为 | 运行 | NOT RUN | 需要可信 UI 控制或人工复核 |
| 禁用模块 OPS 往返 | 运行 | NOT RUN | 需要固定的 `LAVA`、`SPRK`、`MSCR` 携带类型样本 |
| 兼容占位转换 | 运行 | NOT RUN | 未实现；不得静默转换或删除存档粒子 |

## Phase 8 Windows x64 测试包

| 测试 | 类型 | 状态 | 证据/备注 |
|---|---|---|---|
| 最小封装内容 | 静态 | PASS | ZIP 仅包含可执行文件、许可证、中文测试说明、更新日志和清单 |
| 个人数据与脚本排除 | 静态 | PASS | 封装审计拒绝 `.cps`、`.stm`、`.pref`、`.lua` 及所有未声明成员 |
| 可执行文件完整性 | 静态 | PASS | 检查 PE `MZ` 头、大小和 SHA-256 与包内清单一致 |
| 重复 ZIP 成员 | 静态 | PASS | 审计拒绝可覆盖验证结果的重复成员名 |
| 测试包审计单元测试 | 静态 | PASS | 3 项覆盖正常包、个人数据注入和清单/二进制篡改 |
| Python 工具单元测试 | 静态 | PASS | 50/50 |
| Meson `static` suite | 静态 | PASS | 10/10，含测试包审计 |
| Windows x64 构建 | 构建 | PASS | GCC 16.1.0 / Ninja；0 error，2 条既有 `PowderToy.cpp` warning |
| 测试 ZIP 生成与审计 | 构建 | PASS | 从 `f19cf0634e8c024bc5a7711ee1f2c2d652d7d5f6` 构建；`package_test_release.py` 后由 `test_release_audit.py` 复核 |
| 交付 ZIP 完整性 | 构建 | PASS | `69,790,282` 字节，SHA-256 `D97AB00AFB0F7DF42BF8C58981641C1F984B041365B205E8FCD162B2D901A258` |
| 最终 Lua 客户端回归 | 运行 | PASS | 模块、冶金、化学、生态完整/简化及核工业共 6 项通过 |
| Windows UI 手动检查 | 运行 | NOT RUN | 测试步骤见 `docs/TEST_RELEASE.md` |

## Release 加固与已拒绝修复试包

| 测试 | 类型 | 状态 | 证据/备注 |
|---|---|---|---|
| Windows x64 Release clean build | 构建 | PASS | `build-release-public`，GCC 16.1.0 / Meson 1.11.2 / Ninja 1.13.2；500 目标、0 error |
| 调试符号拆分 | 构建 | PASS | `objcopy --only-keep-debug` 后 `strip --strip-debug`；普通 EXE 与 `.debug` 分离 |
| 发布 EXE 调试段 | 自动 | PASS | `release_binary_audit.py` 拒绝 `.debug*` 段 |
| 发布 EXE 开发路径 | 自动 | PASS | 扫描拒绝 `C:\\Users\\`、`/Users/` 和构建路径标记 |
| PE 缓解属性 | 自动 | PASS | 审计 `DYNAMIC_BASE`、`NX_COMPAT`、`HIGH_ENTROPY_VA` 位 |
| 字体来源与许可证 | 源码/自动 | PASS | `docs/FONT_AUDIT.md`；语言目录 2,593 字符全部有合法字形输入 |
| 公共 ZIP 与符号 ZIP | 自动 | PASS | 白名单、成员哈希、ZIP SHA-256、解压后二次审计均通过；最终哈希见 `dist/release-report-0.1.0-test.md` |
| 最终 ZIP 启动 | 实际 GUI | PASS | 解压后的最终 ZIP 运行 8 秒，窗口标题为 `TPT-ZH-OmniPack 0.1.0-test`，`Responding=True`、句柄非零；仅创建隔离的 OmniPack 数据目录 |
| 已拒绝候选中文字体 | 运行/字体 | FAIL | `5828a97f` 的 Unifont 转换高位优先打包，和 `FontReader` 低位优先读取不兼容；整数缩放还跳过一部分源行。旧 ZIP 仅保留为失败基线，不得作为候选 |
| 修复字体容器与转换 | 静态 | PASS | `validate_tpt_font.py`：容器、覆盖、pack/unpack、固定 Unifont 源码点与已知中文字符通过 |
| 修复字体引擎离屏渲染 | 引擎 | PASS | `font_render_probe` 调用 `FontReader` 和 `Graphics`；中文、混合符号和化学文本及 `zh-CN.json` 的 1,262 条文本均可测量、绘制，替换字形 0 |
| 私有修复试包中文人工可读性 | 实际 GUI | FAIL | 用户从 `E52E746B...` ZIP 解压运行后确认中文显示仍不如既有出版中文版本；`font_visual_readability_valid=false`，该试包已拒绝并仅保留作失败对照 |
| Fusion 12px BDF 来源与转换 | 源码/自动 | PASS | 版本 `2026.07.20`、固定 BDF/许可证哈希；1,839 个原生字形直接映射，Unifont 回退 0；六个固定中文字形逐行矩阵一致 |
| Fusion 字体容器与全目录覆盖 | 静态 | PASS | `font.bz2` 为 `47F4EB85...`；14,629 字形、2,593 个语言字符全覆盖、pack/unpack 通过、重复 CJK 位图 0 |
| Fusion 私有试包默认启动 | 实际进程 | PASS | `943DA2A6...` ZIP 审计通过；解压后使用 20 个全新目录启动均响应，崩溃 0 |
| Fusion 中文人工可读性 | 实际 GUI | PASS | 用户确认当前原生 Fusion 12px 方案的中文显示问题已经解决；DPI、语言切换和完整页面矩阵另列为未测试 |
| 便携首次运行安装提示 | 源码/clean build | PASS | 默认 `can_install=no`；生成配置 `CAN_INSTALL=false`、`INSTALL_CHECK=false`；重新打包 ZIP 的人工启动仍待执行 |
| 已拒绝修复试包默认中文启动 | 实际进程 | PASS | 隔离用户目录重复 20 次，`running=true`、`Responding=true`、崩溃 0；与人工字形质量失败是相互独立的结果 |
| 中文/英文点击切换 | 实际 GUI | NOT RUN | 需截图和重启验证 |
| 模块开关与代表元素 | 实际 GUI | NOT RUN | 需四模块和 `ALUM/NUTR/NFUL/CHLR` 实测 |
| OPS 三选项与只读拦截 | 实际 GUI | NOT RUN | 静态门禁已通过，实际点击未完成 |
| 反应与压力样本 | 实际 GUI | NOT RUN | Lua 回归不替代持续 FPS/内存数据 |

## 0.1.0-test 最终验收

| 测试 | 类型 | 状态 | 证据/备注 |
|---|---|---|---|
| 脱敏凭据扫描 | 安全 | BLOCKED | 工作区、可达 Git 对象、ZIP、历史、CI、临时目录和环境已扫描；当前 `GITHUB_PAT_TOKEN` PAT 未证明已撤销 |
| 远端发布引用 | 远端 | BLOCKED | `origin` 不存在 `release/test-public-hardening`、`release/test-public-final-validation` 或 `v0.1.0-test`；凭据门禁前不推送 |
| 最终 ZIP 启动 | 实际进程 | PASS | 解压 EXE 的路径、SHA-256、标题、窗口句柄与 `Responding=True` 记录在 `artifacts/final-validation/logs/` |
| 最终 ZIP Lua 回归 | 实际客户端 | PASS | 6/6；`lua-runtime-final-zip.txt` |
| 本地化与存档兼容审计 | 自动 | PASS | `i18n_audit.py --check` 与 `save_compatibility_audit.py`；`localization-and-compatibility-audits.txt` |
| 当前原生 12px 简中可读性 | 实际 GUI | PASS | 用户确认当前中文显示问题解决；英文切换和 100%/125%/150% DPI 矩阵仍为 `NOT RUN` |
| 四模块、代表元素与持久化 | 实际 GUI | NOT RUN | 未在最终 ZIP 中执行模块开关、搜索、放置、图鉴或重启 |
| OPS 往返、三选项与载体字段 | 实际 GUI | NOT RUN | 未用 GUI 保存 OPS；静态审计不替代运行证据 |
| 代表玩法 | 实际 GUI | NOT RUN | Lua 运行回归仅证明脚本覆盖路径，不能代替交互场景 |
| 十个固定压力样本 | 持续运行 | PASS | 当前候选 `10/10` 已获得 FPS、内存、粒子、事件和双 OPS 往返原始数据；独立评估、停止/恢复和完整门禁均为 `true` |

## 后续运行与压力测试

以下测试尚未因编译成功而被误标为通过：

| 范围 | 状态 |
|---|---|
| 新建沙盘、保存/加载、崩溃恢复、多标签页 | NOT RUN |
| 官方、汉化及来源模组存档迁移 | NOT RUN |
| 在线功能、离线模式、更新检查 | NOT RUN |
| 冶金核心生产链与材料行为 | PASS |
| 基础化学核心工艺 | PASS |
| 生物与受控核工业核心路径 | PASS |
| 扩展元素图鉴内容数据与编译目录 | PASS |
| 禁用模块存档加载保护 | PASS（静态/构建） |
| Windows x64 内部测试包 | PASS（封装/审计） |
| 自动化、灾害玩法 | NOT RUN |
| 炼金进度、成就、挑战、教程 | NOT RUN |
| 七类高粒子数压力样本 | NOT RUN |
| FPS、最低 FPS、内存和无限增长记录 | NOT RUN |

## 测试环境

- OS：Microsoft Windows 11 专业版 64 位，版本 `10.0.26200`
- 编译器：MSYS2 UCRT64 GCC `16.1.0`
- Meson：`1.11.2`
- Ninja：`1.13.2`
- SDL：`2.30.9-tpt-libs`
- JsonCpp：`1.9.5-tpt-libs`
- 固定依赖：`tpt-libs v20251019131007`

## 当前 Phase 2 产物

| 构建 | 状态 | 大小 | SHA-256 |
|---|---|---:|---|
| `build-phase2-clean/tpt-zh-omnipack.exe` | PASS | 228,411,304 | `6428B39852C31AB03A9EEB3C20AFFA16CA9B7B84DB6EC88042831F1CAD2416BB` |

## 当前 Phase 3 冶金产物

| 构建 | 状态 | 大小 | SHA-256 |
|---|---|---:|---|
| `build-phase2-clean/tpt-zh-omnipack.exe` | PASS | 235,565,190 | `86F78585851E54547F1A76CD71E126AD3792260F5D75FFA4E9ACBD02543D5F8D` |

## 当前 Phase 3 基础化学产物

| 构建 | 状态 | 大小 | SHA-256 |
|---|---|---:|---|
| `build-phase2-clean/tpt-zh-omnipack.exe` | PASS | 238,700,275 | `398BAF39F8B4F0EB8ECA87EAF452A2EE2321831A44191C39064EBBBD0CC333E9` |

## 当前 Phase 5 受控核工业产物

| 构建 | 状态 | 大小 | SHA-256 |
|---|---|---:|---|
| `build-phase2-clean/tpt-zh-omnipack.exe` | PASS | 243,554,909 | `F27E73149243B096933C0779EE0701ADA7711C3090C24E9629389299671BF6FD` |

## 当前 Phase 6 扩展元素图鉴内容产物

| 构建 | 状态 | 大小 | SHA-256 |
|---|---|---:|---|
| `build-phase2-clean/tpt-zh-omnipack.exe` | PASS | 243,658,363 | `88DD50F4815356F9E86B97ECAD59DE019BF97BE8EFECABE876AD16363DDD5D91` |

## 当前 Phase 7 禁用模块存档加载兼容产物

| 构建 | 状态 | 大小 | SHA-256 |
|---|---|---:|---|
| `build-phase2-clean/tpt-zh-omnipack.exe` | PASS | 244,470,300 | `860F796A547DACF8EAB014AE4060252DD2199224067A21EA91BC64178FB4690D` |

## 当前 Phase 8 Windows x64 测试包产物

| 构建 | 状态 | 大小 | SHA-256 |
|---|---|---:|---|
| `build-phase2-clean/tpt-zh-omnipack.exe` | PASS | 244,465,722 | `3B96CFEC060705A48681645074AE3C5F53E9A2D40AE56FF76B0E906C2FDBD406` |
| `dist/TPT-ZH-OmniPack-Test-Windows-x64.zip` | PASS | 69,790,282 | `D97AB00AFB0F7DF42BF8C58981641C1F984B041365B205E8FCD162B2D901A258` |

上述 Phase 8 产物由提交 `f19cf0634e8c024bc5a7711ee1f2c2d652d7d5f6` 构建，ZIP 内 `TEST-MANIFEST.txt` 绑定同一提交、可执行文件大小和 SHA-256。
