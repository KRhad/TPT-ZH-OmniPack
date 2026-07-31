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
periodic_elements_implemented=67
periodic_elements_planned=51
periodic_new_elements_active=41
active_element_slots=284
registered_element_slots=462
periodic_source_map_valid=true
pt_num_expansion_required=false
```

## 分配结论

原子序数不等于内部 ID。`docs/PERIODIC_ELEMENT_SOURCE_MAP.csv` 是 118 行的不可变映射：能代表真实纯元素的官方或 OmniPack 实现保留原 ID，其余元素按原子序数遍历后的缺失顺序固定到 `370..461`。泛化 `METL`、泛化 `NBLE`、化合物 `DEUT` 和只有低温相但缺少常温相的 `LNTG` 不被虚报为完整纯元素映射。

`370..461` 已全部显式登记为周期表固定区；当前启用 `HE=370`、`BE=371`、`B=372`、`N=373`、`F=374`、`NE=375`、`NA=376`、`P=377`、`S=378`、`AR=379`、`K=380`、`CA=381`、`SC=382`、`V=383`、`MN=384`、`GA=385`、`GE=386`、`AS=387`、`SE=388`、`BR=389`、`KR=390`、`SR=391`、`IN=401`、`SB=402`、`TE=403`、`I=404`、`XE=405`、`CS=406`、`BA=407`、`TL=428`、`BI=429`、`AT=430`、`RN=431`、`FR=432`、`RA=433`、`NH=456`、`FL=457`、`MC=458`、`LV=459`、`TS=460`、`OG=461`，其余 51 槽继续是不可选择的保留项。既有 `CHLR=360`、`MAGN=261`、`ALUM=256`、`TIN=259`、`LEAD=258`、`CHRM=262`、`COBT=263`、`NICL=260`、`COPR=257`、`ZINC=265` 和官方 `TTAN=144`、`IRON=76`、`DMND=28`、`SLCN=187`、`O2=61`、`POLO=182` 继续复用且 ID 不变。自动化回归仍只复用官方元素，不拥有新稳定 ID。

## OPS 与间接字段

当前 OPS1 在粒子类型超过 255 时写入额外类型字节，并把最低读取版本限制到 93.0；palette 保存 identifier 到数字 ID 的映射。载入使用当前运行时 identifier 重映射，因此新增元素只需保持 identifier 和稳定 ID 不变，无需修改 OPS 容器版本。

`Particle::CarriesTypeIn` 标记的 `ctype/tmp/tmp2` 等字段通过 `PMAPBITS=9` 打包，可携带 `0..511`。新增元素若把其他元素 ID 写入间接字段，必须显式声明相应 `CarriesTypeIn` 位并加入 OPS 往返测试。

## Lua 动态元素

Lua 分配器先从 255 向下寻找禁用槽，只有该区域耗尽后才从 511 向下寻找。官方兼容缓冲 `196..255` 保持禁用，提供 60 个首选一字节动态槽；周期表分配不占用这些槽。Lua identifier 冲突检查仍覆盖整个 `PT_NUM`。

## 容量

完成全部 92 个新增周期元素后，活动元素预计为 `335`。当前活动数为 `284`（原 243 加前九批 41）；`462..511` 仍有 50 个连续槽，现有冶金、生物和核工业区还分别有 9、32 和 25 个空槽。理论活动上限仍可达到 451，覆盖 300–450 的目标上界而无需把 `PMAPBITS` 扩大到 10。

## 仍需实测

- 前九批 41 个新周期直接类型和周期类型在 `LAVA/SPRK/CONV/VIRS` 携带字段中的 OPS 双往返已通过；后续批次仍须逐批复跑；
- 118 种元素同图保存、重载和缺失 identifier 提示；
- Lua 动态元素与已启用周期元素同时存在时的分配及重载；
- 网络保存服务与 GUI `.cps` 路径；
- 大型存档反复加载的性能与内存。
