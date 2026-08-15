from __future__ import annotations

import hashlib
import importlib.util
from pathlib import Path
import sys
import tempfile
import unittest
import zipfile


ROOT = Path(__file__).resolve().parents[2]


def import_script(name: str, path: Path):
    spec = importlib.util.spec_from_file_location(name, path)
    assert spec and spec.loader
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


audit = import_script(
    "release_soak_semantics_audit", ROOT / "tools" / "release_validation_audit.py"
)


class ReleaseSoakSemanticTests(unittest.TestCase):
    COMMIT = "a" * 40

    def make_case(self, root: Path) -> tuple[Path, str, dict[str, object]]:
        candidate = root / "candidate.zip"
        executable = b"current candidate executable"
        with zipfile.ZipFile(candidate, "w", zipfile.ZIP_DEFLATED) as archive:
            archive.writestr("candidate/tpt-zh-omnipack.exe", executable)
        candidate_sha = hashlib.sha256(candidate.read_bytes()).hexdigest().upper()
        executable_sha = hashlib.sha256(executable).hexdigest().upper()
        raw: dict[str, object] = {
            "status": "PASS", "passed": True,
            "sample_id": "S20-FULL-CATALOG", "long_run_requested": True,
            "wall_clock_seconds": 7200.0, "warmup_seconds": 60.0,
            "sample_seconds": 7200.0, "heartbeat_interval_seconds": 60.0,
            "maximum_heartbeat_gap_seconds": 60.0,
            "long_run_gate_pass": True, "performance_gate_pass": True,
            "duration_pass": True, "runtime_pass": True,
            "sample_execution_pass": True, "event_evidence_complete": True,
            "signal_evidence_complete": True, "signal_behavior_pass": True,
            "signal_stop_pass": True, "fixture_evidence_complete": True,
            "fixture_activity_pass": True, "scenario_behavior_pass": True,
            "long_run_evidence_complete": True, "long_run_duration_pass": True,
            "long_run_behavior_pass": True,
            "long_run_save_load_cycles": 10, "long_run_language_switches": 10,
            "long_run_module_toggle_cycles": 10,
            "long_run_settings_recovery_pass": True,
            "omni_atmosphere_active": True, "public_zip_sha256": candidate_sha,
            "source_commit": self.COMMIT, "nan_count": 0, "inf_count": 0,
            "stalls": 0, "simulation_steps": 1000, "heartbeat_count": 121,
            "heartbeat_progress_pass": True, "heartbeat_timing_pass": True,
            "finite_state_pass": True, "atmosphere_range_pass": True,
            "atmosphere_mass_closure_pass": True, "heartbeat_summary_match": True,
            "long_run_diagnostics_pass": True,
            "candidate_extracted_to_fresh_directory": True,
            "candidate_target_executable_count": 1,
            "executable_source": "candidate_zip",
            "candidate_executable_sha256": executable_sha,
            "build_executable_sha256": executable_sha, "exe_sha256": executable_sha,
            "peak_working_set_bytes": 128 * 1024 * 1024,
            "peak_private_bytes": 96 * 1024 * 1024,
            "frame_samples": 7200, "process_samples": 7200,
            "memory_process_samples": 7200,
            "observed_particle_min": 100, "observed_particle_max": 200,
            "particle_tail_first": 150, "particle_tail_last": 150,
            "unbounded_growth": False,
            "working_set_tail_first": 100 * 1024 * 1024,
            "working_set_tail_last": 100 * 1024 * 1024,
            "private_tail_first": 80 * 1024 * 1024,
            "private_tail_last": 80 * 1024 * 1024,
            "memory_leak_suspected": False,
            "atmosphere_mass_initial_kg": 1.0,
            "atmosphere_mass_final_kg": 1.0,
            "atmosphere_mass_min_kg": 1.0, "atmosphere_mass_max_kg": 1.0,
            "atmosphere_mass_residual_abs_max_kg": 0.0,
            "species_mass_residual_abs_max_kg": 0.0,
            "minimum_density_kg_m3": 1.0, "maximum_density_kg_m3": 1.0,
            "minimum_pressure_pa": 100000.0, "maximum_pressure_pa": 100000.0,
            "minimum_temperature_k": 293.15, "maximum_temperature_k": 293.15,
            "result_json_sha256": "8" * 64,
            "input_ops": {"bytes": 128, "sha256": "6" * 64},
            "output_ops": {"bytes": 256, "sha256": "7" * 64},
        }
        return candidate, candidate_sha, raw

    def validate(self, root: Path, candidate: Path, candidate_sha: str,
                 raw: dict[str, object]) -> list[str]:
        return audit.validate_raw_semantics(
            "Soak2Hours", raw, root=root, candidate_sha256=candidate_sha,
            commit=self.COMMIT, candidate_path=candidate,
        )

    def test_complete_soak_summary_is_accepted(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            candidate, candidate_sha, raw = self.make_case(root)
            self.assertEqual(self.validate(root, candidate, candidate_sha, raw), [])

    def test_nan_wall_clock_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            candidate, candidate_sha, raw = self.make_case(root)
            raw["wall_clock_seconds"] = float("nan")
            errors = self.validate(root, candidate, candidate_sha, raw)
            self.assertTrue(any("wall clock" in error for error in errors))

    def test_memory_leak_claim_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            candidate, candidate_sha, raw = self.make_case(root)
            raw["memory_leak_suspected"] = True
            errors = self.validate(root, candidate, candidate_sha, raw)
            self.assertTrue(any("memory diagnostics" in error for error in errors))

    def test_mass_residual_out_of_bounds_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            candidate, candidate_sha, raw = self.make_case(root)
            raw["species_mass_residual_abs_max_kg"] = 1.0
            errors = self.validate(root, candidate, candidate_sha, raw)
            self.assertTrue(any("species_mass_residual" in error for error in errors))


if __name__ == "__main__":
    unittest.main()
