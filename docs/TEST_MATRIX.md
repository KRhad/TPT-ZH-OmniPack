# 测试矩阵

状态：`PASS`、`FAIL`、`BLOCKED`、`NOT RUN`。编译通过不替代运行、视觉、存档或压力测试。

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
| 中文视觉布局 | 运行 | BLOCKED | 本会话缺少受信任 Computer Use native pipe |
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
| 发布包隐私文件 | NOT RUN | 打包脚本尚未实现 |

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
| 禁用模块的存档警告 | 运行 | NOT RUN | 尚未实现兼容加载 UI |

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
| 大型冶金工厂压力样本 | 压力 | NOT RUN | Phase 7 建立固定样本并记录 FPS/内存 |
| 冶金 OPS 往返存档 | 运行 | NOT RUN | Phase 7 固定 `>255`、LAVA/SPRK/MSCR 样本 |

## 后续运行与压力测试

以下测试尚未因编译成功而被误标为通过：

| 范围 | 状态 |
|---|---|
| 新建沙盘、保存/加载、崩溃恢复、多标签页 | NOT RUN |
| 官方、汉化及来源模组存档迁移 | NOT RUN |
| 在线功能、离线模式、更新检查 | NOT RUN |
| 冶金核心生产链与材料行为 | PASS |
| 基础化学、生物、核能、自动化、灾害玩法 | NOT RUN |
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
