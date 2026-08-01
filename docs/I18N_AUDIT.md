# 简体中文本地化自动审计

> 本报告由 `tools/i18n_audit.py` 生成。错误是发布阻塞项；警告必须人工复核，但不会单独令 `--check` 失败。

## 审计对象

- 生成时间（UTC）：`2026-08-01T06:49:38+00:00`
- 当前提交：`4f473ebefe59e436228ec7799f9363d39fe166b1`
- 英文文件：`C:\Users\KR\TPT-ZH-OmniPack\src\lang\en-US.json`
- 中文文件：`C:\Users\KR\TPT-ZH-OmniPack\src\lang\zh-CN.json`
- 命令：`python tools/i18n_audit.py --check --write-report docs\I18N_AUDIT.md`

## 结论

**静态审计通过**

| 指标 | 数量 |
|---|---:|
| 英文键总数 | 1584 |
| 中文键总数 | 1584 |
| 中文缺失键 | 0 |
| 中文多余键 | 0 |
| 英文/中文重复键 | 0 |
| 英文/中文空值 | 0 |
| 占位符错误 | 0 |
| 颜色控制错误 | 0 |
| 换行结构错误 | 0 |
| 链接控制错误 | 0 |
| 结束控制错误 | 0 |
| 疑似未翻译/保留英文项 | 37 |
| 乱码/非法字符项 | 0 |
| 宽度风险 | 1 |
| 源码登记元素 | 386 |
| 缺少英文/中文元素短说明 | 0 |
| 缺少英文/中文元素正式名称 | 0 |
| 源码登记菜单 | 16 |
| 缺少英文/中文菜单键 | 0 |
| 源码字面量 Tr() 键 | 981 |
| 缺少英文/中文 Tr() 键 | 0 |
| 图鉴登记表行 | 513 |
| 图鉴枚举所需键 | 32 |
| 缺少英文/中文图鉴枚举键 | 0 |
| 未知图鉴菜单类别 | 0 |
| 发布阻塞错误 | 0 |
| 人工复核警告 | 38 |

## 元素与菜单登记

- 源码元素：386；短说明缺失（英/中）：0/0；正式名称缺失（英/中）：0/0。
- 源码菜单：16；菜单键缺失（英/中）：0/0。
- 源码字面量 `Tr()` 键：981；缺失（英/中）：0/0。

元素四字符 `Name` 不视为正式英文/中文名称。当前审计约定正式名称键为 `sim.elem.<Identifier>.name`，现有 `sim.elem.<Identifier>` 继续作为短说明键。

## 发布阻塞错误

- 无。

## 人工复核警告

