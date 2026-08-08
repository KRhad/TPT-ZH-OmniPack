#!/usr/bin/env python3
"""Expand the locked official-description table without changing stable IDs.

The Wiki pages are research references only.  This tool never downloads them and
the game never performs a network lookup.  It combines the audited upstream
summary with exact runtime properties shown by ElementInfo.  Hand-written
overrides repair known mistranslations and summaries that are too terse to be
useful to a player.
"""

from __future__ import annotations

import argparse
import csv
from pathlib import Path
from typing import Sequence


OFFICIAL_SOURCE = "The-Powder-Toy/The-Powder-Toy"
FIELDS = (
    "identifier",
    "wiki_url",
    "wiki_snapshot",
    "english_description",
    "chinese_description",
)

# Each timestamp was checked to contain the actual category article rather than
# an empty capture or an Anubis challenge page.  Individual liquid records that
# were already curated keep their more specific source URL and timestamp.
CATEGORY_SOURCES = {
    "SC_ELEC": (
        "https://powdertoy.co.uk/Wiki/W/Elements:Electronics/zh.html",
        "20260521203740",
    ),
    "SC_EXPLOSIVE": (
        "https://powdertoy.co.uk/Wiki/W/Elements:Explosives/zh.html",
        "20260107145511",
    ),
    "SC_FORCE": (
        "https://powdertoy.co.uk/Wiki/W/Elements:Force/zh.html",
        "20251103163404",
    ),
    "SC_GAS": (
        "https://powdertoy.co.uk/Wiki/W/Elements:Gases/zh.html",
        "20250825170646",
    ),
    "SC_LIFE": (
        "https://powdertoy.co.uk/Wiki/W/Elements:Life.html",
        "20251107123141",
    ),
    "SC_LIQUID": (
        "https://powdertoy.co.uk/Wiki/W/Elements:Liquids/zh.html",
        "20260517082733",
    ),
    "SC_NUCLEAR": (
        "https://powdertoy.co.uk/Wiki/W/Elements:Radioactive/zh.html",
        "20240424211836",
    ),
    "SC_POWDERS": (
        "https://powdertoy.co.uk/Wiki/W/Elements:Powders/zh.html",
        "20260315022917",
    ),
    "SC_POWERED": (
        "https://powdertoy.co.uk/Wiki/W/Elements:Powered_materials/zh.html",
        "20251212130500",
    ),
    "SC_SENSOR": (
        "https://powdertoy.co.uk/Wiki/W/Elements:Sensors/zh.html",
        "20260516082540",
    ),
    "SC_SOLIDS": (
        "https://powdertoy.co.uk/Wiki/W/Elements:Solids/zh.html",
        "20250830155822",
    ),
    "SC_SPECIAL": (
        "https://powdertoy.co.uk/Wiki/W/Elements:Special/zh.html",
        "20260315022808",
    ),
}

RUNTIME_DETAIL_EN = (
    "The Simulation Properties section below is read from this build and lists "
    "spawn temperature, heat transfer, heat capacity, weight, gravity, hardness, "
    "flammability, explosiveness, conductivity, neutron absorption, and every "
    "configured pressure or temperature transition."
)
RUNTIME_DETAIL_ZH = (
    "下方“模拟属性”直接读取当前构建，列出生成温度、导热、热容量、重量、重力、"
    "耐蚀性、可燃性、爆炸性、导电、中子吸收以及全部温度和压力相变；实际玩法以"
    "这些参数和当前源码为准。"
)

