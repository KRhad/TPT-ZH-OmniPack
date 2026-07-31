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
    ) -> None:
        ops1 = b"OPS1" + b"\0" * 8 + b"BZh" + b"input"
        ops2 = b"OPS1" + b"\0" * 8 + b"BZh" + b"output"
        (directory / "input-first.stm").write_bytes(ops1)
        (directory / "output-second.stm").write_bytes(ops2)
        result = {
            "schema_version": 1,
            "sample_id": "S01-METALLURGY-LARGE",
            "run_id": "fixture",
            "source_commit": "a" * 40,
            "public_zip_sha256": "B" * 64,
            "exe_sha256": "C" * 64,
            "input_ops_sha256": digest(ops1),
            "output_ops_second_sha256": digest(ops2),
            "warmup_seconds": 0.0 if smoke else 60.0,
            "sample_seconds": 2.0 if smoke else 600.0,
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
            "smoke_run": smoke,
        }
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

    def test_smoke_run_cannot_be_an_execution_pass(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            self.fixture(directory, [100, 100, 100], smoke=True)
            value = analysis.analyze(directory)
        self.assertFalse(value["duration_pass"])
        self.assertFalse(value["sample_execution_pass"])

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


if __name__ == "__main__":
    unittest.main()
