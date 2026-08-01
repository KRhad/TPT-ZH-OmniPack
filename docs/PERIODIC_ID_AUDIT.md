# 周期表稳定 ID 与容量审计

审计基线：`research/mod-source-integration` 分支，纯沙盒清理提交 `cdbb87e2` 之后。此文档记录分配决策，不把规划元素误报为已实现。

```text
pt_num=512
pmapbits=9
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
active_element_slots=385
registered_element_slots=512
periodic_source_map_valid=true
pt_num_expansion_required_for_current_content=false
next_batch_capacity_decision_required=true
```

## 分配结论

原子序数不等于内部 ID。`docs/PERIODIC_ELEMENT_SOURCE_MAP.csv` 是 118 行的不可变映射：能代表真实纯元素的官方或 OmniPack 实现保留原 ID，其余元素按原子序数遍历后的缺失顺序固定到 `370..461`。泛化 `METL`、泛化 `NBLE`、化合物 `DEUT` 和只有低温相但缺少常温相的 `LNTG` 不被虚报为完整纯元素映射。

`370..461` 已全部显式登记并启用为周期表固定区。`370..433` 保留前十二批映射，`434..446` 为新增锕系，`447..455` 依次为 `RF/DB/SG/BH/HS/MT/DS/RG/CN`，`456..461` 为 `NH/FL/MC/LV/TS/OG`；固定区当前无保留空槽。无机三批随后固定使用 `462..511`，没有覆盖周期区、既有模块空槽或 Lua 首选动态空间；高位连续内容区现已占满。既有 `CHLR=360`、`MAGN=261`、`ALUM=256`、`TIN=259`、`LEAD=258`、`CHRM=262`、`COBT=263`、`NICL=260`、`COPR=257`、`MOLY=264`、`ZINC=265` 和官方 `URAN=32`、`PLUT=19`、`TTAN=144`、`TUNG=171`、`PTNM=188`、`GOLD=170`、`MERC=152`、`IRON=76`、`DMND=28`、`SLCN=187`、`O2=61`、`POLO=182` 继续复用且 ID 不变。自动化回归只复用这些既有 identifier，不为同一纯元素分配第二个稳定 ID。

## OPS 与间接字段

当前 OPS1 在粒子类型超过 255 时写入额外类型字节，并把最低读取版本限制到 93.0；palette 保存 identifier 到数字 ID 的映射。载入使用当前运行时 identifier 重映射，因此新增元素只需保持 identifier 和稳定 ID 不变，无需修改 OPS 容器版本。

`Particle::CarriesTypeIn` 标记的 `ctype/tmp/tmp2` 等字段通过 `PMAPBITS=9` 打包，可携带 `0..511`。新增元素若把其他元素 ID 写入间接字段，必须显式声明相应 `CarriesTypeIn` 位并加入 OPS 往返测试。

## Lua 动态元素

Lua 分配器先从 255 向下寻找禁用槽，只有该区域耗尽后才从 511 向下寻找。官方兼容缓冲 `196..255` 保持禁用，提供 60 个首选一字节动态槽；周期表分配不占用这些槽。Lua identifier 冲突检查仍覆盖整个 `PT_NUM`。

## 容量

全部 92 个新增周期元素和 50 个无机材料完成后，当前活动元素为 `385`；`494..511` 已由第三批占满，现有冶金、生物和核工业保留区仍分别有 9、32 和 25 个槽，但这些槽带有既有模块所有权，不能未经审计改作跨族通用内容。下一批开始前必须形成独立的 `PT_NUM/PMAPBITS/OPS/Lua` 扩容决策，核对 OPS 第二类型字节、palette、`ctype/tmp/tmp2`、Lua API、菜单、网络存档和 `can_move` 内存开销；在决策完成前不得分配 `512+` 或覆盖任何旧槽。

## 仍需实测

- 92 个新周期直接类型及全部 118 个周期映射已在同图 OPS 中完成三进程双往返，`LAVA/SPRK/MSCR/CONV/VIRS` 携带字段同时通过；
- 缺失 identifier 的实际 GUI 提示与外部网络保存服务路径；
- Lua 动态元素与已启用周期元素同时存在时的分配及重载；
- 网络保存服务与 GUI `.cps` 路径；
- 大型存档反复加载的性能与内存。
