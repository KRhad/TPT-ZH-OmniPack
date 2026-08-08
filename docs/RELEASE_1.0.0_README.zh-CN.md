# TPT-ZH-OmniPack 1.0.0

TPT-ZH-OmniPack 是基于 The Powder Toy 100.0.399 的非官方 GPL-3.0 物理沙盒整合版，提供简体中文与英文界面，并整合冶金、化学、核工业、电子、生态、环境材料和完整的 118 元素周期表选择器。

周期表和材料菜单只是统一选择入口：同一种材料从不同入口进入时始终指向同一个稳定材料标识与存档 ID。原有存档、模块启用/禁用行为、Lua 选择以及高 ID 材料映射均予以保留。

## 使用方法

Windows：将 ZIP 解压到一个新目录，然后运行 `tpt-zh-omnipack.exe`。这是便携版，默认不会安装文件关联。跨版本使用前请备份重要存档。

Android：在 Android 5.0 或更高版本的 ARM64 设备上安装 `TPT-ZH-OmniPack-1.0.0-Android-arm64-v8a.apk`。触屏短按左下角打开图标浏览本地存档，长按进入在线浏览器；切换语言后客户端会自动重启并保存选择。

当前 Windows 可执行文件没有数字签名，因此 Windows 可能显示未知发布者提示。Android APK 使用项目测试证书，而不是 Google Play 或正式商店证书。两种平台均应先使用旁置 SHA-256 文件核对下载内容。

开始界面显示 `https://github.com/KRhad/TPT-ZH-OmniPack`。游戏会从该公开仓库自动检查更新，下载内容通过大小和 SHA-256 核对后才应用；Windows 更新替换便携 EXE，Android 更新仍需通过系统安装确认。

## 源码与许可证

项目整体依据 GNU GPL v3 发布。`LICENSES` 目录内包含字体、静态库及其他第三方组件的许可证和通知，源码来源记录见 `SOURCE-AND-LICENSES.zh-CN.md`。

本项目以 The Powder Toy 为底座，并明确致谢 Cracker1000 TPT、SpikeViper Biology、TPT Ultimata Mod、Jacob1 Mod、TPT-Alchemy、Seppo's Metallurgy Mod SRC、Cyens Toy / Cyens Toy Source、TPT Biological Mod 与 nucular's Mod。它们分别用于玩法边界、材料概念、参数或界面设计参考；项目没有整库合并这些模组，也没有逐行复制第三方元素更新函数。精确仓库、commit、文件级来源、许可证和采用/拒绝结论随源码包提供。

公开下载只包含玩家程序、可选符号包、完整可构建源码和必要文档，不包含测试样本、测试报告、偏好文件、截图、日志或个人存档。
