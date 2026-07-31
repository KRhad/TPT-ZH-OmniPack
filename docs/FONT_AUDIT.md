# 字体审计

状态：来源和许可证已验证；旧候选字体功能失败，已拒绝。提交 `ca3cccbe` 的转换器修复试包虽通过容器、转换和引擎离屏渲染验证，但用户从 ZIP 解压运行后的人工 GUI 检查判定中文可读性和字形质量仍不合格，因此该试包也已拒绝。当前实现已改用可审计的原生 12px 简体中文字形并通过自动验证；新的最终 ZIP 人工视觉结论仍为 `not_tested`，不具备发布资格。

## 发布字体

程序嵌入 `resources/font.bz2`，这是 TPT 的 12 像素位图格式，不是系统 TTF/OTF。
发布资源由 `tools/build_release_font.py` 从以下受版本控制的输入确定性生成：

| 输入 | 名称/作者 | 版本与来源 | 许可证 | SHA-256 | 用途 |
|---|---|---|---|---|---|
| `resources/third_party/tpt-upstream-font-100.0.399.bz2` | The Powder Toy upstream bitmap font | 官方提交 `bff38ce6959e1c1a7a4d17d0d5d44d127a0dfcbd` | GPL-3.0（随主项目） | `EA86995CC429146F9869C172DC6DAE63D9B6E8BBFC7AC7D07D0CF30FB07D17F7` | 原有 UI、图标和拉丁字符 |
| `resources/third_party/fusion-pixel-12px-monospaced-zh_hans-v2026.07.20.bdf` | Fusion Pixel Font，TakWolf 及其上游字形作者 | `2026.07.20` / commit `c29615e1f629c6bb27dac3e6dcaa4556e629d09f`，https://github.com/TakWolf/fusion-pixel-font | SIL Open Font License 1.1 | `8E4A12E821EFAD608BCB464D685CE50C70693F85A1E95DEAD9575E6CECAFFFC7` | 只补充官方 TPT 字体缺少的嵌入语言字形；简体中文字形原生 12px，不缩放 |
| `resources/third_party/unifont_all-16.0.03.hex.gz` | GNU Unifont，Roman Czyborra、Paul Hardy 及贡献者 | GNU Unifont 16.0.03，https://ftp.gnu.org/gnu/unifont/unifont-16.0.03.hex.gz | SIL Open Font License 1.1，附 GNU 字体嵌入例外 | `23AB31CA87C6614B97928A39DC0C15BC2AAA5B6F130ADF9E1DF481250F73BAAF` | 仅补充嵌入语言目录所需的缺失 Unicode 字形 |
| `resources/third_party/GNU_UNIFONT_COPYING.txt` | GNU Unifont 发布许可证文本 | https://unifoundry.com/LICENSE.txt | SIL OFL 1.1 / GPL 字体例外 | `1E74CB82BF476843E97C2596297B04219B1A7E51F7238944A8C031CB9401FA87` | 随二进制包分发 |
| `resources/third_party/FUSION_PIXEL_FONT_OFL-1.1.txt` | Fusion Pixel Font 主许可证 | 上游发布资产内 `LICENSE-OFL` | SIL OFL 1.1 | `BC518CF64B8032C07690F33CC270C35C179255A6AC8EFA7C165EBAE7E8F76A63` | 随二进制包分发 |
| `resources/third_party/FUSION_PIXEL_FONT_ARK_PIXEL_OFL-1.1.txt` | Ark Pixel Font 许可证 | Fusion Pixel Font 上游许可证清单 | SIL OFL 1.1 | `3AB41567E68E3988BA1EF16DD2644ECA95CA5648EA12E7D46E6287FC0BBE5AEE` | 随二进制包分发 |
| `resources/third_party/FUSION_PIXEL_FONT_CUBIC_11_OFL-1.1.txt` | Cubic 11 许可证 | Fusion Pixel Font 上游许可证清单 | SIL OFL 1.1 | `2B6E5938E5CFFA0B9E183BD05F8C363E174E7EBED1A0556E2855FD1707FA2188` | 随二进制包分发 |
| `resources/third_party/FUSION_PIXEL_FONT_GALMURI_OFL-1.1.txt` | Galmuri 许可证 | Fusion Pixel Font 上游许可证清单 | SIL OFL 1.1 | `86A3EE9495F942F0243F18C103DA9FACA27ADB88142613EDB8BB852E56C892C1` | 随二进制包分发 |

生成步骤只将所需语言 JSON 中的字符加入上游字体，并保持官方 TPT 字形优先。缺失字符先从 Fusion Pixel Font 的 `zh_hans` BDF 取出；BDF 全局格为 12×12、ascent 10、descent 2，直接按基线坐标映射到 TPT 12 行单色像素，不做缩放、灰度膨胀或插值。只有 Fusion 不覆盖的其他语言字符才使用 GNU Unifont 的确定性 16→12 面积覆盖回退。当前嵌入目录的 1,843 个缺失字符均由 Fusion 覆盖，Unifont 回退数为 0。`tools/build_release_font.py` 会先核对全部输入和许可证的固定 SHA-256，并在简体中文目录存在任何非 Fusion 缺字时失败。

## 2026-07-30 中文字体故障与修复

已拒绝候选 `5828a97fc39129547354956dde84d7b6cfb818c2` 的 `font.bz2` 哈希为 `938F989E2375B97B82F40104CA53D2367CF211265B545A6F061D532A5BDFE042`。根因有两项，均位于 GNU Unifont 到 TPT 2bpp 容器的转换器：

