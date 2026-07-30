# 字体审计

状态：`PASS（来源和许可证）`；视觉、高 DPI 和多缩放级别的人工 GUI 检查仍为 `NOT RUN`。

## 发布字体

程序嵌入 `resources/font.bz2`，这是 TPT 的 12 像素位图格式，不是系统 TTF/OTF。
发布资源由 `tools/build_release_font.py` 从以下受版本控制的输入确定性生成：

| 输入 | 名称/作者 | 版本与来源 | 许可证 | SHA-256 | 用途 |
|---|---|---|---|---|---|
| `resources/third_party/tpt-upstream-font-100.0.399.bz2` | The Powder Toy upstream bitmap font | 官方提交 `bff38ce6959e1c1a7a4d17d0d5d44d127a0dfcbd` | GPL-3.0（随主项目） | `EA86995CC429146F9869C172DC6DAE63D9B6E8BBFC7AC7D07D0CF30FB07D17F7` | 原有 UI、图标和拉丁字符 |
| `resources/third_party/unifont_all-16.0.03.hex.gz` | GNU Unifont，Roman Czyborra、Paul Hardy 及贡献者 | GNU Unifont 16.0.03，https://ftp.gnu.org/gnu/unifont/unifont-16.0.03/unifont_all-16.0.03.hex.gz | SIL Open Font License 1.1，附 GNU 字体嵌入例外 | `23AB31CA87C6614B97928A39DC0C15BC2AAAA4544A481CFF314A285CEE3209DD` | 仅补充嵌入语言目录所需的缺失 Unicode 字形 |
| `resources/third_party/GNU_UNIFONT_COPYING.txt` | GNU Unifont 发布许可证文本 | https://unifoundry.com/LICENSE.txt | SIL OFL 1.1 / GPL 字体例外 | `1E74CB82BF476843E97C2596297B04219B1A7E51F7238944A8C031CB9401FA87` | 随二进制包分发 |

生成步骤只将所需语言 JSON 中的字符加入上游字体。GNU Unifont 的 16x16 单色字形按固定 3:4 比例重采样为该渲染器的 12 像素高度，再写入项目自己的压缩位图容器；未改写 Unifont 原始文件。`tools/build_release_font.py` 会在任何嵌入语言字符缺少合法来源字形时失败。

## 替换的未审计资源

原 `resources/font.bz2` 在 Dragonrster 提交 `f0e9b52a0c3a6a87562aa87527ce84316321e720` 被替换为 476,825 字节扩展字库，但提交、仓库和文件内均没有名称、作者、来源或许可证。该资源不再作为发布字体输入或发布依据。

## 许可证结论

SIL OFL 1.1 明确允许字体与软件一起打包、嵌入、再分发和修改，条件是附带许可证且不将字体单独出售。发布包随附 `LICENSES/GNU-UNIFONT-OFL-1.1.txt`。因此来源、再分发、嵌入和修改许可均已确认。

## 测试结论

- **源码确认：**生成器解析全部嵌入语言目录并拒绝缺字；输出按稳定排序和 bzip2 level 9 生成。
- **自动测试确认：**发布构建前运行生成器并记录输出 SHA-256。
- **实际 GUI 运行确认：**`not_tested`。需在最终 ZIP 解压目录检查简体中文、英文、数字、化学式、全角标点、窄菜单、长图鉴、高 DPI 和缩放。
