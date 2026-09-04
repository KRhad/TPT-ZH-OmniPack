#!/usr/bin/env python3

"""Apply the Simplified Chinese source translation used for the local build.

The Korean fork is used only as a maintained inventory of user-facing strings.
Translations are generated in batches, with important game terminology and
short UI labels overridden below.  Element descriptions come from the current
Chinese TPT instruction project where possible.
"""

from __future__ import annotations

import argparse
import difflib
import json
import re
import time
import urllib.parse
import urllib.request
from pathlib import Path


ROOT = Path(__file__).resolve().parent
CACHE = ROOT / "build-zh-translation-cache.json"
GOOGLE_URL = "https://translate.googleapis.com/translate_a/single"
ELEMENT_INDEX_URL = (
    "https://raw.githubusercontent.com/Dragonrster/"
    "The-Powder-Toy-Chinese-Instruction/main/"
    "appendix-L-%E5%AE%8C%E6%95%B4%E5%85%83%E7%B4%A0%E7%B4%A2%E5%BC%95.md"
)

CPP_LITERAL = re.compile(r'"(?:\\.|[^"\\])*"')
TOKEN = re.compile(
    r"(?:\\(?:x[0-9A-Fa-f]+|[0-7]{1,3}|.)"
    r"|https?://[^\s]+"
    r"|(?:[A-Za-z0-9-]+\.)+[A-Za-z]{2,}"
    r"|#[A-Za-z0-9_-]+"
    r"|@[a-z]+"
    r"|%[-+0-9.*hlLzjt]*[A-Za-z%]"
    r"|DEFAULT_[A-Z0-9_]+"
    r"|\[[^\]\n]+\]"
    r"|\b(?:Ctrl|Shift|Alt|Tab|Enter|Esc|Space|Backtick|FPS|HUD|TPT|Lua|HTTP|ID)\b"
    r"|\b[A-Z][A-Z0-9_-]{1,}\b"
    r"|\.[A-Za-z0-9]{2,4}\b)"
)