# These entries repair concrete defects in the old Chinese summaries or explain
# official elements whose upstream one-line description is not self-sufficient.
OVERRIDES = {
    "DEFAULT_PT_NONE": (
        "The eraser removes particles under the brush without placing a material. "
        "It is a drawing tool, so it has no persistent particle behaviour of its own.",
        "擦除工具会删除笔刷覆盖范围内的粒子，而不会放置新材料。它属于绘图工具，"
        "自身不会作为可持续模拟的粒子留在场景中。",
    ),
    "DEFAULT_PT_DUST": (
        "A very light powder that is flammable but produces a faint, easy-to-miss "
        "flame. Neutron exposure can convert it into legacy firework powder.",
        "一种很轻的可燃粉末，火焰较暗，燃烧时容易被忽略。受到中子轰击后可转成"
        "传统烟花粉 FWRK。",
    ),
    "DEFAULT_PT_GOO": (
        "A pressure-sensitive goo that deforms and is gradually consumed under "
        "compression. It is not clay dust; CLST is the separate clay material.",
        "一种会在压力下变形并逐渐消失的黏胶。它不是黏土粉；黏土粉是单独的 CLST。",
    ),
    "DEFAULT_PT_NEUT": (
        "A neutral energy particle affected by Newtonian gravity. Neutrons pass "
        "through or react with materials in element-specific ways and drive many "
        "fission, isotope, purification, and transmutation reactions.",
        "一种不带电、会受牛顿引力影响的能量粒子。中子会按材料规则穿透、被吸收或"
        "发生反应，是裂变、同位素、净化和转化玩法的重要触发物。",
    ),
    "DEFAULT_PT_VACU": (
        "An air vacuum vent that continuously removes local pressure, pulling mobile "
        "particles inward through air flow and heating as it operates.",
        "持续抽走局部空气压力的真空孔；它通过气流把可移动粒子拉向自身，并在工作"
        "过程中升温。这里的吸入来自压力场，不是牛顿引力。",
    ),
    "DEFAULT_PT_VENT": (
        "An air vent that continuously adds local pressure and pushes mobile particles "
        "away through air flow.",
        "持续增加局部空气压力的排气孔，会通过气流把可移动粒子向外推开；它不产生"
        "牛顿斥力。",
    ),
    "DEFAULT_PT_THDR": (
        "A single-use ball of lightning spawned extremely hot. On contact it damages "
        "most matter, creates a strong pressure pulse, and transfers spark to conductors.",
        "一次性的高温球状闪电。接触物质时会造成破坏并产生强压力脉冲，同时可向"
        "导体传递电脉冲。",
    ),
    "DEFAULT_PT_DYST": (
        "Dead yeast produced when YEST is overheated or irradiated by neutrons. It "
        "converts neighbouring live yeast into DYST and eventually becomes DUST at "
        "higher temperature.",
        "死酵母，由 YEST 过热或受到中子作用后产生。它会把邻近活酵母继续转成 "
        "DYST，温度进一步升高后最终变成 DUST；它不是菌丝。",
    ),
    "DEFAULT_PT_MORT": (
        "A novelty steam-train particle that moves while emitting smoke. It is a "
        "legacy decorative element rather than a general-purpose gas or vehicle.",
        "会移动并沿途释放烟雾的趣味蒸汽火车粒子，属于保留的装饰性旧元素，并不是"
        "普通气体或可操纵载具。",
    ),
    "DEFAULT_PT_TESC": (
        "A Tesla coil that emits hot LIGH arcs when sparked. Brush size controls the "
        "maximum arc length, so large coils can affect a wide area.",
        "通电后发射高温 LIGH 电弧的特斯拉线圈。笔刷尺寸决定最大放电距离，因此较大"
        "线圈会影响更宽的区域。",
    ),
    "DEFAULT_PT_BIZRG": (
        "The gaseous phase of bizarre matter. Its phase ordering is reversed: cooling "
        "BIZR makes BIZG, while sufficient heating moves the family toward solid BIZS.",
        "奇异物质的气态相，其相变顺序与常规物质相反：冷却 BIZR 会形成 BIZG，而"
        "充分加热则会使该家族趋向固态 BIZS。",
    ),
    "DEFAULT_PT_BIZRS": (
        "The solid phase of bizarre matter, formed at the hot end of the family's "
        "reversed phase sequence. Cooling it returns the material toward BIZR and BIZG.",
        "奇异物质的固态相，出现在该家族反常相变序列的高温端；冷却后会依次回到 "
        "BIZR 和 BIZG。",
    ),
    "DEFAULT_PT_PSTS": (
        "Solid paste formed when liquid PSTE hardens under pressure. It remains a solid "
        "building material and can be baked into BRCK at high temperature.",
        "液态浆糊 PSTE 受压硬化后形成的固态浆糊，可作为固体建材使用；继续高温"
        "加热会烧结成 BRCK。",
    ),
    "DEFAULT_PT_EQVE": (
        "A hidden legacy shared-velocity experiment retained for save compatibility. "
        "It is not a normal menu material and should not be used as a stable gameplay tool.",
        "为旧存档兼容而保留的隐藏共享速度实验元素。它不是普通菜单材料，也不应当作"
        "稳定玩法工具使用。",
    ),
    "DEFAULT_PT_LOVE": (
        "A hidden decorative element that draws the LOVE pattern. It has no chemistry "
        "or engineering role beyond preserving the original novelty effect.",
        "用于绘制 LOVE 图案的隐藏装饰元素。除保留原有趣味效果外，它没有化学或"
        "工程用途。",
    ),
    "DEFAULT_PT_LOLZ": (
        "A hidden decorative element that draws the LOLZ pattern. It is retained as a "
        "novelty and does not represent a physical material.",
        "用于绘制 LOLZ 图案的隐藏装饰元素，作为趣味内容保留，不代表真实物质。",
    ),
    "DEFAULT_PT_BRAY": (
        "The beam particle created by ARAY and several ray devices. Its colour and life "
        "encode the beam mode; depending on that mode it can conduct, erase, or interact "
        "with other electronics before expiring.",
        "由 ARAY 等射线装置生成的束流粒子，颜色和 life 记录射线模式；不同模式可"
        "传导、擦除或与其他电子元件交互，随后自行消失。",
    ),
    "DEFAULT_PT_BOYL": (
        "A non-flammable gas whose pressure response follows a Boyle-like model. Heating "
        "makes it expand and raise pressure, while cooling contracts it.",
        "按近似波义耳模型响应压力的不可燃气体；受热膨胀并抬高压力，冷却时收缩。",
    ),
    "DEFAULT_PT_INVIS": (
        "A pressure-sensitive solid that becomes invisible and allows particles through "
        "when the pressure threshold is reached. It is distinct from the destructive VOID.",
        "达到压力阈值后会隐形并允许粒子通过的传感固体，与会删除物质的 VOID 完全"
        "不同。",
    ),
    "DEFAULT_PT_DMG": (
        "A fast damage particle that releases a destructive pressure pulse when it hits "
        "matter, breaking vulnerable solids along the impact surface. It is not a gravity bomb.",
        "高速冲击粒子，撞击物质表面时释放破坏性压力脉冲并击碎脆弱固体；它不是"
        "引力炸弹。",
    ),
    "DEFAULT_PT_PSTN": (
        "A piston that extends when sparked from PSCN and retracts when sparked from NSCN. "
        "FRME can connect a larger face so one piston moves a wider structure.",
        "由 PSCN 脉冲驱动伸长、由 NSCN 脉冲驱动回缩的活塞。配合 FRME 可连接更大"
        "的受力面，让一个活塞推动更宽的结构。",
    ),
    "DEFAULT_PT_DRAY": (
        "A duplicator ray triggered like ARAY. It copies a configured line of particles "
        "from the source side to the destination side, with range and spacing controlled "
        "by its stored values.",
        "按 ARAY 方向规则触发的复制射线，会把源侧一段粒子复制到目标侧；复制范围与"
        "间隔由其内部参数控制。",
    ),
    "DEFAULT_PT_RIME": (
        "A frost solid deposited when water vapour cools rapidly. A spark converts RIME "
        "into FOG, providing an electrical route back to a mobile phase.",
        "水蒸气快速冷却并凝华后形成的霜。电脉冲会把 RIME 转成 FOG，使其重新进入"
        "可移动相态。",
    ),
    "DEFAULT_PT_CAUS": (
        "A corrosive caustic gas that attacks many materials in an acid-like way. It is "
        "the gaseous corrosive element, not ordinary carbon dioxide or water vapour.",
        "会以类似 ACID 的方式腐蚀多种材料的苛性气体，是专用的气态腐蚀元素，并非"
        "普通二氧化碳或水蒸气。",
    ),
    "DEFAULT_PT_O2": (
        "Oxygen gas that strongly supports combustion and can be consumed by fire. It "
        "also participates in water formation and several material-specific oxidation reactions.",
        "强烈助燃并会被火焰消耗的氧气，还参与生成水及多种材料专用的氧化反应；"
        "“助燃”比把它简单称作燃料更准确。",
    ),
    "DEFAULT_PT_LIFE": (
        "The cellular-automaton element. The default LIFE tool uses Conway's B3/S23 "
        "rules, while its subtype selects the other built-in Life rule sets.",
        "细胞自动机元素。默认 LIFE 工具采用康威生命游戏 B3/S23 规则，其他内置"
        "生命规则通过其子类型选择。",
    ),
}


