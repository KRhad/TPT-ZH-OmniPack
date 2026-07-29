# 第三方来源与许可证

> 本项目在开发中使用 OpenAI Codex / AI 辅助。AI 不改变第三方版权归属，也不能代替授权核验。

## 主项目许可证

TPT-ZH-OmniPack 是 The Powder Toy 的派生作品，整体按 GNU General Public License version 3 发布。必须随二进制提供完整对应源码、`LICENSE`、构建说明、修改记录和原项目致谢。

Phase 0 审计的九个仓库顶层均含 GPL-3.0 `LICENSE`，本次固定文件的 SHA-256：

`0B383D5A63DA644F628D99C33976EA6487ED89AAA59F0B3257992DEAC1171E6B`

## 固定源码来源

| 项目 | URL | 分支 | 精确 commit | 使用方式 |
|---|---|---|---|---|
| The Powder Toy | https://github.com/The-Powder-Toy/The-Powder-Toy | `master` | `bff38ce6959e1c1a7a4d17d0d5d44d127a0dfcbd` | 生产底座、官方 ID/保存/核心 |
| The Powder Toy Chinese | https://github.com/Dragonrster/The-Powder-Toy-Chinese | `i18n-new` | `445fab51dcf66057e645371aa9c9a556425b3d2f` | 翻译语料和本地化覆盖参考 |
| Cracker1000 TPT | https://github.com/cracker1000/The-Powder-Toy | `master` | `ebbb9aab6aef27d26517682cebbc0a07147a843a` | 元素/自动化设计参考，按当前 API 重写 |
| SpikeViper Biology | https://github.com/SpikeViper/The-Powder-Toy | `master` | `134ebf330eda42b4b300a2b7613ede71261697df` | 生物系统设计参考，重写 |
| TPT Ultimata Mod | https://github.com/Bowserinator/TPT-Ultimata-Mod | `development` | `b74971752433652c033559abea415ec3510ac433` | 特殊物理/电子/载具候选，精选重写 |
| Jacob1 Mod | https://github.com/jacob1/The-Powder-Toy | `c++` | `b492616124d2346a5ec7bc70fa881fb73345396b` | 自动化、UX 和存档来源识别参考 |
| TPT-Alchemy | https://github.com/jacob1/TPT-Alchemy | `master` | `9a593ce11536e2e683bc64a698399c0805ce77a1` | 炼金模式设计参考，进度实现重写 |
| Seppo's Metallurgy Mod SRC | https://github.com/SeppoTPT/Seppo-s-Metallurgy-Mod-SRC | `master` | `c3a8dd171a1c0fefc9a386e7e069f81d91f1514f` | 清洁室需求参考；实现源码缺失 |
| Cyens Toy | https://github.com/cbeimers113/cyens-toy | `master` | `f01d992c97432ec1c46d84ade05131da521f355a` | 烃化学/气体/特殊物理设计参考，重写 |

详细版本、元素数、风险与裁决见 `docs/SOURCE_AUDIT.md`；实际文件级来源进入 `docs/PORTING_LEDGER.md`。

## 论坛与文档资料

- Seppo's Metallurgy Mod 公开主题：  
  https://powdertoy.co.uk/Discussions/Thread/View.html?Thread=24204
- Cyens Toy 公开主题：  
  https://powdertoy.co.uk/Discussions/Thread/View.html?Thread=22486
- Ultimata 仓库内 `Wiki/`：随固定 commit 一并审计。

这些资料只用于核对公开行为和已知问题；不会用论坛二进制补全缺失源码。

## 字体、图像、音效与二进制

- Dragonrster 分支的扩展 `font.bz2` 暂不进入发布：其名称、作者、来源和许可证不可追溯。
- Phase 0 没有引入新字体、音效、图片或第三方二进制。
- 后续字体必须记录原始字体文件、版本、作者、许可证、来源 URL、子集/转换命令及生成物哈希。
- 发布包不会包含任何只有二进制而没有对应合法源码的模组实现。

## 当前代码使用情况

截至 Phase 0：

- 第三方实现代码移植：0；
- 正式新增元素：0；
- 来源仓库只读审计：9；
- 未解决授权项：汉化分支中文字库 1 项；
- 缺失源码项：Seppo 自定义元素实现 32 项。

