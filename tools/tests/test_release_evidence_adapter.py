from __future__ import annotations

import hashlib
import importlib.util
import json
from pathlib import Path
import sys
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[2]


def import_script(name: str, path: Path):
    spec = importlib.util.spec_from_file_location(name, path)
    assert spec and spec.loader
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


audit = import_script(
    "release_evidence_adapter_audit", ROOT / "tools" / "release_validation_audit.py"
)


class ReleaseEvidenceAdapterTests(unittest.TestCase):
    RUN_ID = "20260815T010203Z-1234abcd"
    COMMIT = "a" * 40

    def write_case(
        self,
        root: Path,
        *,
        producer_hash: str | None = None,
        binding: str | None = None,
        producer_overrides: dict[str, object] | None = None,
        adapter_overrides: dict[str, object] | None = None,
    ) -> dict[str, object]:
        producer = root / "gpu-producer.json"
        producer_value = {
            "schema": audit.EVIDENCE_SCHEMA,
            "schema_version": audit.EVIDENCE_SCHEMA_VERSION,
            "test": "gpu_validation",
            "status": "PASS",
            "passed": True,
            "supported": True,
            "fallback": False,
            "backend": "SDL_GPU Vulkan",
            "reason": "PASS",
            "max_abs_error": 0.0,
            "max_rel_error": 0.0,
            "first_mismatch_index": -1,
        }
        producer_value.update(producer_overrides or {})
        producer.write_text(json.dumps(producer_value) + "\n", encoding="utf-8")
        adapter = root / "gpu-bound.json"
        adapter_value = dict(producer_value)
        adapter_value.update(adapter_overrides or {})
        adapter_value.update({
            "gate_name": "GPUNumericalValidation",
            "run_id": self.RUN_ID,
            "commit": self.COMMIT,
            "candidate_sha256": None,
            "status": "PASS",
            "passed": True,
            "gate_started_at": "2000-01-01T00:00:00+00:00",
            "gate_finished_at": "2000-01-01T00:00:01+00:00",
            "identity_binding": binding or "trusted_release_driver_fresh_process_adapter",
            "producer_evidence": producer.name,
            "producer_evidence_sha256": producer_hash or hashlib.sha256(producer.read_bytes()).hexdigest().upper(),
        })
        adapter.write_text(json.dumps(adapter_value) + "\n", encoding="utf-8")
        envelope = root / "gate.json"
        envelope_value = {
            "schema": audit.EVIDENCE_SCHEMA,
            "schema_version": audit.EVIDENCE_SCHEMA_VERSION,
            "test": "gpu_validation",
            "gate_name": "GPUNumericalValidation",
            "run_id": self.RUN_ID,
            "commit": self.COMMIT,
            "status": "PASS",
            "passed": True,
            "exit_code": 0,
            "candidate_sha256": None,
            "gate_started_at": "2000-01-01T00:00:00+00:00",
            "gate_finished_at": "2000-01-01T00:00:01+00:00",
            "source_evidence": adapter.name,
            "source_evidence_sha256": hashlib.sha256(adapter.read_bytes()).hexdigest().upper(),
        }
        envelope.write_text(json.dumps(envelope_value) + "\n", encoding="utf-8")
        return {
            "run_id": self.RUN_ID,
            "commit": self.COMMIT,
            "source_snapshot_sha256": "B" * 64,
            "build_inputs_sha256": "C" * 64,
            "candidate_sha256": None,
            "symbols_sha256": None,
            "symbols_member_sha256": None,
            "gates": {
                "GPUNumericalValidation": {
                    "Status": "PASS",
                    "ExitCode": 0,
                    "Evidence": envelope.name,
                    "EvidenceSha256": hashlib.sha256(envelope.read_bytes()).hexdigest().upper(),
                }
            },
        }

    def test_fresh_hash_bound_adapter_is_accepted(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            validation = self.write_case(root)
            hash_ok, semantic_ok, _ = audit.audit_evidence(validation, root)
            self.assertTrue(hash_ok)
            self.assertTrue(semantic_ok)

    def test_adapter_with_wrong_producer_hash_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            validation = self.write_case(root, producer_hash="D" * 64)
            hash_ok, semantic_ok, rows = audit.audit_evidence(validation, root)
            self.assertTrue(hash_ok)
            self.assertFalse(semantic_ok)
            self.assertTrue(any(
                "producer evidence is absent or stale" in error
                for row in rows for error in row.get("errors", [])
            ))

    def test_unknown_adapter_declaration_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            validation = self.write_case(root, binding="untrusted_adapter")
            _, semantic_ok, rows = audit.audit_evidence(validation, root)
            self.assertFalse(semantic_ok)
            self.assertTrue(any(
                "unrecognized producer adapter" in error
                for row in rows for error in row.get("errors", [])
            ))

    def test_adapter_cannot_launder_failing_producer_semantics(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            validation = self.write_case(
                root,
                producer_overrides={
                    "supported": False,
                    "fallback": True,
                    "backend": "CPU",
                    "reason": "fallback_cpu",
                    "max_abs_error": 999.0,
                    "max_rel_error": 999.0,
                    "first_mismatch_index": 12,
                },
                adapter_overrides={
                    "supported": True,
                    "fallback": False,
                    "backend": "SDL_GPU Vulkan",
                    "reason": "PASS",
                    "max_abs_error": 0.0,
                    "max_rel_error": 0.0,
                    "first_mismatch_index": -1,
                },
            )
            hash_ok, semantic_ok, rows = audit.audit_evidence(validation, root)
            self.assertTrue(hash_ok)
            self.assertFalse(semantic_ok)
            self.assertTrue(any(
                "changed producer-owned field" in error
                or "producer semantic error" in error
                for row in rows for error in row.get("errors", [])
            ))


if __name__ == "__main__":
    unittest.main()
