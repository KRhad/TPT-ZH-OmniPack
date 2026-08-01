# 周期表稳定 ID 与容量审计

审计基线：`development/content-expansion-1.0` 分支，元素行为去重提交 `4f473ebe` 之后。本文件同时记录已经完成的 10 位扩容；规划槽位不算已实现材料。

```text
pt_num=1024
pmapbits=10
official_locked_ids=0..195
lua_preferred_disabled_ids=196..255
omnipack_active_before_periodic_batch=48
periodic_reuse_existing=26
periodic_new_ids=92
periodic_new_id_first=370
periodic_new_id_last=461
periodic_elements_implemented=118
periodic_elements_planned=0
periodic_new_elements_active=92
inorganic_expansion_ids=462..511
engineering_content_ids=512..575
future_content_ids=576..1023
engine_active_element_slots=386
compatibility_aliases=1
playable_materials=385
registered_element_slots=513
explicit_reserved_slots=127
unallocated_capacity_slots=503
periodic_source_map_valid=true
pt_num_expansion_complete=true
next_batch_capacity_decision_required=false
```

## 分配结论

原子序数不等于内部 ID。`docs/PERIODIC_ELEMENT_SOURCE_MAP.csv` 是 118 行的不可变映射：能代表真实纯元素的官方或 OmniPack 实现保留原 ID，其余元素按原子序数遍历后的缺失顺序固定到 `370..461`。泛化 `METL`、泛化 `NBLE`、化合物 `DEUT` 和只有低温相但缺少常温相的 `LNTG` 不被虚报为完整纯元素映射。

`370..461` 已全部显式登记并启用为周期表固定区。`370..433` 保留前十二批映射，`434..446` 为新增锕系，`447..455` 依次为 `RF/DB/SG/BH/HS/MT/DS/RG/CN`，`456..461` 为 `NH/FL/MC/LV/TS/OG`；固定区无保留空槽。无机三批固定使用 `462..511`。10 位扩容没有移动任何旧 ID，工程材料区固定为 `512..575`，未来内容区为 `576..1023`；首个高位材料为 `OMNI_PT_SOLD=512`。

既有 `CHLR=360`、`MAGN=261`、`ALUM=256`、`TIN=259`、`LEAD=258`、`CHRM=262`、`COBT=263`、`NICL=260`、`COPR=257`、`MOLY=264`、`ZINC=265` 和官方 `URAN=32`、`PLUT=19`、`TTAN=144`、`TUNG=171`、`PTNM=188`、`GOLD=170`、`MERC=152`、`IRON=76`、`DMND=28`、`SLCN=187`、`O2=61`、`POLO=182` 继续复用且 ID 不变。重复候选若确有有价值且许可证兼容的行为增量，只把增量重写进官方或当前主元素；不分配第二个元素 ID。完全相同行为没有可合并增量，因此不新增代码或材料。

## OPS 与间接字段

OPS1 的直接粒子类型在超过 255 时写入第二类型字节，并通过 palette 保存 identifier 与数字 ID。加载按当前 identifier 重映射，因此高位元素不要求改变 OPS 容器版本。

当前加载器只接受来源 `pmapbits=8..16`；缺失字段沿用旧 OPS 的 8 位默认值，`0`、`17` 等损坏值会以 `ParseException::Corrupt` 拒绝。`Particle::CarriesTypeIn` 标记的 `ctype/tmp/tmp2` 等字段先按来源位宽进行无符号拆包，再按当前 `PMAPBITS=10` 重打包。`high-id-save-probe` 已覆盖：

- 直接 `SOLD=512`；
- `LAVA.ctype`、`SPRK.ctype`、`BRMT.ctype/tmp4`；
- `CONV.ctype/tmp`、`VIRS.tmp2`；
- palette 中的 `OMNI_PT_SOLD`；
- 缺失的高位 identifier 被加入缺失元素报告并把对应直接类型中和为 `PT_NONE`；
- 损坏 `pmapbits=0/17` 的 fail-closed 拒绝。

新增元素若把其他元素 ID 写入间接字段，仍必须显式声明相应 `CarriesTypeIn` 位并加入 OPS 往返测试。

## Lua 动态元素

Lua 分配器只把 `255..196` 作为一字节首选缓冲，不再扫描官方 `0..195` 的禁用空洞，因此官方保留 ID 146 不会被误占。60 个首选槽耗尽后，从 `1023` 向下分配。实际客户端已确认首槽为 255、一字节末槽为 196、第 61 个动态元素为 1023，且高位 Lua 元素可以选择和创建。

## 容量与内存

`PT_NUM=1024` 使原生 `can_move` 和 Lua `customCanMove` 各包含 `1024²` 个单字节条目，即各 1,048,576 字节。当前登记到 520：共有 394 个活动项，其中 `MSCR=278` 是兼容别名，实际可玩材料为 393；显式保留槽 127 个，`521..1023` 仍有 503 个尚未登记的容量槽。

旧 ID `0..511`、identifier 与模块所有权全部保持。兼容别名 278 永久保留，不因行为合并而释放。继续新增内容仍需逐批登记、预算和 OPS 测试，但本轮 10 位容量决策已经完成，不再以“禁止分配 512+”阻塞工程材料批次。

## 仍需实测

- 缺失高位 identifier 的实际 GUI 提示与外部网络保存服务路径；
- GUI `.cps` 保存、覆盖和取消的完整点击路径；
- 包含全部 393 个可玩材料的大型 OPS 反复加载；
- 正式 60 秒预热 + 600 秒压力以及两小时综合长跑；
- 最终发布包中的高位 Lua 元素、模块切换和多次保存/加载组合。