# Concise labels and TPT-specific wording.  These take priority over machine
# translation and are deliberately shared when the same English literal occurs
# in more than one translated source file.
OVERRIDES = {
    "OK": "确定",
    "Okay": "确定",
    "Cancel": "取消",
    "Close": "关闭",
    "Done": "完成",
    "Add": "添加",
    "Remove": "移除",
    "Delete": "删除",
    "Rename": "重命名",
    "Change": "更改",
    "Open": "打开",
    "Save": "保存",
    "Render": "渲染",
    "Preview": "预览",
    "Search:": "搜索：",
    "Sort": "排序",
    "Submit": "提交",
    "Report": "举报",
    "Publish": "发布",
    "Unpublish": "取消发布",
    "Favourite": "收藏",
    "Unfavourite": "取消收藏",
    "Fav": "收藏",
    "Settings": "设置",
    "Credits": "制作人员",
    "Information": "信息",
    "Success": "成功",
    "Error": "错误",
    "Message": "消息",
    "Confirm": "确认",
    "Dismiss": "忽略",
    "Loading...": "正在加载……",
    "Saving...": "正在保存……",
    "Logging in...": "正在登录……",
    "Paused": "已暂停",
    "None": "无",
    "none": "无",
    "All": "全部",
    "On": "开",
    "Off": "关",
    "Current": "当前",
    "current": "当前",
    "enabled": "启用",
    "disabled": "禁用",
    "display": "显示器",
    "Page": "页",
    "Page 1 of 1": "1 / 1",
    "Page ": "第 ",
    " of ": " / ",
    "of ": "/ ",
    "Next ": "下一页 ",
    " Prev": " 上一页",
    "Search Help": "搜索帮助",
    "Search for ": "搜索 ",
    "By votes": "按评分",
    "By date": "按日期",
    "My Own": "我的作品",
    "Clear selection": "清除选择",
    "Open in browser": "浏览器打开",
    "Open data folder": "打开数据文件夹",
    "Save Browser": "存档浏览器",
    "Server login": "服务器登录",
    "Sign in": "登录",
    "Sign Out": "退出登录",
    "Edit Profile": "编辑资料",
    "Edit profile": "编辑资料",
    "Edit Avatar": "更换头像",
    "Logged in": "已登录",
    "Logged out": "已退出",
    "Username:": "用户名：",
    "Password:": "密码：",
    "Website:": "网站：",
    "Location:": "地区：",
    "Biography:": "个人简介：",
    "Age:": "年龄：",
    "Save ID": "存档 ID",
    "Save ID:": "存档 ID：",
    "Save Browser": "存档浏览器",
    "Delete Save": "删除存档",
    "Report Save": "举报存档",
    "Add Comment": "添加评论",
    "Preview:": "预览：",
    "Title": "标题",
    "Title:": "标题：",
    "Description:": "说明：",
    "Tags:": "标签：",
    "[name]": "[名称]",
    "[value]": "[值]",
    "[reason]": "[原因]",
    "[message]": "[消息]",
    "[search]": "[搜索]",
    "[filename]": "[文件名]",
    "[username]": "[用户名]",
    "[password]": "[密码]",
    "[save name]": "[存档名称]",
    "[save description]": "[存档说明]",
    "[search, F1 for help]": "[搜索，按 F1 查看帮助]",
    "[new tag]": "[新标签]",
    "[sign in]": "[登录]",
    "[no tags set]": "[无标签]",
    "[untitled simulation]": "[未命名模拟]",
    "Enter some text:": "请输入文字：",
    "/Register.html": "/Register.html",
    "/Register.html\\n": "/Register.html\\n",
    "Walls": "墙体",
    "Electronics": "电子",
    "Powered Materials": "可控材料",
    "Sensors": "传感器",
    "Force": "力",
    "Explosives": "爆炸物",
    "Gases": "气体",
    "Liquids": "液体",
    "Powders": "粉末",
    "Solids": "固体",
    "Radioactive": "核能",
    "Special": "特殊",
    "Game Of Life": "生命游戏",
    "Tools": "工具",
    "Favorites": "收藏",
    "Decoration tools": "装饰工具",
    "Find & open a simulation. Hold Ctrl to load offline saves.": "查找并打开在线模拟；按住 Ctrl 可加载本地存档。",
    "Open a simulation from your hard drive.": "从本地磁盘打开模拟存档。",
    "Reload the simulation": "重新加载当前模拟",
    "Erase everything": "清空模拟中的所有内容",
    "Sign into simulation server": "登录模拟服务器",
    "Renderer options": "渲染设置",
    "Pause/Resume the simulation": "暂停/继续模拟",
    "Search for elements": "搜索元素",
    "Pick Colour": "拾取颜色",
    "Add simulation tags": "添加模拟标签",
    "Like this save": "赞这个存档",
    "Dislike this save": "踩这个存档",
    "Re-upload the current simulation": "重新上传当前模拟",
    "Modify simulation properties": "修改模拟属性",
    "Upload a new simulation. Hold Ctrl to save offline.": "上传新模拟；按住 Ctrl 可保存到本地。",
    "Overwrite the open simulation on your hard drive.": "覆盖当前打开的本地模拟存档。",
    "Save the simulation to your hard drive.": "将模拟保存到本地磁盘。",
    "Save the simulation to your hard drive. Login to save online.": "将模拟保存到本地磁盘；登录后可保存到服务器。",
    "Vote": "评分",
    "Empty": "空",
    "Empty, Pressure: ": "空，压力：",
    ", Temp: ": "，温度：",
    ", Life: ": "，寿命：",
    ", Pressure: ": "，压力：",
    "Molten ": "熔融",
    " with ": "，内含 ",
    " with molten ": "，内含熔融",
    " Parts: ": " 粒子数：",
    " [GRID: ": " [网格：",
    " [FIND]": " [查找]",
    " [REPLACE MODE]": " [替换模式]",
    " [SPECIFIC DELETE]": " [指定删除]",
    "\\nSimulation": "\\n模拟",
    "\\nRendering": "\\n渲染",
    "\\n  FPS cap: ": "\\n  FPS 上限：",
    "Settings": "设置",
    "Temperature": "温度",
    "Pressure": "压力",
    "Velocity": "速度",
    "Gravity": "重力",
    "Custom Gravity": "自定义重力",
    "Ambient air velocity": "环境空气流速",
    "Boussinesq": "布辛涅斯克近似",
    "Boussinesq approximation": "布辛涅斯克近似",
    "Threaded rendering": "多线程渲染",
    "Fullscreen": "全屏",
    "Vertical": "垂直",
    "Radial": "径向",
    "Legacy": "旧版",
    "Celsius": "摄氏度",
    "Fahrenheit": "华氏度",
    "Kelvin": "开尔文",
    "Grid": "网格",
    "Rulers": "标尺",
    "Blob Display": "团状显示",
    "Heat Display": "温度显示",
    "Fancy Display": "炫彩显示",
    "Fire Display": "火焰显示",
    "Sand effect": "沙粒效果",
    "Show avatars": "显示头像",
    "Fast quit": "快速退出",
    "Edge mode": "边界模式",
    "Air: On": "空气：开启",
    "Air: Off": "空气：关闭",
    "Air: No Update": "空气：停止更新",
    "Gravity: Off": "重力：关闭",
    "Pressure off": "压力：关闭",
    "Velocity off": "速度：关闭",
    "New sign": "新建标牌",
    "Edit property": "编辑属性",
    "No character": "无字符",
    "No save open": "未打开存档",
    "Former Staff": "往届成员",
    "Average Score:": "平均评分：",
    "Run Updater": "运行更新程序",
    "No update": "没有更新",
    "Snapshot ": "快照版 ",
    " Beta, Build ": " 测试版，构建 ",
    "Mod version ": "模组版本 ",
    "Copied!": "已复制！",
    "Dropped file is not a TPT save file (.cps or .stm format)": "拖入的文件不是 TPT 存档（仅支持 .cps 或 .stm）。",
    "Use the mouse scroll wheel, or '[' and ']', to change the tool size for particles.\\n": "使用鼠标滚轮或 [、] 键调整粒子工具大小。\\n",
    "Key under Esc exits console": "按 Esc 下方的反引号键退出控制台",
    " - https://powdertoy.co.uk, irc.libera.chat #powder, https://tpt.io/discord\\n": " - https://powdertoy.co.uk、irc.libera.chat #powder、https://tpt.io/discord\\n",
    "Use \\bt!\\bw to negate terms, for example \\bg~city !destroyable !desert\\bw\\n": "使用 \\bt!\\bw 排除关键词，例如 \\bg~city !destroyable !desert\\bw\\n",
    "Use \\bt|\\bw to OR together search terms, for example \\bg~bomb | nuke | explosive\\bw\\n": "使用 \\bt|\\bw 表示“或”，例如 \\bg~bomb | nuke | explosive\\bw\\n",
    "Use \\bt\\\"\\bw to create multi-word search terms, for example \\bg~\\\"power plant\\\" uran | plut | polo\\bw\\n": "使用 \\bt\\\"\\bw 创建多词关键词，例如 \\bg~\\\"power plant\\\" uran | plut | polo\\bw\\n",
    "Use \\bt@user\\bw to limit search to only specific users, for example \\bg~@user 117n00b | Catelite | Fluttershy @title laser\\bw\\n": "使用 \\bt@user\\bw 限定作者，例如 \\bg~@user 117n00b | Catelite | Fluttershy @title laser\\bw\\n",
    "Use \\bt@title\\bw to limit search to only save titles, for example \\bg~@title subframe\\bw\\n": "使用 \\bt@title\\bw 仅搜索存档标题，例如 \\bg~@title subframe\\bw\\n",
    "Use \\bt@description\\bw to limit search to only save descriptions, for example \\bg~@description \\\"No description provided\\\"\\bw\\n": "使用 \\bt@description\\bw 仅搜索存档说明，例如 \\bg~@description \\\"No description provided\\\"\\bw\\n",
    "Use \\bt@tags\\bw to limit search to just save tags, for example \\bg~@tags resistcup @title printer | @description spider before:2024-06\\bw\\n": "使用 \\bt@tags\\bw 仅搜索标签，例如 \\bg~@tags resistcup @title printer | @description spider before:2024-06\\bw\\n",
    " X:": " X：",
    " Y:": " Y：",
    " Total:": " 总计：",
    "Do Migration?": "迁移数据？",
    "Fetch them now": "立即获取",
    "Sticky categories": "点击锁定分类",
    "Perfect circle brush": "精确圆形笔刷",
    "Momentum (old) scrolling": "惯性（旧式）滚动",
    "Accelerating instead of step scroll": "连续加速滚动",
    "When saving, copying, stamping, etc.": "保存、复制、制作图章等操作时",
    "Force integer scaling \\bg- less blurry": "整数倍缩放 \\bg- 更清晰",
    "Heat simulation \\bgIntroduced in version 34": "热量模拟 \\bg- 版本 34 引入",
    "This will migrate all stamps, saves, and scripts from\\n\\bt": "将从以下位置迁移所有图章、存档和脚本：\\n\\bt",
    "\\bw\\nto the shared data directory at\\n\\bt": "\\bw\\n到以下共享数据目录：\\n\\bt",
    " - Find out who contributed to TPT": " - 查看 TPT 的贡献者",
    "Go to save ID:": "转到存档 ID：",
    "\\n  Draw cap: ": "\\n  绘制上限：",
    "\\n  Refresh rate: ": "\\n  刷新率：",
    " [TEMP L:": " [温度 最低：",
    " H:": " 最高：",
    "hindered": "受限制",
    "Open forum thread ": "论坛主题 ",
    " in browser": "（浏览器打开）",
    "Are you sure you want to remove ": "确定要移除 ",
    "Dropped stamp could not be loaded: ": "无法加载拖入的图章：",
    "Dropped save file could not be loaded: ": "无法加载拖入的存档：",
    "Error loading stamp": "加载图章失败",
    "Error loading save": "加载存档失败",
    "Cannot open save": "无法打开存档",
    "Unable to build save.": "无法生成存档。",
    "Could not create stamp": "无法创建图章",
    "Error generating save file": "生成存档文件失败",
    "Unable to write save file.": "无法写入存档文件。",
    "You need to login to upload saves.": "请先登录再上传存档。",
    "Saved Successfully": "保存成功",
    "\\boNo saves found": "\\bo未找到存档",
    "\\bthistory:#######\\bw searches for a save by its ID and returns its history\\n": "\\bthistory:#######\\bw 按 ID 查找存档并显示其历史版本\\n",
    " It also concatenates search terms with AND instead of OR.\\n": " 它还会用 AND 而不是 OR 连接搜索词。\\n",
    "Sorting: Click \\bt\"By votes\"\\bw or \\bt\"By date\"\\bw to change the sorting mode\\n": "排序：点击 \\bt\"按评分\"\\bw 或 \\bt\"按日期\"\\bw 切换存档排序方式\\n",
    "Type in the search bar to automatically search save titles and tags. Multiple search terms are ORed together.\\n": "在搜索框输入内容即可自动搜索存档标题和标签；多个关键词按 OR 组合。\\n",
    "Categories: When signed in, click \\bt\"My Own\"\\bw to show your own saves, or \\bt\"Favourites\"\\bw to show your favourites\\n": "分类：登录后可点击 \\bt\"我的作品\"\\bw 查看自己的存档，或点击 \\bt\"收藏\"\\bw 查看收藏的存档。\\n",
    "Parentheses can be used to combine more complex search queries. For example: \\bg~(@user MG99 @description complete) | (@user goglesq @tags tutorial)\\bw": "括号可组合复杂查询。例如：\\bg~(@user MG99 @description complete) | (@user goglesq @tags tutorial)\\bw",
    "Loading save...": "正在加载存档……",
    "Please do not swear": "请勿使用脏话",
    "Do not ask for votes": "请勿索要评分",
    "Bad language may be deleted": "含不当用语的内容可能被删除",
    "Please report stolen saves": "请举报盗用的存档",
    "Stolen? Report the save instead": "发现盗用？请直接举报存档",
    "Click the box below to copy the save ID": "点击下方文本框复制存档 ID",
    "Report submitted": "举报已提交",
    "Unable to file report": "无法提交举报：",
    "\\n\\nA list of element IDs that are missing identifiers. Only the author of this save can fix this.\\n": "\\n\\n以下是没有关联标识符的缺失自定义元素 ID；只有存档作者才能修复。\\n",
    "\\n\\nA list of identifiers for missing custom elements, which may help identify how to fix this issue.\\n": "\\n\\n以下是缺失自定义元素的标识符，可用于判断如何修复问题。\\n",
    "Anti-air. Very light gravity-defying dust. Burns cold instead of hot.": "反空气尘。极轻、反重力；燃烧时吸热而不是放热。",
    "Anti-Matter. Destroys a majority of particles.": "反物质。可湮灭大多数粒子。",
    "Broken Glass, heavy particles, formed when glass breaks under pressure. Melts. Bagels.": "碎玻璃。玻璃受压破碎后形成的重质粒子；可熔化。还有贝果。",
    "Breakable metal. Common conductive building material, can melt and break under pressure.": "可破坏金属。常用导电建材，受压会破碎，也可熔化。",
    "Bizarre gas. Transforms into solid or liquid around room temperature. Conducts electricity. Flammable.": "奇异气体。接近室温时会转变为固体或液体；导电且可燃。",
    "Bizarre liquid. Transforms into solid or gas around room temperature. Conducts electricity. Flammable.": "奇异液体。接近室温时会转变为固体或气体；导电且可燃。",
    "Bizarre solid. Transforms into liquid or gas around room temperature. Conducts electricity. Flammable.": "奇异固体。接近室温时会转变为液体或气体；导电且可燃。",
    "Clay dust. Produces paste when mixed with water.": "黏土粉尘。与水混合会形成糊状物。",
    "Concrete. Strong building material, melts at high temperatures.": "混凝土。坚固的建筑材料，高温下会熔化。",
    "Converter. Converts everything into the first element it touches.": "转换器。把一切转化为它最先接触的元素。",
    "Deuterium oxide. Volume changes with temperature, radioactive with neutrons.": "重水。体积随温度变化，受中子轰击会产生核反应。",
    "Electrons. Produce sparks when hitting conductive materials. Reduce neutrons into hydrogen.": "电子。撞击导电材料时产生火花；可把中子转化为氢。",
    "Fighter. Tries to kill stickmen. You must first give it an element.": "战士。会尝试杀死火柴人；需要先赋予它一种元素。",
    "Filter. Changes the color of light passing through it. Allows photons to pass through, blocking all other particles.": "滤镜。改变穿过它的光线颜色；仅允许光子通过，并阻挡其他粒子。",
    "Force Emitter. Pushes or pulls objects based on its temperature. Use like ARAY.": "力场发射器。按自身温度推拉物体；用法与 ARAY 类似。",
    "Freeze water. Spreads quickly and cools down everything around it.": "冷焰。快速扩散并冷却周围的一切。",
    "Freeze powder. When lit, freezes things instead of burning them.": "冷冻粉。点燃后会冻结物体，而不是使其燃烧。",
    "Fuse Powder. Burns slowly like FUSE.": "引信粉。像 FUSE 一样缓慢燃烧。",
    "Glow. Glows under pressure.": "荧光液。受压时发光。",
    "Gravity bomb. Changes gravity to attract everything nearby, then explodes into repulsive gravity.": "引力炸弹。先以引力吸引附近物质，随后爆发为斥力。",
    "Gravity pump. Changes gravity. Use HEAT and COOL to control.": "引力泵。改变重力；使用 HEAT 和 COOL 调节。",
    "Hydrogen. Combusts with OXYG to make WATR. Undergoes fusion at high temperature and pressure.": "氢气。与 OXYG 燃烧生成 WATR；在高温高压下发生聚变。",
    "Instantly conducts, PSCN to charge, NSCN to discharge.": "瞬间导电；用 PSCN 充电、NSCN 放电。",
    "Insulated wire. Does not conduct to metal or semiconductors.": "绝缘导线。不会向金属或半导体导电。",
    "Iron. Rusts with salt, can be used for electrolysis of WATR.": "铁。接触盐会生锈，可用于电解 WATR。",
    "Mercury. Volume changes with temperature, conductive.": "汞。体积随温度变化，且可导电。",
    "N-Type Silicon. Will transfer current to P-Type Silicon.": "N 型硅。可将电流传给 P 型硅。",
    "Oxygen gas. Ignites easily.": "氧气。容易助燃。",
    "PIPE. Transfers particles around. Once the BRCK is sparked, the PIPE will grow and then become functional.": "管道。用于传送粒子；对 BRCK 通电后，PIPE 会生长并开始工作。",
    "Plant. Drinks water and grows.": "植物。吸水并生长。",
    "Powered breakable clone. Can be activated with PSCN and deactivated with NSCN.": "可控可破坏克隆。用 PSCN 激活、NSCN 关闭。",
    "Powered clone. Can be activated with PSCN and deactivated with NSCN.": "可控克隆。用 PSCN 激活、NSCN 关闭。",
    "Powered version of PIPE, use PSCN/NSCN to Activate/Deactivate.": "可控管道；用 PSCN/NSCN 激活或关闭。",
    "Portal IN. Particles go in here. Also has temperature dependent channels. (Use WIFI channels)": "入口传送门。粒子从这里进入；通道随温度变化（与 WIFI 通道相同）。",
    "Portal OUT. Particles come out here. Also has temperature dependent channels. (Use WIFI channels)": "出口传送门。粒子从这里出来；通道随温度变化（与 WIFI 通道相同）。",
    "P-Type Silicon. Will transfer current to any conductor.": "P 型硅。可将电流传给任何导体。",
    "Platinum. Catalyzes certain reactions.": "铂。可催化某些反应。",
    "Powered VOID. When activated, destroys entering particles.": "可控虚空。激活后会消除进入的粒子。",
    "Ray Point. Once charged, creates a ARAY beam pointed in the direction of the SPRK.": "射线点。通电后，会沿 SPRK 的方向生成 ARAY 光束。",
    "Resist. Very strong, can absorb impacts.": "抗性材料。非常坚固，可吸收冲击。",
    "Shield. Spark it to grow.": "能量盾。通电后会生长。",
    "Solid, created when steam cools rapidly and goes through sublimation.": "霜。蒸汽快速冷却并凝华时形成。",
    "Solid, melts under pressure.": "蜡。受压会熔化。",
    "Solidified resist. Transform into RES when hitting something.": "固化抗性材料。撞击物体时会变为 RES。",
    "Storage. Captures and stores a single particle. Releases when charged with PSCN, also passes to PIPE.": "储存体。捕获并保存一个粒子；用 PSCN 通电时释放，也可传给 PIPE。",
    "Switch. Only conducts when switched on. (PSCN switches on, NSCN switches off)": "开关。仅在开启时导电（PSCN 开启，NSCN 关闭）。",
    "Smart particles. Travels in straight lines and avoids obstacles. Gathers into groups and avoids other groups.": "智能粒子。沿直线移动并避开障碍；会聚成群并避开其他群体。",
    "Titanium. Higher melting temperature than most other metals. Blocks all air pressure.": "钛。熔点高于大多数金属，并能完全阻挡气压。",
    "Vacuum, sucks in other particles and heats up.": "真空。吸入其他粒子并升温。",
    "Vibranium. Stores energy and releases it in violent explosions.": "振金。储存能量，并以猛烈爆炸的形式释放。",
    "Velocity sensor. Generates SPRK when a particle with velocity crosses it.": "速度传感器。运动粒子穿过时产生 SPRK。",
    "Water. Conducts electricity, freezes, and extinguishes fires.": "水。导电，可结冰，并能灭火。",
    "WireWorld wires. Conduction happens on the GOL layer, and is not affected by walls.": "WireWorld 导线。在生命游戏层导电，不受墙体影响。",
    "Cyclone, produces swirling air currents": "旋风，产生旋转气流",
    "Heats the targeted element.": "加热目标元素。",
    "Mixes particles.": "混合粒子。",
    "Vacuum, reduces air pressure.": "真空，降低气压。",
    "Don't have an account? {a:": "没有账号？{a:",
    "|\\btRegister here\\x0E}.": "|\\bt点此注册\\x0E}。",
    "Publishing Info": "发布说明",
    "Save Uploading Rules": "存档上传规则",
    "Saving to server...": "正在保存到服务器……",
    "You must specify a save name.": "必须填写存档名称。",
    "This save was created by ": "此存档的作者是 ",
    ", you're about to publish this under your own name; If you haven't been given permission by the author to do so, please uncheck the publish box, otherwise continue": "；你即将以自己的名义发布它。若未获得原作者许可，请取消勾选“发布”；否则可继续。",
    "Modify simulation properties:": "修改模拟属性：",
    "Upload new simulation:": "上传新模拟：",
    "Upload failed with error:\\n": "上传失败：\\n",
    "In The Powder Toy, one can save simulations to their account in two privacy levels: Published and unpublished. You can choose which one by checking or unchecking the 'publish' checkbox. Saves are unpublished by default, so if you do not check publish nobody will be able to see your saves.\\n": "在 The Powder Toy 中，在线存档分为“已发布”和“未发布”两种可见级别，可通过“发布”复选框选择。新存档默认不发布；不勾选时，其他人不会在存档列表中看到它。\\n",
    "\\btPublished saves\\bw will appear on the 'By Date' feed and will be seen by many people. These saves also contribute to your Average Score, which is displayed publicly on your profile page on the website. Publish saves that you want people to see so they can comment and vote on.\\n": "\\bt已发布的存档\\bw会出现在“按日期”列表中，其他用户可以查看、评论和评分；评分也会计入网站个人资料中公开显示的平均评分。\\n",
    "\\btUnpublished saves\\bw will not be shown on the 'By Date' feed. These will not contribute to your Average Score. They are not completely private though, as anyone who knows the save id will be able to view it. You can give the save id out to show specific people the save but not allow just everyone to see it.\\n": "\\bt未发布的存档\\bw不会出现在“按日期”列表中，也不计入平均评分。但它并非完全私密：知道存档 ID 的人仍可查看，因此可以只把 ID 分享给指定的人。\\n",
    "To quickly resave a save, open it and click the left side of the split resave button to \\bt'Reupload the current simulation'\\bw. If you want to change the description or change the published status, you can click the right side to \\bt'Modify simulation properties'\\bw. Note that you can't change the name of saves; this will create an entirely new save with no comments, votes, or tags; separate from the original.\\n": "要快速更新存档，请打开它并点击分栏保存按钮左侧的\\bt“重新上传当前模拟”\\bw。若要修改说明或发布状态，请点击右侧的\\bt“修改模拟属性”\\bw。存档名称无法修改；改名会创建一个与原存档分离、没有评论、评分或标签的新存档。\\n",
    "You may want to publish an unpublished save after it is finished, or to unpublish some currently published ones. You can do this by opening the save, selecting the 'Modify simulation properties' button, and changing the published status there. You can also \\btunpublish or delete saves\\bw by selecting them in the 'my own' section of the browser and clicking either one of the buttons that appear on bottom.\\n": "完成作品后，可以打开存档并选择“修改模拟属性”来发布原本未发布的存档，或取消发布。也可在存档浏览器的“我的作品”分类中选中存档，再使用底部按钮\\bt取消发布或删除\\bw。\\n",
    "If a save is under a week old and gains popularity fast, it will be automatically placed on the \\btfront page\\bw. Only published saves will be able to get here. Moderators can also choose to promote any save onto the front page, but this happens rarely. They can also demote any save from the front page that breaks a rule or they feel doesn't belong.\\n": "发布不足一周且人气快速上升的存档会自动进入\\bt首页\\bw。只有已发布存档有此机会；管理员也可将存档推荐到首页，或将违规、不适合首页的存档移出。\\n",
    "Once you make a save, you can resave it as many times as you want. A short previous \\btsave history\\bw is saved, just right click any save in the save browser and select 'View History' to view it. This is useful for when you accidentally save something you didn't mean to and want to go back to the old version.\\n": "创建存档后可以反复更新，系统会保留一小段\\bt历史版本\\bw。在存档浏览器中右键单击存档并选择“查看历史”即可查看；误保存时可借此返回旧版本。\\n",
    "No saves found": "未找到存档",
    "Are you sure you want to delete ": "确定要删除 ",
    "Change save name": "更改存档名称",
    "No save name given": "未填写存档名称",
    "Are you sure you wish to overwrite\\n": "确定要覆盖以下文件吗？\\n",
    "Saves:": "存档数：",
    "Count:": "总数：",
    "Highest Score:": "最高评分：",
    "Persistent display mode preset": "残影显示模式预设",
    "Fire display mode preset": "火焰显示模式预设",
    "Blob display mode preset": "团状显示模式预设",
    "Heat display mode preset": "温度显示模式预设",
    "Fancy display mode preset": "炫彩显示模式预设",
    "Nothing display mode preset": "无特效显示模式预设",
    "Life display mode preset": "寿命显示模式预设",
    "Fire effect for gasses": "气体火焰效果",
    "Makes everything be drawn like a blob": "将所有内容绘制成团状",
    "Basic rendering, without this, most things will be invisible": "基础渲染；关闭后大多数内容将不可见",
    "Enables moving solids, stickmen guns, and premium(tm) graphics": "启用移动固体、火柴人的武器和高级画面效果",
    "Nothing Display": "无特效显示",
    "Dynamic Heat Display": "动态温度显示",
    "Middle click or Alt+Click to \\\"sample\\\" the particles.\\n": "单击中键或按住 Alt 单击可取样粒子。\\n",
    "Ctrl+Z will act as Undo.\\n": "Ctrl+Z 可撤销操作。\\n",
    "\\n\\boUse 'Z' for a zoom tool. Click to make the drawable zoom window stay around. Use the wheel to change the zoom strength.\\n": "\\n\\bo按 Z 使用缩放工具；单击可固定放大窗口，滚轮可调整缩放倍率。\\n",
    "Use 'H' to toggle the HUD. Use 'D' to toggle debug mode in the HUD.\\n": "按 H 显示或隐藏 HUD；按 D 切换 HUD 调试信息。\\n",
    "Custom GOL type: ": "自定义 GOL 类型：",
    "Colour blending: Add.": "颜色混合：相加。",
    "Colour blending: Subtract.": "颜色混合：相减。",
    "Colour blending: Divide.": "颜色混合：相除。",
    "x size mode since your screen was determined to be large enough: ": " 倍界面，因为检测到屏幕尺寸足够大：",
    " required": " 要求值",
    "Click the box below to copy the save id": "点击下方文本框复制存档 ID",
    "Logging out...": "正在退出……",
    "Could not favourite the save: ": "无法收藏该存档：",
    "Could not unfavourite the save: ": "无法取消收藏该存档：",
    "Please wait...": "请稍候……",
    "Erases walls.": "擦除墙体。",
    "Blocks everything. Conductive.": "阻挡所有物质，并可导电。",
    "E-Wall. Becomes transparent when electricity is connected.": "电子墙。通电时允许粒子通过。",
    "Detector. Generates electricity when a particle is inside.": "探测墙。内部出现粒子时产生电流。",
    "Streamline. Creates a line that follows air movement.": "流线。绘制一条跟随空气运动的轨迹。",
    "Fan. Accelerates air. Use the line tool to set direction and strength.": "风扇。推动空气；用直线工具设置方向和强度。",
    "Allows liquids, blocks all other particles. Conductive.": "仅允许液体通过，并可导电。",
    "Absorbs particles but lets air currents through.": "吸收粒子，但允许气流通过。",
    "Basic wall, blocks everything.": "基础墙体，阻挡所有物质。",
    "Allows air, but blocks all particles.": "允许空气通过，但阻挡所有粒子。",
    "Allows powders, blocks all other particles.": "仅允许粉末通过。",
    "Conductor. Allows all particles to pass through and conducts electricity.": "导电墙。允许所有粒子通过，并可导电。",
    "E-Hole. absorbs particles, releases them when powered.": "电子洞。吸收粒子，通电时将其释放。",
    "Allows gases, blocks all other particles.": "仅允许气体通过。",
    "Gravity wall. Newtonian Gravity has no effect inside a box drawn with this.": "重力墙。由它围成的区域不受牛顿引力影响。",
    "Allows energy particles, blocks all other particles.": "仅允许能量粒子通过。",
    "Allows all particles, but blocks air.": "允许所有粒子通过，但阻挡空气。",
    "Erases walls, particles, and signs.": "擦除墙体、粒子和标牌。",
    "Freezes particles inside the wall in place until powered.": "固定墙内的粒子；通电时解除固定。",
    "Anti-Matter, destroys a majority of particles.": "反物质。可湮灭大多数粒子。",
    "Broken Glass, heavy particles formed when glass breaks under pressure. Meltable. Bagels.": "碎玻璃。玻璃受压破碎后形成的重质粒子；可熔化。还有贝果。",
    "Bizarre gas.": "奇异气体。",
    "Bizarre solid.": "奇异固体。",
    "Ray Point. Rays create points when they collide.": "射线点。射线碰撞时会产生点。",
    "Broken electronics. Formed from EMP blasts, and when constantly sparked while under pressure, turns to EXOT.": "损坏的电子元件。由 EMP 爆炸产生；受压并持续通电时会变为 EXOT。",
    "Broken metal. Created when iron rusts or when metals break from pressure.": "金属碎片。由铁锈蚀或金属受压破裂形成。",
    "Concrete. Can stack on itself or ROCK, collapses with pressure.": "混凝土。可堆叠在自身或 ROCK 上，受压会坍塌。",
    "Converter. Converts everything into whatever it first touches.": "转换器。把一切转化为它最先接触的元素。",
    "Deuterium oxide. Gets more concentrated when cold, explodes with neutrons or protons.": "重水。低温时浓度提高；与中子或质子反应会爆炸。",
    "Conducts with temperature-dependent delay. (use HEAT/COOL).": "延迟导体，延迟时间随温度变化（使用 HEAT/COOL 调节）。",
    "Generates damaging pressure and breaks any elements it hits.": "产生破坏性压力，并击碎碰到的元素。",
    "A failed shared velocity test.": "一次失败的共享速度测试。",
    "Electrons. Sparks electronics, reacts with NEUT and WATR.": "电子。可使电子元件产生火花，并与 NEUT 和 WATR 反应。",
    "Fighter. Tries to kill stickmen. You must first give it an element to kill him with.": "战士。会尝试杀死火柴人；需要先赋予它一种攻击元素。",
    "Filter. Changes color of PHOT and BIZR. Color depends on temperature.": "滤镜。改变 PHOT 和 BIZR 的颜色，具体颜色取决于温度。",
    "Freeze water. Hybrid liquid formed when Freeze powder melts.": "冷冻水。冷冻粉熔化后形成的混合液体。",
    "Freeze powder. When melted, forms ice that always cools. Spreads with regular water.": "冷冻粉。熔化后形成会持续降温的冰，并可随普通水扩散。",
    "Gravity bomb. Sticks to the first object it touches then produces a strong gravity push.": "引力炸弹。黏在最先接触的物体上，随后产生强大斥力。",
    "Glow, Glows under pressure.": "荧光液。受压时发光。",
    "Corrosion resistant metal, will reverse corrosion of iron. Excellent conductor.": "耐腐蚀金属，可逆转铁的锈蚀；导电性能极佳。",
    "Gravity pump. Changes gravity to its temp when activated. (use HEAT/COOL)": "引力泵。激活后按自身温度改变重力（使用 HEAT/COOL 调节）。",
    "Instantly conducts, PSCN to charge, NSCN to take.": "瞬间导电；由 PSCN 送入电流、NSCN 导出电流。",
    "Insulated wire. Only conducts to PSCN, NSCN, WIFI, and SWCH.": "绝缘导线。仅与 PSCN、NSCN、WIFI 和 SWCH 导电。",
    "Rusts with salt, can be used for electrolysis of WATR.": "铁。接触盐会生锈，可用于电解 WATR。",
    "Mercury. Volume changes with temperature, Conductive.": "汞。体积随温度变化，且可导电。",
    "N-Type Silicon, Will not transfer current to P-Type Silicon. Disables powered materials.": "N 型硅。不会向 P 型硅传导电流；可关闭可控材料。",
    "Powered breakable clone.": "可控可破坏克隆。",
    "Powered clone. When activated, duplicates any particles it touches.": "可控克隆。激活后复制所接触的粒子。",
    "PIPE, moves particles around. Once the BRCK generates, erase some for the exit. Then the PIPE generates and is usable.": "管道，用于传送粒子。BRCK 外壳生成后擦除一部分作为出口，PIPE 随后会生成并可投入使用。",
    "Plant, drinks water and grows.": "植物。吸水并生长。",
    "Portal IN. Particles go in here. Also has temperature dependent channels. (same as WIFI)": "入口传送门。粒子从这里进入；通道随温度变化（与 WIFI 相同）。",
    "Portal OUT. Particles come out here. Also has temperature dependent channels. (same as WIFI)": "出口传送门。粒子从这里出来；通道随温度变化（与 WIFI 相同）。",
    "P-Type Silicon, Will transfer current to any conductor. Enables powered materials.": "P 型硅。可向任何导体传导电流；可开启可控材料。",
    "Solid, created when steam cools rapidly and goes through deposition, skipping the liquid phase.": "霜。蒸汽快速冷却、跳过液态直接凝华时形成。",
    "Solidified resist. Blocks pressure and insulates electricity. Liquefies on contact with neutrons.": "固化抗性材料。阻挡压力并绝缘；接触中子时液化。",
    "Resist. Solidifies on contact with photons, is destroyed by electrons and spark.": "抗性材料。接触光子时固化，会被电子和火花破坏。",
    "Shield. Grows around spark, broken by pressure.": "能量盾。围绕火花生长，受压会破裂。",
    "Light particles. Created when ICE breaks under pressure.": "雪。ICE 受压破碎时形成的轻质粒子。",
    "Electricity. The basis of all electronics in TPT, travels along conductive elements.": "电流。TPT 中所有电子元件的基础，会沿导电元素传播。",
    "Smart particles, Travels in straight lines and avoids obstacles. Grows with time.": "智能粒子。沿直线移动并避开障碍，会随时间生长。",
    "Titanium. Higher melting temperature than most other metals, blocks all air pressure.": "钛。熔点高于大多数金属，并能完全阻挡气压。",
    "Hole, will drain away any particles.": "虚空。会消除进入的任何粒子。",
    "Displaces other elements.": "使其他元素发生位移。",
    "Air vent, creates pressure and pushes other particles away.": "排气口。产生正压并推开其他粒子。",
    "WireWorld wires, conducts based on a set of GOL-like rules.": "WireWorld 导线，按一套类似 GOL 的规则导电。",
    "Steam. Produced from hot water.": "蒸汽。由热水产生。",
    "Newtonian gravity \\bgIntroduced in version 48": "牛顿引力 \\bg- 版本 48 引入",
    "Ambient heat simulation \\bgIntroduced in version 50": "环境热量模拟 \\bg- 版本 50 引入",
    "Can cause odd / broken behaviour with many saves": "可能导致许多存档表现异常或损坏",
    "Water equalisation \\bgIntroduced in version 61": "水量均衡 \\bg- 版本 61 引入",
    "No update": "不更新",
    "Custom": "自定义",
    "Resizable \\bg- allow resizing and maximizing window": "可调整窗口大小 \\bg- 允许缩放和最大化",
    "Fullscreen \\bg- fill the entire screen": "全屏 \\bg- 填满整个屏幕",
    "Blurry scaling \\bg- more blurry, better on very big screens": "平滑缩放 \\bg- 画面较模糊，适合超大屏幕",
    "Global quit shortcut": "全局退出快捷键",
    "Ctrl+q works everywhere": "Ctrl+Q 在任何界面均可退出",
    "Switch between categories by clicking": "单击即可切换分类",
    "Separate rendering thread": "独立渲染线程",
    "Fetch the message of the day and notifications": "获取每日消息和通知",
    "Save errors and other messages to a file": "把错误及其他消息写入文件",
    "Do you wish to install ": "是否要在此电脑上安装 ",
    " on this computer?\\nThis allows you to open save files and saves directly from the website.": "？\\n安装后可直接打开本地存档文件和网站中的存档。",
    "You don't need to install ": "此平台无需安装 ",
    " on this platform": "。",
    "Error serializing save file": "序列化存档文件失败",
    "Day": "日",
    "Week": "周",
    "Month": "月",
    "Year": "年",
    "Type in the search bar to begin automatically searching save titles and tags. Search terms are ORed together.\\n": "在搜索框输入内容即可自动搜索存档标题和标签；多个关键词按 OR 组合。\\n",
    "Sorting: click the \\bt\\\"By Votes\\\"\\bw / \\bt\\\"By Date\\\"\\bw buttons to change the order saves are displayed in\\n": "排序：点击 \\bt\"按评分\"\\bw / \\bt\"按日期\"\\bw 切换存档显示顺序。\\n",
    "Categories: If you're logged in, use \\bt\\\"My Own\\\"\\bw to view only your own saves, or click the Star icon to view your favorited saves\\n": "分类：登录后可点击 \\bt\"我的作品\"\\bw 只看自己的存档，或点击星形图标查看收藏。\\n",
    "Date Range: Click the dropdown to the right of the search box to select the date range for your search\\n": "日期范围：点击搜索框右侧的下拉菜单选择搜索时间范围。\\n",
    "\\btid:#######\\bw - search by save id\\n": "\\btid:#######\\bw - 按存档 ID 搜索\\n",
    "\\bthistory:#######\\bw - see previous versions for a save id\\n": "\\bthistory:#######\\bw - 查看指定存档 ID 的历史版本\\n",
    "\\btuser:XXXXXX\\bw - search for saves by a specific user\\n": "\\btuser:XXXXXX\\bw - 搜索指定用户的存档\\n",
    "\\btbefore:YYYY-MM-DD\\bw - all saves originally created before a certain date. Month and Day portions are both optional\\n": "\\btbefore:YYYY-MM-DD\\bw - 查找在指定日期前创建的存档；月和日均可省略\\n",
    "\\btafter:YYYY-MM-DD\\bw - all saves originally created after a certain date. Month and Day portions are both optional\\n": "\\btafter:YYYY-MM-DD\\bw - 查找在指定日期后创建的存档；月和日均可省略\\n",
    "Start a search with \\bt~\\bw to do an advanced search. This search works across save titles, descriptions, usernames, and tags, rather than only save titles and tags.": "以 \\bt~\\bw 开头可使用高级搜索；它会同时搜索存档标题、说明、用户名和标签。",
    "Parenthesis can be used to further complicate your searches. For example: \\bg~(@user MG99 @description complete) | (@user goglesq @tags tutorial)\\bw": "括号可组合复杂查询。例如：\\bg~(@user MG99 @description complete) | (@user goglesq @tags tutorial)\\bw",
    "Things to consider when reporting:\\n\\bw1)\\bg When reporting stolen saves, please include the ID of the original save.\\n\\bw2)\\bg Do not ask for saves to be removed from front page unless they break the rules.\\n\\bw3)\\bg You may report saves for comments or tags too (including your own saves)": "举报前请注意：\\n\\bw1)\\bg 举报盗用存档时，请附上原存档 ID。\\n\\bw2)\\bg 除非存档违规，否则请勿要求将其移出首页。\\n\\bw3)\\bg 也可以举报存档中的评论或标签（包括自己的存档）。",
    "Save from newer version": "较新版本的存档",
    "Submitting comment...": "正在提交评论……",
    "Unable to file report: ": "无法提交举报：",
    "This save uses custom elements that are not currently available. Make sure that you use the mod and/or have all the scripts the save requires to fully load.": "此存档使用了当前不可用的自定义元素。请使用相应模组，并确认已安装存档所需的全部脚本。",
    "\\n\\nA list of identifiers of missing custom elements follows, which may help you determine how to fix this problem.\\n": "\\n\\n以下是缺失自定义元素的标识符，可用于判断如何修复问题。\\n",
    "\\n\\nA list of element IDs of missing custom elements with no identifier associated follows. This can only be fixed by the author of the save.\\n": "\\n\\n以下是没有关联标识符的缺失自定义元素 ID；只有存档作者才能修复。\\n",
    "This save is from a snapshot or a beta version": "此存档来自快照版或测试版",
    "This save is from a newer version": "此存档来自较新版本",
    "Please update TPT in game or at ": "请在游戏内更新 TPT，或访问 ",
    "Switching to ": "检测到屏幕尺寸足够大，将切换到 ",
    "x size mode since your screen was determined to be large enough: ": " 倍界面：",
    " detected, ": "（检测到），需要 ",
    " required": "。",
    "View History": "查看历史",
    "More by this user": "该用户的更多作品",
    "Simulation framerate cap": "模拟帧率上限",
    "Rendering framerate cap": "渲染帧率上限",
    "Follow display": "跟随显示器刷新率",
    " - Set both limits to sane defaults": " - 将两个上限设为合理默认值",
    "Cannot open data folder: Platform::GetCwd(...) failed": "无法打开数据文件夹：获取当前目录失败",
    "Saving vote...": "正在提交评分……",
    "Paste content has missing custom elements": "粘贴内容缺少自定义元素",
    "set colour": "设置颜色",
    "subtract colour": "减去颜色",
    "red shift": "红移",
    "blue shift": "蓝移",
    "no effect": "无效果",
    "old QRTZ scattering": "旧版 QRTZ 散射",
    "variable red shift": "可变红移",
    "variable blue shift": "可变蓝移",
    " (unknown mode)": "（未知模式）",
    "If you see this it is a bug": "如果看到此提示，说明程序发生了错误",
    "Error loading comments": "加载评论失败",
    "Could not save user info: ": "无法保存用户资料：",
    "Could not load user info: ": "无法加载用户资料：",
    "Downloaded update is corrupted\\n": "下载的更新文件已损坏\\n",
    "Please go online to manually download a newer version.\\n": "请联网后手动下载新版本。\\n",
    "Edit sign": "编辑标牌",
    "odd, the update has disappeared": "更新信息已失效",
    "Could not open ": "无法打开 ",
    "Could not decompress font data": "无法解压字体数据",
    "Could not compress font data": "无法压缩字体数据",
    " is present in both files and is different!": " 同时存在于两个文件中，但内容不同！",
    "Adding U+": "正在添加 U+",
    " to the target": " 到目标字体",
    "Mod ": "模组 ",
    "screenshot ": "截图 ",
    "SDL_SaveBMP failed: ": "保存截图失败：",
    "Error: ": "错误：",
    "The message of the day and notifications have not yet been fetched, you can enable this in Settings": "尚未获取每日消息和通知，可在“设置”中启用。",
    "Fetching the message of the day...": "正在获取每日消息……",
    "Failed to load SSL certificates": "无法加载 SSL 证书",
    "Error while fetching MotD: ": "获取每日消息时出错：",
    "Error while checking for updates: ": "检查更新时出错：",
    "Error renaming stamp": "图章重命名失败",
    "A stamp with this name already exists.": "已存在同名图章。",
    "Could not rename the stamp.": "无法重命名图章。",
    "Nothing to migrate.": "没有需要迁移的数据。",
    "Nothing to migrate. This button is used to migrate data from pre-96.0 TPT installations to the shared directory": "没有需要迁移的数据。此按钮用于把 TPT 96.0 之前版本的数据迁移到共享目录。",
    "Downloaded scripts": "已下载脚本",
    "Migration complete. Results: ": "迁移完成。结果：",
    "Save format from newer version": "存档格式来自较新版本",
    "Invalid save format": "存档格式无效",
    "No data": "没有数据",
    "Cannot allocate memory": "无法分配内存",
    "Save error, out of memory": "存档处理失败：内存不足",
    "Wrong type for ": "类型错误：",
    "Wrong size for ": "大小错误：",
    "Incorrect CELL size": "CELL 尺寸不正确",
    "Save is of invalid size": "存档尺寸无效",
    "Save extends beyond canvas": "存档内容超出画布范围",
    "Save data too large, refusing": "存档数据过大，已拒绝加载",
    "Cannot decompress: status ": "无法解压，状态码：",
    "BSON error when parsing save: ": "解析存档时出现 BSON 错误：",
    "Save from a newer version: Requires version ": "存档来自较新版本，需要版本 ",
    "No save data": "没有存档数据",
    "Unknown format": "未知格式",
    "Save too large": "存档过大",
    "Save data too large": "存档数据过大",
    "Save data corrupt (missing data)": "存档数据损坏（数据缺失）",
    "Cannot compress: status ": "无法压缩，状态码：",
    "ERROR - Details: ": "错误——详细信息：",
    "An unrecoverable fault has occurred, please report it by visiting the website below\\n\\n  ": "程序发生无法恢复的故障，请访问以下网站报告：\\n\\n  ",
    "An attempt will be made to save all of this information to ": "程序将尝试把以上信息保存到 ",
    " in your data folder.\\n": "（位于数据文件夹中）。\\n",
    "Please attach this file to your report.\\n\\n": "报告问题时请附上此文件。\\n\\n",
    "Version: ": "版本：",
    "Tag: ": "标签：",
    "Date: ": "日期：",
    "Stack trace not available\\n": "无法获取堆栈跟踪\\n",
    "Floating point exception": "浮点运算异常",
    "Program execution exception": "程序执行异常",
    "Unexpected program abort": "程序意外中止",
    "Unknown signal": "未知信号",
    "unhandled exception: ": "未处理的异常：",
    "Could not read file": "无法读取文件",
    "Could not open save file:\\n": "无法打开存档文件：\\n",
    "Could not open file": "无法打开文件",
    "Not a ptsave link": "不是有效的 ptsave 链接",
    "Invalid save link": "存档链接无效",
    "No Save ID": "缺少存档 ID",
}


