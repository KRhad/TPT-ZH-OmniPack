# TPT-ZH-OmniPack 开发进度

> 本文件只记录有源码差异、构建日志或测试输出支持的结果。未执行项目明确标为 `NOT RUN` 或 `BLOCKED`。

## 当前阶段

版本门禁：`0.1.0-test` 稳定化，开发分支 `development/omnipack-1.0`。当前只修复公开测试基线、补齐 OPS/GUI/压力证据，不新增大型玩法；0.1 本地门禁完成后进入 0.2 跨模块联动。

## 2026-07-30 接管基线

- 接管时 HEAD：`e18abad9753e61e8f6c9f8fdf671c4bd80a4ca53`；工作树干净；中文字体实现提交为 `c743db2fcc49c01033e68023cceff897ed4c35f6`。
- 用户已确认当前 Fusion Pixel Font 原生 12px 方案解决中文显示问题；该结论只覆盖中文实际可读性，不替代 100%/125%/150% DPI、完整页面和语言切换矩阵。
- 字体可复现构建 SHA-256：`47F4EB851ABFC4CABDFC780E3D427ECBA39077418324A4E291CCDE552F0C139D`；2,593 个语言字符全覆盖，已知中文 BDF 矩阵和引擎离屏探针通过。
- 接管后重跑：clean Release 502/502、Meson 12/12、Lua 6/6；中英文目录 1,262/1,262、缺失 0、额外 0。
- 首次运行曾出现文件关联“安装”提示。提交 `a590f8b5` 将便携构建默认设为 `can_install=no`，生成配置确认 `CAN_INSTALL=false`、`INSTALL_CHECK=false`，并让打包器拒绝该设置回退。
- `origin` 仍指向旧汉化仓库；没有安全、明确的正式发布远端授权，因此未推送、未创建 tag 或 GitHub Release。

## 已完成

