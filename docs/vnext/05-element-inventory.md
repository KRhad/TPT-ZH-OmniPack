# vNext Element Update Inventory（第一轮）

## Goal

为 base commit `729f72cbad6ea3962940d1e60feb2c5fd40d6954` 建立可重复生成、机器可读的元素更新静态清单，覆盖注册槽位、`Update` 绑定、实现入口、粒子/邻域访问和并行化风险信号。本轮不修改任何生产模拟代码，也不把词法命中提升为 GPU 或行为分类结论。

产物：

- `tools/vnext/element_update_inventory.py`
- `docs/vnext/element-update-inventory.json`

## Gate result

本工具和 JSON 生成门禁为 **GREEN**：全部已注册构造器已处理，431 个 custom update 绑定全部解析到实现入口，JSON 可解析且两次独立生成逐字节一致。

完整 Element semantic inventory 门禁仍为 **YELLOW**，OmniCore/GPU 分类门禁仍为 **RED**：`analysis_complete=false`，AST、预处理器、别名、跨文件歧义调用、函数指针、Lua runtime override 和人工语义复核均未完成。所有 488 个元素的正式分类保持 `UNKNOWN`。

## Registry and binding counts

| Metric | Result |
|---|---:|
| Registered active elements / constructors | 488 |
| Upstream-identifier elements (`DEFAULT_PT_*`) | 195 |
| OmniPack-identifier elements (`OMNI_PT_*`) | 293 |
| Custom update bindings | 431 |
| Direct constructor `Update = &...` bindings | 362 |
| Configurator-derived bindings | 69 |
| Elements with no custom `Update` | 57 |
| Resolved bindings | 431 |
| Unresolved bindings | 0 |
| Unique binding implementation roots | 164 |
| Source-local roots named `update` | 130 |
| Shared/named roots | 34 |
| Formal classifications | `UNKNOWN`: 488 |

此前的 `488 / 431 / 164` 分别表示：488 个启用的 Meson 注册项；其中 431 个最终拥有 custom `Update`；这些绑定按 `source:line:symbol` 去重后形成 164 个入口。164 不是“164 种已证明等价的行为”，也不包含每个可达 helper 作为独立 root。

431 个绑定按来源拆分为：

- upstream identifier：143；
- OmniPack identifier：288；
- 362 个直接绑定；
- 69 个间接绑定，其中 `OmniConfigureOrganicElement` 33、`OmniConfigureElectronicsElement` 20、`OmniConfigureEnvironmentElement` 16。

57 个无 custom `Update` 的元素不能解释为“无行为”：它们仍可能依赖 generic simulation loop、属性、transition、create/graphics hooks 或其他系统。

## Analysis method

工具按稳定 ID 排序并执行以下流程：

1. 从 `src/simulation/elements/meson.build` 解析有效槽位，保留 `disabler()` 造成的 ID 空洞。
2. 解析每个 `Element::Element_NAME()` 构造器的 identifier、display name、直接 `Update` 赋值和 `OmniConfigure*(*this, ...)` 调用。
3. 对 configurator 入口建立词法调用闭包，从可达的 `element.Update = &...` 唯一赋值恢复间接绑定。
4. 解析每个 update root，并跟踪同文件调用以及全局唯一的裸函数调用；不跟踪对象/成员调用、歧义重载或无法解析的宏调用。
5. 记录 `parts[i]` 与非 `i` 索引访问、`pmap`/`photons`、spawn/delete/type-change/movement/swap、`pv`、air `vx/vy`、`hv`、gravity、electricity、RNG，以及 long-range/order/same-frame 风险信号。
6. 每个 implementation root 保存限量行级证据；每个元素保存紧凑观察值并引用 root。

JSON 顶层包含 schema/version/base commit、499 个输入文件的逐文件 SHA-256 与聚合 manifest hash。为消除 `core.autocrlf` 和平台 checkout 差异，文本输入在哈希前统一把 CRLF 规范为 LF。输入 manifest hash 为 `3f52c6a5f86cbb7008d6d4778464ff8f02613bf1474d7b1fed81ff94763a9dfb`。

## Static findings

按元素展开的词法结果（共享 root 会把该 root 的所有可达分支并到每个绑定元素）为：

| Detected signal | Elements | Unique roots |
|---|---:|---:|
| `create_part` | 332 | 100 |
| `kill_part` | 250 | 76 |
| `part_change_type` | 361 | 98 |
| movement API | 4 | 4 |
| swap API | 0 | 0 |
| explicit `pv` write | 86 | 38 |
| explicit air `vx/vy` write | 1 | 1 |
| explicit `hv` write | 1 | 1 |
| explicit gravity-state write | 5 | 5 |
| explicit electrical-state write | 1 | 1 |
| RNG | 237 | 112 |

明确检测到的 air field writers：

