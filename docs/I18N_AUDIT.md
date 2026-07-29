# 简体中文本地化审计

## Phase 0 基线

审计对象：

- 仓库：`Dragonrster/The-Powder-Toy-Chinese`
- 分支：`i18n-new`
- 提交：`445fab51dcf66057e645371aa9c9a556425b3d2f`
- TPT：`100.0.398`

## 自动可复核统计

| 指标 | 数量 |
|---|---:|
| `en-US` 键 | 878 |
| `zh-CN` 键 | 878 |
| 中文缺失键 | 0 |
| 中文多余键 | 0 |
| 重复键 | 0 |
| 中文空字符串 | 1 |
| 完全等同英文的中文值 | 37 |
| 源码字面量 `Tr()` 键 | 747 |

中文空值为 `gameview.tooltip.open_forum_thread_suffix`。37 个等同值多数是代号、格式片段、GOL 名称或允许保留的技术文本，仍需人工分类。

## 已确认缺陷

1. `prefs.Get("Language", 0)` 令首次启动默认选择英文，不满足默认简体中文。
2. `OptionsModel`/`OptionsController` 保留语言接口，但当前 `OptionsView` 没有语言下拉框。
3. `BASE.cpp` 直接硬编码中文说明，`en-US` 和 `zh-CN` 均缺少 `sim.elem.DEFAULT_PT_BASE`，英文切换不完整。
4. 当前只有元素说明键，没有中文正式名称、别名和中文搜索元数据。
5. 至少以下玩家可见文本仍为英文：
   - `PreviewView.cpp`：`Save from newer version`
   - `LocalBrowserView.cpp`：`Next`
   - `OptionsView.cpp`：`X / Y / Total`
   - `Textbox.cpp`：`Copy / Paste`
6. 错误、客户端通知、存档异常等路径仍需完整静态扫描。
7. 当前 `Localization.cpp` 使用遇错静默停止的手写 JSON 解析器，不能可靠报告重复键、尾部垃圾或错误位置。
8. 现有中文字库没有来源和授权记录，不能进入发布包。

## 占位符与控制符

Phase 0 的简单占位符比较只对 credits 标记产生疑似项，属于启发式误报。Phase 2 的 `tools/i18n_audit.py` 必须分别比较：

- `printf`/format 占位符的名称、类型和数量；
- TPT 颜色控制字节；
- 显式换行；
- JSON 转义；
- 不能翻译的内部代码片段。

## UI 宽度风险

尚未执行像素级 UI 宽度测试。Phase 6 将使用实际字体度量和渲染截图，不以字符数作为唯一结论。窄按钮只显示短译名，完整说明进入图鉴。

## 发布门禁

- 缺失键：必须为 0；
- 空字符串：必须为 0；
- 占位符错误：必须为 0；
- 玩家可见硬编码英文：必须完成分类，非允许项为 0；
- 中文字体：必须有名称、作者、来源 URL、许可证与生成方法；
- 默认简体中文与英文切换均需实际运行验证。

## 当前结论

`不合格（Phase 0 基线）`。键集合相等不等于完全汉化；默认语言、切换 UI、元素中文名称、硬编码文本、严格解析和字体授权均未满足发布要求。

