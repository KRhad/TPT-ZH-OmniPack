# 存档兼容性

## 当前格式与不可变约束

- 保存容器：`OPS1`；压缩：BZip2；元素映射：palette identifier。
- 当前容量：`PT_NUM=1024`、`PMAPBITS=10`；已公开的 `0..511` 数字 ID 与全部 identifier 均未重编号。
- `ctype/tmp/tmp2` 等间接元素字段按 `Particle::CarriesTypeIn` 元数据检查和往返。
- 模块关闭会隐藏并阻止创建该模块工具，不删除已加载粒子；对应模块更新在下一模拟刻暂停，重新启用后恢复。
- 普通官方 OPS 不需要迁移为 OmniPack 专用容器格式。
- 与官方元素重复的 OmniPack 内容合并为主元素行为增强；旧扩展 ID 仅作永久兼容墓碑，不计入可玩材料，也不复用。

## 10 位元素空间

- 118 种周期元素中 26 种复用现有纯元素实现及原 ID，其余 92 种固定使用 `370..461`。
- 无机三批固定为 `462..511`；工程材料区固定为 `512..575`，当前已实现 `OMNI_PT_SOLD..OMNI_PT_RFBK=512..532`；代表性核素固定为 `OMNI_PT_H2IS..OMNI_PT_CF52=576..588`；未来内容区为 `589..1023`。
- OPS 粒子 `type` 在需要时通过 field descriptor bit 14 写入第二字节，读取端恢复该字节；ID 大于 255 会把最低可读版本限制到 93.0。
- palette 同时保存 identifier 与数字 ID，载入时按来源 `pmapbits` 建立数字槽表，再按当前 identifier 重映射；即使来源槽大于当前 `PT_NUM`，也先尝试 identifier 映射。缺失 identifier 进入现有缺失元素报告，不猜测同号材料。
- 来源 `pmapbits` 只接受 `8..16`。缺失值按旧 OPS 的 8 位默认读取；损坏的 `0`、`17` 等值 fail-closed 拒绝。
- 携带类型字段先按来源位宽无符号拆包，再按当前 10 位重打包，避免把旧 8/9 位的高位附加数据当作有符号右移。
- Lua `elements.allocate` 先使用 `255..196`，耗尽后从 1023 向下；不会误占官方保留 ID 146。
- 原生 `can_move` 和 Lua `customCanMove` 两张矩阵各为 `1024²` 个单字节条目，即各 1,048,576 字节。

`high-id-save-probe` 已对直接 `SOLD=512/RFBK=532/CF52=588`、以 588 为目标的 `LAVA.ctype`、`SPRK.ctype`、`CONV.ctype/tmp`、`VIRS.tmp2`、带 `NITI` 来源的 `BRMT.ctype/tmp4` 和 identifier palette 执行原生序列化往返，并确认来源 `pmapbits=11` 的已知 `OMNI_PT_SOLD` 槽 1536 会映射到当前稳定 ID 512；缺失高位 identifier 会进入缺失报告且对应直接类型被中和，损坏 `pmapbits=0/17` 被拒绝。

## 已移除进度字段

旧开发版曾在 OPS 顶层写入 `omniAlchemy`。当前 `GameSave` 不声明、不解析、不写出该字段；BSON 的未知顶层字段容忍路径会忽略它。因此：

1. 旧存档可加载；
2. 旧进度不会限制菜单、Lua、粘贴或新建粒子；
3. 原有沙盘粒子不会因删除进度系统而消失；
4. 重存后不再包含 `omniAlchemy`；
5. 旧 `powder.pref` 中的 `Omni.Progress.AlchemyMode` 不再读取，遗留键无副作用。

`legacy-progress-save-probe` 对两个真实含旧字段 OPS 完成加载→重存→再加载，验证新输出不写该字段且粒子类型、`ctype`、`tmp`、`tmp2` 保持一致。

## 已验证范围

- 官方、工业冶金、局部生态、化学与无机物、受控核工业/核素、完整周期表六类隔离 stamp 双往返：18 个进程、287 粒子、每次加载合计 339 个字段断言、291 个稳定 identifier 和 287 个 palette identifier；核类样本覆盖 7 个核工业材料及全部 13 个核素，最高为 `OMNI_PT_CF52=588`；
- mixed OPS：3 个进程、11 粒子、21 个字段断言；
- 化学隔离 stamp 覆盖全部 60 个化学/无机元素和最高 ID `AMCL=511`；
- 全部 118 个周期元素在同一隔离 stamp 中完成双往返，直接类型和 `LAVA/SPRK/BRMT/CONV/VIRS` 携带字段均覆盖；
- 四模块禁用回归保留 12 个粒子，包括 `NITI=520`、`RFBK=532`、`CF52=588`、带标记 `BRMT(ctype=ALUM)`、生态、化学和核工业代表项；一模拟刻后 Omni 更新事件为 0；
- 旧 `MSCR=278` 三进程迁移回归确认新 OPS 不再包含 278 粒子，官方 `BRMT=30` 的 `ctype/tmp4` 保持；
- Lua 实际运行确认 `255`、`196`、`1023` 三个分配边界，官方空洞 146 未占用且高位 Lua 元素可用；
- `save_compatibility_audit.py` 检查统一加载入口、模块禁用提示和只读保存/上传门禁；Phase 1 旧进度样本兼容测试已纳入静态套件。

## 尚未验证

- GUI `.cps` 保存、覆盖与取消的完整点击路径；
- 包含全部 418 个可玩材料的大型 OPS、多次反复加载和缺失高位 identifier 提示；兼容别名不作为直接样本凑数；
- 外部旧模组存档的合法来源识别和逐项迁移；
- 网络保存服务对高位 ID 的端到端行为；
- 正式 600 秒压力和 7,200 秒综合长跑中的重复保存/加载。

无法确认来源的旧数字 ID 不得猜测映射，也不得静默替换为当前同号元素。
