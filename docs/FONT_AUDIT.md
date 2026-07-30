# 字体审计

状态：来源和许可证已验证；旧候选字体功能失败，已拒绝。修复候选完成容器、转换和引擎离屏渲染验证；最终 ZIP 的人工 GUI 视觉检查仍为 `NOT RUN`，因此不具备发布资格。

## 发布字体

程序嵌入 `resources/font.bz2`，这是 TPT 的 12 像素位图格式，不是系统 TTF/OTF。
发布资源由 `tools/build_release_font.py` 从以下受版本控制的输入确定性生成：

| 输入 | 名称/作者 | 版本与来源 | 许可证 | SHA-256 | 用途 |
|---|---|---|---|---|---|
| `resources/third_party/tpt-upstream-font-100.0.399.bz2` | The Powder Toy upstream bitmap font | 官方提交 `bff38ce6959e1c1a7a4d17d0d5d44d127a0dfcbd` | GPL-3.0（随主项目） | `EA86995CC429146F9869C172DC6DAE63D9B6E8BBFC7AC7D07D0CF30FB07D17F7` | 原有 UI、图标和拉丁字符 |
| `resources/third_party/unifont_all-16.0.03.hex.gz` | GNU Unifont，Roman Czyborra、Paul Hardy 及贡献者 | GNU Unifont 16.0.03，https://ftp.gnu.org/gnu/unifont/unifont-16.0.03.hex.gz | SIL Open Font License 1.1，附 GNU 字体嵌入例外 | `23AB31CA87C6614B97928A39DC0C15BC2AAA5B6F130ADF9E1DF481250F73BAAF` | 仅补充嵌入语言目录所需的缺失 Unicode 字形 |
| `resources/third_party/GNU_UNIFONT_COPYING.txt` | GNU Unifont 发布许可证文本 | https://unifoundry.com/LICENSE.txt | SIL OFL 1.1 / GPL 字体例外 | `1E74CB82BF476843E97C2596297B04219B1A7E51F7238944A8C031CB9401FA87` | 随二进制包分发 |

生成步骤只将所需语言 JSON 中的字符加入上游字体。GNU Unifont 的 16x16 单色字形按固定 3:4 比例使用确定性的面积覆盖量化为该渲染器的 12 像素高度，再写入项目自己的压缩位图容器；未改写 Unifont 原始文件。`tools/build_release_font.py` 会在任何嵌入语言字符缺少合法来源字形时失败。

## 2026-07-30 中文字体故障与修复

已拒绝候选 `5828a97fc39129547354956dde84d7b6cfb818c2` 的 `font.bz2` 哈希为 `938F989E2375B97B82F40104CA53D2367CF211265B545A6F061D532A5BDFE042`。根因有两项，均位于 GNU Unifont 到 TPT 2bpp 容器的转换器：

1. 转换器将每四个 2bpp 像素按高位优先写入；`FontReader::NextPixel()` 按低位优先读取。因此每组四个像素的横向顺序被反转，导致中文笔画碎裂和错位。
2. 16x16 到 12 高度的整数点采样跳过源行 3、7、11、15；例如 `U+4E00` 的唯一横笔在第 7 行，转换后成为空白字形。

修复后的 `resources/font.bz2` 哈希为 `4306249BD82DDEF2EEB15E2A3668AC02B970662768B392C8B45515BB550CBA34`。转换器现在按 `FontReader` 的低位优先协议打包，并用面积覆盖量化保留细笔画。`tools/validate_tpt_font.py` 验证压缩容器、三字节码点排序、字形边界、替换字形、语言目录覆盖、pack/unpack 往返及固定 Unifont 源码点 `U+4E2D/U+6587/U+7B80/U+4F53/U+5DE5/U+4E1A`。指定字形 PNG 输出至 `artifacts/zh-ui-fix/font/decoded/`。

`font_render_probe` 直接使用 `FontReader`、`Graphics::TextSize` 和 `Graphics::BlendText` 渲染中文、混合符号及化学文本，并逐条测量和绘制 `zh-CN.json` 的 1,262 条文本（替换字形为 0）；其输出位于 `artifacts/zh-ui-fix/render-probe/`。该离屏验证通过不等于人工 GUI 可读性验证。

## 替换的未审计资源

原 `resources/font.bz2` 在 Dragonrster 提交 `f0e9b52a0c3a6a87562aa87527ce84316321e720` 被替换为 476,825 字节扩展字库，但提交、仓库和文件内均没有名称、作者、来源或许可证。该资源不再作为发布字体输入或发布依据。

## 许可证结论

SIL OFL 1.1 明确允许字体与软件一起打包、嵌入、再分发和修改，条件是附带许可证且不将字体单独出售。发布包随附 `LICENSES/GNU-UNIFONT-OFL-1.1.txt`。因此来源、再分发、嵌入和修改许可均已确认。

## 测试结论

- **font_source_verified：**`true`。
- **font_license_verified：**`true`。
- **font_container_valid：**`true`。
- **font_glyph_coverage_valid：**`true`。
- **font_pack_roundtrip_test：**`true`。
- **unifont_source_decode_test：**`true`。
- **font_pixel_conversion_valid：**`true`。
- **font_engine_render_valid：**`true`。
- **zh_known_glyph_test：**`true`。
- **font_visual_readability_valid：**`not_tested`。必须在最终 ZIP 解压目录由人工检查简体中文、英文、数字、化学式、全角标点、窄菜单、长图鉴、高 DPI 和缩放。
