# 公共测试版发布加固

版本：`0.1.0-test`。此文档记录当前发布候选的可复核结论；不把构建或字符串扫描冒充为 GUI、存档或压力验证。

## 基线

- **源码确认：**内部测试包构建提交为 `f19cf0634e8c024bc5a7711ee1f2c2d652d7d5f6`；加固从其后的文档提交 `86609b3b2f5ce99f450b0c818add880744f82281` 分支 `release/test-public-hardening` 开始。
- **构建确认：**旧基线是 `debugoptimized`、`debug=true`、`strip=false` 的 Windows x64 静态构建，不能作为公开二进制。
- **自动测试确认：**基线 Meson 静态测试 `10/10`、Python 工具测试 `50/50`、Lua 回归 `6/6` 已记录；本加固提交会重新运行相应测试。
- **实际 GUI 运行确认：**内部包仅有进程级启动和 Lua 回归记录；公共候选的 GUI 验证尚未完成。

## 正式构建

正式命令不使用旧 `build.bat`：

```powershell
$env:PATH = 'C:\\msys64\\ucrt64\\bin;' + $env:PATH
meson setup build-release-public-static . --buildtype=release -Ddebug=true -Dstatic=prebuilt -Dapp_exe=tpt-zh-omnipack '-Drelease_label=0.1.0-test' -Dresolve_vcs_tag=no -Dmanifest_date=2026-07-30 "-Dc_args=['-ffunction-sections','-fdata-sections']" "-Dcpp_args=['-ffunction-sections','-fdata-sections']" "-Dc_link_args=['-Wl,--gc-sections','-static','-static-libgcc','-static-libstdc++']" "-Dcpp_link_args=['-Wl,--gc-sections','-static','-static-libgcc','-static-libstdc++']"
meson compile -C build-release-public-static
py -3.14 tools/prepare_windows_release.py --raw-executable build-release-public-static/tpt-zh-omnipack.exe --executable artifacts/release-hardening/tpt-zh-omnipack.exe --symbols artifacts/release-hardening/tpt-zh-omnipack.debug --objcopy C:\\msys64\\ucrt64\\bin\\objcopy.exe --strip C:\\msys64\\ucrt64\\bin\\strip.exe
py -3.14 tools/release_binary_audit.py --executable artifacts/release-hardening/tpt-zh-omnipack.exe --symbols artifacts/release-hardening/tpt-zh-omnipack.debug --objdump C:\\msys64\\ucrt64\\bin\\objdump.exe --strings C:\\msys64\\ucrt64\\bin\\strings.exe
```

`release + debug=true` produces a RelWithDebInfo-equivalent input. `objcopy --only-keep-debug` runs before `strip --strip-debug`; the normal ZIP never receives the detached symbols. `-Dstatic=prebuilt` alone is insufficient: the documented GCC runtime link flags prevent an otherwise clean-machine failure on `libgcc_s_seh-1.dll`, `libstdc++-6.dll` or `libwinpthread-1.dll`. The audit fails if these imports, user path markers, a `.debug*` PE section, or required ASLR/DEP/high-entropy flags remain absent.

## 产品标识

Windows string metadata, window title and in-game version text show `TPT-ZH-OmniPack 0.1.0-test` and retain `The Powder Toy 100.0.399` as the upstream basis. Protocol and saved-game upstream version constants remain unchanged.

## 发布门禁

`release_ready=false` until the report records actual final-ZIP GUI, language switch, module toggle, OPS read-only, reaction and pressure evidence. Font licensing, detached symbols, path scans, package checksums and static checks are independently gated and must also pass.
