from __future__ import annotations

import csv
import hashlib
import importlib.util
import json
from pathlib import Path
import sys
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / "tools" / "analyze_stress_result.py"
SPEC = importlib.util.spec_from_file_location("stress_result_analysis", SCRIPT)
assert SPEC is not None and SPEC.loader is not None
analysis = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = analysis
SPEC.loader.exec_module(analysis)


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest().upper()


class AnalyzeStressResultTest(unittest.TestCase):
    def fixture(
        self,
        directory: Path,
        particles: list[int],
        smoke: bool = False,
        complete_gate_evidence: bool = False,
        sample_id: str = "S01-METALLURGY-LARGE",
        fixture_type_count: int | None = None,
        fixture_created_type_count: int | None = None,
        fixture_visible_type_count: int | None = None,
        long_run: bool = False,
    ) -> None:
        ops1 = b"OPS1" + b"\0" * 8 + b"BZh" + b"input"
        ops2 = b"OPS1" + b"\0" * 8 + b"BZh" + b"output"
        (directory / "input-first.stm").write_bytes(ops1)
        (directory / "output-second.stm").write_bytes(ops2)
        result = {
            "schema_version": 1,
            "sample_id": sample_id,
            "run_id": "fixture",
            "source_commit": "a" * 40,
            "public_zip_sha256": "B" * 64,
            "exe_sha256": "C" * 64,
            "input_ops_sha256": digest(ops1),
            "output_ops_second_sha256": digest(ops2),
            "warmup_seconds": 60.0 if long_run and not smoke else 0.0,
            "sample_seconds": 2.0 if smoke else (7200.0 if long_run else 30.0),
            "initial_particles": particles[0],
            "peak_particles": max(particles),
            "final_particles": particles[-1],
            "average_fps": 60.0,
            "one_percent_low_fps": 50.0,
            "minimum_fps": 40.0,
            "peak_working_set_bytes": 120000000,
            "peak_private_bytes": 110000000,
            "event_count_total": "not_tested",
            "event_count_peak_per_frame": "not_tested",
            "crashed": False,
            "hung": False,
            "roundtrip_pass": True,
            "long_run": long_run,
            "smoke_run": smoke,
        }
        if long_run:
            result.update(
                {
                    "long_run_save_load_cycles": 10,
                    "long_run_language_switches": 10,
                    "long_run_module_toggle_cycles": 10,
                    "long_run_settings_recovery_pass": True,
                    "long_run_checkpoint_save_ms_total": 100.0,
                    "long_run_checkpoint_load_ms_total": 80.0,
                }
            )
        if complete_gate_evidence:
            result.update(
                {
                    "event_count_total": 123,
                    "event_count_peak_per_frame": 7,
                    "scenario_stop_pass": True,
                    "scenario_recovery_pass": True,
                    "stop_event_delta": 0,
                    "scenario_recovery_assertions": 7,
                }
            )
        if fixture_type_count is not None:
            result["fixture_type_count"] = fixture_type_count
        if fixture_created_type_count is not None:
            result["fixture_created_type_count"] = fixture_created_type_count
        if fixture_visible_type_count is not None:
            result["fixture_visible_type_count"] = fixture_visible_type_count
        (directory / "result.json").write_text(
            json.dumps(result), encoding="utf-8"
        )
        with (directory / "frame-series.csv").open(
            "w", encoding="utf-8", newline=""
        ) as stream:
            writer = csv.writer(stream, lineterminator="\n")
            writer.writerow(("elapsed_seconds", "frames", "particles"))
            for index, value in enumerate(particles):
                writer.writerow((index * 10, index * 600, value))
        with (directory / "process-series.csv").open(
            "w", encoding="utf-8", newline=""
        ) as stream:
            writer = csv.writer(stream, lineterminator="\n")
            writer.writerow(
                ("elapsed_seconds", "working_set_bytes", "private_bytes")
            )
            for index in range(max(8, len(particles))):
                writer.writerow((index * 10, 120000000 - index, 110000000 - index))

    def write_process_series(
        self,
        directory: Path,
        working_set: list[int],
        private: list[int],
    ) -> None:
        self.assertEqual(len(working_set), len(private))
        with (directory / "process-series.csv").open(
            "w", encoding="utf-8", newline=""
        ) as stream:
            writer = csv.writer(stream, lineterminator="\n")
            writer.writerow(
                ("elapsed_seconds", "working_set_bytes", "private_bytes")
            )
            for index, (working, private_bytes) in enumerate(
                zip(working_set, private)
            ):
                writer.writerow((index, working, private_bytes))

    def test_stable_full_run_is_gated_by_evidence_completeness(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            self.fixture(directory, [100, 90, 80, 80, 80, 80, 80, 80])
            value = analysis.analyze(directory)
        self.assertFalse(value["unbounded_growth"])
        self.assertFalse(value["memory_leak_suspected"])
        self.assertTrue(value["sample_execution_pass"])
        self.assertFalse(value["event_evidence_complete"])
        self.assertFalse(value["performance_gate_pass"])
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            self.fixture(
                directory,
                [100, 90, 80, 80, 80, 80, 80, 80],
                complete_gate_evidence=True,
            )
            value = analysis.analyze(directory)
        self.assertTrue(value["event_evidence_complete"])
        self.assertTrue(value["scenario_behavior_pass"])
        self.assertTrue(value["performance_gate_pass"])

    def test_monotonic_tail_growth_is_reported(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            self.fixture(directory, [100, 90, 80, 81, 82, 83, 84, 85])
            value = analysis.analyze(directory)
        self.assertTrue(value["unbounded_growth"])
        self.assertFalse(value["sample_execution_pass"])

    def test_page_scale_memory_growth_does_not_trigger_leak_signal(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            self.fixture(
                directory,
                [100, 90, 80, 80, 80, 80, 80, 80],
                complete_gate_evidence=True,
            )
            self.write_process_series(
                directory,
                [120000000] * 7 + [120049152],
                [110000000] * 7 + [110049152],
            )
            value = analysis.analyze(directory)
        self.assertEqual(value["working_set_tail_growth_bytes"], 49152)
        self.assertEqual(value["private_tail_growth_bytes"], 49152)
        self.assertFalse(value["memory_leak_suspected"])
        self.assertTrue(value["performance_gate_pass"])

    def test_material_monotonic_memory_growth_triggers_leak_signal(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            self.fixture(
                directory,
                [100, 90, 80, 80, 80, 80, 80, 80],
                complete_gate_evidence=True,
            )
            self.write_process_series(
                directory,
                [120000000 + index * 2000000 for index in range(8)],
                [110000000 + index * 2000000 for index in range(8)],
            )
            value = analysis.analyze(directory)
        self.assertEqual(value["memory_growth_minimum_bytes"], 1048576)
        self.assertEqual(value["memory_growth_minimum_ratio"], 0.01)
        self.assertTrue(value["memory_leak_suspected"])
        self.assertFalse(value["sample_execution_pass"])
        self.assertFalse(value["performance_gate_pass"])

    def test_smoke_run_cannot_be_an_execution_pass(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            self.fixture(directory, [100, 100, 100], smoke=True)
            value = analysis.analyze(directory)
        self.assertFalse(value["duration_pass"])
        self.assertFalse(value["sample_execution_pass"])

    def test_standard_gate_requires_at_least_thirty_seconds(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            self.fixture(
                directory,
                [100, 90, 80, 80, 80, 80, 80, 80],
                complete_gate_evidence=True,
            )
            result_path = directory / "result.json"
            result = json.loads(result_path.read_text(encoding="utf-8"))
            result["sample_seconds"] = 29.999
            result_path.write_text(json.dumps(result), encoding="utf-8")
            value = analysis.analyze(directory)
            self.assertFalse(value["duration_pass"])
            self.assertFalse(value["performance_gate_pass"])
            result["sample_seconds"] = 30.0
            result_path.write_text(json.dumps(result), encoding="utf-8")
            value = analysis.analyze(directory)
        self.assertTrue(value["duration_pass"])
        self.assertTrue(value["performance_gate_pass"])

    def test_ops_hash_tampering_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            self.fixture(directory, [100, 90, 90, 90])
            (directory / "input-first.stm").write_bytes(b"tampered")
            with self.assertRaisesRegex(ValueError, "OPS1/BZip2"):
                analysis.analyze(directory)

    def test_automation_sample_requires_bounded_signal_evidence(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            self.fixture(
                directory,
                [100, 90, 80, 80, 80, 80, 80, 80],
                complete_gate_evidence=True,
            )
            result_path = directory / "result.json"
            result = json.loads(result_path.read_text(encoding="utf-8"))
            result.update(
                sample_id="S11-AUTOMATION-FACTORY",
                signal_count_total=100,
                signal_count_peak_per_frame=10,
                signal_stop_pass=True,
            )
            result_path.write_text(json.dumps(result), encoding="utf-8")
            value = analysis.analyze(directory)
            self.assertTrue(value["signal_behavior_pass"])
            self.assertTrue(value["performance_gate_pass"])
            result["signal_stop_pass"] = False
            result_path.write_text(json.dumps(result), encoding="utf-8")
            value = analysis.analyze(directory)
        self.assertFalse(value["signal_behavior_pass"])
        self.assertFalse(value["performance_gate_pass"])

    def test_catalog_samples_require_exact_fixture_evidence(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            self.fixture(
                directory,
                [200, 190, 180, 180, 180, 180, 180, 180],
                complete_gate_evidence=True,
                sample_id="S15-PERIODIC-ALL",
                fixture_type_count=118,
                fixture_created_type_count=118,
                fixture_visible_type_count=118,
            )
            value = analysis.analyze(directory)
            self.assertTrue(value["fixture_evidence_complete"])
            self.assertTrue(value["fixture_activity_pass"])
            self.assertTrue(value["performance_gate_pass"])
            result_path = directory / "result.json"
            result = json.loads(result_path.read_text(encoding="utf-8"))
            result["fixture_created_type_count"] = 117
            result_path.write_text(json.dumps(result), encoding="utf-8")
            value = analysis.analyze(directory)
        self.assertFalse(value["fixture_evidence_complete"])
        self.assertFalse(value["performance_gate_pass"])

    def test_catalog_samples_require_observed_activity(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            self.fixture(
                directory,
                [200, 190, 180, 180, 180, 180, 180, 180],
                complete_gate_evidence=True,
                sample_id="S19-ORGANICS-DENSE",
                fixture_type_count=33,
                fixture_created_type_count=33,
                fixture_visible_type_count=33,
            )
            result_path = directory / "result.json"
            result = json.loads(result_path.read_text(encoding="utf-8"))
            result["event_count_total"] = 0
            result["event_count_peak_per_frame"] = 0
            result_path.write_text(json.dumps(result), encoding="utf-8")
            value = analysis.analyze(directory)
        self.assertFalse(value["fixture_activity_pass"])
        self.assertFalse(value["performance_gate_pass"])

    def test_full_catalog_distinguishes_active_and_directly_selectable_types(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            self.fixture(
                directory,
                [500, 490, 480, 480, 480, 480, 480, 480],
                complete_gate_evidence=True,
                sample_id="S20-FULL-CATALOG",
                fixture_type_count=487,
                fixture_created_type_count=484,
                fixture_visible_type_count=466,
            )
            value = analysis.analyze(directory)
        self.assertTrue(value["fixture_evidence_complete"])
        self.assertTrue(value["performance_gate_pass"])

    def test_long_run_requires_all_same_process_cycles(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            self.fixture(
                directory,
                [500, 490, 480, 480, 480, 480, 480, 480],
                complete_gate_evidence=True,
                sample_id="S20-FULL-CATALOG",
                fixture_type_count=487,
                fixture_created_type_count=484,
                fixture_visible_type_count=466,
                long_run=True,
            )
            value = analysis.analyze(directory)
            self.assertTrue(value["long_run_evidence_complete"])
            self.assertTrue(value["long_run_duration_pass"])
            self.assertTrue(value["long_run_behavior_pass"])
            self.assertTrue(value["long_run_gate_pass"])
            self.assertTrue(value["performance_gate_pass"])

            result_path = directory / "result.json"
            result = json.loads(result_path.read_text(encoding="utf-8"))
            result["long_run_save_load_cycles"] = 9
            result_path.write_text(json.dumps(result), encoding="utf-8")
            value = analysis.analyze(directory)
        self.assertFalse(value["long_run_behavior_pass"])
        self.assertFalse(value["long_run_gate_pass"])
        self.assertFalse(value["performance_gate_pass"])

    def test_long_run_smoke_cannot_pass_duration_gate(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            self.fixture(
                directory,
                [500, 490, 480, 480, 480, 480, 480, 480],
                smoke=True,
                complete_gate_evidence=True,
                sample_id="S20-FULL-CATALOG",
                fixture_type_count=487,
                fixture_created_type_count=484,
                fixture_visible_type_count=466,
                long_run=True,
            )
            value = analysis.analyze(directory)
        self.assertFalse(value["long_run_duration_pass"])
        self.assertFalse(value["long_run_gate_pass"])
        self.assertFalse(value["performance_gate_pass"])


if __name__ == "__main__":
    unittest.main()
