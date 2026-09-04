#!/usr/bin/env python3
"""Extract the detailed Chinese element notes from TPT元素说明97.pdf.

The PDF describes TPT 96.2-era display codes, while this source tree uses a
few renamed C++ element files.  This script keeps the PDF wording as the
reference text, maps those old display codes to the current implementation,
and writes a deterministic JSON file consumed by
``rewrite_element_descriptions.py``.

Usage:
    python tools/extract_pdf97_element_descriptions.py PDF_PATH
"""

from __future__ import annotations

import argparse
import bisect
import json
import re
from pathlib import Path

from pypdf import PdfReader


# PDF pages 28--101 contain the element reference.  Page numbers here are
# one-based so they can be checked directly against a PDF viewer.
FIRST_ELEMENT_PAGE = 28
LAST_ELEMENT_PAGE = 101

HEADER_RE = re.compile(r"(?m)^[^\n]{0,500}?Type[:：]\s*\d{1,4}[^\n]*$")
CODE_RE = re.compile(r"\(([A-Z][A-Z0-9-]{1,7})\)")

# Current source filename -> display code used by the PDF.  Many of these are
# internal renames only; the in-game four-letter code is still the old name.
SOURCE_TO_PDF = {
    "BANG": "TNT",
    "BHOL": "VACU",
    "BIZRG": "BIZG",
    "BIZRS": "BIZS",
    "C5": "C-5",
    "CBNW": "BUBW",
    "EQUALVEL": "EQVE",
    "GUNP": "GUN",
    "H2": "HYGN",
    "HFLM": "CFLM",
    "ICEI": "ICE",
    "IGNT": "IGNC",
    "INVIS": "INVS",
    "LIFE": "GOL",
    "LNTG": "LN2",
    "LO2": "LOXY",
    "NBHL": "BHOL",
    "NWHL": "WHOL",
    "O2": "OXYG",
    "PLEX": "C-4",
    "SHLD1": "SHLD",
    "SHLD2": "SHLD",
    "SHLD3": "SHLD",
    "SHLD4": "SHLD",
    "SPAWN": "SPWN",
    "SPAWN2": "SPWN2",
    "STKM2": "STK2",
    "WHOL": "VENT",
    "WIRE": "WWLD",
}


# The PDF text layer contains a small number of transcription/OCR mistakes and
# two mathematical glyphs that are absent from the verified reference font.
# Keep the corrections here so regenerating the JSON cannot reintroduce them.
OCR_TOKEN_REPLACEMENTS = {
    "ATMR": "AMTR",
    "BRAYA": "BRAY",
    "BRTM": "BRMT",
    "CLFM": "CFLM",
    "ENUT": "NEUT",
    "GAUS": "CAUS",
    "HYGH": "HYGN",
    "IMWR": "INWR",
    "PTOH": "PHOT",
    "QRYN": "QRTZ",
    "RPTI": "PRTI",
    "RQRT": "PQRT",
    "SADN": "SAND",
    "UNG": "TUNG",
    "WARE": "WATR",
    "WATA": "WATR",
}


