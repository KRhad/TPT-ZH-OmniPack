# TPT-ZH-OmniPack 0.1.0-test 发布加固报告

构建对象：Windows x64 公共测试候选。此报告只把实际获得的证据标记为通过；未执行的 GUI 交互、OPS 和压力验证不会由编译或静态扫描代替。

## 机器可读结论

```text
source_commit=0e9ff54c65c64f01e3af567366d2eed1de2c5dd9
release_tag=not_tested
version=0.1.0-test
clean_build_pass=true
meson_tests=10/10
python_tests=50
lua_runtime_tests=6
gui_launch_test=true
zh_en_switch_test=not_tested
ops_roundtrip_test=not_tested
disabled_module_dialog_test=not_tested
readonly_save_block_test=not_tested
readonly_upload_block_test=not_tested
stress_test=not_tested
font_license_resolved=true
release_exe_stripped=true
debug_symbols_separated=true
developer_paths_removed=true
pe_security_flags_preserved=true
authenticode_signed=false
zip_sha256=8266231A3DE12D504706B3174D01570456DAC3948C3C7AB0187949EAADE05DBD
release_ready=false
```

## 源码与基线

- **源码确认：**构建提交为 `0e9ff54c65c64f01e3af567366d2eed1de2c5dd9`，分支为 `release/test-public-hardening`。内部测试包的功能构建提交是 `f19cf0634e8c024bc5a7711ee1f2c2d652d7d5f6`；其后的 `86609b3b2f5ce99f450b0c818add880744f82281` 仅记录先前内部包交付信息。
- **源码确认：**未修改官方元素 ID，也未重新分配 48 个 OmniPack 元素的稳定 ID；本轮没有新增元素或大型玩法系统。
- **源码确认：**对应源码地址已在 README 声明为 `https://github.com/Dragonrster/The-Powder-Toy-Chinese`，目标分支为 `release/test-public-hardening`。本会话对远端的验证因连接重置失败，不能证明该 commit 已公开可取得。
- **构建确认：**旧内部 EXE 是 `debugoptimized`、`debug=true`、`strip=false`，不适合作为公开 EXE。它的构建大小约为 244 MB，并且不提供可分离符号流程。

## 构建环境和命令

- **构建确认：**操作系统为 Microsoft Windows 11 Pro x64，`10.0.26200`；编译器为 MSYS2 UCRT64 GCC `16.1.0`，链接器 GNU ld.bfd `2.46.1`，Meson `1.11.2`，Ninja `1.13.2`，Python `3.14.5`。
- **构建确认：**构建目录为 `build-release-public-static`，构建类型为 `release` 加 `debug=true`，以便先分离符号；预编译静态依赖为 `tpt-libs v20251019131007`。
- **构建确认：**正式 Meson 参数同时使用 `-Dstatic=prebuilt` 和 `-static -static-libgcc -static-libstdc++`。第一次仅使用 `-Dstatic=prebuilt` 的 Release 输出导入 `libgcc_s_seh-1.dll`、`libstdc++-6.dll` 和 `libwinpthread-1.dll`，在本机之外无法保证启动；已定位并修复为上述完整静态参数，且 `release_binary_audit.py` 现在拒绝这些导入。
- **构建确认：**最终 clean build 完成 500 个目标，编译错误为 0。GCC 16 仍在现有 `Bson.cpp`、`PowderToy.cpp` 和 `Editing.cpp` 路径报告警告；本轮未把它们隐藏或改写为无关的发布修复。

```powershell
$env:PATH = 'C:\msys64\ucrt64\bin;' + $env:PATH
meson setup build-release-public-static . --buildtype=release -Ddebug=true -Dstatic=prebuilt -Dapp_exe=tpt-zh-omnipack '-Drelease_label=0.1.0-test' -Dresolve_vcs_tag=no -Dmanifest_date=2026-07-30 "-Dc_args=['-ffunction-sections','-fdata-sections']" "-Dcpp_args=['-ffunction-sections','-fdata-sections']" "-Dc_link_args=['-Wl,--gc-sections','-static','-static-libgcc','-static-libstdc++']" "-Dcpp_link_args=['-Wl,--gc-sections','-static','-static-libgcc','-static-libstdc++']"
meson compile -C build-release-public-static
```