- Phase 0：固定并审计九个来源仓库、许可证、精确提交、元素登记、ID 冲突、核心差异和确定风险。
- Phase 1：完成 Dragonrster 100.0.398 未修改基线构建，合并官方 100.0.399，建立 `integration/zh-omnipack`，恢复默认简体中文与 12 语言切换，并完成独立项目命名。
- 建立 196 槽官方元素锁表：195 个活动元素和 ID 146 tombstone；官方 ID 0–195 不可漂移。
- 建立 0–511 稳定 ID 分区，冶金、生物、核能、化学、自动化、特殊物理、灾害、兼容和实验内容均有固定区间。
- 实现只读元素登记门禁，检查重复 ID/identifier、`PT_NUM` 上限、Meson 次序、菜单、名称、说明、来源、许可证、状态和官方锁哈希。
- 用 JsonCpp 严格解析所有内嵌语言文件；拒绝重复键、注释、尾随逗号、根类型错误、非字符串值和尾随垃圾，并保留安全英文回退。
- 实现英中本地化审计、元素名称同步及其单元测试。
- 为 195 个官方活动元素登记中英文正式名称；ID 146 tombstone 不生成可见名称。
- 实现统一模块设置与中央工具选择门禁；菜单、收藏、搜索、采样、延迟选择和 Lua 调用均经过同一选择判定。
- 对 Lua 运行时元素按“稳定 ID + identifier”识别，避免把运行时占用的保留数字槽误判成整合模块元素。
- 建立编译时只读图鉴目录；搜索支持代号、identifier、中英文名称与说明、来源和分类。
- 图鉴中的分类、状态、存档兼容性、实现状态和测试状态均使用本地化键；来源名、commit 和许可证按允许保留的专有标识显示。
- 建立模块设置 UI。六个内容模块可控制；“简化生物模拟”已在 Phase 4 启用，另外三项尚未实现的设置明确标注“待实现”并强制禁用。
- 注册 3 个 Meson 静态测试目标，并增加可复现 Windows Lua 运行回归脚本。
- 完成首批工业冶金：在固定 ID `256..278` 注册 23 个材料、工艺与回收元素，且不占用 `196..255` 官方兼容缓冲区。
- 建立集中、局部的冶金反应引擎：6 条定比熔融合金配方、1 条炼钢配方、木材炭化、煤炭炼焦、压力碎料回收、铜腐蚀、镁燃烧、锌牺牲保护与镍铬电热。
- 合金与炼钢只收集触发点 `3x3` 邻域，每帧共享 2,048 次成功反应预算，并使用 tick 标记阻止同帧重复级联。
- 为 23 个冶金元素登记中英文正式名称、短说明、来源、稳定 ID、菜单分类、存档状态与测试状态；缺失的 Seppo 构造器明确按公开概念独立实现，未从二进制反推或复制。
- 增加冶金静态审计、两项 Python 单测与实际客户端 Lua 回归，覆盖配方、熔体凝固、炭化、电热、压力/碎料、镁燃烧与锌保护。
- 完成基础化学首批：固定 ID `360..369` 注册氯气、氨气、乙醇、煤油、汽油、乙炔、催化剂、聚合物、过氧化氢和肥料；当时的空槽保持稳定分区，当前已显式保留 `279..287` 与 `296..359`，避免后续模块移动 ID。
- 建立集中、局部的 `OmniChemistry` 反应引擎：燃料裂化、乙炔聚合、氯氢反应、氨合成、肥料循环、酵母发酵、过氧化氢制备/分解和普通水电解均使用固定 `3x3` 邻域。
- 化学反应共享每帧 1,536 次成功预算，并以 tick 标记避免同帧级联；氨合成显式排除重复输入，过氧化氢分解先确认氧气输出槽位再消耗输入。
- 增加“高级化学”模块设置，接入统一菜单、搜索、采样、延迟选择和 Lua 的元素选择门禁。
- 增加化学静态审计、两项 Python 单测与实际客户端 Lua 回归，覆盖 9 个运行场景和冷催化剂负例。
- 完成局部生态首批：固定 ID `288..295` 注册营养盐、藻类、菌丝、孢子、病原体、消毒剂、腐殖质和生物膜；`296..327` 保留，未占用其他模块区间。
- 建立集中、局部的 `OmniBiology` 事件引擎：藻类光合、菌丝分解、孢子萌发、病原感染、消毒处理和生物膜过滤都只检查固定 `3x3` 邻域，且不改写官方 `PLNT`、`VIRS`、`WATR` 或 `LIFE` 状态机。
- 生物成功事件共享每帧 1,024 次预算，并以 tick 标记避免同帧级联；完整模式只转换既有营养粒子，简化模式把相应扩增输入转为腐殖质，二者均不增加粒子数。
- 启用“简化生物模拟”设置，并增加生物静态审计、两项 Python 单测和完整/简化两种独立偏好下的实际客户端 Lua 回归。
- 完成受控核工业首批：固定 ID `328..334` 注册反应堆燃料、慢化剂、控制棒、冷却剂、核废料、中子发生器和辐射屏蔽体；`335..359` 保留，未占用其他模块区间。
- 建立集中、局部的 `OmniNuclear` 反应器引擎：有燃料前提的每次 `SPRK(NGEN)` 脉冲只产生一粒静止官方中子；燃料必须同时具备慢化剂、局部中子且无控制棒才会变成高温废料；冷却剂与屏蔽体分别提供有限排热和中子吸收。
- 所有核工业成功路径共享每帧 512 次预算，使用固定 `3x3` 搜寻和 tick 标记防止同帧级联；不改写官方 `URAN`、`PLUT`、`NEUT` 或 `DEUT` 状态机。
- 增加核工业静态审计、四项 Python 单测与实际客户端 Lua 回归，覆盖受控转换、控制棒和无燃料负例、单脉冲上限、低于自身相变阈值的冷却路径以及屏蔽吸收。
- 完成扩展元素图鉴内容：`ELEMENT_CONTENT.csv` 为 48 个已实现 OmniPack 元素提供双语配方、生产、用途和危险说明；官方元素保持现有登记说明，不虚构本项目未实现的工艺。
- 图鉴目录在构建时合并内容表并保持只读；搜索会索引八个新增内容字段，图鉴只显示确有登记的内容行。
- 新增内容静态审计和五项相关 Python 覆盖，拒绝缺失、未知或大小写漂移的元素条目，并验证内容文本实际编入生成目录。
- 实现禁用模块存档加载门禁：在切换沙盘前检查直接元素和由 `LAVA`、`SPRK`、`MSCR` 等载体保存的元素类型；载体字段按 `TYP()` 解包，避免把粒子索引元数据误判为元素 ID。
- 本地文件、搜索、预览和 URL 等所有控制器存档加载入口统一经过中文三选项提示：正常加载、只读加载或取消。检测仅读取存档结构，不会删除、重映射或替换存档粒子。
- 只读加载会在本地另存、覆盖保存、新建在线上传和在线更新保存四个控制器入口阻止写入；新建空白沙盘或正常加载会解除该状态。
- 新增存档兼容静态审计与三项 Python 回归，覆盖携带类型的 `TYP()` 解包、所有加载入口以及保存拦截。
- 新增最小 Windows x64 测试包流程：仅封入静态可执行文件、许可证、中文启动说明、更新日志和可校验清单；封包审计拒绝偏好、存档、图章、账户资料和脚本。
- 为测试包增加 ZIP 成员、PE 头、可执行文件 SHA-256、清单尺寸及隐私文件回归检查；提供明确的已实现模块范围、手动验证步骤和正式发布阻塞项。

