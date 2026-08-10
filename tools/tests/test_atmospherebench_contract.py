from __future__ import annotations

import importlib.util
import json
from pathlib import Path
import re
import sys
import unittest
from decimal import Decimal


ROOT = Path(__file__).resolve().parents[2]
BENCH_ROOT = ROOT / "tools" / "atmospherebench"
PACKAGE_TOOL_PATH = ROOT / "tools" / "package_source_release.py"
PACKAGE_SPEC = importlib.util.spec_from_file_location(
    "package_source_release_for_atmospherebench_test", PACKAGE_TOOL_PATH
)
assert PACKAGE_SPEC is not None and PACKAGE_SPEC.loader is not None
package_tool = importlib.util.module_from_spec(PACKAGE_SPEC)
sys.modules[PACKAGE_SPEC.name] = package_tool
PACKAGE_SPEC.loader.exec_module(package_tool)


class AtmosphereBenchSourceContractTests(unittest.TestCase):
    def test_scaffold_isolated_from_production_simulation(self) -> None:
        text = "\n".join(path.read_text(encoding="utf-8") for path in BENCH_ROOT.glob("*.cpp"))
        forbidden = (
            "src/simulation", "Simulation.h", "Air.h", "GameSave",
            "LuaScriptInterface", "project_cpp_args", "-ffast-math",
            "-funsafe-math-optimizations",
        )
        for needle in forbidden:
            with self.subTest(needle=needle):
                self.assertNotIn(needle, text)
        includes = re.findall(r'^#include "([^"]+)"', text, flags=re.MULTILINE)
        self.assertEqual(includes.count("AtmosphereBench.h"), 2)
        self.assertEqual(includes.count("Rusanov1D.h"), 3)

    def test_bench_has_explicit_strict_fp_target_and_only_rusanov_candidate(self) -> None:
        meson = (ROOT / "meson.build").read_text(encoding="utf-8")
        self.assertIn("atmospherebench", meson)
        self.assertIn("-fno-fast-math", meson)
        self.assertIn("-ffp-contract=off", meson)
        start = meson.index("atmospherebench = executable(")
        end = meson.index("font_render_probe = executable(", start)
        target = meson[start:end]
        self.assertIn("cpp_args: atmospherebench_cpp_args", target)
        self.assertNotIn("cpp_args: project_cpp_args", target)
        self.assertNotIn("sta_libs['simulation']", target)
        self.assertIn("'atmospherebench-rusanov-uniform'", target)
        self.assertIn("args: [ '--run-rusanov-uniform' ]", target)
        source = (BENCH_ROOT / "AtmosphereBench.cpp").read_text(encoding="utf-8")
        self.assertIn('"registered_only"', source)
        self.assertIn('"implemented_1d_periodic_uniform_probe"', source)
        self.assertIn("RUSANOV_UNIFORM_PROBE", source)
        self.assertIn("=UNSELECTED", source)
        self.assertIn("synthetic_nondimensional", source)
        self.assertNotIn("287.05", source)

    def test_rusanov_candidate_is_strict_double_and_keeps_other_candidates_registered_only(self) -> None:
        header = (BENCH_ROOT / "Rusanov1D.h").read_text(encoding="utf-8")
        source = (BENCH_ROOT / "Rusanov1D.cpp").read_text(encoding="utf-8")
        self.assertIn("RusanovProbeSummary", header)
        self.assertIn("RunRusanovUniform", header)
        self.assertIn("RusanovFluxX", source)
        self.assertIn("maximumWaveSpeed", source)
        self.assertIn("std::vector<ConservativeState>", source)
        self.assertIn("BoundaryMode::Periodic", source)
        self.assertIn("summary.ledger.Closes(1e-12)", source)
        self.assertIn("summary.corrections.IsEmpty()", source)
        self.assertIn('output << "momentum_drift="', source)
        self.assertIn('output << "correction_event_count="', source)
        atmosphere = (BENCH_ROOT / "AtmosphereBench.cpp").read_text(encoding="utf-8")
        self.assertIn('"fvm_hlle", "registered_only", false', atmosphere)
        self.assertIn('"lbm_d2q9", "registered_only", false', atmosphere)

    def test_cpp_scale_fixture_matches_the_unselected_json_candidate(self) -> None:
        candidate = json.loads(
            (ROOT / "resources/omnicore/v1/physical-scale-candidates.json").read_text(encoding="utf-8")
        )["geometry_candidates"][0]
        header = (BENCH_ROOT / "AtmosphereBench.h").read_text(encoding="utf-8")
        expected = {
            "pixelLengthM": candidate["pixel_length"]["value"],
            "cellLengthM": candidate["atmosphere_cell_length"]["value"],
            "effectiveDepthM": candidate["effective_depth"]["value"],
        }
        for field, value in expected.items():
            match = re.search(rf"{field}\s*=\s*([0-9.]+)", header)
            self.assertIsNotNone(match, field)
            self.assertEqual(Decimal(match.group(1)), Decimal(value), field)

    def test_shared_contract_types_remain_solver_free_and_nondimensional(self) -> None:
        header = (BENCH_ROOT / "AtmosphereBench.h").read_text(encoding="utf-8")
        source = (BENCH_ROOT / "AtmosphereBench.cpp").read_text(encoding="utf-8")
        for symbol in (
            "AtmosphereGrid", "BenchmarkCase", "NumericalCorrectionLedger",
            "BenchmarkResult", "NondimensionalContract",
        ):
            with self.subTest(symbol=symbol):
                self.assertIn(symbol, header)
        self.assertIn('"case_time_domain=nondimensional_contract', source)
        self.assertIn('"case_step_count="', source)
        self.assertIn('"state_bytes_per_cell="', source)
        self.assertIn("!invalidGrid.IsValid()", source)
        self.assertIn("!invalidCase.IsValid()", source)
        self.assertIn("eventCount", header)
        self.assertIn("!uncountedCorrections.IsConsistent()", source)

    def test_scaffold_contract_is_not_a_runtime_consumer(self) -> None:
        production = []
        for path in (ROOT / "src").rglob("*"):
            if path.suffix in {".cpp", ".h"}:
                text = path.read_text(encoding="utf-8")
                if "atmospherebench" in text.lower() or "physical-scale-candidates" in text:
                    production.append(str(path.relative_to(ROOT)))
        self.assertEqual(production, [])

    def test_source_package_keeps_physical_scale_and_bench_tools(self) -> None:
        tools = {
            "tools/physical_scale_check.py",
            "tools/run_atmospherebench.ps1",
            "tools/atmospherebench/AtmosphereBench.cpp",
            "tools/atmospherebench/AtmosphereBench.h",
            "tools/atmospherebench/main.cpp",
            "tools/atmospherebench/Rusanov1D.cpp",
            "tools/atmospherebench/Rusanov1D.h",
        }
        required = tools | {"resources/omnicore/v1/physical-scale-candidates.json"}
        self.assertTrue(tools.issubset(package_tool.ALLOWED_TOOLS))
        self.assertTrue(required.issubset(package_tool.REQUIRED_MEMBERS))
        for member in required:
            with self.subTest(member=member):
                self.assertFalse(package_tool.is_excluded(member))


if __name__ == "__main__":
    unittest.main()
