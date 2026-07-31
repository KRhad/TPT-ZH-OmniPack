#!/usr/bin/env python3
"""Generate the stable 118-element source map from audited registries."""

from __future__ import annotations

import argparse
import csv
from io import StringIO
from pathlib import Path
import re
import sys
from typing import Sequence


ELEMENTS = (
    ("H", "氢", "Hydrogen"), ("He", "氦", "Helium"), ("Li", "锂", "Lithium"),
    ("Be", "铍", "Beryllium"), ("B", "硼", "Boron"), ("C", "碳", "Carbon"),
    ("N", "氮", "Nitrogen"), ("O", "氧", "Oxygen"), ("F", "氟", "Fluorine"),
    ("Ne", "氖", "Neon"), ("Na", "钠", "Sodium"), ("Mg", "镁", "Magnesium"),
    ("Al", "铝", "Aluminium"), ("Si", "硅", "Silicon"), ("P", "磷", "Phosphorus"),
    ("S", "硫", "Sulfur"), ("Cl", "氯", "Chlorine"), ("Ar", "氩", "Argon"),
    ("K", "钾", "Potassium"), ("Ca", "钙", "Calcium"), ("Sc", "钪", "Scandium"),
    ("Ti", "钛", "Titanium"), ("V", "钒", "Vanadium"), ("Cr", "铬", "Chromium"),
    ("Mn", "锰", "Manganese"), ("Fe", "铁", "Iron"), ("Co", "钴", "Cobalt"),
    ("Ni", "镍", "Nickel"), ("Cu", "铜", "Copper"), ("Zn", "锌", "Zinc"),
    ("Ga", "镓", "Gallium"), ("Ge", "锗", "Germanium"), ("As", "砷", "Arsenic"),
    ("Se", "硒", "Selenium"), ("Br", "溴", "Bromine"), ("Kr", "氪", "Krypton"),
    ("Rb", "铷", "Rubidium"), ("Sr", "锶", "Strontium"), ("Y", "钇", "Yttrium"),
    ("Zr", "锆", "Zirconium"), ("Nb", "铌", "Niobium"), ("Mo", "钼", "Molybdenum"),
    ("Tc", "锝", "Technetium"), ("Ru", "钌", "Ruthenium"), ("Rh", "铑", "Rhodium"),
    ("Pd", "钯", "Palladium"), ("Ag", "银", "Silver"), ("Cd", "镉", "Cadmium"),
    ("In", "铟", "Indium"), ("Sn", "锡", "Tin"), ("Sb", "锑", "Antimony"),
    ("Te", "碲", "Tellurium"), ("I", "碘", "Iodine"), ("Xe", "氙", "Xenon"),
    ("Cs", "铯", "Caesium"), ("Ba", "钡", "Barium"), ("La", "镧", "Lanthanum"),
    ("Ce", "铈", "Cerium"), ("Pr", "镨", "Praseodymium"), ("Nd", "钕", "Neodymium"),
    ("Pm", "钷", "Promethium"), ("Sm", "钐", "Samarium"), ("Eu", "铕", "Europium"),
    ("Gd", "钆", "Gadolinium"), ("Tb", "铽", "Terbium"), ("Dy", "镝", "Dysprosium"),
    ("Ho", "钬", "Holmium"), ("Er", "铒", "Erbium"), ("Tm", "铥", "Thulium"),
    ("Yb", "镱", "Ytterbium"), ("Lu", "镥", "Lutetium"), ("Hf", "铪", "Hafnium"),
    ("Ta", "钽", "Tantalum"), ("W", "钨", "Tungsten"), ("Re", "铼", "Rhenium"),
    ("Os", "锇", "Osmium"), ("Ir", "铱", "Iridium"), ("Pt", "铂", "Platinum"),
    ("Au", "金", "Gold"), ("Hg", "汞", "Mercury"), ("Tl", "铊", "Thallium"),
    ("Pb", "铅", "Lead"), ("Bi", "铋", "Bismuth"), ("Po", "钋", "Polonium"),
    ("At", "砹", "Astatine"), ("Rn", "氡", "Radon"), ("Fr", "钫", "Francium"),
    ("Ra", "镭", "Radium"), ("Ac", "锕", "Actinium"), ("Th", "钍", "Thorium"),
    ("Pa", "镤", "Protactinium"), ("U", "铀", "Uranium"), ("Np", "镎", "Neptunium"),
    ("Pu", "钚", "Plutonium"), ("Am", "镅", "Americium"), ("Cm", "锔", "Curium"),
    ("Bk", "锫", "Berkelium"), ("Cf", "锎", "Californium"), ("Es", "锿", "Einsteinium"),
    ("Fm", "镄", "Fermium"), ("Md", "钔", "Mendelevium"), ("No", "锘", "Nobelium"),
    ("Lr", "铹", "Lawrencium"), ("Rf", "𬬻", "Rutherfordium"), ("Db", "𬭊", "Dubnium"),
    ("Sg", "𬭳", "Seaborgium"), ("Bh", "𬭛", "Bohrium"), ("Hs", "𬭶", "Hassium"),
    ("Mt", "鿏", "Meitnerium"), ("Ds", "𫟼", "Darmstadtium"), ("Rg", "𬬭", "Roentgenium"),
    ("Cn", "鿔", "Copernicium"), ("Nh", "鿭", "Nihonium"), ("Fl", "𫓧", "Flerovium"),
    ("Mc", "镆", "Moscovium"), ("Lv", "𫟷", "Livermorium"), ("Ts", "鿬", "Tennessine"),
    ("Og", "鿫", "Oganesson"),
)