ELEMENT_ALIASES = {
    "BANG": "BANG",
    "BIZRG": "BIZG",
    "BIZRS": "BIZS",
    "BREC": "BREL",
    "CBNW": "BUBW",
    "GUNP": "GUN",
    "H2": "HYGN",
    "ICEI": "ICE",
    "IGNT": "IGNC",
    "INVIS": "INVS",
    "LNTG": "LN2",
    "LO2": "LOXY",
    "O2": "OXYG",
    "SHLD1": "SHLD",
    "SHLD2": "SHD2",
    "SHLD3": "SHD3",
    "SHLD4": "SHD4",
    "SPAWN": "SPWN",
}

# These literals are machine-facing protocol, preference, serialization or Lua
# API identifiers.  They must stay byte-for-byte compatible with upstream even
# when the same English word is translated in the UI.
PROTECTED_LITERALS_BY_PATH = {
    Path("src/PowderToy.cpp"): {"Fullscreen"},
    Path("src/client/User.cpp"): {"None"},
    Path("src/client/http/DeleteSaveRequest.cpp"): {"Delete"},
    Path("src/client/http/FavouriteSaveRequest.cpp"): {"Remove"},
    Path("src/client/http/GetSaveRequest.cpp"): {"Favourite"},
    Path("src/client/http/Request.cpp"): {"OK", "Error", "Error: "},
    Path("src/client/http/UnpublishSaveRequest.cpp"): {"Unpublish"},
    Path("src/client/http/UploadSaveRequest.cpp"): {"Publish"},
    Path("src/gui/game/Favorite.cpp"): {"Favorites"},
    Path("src/gui/options/OptionsModel.cpp"): {"Fullscreen"},
    Path("src/lua/LuaHttp.cpp"): {"OK"},
    Path("src/lua/LuaInterface.cpp"): {"Fullscreen"},
    Path("src/lua/LuaMisc.cpp"): {"display"},
    Path("src/lua/luascripts/compat.lua"): {"enabled", "Gravity", "Temperature"},
    Path("src/simulation/Element.cpp"): {"Gravity", "Temperature"},
}


