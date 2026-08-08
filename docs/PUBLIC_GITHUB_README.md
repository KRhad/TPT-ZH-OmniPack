# TPT-ZH-OmniPack 1.0.0 / 万象沙盘整合版

面向 Windows x64 与 Android ARM64 的双语物理沙盒整合版。它以 [The Powder Toy](https://powdertoy.co.uk/) 100.0.399 为基础，把简体中文界面、完整元素说明、118 元素周期表和多类现实材料统一到一个仍然自由开放的沙盒中。

没有任务、成就、科技树、发现进度或强制解锁。启用的材料可以直接选择；同一种材料从周期表、材料分类或搜索进入时，始终落到同一个稳定材料 ID，不复制概念，也不破坏旧存档。

> **发布状态：** 当前公开的是 1.0.0 构建快照，不是已经完成全部门禁的签名正式版。Windows 程序尚无 Authenticode 签名，Android APK 使用项目测试证书；物理 Android 设备、完整 GUI/DPI 人工矩阵和独立长跑仍未全部完成，尚未创建 `v1.0.0` tag 或 GitHub Release，因此 `release_ready=false`。

## 核心特色

- 完整显示并可访问 118 个化学元素；元素面板按单质、同素异形体、同位素、氧化物、氢化物、酸碱盐及相关材料分组；
- 466 种可直接选择的材料，覆盖冶金、无机/有机化学、核工业、电子、生态、污染、矿物、陶瓷、玻璃、聚合物、半导体与工程材料；
- 精简主材料菜单，保留官方元素、OmniPack 独立玩法材料和必要快捷入口；
- 支持中文名、英文名、元素符号、化学式、原子序数和内部代号搜索；
- 195 个官方规范元素具备双语完整说明，OmniPack 材料另有配方、生产、用途和危险信息；
- 所有玩家可见的说明字段均纳入发布字体覆盖门禁，触屏选择后的简要说明会自动淡出；
- Windows 与 Android 共用模拟规则、稳定 ID、模块开关和 OPS 存档映射；
- 开始界面直接显示项目 GitHub 地址，并通过公开静态清单检查更新。

源码可以直接浏览：[src](./src/)、[resources](./resources/)、[Meson 配置](./meson.build)和[必要生成器](./tools/)。公开历史从无测试源码快照建立，不包含开发分支的私有测试历史。

## 下载

| 平台 / 内容 | 文件 | 说明 |
|---|---|---|
| Windows x64 | [用户 ZIP](./TPT-ZH-OmniPack-1.0.0-Windows-x64.zip) · [SHA-256](./TPT-ZH-OmniPack-1.0.0-Windows-x64.zip.sha256) | 便携版；解压后运行 `tpt-zh-omnipack.exe` |
| Android ARM64 | [APK](./updates/TPT-ZH-OmniPack-1.0.0-Android-arm64-v8a.apk) · [SHA-256](./updates/TPT-ZH-OmniPack-1.0.0-Android-arm64-v8a.apk.sha256) | Android 5.0+、仅 `arm64-v8a`；安装和升级由系统确认 |
| 调试符号 | [Symbols ZIP](./TPT-ZH-OmniPack-1.0.0-Symbols-Windows-x64.zip) · [SHA-256](./TPT-ZH-OmniPack-1.0.0-Symbols-Windows-x64.zip.sha256) | 供崩溃定位，不是运行必需文件 |
| 对应源码 | [Source ZIP](./TPT-ZH-OmniPack-1.0.0-Source.zip) · [SHA-256](./TPT-ZH-OmniPack-1.0.0-Source.zip.sha256) | 完整可构建、无公开测试目录的源码快照 |

补充文件：[Android 包清单](./ANDROID-MANIFEST.json)、[Android 使用说明](./README-Android.md)、[发布清单](./RELEASE-MANIFEST.txt)和[更新通道清单](./updates/UPDATE-MANIFEST.txt)。运行未签名程序前请核对旁置 SHA-256。

## 自动更新如何工作

Windows 与 Android 开始界面均显示 `https://github.com/KRhad/TPT-ZH-OmniPack`。发布构建从本仓库 `public-source` 分支读取 `Startup.json`，按平台选择 Windows 更新包或 Android APK，并在应用前同时核对精确字节数和 SHA-256。

- Windows 下载 BuTT + bzip2 更新包，在保留失败回滚路径的前提下替换便携 EXE；
- Android 下载并核对 APK 后调用系统 `PackageInstaller`；Android 8+ 如未授权，会先打开“安装未知应用”设置，最终安装始终需要系统界面和用户确认；
- 游戏不会静默安装 Android APK，也不会在运行时联网获取材料分类或元素说明。

## 参考模组与致谢

本项目以 [The Powder Toy](https://github.com/The-Powder-Toy/The-Powder-Toy) 为底座，并参考了以下社区模组。表中的“参考/重写”不表示直接合并整个模组：本项目没有打包这些模组的源码树或二进制，也没有逐行复制第三方元素更新函数；实际采用的材料概念、参数适配与文件级来源均保留稳定 commit 和许可证记录。

| 模组 / 作者 | 本项目中的参考边界 |
|---|---|
| [Cracker1000 TPT](https://github.com/cracker1000/The-Powder-Toy) / cracker1000 | 铜的参数与玩法定位、自动化设计参考；更新逻辑改为当前架构下的局部有界实现 |
| [SpikeViper Biology](https://github.com/SpikeViper/The-Powder-Toy) / SpikeViper | 生物及核内容的玩法边界参考；OmniPack 对应模块独立实现 |
| [TPT Ultimata Mod](https://github.com/Bowserinator/TPT-Ultimata-Mod) / Bowserinator | 特殊物理、电子、核内容及 `SOIL/BLOD` 概念参考；采用项重新实现 |
| [Jacob1 Mod](https://github.com/jacob1/The-Powder-Toy) / jacob1 | 自动化、界面体验和存档来源识别参考 |
| [TPT-Alchemy](https://github.com/jacob1/TPT-Alchemy) / jacob1 | 仅评估无解锁的反应概念；进度、发现和强制解锁系统明确不采用 |
| [Seppo's Metallurgy Mod SRC](https://github.com/SeppoTPT/Seppo-s-Metallurgy-Mod-SRC) / SeppoTPT | 冶金命名与公开反应碎片参考；缺失实现不从二进制反推，相关玩法按清洁室方式实现 |
| [Cyens Toy](https://github.com/cbeimers113/cyens-toy) / cbeimers113 | 烃化学、分馏和特殊物理概念参考；不覆盖官方元素语义 |
| [Cyens Toy Source](https://github.com/cbeimers113/cyens-toy-src) / cbeimers113 | `ACET/UREA` 的 GPL 文件级材料概念来源；属性、反应和预算重新实现 |
| [TPT Biological Mod](https://github.com/sam-astro/TPT-Biological-Mod) / sam-astro | `BLOD` 携氧概念的交叉核对；状态机未复制 |
| [nucular's Mod](https://github.com/nucular/The-Powder-Toy-nuculars-mod) / nucular | `SOIL` 储水及核主题行为的交叉核对；采用内容重新实现 |

精确 commit、文件级锚点、采用/拒绝结论和许可证见[第三方来源说明](./docs/THIRD_PARTY_SOURCES.md)、[移植台账](./docs/PORTING_LEDGER.md)、[模组来源目录](./docs/MOD_SOURCE_CATALOG.csv)及[第三方许可证清单](./docs/THIRD_PARTY_LICENSE_MANIFEST.csv)。授权不明或源码缺失的候选只保留审计记录，不进入构建与发布包。

## 从源码构建

公开源码不包含 `tests/`、`tools/tests/`、`tools/runtime/`、压力测试脚本与样本、测试报告、日志、截图、私测存档、性能工件、本地配置或个人数据。使用 Meson 时关闭开发测试目标：

```powershell
meson setup build --buildtype=release -Dbuild_tests=false -Dstatic=prebuilt -Drelease_label=1.0.0 -Doverride_display_version=1.0.0 -Dupdate_build=1
meson compile -C build
```

`update_build=1` 是 1.0.0 公开快照的内部更新修订号，不改变界面、文件名或 Android `versionName` 中的 `1.0.0`。它让较早的 1.0.0 build 0 客户端只更新一次，并防止更新后的客户端反复提示同一资产。

Android ARM64 使用 `tools/build_android.ps1 -UpdateBuild 1 -AndroidVersionCode 1000001`。需要 Android SDK、NDK r29、Build Tools 35.0.0、Platform 31、JDK 8、JDK 17+ 与 MSYS2 UCRT64；脚本固定 `build_tests=false`，密钥必须位于仓库外。详细参数见 [Android 构建说明](./docs/ANDROID_PORT.md)。

## 许可证与来源

项目整体依据 GNU GPL v3 发布，并保留 The Powder Toy 及兼容来源作者的版权与通知。用户包和源码包均携带第三方来源记录、字体许可证及依赖许可证。项目开发使用了 OpenAI Codex / AI 辅助，披露见 [AI_DISCLOSURE.md](./docs/AI_DISCLOSURE.md)。

---

TPT-ZH-OmniPack is an unofficial GPL-3.0 bilingual physics-sandbox integration based on The Powder Toy 100.0.399. It supports Windows x64 and Android ARM64, provides a complete 118-element picker, unified material routing, bilingual search and descriptions, stable save IDs, and SHA-256-verified GitHub updates. Public source omits private developer tests; configure it with `-Dbuild_tests=false`. This snapshot remains `release_ready=false` until the outstanding signing, device, long-run, and visual gates are completed.