def normalise_description(text: str) -> str:
    """Fix known PDF text-layer damage and use only verified font glyphs."""

    text = text.replace("⩽", "≤").replace("⩾", "≥")
    text = text.replace("ºC", "℃").replace("°C", "℃")
    text = text.replace("（", "(").replace("）", ")")
    text = text.replace("Llfe", "Life")
    for wrong, correct in OCR_TOKEN_REPLACEMENTS.items():
        text = re.sub(
            rf"(?<![A-Z0-9]){re.escape(wrong)}(?![A-Z0-9])",
            correct,
            text,
        )

    # Remove visual-layout spaces accidentally embedded inside Chinese words,
    # while retaining useful spacing around Latin element codes and values.
    text = re.sub(r"(?<=[\u3400-\u9fff]) +(?=[\u3400-\u9fff])", "", text)
    text = re.sub(r"\( +(?=[A-Z0-9])", "(", text)
    text = re.sub(r"(?<=[A-Z0-9]) +\)", ")", text)
    text = re.sub(r" +([，。；：、）])", r"\1", text)
    text = re.sub(r"([，。；：、]) +", r"\1", text)
    text = re.sub(r"（ +", "（", text)

    # Normalise technical notation that the PDF mixes between English,
    # Chinese and several OCR spellings.  Keeping one spelling prevents
    # fragments such as "T mp", "295.15k" and "10Pressure" from looking
    # like mojibake in the full-description dialog.
    text = re.sub(r"\bT\s+mp\b", "Tmp", text)
    text = re.sub(r"\bTem\s+p\b", "Temp", text)
    text = re.sub(r"\bC\s+type\b", "Ctype", text)
    text = re.sub(r"\bL\s*ife\b", "Life", text)
    text = text.replace("Cype", "Ctype")
    text = text.replace("Stamps/Saves", "图章/存档")
    text = text.replace("Snapshot54", "54.0 版本")
    text = text.replace("WiFi", "WIFI")
    text = text.replace("Pressure", "P")
    text = text.replace("no eff", "无变化")
    text = text.replace("QRTZ scat", "QRTZ 散射")
    text = re.sub(r"MoltenAAAA", "熔融材料名", text)
    text = re.sub(r"Molten([A-Z][A-Z0-9-]*)", r"熔融 \1", text)
    text = re.sub(r"(?<=\d)k\b", "K", text)
    text = re.sub(r"(?<=\d)C\b", "℃", text)
    text = re.sub(r"(?<![A-Za-z0-9])([1-9][0-9]*)x(?=\d)", r"\1×", text)
    text = re.sub(r"(?<![A-Za-z0-9])([0-9]+)x(?=\s*[A-Z])\s*", r"\1×", text)
    text = re.sub(r"(?<=\d)Tmp\b", " Tmp", text)
    text = re.sub(r"(?<=\d)P\b", " P", text)
    text = re.sub(r"\bTemp\(℃\)/10=vX=vY\b", "温度(℃)/10=Vx=Vy", text)
    text = text.replace("(NEUT）", "(NEUT)")
    text = text.replace("(WHOL)", "(WHOL)").replace("(BHOL)", "(BHOL)")

    # Undo hard line wraps that split a Chinese word.  Real fields retain
    # their newline because they begin with a labelled heading.
    field_names = (
        "描述|制取|产生|特性|性质|能力|控制|限制|参数|元素参数|反应|"
        "内在反应|熔点|沸点|凝固点|燃点|压力极限|导热率|初始温度|"
        "频道|用法|穿透|破坏|合成|吸水|遗传|特殊转化|热力过程|"
        "参数获取|稀释与相变|主要反应|绝缘"
    )
    text = re.sub(
        rf"(?<=[\u3400-\u9fff])\n(?!(?:{field_names})\s*[：:])(?=[\u3400-\u9fff])",
        "",
        text,
    )

    # Clear a few unmistakable sentence-level extraction glitches.
    text = text.replace("V96.2 以后", "96.2 版本以后")
    text = text.replace("然后它就会中消失", "然后它就会消失")
    text = text.replace("压力))K]", "压力)K]")
    text = text.replace("接收。特点：", "接收。\n特点：")
    text = text.replace("100%。在超过", "100%。\n在超过")
    text = text.replace(
        "INWR,PSCN,NSCN--INWR 与这些元素互相传导SWCH,WIFI-INWR 传导至这些元素，但不从这些元素传导",
        "双向传导：INWR、PSCN、NSCN。\n单向输出：INWR 可向 SWCH、WIFI 传导，但不会从它们接收。",
    )
    text = text.replace(
        "WTRV + BCOL → OIL SHLD 进入下一阶段并在接触 PTNM 时立即增长以下 3 种反应基于二次概率曲线发生，从<=0C 时的 0%到 1500℃时的 100%几率",
        "WTRV+BCOL→OIL\nSHLD 接触 PTNM 时立即进入下一阶段。\n以下 3 种反应按二次概率曲线发生：0℃及以下为 0%，1500℃时达到 100%。",
    )
    text = text.replace(
        "SMKE → CO2氢反应当 HYGN 和另一个元素都在附近时，会发生反应。",
        "SMKE→CO2\n氢反应：当 HYGN 和另一个反应物都在附近时，会发生反应。",
    )
    text = text.replace(
        "DSTW + SPRK+(5℃)，PTNM 被触发两个 HYGN 在 500℃以上",
        "DSTW+SPRK，并升温 5℃，同时激活 PTNM。\n两个 HYGN 在 500℃以上",
    )
    text = text.replace(
        "铀用于核反应堆 IRL 以产生热量以产生蒸汽",
        "现实中的核反应堆利用铀产生热量和蒸汽",
    )
    text = text.replace("液化点：371℃/97.85k", "液化点：97.85℃/371K")
    return text