- `translation.identical` `gameview.fps.grid_suffix`：英中值完全相同；可能是代号、URL、格式片段或未翻译文本，需人工分类。
- `translation.identical` `gameview.fps.temp_h`：英中值完全相同；可能是代号、URL、格式片段或未翻译文本，需人工分类。
- `translation.identical` `intro.title_after_version`：英中值完全相同；可能是代号、URL、格式片段或未翻译文本，需人工分类。
- `translation.identical` `intro.title_prefix`：英中值完全相同；可能是代号、URL、格式片段或未翻译文本，需人工分类。
- `translation.identical` `intro.version_prefix`：英中值完全相同；可能是代号、URL、格式片段或未翻译文本，需人工分类。
- `translation.identical` `login.use_account_not_email_suffix`：英中值完全相同；可能是代号、URL、格式片段或未翻译文本，需人工分类。
- `translation.identical` `options.deco.gamma18`：英中值完全相同；可能是代号、URL、格式片段或未翻译文本，需人工分类。
- `translation.identical` `options.deco.gamma22`：英中值完全相同；可能是代号、URL、格式片段或未翻译文本，需人工分类。
- `translation.identical` `options.deco.srgb`：英中值完全相同；可能是代号、URL、格式片段或未翻译文本，需人工分类。
- `translation.identical` `search.status_favouring_suffix`：英中值完全相同；可能是代号、URL、格式片段或未翻译文本，需人工分类。
- `translation.identical` `search.status_publishing_suffix`：英中值完全相同；可能是代号、URL、格式片段或未翻译文本，需人工分类。
- `translation.identical` `search.status_unfavouring_suffix`：英中值完全相同；可能是代号、URL、格式片段或未翻译文本，需人工分类。
- `translation.identical` `search.status_unpublishing_suffix`：英中值完全相同；可能是代号、URL、格式片段或未翻译文本，需人工分类。
- `layout.width_risk` `sim.elem.OMNI_PT_NO`：启发式显示宽度 58 超过窄 UI 阈值 24；必须以实际字体截图复核。
- `translation.identical` `sim.gol.2X2.name`：英中值完全相同；可能是代号、URL、格式片段或未翻译文本，需人工分类。
- `translation.identical` `sim.gol.34.name`：英中值完全相同；可能是代号、URL、格式片段或未翻译文本，需人工分类。
- `translation.identical` `sim.gol.AMOE.name`：英中值完全相同；可能是代号、URL、格式片段或未翻译文本，需人工分类。
- `translation.identical` `sim.gol.ASIM.name`：英中值完全相同；可能是代号、URL、格式片段或未翻译文本，需人工分类。
- `translation.identical` `sim.gol.BRAN.name`：英中值完全相同；可能是代号、URL、格式片段或未翻译文本，需人工分类。
- `translation.identical` `sim.gol.COAG.name`：英中值完全相同；可能是代号、URL、格式片段或未翻译文本，需人工分类。
- `translation.identical` `sim.gol.DANI.name`：英中值完全相同；可能是代号、URL、格式片段或未翻译文本，需人工分类。
- `translation.identical` `sim.gol.DMOE.name`：英中值完全相同；可能是代号、URL、格式片段或未翻译文本，需人工分类。
- `translation.identical` `sim.gol.FRG2.name`：英中值完全相同；可能是代号、URL、格式片段或未翻译文本，需人工分类。
- `translation.identical` `sim.gol.FROG.name`：英中值完全相同；可能是代号、URL、格式片段或未翻译文本，需人工分类。
- `translation.identical` `sim.gol.GNAR.name`：英中值完全相同；可能是代号、URL、格式片段或未翻译文本，需人工分类。
- `translation.identical` `sim.gol.GOL.name`：英中值完全相同；可能是代号、URL、格式片段或未翻译文本，需人工分类。
- `translation.identical` `sim.gol.HLIF.name`：英中值完全相同；可能是代号、URL、格式片段或未翻译文本，需人工分类。
- `translation.identical` `sim.gol.LLIF.name`：英中值完全相同；可能是代号、URL、格式片段或未翻译文本，需人工分类。
- `translation.identical` `sim.gol.LOTE.name`：英中值完全相同；可能是代号、URL、格式片段或未翻译文本，需人工分类。
- `translation.identical` `sim.gol.MAZE.name`：英中值完全相同；可能是代号、URL、格式片段或未翻译文本，需人工分类。
- `translation.identical` `sim.gol.MOVE.name`：英中值完全相同；可能是代号、URL、格式片段或未翻译文本，需人工分类。
- `translation.identical` `sim.gol.MYST.name`：英中值完全相同；可能是代号、URL、格式片段或未翻译文本，需人工分类。
- `translation.identical` `sim.gol.PGOL.name`：英中值完全相同；可能是代号、URL、格式片段或未翻译文本，需人工分类。
- `translation.identical` `sim.gol.REPL.name`：英中值完全相同；可能是代号、URL、格式片段或未翻译文本，需人工分类。
- `translation.identical` `sim.gol.SEED.name`：英中值完全相同；可能是代号、URL、格式片段或未翻译文本，需人工分类。
- `translation.identical` `sim.gol.STAN.name`：英中值完全相同；可能是代号、URL、格式片段或未翻译文本，需人工分类。
- `translation.identical` `sim.gol.STAR.name`：英中值完全相同；可能是代号、URL、格式片段或未翻译文本，需人工分类。
- `translation.identical` `sim.gol.WALL.name`：英中值完全相同；可能是代号、URL、格式片段或未翻译文本，需人工分类。

## 审计规则说明

- 严格 UTF-8/JSON、重复键、平面字符串对象、键集合、空值、占位符及 TPT 控制符属于可自动判定门禁。
- 颜色控制支持 `\b` 加 `wgorlbtuU`；链接必须使用 `{a:URL|文本}` 并紧跟 `\x0E` 或实际 0x0E。
- 换行比较换行总数、首尾状态及连续换行段；任何差异均需先修正或明确调整审计策略。
- 疑似英文和宽度检查是启发式。宽度按 Unicode 东亚宽度估算，不能替代实际 TPT 字体渲染截图。
- 当前语言加载器对字面量 `\x0E` 的运行时转换能力仍需由 C++ 测试单独验证；本脚本验证英中结构一致性。
