# TPT-ZH-OmniPack 1.0.0 / 万象沙盘整合版

TPT-ZH-OmniPack 是基于 [The Powder Toy](https://powdertoy.co.uk/) 100.0.399 的非官方 GPL-3.0 双语物理沙盒整合版，提供简体中文与英文界面。游戏保持自由沙盒定位，不包含任务、成就、科技树或强制解锁。

## 主要内容

- 完整的 118 元素周期表选择器；
- 单质、同素异形体、同位素、氧化物、氢化物和酸碱盐分组；
- 有机物、合金、矿物、陶瓷、玻璃、聚合物、半导体和工程材料关联；
- 精简的主材料菜单，以及独立的“有机材料”和“合金与工程材料”分类；
- 中英文材料搜索、完整元素说明、可滚动材料面板和长按图鉴；
- 稳定材料 ID、OPS 存档映射、模块启用/禁用行为及高 ID 材料兼容。

## 下载

- [Windows x64 用户包](./TPT-ZH-OmniPack-1.0.0-Windows-x64.zip)
- [Windows x64 SHA-256](./TPT-ZH-OmniPack-1.0.0-Windows-x64.zip.sha256)
- [1.0.0 对应源码](./TPT-ZH-OmniPack-1.0.0-Source.zip)
- [源码 SHA-256](./TPT-ZH-OmniPack-1.0.0-Source.zip.sha256)

用户包为便携版，解压后运行 `tpt-zh-omnipack.exe`。当前 Windows 程序未签名，运行前请核对 SHA-256。

## 从源码构建

公开源码包不包含开发测试目录、测试脚本和测试报告。使用 Meson 配置时请关闭测试目标：

```powershell
meson setup build --buildtype=release -Dbuild_tests=false -Drelease_label=1.0.0
meson compile -C build
```

静态 Windows 构建可再加 `-Dstatic=prebuilt`，Meson 会按照 `subprojects` 中的固定 wrap 下载对应依赖。

## 许可证与来源

项目整体依据 GNU GPL v3 发布，并保留 The Powder Toy 及兼容来源作者的版权和通知。用户包和源码包均包含第三方来源记录、字体许可证及依赖许可证信息。项目开发使用了 OpenAI Codex / AI 辅助，披露见源码中的 `docs/AI_DISCLOSURE.md`。

---

TPT-ZH-OmniPack is an unofficial GPL-3.0 bilingual physics-sandbox integration based on The Powder Toy 100.0.399. It includes a complete 118-element periodic-table picker, unified material routing, bilingual search and descriptions, stable save IDs, and directly selectable sandbox content. The public source archive omits developer test assets; configure it with `-Dbuild_tests=false`.