## 修改文件

Phase 2、已完成的 Phase 3 冶金/基础化学、Phase 4 局部生态和 Phase 5 受控核工业的主要变更：

- `docs/ELEMENT_REGISTRY.csv`
- `docs/ELEMENT_CONTENT.csv`
- `docs/ELEMENT_DESIGN.md`
- `docs/I18N_AUDIT.md`
- `tools/data/official_elements_100_0.csv`
- `tools/element_registry_check.py`
- `tools/i18n_audit.py`
- `tools/sync_element_localization.py`
- `tools/generate_element_catalog.py`
- `tools/element_content_audit.py`
- `tools/tests/*.py`
- `tools/runtime_lua_module_test.ps1`
- `tools/runtime/module_filter_regression.lua`
- `src/simulation/OmniMetallurgy.cpp`
- `src/simulation/OmniMetallurgy.h`
- `src/simulation/elements/{ALUM,COPR,LEAD,TIN,NICL,MAGN,CHRM,COBT,MOLY,ZINC,CHRC,COKE,STEL,BRNZ,BRAS,SSIL,NCRM,ALMG,TSTL,SLAG,FLUX,CRUC,MSCR}.cpp`
- `tools/metallurgy_audit.py`
- `tools/tests/test_metallurgy_audit.py`
- `tools/runtime/metallurgy_regression.lua`
- `tools/runtime_lua_metallurgy_test.ps1`
- `src/simulation/OmniChemistry.cpp`
- `src/simulation/OmniChemistry.h`
- `src/simulation/elements/{CHLR,AMON,ETHL,KERO,GASO,ACTY,CATA,POLY,PERO,FERT}.cpp`
- `tools/chemistry_audit.py`
- `tools/tests/test_chemistry_audit.py`
- `tools/runtime/chemistry_regression.lua`
- `tools/runtime_lua_chemistry_test.ps1`
- `src/simulation/OmniBiology.cpp`
- `src/simulation/OmniBiology.h`
- `src/simulation/elements/{NUTR,ALGA,MYCL,SPOR,PATH,STER,HUMS,BIOF}.cpp`
- `tools/biology_audit.py`
- `tools/tests/test_biology_audit.py`
- `tools/runtime/biology_regression.lua`
- `tools/runtime_lua_biology_test.ps1`
- `src/simulation/OmniNuclear.cpp`
- `src/simulation/OmniNuclear.h`
- `src/simulation/elements/{NFUL,MODR,CROD,NCLT,NWST,NGEN,RSHD}.cpp`
- `tools/nuclear_audit.py`
- `tools/tests/test_nuclear_audit.py`
- `tools/runtime/nuclear_regression.lua`
- `tools/runtime_lua_nuclear_test.ps1`
- `src/common/Localization.cpp`
- `src/common/Localization.h`
- `src/gui/elementsearch/ElementCatalog.h`
- `src/gui/elementsearch/ElementSearchActivity.cpp`
- `src/gui/game/OmniContent.cpp`
- `src/gui/game/OmniContent.h`
- GameModel、GameController、Options、Tool 和 Meson 相关文件
- `src/lang/en-US.json`
- `src/lang/zh-CN.json`

## 新增元素

Phase 2：0。只锁定官方 ID 并建立基础设施，没有把预留内容伪装成已实现元素。