def load_cache() -> dict[str, str]:
    if CACHE.exists():
        return json.loads(CACHE.read_text(encoding="utf-8"))
    return {}


def save_cache(cache: dict[str, str]) -> None:
    CACHE.write_text(
        json.dumps(cache, ensure_ascii=False, indent=2, sort_keys=True),
        encoding="utf-8",
    )


def protect(text: str) -> tuple[str, list[tuple[str, str]]]:
    replacements: list[tuple[str, str]] = []

    def repl(match: re.Match[str]) -> str:
        marker = f"ZXQTPT{len(replacements):03d}QXZ"
        replacements.append((marker, match.group(0)))
        return marker

    return TOKEN.sub(repl, text), replacements


def restore(text: str, replacements: list[tuple[str, str]]) -> str:
    for marker, value in replacements:
        compact = re.sub(r"\s+", "", marker)
        text = text.replace(marker, value).replace(compact, value)
    return text


def translate_batch(items: list[tuple[str, str]]) -> dict[str, str]:
    protected: list[tuple[str, str, list[tuple[str, str]]]] = []
    payload_parts = []
    for index, (key, source) in enumerate(items):
        value, tokens = protect(source)
        protected.append((key, source, tokens))
        payload_parts.append(f"TPTSEG{index:03d}ZZZ\n{value}")
    payload = "\n".join(payload_parts)
    query = urllib.parse.urlencode({
        "client": "gtx",
        "sl": "en",
        "tl": "zh-CN",
        "dt": "t",
        "q": payload,
    })
    request = urllib.request.Request(
        f"{GOOGLE_URL}?{query}",
        headers={"User-Agent": "Mozilla/5.0"},
    )
    last_error: Exception | None = None
    for attempt in range(4):
        try:
            with urllib.request.urlopen(request, timeout=45) as response:
                data = json.load(response)
            translated = "".join(part[0] for part in data[0])
            break
        except Exception as exc:  # network retries are intentional here
            last_error = exc
            time.sleep(1.5 * (attempt + 1))
    else:
        raise RuntimeError(f"translation request failed: {last_error}")

    matches = list(re.finditer(r"TPTSEG(\d{3})ZZZ\s*", translated))
    if len(matches) != len(items):
        raise RuntimeError(
            f"translation response lost segment markers: {len(matches)}/{len(items)}"
        )

    result: dict[str, str] = {}
    for pos, match in enumerate(matches):
        index = int(match.group(1))
        end = matches[pos + 1].start() if pos + 1 < len(matches) else len(translated)
        value = translated[match.end():end].strip("\r\n")
        key, source, tokens = protected[index]
        value = restore(value, tokens)
        leading = re.match(r"^\s*", source).group(0)
        trailing = re.search(r"\s*$", source).group(0)
        value = leading + value.strip() + trailing
        result[key] = value
    return result