## 二进制与版本

- **构建确认：**原始带符号 EXE 为 `253,431,235` 字节，SHA-256 `DDC26C439D17D8052CE68DF536AFBF8A0F43E42D5B63E0989A4CFD56EE559ADF`。
- **构建确认：**发布 EXE 为 `17,663,659` 字节，SHA-256 `5DB08B15A18AF1F5A59FE8F16B1245057D13CC4F240489AAC193EE5ADA2A205E`；相对带符号输入减少 `93.03%`。
- **构建确认：**独立符号文件为 `241,963,459` 字节，SHA-256 `DCAEBC30D72D3A2395240290F4686C23435E37AEAD389EF07483BA5579F42669`。执行顺序为 `objcopy --only-keep-debug`，再对复制出的发布 EXE 执行 `strip --strip-debug`。
- **自动测试确认：**`objdump -h` 未在发布 EXE 中发现 `.debug*` 段；`strings -a` 对 `C:\Users\KR`、`C:/Users/KR`、`/Users/KR` 的匹配数为 0；审计也拒绝构建路径标记和动态 MSYS2 runtime 导入。
- **自动测试确认：**PE `DllCharacteristics=0x160`，含 `HIGH_ENTROPY_VA`、`DYNAMIC_BASE` 和 `NX_COMPAT`。
- **构建确认：**Windows 资源为 `FileDescription=TPT-ZH-OmniPack`、`FileVersion=TPT-ZH-OmniPack 0.1.0-test (The Powder Toy 100.0.399)`、`ProductVersion=0.1.0-test (upstream 100.0.399)`、`OriginalFilename=tpt-zh-omnipack.exe`、`InternalName=org.tptzh.omnipack`。上游存档/协议版本逻辑未修改。
- **构建确认：**`Get-AuthenticodeSignature` 返回 `NotSigned`，因此 `authenticode_signed=false`。测试说明披露 Windows SmartScreen 可能提示和 SHA-256 核对方式。

## 字体、法律与文档

- **源码确认：**原 Dragonrster 扩展字库无名称、作者、来源和许可证记录，已不再用作发布输入。
- **自动测试确认：**`tools/build_release_font.py` 从固定的官方 TPT `100.0.399` 字库和 GNU Unifont `16.0.03` 输入构建 `resources/font.bz2`。它为全部嵌入语言目录的 `2,593` 个字符解析字形，补充 `1,839` 个 GNU Unifont 字形；输出哈希为 `938F989E2375B97B82F40104CA53D2367CF211265B545A6F061D532A5BDFE042`。
- **源码确认：**GNU Unifont 输入哈希为 `23AB31CA87C6614B97928A39DC0C15BC2AAAA5B6F130ADF9E1DF481250F73BAAF`，其 SIL OFL 1.1 / GPL 字体例外文本哈希为 `1E74CB82BF476843E97C2596297B04219B1A7E51F7238944A8C031CB9401FA87`，并已随普通 ZIP 提供。
- **源码确认：**README、CHANGELOG、TESTING、第三方来源、AI 披露、已知问题、测试矩阵、字体审计和发布加固文档已统一使用“工业冶金、局部生态、高级化学、受控核工业”，并明确 48 个元素已经实现；自动化、灾害、炼金进度和旧模组全面迁移仍未完成。
- **尚未验证：**最终 ZIP 的高 DPI、多缩放、窄菜单、长图鉴和字体视觉质量需要人工 GUI 检查。字体来源和许可证已解决，但视觉验证不能由字形覆盖扫描代替。

## 自动测试与运行回归