# The PDF's text layer contains several tables and multi-column layouts that
# cannot be reconstructed reliably by linear extraction.  These entries are
# manually structured from the rendered PDF pages and checked against the
# current implementation.  This keeps the original level of detail without
# exposing table debris, placeholders or sentence fragments to players.
PDF_DESCRIPTION_OVERRIDES = {
    "EXOT": """描述：奇异物质(EXOT)是一种特殊液体，具有类似岩浆(LAVA)的密度和压力特性，也有部分冰(ICE)的性质。冷却后会凝固，并以常温状态一半的速度闪烁。未受电子激发时，低于常温会产生负压，高于常温会逐渐释放正压；受到大量电子冲击时可能猛烈爆炸。
产生：让电渣(BREL)持续通电，在最大压力下加热至 9000℃以上。
与电子的反应：ELEC 会使 EXOT 发出彩虹色光并产生压力，压力强度与电子撞击数量有关。当环境压力与 EXOT 内部压力相等时停止增压。电子撞击累计超过 Tmp2+1000 后，EXOT 会转化为具有极高温度和压力的迁跃粉(WARP)。
与中子的反应：NEUT 会使 EXOT 快速褪色但保留闪烁；中子过多时，EXOT 会复制与其直接接触的物质，光子、中子、电子和墙除外。
元素参数：Tmp 控制闪烁循环；Tmp2 表示最大辐射承受量。
导热率：250
初始温度：20℃/293.15K""",
    "FILT": """描述：滤镜(FILT)可改变光子(PHOT)和射线(BRAY)的波长与颜色。Ctype 用低 30 位保存波长数据；0x3FFFFFFF 或 -1 表示全部波长，显示为白色。Ctype=0 时由温度自动计算颜色：低温偏蓝，高温偏红，从 0℃起每升高约 40℃红移一位，1000℃附近达到红端。
颜色结构：30 位分为红、黄、绿、青、蓝五组，长度依次为 9、3、6、3、9 位；显示颜色取决于各组中有效位的比例。FILT 不受环境热辐射影响，只与接触物交换热量。
常见用途：FILT 导热率很高且不易损坏，可传递热量；与 ARAY 组合时可给 BRAY 着色、存储数据和执行逻辑运算。棕色 BRAY 或沿路径放置的透明粒子可清除旧射线，避免下一次 SPRK 周期受到干扰。
操作模式(Tmp)：
0 设置：把进入粒子的波长改为 FILT 的波长。
1 过滤：原波长与 FILT 波长执行按位与。
2 增加：执行按位或，加入 FILT 的波长。
3 删除：从原波长中清除 FILT 对应的位。
4 红移：按温度决定的位数向红端移动。
5 蓝移：按温度决定的位数向蓝端移动。
6 透明：不改变波长。
7 异或：原波长与 FILT 波长执行按位异或。
8 反色：30 位范围内按位取反；全白输入会被吸收。
9 散射：模拟石英(QRTZ)的随机波长扰动。
10 可变红移：按 FILT 最低有效位决定红移量。
11 可变蓝移：按 FILT 最低有效位决定蓝移量。
其他 Tmp 值：按模式 0 处理。
逻辑说明：每一位都可独立参与运算，因此一个 FILT 像素可保存 30 位数据。若运算结果为 0，BRAY 会终止且不会输出 SPRK；实际电路常保留第 30 位作为存在标记，防止有效的零值被误判为没有数据。移位超出 30 位范围的数据会丢失。
存储方式：简易存储是每个 FILT 保存一个值；参考存储用较小索引代替大数值；共享存储把多个短字段移入同一个 30 位值。DTEC 可把范围内 PHOT/BRAY 的 Ctype 写入相邻 FILT；LDTC 还能读取 PHOT、BRAY 或 FILT，并把值写到检测方向另一侧的 FILT。
导热率：251
初始温度：22℃/295.15K""",
    "FRAY": """描述：动力射线发射器(FRAY)通电后沿电流方向寻找可移动粒子并施加速度，方向判定与 ARAY 相同。目标温度高于 FRAY 时会被吸引，低于 FRAY 时会被推开；也能影响光子、中子等能量粒子。
控制：FRAY 不导热，但可用升温笔(HEAT)和降温笔(COOL)设置自身温度，从而改变作用方向和强度。
元素参数：Tmp 表示一次最多处理的粒子数；Tmp=0 时最多处理 10 个粒子。
导热率：0
初始温度：22℃/295.15K""",
    "H2": """描述：氢气(HYGN)可被火焰(FIRE)点燃，并与氧气(OXYG)燃烧生成水蒸气(WTRV)。它自身不产生气压，因此在低温下可接触石英(QRTZ)而不使石英因压力破碎。
与柴油的反应：HYGN 压力大于 8 P 且接触 DESL 时，两者分别转化为 OIL 和 WATR。DESL 在压力超过 5 P 时会先变成 FIRE，因此需要迅速完成反应，或用 TTAN 隔绝 DESL 所受压力，只给 HYGN 加压。
聚变：约 2000℃、50 P 时，HYGN 可聚变为惰性气体(NBLE)，同时产生 PLSM、NEUT、黄色 PHOT，以及 1 至 2 个 NBLE；另有 10% 概率产生 ELEC。反应会释放约 50 P 压力并把温度提高到约 4000℃。
产生：NEUT+ELEC→HYGN。
导热率：251
初始温度：22℃/295.15K""",
    "IRON": """描述：铁(IRON)会被盐(SALT)、盐水(SLTW)、氧气(OXYG)、水(WATR)和液氧(LOXY)腐蚀，并逐步转化为脆金属(BMTL)，可用于电解水相关装置。
腐蚀过程：IRON 长时间接触上述物质后先变成 BMTL，继续暴露会变成金属粉(BRMT)，表示进一步锈蚀。附近放置 GOLD 可逆转并阻止这一过程。
熔点：1413.85℃/1687K
导热率：251
初始温度：22℃/295.15K""",
    "LAVA": """描述：熔岩(LAVA)表示各种材料的熔融状态，外观相同，但 Ctype 记录原材料。冷却后通常恢复为 Ctype 对应的固体；核反应也可能生成熔融物。
HUD 显示：尚未生成过的组合会显示为“熔融+材料名”。即使名称没有明确写出“熔岩”，粒子类型仍是 LAVA。用控制台改变 Ctype 可以制造熔融火柴人、熔融水等特殊组合，但这些组合不一定具有正常相变行为。
高温可熔融物：除 BTRY、INST、WWLD 外的大多数电子元件；除 SNOW、BREL、ANAR、GRAV、FRZZ、BCOL、FSEP、YEST、DUST 外的大多数粉末；以及 BMTL、GLAS 等固体。
导热率：60
初始温度：1522℃/1795.15K""",
    "METL": """描述：基础金属导体(METL)，可传导电脉冲并熔化。SPRK 通过时会把它加热到约 300℃并产生少量压力；达到熔点后变成 Ctype=METL 的熔岩(LAVA)，冷却后重新凝固为 METL。
制取：把熔融铁(IRON)倒在煤(COAL)或煤粉(BCOL)上，再冷却即可得到 METL。
熔点：999.85℃/1273K
导热率：251
初始温度：22℃/295.15K""",
    "NEUT": """描述：中子(NEUT)不受普通重力影响，但会受牛顿引力场作用。它可由钚(PLUT)或重水(DEUT)裂变产生；SNOW 和 ICE 会使其减速，GOLD 会吸收少量中子，TTAN 在反射时吸收约 5%，MERC 会完全吸收接触的中子。
主要反应：
PLUT、DEUT：触发裂变。
GUNP→DUST；PLNT→WOOD；DUST→FWRK。
NITR→GAS；C-4→GOO；WATR→DSTW；ACID→ISOZ。
DESL→GAS；YEST→DYST；COAL→WOOD；BCOL→SAWD。
RFRG→CAUS 或 GAS。
存在时间(Life)：随机值，不超过约 1000 帧。
导热率：60
初始温度：22℃/295.15K""",
    "O2": """描述：氧气(OXYG)是高度助燃的气体，可被 FIRE 点燃；低温或压力超过 100 P 时会液化成液氧(LOXY)。植物(PLNT)吸收 SMKE 或 CO2 时可产生 OXYG，用于模拟光合作用。OXYG 与 BOYL 反应会产生 WTRV 和约 4 P 压力。
聚变：在极强牛顿重力、9700℃以上高温及 250 P 以上压力下，OXYG 会聚变为熔融脆金属(BMTL)，并各产生一个 PHOT、PLSM 和 GRVT。
液化点：-183.15℃/90K
导热率：70
初始温度：22℃/295.15K""",
    "PIPE": """描述：动力管(PIPE)可沿固定方向运输物质，放置后会在周围自动生成一圈砖块(BRCK)。管内粒子仍保留真实类型与 Ctype；例如“熔融 PSCN”在类型上仍是 LAVA。
使用方法：放置 PIPE 后，先擦除预定出口处的 BRCK，让管道开始形成；完全形成后再擦除入口端 BRCK。成形完成后可以移除其余 BRCK，换成其他材料。可用于运输系统或单向门。
压力极限：10 P，超过后变成金属粉(BRMT)。
导热率：0
初始温度：0℃/273.15K""",
    "PROT": """描述：质子(PROT)不能穿透 INSL、VOID/PVOD、DMND、VIBR、墙等屏障，并会清除碰到的电脉冲。离开其他物质内部后会缓慢衰减，通常约 680 帧消失。
温度作用：温度超过 500℃时可引爆爆炸物。进入不导热材料时，若质子更热，会把该材料加热到自身温度，例如 CRAY、PRTI、PRTO。对 WIFI：质子高于 200℃时使频道温度升高 1000℃；100℃至 200℃升高 100℃；-100℃至 0℃降低 100℃；-200℃至 -100℃降低 1000℃。
质子对撞：只有运动方向几乎相反的两个质子才算对撞，产物由两者速度平方和决定：
大于 4250→SING；大于 275→PLUT；大于 170→URAN；大于 100→PLSM；大于 40→OXYG；大于 20→CO2；大于 10→NBLE；不超过 10 时不反应。
其他反应：PROT+ELEC→HYGN；PROT+INVS→NEUT；PROT+LCRY→PHOT；PROT+POLO→PLUT；与 DEUT 反应可产生更多质子；碰到 EXOT 会持续使其降温并最终生成 CFLM。
导热率：61
初始温度：22℃/295.15K""",
    "PTNM": """描述：铂(PTNM)可催化多种反应，导电速度与 GOLD 相近，但熔点更高，可在高温电路中代替 GOLD。反应物必须直接接触 PTNM。
直接催化：ISZS/ISOZ→PLUT+PHOT；WTRV+BCOL→OIL；SHLD 接触 PTNM 后立即进入下一防护层级。
温压催化：下列反应按二次概率曲线发生，0℃及以下概率为 0%，1500℃时达到 100%。压力超过 2 P 且温度超过 200℃时，GAS→INSL 并升温 60℃；压力超过 50 P 且温度超过 1000℃时，BREL→EXOT 并降温 30℃；SMKE→CO2。
氢反应：HYGN+DESL→OIL+WATR；HYGN+OXYG→DSTW+SPRK，升温 5℃并激活 PTNM。两个 HYGN 在 500℃以上时，每帧约有 1/1000 概率发生冷聚变，生成 NBLE、NEUT、PHOT，并有 1/10 概率额外生成 ELEC，同时产生约 1000℃高温和 10 P 压力。
导热率：251
初始温度：22℃/295.15K""",
    "ROCK": """描述：岩石(ROCK)是坚固固体，可作为混凝土(CNCT)的地基；CNCT 堆在 ROCK 上时不会从边缘滑落。ROCK 耐酸(ACID)、耐破坏炸药(DEST)，但仍会被高速水流缓慢侵蚀：水与周围速度差大于 0.5 时，每帧约有 1/1000 概率把 ROCK 变成 SAND(33%)或 STNE(67%)。
熔融反应：多数反应只发生在熔融 ROCK 上。压力至少 25 P 时，每帧约有 1/12500 概率转化：
25-50 P：BRMT 50%，CNCT 50%。
50-73 P：QRTZ。
73-75 P：GOLD 12.5%，QRTZ 87.5%。
75-100 P 且温度至少 4726.85℃：TTAN 20%，IRON 80%。
100 P 以上且温度至少 4726.85℃：另有 20% 概率生成放射性熔融物，分布为 URAN 20%、PLUT 16%、TUNG 64%。
导热率：200
初始温度：22℃/295.15K""",
    "STNE": """描述：石头(STNE)是重粉末，加热后熔化为 LAVA。
产生：冷却 LAVA，或对砖块(BRCK)施加足够压力。
反应：ROCK 与 WATR 反应可生成 STNE。熔融 SLCN 与 OXYG 反应时有 1/3 概率生成 STNE，同时还会在 SAND、CLST/PQRT 等产物之间分配。
熔点：709.85℃/983K
导热率：150
初始温度：22℃/295.15K""",
    "THDR": """描述：球状闪电(THDR)是温度约 9000℃的带电类液体粒子，运动不受空气压力影响。接触物质时会产生约 256 P 的强烈压力冲击波，并把高温传给非金属或刚结束 SPRK、暂时不能导电的金属。
用途：可提供启动聚变所需的瞬时压力和热量，例如与 HYGN 配合。但其导热率极低，不适合持续加热。
导热率：1
初始温度：9000℃/9273.15K""",
    "THRM": """描述：铝热剂(THRM)只能由 FIRE、PLSM、LAVA 或 LIFE 点燃，燃烧时可达到约 3000℃。反应后的熔融铝热产物冷却会生成脆金属(BMTL)。THRM 密度较大，会沉入大多数液体和部分粉末。
产生：将金属粉(BRMT)与电渣(BREL)一起加热到 250℃/523.15K 以上。
导热率：211
初始温度：22℃/295.15K""",
    "VIBR": """描述：振金(VIBR)可吸收并储存热量、压力和能量粒子。能量增加时颜色由深绿逐渐变亮；达到极限后发出绿光并快速白色闪烁，向所有直接相连的导体输出 SPRK、释放热量，并在约 750 帧后爆炸。BOMB 不会直接摧毁 VIBR，只会少量增加其 Tmp。
能量换算：温度每偏离 0℃约 3℃增加 1 Tmp，并被限制在约 -2.5℃到 2.5℃；正压每 1 P 增加 7 Tmp，负压每 1 P 增加 2 Tmp，压力会被拉回 0 P 附近；每吸收 20 个能量粒子(包括 GRVT)增加 1 Tmp。
产生：把 EXOT 与熔融 TTAN 混合可得到熔融 VIBR；清除剩余 EXOT 后冷却即可凝固为 VIBR。
其他反应：接触 EXOT 会转化为 EXOT；接触 ANAR 会变成振金粉(BVBR)并产生负压；达到能量极限时用冷焰(CFLM)灼烧会短暂变蓝，之后恢复。
元素参数：Tmp 表示已吸收的能量总量。
导热率：251
初始温度：0℃/273.15K""",
    "WATR": """描述：普通水(WATR)可以导电。蒸馏水(DSTW)接触多数杂质后会变成 WATR；植物(PLNT)能吸收它生长。NEUT 穿过水时会逐步把 WATR 转成 DSTW，同时自身减速并可能被吸收。
沸点：99.85℃/373K
凝固点：0℃/273.15K
相变：温度达到 99.86℃+2×压力时变成 WTRV 并增加约 0.5 P；温度不高于 -0.01℃时，压力至少 0.8 P 生成 SNOW，否则生成 Ctype=WATR 的 ICE。
产生：BOYL+OXYG→WATR；低压 HYGN+DESL→WATR+OIL；BUBW 放置一段时间后→WATR+CO2；SPNG 可释放已吸收的 WATR；RIME 在 0℃以上→WATR。
电与爆炸：SPRK 可在 WATR 中缓慢传导。WATR 可熄灭普通 FIRE；接触 LRBD/RBDM 时生成 WTRV 和高温 FIRE。
与气体和液体：CO2→BUBW；BOYL→FOG；DSTW 会被污染成 WATR；与 SLTW 混合得到更多 SLTW；与 GLOW 逐步生成 DEUT；与 FRZW 接触会转为 FRZW；GEL、SPNG 可吸水并增加相应储水参数。
与粉末和固体：SALT→SLTW；FRZZ→FRZW；CLST→PSTE；PLNT 吸收后生长；IRON 被水腐蚀为 BMTL。
与辐射：NEUT 使 WATR→DSTW 并减速；ELEC 作用后生成 HYGN 和 OXYG，同时电子被反射。
导热率：29
初始温度：22℃/295.15K""",
}


