# 已移除的玩家游戏系统

## 审计范围

以 `3ee6b0a15cbd7d76f0605af3b614215a3b54a9d4` 为清理前基线审计 `src/`、`tools/`、本地化、OPS、打包配置和路线文档。唯一已实现并限制玩家元素选择的系统是 `OmniAlchemy` 炼金进度；没有发现独立的成就、每日任务、主线、支线或科技树 C++ 玩家系统。

“tutorial/challenge”旧文件用于开发回归和 OPS 样例，不是游戏内任务菜单。它们作为自动测试资产保留，但产品文案统一称为“反应样例”或“回归场景”。

## 移除结果

| 系统 | 清理前状态 | 当前处理 | 兼容行为 |
|---|---|---|---|
| 炼金探索/元素发现 | 已实现 | 删除服务、UI、选取锁、Lua API、测试桩和打包 profile | 旧 OPS 字段忽略 |
| 元素/配方进度 | 随炼金实现 | 不再读取或写入 | 不影响粒子 |
| 强制元素解锁 | 随炼金实现 | 所有已启用内容可直接使用 | 仅模块关闭仍受控 |
| 科技树/时代推进 | 仅路线计划 | 从路线和门禁删除 | 无存档字段 |
| 成就/任务/每日任务 | 未实现 | 从产品计划删除 | 无存档字段 |
| 玩家挑战/完成弹窗 | 未实现 | 不新增入口 | 开发回归场景保留 |

被删除的实现包括：

- `src/simulation/OmniAlchemy.*` 与 `src/client/OmniAlchemySaveState.h`；
- Options 里的炼金复选框和进度窗口；
- GameModel 的加载、重置、扫描、提示和粘贴拦截；
- Lua 的 `omniAlchemyProgress`、`omniAlchemyUnlocked` 和 stamp 锁；
- GameSave/Simulation 对 `omniAlchemy` 的解析和写出；
- `docs/ALCHEMY_PROGRESSION.json`、0.4 炼金包 profile、探针和专用测试；
- 英中 `alchemy.*` 与未实现模块设置键。

## 存档证明

`legacy-progress-save-probe` 使用两个真实旧样本：

- `examples/0.2.0/01-peroxide-pathogen.stm`；
- `examples/0.3.0/09-integrated-factory.stm`。

探针要求样本原始 BSON 含 `omniAlchemy`，然后执行加载、重存和再加载，并断言：新 BSON 不再含该字段、粒子数不变、直接类型及 `ctype/tmp/tmp2` 不变。当前结果为 `samples=2`、`particles=29`；该测试已进入 Meson static suite。

```text
game_tasks_removed=true
achievements_removed=true
challenge_system_removed=true
technology_tree_removed=true
alchemy_progression_removed=true
forced_unlocks_removed=true
legacy_progress_field_ignored=true
legacy_progress_field_written=false
stable_ids_reassigned=false
development_tests_preserved=true
```