- **自动测试确认：**Meson 静态测试 `10/10` 通过。
- **自动测试确认：**Python 工具单元测试 `50` 通过；其中 2 项仅因 `py -3.14` 进程的 PATH 未提供 C++ 编译器而跳过 C++ 语法校验，Windows Release 编译本身已由 GCC 16 完成。
- **实际 GUI 运行确认：**对解压后的最终 ZIP 运行 8 秒。进程保持 `Responding=True`，窗口句柄非零，标题为 `TPT-ZH-OmniPack 0.1.0-test`，工作集为 `127,381,504` 字节。测试在同一 Windows 主机的隔离用户配置目录执行，不是无开发工具的虚拟机。
- **实际 GUI 运行确认：**首次运行创建隔离的 `AppData\Roaming\TPT-ZH-OmniPack`；隔离路径中不存在官方 `The Powder Toy` 数据目录，ZIP 中也不存在 `powder.pref`。
- **实际 GUI 运行确认：**程序由测试进程强制关闭后退出；这仅证明启动、窗口和基本响应，不能证明手动菜单或存档交互。
- **实际运行确认：**Lua 客户端回归 `6/6` 通过：模块选择、冶金、化学、局部生态完整模式、局部生态简化模式和受控核工业。冶金验证 7 条配方/5 种行为/ID `256..278`；化学 9 条路径/ID `360..369`；生态 6 条路径/ID `288..295`；核工业 4 条路径/ID `328..334`。

## 交付物与审计

| 文件 | 字节数 | SHA-256 | 结论 |
|---|---:|---|---|
| `TPT-ZH-OmniPack-0.1.0-test-Windows-x64.zip` | 5,881,521 | `8266231A3DE12D504706B3174D01570456DAC3948C3C7AB0187949EAADE05DBD` | 自动打包、解压后审计通过 |
| `TPT-ZH-OmniPack-0.1.0-test-Symbols-Windows-x64.zip` | 69,389,193 | `296794B51D11ACD73198CE96EE300C61320F8E282B9307F2EF03AF4A2D31CF1B` | 仅含匹配的 `.debug` 与清单，审计通过 |

- **自动测试确认：**`package_test_release.py` 以白名单创建两个固定时间戳 ZIP，并写入 `TEST-MANIFEST.txt`：版本、完整 Git commit、构建 epoch、每个成员大小和 SHA-256。
- **自动测试确认：**`test_release_audit.py` 在生成后重新读取 ZIP，检查成员白名单、重复成员、禁用个人/脚本/对象/调试文件、内嵌清单、EXE MZ 头、开发路径、ZIP SHA-256 和字体许可证。普通包不含独立调试符号、用户数据、`powder.pref`、存档、图章或 Lua 脚本。
- **构建确认：**普通 ZIP 的 `.sha256` 与上表匹配；符号 ZIP 的 `.sha256` 与上表匹配。

## 发布阻塞与最小下一步

`release_ready=false`。以下项目是阻止公开下载的事实性原因：

1. **对应源码公开可得性未验证。** 远端 `git ls-remote` 在本会话中连接重置，无法证明 `0e9ff54c65c64f01e3af567366d2eed1de2c5dd9` 已推送。最小动作：将该分支或带该 commit 的公开 tag 推送到 README 指定仓库，并从独立网络重新 `git ls-remote` 和克隆验证。
2. **简体中文/英文真实点击切换未执行。** 没有可信 UI 控制接口可生成点击和截图证据。最小动作：在解压后的最终 ZIP 手动切换中文和英文、重启确认、检查菜单/图鉴/兼容对话框，并保存要求的截图。
3. **四模块 UI、代表元素和禁用模块三选项未执行。** 静态与 Lua 检查通过，但没有最终 ZIP 中的点击证据。最小动作：依次测试四个开关、`ALUM/NUTR/NFUL/CHLR` 搜索放置，以及正常加载/只读加载/取消。
4. **OPS 往返、只读保存/上传与载体字段未执行。** 保存门禁已有静态审计和单测，但无最终 ZIP 的保存、重启、加载和菜单/快捷键/UI 上传流程证据。最小动作：按 `docs/SAVE_COMPATIBILITY.md` 建立四模块和 `LAVA/SPRK/MSCR` 载体样本，记录哈希、粒子统计和结果。
5. **持续压力/性能测试未执行。** 没有所需的固定样本、FPS、内存、运行时长和无限增长证据。最小动作：按 `docs/TEST_MATRIX.md` 的十类样本在不改功能的情况下记录数据。

未签名是已披露限制，不是本轮唯一阻塞项；字体授权、调试信息分离、路径清理、PE 缓解、包内容和最终 ZIP 启动均已取得相应证据。