Phase 3 冶金：23。`ALUM`、`COPR`、`LEAD`、`TIN`、`NICL`、`MAGN`、`CHRM`、`COBT`、`MOLY`、`ZINC`、`CHRC`、`COKE`、`STEL`、`BRNZ`、`BRAS`、`SSIL`、`NCRM`、`ALMG`、`TSTL`、`SLAG`、`FLUX`、`CRUC`、`MSCR`；稳定 ID 为 `256..278`。

Phase 3 基础化学：10。`CHLR`、`AMON`、`ETHL`、`KERO`、`GASO`、`ACTY`、`CATA`、`POLY`、`PERO`、`FERT`；稳定 ID 为 `360..369`。

Phase 4 局部生态：8。`NUTR`、`ALGA`、`MYCL`、`SPOR`、`PATH`、`STER`、`HUMS`、`BIOF`；稳定 ID 为 `288..295`。

Phase 5 受控核工业：7。`NFUL`、`MODR`、`CROD`、`NCLT`、`NWST`、`NGEN`、`RSHD`；稳定 ID 为 `328..334`。

## 汉化状态

- 英文键：1,256
- 中文键：1,256
- 缺失键：0
- 多余键：0
- 发布阻塞错误：0
- 占位符/控制符错误：0
- 疑似未翻译警告：37，均保留在 `docs/I18N_AUDIT.md` 供人工分类
- 官方活动元素名称登记：195/195；冶金/化学/生物/核工业元素中英文正式名称登记：23/10/8/7
- 默认语言：简体中文；英文及其他 10 个原有语言入口保留

该结果证明键集合、格式和登记完整性通过自动门禁；不等于视觉布局已通过。

## 编译命令

在清空的 `build-phase2-clean` 中执行：

```powershell
meson setup build-phase2-clean `
  -Dbuildtype=debugoptimized `
  -Dstatic=prebuilt `
  -Dstrip=false `
  -Dlto=false `
  "-Dc_args=['-ffunction-sections','-fdata-sections']" `
  "-Dcpp_args=['-ffunction-sections','-fdata-sections']" `
  "-Dc_link_args=['-Wl,--gc-sections','-static','-static-libgcc','-static-libstdc++']" `
  "-Dcpp_link_args=['-Wl,--gc-sections','-static','-static-libgcc','-static-libstdc++']" `
  -Dmanifest_date=2026-07-29
