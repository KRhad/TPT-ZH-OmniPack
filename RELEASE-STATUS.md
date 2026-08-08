# 1.0.0 公开构建快照状态

这里的 `1.0.0` 是产品版本号。当前内容作为 Windows x64 与 Android ARM64 的公开构建快照发布在 `public-source` 分支，不等同于已经完成全部发布门禁的 `v1.0.0` GitHub Release。

公开源码由 `f6ac7ce6501773fbea41f5229b855ea2feb01717` 对应的 Source ZIP 全新同步而来；公开 Git 历史从单独的无测试根提交开始，不包含开发分支或私有测试历史。该源码树已在仓库外使用 `build_tests=false`、Release、静态依赖完成一次全新 Windows 构建（`764/764`）。开发树同一源码提交的完整 Meson 测试为 `42/42`，Android ARM64 clean build 为 `767/767`。

Windows 与 Android 开始界面显示 `https://github.com/KRhad/TPT-ZH-OmniPack`。静态更新通道对 Windows `.update` 和 Android APK 同时校验精确大小及 SHA-256，并为 GitHub 清单和资产下载使用 HTTP/1.1；资产下载最多尝试两次且每次重新建立请求。代码与本地资产审计 `45/45` 通过。远端下载、Windows 原地替换及 Android 系统安装确认必须在本次文件推送后才能完成端到端验证，因此当前仍明确标记为 `not_tested`。

未完成的正式发布门禁：

- Windows 可执行文件未做 Authenticode 签名；
- Android APK 使用项目测试证书，不是商店或正式发布证书；
- 物理 Android ARM64 设备验证未完成；
- 中文/英文、常见窗口缩放与 100%/125%/150% DPI 的完整人工视觉矩阵未完成；
- 独立两小时综合长跑未完成；
- 未创建 `v1.0.0` tag；
- 未创建 GitHub Release。

```text
source_revision=f6ac7ce6501773fbea41f5229b855ea2feb01717
public_tests_included=false
private_test_evidence_preserved=true
public_source_buildable=true
public_source_build_targets=764/764
public_source_commit_available=true
development_meson_tests=42/42
android_arm64_clean_build=767/767
github_update_asset_audit=45/45
windows_update_runtime=not_tested
android_update_runtime=not_tested
github_release_created=false
release_tag_created=false
authenticode_signed=false
android_production_signed=false
android_physical_device_pass=not_tested
gui_dpi_matrix_complete=false
long_run_complete=false
release_ready=false
```