# Only source implementations that represent the pure element are reused. Generic
# METL/NBLE and compounds such as DEUT are intentionally absent.
REUSE_STABLE_IDS = {
    1: 148, 3: 191, 6: 28, 8: 61, 12: 261, 13: 256, 14: 187, 17: 360,
    22: 144, 24: 262, 26: 76, 27: 263, 28: 260, 29: 257, 30: 265,
    37: 41, 42: 264, 50: 259, 74: 171, 78: 188, 79: 170, 80: 152,
    82: 258, 84: 182, 92: 32, 94: 19,
}

COLUMNS = (
    "atomic_number", "symbol", "zh_name", "en_name", "official_mapping",
    "omnipack_mapping", "candidate_mods", "selected_source",
    "selected_source_commit", "implementation_type", "stable_id", "status", "tests",
)


def normalized(value: str) -> str:
    return re.sub(r"[^a-z0-9]+", "", value.lower())


def read_csv(path: Path) -> list[dict[str, str]]:
    with path.open("r", encoding="utf-8", newline="") as stream:
        return list(csv.DictReader(stream))


def generate(root: Path) -> str:
    if len(ELEMENTS) != 118:
        raise ValueError(f"periodic data has {len(ELEMENTS)} rows, expected 118")
    registry = {int(row["stable_id"]): row for row in read_csv(root / "docs/ELEMENT_REGISTRY.csv")}
    candidates = read_csv(root / "docs/MOD_ELEMENT_CATALOG.csv")
    rows: list[dict[str, str]] = []
    next_id = 370
    for atomic_number, (symbol, zh_name, en_name) in enumerate(ELEMENTS, 1):
        candidate_mods = sorted({
            row["source_mod"]
            for row in candidates
            if row["source_mod"] != "official"
            and (
                normalized(row["source_code"]) == normalized(symbol)
                or normalized(row["source_name"]) == normalized(en_name)
            )
        })
        reused_id = REUSE_STABLE_IDS.get(atomic_number)
        if reused_id is not None:
            current = registry.get(reused_id)
            if current is None or current["implementation_status"] != "implemented":
                raise ValueError(f"atomic number {atomic_number} maps to unavailable stable ID {reused_id}")
            official = current["module"] == "official"
            implementation_type = "reuse_official" if official else "reuse_omnipack"
            stable_id = reused_id
            selected_source = current["source"]
            selected_commit = current["source_commit"]
            status = "implemented"
            tests = current["tests"]
            official_mapping = current["identifier"] if official else ""
            omnipack_mapping = "" if official else current["identifier"]
        else:
            stable_id = next_id
            next_id += 1
            implementation_type = "family_generated"
            official_mapping = ""
            current = registry.get(stable_id)
            if current is not None and current["implementation_status"] == "implemented":
                if current["module"] != "periodic":
                    raise ValueError(
                        f"periodic stable ID {stable_id} has unexpected module {current['module']!r}"
                    )
                omnipack_mapping = current["identifier"]
                selected_source = current["source"]
                selected_commit = current["source_commit"]
                status = "implemented"
                tests = current["tests"]
            else:
                selected_source = "TPT-ZH-OmniPack/family-generated"
                selected_commit = "not_implemented"
                status = "planned"
                tests = "not_tested"
                omnipack_mapping = ""
        rows.append({
            "atomic_number": str(atomic_number),
            "symbol": symbol,
            "zh_name": zh_name,
            "en_name": en_name,
            "official_mapping": official_mapping,
            "omnipack_mapping": omnipack_mapping,
            "candidate_mods": ";".join(candidate_mods),
            "selected_source": selected_source,
            "selected_source_commit": selected_commit,
            "implementation_type": implementation_type,
            "stable_id": str(stable_id),
            "status": status,
            "tests": tests,
        })
    if next_id != 462:
        raise ValueError(f"new periodic stable ID range ended at {next_id - 1}, expected 461")
    stable_ids = [int(row["stable_id"]) for row in rows]
    if len(stable_ids) != len(set(stable_ids)):
        raise ValueError("periodic source map contains duplicate stable IDs")
    output = StringIO(newline="")
    writer = csv.DictWriter(output, fieldnames=COLUMNS, lineterminator="\n")
    writer.writeheader()
    writer.writerows(rows)
    return output.getvalue()


def main(argv: Sequence[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--output", type=Path)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args(argv)
    root = args.source_root.resolve()
    output = args.output or root / "docs/PERIODIC_ELEMENT_SOURCE_MAP.csv"
    rendered = generate(root)
    if args.check:
        if not output.is_file() or output.read_text(encoding="utf-8") != rendered:
            print(f"ERROR {output}: periodic source map is missing or stale", file=sys.stderr)
            return 1
        print("periodic-source-map: PASS rows=118 new_ids=92 range=370..461")
        return 0
    output.write_text(rendered, encoding="utf-8", newline="")
    print(f"periodic-source-map: WROTE rows=118 new_ids=92 output={output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
