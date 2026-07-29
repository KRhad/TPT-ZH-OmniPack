# Windows x64 基线构建报告

## 审计对象

- 仓库：`Dragonrster/The-Powder-Toy-Chinese`
- 分支：`i18n-new`
- 固定提交：`445fab51dcf66057e645371aa9c9a556425b3d2f`
- 源码版本：TPT `100.0.398`
- 构建副本：`C:\Users\KR\tpt-omnipack-sources\audit_primary\dragonrster-i18n-new`
- 构建前后 tracked 工作树：干净

基线构建不含 Phase 0 文档提交，也没有任何生产代码修改。

## 环境

| 项目 | 版本 |
|---|---|
| OS | Microsoft Windows 11 专业版 x64，`10.0.26200` |
| C++ 编译器 | MSYS2 UCRT64 GCC `16.1.0` |
| 链接器 | GNU ld.bfd `2.46.1` |
| Meson | `1.11.2` |
| Ninja | `1.13.2` |
| Python | MSYS2 UCRT64 Python 3 |
| SDL2 | `2.30.9-tpt-libs` |
| LuaJIT | `2.1.0-git-tpt-libs` |
| libcurl | `8.10.1`，官方 tpt-libs 组合 |
| FFTW | `3.3.8-tpt-libs` |
| libpng | `1.6.49-tpt-libs` |
| bzip2 | `1.0.8-tpt-libs` |
| JsonCpp | `1.9.5-tpt-libs` |
| 预编译依赖包 | `tpt-libs v20251019131007` |

## 可复核命令

PowerShell 进程内 PATH：

```powershell
$env:Path = 'C:\msys64\ucrt64\bin;C:\msys64\usr\bin;' + $env:Path
```

Meson 设置：

```powershell
meson setup build-baseline `
  -Dbuildtype=debugoptimized `
  -Dstatic=prebuilt `
  -Dstrip=false `
  -Dlto=false `
  "-Dc_args=['-ffunction-sections','-fdata-sections']" `
  "-Dcpp_args=['-ffunction-sections','-fdata-sections']" `
  "-Dc_link_args=['-Wl,--gc-sections','-static','-static-libgcc','-static-libstdc++']" `
  "-Dcpp_link_args=['-Wl,--gc-sections','-static','-static-libgcc','-static-libstdc++']" `
  -Dmanifest_date=2026-07-29
```

构建和测试：

```powershell
meson compile -C build-baseline -v
meson test -C build-baseline --print-errorlogs
```

`-Dstatic=prebuilt` 本身不会静态链接 GCC runtime；必须保留 `.github/build.sh` 使用的 `-static -static-libgcc -static-libstdc++`，否则产物依赖开发机的 `libgcc_s_seh-1.dll`、`libstdc++-6.dll` 和 `libwinpthread-1.dll`。

## 构建结果

- Meson 配置：PASS
- Ninja：445/445，PASS
- 编译错误：0
- 编译警告：2
- Meson test：退出码 0，但项目注册测试数为 **0**
- `powder.exe` 大小：231,598,426 字节
- SHA-256：`A15B5D25C5552B9954040F94001C96B4289072D88B9820DCEC3FA5EDDFA69AC7`
- PE：PE32+、Windows GUI、x86-64
- 非系统 GCC runtime DLL 导入：0

两个警告均是 GCC 16 在 `PowderToy.cpp:429` 经 `std::optional<ByteString>` 内联路径给出的 `-Wmaybe-uninitialized`。本次没有把警告当错误，也没有声称它是误报；后续 Debug/Release 构建分别追踪。

## 启动测试

在隔离目录 `C:\Users\KR\tpt-omnipack-runtime\baseline-445fab51` 启动产物并等待 6 秒：

- 进程仍在运行：PASS
- Windows `Responding`：`True`
- 主窗口标题：`The Powder Toy i18n`
- 主窗口句柄：非零
- 隔离目录新增账户/偏好/存档文件：0
- 测试后只终止本次精确进程：PASS

Windows Computer Use 在本会话没有暴露受信任 `node_repl`/native pipe，因此没有绕过该安全层改用 PowerShell UI 自动化。故本报告不声称完成视觉中文、英文切换、菜单点击或存档操作。

源码证据明确显示：

- 初次启动默认语言索引为 `0`（英文）：FAIL
- Options UI 没有语言下拉框：FAIL
- `BASE` 说明硬编码中文，英文模式不完整：FAIL

这些失败在 `integration/zh-omnipack` 的下一独立提交中修复并重新构建。

## 官方 100.0.399 合并验证

合并固定官方 commit `bff38ce6959e1c1a7a4d17d0d5d44d127a0dfcbd` 后，在提交前执行同等静态配置：

- 项目版本：100.0.399
- Ninja：445/445，PASS
- 编译错误：0
- 编译警告：2，同一 `PowderToy.cpp` 路径
- Meson test：0 项，退出码 0
- 产物：`build-upstream-100\powder.exe`
- 大小：225,398,731 字节
- SHA-256：`433F7815624E77F2BF114FC6A923F2D61BC30AD681DF7445E3537C73A1898D44`
- GCC runtime DLL 导入：0

首次依赖下载返回了与 wrap 文件不符的哈希，Meson 正确拒绝继续。重试使用此前已经通过官方 SHA-256 校验的同版本本地 archive 后成功；没有关闭哈希检查或接受不匹配内容。