def translate_all(values: set[str], cache: dict[str, str]) -> dict[str, str]:
    for source, target in OVERRIDES.items():
        cache[source] = target

    pending = []
    for source in sorted(values, key=lambda item: (len(item), item)):
        if source in cache:
            continue
        if len(source) == 1:
            cache[source] = source
            continue
        if source.startswith("DEFAULT_") or source.startswith("http") or source.startswith("/"):
            cache[source] = source
            continue
        if re.fullmatch(r"[A-Z0-9_.:/+%-]+", source):
            cache[source] = source
            continue
        pending.append(source)

    batches: list[list[str]] = []
    current: list[str] = []
    current_size = 0
    for source in pending:
        if current and (len(current) >= 20 or current_size + len(source) > 2200):
            batches.append(current)
            current = []
            current_size = 0
        current.append(source)
        current_size += len(source)
    if current:
        batches.append(current)

    for batch_index, batch in enumerate(batches, 1):
        keyed = [(source, source) for source in batch]
        cache.update(translate_batch(keyed))
        save_cache(cache)
        print(f"translated batch {batch_index}/{len(batches)} ({len(batch)} strings)")
        time.sleep(0.15)
    return cache


def candidate_strings(reference: Path) -> dict[Path, set[str]]:
    result: dict[Path, set[str]] = {}
    for path in ROOT.rglob("*"):
        if not path.is_file() or path.suffix.lower() not in {".cpp", ".h", ".lua"}:
            continue
        if any(part.startswith("build") for part in path.relative_to(ROOT).parts):
            continue
        try:
            original_text = path.read_text(encoding="utf-8")
        except UnicodeDecodeError:
            continue

        # The reference fork leaves a few secondary dialogs untranslated.
        # Explicit overrides must still be applied wherever their exact source
        # literal occurs, even when the corresponding reference line is unchanged.
        values = {
            match.group(0)[1:-1]
            for match in CPP_LITERAL.finditer(original_text)
            if match.group(0)[1:-1] in OVERRIDES
        }

        other = reference / path.relative_to(ROOT)
        if other.exists():
            try:
                original = original_text.splitlines()
                translated = other.read_text(encoding="utf-8").splitlines()
            except UnicodeDecodeError:
                translated = original
            if original != translated:
                for line in difflib.unified_diff(original, translated, n=0):
                    if not line.startswith("-") or line.startswith("---"):
                        continue
                    for match in CPP_LITERAL.finditer(line[1:]):
                        raw = match.group(0)[1:-1]
                        if re.search(r"[A-Za-z]", raw):
                            values.add(raw)
        if values:
            values.difference_update(
                PROTECTED_LITERALS_BY_PATH.get(path.relative_to(ROOT), set())
            )
        if values:
            result[path] = values
    return result


