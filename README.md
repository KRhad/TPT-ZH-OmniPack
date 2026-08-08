# TPT-ZH-OmniPack 1.0.0 / 万象沙盘整合版

TPT-ZH-OmniPack 是基于 [The Powder Toy](https://powdertoy.co.uk/) 100.0.399 的非官方 GPL-3.0 双语物理沙盒整合版，提供简体中文与英文界面。游戏保持自由沙盒定位，不包含任务、成就、科技树或强制解锁。

> 当前公开的是 **1.0.0 未签名构建快照**，不是已经通过全部发布门禁的 GitHub Release。项目没有创建 `v1.0.0` Tag 或 GitHub Release；GUI/DPI 人工视觉矩阵、独立两小时长跑和代码签名仍未完成，因此 `release_ready=false`。详见 [发布状态](./RELEASE-STATUS.md)。

## 主要内容

- 完整的 118 元素周期表选择器；
- 单质、同素异形体、同位素、氧化物、氢化物和酸碱盐分组；
- 有机物、合金、矿物、陶瓷、玻璃、聚合物、半导体和工程材料关联；
- 精简的主材料菜单，以及独立的有机材料、合金与工程材料入口；
- 中英文材料搜索、完整元素说明、可滚动材料面板和长按图鉴；
- 稳定材料 ID、OPS 存档映射、模块启用/禁用行为及高 ID 材料兼容。

源码可直接浏览：[src](./src/)、[resources](./resources/)、[Meson 配置](./meson.build) 和 [必要生成器](./tools/)。

## 下载

- [Windows x64 用户包](./TPT-ZH-OmniPack-1.0.0-Windows-x64.zip)
- [用户包 SHA-256](./TPT-ZH-OmniPack-1.0.0-Windows-x64.zip.sha256)
- [Windows x64 Symbols 包](./TPT-ZH-OmniPack-1.0.0-Symbols-Windows-x64.zip)
- [Symbols SHA-256](./TPT-ZH-OmniPack-1.0.0-Symbols-Windows-x64.zip.sha256)
- [对应源码 ZIP](./TPT-ZH-OmniPack-1.0.0-Source.zip)
- [源码 SHA-256](./TPT-ZH-OmniPack-1.0.0-Source.zip.sha256)
- [发布 Manifest](./RELEASE-MANIFEST.txt)
- [源码 ZIP 成员 Manifest](./SOURCE-MANIFEST.txt)

用户包为便携版，解压后运行 `tpt-zh-omnipack.exe`。当前 Windows 程序没有 Authenticode 签名，运行前请核对 SHA-256。`SOURCE-MANIFEST.txt` 描述源码 ZIP 的原始载荷；公开分支在该载荷之上增加了仓库 README、状态、工作流和下载产物。

## 从源码构建

需要 Meson 0.64 或更高版本、Ninja、Python 3 和支持 C++20 的编译器。Windows 推荐在 MSYS2 UCRT64 环境中构建：

```bash
meson setup build --buildtype=release -Dbuild_tests=false -Dstatic=prebuilt -Drelease_label=1.0.0 -Dresolve_vcs_tag=no
meson compile -C build
```

公开源码不包含 `tests/`、`tools/tests/`、`tools/runtime/`、压力测试脚本/样本、测试报告、日志、截图、私测存档、性能工件、本地配置或个人数据。`build_tests=false` 会关闭开发验证目标。仓库内的 [GitHub Actions 工作流](./.github/workflows/build-source.yml)也只配置和编译，不运行测试、不发布产物。

## 许可证与来源

项目整体依据 GNU GPL v3 发布，并保留 The Powder Toy 及兼容来源作者的版权和通知。`LICENSE`、[第三方来源说明](./docs/THIRD_PARTY_SOURCES.md)、[第三方许可证清单](./docs/THIRD_PARTY_LICENSE_MANIFEST.csv)及字体许可证均随源码公开。项目开发使用了 OpenAI Codex / AI 辅助，披露见 [AI_DISCLOSURE.md](./docs/AI_DISCLOSURE.md)。

---

TPT-ZH-OmniPack is an unofficial GPL-3.0 bilingual physics-sandbox integration based on The Powder Toy 100.0.399. This repository publishes a browsable source tree and unsigned 1.0.0 build artifacts, but it is not yet a tagged GitHub Release and remains `release_ready=false`.
