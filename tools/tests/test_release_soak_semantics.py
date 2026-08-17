from __future__ import annotations

import hashlib
import importlib.util
import json
from pathlib import Path
import sys
import tempfile
import unittest
import zipfile
from datetime import datetime, timedelta, timezone


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
        finished_at = datetime.now(timezone.utc) - timedelta(seconds=1)
        started_at = finished_at - timedelta(seconds=7200)
        started = started_at.isoformat()
        finished = finished_at.isoformat()
        self.RELEASE_RUN_ID = (
            started_at.strftime("%Y%m%dT%H%M%SZ") + "-8f31c1c7"
        )
        self.HARNESS_RUN_ID = (
            started_at.strftime("%Y%m%dT%H%M%SZ") + "-7f31c1c7"
        )
        candidate = root / "candidate.zip"
        executable = b"current candidate executable"
        with zipfile.ZipFile(candidate, "w", zipfile.ZIP_DEFLATED) as archive:
            archive.writestr("candidate/tpt-zh-omnipack.exe", executable)
        candidate_sha = hashlib.sha256(candidate.read_bytes()).hexdigest().upper()
        executable_sha = hashlib.sha256(executable).hexdigest().upper()
        raw: dict[str, object] = {
            "schema": "omnipack-release-evidence", "schema_version": 1,
            "test": "soak_2h", "gate_name": "Soak2Hours",
            "run_id": self.RELEASE_RUN_ID, "commit": self.COMMIT,
            "candidate_sha256": candidate_sha,
            "gate_started_at": started,
            "gate_finished_at": finished,
            "status": "PASS", "passed": True,
            "release_binding_passed": True, "analysis_exit_code": 0,
            "analyzer_schema": "omnipack-release-evidence",
            "analyzer_schema_version": 1, "analyzer_test": "soak_2h",
            "analyzer_status": "PASS", "analyzer_passed": True,
            "harness_run_id": self.HARNESS_RUN_ID,
            "artifact_run_id": self.HARNESS_RUN_ID,
            "analyzer_candidate_sha256": candidate_sha,
            "observed_candidate_sha256": candidate_sha,
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
            "input_ops": {"bytes": 128, "sha256": "6" * 64},
            "output_ops": {"bytes": 256, "sha256": "7" * 64},
        }
        result_path = (
            root / "soak" / "FIXTURE-0123456789AB" /
            "S20-FULL-CATALOG" / self.HARNESS_RUN_ID / "result.json"
        )
        result_path.parent.mkdir(parents=True)
        result_path.write_text(json.dumps({
            "schema_version": 1,
            "sample_id": raw["sample_id"], "run_id": self.HARNESS_RUN_ID,
            "source_commit": raw["source_commit"],
            "public_zip_sha256": raw["public_zip_sha256"],
            "exe_sha256": raw["exe_sha256"],
            "wall_clock_seconds": raw["wall_clock_seconds"],
            "warmup_seconds": raw["warmup_seconds"],
            "sample_seconds": raw["sample_seconds"],
            "start_time_utc": started,
            "end_time_utc": finished,
            "simulation_steps": raw["simulation_steps"],
            "nan_count": raw["nan_count"], "inf_count": raw["inf_count"],
            "stalls": raw["stalls"],
            "omni_atmosphere_active": raw["omni_atmosphere_active"],
            "long_run": True, "smoke_run": False,
        }) + "\n", encoding="utf-8")
        raw["result_json"] = result_path.relative_to(root).as_posix()
        raw["result_json_sha256"] = hashlib.sha256(
            result_path.read_bytes()
        ).hexdigest().upper()
        analyzer_path = root / "soak-analyzer.json"
        analyzer_value = dict(raw)
        analyzer_value["run_id"] = self.HARNESS_RUN_ID
        analyzer_value["candidate_sha256"] = candidate_sha
        analyzer_path.write_text(
            json.dumps(analyzer_value) + "\n", encoding="utf-8"
        )
        raw["analyzer_evidence"] = analyzer_path.name
        raw["analyzer_evidence_sha256"] = hashlib.sha256(
            analyzer_path.read_bytes()
        ).hexdigest().upper()
        return candidate, candidate_sha, raw

    def make_envelope(self, root: Path, raw: dict[str, object],
                      candidate_sha: str) -> tuple[dict[str, object], Path]:
        raw_path = root / "soak-2h.json"
        raw_path.write_text(json.dumps(raw) + "\n", encoding="utf-8")
        envelope = {
            "schema": "omnipack-release-evidence", "schema_version": 1,
            "test": "soak_2h", "gate_name": "Soak2Hours",
            "run_id": self.RELEASE_RUN_ID, "commit": self.COMMIT,
            "candidate_sha256": candidate_sha,
            "status": "PASS", "passed": True, "exit_code": 0,
            "gate_started_at": raw["gate_started_at"],
            "gate_finished_at": raw["gate_finished_at"],
            "source_evidence": raw_path.name,
            "source_evidence_sha256": hashlib.sha256(raw_path.read_bytes()).hexdigest().upper(),
        }
        return envelope, raw_path

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

    def test_complete_soak_gate_envelope_is_accepted(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            candidate, candidate_sha, raw = self.make_case(root)
            envelope, _ = self.make_envelope(root, raw, candidate_sha)
            errors = audit.validate_gate_evidence(
                "Soak2Hours", {"Status": "PASS"}, envelope, root=root,
                run_id=self.RELEASE_RUN_ID, commit=self.COMMIT,
                candidate_sha256=candidate_sha, symbols_sha256=None,
                symbols_member_sha256=None, candidate_path=candidate,
            )
            self.assertEqual(errors, [])

    def test_release_run_identity_attack_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            candidate, candidate_sha, raw = self.make_case(root)
            raw["run_id"] = "20260814T041501Z-deadbeef"
            envelope, _ = self.make_envelope(root, raw, candidate_sha)
            errors = audit.validate_gate_evidence(
                "Soak2Hours", {"Status": "PASS"}, envelope, root=root,
                run_id=self.RELEASE_RUN_ID, commit=self.COMMIT,
                candidate_sha256=candidate_sha, symbols_sha256=None,
                symbols_member_sha256=None, candidate_path=candidate,
            )
            self.assertTrue(any("source evidence run_id" in error for error in errors))

    def test_candidate_identity_attack_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            candidate, candidate_sha, raw = self.make_case(root)
            raw["analyzer_candidate_sha256"] = "F" * 64
            errors = self.validate(root, candidate, candidate_sha, raw)
            self.assertTrue(any("observed/analyzer candidate" in error for error in errors))

    def test_harness_artifact_run_mismatch_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            candidate, candidate_sha, raw = self.make_case(root)
            raw["artifact_run_id"] = "20260814T041601Z-deadbeef"
            errors = self.validate(root, candidate, candidate_sha, raw)
            self.assertTrue(any("harness run identity" in error for error in errors))

    def test_result_json_bytes_are_hash_bound(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            candidate, candidate_sha, raw = self.make_case(root)
            result_path = root / str(raw["result_json"])
            result_path.write_bytes(result_path.read_bytes() + b" ")
            errors = self.validate(root, candidate, candidate_sha, raw)
            self.assertTrue(any("bytes do not match" in error for error in errors))

    def test_result_json_harness_run_is_semantically_bound(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            candidate, candidate_sha, raw = self.make_case(root)
            result_path = root / str(raw["result_json"])
            value = json.loads(result_path.read_text(encoding="utf-8"))
            value["run_id"] = "20260814T041601Z-deadbeef"
            result_path.write_text(json.dumps(value) + "\n", encoding="utf-8")
            raw["result_json_sha256"] = hashlib.sha256(
                result_path.read_bytes()
            ).hexdigest().upper()
            errors = self.validate(root, candidate, candidate_sha, raw)
            self.assertTrue(any("result.json run_id" in error for error in errors))

    def test_analyzer_failure_cannot_be_relabelled_pass(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            candidate, candidate_sha, raw = self.make_case(root)
            raw["analyzer_status"] = "FAIL"
            raw["analyzer_passed"] = False
            raw["analysis_exit_code"] = 2
            errors = self.validate(root, candidate, candidate_sha, raw)
            self.assertTrue(any("analyzer schema or result" in error for error in errors))
            self.assertTrue(any("analyzer did not exit" in error for error in errors))

    def test_boolean_zero_values_do_not_impersonate_integer_counters(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            candidate, candidate_sha, raw = self.make_case(root)
            raw["analysis_exit_code"] = False
            raw["nan_count"] = False
            result_path = root / str(raw["result_json"])
            result = json.loads(result_path.read_text(encoding="utf-8"))
            result["nan_count"] = False
            result_path.write_text(json.dumps(result) + "\n", encoding="utf-8")
            raw["result_json_sha256"] = hashlib.sha256(
                result_path.read_bytes()
            ).hexdigest().upper()
            errors = self.validate(root, candidate, candidate_sha, raw)
            self.assertTrue(any("analyzer did not exit" in error for error in errors))
            self.assertTrue(any("non-finite values" in error for error in errors))

    def test_release_binding_failure_cannot_be_relabelled_pass(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            candidate, candidate_sha, raw = self.make_case(root)
            raw["release_binding_passed"] = False
            errors = self.validate(root, candidate, candidate_sha, raw)
            self.assertTrue(any("release-run identity binding" in error for error in errors))

    def test_missing_analyzer_binding_fields_are_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            candidate, candidate_sha, raw = self.make_case(root)
            for field in (
                "analyzer_schema", "analyzer_schema_version", "analyzer_test",
                "analyzer_status", "analyzer_passed", "analysis_exit_code",
                "harness_run_id", "artifact_run_id", "analyzer_candidate_sha256",
                "observed_candidate_sha256", "release_binding_passed",
            ):
                raw.pop(field)
            errors = self.validate(root, candidate, candidate_sha, raw)
            self.assertGreaterEqual(len(errors), 5)

    def test_nan_wall_clock_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            candidate, candidate_sha, raw = self.make_case(root)
            raw["wall_clock_seconds"] = float("nan")
            errors = self.validate(root, candidate, candidate_sha, raw)
            self.assertTrue(any("wall clock" in error for error in errors))

    def test_string_wall_clock_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            candidate, candidate_sha, raw = self.make_case(root)
            raw["wall_clock_seconds"] = "7200"
            result_path = root / str(raw["result_json"])
            result = json.loads(result_path.read_text(encoding="utf-8"))
            result["wall_clock_seconds"] = "7200"
            result_path.write_text(json.dumps(result) + "\n", encoding="utf-8")
            raw["result_json_sha256"] = hashlib.sha256(
                result_path.read_bytes()
            ).hexdigest().upper()
            errors = self.validate(root, candidate, candidate_sha, raw)
            self.assertTrue(any("wall clock" in error for error in errors))

    def test_claimed_duration_must_match_timestamp_span(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            candidate, candidate_sha, raw = self.make_case(root)
            result_path = root / str(raw["result_json"])
            result = json.loads(result_path.read_text(encoding="utf-8"))
            result["start_time_utc"] = result["end_time_utc"]
            result_path.write_text(json.dumps(result) + "\n", encoding="utf-8")
            raw["result_json_sha256"] = hashlib.sha256(
                result_path.read_bytes()
            ).hexdigest().upper()
            errors = self.validate(root, candidate, candidate_sha, raw)
            self.assertTrue(any("timestamps span" in error for error in errors))
            self.assertTrue(any("does not match" in error for error in errors))

    def test_release_run_id_timestamp_is_bound_to_gate_interval(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            candidate, candidate_sha, raw = self.make_case(root)
            raw["run_id"] = "20000101T000000Z-8f31c1c7"
            errors = self.validate(root, candidate, candidate_sha, raw)
            self.assertTrue(any("release run ID timestamp" in error for error in errors))

    def test_result_json_must_use_canonical_soak_hierarchy(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            candidate, candidate_sha, raw = self.make_case(root)
            original = root / str(raw["result_json"])
            relocated = root / "unrelated" / self.HARNESS_RUN_ID / "result.json"
            relocated.parent.mkdir(parents=True)
            relocated.write_bytes(original.read_bytes())
            raw["result_json"] = relocated.relative_to(root).as_posix()
            raw["result_json_sha256"] = hashlib.sha256(
                relocated.read_bytes()
            ).hexdigest().upper()
            errors = self.validate(root, candidate, candidate_sha, raw)
            self.assertTrue(any("canonical" in error for error in errors))

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
