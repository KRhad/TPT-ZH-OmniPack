# TPT-ZH-OmniPack 0.7.0-dev 私测包证据

本文件记录本地私测包的构建、封装和环境压力证据。它不是 1.0.0 发布报告；人工 GUI、DPI、两小时长跑、完整许可证总审计、公开源码、签名、tag 和公开 Release 均不由本文件替代。

## 来源与构建

- 分支：`development/content-expansion-1.0`
- 包内清单 revision：`ee75bc2773bae67958a8b07eba6e896569045cea`
- 构建目录：`build-0.7.0-dev-private-static-clean`
- 配置：`release`、`debug=true`、`strip=false`、`lto=false`、`static=prebuilt`、`can_install=no`，并使用 `-static -static-libgcc -static-libstdc++` 与节区回收参数。
- 全新构建：`771/771`
- Meson static：`31/31`
- Python：`203/203`
- 未带静态编译器运行库的开发 EXE 曾被发布二进制审计拒绝；没有降低门禁，而是重新建立上述静态构建。

| 文件 | 字节 | SHA-256 |
|---|---:|---|
| 静态原始 EXE | 337,395,645 | `46172C2AD4DC91780E8930704B3AFCD71A16C255244A915F857D3A02EF326134` |
| 剥离私测 EXE | 19,252,827 | `429C3FC72E415931BD9D6DCDF07602D7F590864EC76275524214651CEDD07CD1` |
| 分离调试符号 | 324,526,013 | `B59BE41EE5180AB24F75FE986BC6461468789EA31E11C2DC5FF7E2A3FF48C3F7` |

剥离 EXE 不含 `.debug` 节，不导入 `libgcc_s_seh-1.dll`、`libstdc++-6.dll` 或 `libwinpthread-1.dll`；`DYNAMIC_BASE`、`NX_COMPAT` 和 `HIGH_ENTROPY_VA` 保持启用。开发路径字符串扫描通过。

## ZIP

| 包 | 字节 | SHA-256 |
|---|---:|---|
| `TPT-ZH-OmniPack-0.7.0-dev-Windows-x64.zip` | 6,107,003 | `C375BA1BD85F5C6838282FDF8979DFBC010B02FF0E0D0216CD08D342B70CE05F` |
| `TPT-ZH-OmniPack-0.7.0-dev-Symbols-Windows-x64.zip` | 88,879,329 | `FA46279490D9A3EDF0426130588C4B5CEC46BECC0070D6AFCB59CEEDAA2B4532` |

普通 ZIP 与符号 ZIP 均通过精确成员、重复成员、禁止后缀、manifest 字段、成员大小/哈希、个人数据路径和随包许可证文件审计。解压后的 EXE 与 manifest 中的 19,252,827 字节及 SHA-256 完全一致。既有 `dist/0.6.0-dev-private` 未覆盖。

## S14 正式压力

运行目录：`artifacts/performance/0.7.0-dev/DESKTOP-14BQH2Q-276049E7945C/S14-ENVIRONMENT-DENSE/20260802T000456Z-a012fd04`

| 指标 | 结果 |
|---|---:|
| 预热 | 60.001206 秒 |
| 采样 | 600.000936 秒 |
| 平均 FPS | 60.001573 |
| 1% low FPS | 55.356465 |
| 最低 FPS | 52.095390 |
| 初始 / 峰值 / 最终粒子 | 38,184 / 38,184 / 908 |
| 成功事件总数 / 单帧总峰值 | 37,669 / 1,026 |
| 峰值工作集 / 私有字节 | 157,233,152 / 142,528,512 |
| 停止后事件增量 | 0 |
| 恢复断言 | 14 |

`result.json` SHA-256 为 `CE6555727854C595671874CB9F4525217899FBA04A9FD2B0200CC696308062CC`；独立 `assessment.json` SHA-256 为 `192C59787C8378B9F329B4F52B080AAC7B6460ECD689E775198743DFFBDD01D8`。运行未崩溃、未挂起，OPS 往返、场景停止和恢复均通过。有限观测分类为 `unbounded_growth=false`、`memory_leak_suspected=false`，最终 `performance_gate_pass=true`；这不是两小时长期有界性的证明。

单帧总事件峰值包含压力场景中全部启用模块的成功事件计数；环境/生态自己的共享预算仍由专项 1,100 样本回归确认不超过 `1024/frame`。

## 机器可读状态

```text
private_test_version=0.7.0-dev
private_test_ready=true
package_manifest_revision=ee75bc2773bae67958a8b07eba6e896569045cea
clean_static_build_pass=true
static_tests_pass=true
python_tests_pass=true
release_binary_audit_pass=true
release_exe_stripped=true
developer_paths_removed=true
pe_security_flags_preserved=true
zip_audit_pass=true
extracted_exe_hash_match=true
old_0_6_private_package_preserved=true
environment_s14_formal_600s=true
environment_s14_performance_gate=true
periodic_table_gui_visual_test=not_tested
official_style_gas_gui_visual_test=not_tested
dpi_visual_test=not_tested
long_run_7200s=not_tested
third_party_license_audit=false
source_public=false
release_tag=not_tested
release_ready=false
```
