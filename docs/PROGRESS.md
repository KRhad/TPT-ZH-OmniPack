# 开发进度

## 当前分支基线

```text
branch=research/mod-source-integration
audit_start_head=3ee6b0a15cbd7d76f0605af3b614215a3b54a9d4
pt_num=512
pmapbits=9
official_active_elements=195
omnipack_active_elements=48
total_active_elements=243
registered_slots=370
reserved_slots=127
enabled_content_modules=4
save_format=OPS1/BZip2
release_ready=false
```

## Phase 1：纯沙盒方向清理

- 删除玩家炼金服务、十阶段进度、元素发现锁、进度窗口和通知；
- 删除 GameSave/Simulation 的 `omniAlchemy` 读取与写出；
- 删除 Lua 进度 API、stamp 锁和普通创建中的进度分支；
- 保留模块关闭、非法 ID 和保留槽门禁；
- 删除 0.4 炼金本地包 profile、文档、探针和专用测试；
- 删除五个没有实际内容的玩家设置入口，稳定 ID 区间不变；
- 新增两个真实旧 OPS 的忽略字段/重存兼容探针；
- 图鉴正文新增“元素说明 / Element description”标签；
- 重建 12px 字体：14,623 字形、2,587 必需字符、SHA-256 `C0C4AB8472EEAFC22F73C0BB131D73CE2A3822DDC2EB893F1E52D5F8368236A9`；
- 全新 Release clean build `509/509` 通过，EXE SHA-256 `CAC60B021E92E46C232B8DEF07126BE8CF2F791BDE1722550C341AA02D3A05C4`；Meson static `18/18`、Python `124/124`、0 skip。
- 当前四模块及完整/简化生态 Lua 回归 `6/6`；0.2 反应样例 `7/7`、回归场景 `8/8`；自动化场景 `9/9`、工程断言组 `6/6`、95 断言、停止增量 0。
- 官方与四模块 OPS `5/5`：15 个进程、10 次重启、10 次加载、79 粒子、每次加载合计 120 个字段断言。

## 下一步

1. 提交已通过的 Phase 1；
2. 建立 `external/` 只读来源目录和 `tools/mod_catalog/` 提取器；
3. 联网固定主要公开模组的仓库、分支、commit 和许可证；
4. 建立 `MOD_*` 目录、去重报告和 118 元素来源映射；
5. 审计 ID 空间与 Lua 动态槽，再进入周期表基础设施和第一批元素。

开发回归场景、构建脚本、测试矩阵和版本门禁继续保留；它们不属于已删除的玩家游戏任务。