def fetch_element_descriptions() -> dict[str, str]:
    request = urllib.request.Request(
        ELEMENT_INDEX_URL,
        headers={"User-Agent": "Codex-localization"},
    )
    with urllib.request.urlopen(request, timeout=45) as response:
        markdown = response.read().decode("utf-8")
    result: dict[str, str] = {}
    for line in markdown.splitlines():
        match = re.match(
            r"^\|\s*([A-Z0-9]+)\s*\|\s*\d+\s*\|[^|]*\|\s*(.*?)\s*\|$",
            line,
        )
        if match:
            result[match.group(1)] = match.group(2).replace(",", "，")
    return result


def add_element_overrides(cache: dict[str, str]) -> None:
    descriptions = fetch_element_descriptions()
    descriptions["BANG"] = "TNT，一次性整体爆炸。"
    descriptions["SPAWN2"] = "火柴人 2 号出生点。"
    for path in (ROOT / "src" / "simulation" / "elements").glob("*.cpp"):
        text = path.read_text(encoding="utf-8")
        match = re.search(r'Description\s*=\s*"((?:\\.|[^"\\])*)";', text)
        if not match:
            continue
        source = match.group(1)
        code = ELEMENT_ALIASES.get(path.stem, path.stem)
        if code in descriptions:
            cache[source] = descriptions[code]


