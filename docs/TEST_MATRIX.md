# 测试矩阵

状态：`PASS`、`FAIL`、`BLOCKED`、`NOT RUN`。每个 PASS 必须链接日志或固定输出。

## Phase 0 / Phase 1 基线

| 测试 | 类型 | 状态 | 证据/备注 |
|---|---|---|---|
| 工作树隔离 | 静态 | PASS | 独立目录，未修改用户已有脏仓库 |
| 九来源提交固定 | 静态 | PASS | `docs/SOURCE_AUDIT.md` |
| GPL 顶层许可证 | 静态 | PASS | 九来源 LICENSE 哈希一致 |
| 官方元素 ID 比较 | 静态 | PASS | 汉化分支与官方 398b 注册表一致 |
| Windows x64 基线配置 | 构建 | NOT RUN | Phase 1 |
| Windows x64 基线编译 | 构建 | NOT RUN | Phase 1 |
| 基线启动 | 运行 | NOT RUN | Phase 1 |
| 基线中文显示 | 运行 | NOT RUN | Phase 1 |
| 基线英文切换 | 运行 | NOT RUN | 当前切换 UI 缺失 |
| 官方存档载入/重存 | 运行 | NOT RUN | Phase 1 |

## 静态门禁

| 测试 | 状态 | 计划实现 |
|---|---|---|
| 元素稳定 ID 冲突 | NOT RUN | `tools/element_registry_check.py` |
| 元素 identifier 冲突 | NOT RUN | 同上 |
| ID 超出 `PT_NUM` | NOT RUN | 同上 |
| 菜单分类非法 | NOT RUN | 同上 |
| 中英文名称/说明缺失 | NOT RUN | 同上 |
| 来源登记缺失 | NOT RUN | 同上 |
| JSON 严格校验 | NOT RUN | `tools/i18n_audit.py` |
| i18n 键差 | NOT RUN | 同上 |
| 格式占位符差异 | NOT RUN | 同上 |
| 玩家可见硬编码英文 | NOT RUN | 同上加人工白名单 |
| 非法温度/数值 | NOT RUN | 元素注册检查 |
| 明显无限复制/循环模式 | NOT RUN | 静态规则加代码复核 |
| 发布包隐私文件 | NOT RUN | `tools/package_audit.py` |

## 运行与压力测试

用户要求的启动、语言、保存、模块关闭、搜索、图鉴、炼金、生物、冶金、核能、灰蛊、标签页、恢复、Lua、在线/离线、更新以及七类压力测试均为 `NOT RUN`。将在对应实现提交后逐项增加固定样本、自动记录 FPS/粒子数/内存并更新本表。

## 测试环境基线

- OS：Microsoft Windows 11 专业版 64 位，版本 `10.0.26200`
- 编译器候选：MSYS2 UCRT64 GCC `16.1.0`
- Meson：`1.11.2`
- Ninja：`1.13.2`
- SDL 与链接库版本：等待 Phase 1 Meson 配置日志给出；不能用缺少 `PKG_CONFIG_PATH` 的外壳查询结果代替。

