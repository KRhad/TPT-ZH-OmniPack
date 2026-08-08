# TPT-ZH-OmniPack 1.0.0 / 万象沙盘整合版

TPT-ZH-OmniPack 是基于 [The Powder Toy](https://powdertoy.co.uk/) 100.0.399 的非官方 GPL-3.0 双语物理沙盒整合版，提供简体中文与英文界面。游戏保持自由沙盒定位，不包含任务、成就、科技树或强制解锁。

> 当前公开的是 **1.0.0 未签名构建快照**，不是已经通过全部发布门禁的 GitHub Release。项目没有创建 `v1.0.0` Tag 或 GitHub Release；GUI/DPI 人工视觉矩阵、独立两小时长跑和代码签名仍未完成，因此 `release_ready=false`。详见根目录 `RELEASE-STATUS.md`。

## 主要内容

- 完整的 118 元素周期表选择器；
- 单质、同素异形体、同位素、氧化物、氢化物和酸碱盐分组；
- 有机物、合金、矿物、陶瓷、玻璃、聚合物、半导体和工程材料关联；
- 精简的主材料菜单，以及独立的有机材料、合金与工程材料入口；
- 中英文材料搜索、完整元素说明、可滚动材料面板和长按图鉴；
- 稳定材料 ID、OPS 存档映射、模块启用/禁用行为及高 ID 材料兼容。

## 下载与构建

公开仓库根目录提供 Windows x64 用户包、Symbols 包、对应源码 ZIP、SHA-256 sidecar 和 `RELEASE-MANIFEST.txt`。用户包为便携版，当前 Windows 程序没有 Authenticode 签名。

```bash
meson setup build --buildtype=release -Dbuild_tests=false -Dstatic=prebuilt -Drelease_label=1.0.0 -Dresolve_vcs_tag=no
meson compile -C build
```

公开源码不包含开发测试、运行回归、压力样本、测试报告、日志、截图、私测存档、性能工件、本地配置或个人数据。GitHub Actions 只配置和编译，不运行测试。

## 许可证与来源

项目整体依据 GNU GPL v3 发布，并保留 The Powder Toy 及兼容来源作者的版权和通知。`LICENSE`、第三方来源说明、许可证清单和字体许可证均随源码公开。项目开发使用了 OpenAI Codex / AI 辅助，披露见 `docs/AI_DISCLOSURE.md`。

---

TPT-ZH-OmniPack is an unofficial GPL-3.0 bilingual physics-sandbox integration based on The Powder Toy 100.0.399. This repository publishes a browsable source tree and unsigned 1.0.0 build artifacts, but it is not yet a tagged GitHub Release and remains `release_ready=false`.