def clear_element_descriptions(cache: dict[str, str]) -> None:
    """Translate the exact upstream descriptions instead of replacing semantics."""
    for path in (ROOT / "src" / "simulation" / "elements").glob("*.cpp"):
        text = path.read_text(encoding="utf-8")
        match = re.search(r'Description\s*=\s*"((?:\\.|[^"\\])*)";', text)
        if match:
            cache.pop(match.group(1), None)


def apply(candidates: dict[Path, set[str]], translations: dict[str, str], dry_run: bool) -> None:
    changed_files = 0
    replacements = 0
    for path, values in candidates.items():
        text = path.read_text(encoding="utf-8")
        updated = text
        for source in sorted(values, key=len, reverse=True):
            target = translations.get(source, source)
            if target == source:
                continue
            target = re.sub(r'(?<!\\)"', r'\\"', target)
            old = f'"{source}"'
            new = f'"{target}"'
            count = updated.count(old)
            if count:
                updated = updated.replace(old, new)
                replacements += count
        if updated != text:
            changed_files += 1
            if not dry_run:
                path.write_text(updated, encoding="utf-8", newline="")
    print(f"{replacements} literal replacements in {changed_files} files")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--reference", type=Path, required=True)
    parser.add_argument("--dry-run", action="store_true")
    args = parser.parse_args()

    reference = args.reference.resolve()
    if not (reference / "src").is_dir():
        raise SystemExit(f"invalid reference tree: {reference}")

    candidates = candidate_strings(reference)
    all_values = {value for values in candidates.values() for value in values}
    print(f"found {len(all_values)} candidate literals in {len(candidates)} files")

    cache = load_cache()
    clear_element_descriptions(cache)
    cache = translate_all(all_values, cache)
    save_cache(cache)
    apply(candidates, cache, args.dry_run)


if __name__ == "__main__":
    main()
