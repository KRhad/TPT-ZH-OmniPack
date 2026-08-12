import json
from pathlib import Path


def main() -> int:
    root = Path(__file__).resolve().parents[1]
    data = json.loads((root / "resources/omnicore/v1/omni-corrosion-runtime-v1.json").read_text(encoding="utf-8"))
    assert data["schema"] == "omnicore.corrosion-runtime"
    assert data["runtime_version"] == "1.0.9"
    assert data["rate_policy"]["status"] == "game_tuned_explicit"
    assert data["classic_mode"] == "legacy_behavior_unchanged"
    assert "no_stoichiometric_atmosphere_oxygen_consumption_in_v1" in data["limitations"]
    source = (root / "src/simulation/OmniCorrosion.h").read_text(encoding="utf-8")
    simulation = (root / "src/simulation/Simulation.cpp").read_text(encoding="utf-8")
    iron = (root / "src/simulation/elements/IRON.cpp").read_text(encoding="utf-8")
    lua = (root / "src/lua/LuaSimulation.cpp").read_text(encoding="utf-8")
    for needle in ("ProgressPerWetReferenceTick", "TemperatureFactor", "HumidityMoistureFactor"):
        assert needle in source
    for needle in ("SpeciesPartialPressurePa", "relativeHumidity", "chlorideSeverity", "protectedByZinc"):
        assert needle in simulation
    assert "if (sim->IsOmniAtmosphereActive())" in iron
    assert "static int omniCorrosion(lua_State *L)" in lua
    assert "LFUNC(omniCorrosion)" in lua
    print("omni_corrosion_contract_pass=true")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
