# 1.0.0 公开构建快照状态

这里的 `1.0.0` 是产品版本号。当前内容作为未签名构建快照直接发布在 `public-source` 分支，不等同于已完成发布门禁的 `v1.0.0` GitHub Release。

公开源码由 `fb72d5e8f658053842d31366486b5314104b4c41` 对应源码包全新解压而来，公开 Git 历史从单独的根提交开始，不包含开发分支历史。源码包已在独立目录使用 `build_tests=false` 完成 Release 编译；公开 Actions 同样只编译、不运行测试。

未完成的正式发布门禁：

- Windows 可执行文件未做 Authenticode 签名；
- 中文/英文、常见窗口缩放与 100%/125%/150% DPI 的完整人工视觉矩阵未完成；
- 独立两小时综合长跑未完成；
- 未创建 `v1.0.0` Tag；
- 未创建 GitHub Release。

```text
public_tests_included=false
private_test_evidence_preserved=true
public_source_buildable=true
public_source_commit_available=true
github_release_created=false
release_tag_created=false
authenticode_signed=false
gui_dpi_matrix_complete=false
long_run_complete=false
release_ready=false
```