- air velocity：`DMG`；
- air heat：`LIGH`；
- gravity state：`NBHL`, `NWHL`, `GPMP`, `GBMB`, `GRVT`；
- direct wireless/electrical state：`WIFI`。

显式 `pv` writer 共 86 个，完整稳定 ID/名称和行级证据在 JSON 中。这个集合与“所有会影响 Air 的元素”不是同一集合：构造器 `HotAir`、generic simulation coupling、transition 和间接 member-call effects 不属于此处的 explicit update-body `pv` write 统计。

## GPU risk triage

词法 GPU risk 只用于安排人工审计，不是 `GENERIC` / `SPECIAL_CPU` / `SPECIAL_GPU_CANDIDATE` 分类。431 个 resolved custom-update 元素中：

- high：420（154 roots）；
- medium：5（5 roots）；
- unknown：6（5 roots）。

high 数量很大，主要原因是共享 Omni root 的路径并集：某一 helper 中出现 allocator、邻居写或 pressure 写，会保守传播给绑定该 root 的所有元素。这是有意的过度近似，不能用来计算 GPU 覆盖率。

首批人工审计应优先覆盖以下可观察高风险族：

- `WARP`：直接写 particle position 与 `pmap`，并写 pressure、spawn；
- `PSTN`：bulk/occupancy/particle relocation 路径；
- `PIPE` / `PPIP`、`PRTI` / `PRTO`：跨位置存储、portal/allocator 路径；
- `ARAY` / `CRAY` / `DRAY`：长距离扫描、spawn/delete/type/map effects；
- `WIFI`：双帧 wireless state 与 same-frame electrical interactions；
- `SPRK`、`STKM` / `STKM2` / `FIGH`：广泛跨系统 helper、spawn/delete/movement/pressure effects；
- 所有共享 Omni chemistry/material/periodic roots：必须按 element type 分支做 AST path-sensitive 分析，不能直接采用当前 union 结果。

## Known limitations

`analysis_complete=false` 是硬性结论，原因包括：

- 不是 Clang AST；没有预处理器条件、类型、重载、模板或宏展开语义。
- 仅跟踪同文件调用与全局唯一裸函数；member calls、header inline、函数指针和歧义目标不展开。
- `parts[i]` 只识别字面 self；其他索引统一记作 `non_self`，其中可能是邻居、新生粒子、portal 粒子或远程粒子。
- reference/pointer alias 仅报告“检测到 alias”，不继续做 data-flow，因此可能漏报后续 field write。
- plain assignment/read-write 的识别是词法近似；通过函数参数、返回引用或容器 API 的 mutation 可能漏报。
- shared update 的结果是可达分支并集，可能把只属于一种 `parts[i].type` 的行为报告给同 root 的其他元素。
- `not_detected` 只表示当前受支持 pattern 没有命中，绝不证明不存在。
- `long_range_read`、iteration-order 和 same-frame dependency 只有 `possible` / `unknown`，没有“已证明安全”。
- Lua 可以替换 built-in update；Lua runtime override 不在本轮静态生产源 inventory 内。
- 没有运行 gameplay、save、Lua、GUI 或 benchmark；本任务只新增分析工具和文档。

## Validation

本地使用 Python `3.14.5`，执行：

```text
python -m py_compile tools/vnext/element_update_inventory.py
python tools/vnext/element_update_inventory.py --base-commit 729f72cbad6ea3962940d1e60feb2c5fd40d6954 --self-test --output <run-a.json>
python tools/vnext/element_update_inventory.py --base-commit 729f72cbad6ea3962940d1e60feb2c5fd40d6954 --self-test --output <run-b.json>
python tools/vnext/element_update_inventory.py --base-commit 729f72cbad6ea3962940d1e60feb2c5fd40d6954 --check docs/vnext/element-update-inventory.json
python -m json.tool docs/vnext/element-update-inventory.json <discard-output>
```

结果：

- scanner self-tests：PASS；
- registered/bindings/roots：`488 / 431 / 164`；
- unresolved：`0`；
- JSON parse + structural validation：PASS；
- run A canonical-LF SHA-256：`EB1464013FF7A5E25DE97EB52DB8B8DF25393D0B56D496B67407EF6B96FA7BF8`；
- run B canonical-LF SHA-256：`EB1464013FF7A5E25DE97EB52DB8B8DF25393D0B56D496B67407EF6B96FA7BF8`；
- checked artifact canonical-LF SHA-256：`EB1464013FF7A5E25DE97EB52DB8B8DF25393D0B56D496B67407EF6B96FA7BF8`；
- deterministic byte equality：PASS。

## Recommendation

主控可合并本工具与第一轮 JSON，作为 AST inventory 的输入和回归基线，但不得据此启动 production Air/GPU element replacement。下一步应在独立阶段建立 Clang-based、path-sensitive inventory，优先人工确认上述高风险族，并把 `UNKNOWN` 逐项提升；在此之前 GPU element classification gate 保持 RED。
