# 外部模组只读素材区

`external/repositories/`、`external/lua-mods/` 和 `external/rejected/` 中的下载内容不进入 OmniPack Git 历史，也不得整库 merge。克隆仓库固定 URL、分支与 commit 后，由 `tools/mod_catalog/` 只读扫描；可复核元数据写入 `external/metadata/` 和 `docs/MOD_*`。

- `repositories/`：公开 C++/mixed 源码克隆；
- `lua-mods/`：公开 Lua 源文件的原始只读副本；
- `metadata/`：提交来源种子与论坛下载摘要；体积较大的扫描 JSON 是可再生成的本地缓存，不进入 Git 历史；
- `patches/`：经过人工审核、保留来源的最小移植补丁；
- `rejected/`：不进入构建的隔离样本或拒绝元数据。

二进制-only 内容不得反编译。许可证不明内容不得复制代码；发现概念有价值时也必须记录为 `reference_only`，不能逐句改写规避许可证。
