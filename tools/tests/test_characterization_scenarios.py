from __future__ import annotations

import json
from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[2]
CASES = ROOT / "tools" / "runtime" / "characterization_cases.json"
LUA = ROOT / "tools" / "runtime" / "characterization_scenarios.lua"
POWERSHELL = ROOT / "tools" / "runtime_generate_characterization.ps1"
GITIGNORE = ROOT / ".gitignore"


EXPECTED = {
    "C01": "sand",
    "C02": "water",
    "C03": "gas",
    "C04": "fire",
    "C05": "explosion",
    "C06": "vacuum",
    "C07": "pressure",
    "C08": "heat",
    "C09": "electrical",
    "C10": "photons",
    "C11": "pipe",
    "C12": "complex-electronics",
    "C13": "mixed",
    "C14": "maximum-particle",
}


class CharacterizationScenarioContractTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.cases_text = CASES.read_text(encoding="utf-8")
        cls.cases = json.loads(cls.cases_text)
        cls.lua = LUA.read_text(encoding="utf-8")
        cls.powershell = POWERSHELL.read_text(encoding="utf-8")
        cls.gitignore = GITIGNORE.read_text(encoding="utf-8")

    def test_manifest_is_exactly_c01_through_c14(self) -> None:
        self.assertEqual(self.cases["schema_version"], 1)
        self.assertEqual(self.cases["suite_id"], "legacy-c01-c14-v1")
        actual = {case["id"]: case["slug"] for case in self.cases["cases"]}
        self.assertEqual(actual, EXPECTED)
        self.assertEqual(len(actual), 14)

    def test_every_case_has_fixed_seed_steps_and_invariants(self) -> None:
        seeds: set[tuple[int, ...]] = set()
        for case in self.cases["cases"]:
            seed = tuple(case["seed"])
            self.assertEqual(len(seed), 4)
            self.assertNotIn(seed, seeds)
            seeds.add(seed)
            self.assertGreaterEqual(case["warmup_steps"], 0)
            self.assertGreater(case["trace_steps"], 0)
            self.assertLessEqual(
                case["expected_min_particles"], case["expected_max_particles"]
            )
            self.assertTrue(case["required_identifiers"])
            self.assertTrue(case["purpose"].strip())
        maximum = next(case for case in self.cases["cases"] if case["id"] == "C14")
        self.assertEqual(maximum["expected_min_particles"], 235008)
        self.assertEqual(maximum["expected_max_particles"], 235008)

    def test_artifacts_are_explicitly_private_and_self_generated(self) -> None:
        self.assertEqual(
            self.cases["artifact_policy"],
            "private_local_not_for_public_release",
        )
        self.assertIn("self-generated", self.cases["provenance"])
        self.assertIn("no user or third-party saves", self.cases["provenance"])
        self.assertIn("/artifacts/", self.gitignore)
        self.assertIn(
            '[string] $OutputDirectory = "artifacts/vnext-characterization"',
            self.powershell,
        )

    def test_lua_implements_every_generator_and_pristine_reset(self) -> None:
        for case_id in EXPECTED:
            self.assertIn(f"local function generate_{case_id.lower()}()", self.lua)
            self.assertIn(f"{case_id} = generate_{case_id.lower()}", self.lua)
        self.assertIn("local function reset_world()", self.lua)
        self.assertIn("clearSim leaves derived Air blocking maps", self.lua)
        self.assertIn("sim.updateUpTo()", self.lua)
        self.assertIn("reset_sanitization_steps", self.lua)

    def test_generation_saves_same_state_twice_without_mutation(self) -> None:
        self.assertEqual(self.lua.count("sim.saveStamp("), 2)
        self.assertIn("first_stamp", self.lua)
        self.assertIn("second_stamp", self.lua)
        self.assertIn(
            'assert(sim.hash() == hash_before_save, "saveStamp changed Simulation state")',
            self.lua,
        )
        self.assertIn(
            'assert(rng_text() == rng_before_save, "saveStamp changed Simulation RNG")',
            self.lua,
        )
        self.assertIn("same-state OPS logical payloads differ", self.powershell)
        self.assertIn("canonical_payload_sha256", self.powershell)

    def test_verification_uses_two_fresh_open_processes_and_exact_traces(self) -> None:
        self.assertIn('$verifyARoot = Join-Path $testRoot "verify-a"', self.powershell)
        self.assertIn('$verifyBRoot = Join-Path $testRoot "verify-b"', self.powershell)
        self.assertIn('$startInfo.ArgumentList.Add("open")', self.powershell)
        self.assertIn("Cross-restart characterization traces differ", self.powershell)
        self.assertIn("Cross-restart characterization field differs", self.powershell)
        self.assertIn("OPS load did not restore RNG state", self.powershell)
        self.assertIn("trace_line_count", self.powershell)
        self.assertIn("process_count = 2", self.powershell)

    def test_verifier_requires_deterministic_save_settings(self) -> None:
        self.assertIn('assert(sim.paused(), "loaded characterization save is not paused")', self.lua)
        self.assertIn("assert(sim.ensureDeterminism()", self.lua)
        for setting in (
            "sim.edgeMode()",
            "sim.gravityMode()",
            "sim.airMode()",
            "sim.ambientHeatSim()",
            "sim.heatSim()",
        ):
            self.assertIn(setting, self.lua)
        self.assertIn("loaded_rng", self.lua)
        self.assertIn("loaded_hash", self.lua)
        self.assertIn("final_hash", self.lua)

    def test_wrapper_binds_source_build_binary_and_ops(self) -> None:
        for text in (
            "worktree_state_sha256",
            "lua_sha256",
            "case_definitions_sha256",
            "wrapper_sha256",
            "simulation_compile_command",
            "Characterization baselines require fp_mode=legacy_fast",
            'format = "OPS1+bzip2"',
            "payload_length_bytes",
            "same_state_logical_payload_equal",
            "OPS load changed required element counts",
            "snapshot_hash_equal",
            "Legacy OPS may normalize or quantize state",
        ):
            self.assertIn(text, self.powershell)
        for secret in ("GITHUB_PAT_TOKEN", "GITHUB_TOKEN", "GH_TOKEN"):
            self.assertIn(secret, self.powershell)

    def test_maximum_case_fills_exact_particle_capacity(self) -> None:
        maximum_body = self.lua[
            self.lua.index("local function generate_c14()") : self.lua.index(
                "local generators ="
            )
        ]
        self.assertIn("for y = 0, sim.YRES - 1", maximum_body)
        self.assertIn("for x = 0, sim.XRES - 1", maximum_body)
        self.assertIn("created == sim.MAX_PARTS", maximum_body)

    def test_electrical_lanes_do_not_overlap_insulation_barrier(self) -> None:
        electrical_body = self.lua[
            self.lua.index("local function generate_c09()") : self.lua.index(
                "local function generate_c10()"
            )
        ]
        self.assertIn(
            "fill_rectangle(70, y, 299, y + 1, ids.metl)", electrical_body
        )
        self.assertIn(
            "fill_rectangle(306, y, 540, y + 1, ids.metl)", electrical_body
        )
        self.assertIn("fill_rectangle(300, 60, 305, 320, ids.insl)", electrical_body)
        self.assertIn("create_particle(68, y, ids.btry)", electrical_body)
        self.assertNotIn(
            "fill_rectangle(70, y, 540, y + 1, ids.metl)", electrical_body
        )

    def test_complex_electronics_lanes_do_not_overlap_barrier(self) -> None:
        body = self.lua[
            self.lua.index("local function generate_c12()") : self.lua.index(
                "local function generate_c13()"
            )
        ]
        self.assertIn("fill_rectangle(70, y, 304, y + 1, ids.inwr)", body)
        self.assertIn("fill_rectangle(309, y, 540, y + 1, ids.inwr)", body)
        self.assertIn("fill_rectangle(305, 60, 308, 325, ids.insl)", body)
        self.assertIn("create_particle(68, y, ids.btry)", body)
        self.assertNotIn("fill_rectangle(70, y, 540, y + 1, ids.inwr)", body)

    def test_cleanup_is_bounded_and_processes_are_disposed(self) -> None:
        self.assertIn("function Remove-IsolatedRoot", self.powershell)
        self.assertIn("$attempt -le 20", self.powershell)
        self.assertIn("Refusing to remove characterization data", self.powershell)
        self.assertIn("$process.Dispose()", self.powershell)


if __name__ == "__main__":
    unittest.main()
