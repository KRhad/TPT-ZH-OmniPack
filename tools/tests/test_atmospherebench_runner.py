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
        self.assertIn("tools/atmospherebench/Rusanov1D.cpp", RUNNER)
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

    def test_runner_hashes_source_state_on_windows_powershell_5_1(self) -> None:
        self.assertIn("function Get-TextSha256", RUNNER)
        self.assertIn("SHA256Managed", RUNNER)
        self.assertIn("ComputeHash($bytes)", RUNNER)
        self.assertNotIn("SHA256]::HashData", RUNNER)
        self.assertNotIn("$buildSystemFiles = @(", RUNNER)
        self.assertNotIn("$targets = @($targetsText", RUNNER)

    def test_runner_rechecks_source_after_measurement(self) -> None:
        self.assertIn("$finalSourceState = Get-SourceState", RUNNER)
        self.assertIn("source changed during build or measurement", RUNNER)
        self.assertIn("source_stable_through_measurement = $true", RUNNER)

    def test_runner_records_unselected_contract_only_status(self) -> None:
        self.assertIn("not_evaluated_contract_only", RUNNER)
        self.assertIn("$candidateImplementations = \"none\"", RUNNER)
        self.assertIn("standalone_contract_uniform_no_solver_step", RUNNER)
        self.assertIn("artifacts/vnext-atmospherebench", RUNNER)

    def test_runner_has_an_explicit_rusanov_probe_mode(self) -> None:
        self.assertIn("[switch] $RunRusanovUniform", RUNNER)
        self.assertIn("--run-rusanov-uniform", RUNNER)
        self.assertIn("atmospherebench_rusanov_uniform_probe", RUNNER)
        self.assertIn("not_evaluated_candidate_probe", RUNNER)
        self.assertIn("candidate_implementations = $candidateImplementations", RUNNER)
        self.assertIn("Rusanov probe must not claim solver selection", RUNNER)
        self.assertIn("first-order strict-double 1D periodic uniform probe", RUNNER)
        self.assertIn("[double]::IsNaN($maximumCfl)", RUNNER)
        self.assertIn("[double]::IsInfinity($maximumCfl)", RUNNER)
        self.assertIn('candidate=fvm_hlle|status=registered_only|solver_implemented=false', RUNNER)
        self.assertIn('candidate=lbm_d2q9|status=registered_only|solver_implemented=false', RUNNER)

    def test_runner_rejects_a_dimensional_or_stepped_scaffold_case(self) -> None:
        self.assertIn("must keep the shared case nondimensional", RUNNER)
        self.assertIn("must not claim a solver step", RUNNER)
        self.assertIn("shared grid contract drifted", RUNNER)
        self.assertIn("state_bytes_per_cell", RUNNER)
        self.assertIn("numerical_correction_ledger", RUNNER)
        self.assertIn("correction_energy_removed", RUNNER)
        self.assertIn("correction_event_count", RUNNER)

    def test_runner_has_no_production_client_launch_or_legacy_air_binding(self) -> None:
        forbidden = ("tpt-zh-omnipack", "updateUpTo", "Air.cpp", "Simulation.cpp", "LuaScriptInterface")
        for needle in forbidden:
            with self.subTest(needle=needle):
                self.assertNotIn(needle, RUNNER)


if __name__ == "__main__":
    unittest.main()
