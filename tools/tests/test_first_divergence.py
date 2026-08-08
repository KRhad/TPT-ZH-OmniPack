from __future__ import annotations

import importlib.util
from pathlib import Path
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[2]
COMPARATOR_PATH = ROOT / "tools" / "differential_compare.py"
LUA_PATH = ROOT / "tools" / "runtime" / "differential_probe.lua"
WRAPPER_PATH = ROOT / "tools" / "runtime_first_divergence.ps1"
GITIGNORE_PATH = ROOT / ".gitignore"
SPEC = importlib.util.spec_from_file_location("differential_compare", COMPARATOR_PATH)
assert SPEC and SPEC.loader
comparator = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(comparator)


TRACE_HEADER = (
    "step,state_hash_fnv1a32,particles,rng_a,rng_b,rng_c,rng_d\n"
)
PARTICLE_HEADER = ",".join(comparator.PARTICLE_FIELDS) + "\n"
CELL_HEADER = ",".join(comparator.CELL_FIELDS) + "\n"


class FirstDivergenceComparatorTest(unittest.TestCase):
    def test_trace_finds_exact_first_divergence(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            left = root / "left.csv"
            right = root / "right.csv"
            left.write_text(
                TRACE_HEADER
                + "0,10,2,1,2,3,4\n"
                + "1,11,2,5,6,7,8\n"
                + "2,12,2,9,10,11,12\n",
                encoding="utf-8",
            )
            right.write_text(
                TRACE_HEADER
                + "0,10,2,1,2,3,4\n"
                + "1,11,2,5,6,7,8\n"
                + "2,99,3,9,10,11,12\n",
                encoding="utf-8",
            )
            result = comparator.compare_traces(left, right)

        self.assertTrue(result["divergence_found"])
        self.assertEqual(result["first_divergence"]["step"], 2)
        self.assertEqual(
            result["first_divergence"]["fields"],
            ["state_hash_fnv1a32", "particles"],
        )

    def test_nonsequential_trace_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "trace.csv"
            path.write_text(
                TRACE_HEADER + "0,10,2,1,2,3,4\n2,11,2,5,6,7,8\n",
                encoding="utf-8",
            )
            with self.assertRaisesRegex(ValueError, "non-sequential"):
                comparator.read_trace(path)

    def test_capture_reports_particle_cell_and_neighbor_state(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            left_particles = root / "left-particles.csv"
            right_particles = root / "right-particles.csv"
            left_cells = root / "left-cells.csv"
            right_cells = root / "right-cells.csv"

            left_particles.write_text(
                PARTICLE_HEADER
                + "7,1,DEFAULT_PT_DUST,0,0,8,8,1,2,295.15,0,0,0,0,0,0\n"
                + "8,2,DEFAULT_PT_WATR,0,0,9,8,0,0,295.15,0,0,0,0,0,0\n",
                encoding="utf-8",
            )
            right_particles.write_text(
                PARTICLE_HEADER
                + "7,1,DEFAULT_PT_DUST,0,0,8,8,1.25,2,295.15,0,0,0,0,0,0\n"
                + "8,2,DEFAULT_PT_WATR,0,0,9,8,0,0,295.15,0,0,0,0,0,0\n",
                encoding="utf-8",
            )
            left_cells.write_text(
                CELL_HEADER + "2,2,1,0,0,295.15,0,0,0,0,0,4294967295,0,0\n",
                encoding="utf-8",
            )
            right_cells.write_text(
                CELL_HEADER + "2,2,1.5,0,0,295.15,0,0,0,0,0,4294967295,0,0\n",
                encoding="utf-8",
            )
            result = comparator.compare_captures(
                left_particles,
                right_particles,
                left_cells,
                right_cells,
                step=1,
                left_hash=10,
                right_hash=20,
            )

        self.assertFalse(result["unexplained_hash_divergence"])
        self.assertEqual(result["difference_field"]["primary_domain"], "particle")
        self.assertEqual(result["difference_field"]["particle_id"], 7)
        self.assertEqual(result["difference_field"]["particle_fields"], ["vx"])
        self.assertEqual(result["differing_particle_ids"], 1)
        self.assertEqual(result["differing_atmosphere_cells"], 1)
        self.assertEqual(
            result["particle_atmosphere_cells"][0]["fields"], ["pressure"]
        )
        self.assertEqual(
            [row["id"] for row in result["neighbor_state"]["left"]], [7, 8]
        )

    def test_nonfinite_capture_value_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "particles.csv"
            path.write_text(
                PARTICLE_HEADER
                + "7,1,DEFAULT_PT_DUST,0,0,8,8,nan,2,295.15,0,0,0,0,0,0\n",
                encoding="utf-8",
            )
            with self.assertRaisesRegex(ValueError, "non-finite"):
                comparator.read_particles(path)


class FirstDivergenceSourceContractTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.lua = LUA_PATH.read_text(encoding="utf-8")
        cls.wrapper = WRAPPER_PATH.read_text(encoding="utf-8")
        cls.gitignore = GITIGNORE_PATH.read_text(encoding="utf-8")

    def test_probe_records_each_tick_and_reproduces_capture_step(self) -> None:
        self.assertIn('trace:write("step,state_hash_fnv1a32', self.lua)
        self.assertIn("for step = 1, trace_steps do", self.lua)
        self.assertIn("for _ = 1, capture_step do", self.lua)
        self.assertIn("sim.updateUpTo()", self.lua)
        self.assertIn("sim.hash()", self.lua)
        self.assertIn("sim.randomSeed", self.lua)
        self.assertIn("Differential capture did not reproduce trace", self.wrapper)

    def test_capture_exports_particle_atmosphere_and_neighbor_inputs(self) -> None:
        for field in comparator.PARTICLE_FIELDS:
            self.assertIn(field, self.lua)
        for field in comparator.CELL_FIELDS:
            self.assertIn(field, self.lua)
        for getter in (
            "sim.pressure",
            "sim.velocityX",
            "sim.velocityY",
            "sim.ambientHeat",
            "sim.wallMap",
            "sim.elecMap",
            "sim.gravityField",
        ):
            self.assertIn(getter, self.lua)
        self.assertIn("neighbor_state", COMPARATOR_PATH.read_text(encoding="utf-8"))
        self.assertIn("particle_atmosphere_cells", COMPARATOR_PATH.read_text(encoding="utf-8"))

    def test_wrapper_is_private_isolated_and_fail_closed(self) -> None:
        self.assertIn(
            '[string] $OutputDirectory = "artifacts/vnext-differential"',
            self.wrapper,
        )
        self.assertIn("/artifacts/", self.gitignore)
        self.assertIn("function Remove-IsolatedRoot", self.wrapper)
        self.assertIn("$attempt -le 20", self.wrapper)
        self.assertIn("CreateNoWindow = $true", self.wrapper)
        self.assertIn("$process.Dispose()", self.wrapper)
        self.assertIn("unexplained_hash_divergence", self.wrapper)
        self.assertIn("did not explain the hash divergence", self.wrapper)
        for secret in ("GITHUB_PAT_TOKEN", "GITHUB_TOKEN", "GH_TOKEN"):
            self.assertIn(secret, self.wrapper)

    def test_wrapper_binds_source_build_binary_and_tool_hashes(self) -> None:
        for field in (
            "worktree_state_sha256",
            "lua_sha256",
            "comparator_sha256",
            "wrapper_sha256",
            "executable_sha256",
            "fp_mode",
            "simulation_compile_command",
            "artifact_files",
            'comparison_kind = "same_source_cpu_fp_mode"',
            'performance_gate = "not_evaluated"',
            "portable_runtime_tested = $false",
        ):
            self.assertIn(field, self.wrapper)


if __name__ == "__main__":
    unittest.main()
