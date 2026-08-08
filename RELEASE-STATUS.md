# 1.0.0 公开构建快照状态

这里的 `1.0.0` 是产品版本号。当前内容作为 Windows x64 与 Android ARM64 的公开构建快照发布在 `public-source` 分支，不等同于已经完成全部发布门禁的 `v1.0.0` GitHub Release。

公开源码由 `4d77fe58b2327721db4ada2a8389a06b743bd783` 对应的 Source ZIP 全新同步而来；公开 Git 历史从单独的无测试根提交开始，不包含开发分支或私有测试历史。该无测试源码包已在独立目录使用 `build_tests=false` 完成全新构建（`764/764`）。开发树同一源码提交的 Windows clean Release 构建为 `790/790`，完整 Meson 测试为 `42/42`，Python 检查为 `253 passed, 2 skipped`，Android ARM64 clean signed build 为 `767/767`。

Windows 与 Android 开始界面显示 `https://github.com/KRhad/TPT-ZH-OmniPack`。静态更新通道使用内部 build `1`：玩家可见版本与 Android `versionName` 仍为 `1.0.0`，Android `versionCode=1000001`。Windows `.update` 和 Android APK 均校验精确大小及 SHA-256；Android APK 保持旧公开 build `0` 的同一测试证书，并通过 v1/v2/v3 签名、16 KB ZIP/ELF 对齐检查。代码与最终本地资产审计 `47/47` 通过。

当前 build `1` 的远端下载、Windows 原地替换、Android 系统安装确认，以及中文/英文和 GUI/DPI 人工视觉矩阵仍未完成，因此这些项目继续标记为 `not_tested` 或 `false`。签名、编译、进程存活和自动化探针均不单独作为 GUI 通过证据。

未完成的正式发布门禁：

- Windows 可执行文件未做 Authenticode 签名；
- Android APK 使用项目测试证书，不是商店或正式发布证书；
- 物理 Android ARM64 设备验证未完成；
- 当前 build `1` 的中文/英文、常见窗口缩放与 100%/125%/150% DPI 完整人工视觉矩阵未完成；
- 独立两小时综合长跑未完成；
- 未创建 `v1.0.0` tag；
- 未创建 GitHub Release。

```text
source_revision=4d77fe58b2327721db4ada2a8389a06b743bd783
update_build=1
android_version_code=1000001
public_tests_included=false
private_test_evidence_preserved=true
public_source_buildable=true
public_source_build_targets=764/764
public_source_commit_available=true
development_windows_release_build=790/790
development_meson_tests=42/42
development_python_checks=253_passed_2_skipped
android_arm64_clean_signed_build=767/767
android_signature_v1_v2_v3=true
android_upgrade_certificate_preserved=true
github_update_asset_audit=47/47
windows_update_runtime=not_tested
android_update_runtime=not_tested
current_build_zh_en_runtime=not_tested
gui_visual_pass=not_tested
github_release_created=false
release_tag_created=false
authenticode_signed=false
android_production_signed=false
android_physical_device_pass=not_tested
gui_dpi_matrix_complete=false
long_run_complete=false
release_ready=false
```