meson compile -C build-phase2-clean -v
```

## 编译结果

- Windows x64 `debugoptimized` 静态 clean build：PASS，448/448。
- 编译错误：0。
- 编译警告：2；均为基线已有的 `PowderToy.cpp` / `std::optional<ByteString>` GCC `-Wmaybe-uninitialized`。
- 产物：`build-phase2-clean/tpt-zh-omnipack.exe`
- 大小：228,411,304 字节
- SHA-256：`6428B39852C31AB03A9EEB3C20AFFA16CA9B7B84DB6EC88042831F1CAD2416BB`
- PE：PE32+、Windows GUI、x86-64。
- 非系统 GCC runtime DLL 导入：0。

Phase 3 冶金增量构建：

- Windows x64 `debugoptimized` 静态增量构建：PASS，28/28，0 error。
- 产物：`build-phase2-clean/tpt-zh-omnipack.exe`
- 大小：235,565,190 字节
- SHA-256：`86F78585851E54547F1A76CD71E126AD3792260F5D75FFA4E9ACBD02543D5F8D`
- 本次恢复会话已通过恢复 MSYS2 UCRT64 工具链复核并重跑后续编译。

Phase 3 基础化学增量构建：

- Windows x64 `debugoptimized` 静态增量构建：PASS，0 error。
- 工具链：GCC 16.1.0、Meson 1.11.2、Ninja 1.13.2。
- 产物：`build-phase2-clean/tpt-zh-omnipack.exe`
- 大小：238,700,275 字节
- SHA-256：`398BAF39F8B4F0EB8ECA87EAF452A2EE2321831A44191C39064EBBBD0CC333E9`
- 非系统 GCC runtime DLL 导入：0。

Phase 4 局部生态增量构建：

- Windows x64 `debugoptimized` 静态增量构建：PASS，0 error。
- 工具链：GCC 16.1.0、Meson 1.11.2、Ninja 1.13.2。
- 产物：`build-phase2-clean/tpt-zh-omnipack.exe`，241,372,172 字节。
- SHA-256：`76EDD75EB2699EF3618ACEE28E76427766348FDA7FB245B89666062EE341B625`。

Phase 5 受控核工业增量构建：

- Windows x64 `debugoptimized` 静态增量构建：PASS，0 error。
- 工具链：GCC 16.1.0、Meson 1.11.2、Ninja 1.13.2。
- 产物：`build-phase2-clean/tpt-zh-omnipack.exe`，243,554,909 字节。
- SHA-256：`F27E73149243B096933C0779EE0701ADA7711C3090C24E9629389299671BF6FD`。

Phase 6 扩展元素图鉴内容增量构建：

- Windows x64 `debugoptimized` 静态增量构建：PASS，0 error。
- 工具链：GCC 16.1.0、Meson 1.11.2、Ninja 1.13.2。
- 产物：`build-phase2-clean/tpt-zh-omnipack.exe`，243,658,363 字节。
- SHA-256：`88DD50F4815356F9E86B97ECAD59DE019BF97BE8EFECABE876AD16363DDD5D91`。

Phase 7 禁用模块存档加载兼容增量构建：

- Windows x64 `debugoptimized` 静态增量构建：PASS，0 error。
- 工具链：GCC 16.1.0、Meson 1.11.2、Ninja 1.13.2。
- 产物：`build-phase2-clean/tpt-zh-omnipack.exe`，244,470,300 字节。
- SHA-256：`860F796A547DACF8EAB014AE4060252DD2199224067A21EA91BC64178FB4690D`。

## 测试结果

- Meson 静态测试：Phase 2 为 3/3 PASS；当前含生物门禁为 6/6 PASS。
- Python 工具单元测试：Phase 2 为 28/28 PASS；当前为 35/35 PASS，2 项 C++ 语法测试因该 Python 环境未找到编译器跳过。
- 元素登记：PASS，196 槽、195 活动、1 tombstone、`PT_NUM=512`。
- i18n：PASS，1,154/1,154 键，0 missing、0 extra、0 error；196 行登记表派生的 27 个图鉴枚举键全部存在。
- 元素名称同步只读检查：PASS，0 change。
- Lua 动态元素分配/选择运行回归：PASS；首个动态元素 ID 255，`ui.activeTool(0)` 返回 `OMNITEST_PT_LUA1`，客户端 `Responding=True`。
- clean 构建日志密钥扫描：0 个令牌模式命中；构建目录中 0 个 `powder.pref`、账户或凭据候选文件。
- 图鉴视觉布局、设置复选框交互、英文切换实际交互：BLOCKED；本会话没有可用的受信任 Computer Use native pipe。
- 官方存档载入/往返：NOT RUN。
- 禁用自定义模块存档加载：静态门禁与构建 PASS；实际三选项 UI、OPS 往返和兼容占位转换仍为 NOT RUN。

Phase 3 冶金：

- 元素登记：PASS，279 槽、218 活动、61 保留、`PT_NUM=512`。
- 本地化审计：PASS，en/zh `1,200/1,200`，0 missing、0 extra、0 error；37 项保留英文/代号警告待人工分类。
- 冶金静态审计：PASS，23 个元素、6 条合金配方、1 条炼钢配方、固定 `3x3` 邻域。
- Python 工具单测：30/30 PASS，2 项 C++ 编译验证因当前环境无 C++ 编译器跳过。
- 实际客户端 Lua 冶金回归：PASS；7 条配方/凝固场景、5 组材料行为，`256..278` 全部可解析，客户端保持响应。
- Meson `static` suite：当前为 5/5 PASS，含 registry、i18n、冶金、化学和工具测试。
- 冶金 OPS 往返存档、禁用冶金模块存档警告、固定生产线 FPS/内存压力样本：NOT RUN。

Phase 3 基础化学：

- 元素登记：PASS，370 槽、228 活动、142 保留、`PT_NUM=512`。
- 本地化审计：PASS，en/zh `1,222/1,222`，0 missing、0 extra、0 error；37 项保留英文/代号警告待人工分类。
- 化学静态审计：PASS，10 个元素、7 类受限工艺、固定 `3x3` 邻域。
- 实际客户端 Lua 化学回归：PASS；9 个场景，覆盖裂化、聚合、过氧化物、氨、氯氢、肥料、发酵和负例；`360..369` 全部可解析，客户端保持响应。
- 化学 OPS 往返、禁用化学模块载入警告、固定生产线 FPS/内存压力样本：NOT RUN。

Phase 4 局部生态：

- 元素登记：PASS，370 槽、236 活动、134 保留、`PT_NUM=512`。
- 本地化审计：PASS，en/zh `1,238/1,238`，0 missing、0 extra、0 error；37 项保留英文/代号警告待人工分类。
- 生物静态审计：PASS，8 个元素、6 条局部路径、固定 `3x3` 邻域。
- 实际客户端 Lua 回归：PASS；完整和简化模式各覆盖 6 个场景，`288..295` 全部可解析，客户端保持响应。
- 生物 OPS 往返、关闭生物模块载入警告、固定生态压力样本：NOT RUN。

Phase 5 受控核工业：

- 元素登记：PASS，370 槽、243 活动、127 保留、`PT_NUM=512`。
- 本地化审计：PASS，en/zh `1,252/1,252`，0 missing、0 extra、0 error；37 项保留英文/代号警告待人工分类。
- 核工业静态审计：PASS，7 个元素、4 类受限反应器路径、固定 `3x3` 邻域。
- Python 工具单测：39/39 PASS，2 项 C++ 编译验证因当前 Python 环境未发现编译器跳过。
- Meson `static` suite：7/7 PASS，含核工业门禁。
- 实际客户端 Lua 回归：PASS；受控转换、控制棒、无燃料发生器负例、冷却剂与屏蔽均覆盖；`328..334` 全部可解析，客户端保持响应。
- 核工业 OPS 往返、关闭核工业模块载入警告、固定反应堆压力样本：NOT RUN。

Phase 6 扩展元素图鉴内容：

- 内容登记：PASS，48/48 已实现 OmniPack 元素均有双语配方、生产、用途和危险说明；官方元素不添加推断内容。
- 内容静态审计：PASS，缺失、未知和大小写漂移 identifier 均会被拒绝。
- 目录生成与搜索：PASS，内容字段编入只读目录并加入中英文搜索索引；完整 GUI 视觉布局仍未执行。
- 本地化审计：PASS，en/zh `1,256/1,256`，0 missing、0 extra、0 error；37 项保留英文/代号警告待人工分类。
- Python 工具单测：44/44 PASS，2 项 C++ 编译验证因当前 Python 环境未发现编译器跳过。
- Meson `static` suite：8/8 PASS，含内容审计。

Phase 7 禁用模块存档加载兼容：

- 加载门禁：PASS，检测直接粒子类型及由 `CarriesTypeIn` 标记的 `life`、`ctype`、`tmp..tmp4` 携带元素类型；使用 `TYP()` 保留 OPS 打包元数据。
- 入口覆盖：PASS，本地文件、搜索、预览和 URL 加载都经由 `LoadSaveFile` 或 `LoadSave` 的统一门禁。
- 用户选择：PASS，中文和英文均提供正常加载、只读加载、取消；正常加载不改变原存档粒子，只读加载禁止后续本地保存和在线上传。
- 静态审计与 Python 工具单测：PASS，47/47；2 项 C++ 语法验证因当前 Python 环境未发现编译器跳过。
- Meson `static` suite：9/9 PASS，含存档兼容门禁。
- 实际三选项 UI、禁用模块 OPS 往返、兼容占位转换：NOT RUN。

Phase 8 Windows x64 测试包：

- 测试包封装与审计：PASS，ZIP 仅含可执行文件、许可证、中文说明、更新日志和 `TEST-MANIFEST.txt`；禁止封入个人数据和脚本。
- 测试包门禁：PASS，检查 ZIP 成员精确集合、重复成员、PE `MZ` 头、可执行文件大小/SHA-256 与清单的一致性。
- 静态审计与 Python 工具单测：PASS，50/50。
- Meson `static` suite：10/10 PASS，含测试包审计。
- 交付产物：从提交 `f19cf0634e8c024bc5a7711ee1f2c2d652d7d5f6` 构建；`tpt-zh-omnipack.exe` 为 `244,465,722` 字节，SHA-256 为 `3B96CFEC060705A48681645074AE3C5F53E9A2D40AE56FF76B0E906C2FDBD406`。ZIP 为 `69,790,282` 字节，SHA-256 为 `D97AB00AFB0F7DF42BF8C58981641C1F984B041365B205E8FCD162B2D901A258`。
- 最终客户端 Lua 回归：PASS，模块分配/选择、冶金（7 条配方、5 类行为）、化学（9 条路径）、生态完整/简化模式（各 6 条路径）和核工业（4 条路径）均以交付可执行文件运行通过。
- Windows x64 重建：PASS，0 error；GCC 16.1.0 仍报告两条既有 `PowderToy.cpp` `-Wmaybe-uninitialized` 警告。
- 实际 UI 手动检查、OPS 往返、跨模组迁移、兼容占位和高粒子数压力：NOT RUN。

## 性能结果

Phase 2 没有新增粒子更新函数，因此没有新增每帧粒子成本。Phase 3 冶金反应路径限定为局部 `3x3` 搜索；合金/炼钢共享每帧 2,048 次成功反应预算。基础化学同样只用局部 `3x3` 搜索，所有成功反应共享每帧 1,536 次预算。Phase 4 局部生态同样固定为 `3x3`，共享每帧 1,024 次成功事件预算，且完整和简化模式都不创建新粒子。Phase 5 受控核工业同样固定为 `3x3`，燃料、冷却、屏蔽和中子发生共享 512 次成功事件预算。四个模块均不扫描全粒子表；Lua 回归中客户端保持响应。FPS、内存、粒子增长和大型生产线压力数据仍为 `NOT RUN`，将在 Phase 7 建立固定样本。

## 已知问题

- 当前中文字体的名称、上游版本、固定 BDF、许可证和生成链路已审计；用户确认当前中文可读。完整 DPI、长文本、语言切换和页面矩阵仍未执行。
- 官方 100.0 存档样本载入/重存尚未执行。
- 禁用模块存档的中文提示、正常/只读加载与保存拦截已实现；实际三选项 UI、禁用模块 OPS 往返以及兼容占位转换仍未执行或实现，是发布阻塞项。
- Windows x64 内部测试 ZIP 已生成并有隐私/完整性审计，但 UI 人工复核、OPS 往返、跨来源迁移及高粒子数性能数据仍缺失，不得称为正式公开发布。
- 图鉴内容目前覆盖 48 个已实现 OmniPack 元素；官方元素仍只显示登记说明和基础热学参数，完整图鉴视觉布局尚未执行。
- 曾发现旧 Meson `testlog.txt` 含明文 GitHub PAT 环境变量。该原始日志已删除且未提交；后续测试在清理敏感环境后重跑，令牌模式扫描为 0。凭据轮换属于仓库外必要操作。
- 两条基线 GCC 警告仍待定位。

## 下一阶段

1. 完成 0.1.0-test 的 OPS 往返、禁用模块实际三选项/只读写入门禁与十个固定压力样本。
2. 从重新打包的便携 ZIP 执行中文/英文切换、DPI、四模块 UI 和代表元素检查。
3. 完成本地安全门禁；PAT 撤销、正式远端推送和匿名克隆在取得外部权限后执行。
4. 0.1 本地门禁固化后进入 0.2.0 的用途矩阵和四条跨模块闭环。

## 当前 commit hash

当前长期开发分支：`development/omnipack-1.0`。

- 中文原生 12px 字体：`c743db2fcc49c01033e68023cceff897ed4c35f6`
- 中文字体试包证据：`e18abad9753e61e8f6c9f8fdf671c4bd80a4ca53`
- 便携首次运行安装提示修复：`a590f8b5`

Phase 2 最终加固提交：

`f28cdcb734c6829ae2f69ca10245d494704a8164`

同阶段前置提交：

- 模块门禁与图鉴框架：`d7312a9b75176bcfa938e9727be9131c7835c728`
- 官方元素锁与登记：`08fe8a82b180d357420bb24df34c475d983bb943`
- 严格本地化门禁：`457233acce404dd1f8d2e3566abea23f0ec6c0e3`

Phase 3 冶金提交：`metallurgy: add bounded industrial materials module`（以当前 Git 历史中的该提交为准）。

Phase 7 禁用模块存档加载兼容：`0c2e6cbabbccd40ef380c7953941a3e4e1dbb266`

Phase 8 Windows x64 测试包（本次交付产物源提交）：`f19cf0634e8c024bc5a7711ee1f2c2d652d7d5f6`