# Elements added after the PDF's 96.2 snapshot.  These notes are written from
# the current 100.0.399 implementation and intentionally follow the PDF's
# structure and level of detail instead of falling back to a one-line summary.
SUPPLEMENTAL_DESCRIPTIONS = {
    "ANIM": """描述：动画液晶，每个粒子都能为多个动画帧保存独立装饰色；即使关闭普通装饰显示，也会用当前帧颜色绘制。可用动画工具新增、复制、切换或删除帧，再用装饰工具逐帧作画。
控制：PSCN 电脉冲开始自动播放，NSCN 停止并复位。Life=10 时播放；Tmp2 是当前帧，Ctype 是最后一帧编号。温度的摄氏数值用作换帧间隔，例如 22℃约每 22 帧换一次；0℃及以下会每帧前进。
限制：无效或缺失的帧数据会使对应粒子消失；大量动画液晶会增加存档和内存占用。
导热率：0
初始温度：22℃/295.15K""",
    "BASE": """描述：碱性腐蚀液体，Life 表示浓度，范围 1～100，默认 76；相邻 BASE 会交换浓度。颜色随浓度改变。它会腐蚀低硬度材料，并把多数导电固体氧化成可破坏金属(BMTL)，反应时消耗自身浓度。
稀释与相变：WATR、DSTW、BUBW 会稀释并分出新的 BASE。低于 0℃－Life/4 的温度时冻结成 Ctype=BASE 的 ICE；压力低于 10P 且温度高于 120℃时，稀溶液会缓慢蒸发成 BOYL，或因失水而提高浓度。
主要反应：浓度不低于酸时，BASE+ACID→SLTW×2；BASE+CAUS→SLTW，并消耗 CAUS。浓度≥70 时 BASE+OIL→SOAP；BASE+GOO→GEL；BASE+BCOL→GUNP。压力≥10P 时与熔融 ROCK 有小概率生成 MERC。温度高于 50℃时，中子还可能把 BASE 转为 LRBD。
限制：SALT、SLTW、BOYL、MERC、BMTL、BRMT、SOAP、复制体，以及由 BASE 形成的冰雪等不会被普通腐蚀逻辑处理。
导热率：31；热容量：1.5
初始温度：22℃/295.15K""",
    "BUTN": """描述：可通电的触发按钮。PSCN 使相连的 BUTN 开启，NSCN 关闭；开启时呈青色发光状态，并一直保持到收到关闭信号。
用法：按钮开启后，在它上面绘制任意非 BUTN 元素，或让普通导体的电脉冲接触它，BUTN 会向电路输出一次 SPRK。适合在触屏上制作需要人工点击/绘制触发的电路。
绝缘：INSL 和 RSSS 可阻隔按钮信号。
导热率：251
初始温度：22℃/295.15K""",
    "EXPL": """描述：连锁爆炸粒子，是会移动的高亮粉末。它检查周围 3×3 范围，并给所有非不可破坏、非 EMBR 的相邻粒子设置爆炸标记，使这些粒子按各自的爆炸规则被摧毁或引爆。
特性：EXPL 自身具有不可破坏属性，不靠燃烧或倒计时工作，因此一小团即可沿可破坏材料持续传播爆炸；不可破坏材料能阻止传播。它不会把余烬(EMBR)标记为爆炸。
导热率：29
初始温度：20℃/293.15K""",
    "INDI": """描述：不可破坏绝缘体，常态下不导电、不导热，也不受普通燃烧、腐蚀和爆炸破坏，适合永久电路隔离和极端环境容器。
相变：压力高于 10P 时会变成损坏电子元件(BREL)；温度达到 999.85℃/1273K 时变为 LAVA。因此“不可破坏”不等于无视这两项明确的压力和温度转化。
导热率：0
初始温度：22℃/295.15K""",
    "MOVS": """描述：移动固体，游戏内显示为 BALL。相连粒子组成一个可整体平移、碰撞和旋转的刚体，碰到墙、普通固体或另一组 BALL 时会反弹；开启“移动固体旋转”后，碰撞还会改变角速度。
限制：最多同时建立 255 组移动固体，同一像素不能新建一组。控制中心丢失后，剩余粒子会逐渐瓦解；中心承受超过约±10P 的压力会被摧毁，整体在约±25P 的压力下也会消失。
性质：重力 0.1，重量 85，硬度 30。
导热率：70
初始温度：22℃/295.15K""",
    "PINV": """描述：可控隐形材料。PSCN 开启整片 PINV，NSCN 关闭；开启状态(Life≥10)允许普通粒子进入并穿过，关闭时成为不可破坏的固体屏障。开启时颜色变为半透明紫色。
穿透：中子和光子可穿过 PINV。普通粒子穿过时可以与 PINV 占据同一像素，内部粒子由专用槽保存；关闭后不再允许新的普通粒子进入。
特性：不导热，不会因普通压力或温度发生相变。
导热率：0
初始温度：22℃/295.15K""",
    "PPTI": """描述：可控传送门入口，是 PRTI 的通电版本。PSCN 开启相连区域，NSCN 关闭；开启时产生轻微负压，把接触的物质、能量粒子和电脉冲存入对应频道，等待出口释放。
频道：频道由温度决定，只有温度对应的 PPTI/PRTI 与 PPTO/PRTO 才互通。入口表面积越大，吸收速度越高；频道暂时没有出口时可保存有限数量的粒子。
限制：关闭时不执行入口传送；不导热。
导热率：0
初始温度：22℃/295.15K""",
    "PPTO": """描述：可控传送门出口，是 PRTO 的通电版本。PSCN 开启相连区域，NSCN 关闭；开启时从温度对应的频道释放 PPTI/PRTI 收集的物质、能量粒子和电脉冲，并产生轻微正压。
频道：必须与入口保持相同温度才能互通。出口周围需要有空位；出口面积越大，可同时释放的粒子越多。没有可用粒子或没有空间时会等待。
限制：关闭时不释放频道内容；不导热。
导热率：0
初始温度：22℃/295.15K""",
    "PWHT": """描述：可控洪泛属性写入器，默认用自身温度一次性修改紧贴其上方、彼此连通的整片粒子；PSCN 开启，NSCN 关闭。它不是逐点传热，因此一个 PWHT 就能瞬间处理大面积连通区域。
参数：Ctype=0 写入温度；1=Life，2=Ctype，3=Type，4=Tmp，5=Tmp2，6=Vy，7=Vx，8=X，9=Y，10=装饰色，11=Flags，12=Tmp3，13=Tmp4。写入浮点属性时数值取 PWHT 温度；普通整数属性取 PWHT 的 Tmp；装饰色取 PWHT 自身装饰色。
放置限制：PWHT 正上方和正下方不能紧邻另一个 PWHT。修改 Type、坐标或标志等底层属性可能破坏结构，应先在副本中测试。
导热率：0
初始状态：Life=10（开启）；初始温度 22℃/295.15K""",
    "RAZR": """描述：极重的银色致命粉末，重力和惯性都很强。移动时能挤开或穿过绝大多数可移动材料，尤其可穿过 CNCT 和 GEL，因此常用来切割、压碎或快速清理粉末与液体。
性质：重力 1.5，重量 500；不燃烧、不熔化，也没有普通高低温或高低压相变。它仍会被不能移动的墙体和特殊不可破坏结构阻挡。
导热率：50
初始温度：22℃/295.15K""",
    "RFGL": """描述：液态制冷剂，是 RFRG 在较高压力下形成的液相。压力低于 2P 时转回气态 RFRG；气态 RFRG 压力升到 2P 时凝结为 RFGL，由此可构成压缩—膨胀制冷循环。
热力过程：RFRG 会按新旧绝对压力比例改变温度，近似公式为 T新=T旧×(P新+257)/(P旧+257)：压缩升温、膨胀降温。RFGL/RFRG 导热很慢，便于把温差带到换热端。
反应：中子撞击气态 RFRG 时，等概率转成 GAS 或 CAUS。RFGL 和 RFRG 都具有致命属性，火柴人应避免接触。
导热率：3
初始温度：22℃/295.15K""",
    "RSSS": """描述：固态抗性材料。它像 TTAN 一样阻挡空气和压力传播，并在电路判定中像 INSL 一样隔断 SPRK；同时允许中子进入，不会主动导电。
转化：中子与 RSSS 占据同一像素时会被吸收，并把 RSSS 液化。若 Ctype 有效，就转为 Ctype 指定元素；否则默认转为液态 RSST。若目标元素能携带 Ctype，则可用 Tmp 指定其 Ctype。
参数获取：邻近 CLNE/PCLN 时复制其 Ctype；邻近 BCLN/PBCN 时把其 Ctype 复制到 Tmp。GRVT 进入 RSSS 后每帧有 1/5 概率被吸收。
导热率：130
初始温度：22℃/295.15K""",
    "RSST": """描述：液态抗性材料，能导电，并允许光子和中子穿过。光子与 RSST 占据同一像素时会被吸收并使其固化：Ctype 有效则转为指定元素，否则默认变成 RSSS；Tmp 可继续指定目标元素的 Ctype。
破坏：电子(ELEC)与 RSST 同像素时两者一起消失；SPRK 可在 RSST 中传播，但一个电火花周期结束后该 RSST 会消失。
合成：RSST+GUNP→FIRW；RSST+BCOL→FSEP(Life=50)。邻近 CLNE/PCLN 时读取 Ctype，邻近 BCLN/PBCN 时读取 Tmp。
导热率：55
初始温度：42℃/315.15K""",
    "SEED": """描述：带遗传参数的种子。它需要处在 5～70℃、重力下方紧贴 SAND、上方有空位，并储存超过 3 份水；条件连续维持约 200 帧后变成 PLNT，最初枝条长度随储水量增加。
吸水：WATR 最多把储水提高到 31；DEUT 可提高到 255；DSTW 和 BUBW 只在储水低于 3 时少量吸收。SLTW 有害，会消耗一份储水。超出适温范围会清空储水并重置发芽计时。
遗传：两个尚未杂交且有水的相邻 SEED 每帧有 1/10 概率杂交，后代交换颜色、分枝和生长阶段基因；中子命中时会随机翻转一项基因。
特殊转化：温度高于 46.85℃/320K 且压力超过 50P 时，每帧有 1/150 概率变成 MWAX；达到 400℃/673.15K 时直接燃烧成 FIRE。
导热率：32
初始温度：22℃/295.15K""",
}