def load_csv(path: Path) -> list[dict[str, str]]:
    with path.open("r", encoding="utf-8-sig", newline="") as stream:
        return list(csv.DictReader(stream))


def complete_description(summary: str, runtime_detail: str) -> str:
    summary = summary.strip()
    if summary and summary[-1] not in ".!?。！？":
        summary += "。" if any("\u3400" <= char <= "\u9fff" for char in summary) else "."
    return f"{summary} {runtime_detail}".strip()


def render(root: Path) -> list[dict[str, str]]:
    registry = load_csv(root / "docs" / "ELEMENT_REGISTRY.csv")
    description_path = root / "docs" / "OFFICIAL_ELEMENT_DESCRIPTIONS.csv"
    curated = {
        row["identifier"]: row
        for row in load_csv(description_path)
        if row.get("identifier")
    }
    official = [
        row
        for row in registry
        if row.get("source_mod") == OFFICIAL_SOURCE
        and row.get("implementation_status") == "implemented"
        and row.get("is_duplicate") != "true"
    ]
    rows: list[dict[str, str]] = []
    for row in sorted(official, key=lambda item: int(item["stable_id"])):
        identifier = row["identifier"]
        # The original liquid batch was individually researched and edited.  All
        # other categories are regenerated so newly added overrides cannot be
        # masked by an older generated row.
        if row["menu_category"] == "SC_LIQUID" and identifier in curated:
            rows.append({field: curated[identifier][field] for field in FIELDS})
            continue
        try:
            wiki_url, wiki_snapshot = CATEGORY_SOURCES[row["menu_category"]]
        except KeyError as exc:
            raise ValueError(
                f"{identifier}: no locked Wiki category source for "
                f"{row['menu_category']}"
            ) from exc
        english, chinese = OVERRIDES.get(
            identifier,
            (row["english_description"], row["chinese_description"]),
        )
        rows.append(
            {
                "identifier": identifier,
                "wiki_url": wiki_url,
                "wiki_snapshot": wiki_snapshot,
                "english_description": complete_description(
                    english, RUNTIME_DETAIL_EN
                ),
                "chinese_description": complete_description(
                    chinese, RUNTIME_DETAIL_ZH
                ),
            }
        )
    return rows


def main(argv: Sequence[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--source-root", type=Path, default=Path(__file__).resolve().parents[1]
    )
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args(argv)
    root = args.source_root.resolve()
    path = root / "docs" / "OFFICIAL_ELEMENT_DESCRIPTIONS.csv"
    rows = render(root)
    from io import StringIO

    buffer = StringIO(newline="")
    writer = csv.DictWriter(buffer, fieldnames=FIELDS, lineterminator="\n")
    writer.writeheader()
    writer.writerows(rows)
    rendered = buffer.getvalue()
    if args.check:
        current = path.read_text(encoding="utf-8-sig")
        if current != rendered:
            print(f"{path}: stale; run {Path(__file__).name}")
            return 1
        return 0
    path.write_text(rendered, encoding="utf-8", newline="")
    print(f"wrote {len(rows)} official descriptions to {path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
