# Android 直接移植版

Android 版与 Windows 版使用同一套模拟、元素登记、反应、中文字体和 OPS 存档代码。它不是删减重制版，也不包含任务、成就或元素解锁。

## 当前目标

- 架构：`arm64-v8a`
- 最低系统：Android 5.0（API 21）
- 目标系统行为：Android 13（target SDK 33）
- 显示：横屏全屏，SDL 逻辑分辨率等比缩放
- 输入：Android 触控事件由 SDL 映射到现有触控界面
- 本地存档：触屏短按左下角打开按钮；长按同一按钮进入在线存档浏览器
- 数据目录：应用专属外部目录，不申请旧式全盘存储权限
- 中文启动器名称：`万象沙盘`
- 应用 ID：`org.tptzh.omnipack`
- 更新：从公开 GitHub `Startup.json` 按 `ANDROIDARM64` 选择 APK，校验大小与 SHA-256 后通过系统 `PackageInstaller` 请求用户确认安装

## 构建

本机需要 Android SDK、NDK r29、Build Tools 35.0.0、Android Platform 31、JDK 8、JDK 17 或更高版本，以及 MSYS2 UCRT64 的 Meson/Ninja/Python。

```powershell
$env:ANDROID_SDK_ROOT = 'D:\CodexWork\AndroidToolchain\sdk'
$env:JAVA8_HOME = '<JDK 8 根目录>'
$env:JAVA_HOME = '<JDK 17 或更高版本根目录>'
.\tools\build_android.ps1 -RequireCleanSource
```

不提供密钥时生成对齐后的未签名 APK。若要生成可安装测试包，应在仓库外准备密钥，并通过进程环境传入密码：

```powershell
$env:ANDROID_KEYSTORE_PASS = '<在本机设置，不写入仓库>'
.\tools\build_android.ps1 -Keystore 'D:\安全位置\omnipack-test.jks'
```

构建图保留本地未剥离 ARM64 库供诊断，但 APK 只封装移除调试段和非必要符号后的副本。构建脚本固定使用 `build_tests=false`，检查 APK Manifest、16 KB ZIP 对齐和签名，并在 `artifacts/android/1.0.0/` 输出：

- `TPT-ZH-OmniPack-1.0.0-Android-arm64-v8a.apk`；
- 同名 `.sha256`；
- `ANDROID-MANIFEST.json`，记录源码 revision、包元数据、ABI、大小、哈希和证书指纹；
- `README-Android.md` 用户说明。

`-RequireCleanSource` 会拒绝从脏工作树生成最终候选。密钥、密码和私有验证证据均不得放入仓库或 APK 交付目录。

自动更新不会绕过 Android 安全模型。Android 8 及以上版本若未授权本应用安装未知来源软件，会先打开系统授权页；授权后仍由系统显示最终安装确认。APK 在 C++ 层完成大小和 SHA-256 校验后才会交给 Java 安装会话。

## 已完成的真实客户端检查

在独立 MuMu Android 15 实例（`x86_64,arm64-v8a,x86`，ARM64 转译执行）中完成：

- 简体中文与英文切换后自动重启，偏好在新进程中生效；
- 118 元素周期表、跨周期元素、材料详情、长列表滚动、搜索和长按完整说明；
- 中文输入、触控绘制、本地 `.cps` 保存、清空后从存档浏览器重新打开；
- 触屏短按打开本地存档、长按进入在线浏览器；
- `1600x900/240 dpi`、`1280x720/160 dpi`、`1920x1080/320 dpi` 下的周期表和说明窗口边界。

这些是 Android 模拟器中的真实客户端证据，不等于物理 ARM64 手机的性能、温度或生命周期验收。

## 兼容边界

- Windows 与 Android 使用相同稳定元素 ID 和 OPS 粒子格式。
- Android 数据位于应用专属目录；卸载应用前应导出需要保留的存档。
- 当前只产出 ARM64 包；32 位 ARM、x86 模拟器和 Google Play AAB 尚未作为本批门禁。
- 物理 ARM64 手机上的安装升级、后台/前台恢复、温度、耗电、持续帧率和长跑仍为 `not_tested`。
- APK 使用仓库外保留的项目测试证书，不是 Google Play 或正式商店签名。