def clean_segment(segment: str) -> str:
    """Remove page furniture while retaining the PDF's useful paragraphs."""

    segment = segment.replace("\u00a0", " ").replace("\r", "")
    # A new category heading means the previous element is finished.  Text
    # between that heading and the first element of the new category is an
    # introduction, not part of the preceding element.
    segment = re.split(r"(?m)^[^\n]*-{8,}[^\n]*$", segment, maxsplit=1)[0]
    # Examples are bitmap figures in the PDF.  Their captions without the
    # figures are not useful inside the game and can accidentally include
    # text extracted from a following illustration.
    segment = re.split(r"(?m)^\s*示例\s*[：:]", segment, maxsplit=1)[0]
    lines: list[str] = []
    for raw_line in segment.splitlines():
        line = re.sub(r"[ \t]+", " ", raw_line).strip()
        if not line or re.fullmatch(r"-?\s*\d{1,3}\s*-?", line):
            continue
        # Category headings can appear at the end of a page after the final
        # element.  They contain a long dash rule and no factual description.
        if re.search(r"-{8,}", line) and "Type" not in line:
            continue
        lines.append(line)

    # PDF extraction preserves visual line wraps, including splits such as
    # "因\n为".  Join those soft wraps and retain only semantic paragraph,
    # parameter, list and reaction boundaries; the game will wrap again for
    # its own screen width.
    paragraphs: list[str] = []
    field_re = re.compile(
        r"^(?:描述|制取|产生|特性|性质|能力|控制|限制|参数|元素参数|反应|"
        r"内在反应|与[^：:]{0,12}的反应|熔点|沸点|凝固点|燃点|烧制温度|"
        r"压力极限|导热率|初始温度|移动距离|频道|用法|穿透|破坏|合成|"
        r"吸水|遗传|特殊转化|热力过程|参数获取|稀释与相变|主要反应|绝缘)\s*[：:]"
    )
    generic_field_re = re.compile(r"^.{1,20}[：:]")
    formula_re = re.compile(r"^[A-Z0-9].*(?:→|=)")
    list_re = re.compile(r"^(?:\d+[.、)]|[一二三四五六七八九十]+[、.])")
    named_item_re = re.compile(r"^[^，。；：:]{1,18}\([A-Z][A-Z0-9/\-]{1,12}\)")

    for line in lines:
        new_paragraph = (
            not paragraphs
            or bool(field_re.match(line))
            or bool(generic_field_re.match(line))
            or bool(formula_re.match(line))
            or bool(list_re.match(line))
            or bool(named_item_re.match(line))
            or paragraphs[-1].endswith(("：", ":"))
        )
        if new_paragraph:
            paragraphs.append(line)
        else:
            separator = " " if paragraphs[-1][-1:].isascii() and line[:1].isascii() else ""
            paragraphs[-1] += separator + line
    result = "\n".join(paragraphs).strip()
    # Correct a handful of clear transcription errors in the PDF text while
    # preserving its factual content and detail.
    result = result.replace("产生：以通过", "产生：可以通过")
    result = result.replace("HYGH[", "HYGN[")
    result = result.replace("ROCK 和 WATR 反应时生成 STEN", "ROCK 和 WATR 反应时生成 STNE")
    result = result.replace("复制体 CLNE)", "复制体(CLNE)")
    result = result.replace("Cype 值", "Ctype 值")
    result = result.replace("反应成每个酸粒子", "反应成 CAUS。\n元素参数：每个酸粒子")
    return normalise_description(result)


