# TPT-ZH-OmniPack 开发进度

> 本文件只记录有源码差异、构建日志或测试输出支持的结果。未执行项目明确标为 `NOT RUN` 或 `BLOCKED`。

## 当前阶段

Phase 3：工业冶金首批完成；下一阶段为 Phase 3：基础化学。

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
- 建立模块设置 UI。六个内容模块可控制；四个尚未实现的设置明确标注“待实现”并强制禁用。
- 注册 3 个 Meson 静态测试目标，并增加可复现 Windows Lua 运行回归脚本。
- 完成首批工业冶金：在固定 ID `256..278` 注册 23 个材料、工艺与回收元素，且不占用 `196..255` 官方兼容缓冲区。
- 建立集中、局部的冶金反应引擎：6 条定比熔融合金配方、1 条炼钢配方、木材炭化、煤炭炼焦、压力碎料回收、铜腐蚀、镁燃烧、锌牺牲保护与镍铬电热。
- 合金与炼钢只收集触发点 `3x3` 邻域，每帧共享 2,048 次成功反应预算，并使用 tick 标记阻止同帧重复级联。
- 为 23 个冶金元素登记中英文正式名称、短说明、来源、稳定 ID、菜单分类、存档状态与测试状态；缺失的 Seppo 构造器明确按公开概念独立实现，未从二进制反推或复制。
- 增加冶金静态审计、两项 Python 单测与实际客户端 Lua 回归，覆盖配方、熔体凝固、炭化、电热、压力/碎料、镁燃烧与锌保护。

## 修改文件

Phase 2 和已完成的 Phase 3 冶金的主要变更：

- `docs/ELEMENT_REGISTRY.csv`
- `docs/ELEMENT_DESIGN.md`
- `docs/I18N_AUDIT.md`
- `tools/data/official_elements_100_0.csv`
- `tools/element_registry_check.py`
- `tools/i18n_audit.py`
- `tools/sync_element_localization.py`
- `tools/generate_element_catalog.py`
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

## 汉化状态

- 英文键：1,200
- 中文键：1,200
- 缺失键：0
- 多余键：0
- 发布阻塞错误：0
- 占位符/控制符错误：0
- 疑似未翻译警告：37，均保留在 `docs/I18N_AUDIT.md` 供人工分类
- 官方活动元素名称登记：195/195；冶金元素中英文正式名称登记：23/23
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
- 本次恢复会话可复核现有二进制和全部源码/运行门禁；由于当前 PATH 缺少 Meson 和 C++ 编译器，未从当前会话重新编译。该环境限制不改变上述 2026-07-29 构建结果。

## 测试结果

- Meson 静态测试：3/3 PASS。
- Python 工具单元测试：28/28 PASS。
- 元素登记：PASS，196 槽、195 活动、1 tombstone、`PT_NUM=512`。
- i18n：PASS，1,154/1,154 键，0 missing、0 extra、0 error；196 行登记表派生的 27 个图鉴枚举键全部存在。
- 元素名称同步只读检查：PASS，0 change。
- Lua 动态元素分配/选择运行回归：PASS；首个动态元素 ID 255，`ui.activeTool(0)` 返回 `OMNITEST_PT_LUA1`，客户端 `Responding=True`。
- clean 构建日志密钥扫描：0 个令牌模式命中；构建目录中 0 个 `powder.pref`、账户或凭据候选文件。
- 图鉴视觉布局、设置复选框交互、英文切换实际交互：BLOCKED；本会话没有可用的受信任 Computer Use native pipe。
- 官方存档载入/往返：NOT RUN。
- 禁用自定义模块存档警告/只读/占位：NOT RUN；尚无正式自定义元素。

Phase 3 冶金：

- 元素登记：PASS，279 槽、218 活动、61 保留、`PT_NUM=512`。
- 本地化审计：PASS，en/zh `1,200/1,200`，0 missing、0 extra、0 error；37 项保留英文/代号警告待人工分类。
- 冶金静态审计：PASS，23 个元素、6 条合金配方、1 条炼钢配方、固定 `3x3` 邻域。
- Python 工具单测：30/30 PASS，2 项 C++ 编译验证因当前环境无 C++ 编译器跳过。
- 实际客户端 Lua 冶金回归：PASS；7 条配方/凝固场景、5 组材料行为，`256..278` 全部可解析，客户端保持响应。
- Meson `static` suite：2026-07-29 为 4/4 PASS；本恢复会话因 PATH 缺少 Meson 未重跑。
- 冶金 OPS 往返存档、禁用冶金模块存档警告、固定生产线 FPS/内存压力样本：NOT RUN。

## 性能结果

Phase 2 没有新增粒子更新函数，因此没有新增每帧粒子成本。Phase 3 冶金的反应路径限定为局部 `3x3` 搜索；合金/炼钢共享每帧 2,048 次成功反应预算，且不进行全粒子表扫描。clean build 和 Lua 启动回归未崩溃。FPS、内存、粒子增长和大型生产线压力数据仍为 `NOT RUN`，将在 Phase 7 建立固定样本。

## 已知问题

- 中文字体 `resources/font.bz2` 的名称、来源和许可证仍不可追溯，是正式发布阻塞项。
- 视觉中文布局与实际语言切换交互尚未执行。
- 官方 100.0 存档样本载入/重存尚未执行。
- 关闭冶金模块后加载含该模块元素的中文警告、只读和占位路径尚未实现；这是 Phase 7 存档兼容测试的发布阻塞项。
- 图鉴框架目前显示登记说明和基础热学参数；反应、生产方法、用途、危险等级、原作者等正式字段将在内容登记扩展时加入。
- 曾发现旧 Meson `testlog.txt` 含明文 GitHub PAT 环境变量。该原始日志已删除且未提交；后续测试在清理敏感环境后重跑，令牌模式扫描为 0。凭据轮换属于仓库外必要操作。
- 两条基线 GCC 警告仍待定位。

## 下一阶段

1. 在固定 ID `360..391` 中实现基础化学的中央反应表，避免元素更新逻辑相互覆盖。
2. 扩展图鉴登记字段与反应数据结构，添加配方、生产方法、用途和危险说明。
3. 实现禁用模块存档的中文警告和兼容占位策略，再进行 OPS 往返存档测试。
4. 建立冶金生产线、官方存档和性能固定样本。

## 当前 commit hash

Phase 2 最终加固提交：

`f28cdcb734c6829ae2f69ca10245d494704a8164`

同阶段前置提交：

- 模块门禁与图鉴框架：`d7312a9b75176bcfa938e9727be9131c7835c728`
- 官方元素锁与登记：`08fe8a82b180d357420bb24df34c475d983bb943`
- 严格本地化门禁：`457233acce404dd1f8d2e3566abea23f0ec6c0e3`

Phase 3 冶金提交：`153106ac78fcf16ecff40d37bd74d9b129c2e9aa`。
