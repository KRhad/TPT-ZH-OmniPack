# 模组许可证审计

只有仓库根许可证或可核对的许可证文件存在并被识别时，`license_verified=true`。API 标签、仓库描述或论坛口头说明不能单独替代文件证据。

本表同时扫描 README 许可证字样、元素源码前 80 行、子模块、非代码资源和嵌套 NOTICE/LICENSE。自动扫描完成不等于版权判断完成；实际移植仍须对选定文件逐项复核并登记来源。

| mod_id | 根许可证 | 根证据 | README 证据 | 元素源码扫描 | 资源/嵌套通知 | 子模块 | 初判 | 代码使用边界 |
|---|---|---|---|---:|---|---|---|---|
| dragonrster | unknown (false) | none | none | 0; header_notice=0; restrictive=0 | assets=0; nested_notice=0 | none | C-unknown | 只能记录概念；不得复制代码或资源 |
| official | GPL-3.0 (true) | LICENSE | README.md:13:There is a Lua API – you can automate your work or even make plugins for the game. The Powder Toy is free and the source code is distributed under the GNU General Public License, s | 213; header_notice=0; restrictive=0 | assets=14; nested_notice=0 | none | A-code-scope | 仅可让选定代码进入逐文件复核；保留作者、commit、文件和 GPL 通知 |
| biological_mod | GPL-3.0 (true) | LICENSE | none | 217; header_notice=0; restrictive=0 | assets=19; nested_notice=1 | none | A-code-scope | 仅可让选定代码进入逐文件复核；保留作者、commit、文件和 GPL 通知 |
| chem_mod_lua | unknown (false) | none | none | 1; header_notice=0; restrictive=0 | assets=0; nested_notice=0 | none | C-unknown | 只能记录概念；不得复制代码或资源 |
| chemistry_mod | GPL-3.0 (true) | LICENSE | README.md:13:There is a Lua API – you can automate your work or even make plugins for the game. The Powder Toy is free and the source code is distributed under the GNU General Public License, s | 189; header_notice=0; restrictive=0 | assets=17; nested_notice=0 | none | A-code-scope | 仅可让选定代码进入逐文件复核；保留作者、commit、文件和 GPL 通知 |
| cracker1000 | GPL-3.0 (true) | LICENSE | README.md:14:There is a Lua API – you can automate your work or even make plugins for the game. The Powder Toy is free and the source code is distributed under the GNU General Public License, s | 245; header_notice=0; restrictive=0 | assets=12; nested_notice=0 | none | A-code-scope | 仅可让选定代码进入逐文件复核；保留作者、commit、文件和 GPL 通知 |
| cyens_src | GPL-3.0 (true) | LICENSE | none | 228; header_notice=0; restrictive=0 | assets=23; nested_notice=0 | none | A-code-scope | 仅可让选定代码进入逐文件复核；保留作者、commit、文件和 GPL 通知 |
| fun_chemicals | unknown (false) | none | none | 1; header_notice=0; restrictive=0 | assets=0; nested_notice=0 | none | C-unknown | 只能记录概念；不得复制代码或资源 |
| periodic_mod | unknown (false) | none | none | 0; header_notice=0; restrictive=0 | assets=0; nested_notice=0 | none | C-unknown | 只能记录概念；不得复制代码或资源 |
| spikeviper | GPL-3.0 (true) | LICENSE | README.md:13:There is a Lua API – you can automate your work or even make plugins for the game. The Powder Toy is free and the source code is distributed under the GNU General Public License, s | 222; header_notice=0; restrictive=0 | assets=12; nested_notice=0 | none | A-code-scope | 仅可让选定代码进入逐文件复核；保留作者、commit、文件和 GPL 通知 |
| cyens | GPL-3.0 (true) | LICENSE | README.md:53:There is a Lua API – you can automate your work or even make plugins for the game. The Powder Toy is free and the source code is distributed under the GNU General Public License, s | 194; header_notice=0; restrictive=0 | assets=33; nested_notice=1 | none | A-code-scope | 仅可让选定代码进入逐文件复核；保留作者、commit、文件和 GPL 通知 |
| fanmod | unknown (false) | none | none | 1; header_notice=0; restrictive=0 | assets=0; nested_notice=0 | none | C-unknown | 只能记录概念；不得复制代码或资源 |
| jacob1 | GPL-3.0 (true) | LICENSE | none | 217; header_notice=208; restrictive=0 | assets=11; nested_notice=0 | none | A-code-scope | 仅可让选定代码进入逐文件复核；保留作者、commit、文件和 GPL 通知 |
| mod_archive | GPL-3.0 (true) | LICENSE | README:24:There is a Lua API � you can automate your work or even make plugins for the game. The Powder Toy is free and the source code is distributed under the GNU General Public License, s | 0; header_notice=0; restrictive=0 | assets=0; nested_notice=0 | none | A-code-scope | 仅可让选定代码进入逐文件复核；保留作者、commit、文件和 GPL 通知 |
| nucular_mod | GPL-3.0 (true) | LICENSE | README:24:There is a Lua API - you can automate your work or even make plugins for the game. The Powder Toy is free and the source code is distributed under the GNU General Public License, s | 202; header_notice=0; restrictive=0 | assets=2; nested_notice=0 | none | A-code-scope | 仅可让选定代码进入逐文件复核；保留作者、commit、文件和 GPL 通知 |
| plant_mod | unknown (false) | none | none | 0; header_notice=0; restrictive=0 | assets=0; nested_notice=0 | none | C-unknown | 只能记录概念；不得复制代码或资源 |
| realistic_combustibles | unknown (false) | none | none | 4; header_notice=0; restrictive=0 | assets=0; nested_notice=0 | none | C-unknown | 只能记录概念；不得复制代码或资源 |
| realistic_science | unknown (false) | none | none | 0; header_notice=0; restrictive=0 | assets=0; nested_notice=0 | none | C-unknown | 只能记录概念；不得复制代码或资源 |
| science_toy | unknown (false) | none | none | 195; header_notice=0; restrictive=0 | assets=0; nested_notice=0 | none | C-unknown | 只能记录概念；不得复制代码或资源 |
| seppo | GPL-3.0 (true) | LICENSE | README.md:13:There is a Lua API – you can automate your work or even make plugins for the game. The Powder Toy is free and the source code is distributed under the GNU General Public License, s | 187; header_notice=0; restrictive=0 | assets=17; nested_notice=0 | none | A-code-scope | 仅可让选定代码进入逐文件复核；保留作者、commit、文件和 GPL 通知 |
| tommig_mod | unknown (false) | none | none | 0; header_notice=0; restrictive=0 | assets=0; nested_notice=0 | none | C-unknown | 只能记录概念；不得复制代码或资源 |
| tpt_remade | unknown (false) | none | none | 1; header_notice=0; restrictive=0 | assets=0; nested_notice=0 | none | C-unknown | 只能记录概念；不得复制代码或资源 |
| ultimata | GPL-3.0 (true) | LICENSE | README.md:13:There is a Lua API – you can automate your work or even make plugins for the game. The Powder Toy is free and the source code is distributed under the GNU General Public License, s | 334; header_notice=0; restrictive=0 | assets=20; nested_notice=0 | none | A-code-scope | 仅可让选定代码进入逐文件复核；保留作者、commit、文件和 GPL 通知 |
| lua_mods_serg | unknown (false) | none | none | 2; header_notice=0; restrictive=0 | assets=0; nested_notice=0 | none | C-unknown | 只能记录概念；不得复制代码或资源 |
| magnet_toy | GPL-3.0 (true) | LICENSE | README.md:165:## License;README.md:167:Derivative work of [The Powder Toy](https://github.com/The-Powder-Toy/The-Powder-Toy), licensed under GPLv3. This mod inherits the same license. | 220; header_notice=0; restrictive=0 | assets=14; nested_notice=0 | none | A-code-scope | 仅可让选定代码进入逐文件复核；保留作者、commit、文件和 GPL 通知 |
| metacircuits | GPL-3.0 (true) | LICENSE | none | 200; header_notice=0; restrictive=0 | assets=17; nested_notice=0 | none | A-code-scope | 仅可让选定代码进入逐文件复核；保留作者、commit、文件和 GPL 通知 |
| norbs_mod | GPL-3.0 (true) | LICENSE | none | 218; header_notice=0; restrictive=0 | assets=14; nested_notice=0 | none | A-code-scope | 仅可让选定代码进入逐文件复核；保留作者、commit、文件和 GPL 通知 |
| powder_better | GPL-3.0 (true) | LICENSE | none | 217; header_notice=0; restrictive=0 | assets=14; nested_notice=0 | none | A-code-scope | 仅可让选定代码进入逐文件复核；保留作者、commit、文件和 GPL 通知 |
| zakpack | unknown (false) | none | none | 0; header_notice=0; restrictive=0 | assets=0; nested_notice=0 | none | C-unknown | 只能记录概念；不得复制代码或资源 |
| alchemagica | unknown (false) | none | none | 0; header_notice=0; restrictive=0 | assets=0; nested_notice=0 | none | C-unknown | 只能记录概念；不得复制代码或资源 |
| alchemy | GPL-3.0 (true) | LICENSE | README:15:There is a Lua API - you can automate your work or even make plugins for the game. The Powder Toy is free and the source code is distributed under the GNU General Public License, s | 182; header_notice=0; restrictive=0 | assets=2; nested_notice=0 | none | A-code-scope | 仅可让选定代码进入逐文件复核；保留作者、commit、文件和 GPL 通知 |
| bmn_mod | GPL-3.0 (true) | LICENSE | README:12:There is a Lua API - you can automate your work or even make plugins for the game. The Powder Toy is free and the source code is distributed under the GNU General Public License, s | 181; header_notice=0; restrictive=0 | assets=2; nested_notice=0 | none | A-code-scope | 仅可让选定代码进入逐文件复核；保留作者、commit、文件和 GPL 通知 |
| dtttpt | GPL-3.0 (true) | LICENSE | README:12:There is a Lua API - you can automate your work or even make plugins for the game. The Powder Toy is free and the source code is distributed under the GNU General Public License, s | 199; header_notice=0; restrictive=0 | assets=2; nested_notice=0 | none | A-code-scope | 仅可让选定代码进入逐文件复核；保留作者、commit、文件和 GPL 通知 |
| record_mod | GPL-3.0 (true) | LICENSE | none | 193; header_notice=0; restrictive=0 | assets=12; nested_notice=0 | none | A-code-scope | 仅可让选定代码进入逐文件复核；保留作者、commit、文件和 GPL 通知 |
| ryan_mod | unknown (false) | none | none | 0; header_notice=0; restrictive=0 | assets=0; nested_notice=0 | none | C-unknown | 只能记录概念；不得复制代码或资源 |
| temp_mod_fork | unknown (false) | none | none | 0; header_notice=0; restrictive=0 | assets=0; nested_notice=0 | none | C-unknown | 只能记录概念；不得复制代码或资源 |
| toast_mod | unknown (false) | none | none | 0; header_notice=0; restrictive=0 | assets=0; nested_notice=0 | none | C-unknown | 只能记录概念；不得复制代码或资源 |
| tpt_elements_editor | unknown (false) | none | none | 0; header_notice=0; restrictive=0 | assets=0; nested_notice=0 | none | C-unknown | 只能记录概念；不得复制代码或资源 |
| triheron_ui | unknown (false) | none | none | 0; header_notice=0; restrictive=0 | assets=0; nested_notice=0 | none | C-unknown | 只能记录概念；不得复制代码或资源 |
| unproductive | GPL-2.0 (true) | LICENSE | src/python/stdlib/ctypes/macholib/README.ctypes:3:License: Any components of the py2app suite may be distributed under;src/python/stdlib/ctypes/macholib/README.ctypes:4:the MIT or PSF open source licenses. | 83; header_notice=0; restrictive=0 | assets=6; nested_notice=0 | none | B-review | 许可证明确但兼容性需人工法律审查 |
| yeah_mod | unknown (false) | none | none | 0; header_notice=0; restrictive=0 | assets=0; nested_notice=0 | none | C-unknown | 只能记录概念；不得复制代码或资源 |