def extract_pdf_entries(pdf_path: Path) -> tuple[dict[str, str], dict[str, int]]:
    reader = PdfReader(str(pdf_path))
    if len(reader.pages) < LAST_ELEMENT_PAGE:
        raise SystemExit(f"PDF only has {len(reader.pages)} pages")

    full_text = ""
    page_offsets: list[int] = []
    for page_number in range(FIRST_ELEMENT_PAGE, LAST_ELEMENT_PAGE + 1):
        page_offsets.append(len(full_text))
        text = reader.pages[page_number - 1].extract_text() or ""
        full_text += text + "\n"

    entries: dict[str, str] = {}
    source_pages: dict[str, int] = {}
    headers = list(HEADER_RE.finditer(full_text))
    for index, header in enumerate(headers):
        codes = CODE_RE.findall(header.group(0))
        if not codes:
            continue
        end = headers[index + 1].start() if index + 1 < len(headers) else len(full_text)
        description = clean_segment(full_text[header.end():end])
        if not description:
            continue
        page_index = bisect.bisect_right(page_offsets, header.start()) - 1
        page_number = FIRST_ELEMENT_PAGE + max(page_index, 0)
        for code in codes:
            # A repeated header in the appendix must not overwrite the full
            # entry found in the main element reference.
            if code not in entries or len(description) > len(entries[code]):
                entries[code] = description
                source_pages[code] = page_number
    return entries, source_pages


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("pdf", type=Path)
    parser.add_argument(
        "--output",
        type=Path,
        default=Path(__file__).resolve().parents[1]
        / "resources"
        / "element_descriptions_zh_CN.json",
    )
    args = parser.parse_args()

    root = Path(__file__).resolve().parents[1]
    source_codes = sorted(path.stem for path in (root / "src/simulation/elements").glob("*.cpp"))
    pdf_entries, source_pages = extract_pdf_entries(args.pdf)

    descriptions: dict[str, str] = {}
    mapping: dict[str, dict[str, object]] = {}
    for source_code in source_codes:
        pdf_code = SOURCE_TO_PDF.get(source_code, source_code)
        if pdf_code in pdf_entries:
            descriptions[source_code] = normalise_description(
                PDF_DESCRIPTION_OVERRIDES.get(source_code, pdf_entries[pdf_code])
            )
            mapping[source_code] = {"pdf_code": pdf_code, "pdf_page": source_pages[pdf_code]}

    for source_code, description in SUPPLEMENTAL_DESCRIPTIONS.items():
        if source_code not in source_codes:
            raise SystemExit(f"supplemental description has no source element: {source_code}")
        descriptions[source_code] = normalise_description(description)
        mapping[source_code] = {"source": "current C++ implementation"}

    missing = sorted(set(source_codes) - set(descriptions))
    payload = {
        "_meta": {
            "source": args.pdf.name,
            "pdf_pages": len(PdfReader(str(args.pdf)).pages),
            "element_reference_pages": [FIRST_ELEMENT_PAGE, LAST_ELEMENT_PAGE],
            "source_elements": len(source_codes),
            "pdf_mapped_elements": sum("pdf_code" in item for item in mapping.values()),
            "current_source_supplements": len(SUPPLEMENTAL_DESCRIPTIONS),
            "missing_current_elements": missing,
            "mapping": mapping,
        },
        "descriptions": descriptions,
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(
        f"pdf_entries={len(pdf_entries)} mapped={len(descriptions)} "
        f"missing={len(missing)} output={args.output}"
    )
    if missing:
        print("missing:", " ".join(missing))


if __name__ == "__main__":
    main()
