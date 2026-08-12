import json
import math
from pathlib import Path


EXPECTED = {
    "TIAL": {"TTAN": 4 / 6, "ALUM": 1 / 6, "V": 1 / 6},
    "NSAL": {"NICL": 4 / 6, "CHRM": 1 / 6, "COBT": 1 / 6},
    "WALY": {"TUNG": 4 / 6, "NICL": 1 / 6, "IRON": 1 / 6},
    "ZRAL": {"ZR": 4 / 5, "TIN": 1 / 5},
    "NITI": {"NICL": 1 / 2, "TTAN": 1 / 2},
}


def main() -> int:
    root = Path(__file__).resolve().parents[1]
    data = json.loads((root / "resources/omnicore/v1/omni-engineering-alloy-compositions-v1.json").read_text(encoding="utf-8"))
    assert data["schema"] == "omnicore.fixed-alloy-composition"
    assert data["version"] == 1
    assert data["status"] == "game_recipe_fraction_not_industrial_grade_claim"
    assert data["persistence"] == "stable_element_id"
    found = {row["element"]: row["composition"] for row in data["alloys"]}
    assert set(found) == set(EXPECTED)
    for alloy, composition in EXPECTED.items():
        assert set(found[alloy]) == set(composition)
        assert math.isclose(sum(found[alloy].values()), 1.0, abs_tol=1e-12)
        for element, fraction in composition.items():
            assert math.isclose(found[alloy][element], fraction, abs_tol=1e-12)
    metallurgy = (root / "src/simulation/OmniMetallurgy.cpp").read_text(encoding="utf-8")
    for product, ingredients in {
        "PT_TIAL": ("PT_TTAN, 4", "PT_ALUM, 1", "PT_V,    1"),
        "PT_NSAL": ("PT_NICL, 4", "PT_CHRM, 1", "PT_COBT, 1"),
        "PT_WALY": ("PT_TUNG, 4", "PT_NICL, 1", "PT_IRON, 1"),
        "PT_ZRAL": ("PT_ZR,   4", "PT_TIN,  1"),
        "PT_NITI": ("PT_NICL, 1", "PT_TTAN, 1"),
    }.items():
        assert product in metallurgy
        for ingredient in ingredients:
            assert ingredient in metallurgy
    lua = (root / "src/lua/LuaSimulation.cpp").read_text(encoding="utf-8")
    assert "static int omniAlloyComposition(lua_State *L)" in lua
    assert "LFUNC(omniAlloyComposition)" in lua
    print("omni_alloy_composition_contract_pass=true")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

