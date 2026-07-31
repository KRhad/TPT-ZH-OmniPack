# 存档兼容性

## 当前格式与不可变约束

- 保存容器：`OPS1`；压缩：BZip2；元素映射：palette identifier。
- `PT_NUM=512`、`PMAPBITS=9`；现有公开 ID 与 identifier 不得重编号。
- `ctype/tmp/tmp2` 等间接元素字段按 `Particle::CarriesTypeIn` 元数据检查和往返。
- 模块关闭只隐藏或阻止创建该模块工具，不删除已加载粒子。
- 普通官方 OPS 不需要迁移为 OmniPack 专用格式。

## 周期表稳定 ID 审计

- 118 种元素中 26 种复用现有纯元素实现及原 ID；不会为同一纯元素创建第二个编号。
- 其余 92 种固定使用 `370..461`，映射见 `PERIODIC_ELEMENT_SOURCE_MAP.csv`；发布后不得重排。
- OPS 粒子 `type` 在需要时通过 field descriptor bit 14 写入第二字节，读取端恢复该字节；ID 大于 255 会把最低可读版本限制到 93.0。
- palette 同时保存 identifier 与数字 ID，载入时按当前 identifier 重映射；缺失 identifier 会进入现有缺失元素报告，而不是猜测同号材料。
- `ctype/tmp/tmp2` 等携带类型字段仍使用 `PMAPBITS=9` 和 `CarriesTypeIn` 元数据，能够保存 `0..511`。
- Lua `elements.allocate` 优先从 255 向下寻找禁用槽；`196..255` 保持禁用，因此周期表 `370..461` 不挤占首选 Lua 动态空间。
- 当前无需扩大 `PMAPBITS`。扩大到 10 位会使 `can_move` 矩阵由 512² 增为 1024²，并触及 OPS、Lua、菜单及所有打包类型字段；本阶段禁止无必要扩容。

## 已移除进度字段

旧开发版曾在 OPS 顶层写入 `omniAlchemy`。当前 `GameSave` 不声明、不解析、不写出该字段；BSON 的未知顶层字段容忍路径会忽略它。因此：

1. 旧存档可加载；
2. 旧进度不会限制菜单、Lua、粘贴或新建粒子；
3. 原有沙盘粒子不会因删除进度系统而消失；
4. 重存后不再包含 `omniAlchemy`；
5. 旧 `powder.pref` 中的 `Omni.Progress.AlchemyMode` 不再读取，遗留键无副作用。

`legacy-progress-save-probe` 对两个真实含旧字段 OPS 完成加载→重存→再加载，验证新输出不写该字段且粒子类型、`ctype`、`tmp`、`tmp2` 保持一致。

## 已验证范围

- 官方、工业冶金、局部生态、高级化学、受控核工业、周期表前十批和混合场景的隔离 stamp 双往返已由当前第二过渡系 clean EXE 复跑；六类合计 135 粒子、每次加载 184 个字段断言；
- 周期表前十批 50 个新元素完成隔离 stamp 双往返：3 个进程、2 次重启、2 次加载、56 粒子、每次加载 64 个字段断言、50 个大于 255 的直接类型；
- 周期类型在 `LAVA.ctype`、`SPRK.ctype`、`CONV.ctype/tmp`、`VIRS.tmp2` 中的 identifier palette 和 9 位携带值均保持；
- 直接 ID 大于 255 和 `LAVA/SPRK/MSCR/CONV/VIRS` 间接类型字段已有覆盖；
- `save_compatibility_audit.py` 检查统一加载入口、模块禁用提示和只读保存/上传门禁；
- Phase 1 旧进度样本兼容测试已纳入静态套件。

## 尚未验证

- GUI `.cps` 保存对话框的完整点击路径；
- 300+ 材料后的大型 OPS、多次反复加载和缺失新元素提示；
- 外部旧模组存档的合法来源识别和逐项迁移；
- 网络保存服务对未来扩展 ID 的端到端行为。
- `370..461` 全批元素实现后的完整 118 元素同图 OPS、大型反复加载和缺失 identifier 交互；当前前十批 50 个新 ID 已实测通过。

无法确认来源的旧数字 ID 不得猜测映射，也不得静默替换为当前同号元素。
