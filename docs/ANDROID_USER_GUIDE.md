# TPT-ZH-OmniPack 1.0.0 Android 版

这是与 Windows 版共用模拟核心、材料稳定 ID、中文字体和 OPS 存档格式的 ARM64 直接移植版，不是删减重制版。

## 安装

- 支持 Android 5.0（API 21）或更高版本的 `arm64-v8a` 设备。
- 下载 APK 与同名 `.sha256` 文件，核对 SHA-256 后安装 APK。
- APK 使用项目测试证书签名，不是应用商店签名；Android 可能要求允许从当前文件管理器安装应用。
- 应用 ID 为 `org.tptzh.omnipack`，启动器名称为“万象沙盘”。

## 触控与存档

- 横屏运行；短按材料进行选择，在模拟区拖动即可绘制。
- 周期表中的元素格先打开材料面板，再点具体材料进行选择。
- 短按左下角“打开”图标浏览本地 `.cps` 存档；长按同一图标打开在线存档浏览器。
- 未登录时点击保存按钮会保存到本地。重要存档位于应用专属目录，卸载前请先备份。
- 设置中切换语言后应用会自动重启，并保留所选语言。

## GitHub 与自动更新

- 开始界面显示项目地址：`https://github.com/KRhad/TPT-ZH-OmniPack`。
- 启用“启动时获取消息/检查更新”后，游戏会读取公开 `public-source` 分支的 `Startup.json`。
- 发现更高版本时，游戏先下载 APK，并核对清单声明的字节数和 SHA-256；校验成功后才调用 Android `PackageInstaller`。
- Android 8 及以上版本可能先要求允许本应用“安装未知应用”，随后仍会显示系统安装确认页。游戏不能也不会绕过这些系统步骤。
- 如果取消授权或安装，当前版本会继续运行，也可从上述 GitHub 地址手动下载。

## 已知边界

- 本包仅提供 `arm64-v8a`；未提供 32 位 ARM、x86 APK 或 Google Play AAB。
- Windows 和 Android 使用相同 OPS 材料映射，但跨设备复制文件仍建议保留原始备份。
- 本次公开的是 1.0.0 未签名发布门禁下的 Android 构建快照；未创建正式 GitHub Release，`release_ready=false`。

---

This ARM64 Android build directly ports the same simulation core, stable material IDs, Chinese font, and OPS save format used by the Windows build. Tap the lower-left open icon for local saves and long-press it for the online browser. The APK is signed with the project's test certificate, targets Android 13 behavior, supports Android 5.0 and newer, and is not an app-store release. `release_ready=false`.
