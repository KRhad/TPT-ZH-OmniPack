import json
import math
from pathlib import Path


def main() -> int:
    root = Path(__file__).resolve().parents[1]
    contract_path = root / "resources/omnicore/v1/omni-solution-nacl-runtime-v1.json"
    contract = json.loads(contract_path.read_text(encoding="utf-8"))
    assert contract["schema"] == "omnicore.solution-runtime"
    assert contract["version"] == 1
    assert contract["authoritative_state"] == ["solvent_mass_kg", "solute_mass_kg"]
    assert contract["classic_mode"] == "legacy_behavior_unchanged"
    assert contract["saturation"]["source_url"].startswith("https://www.usgs.gov/")
    assert contract["parcel_policy"]["status"] == "game_tuned_explicit"
    values = []
    for celsius in (0.0, 25.0, 100.0):
        percent = 26.218 + 0.0072 * celsius + 0.000106 * celsius * celsius
        assert math.isfinite(percent) and 0.0 < percent < 100.0
        values.append(percent)
    assert values[0] < values[1] < values[2]

    source = (root / "src/simulation/OmniSolution.h").read_text(encoding="utf-8")
    simulation = (root / "src/simulation/Simulation.cpp").read_text(encoding="utf-8")
    lua = (root / "src/lua/LuaSimulation.cpp").read_text(encoding="utf-8")
    for needle in (
        "SodiumChlorideSaturationMassFraction",
        "MaximumDissolvedSoluteKg",
        "MaximumDissolutionMassPerTransactionKg",
        "MaximumCrystallisationMassPerTickKg",
    ):
        assert needle in source
    for needle in (
        "BeginOmniSolutionTick();",
        "FinishOmniSolutionTick();",
        "UpdateOmniSolutionParticle",
        "omniSolutionInternalMutation",
    ):
        assert needle in simulation
    assert "static int omniSolution(lua_State *L)" in lua
    assert "LFUNC(omniSolution)" in lua
    print("omni_solution_contract_pass=true")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