1. 转换器将每四个 2bpp 像素按高位优先写入；`FontReader::NextPixel()` 按低位优先读取。因此每组四个像素的横向顺序被反转，导致中文笔画碎裂和错位。
2. 16x16 到 12 高度的整数点采样跳过源行 3、7、11、15；例如 `U+4E00` 的唯一横笔在第 7 行，转换后成为空白字形。

结构修复试验所用 `resources/font.bz2` 哈希为 `4306249BD82DDEF2EEB15E2A3668AC02B970662768B392C8B45515BB550CBA34`。转换器按 `FontReader` 的低位优先协议打包，并用面积覆盖量化避免旧点采样直接跳行。`tools/validate_tpt_font.py` 验证压缩容器、三字节码点排序、字形边界、替换字形、语言目录覆盖、pack/unpack 往返及固定 Unifont 源码点 `U+4E2D/U+6587/U+7B80/U+4F53/U+5DE5/U+4E1A`。指定字形 PNG 输出至 `artifacts/zh-ui-fix/font/decoded/`。这些结构性质通过不代表缩放后的中文字形达到人工可读性要求。

`font_render_probe` 直接使用 `FontReader`、`Graphics::TextSize` 和 `Graphics::BlendText` 渲染中文、混合符号、化学及炼金进度文本；当前 0.4 候选逐条测量和绘制 `zh-CN.json` 的 1,307 条文本（替换字形为 0），输出保存在忽略的 `artifacts/font-render/0.4.0-dev-*` 证据目录。该离屏验证通过不等于人工 GUI 可读性验证。

私有试包绑定提交 `ca3cccbee13a41c37ee0b7975c4b5f060cb34a95`，ZIP SHA-256 为 `E52E746BF925B2096ED93D659E54A52876179230D29D9534D4E579EB755E4081`。用户从 ZIP 解压运行后确认其中文显示仍不如既有出版中文 EXE（SHA-256 `3B96CFEC060705A48681645074AE3C5F53E9A2D40AE56FF76B0E906C2FDBD406`）。该人工结论使中文可读性/字形质量门禁失败；试包仅保留作失败对照，不得称为可发布候选。该结论没有发现或推断进程崩溃。

当前原生 12px 方案生成 `resources/font.bz2` SHA-256 `91AA3E913051E73B1CE412D2E84487BD459AB78BE606D062716608715479B7FA`，包含 14,633 个字形，其中官方 TPT 基线 12,790 个、Fusion 补充 1,843 个、Unifont 回退 0 个。固定测试逐行比较 `U+4E2D/U+6587/U+7B80/U+4F53/U+5DE5/U+4E1A` 的 BDF 单色矩阵与 TPT 解包矩阵，并验证所有 2,597 个嵌入语言字符、替换字形、容器边界、码点排序、宽度和 pack/unpack。此次重建覆盖模组元素自识别说明及炼金进度界面的新增文案；两次分离生成得到相同哈希。自动结果不代替下一私有 ZIP 的人工桌面可读性判断。

该方案的私有人工试包绑定提交 `c743db2fcc49c01033e68023cceff897ed4c35f6`，普通 ZIP SHA-256 `943DA2A60C0B371A1D3F921FEC525FB3F7B5AEBC7C5CE7775A8AEFA883C13F14`，符号 ZIP SHA-256 `BE14C7D53658963DF1C6B1ECFAA44AC51F8004883530CB7DF922E4282FC11031`。ZIP 内容与 PE 审计通过，从 ZIP 解压后以 20 个全新数据目录启动均响应，进程崩溃 0；人工可读性仍为 `not_tested`。

## 替换的未审计资源

原 `resources/font.bz2` 在 Dragonrster 提交 `f0e9b52a0c3a6a87562aa87527ce84316321e720` 被替换为 476,825 字节扩展字库，但提交、仓库和文件内均没有名称、作者、来源或许可证。该资源不再作为发布字体输入或发布依据。

## 许可证结论

SIL OFL 1.1 明确允许字体与软件一起打包、嵌入、再分发和修改，条件是附带许可证且不将字体单独出售。发布包随附 GNU Unifont 许可证、Fusion Pixel Font 主许可证及其 Ark Pixel Font、Cubic 11、Galmuri 上游许可证。BDF 文件名和字体内部名称未冒用保留字体名，程序只嵌入转换后的字形子集。因此来源、再分发、嵌入和修改许可均已确认。

## 测试结论

- **font_source_verified：**`true`。
- **font_license_verified：**`true`。
- **font_container_valid：**`true`。
- **font_glyph_coverage_valid：**`true`。
- **font_pack_roundtrip_test：**`true`。
- **unifont_source_decode_test：**`true`。
- **fusion_bdf_source_decode_test：**`true`。
- **font_pixel_conversion_valid：**`true`。
- **font_engine_render_valid：**`true`。
- **zh_known_glyph_test：**`true`。
- **rejected_fix_font_visual_readability_valid：**`false`。用户已对 `E52E746B...` 私有试包执行解压运行和中文人工查看，结果未达到既有出版中文基线。
- **font_visual_readability_valid：**`not_tested`。原生 Fusion 12px 方案必须从新的最终 ZIP 解压运行，由人工检查简体中文、英文、数字、化学式、全角标点、窄菜单、长图鉴、高 DPI 和语言切换。
