# Android 直接移植版

Android 版与 Windows 版使用同一套模拟、元素登记、反应、中文字体和 OPS 存档代码。它不是删减重制版，也不包含任务、成就或元素解锁。

## 当前目标

- 架构：`arm64-v8a`
- 最低系统：Android 5.0（API 21）
- 目标系统行为：Android 13（target SDK 33）
- 显示：横屏全屏，SDL 逻辑分辨率等比缩放
- 输入：Android 触控事件由 SDL 映射到现有触控界面
- 数据目录：应用专属外部目录，不申请旧式全盘存储权限
- 中文启动器名称：`万象沙盘`
- 应用 ID：`org.tptzh.omnipack`

## 构建

本机需要 Android SDK、NDK r29、Build Tools 35.0.0、Android Platform 31、JDK 8、JDK 17 或更高版本，以及 MSYS2 UCRT64 的 Meson/Ninja/Python。

```powershell
$env:ANDROID_SDK_ROOT = 'D:\CodexWork\AndroidToolchain\sdk'
$env:JAVA8_HOME = '<JDK 8 根目录>'
$env:JAVA_HOME = '<JDK 17 或更高版本根目录>'
.\tools\build_android.ps1
```

不提供密钥时生成对齐后的未签名 APK。若要生成可安装测试包，应在仓库外准备密钥，并通过进程环境传入密码：

```powershell
$env:ANDROID_KEYSTORE_PASS = '<在本机设置，不写入仓库>'
.\tools\build_android.ps1 -Keystore 'D:\安全位置\omnipack-test.jks'
```

构建图保留本地未剥离 ARM64 库供诊断，但 APK 只封装移除调试段和非必要符号后的副本。构建脚本会检查 APK Manifest、16 KB ZIP 对齐和签名，并在 `artifacts/android/` 下输出 SHA-256。

## 兼容边界

- Windows 与 Android 使用相同稳定元素 ID 和 OPS 粒子格式。
- Android 数据位于应用专属目录；卸载应用前应导出需要保留的存档。
- 当前只产出 ARM64 包；32 位 ARM、x86 模拟器和 Google Play AAB 尚未作为本批门禁。
- 没有连接真机时，编译和 APK 审计通过不能替代触控、输入法、生命周期和实际帧率测试。
