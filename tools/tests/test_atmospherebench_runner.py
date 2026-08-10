from __future__ import annotations

from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[2]
RUNNER = (ROOT / "tools" / "run_atmospherebench.ps1").read_text(encoding="utf-8")


class AtmosphereBenchRunnerContractTests(unittest.TestCase):
    def test_runner_requires_clean_source_and_strict_build_flags(self) -> None:
        self.assertIn("requires a clean source worktree", RUNNER)
        self.assertIn("-fno-fast-math", RUNNER)
        self.assertIn("-ffp-contract=off", RUNNER)
        self.assertIn("/fp:strict", RUNNER)
        self.assertIn("inherited a Legacy fast-math flag", RUNNER)

    def test_runner_rebuilds_and_binds_the_build_tree_to_current_source(self) -> None:
        self.assertIn('"compile" "-C" $resolvedBuildDirectory "atmospherebench"', RUNNER)
        self.assertIn("build directory is configured from a different source root", RUNNER)
        self.assertIn("compile commands are not bound to current source", RUNNER)
        self.assertIn("rebuilt_before_measurement = $true", RUNNER)
        self.assertIn("source_root_verified = $true", RUNNER)

    def test_runner_executes_only_the_unique_meson_target_output(self) -> None:
        self.assertIn('"introspect" "--targets" $resolvedBuildDirectory', RUNNER)
        self.assertIn('$_.name -eq "atmospherebench"', RUNNER)
        self.assertIn("AtmosphereBench Meson target must resolve uniquely", RUNNER)
        self.assertIn("Requested executable does not match the freshly built AtmosphereBench Meson target", RUNNER)
        self.assertIn("meson_target_output = $resolvedExecutable", RUNNER)

    def test_runner_records_its_explicit_git_provenance(self) -> None:
        self.assertIn("[string] $GitExecutable = \"git\"", RUNNER)
        self.assertIn("git_executable = $gitCommand", RUNNER)
        self.assertIn("git_sha256 = Get-Sha256 -Path $gitCommand", RUNNER)

    def test_runner_rechecks_source_after_measurement(self) -> None:
        self.assertIn("$finalSourceState = Get-SourceState", RUNNER)
        self.assertIn("source changed during build or measurement", RUNNER)
        self.assertIn("source_stable_through_measurement = $true", RUNNER)

    def test_runner_records_unselected_contract_only_status(self) -> None:
        self.assertIn("not_evaluated_contract_only", RUNNER)
        self.assertIn("candidate_implementations = \"none\"", RUNNER)
        self.assertIn("standalone_contract_uniform_no_solver_step", RUNNER)
        self.assertIn("artifacts/vnext-atmospherebench", RUNNER)

    def test_runner_has_no_production_client_launch_or_legacy_air_binding(self) -> None:
        forbidden = ("tpt-zh-omnipack", "updateUpTo", "Air.cpp", "Simulation.cpp", "LuaScriptInterface")
        for needle in forbidden:
            with self.subTest(needle=needle):
                self.assertNotIn(needle, RUNNER)


if __name__ == "__main__":
    unittest.main()
