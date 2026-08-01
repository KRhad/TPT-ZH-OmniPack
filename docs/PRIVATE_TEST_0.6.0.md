# TPT-ZH-OmniPack 0.6.0-dev 私有测试说明

这是当前纯沙盒内容扩展分支的 Windows x64 私有测试版，不是 1.0.0 正式版，也不是公开发布候选。它不包含玩家任务、成就、科技树、发现进度或强制解锁；所有已启用内容均可直接选择和放置。

## 本版范围

- 可玩材料 `451` 种，其中完整周期表为 `118/118`。
- 当前最高稳定元素 ID 为 `621`；旧 ID 和 identifier 不重新分配。
- 包含冶金、工程材料、矿物/陶瓷/玻璃、无机化学、代表性核素、核工业、生态，以及两批有机物和聚合物。
- 图鉴正文前应先显示“元素说明 / Element description”，再介绍具体材料。
- 本测试版仍为 `release_ready=false`。

## 建议启动方式

完全解压 ZIP 后再启动，不要直接在压缩包内运行。普通双击会使用 OmniPack 自己的应用数据目录；如果希望把本次测试数据完全隔离，可在解压目录打开 PowerShell 后运行：

```powershell
New-Item -ItemType Directory -Force .\test-data | Out-Null
.\tpt-zh-omnipack.exe ddir .\test-data
```

便携构建不应要求安装程序或注册文件关联。EXE 未签名，核对随包 `.sha256` 和 ZIP 内 `TEST-MANIFEST.txt` 后再运行。

## 请优先检查

1. 简体中文默认显示是否清晰；切到英文再切回中文，菜单、搜索、周期表和图鉴是否正常。
2. 周期表面板能否按中文名、英文名、符号和原子序数搜索并直接放置 118 种元素。
3. 搜索并放置第二批有机/聚合物：`GLUC STRC CELU PRPE BDIE VCHL STYR TFET ADIP DIAM ERES PPLY PVCL PSTY NYLN RUBR EPXY PTFE BITM EACT`。
4. 检查淀粉/纤维素水解、葡萄糖发酵、单体聚合、尼龙缩合、环氧固化、酯化、沥青残余物和 PVC 强热分解。
5. 查看任一模组元素图鉴，确认先出现“元素说明”，正文再以对应材料名称开始。
6. 保存包含 `EACT=621`、核素和周期高位元素的场景，退出后重新加载，确认粒子没有丢失或变成其他元素。
7. 分别关闭冶金、生态、化学和核工业模块，确认对应元素不能新建，存档中的已有粒子仍保留但模块反应暂停。

## 尚未完成的正式门禁

- 600 秒正式压力矩阵：`not_tested`。
- 7,200 秒长跑：`not_tested`。
- 本批新增中文字形、全部 DPI 和完整 GUI 页面人工视觉矩阵：`not_tested`。
- 代码签名、公开源码匿名克隆、正式 tag 和公开 Release：未执行。

发现问题时请记录 ZIP SHA-256、`TEST-MANIFEST.txt` 中的 revision、操作步骤、截图及相关存档；不要附带账户令牌或个人配置。
