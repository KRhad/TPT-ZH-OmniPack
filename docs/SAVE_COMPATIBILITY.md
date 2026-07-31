# 存档兼容性

## 当前格式与不可变约束

- 保存容器：`OPS1`；压缩：BZip2；元素映射：palette identifier。
- `PT_NUM=512`、`PMAPBITS=9`；现有公开 ID 与 identifier 不得重编号。
- `ctype/tmp/tmp2` 等间接元素字段按 `Particle::CarriesTypeIn` 元数据检查和往返。
- 模块关闭只隐藏或阻止创建该模块工具，不删除已加载粒子。
- 普通官方 OPS 不需要迁移为 OmniPack 专用格式。

## 已移除进度字段

旧开发版曾在 OPS 顶层写入 `omniAlchemy`。当前 `GameSave` 不声明、不解析、不写出该字段；BSON 的未知顶层字段容忍路径会忽略它。因此：

1. 旧存档可加载；
2. 旧进度不会限制菜单、Lua、粘贴或新建粒子；
3. 原有沙盘粒子不会因删除进度系统而消失；
4. 重存后不再包含 `omniAlchemy`；
5. 旧 `powder.pref` 中的 `Omni.Progress.AlchemyMode` 不再读取，遗留键无副作用。

`legacy-progress-save-probe` 对两个真实含旧字段 OPS 完成加载→重存→再加载，验证新输出不写该字段且粒子类型、`ctype`、`tmp`、`tmp2` 保持一致。

## 已验证范围

- 官方、工业冶金、局部生态、高级化学、受控核工业和混合场景的隔离 stamp 双往返已有历史运行证据；
- 直接 ID 大于 255 和 `LAVA/SPRK/MSCR/CONV/VIRS` 间接类型字段已有覆盖；
- `save_compatibility_audit.py` 检查统一加载入口、模块禁用提示和只读保存/上传门禁；
- Phase 1 旧进度样本兼容测试已纳入静态套件。

## 尚未验证

- GUI `.cps` 保存对话框的完整点击路径；
- 300+ 材料后的大型 OPS、多次反复加载和缺失新元素提示；
- 外部旧模组存档的合法来源识别和逐项迁移；
- 网络保存服务对未来扩展 ID 的端到端行为。

无法确认来源的旧数字 ID 不得猜测映射，也不得静默替换为当前同号元素。