## 已选文件复核：Cyens `ACET/UREA`

- 固定仓库：`cbeimers113/cyens-toy-src`，`master`，`1b74504e4642cd967c0079499b57faa7f37d9668`；
- 根许可证：GNU GPL v3，文件 SHA-256 `0B383D5A63DA644F628D99C33976EA6487ED89AAA59F0B3257992DEAC1171E6B`；无子模块；
- 选定文件：`src/simulation/elements/ACET.cpp`（blob `c17e871f...`）与 `UREA.cpp`（blob `6f324d1f...`），提交作者 DaveYognaught；
- 使用范围：只保留可追溯材料概念和原始 ID 记录，当前属性/更新/反应全部重写，第三方更新函数逐行复制为 0；
- 资源范围：没有从该仓库移植字体、图片、声音或二进制；
- 结论：这两个文件的代码范围许可证复核为 `A-code-scope`，但整个 41 来源目录的逐文件/资源审计仍未完成，不能把局部通过提升成全局 `license_audit_pass=true`。

## 当前门禁

- 根许可证自动识别：已执行。
- README、元素源码头、子模块与资源清单自动扫描：已执行于成功检出的仓库。
- 每个最终移植文件的人工作者/许可证/资源兼容复核：尚未完成。
- 因此当前 `license_audit_pass=false`。未知许可证、仅二进制和下载失效来源不得复制。
