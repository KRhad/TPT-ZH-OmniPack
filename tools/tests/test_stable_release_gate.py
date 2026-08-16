from __future__ import annotations

import json
import importlib.util
import io
from contextlib import redirect_stderr, redirect_stdout
from pathlib import Path
import hashlib
import os
import shutil
import struct
import subprocess
import sys
import tempfile
import unittest
from unittest import mock
import zipfile


ROOT = Path(__file__).resolve().parents[2]


def import_script(name: str, path: Path):
    spec = importlib.util.spec_from_file_location(name, path)
    assert spec and spec.loader
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


official_provenance = import_script(
    "official_tpt_provenance", ROOT / "tools" / "official_tpt_provenance.py"
)
release_validation_audit = import_script(
    "stable_release_validation_audit", ROOT / "tools" / "release_validation_audit.py"
)
release_finalizer = import_script(
    "stable_release_finalizer", ROOT / "tools" / "finalize_release_1_1_0.py"
)
release_negative_suite = import_script(
    "stable_release_negative_suite", ROOT / "tools" / "release_negative_gate_suite.py"
)
official_compatibility = import_script(
    "stable_official_save_compatibility", ROOT / "tools" / "official_save_compatibility.py"
)
source_snapshot = import_script(
    "stable_source_snapshot", ROOT / "tools" / "source_snapshot.py"
)


class FakeUpstream:
    def __init__(self, *, url: str = official_provenance.OFFICIAL_REPOSITORY,
                 exists: bool = True, reachable: bool = True,
                 blobs: dict[str, bytes] | None = None) -> None:
        self.url = url
        self.exists = exists
        self.reachable = reachable
        self.blobs = blobs or {}
        self.fetched = False

    def repository_url(self) -> str:
        return self.url

    def fetch(self) -> None:
        self.fetched = True

    def revision_exists(self, revision: str) -> bool:
        return self.exists

    def revision_reachable(self, revision: str) -> bool:
        return self.reachable

    def blob(self, revision: str, path: str) -> bytes:
        if path not in self.blobs:
            raise FileNotFoundError(path)
        return self.blobs[path]


class StableReleaseGateTests(unittest.TestCase):
    def test_official_save_gate_fails_closed_without_corpus(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            output = root / "official.json"
            completed = subprocess.run([
                sys.executable, str(ROOT / "tools/official_tpt_provenance.py"),
                "--corpus", str(root / "missing"),
                "--manifest", str(root / "missing" / "provenance.json"),
                "--repository", str(ROOT), "--git", "git", "--run-id", "20260814T041500Z-8f31c1c7",
                "--commit", "a" * 40,
                "--output", str(output),
            ], check=False)
            self.assertNotEqual(completed.returncode, 0)
            result = json.loads(output.read_text(encoding="utf-8"))
            self.assertFalse(result["passed"])
            self.assertEqual(result["status"], "NOT_TESTED")

    def make_provenance_case(self, root: Path) -> tuple[Path, dict[str, object], str, bytes]:
        corpus = root / "corpus"
        corpus.mkdir()
        path = "tests/saves/official.cps"
        fixture = corpus / path
        fixture.parent.mkdir(parents=True)
        upstream_bytes = b"official-save-object"
        fixture.write_bytes(upstream_bytes)
        revision = "a" * 40
        digest = hashlib.sha256(upstream_bytes).hexdigest()
        manifest = {
            "schema": "omnipack-official-tpt-save-corpus-v1",
            "corpus_id": "test-corpus",
            "source_repository": official_provenance.OFFICIAL_REPOSITORY,
            "source_revision": revision,
            "retrieved_at": "2026-08-14",
            "redistribution": {
                "status": "local_only_not_for_redistribution",
                "basis": "test-only local fixture",
            },
            "files": [{
                "path": path,
                "sha256": digest,
                "source_locator": f"{official_provenance.OFFICIAL_REPOSITORY}/blob/{revision}/{path}",
            }],
        }
        return corpus, manifest, path, upstream_bytes

    def test_official_provenance_accepts_three_way_git_object_match(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            corpus, manifest, path, upstream_bytes = self.make_provenance_case(root)
            result = official_provenance.validate_provenance(
                corpus, manifest, FakeUpstream(blobs={path: upstream_bytes}),
                run_id="20260814T041500Z-8f31c1c7", commit="b" * 40,
            )
            self.assertTrue(result["passed"])
            self.assertEqual(result["files_verified"], 1)

    def test_official_git_fetch_uses_isolated_remote_tag_namespace(self) -> None:
        calls = []
        upstream = official_provenance.GitUpstream(Path("repo"), "git", "official")
        upstream._run = lambda *args, **kwargs: calls.append(args)  # type: ignore[method-assign]
        upstream.fetch()
        self.assertEqual(len(calls), 1)
        self.assertIn("--no-tags", calls[0])
        self.assertNotIn("--tags", calls[0])
        self.assertIn(official_provenance.OFFICIAL_REPOSITORY + ".git", calls[0])
        self.assertNotIn("official", calls[0][:3])
        self.assertIn("+refs/heads/*:refs/remotes/official/*", calls[0])
        self.assertIn("+refs/tags/*:refs/remotes/official/tags/*", calls[0])

    def test_official_git_fetch_does_not_import_real_upstream_tags_locally(self) -> None:
        # Use native Git for Windows for this Windows file:// transport test.
        # MSYS Git rewrites native temporary paths and has produced intermittent
        # fetch failures when Meson launches this test alongside other jobs.
        git_candidates = (
            Path(r"E:\Git\cmd\git.exe"),
            Path(r"C:\Program Files\Git\cmd\git.exe"),
        )
        git_executable = next(
            (str(candidate) for candidate in git_candidates if candidate.is_file()),
            shutil.which("git.exe") or shutil.which("git"),
        )
        self.assertIsNotNone(git_executable)

        def git(repository: Path, *args: str) -> subprocess.CompletedProcess[str]:
            return subprocess.run(
                [str(git_executable), "-C", str(repository), *args],
                check=True, capture_output=True, text=True,
            )

        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            upstream_worktree = root / "official-source-worktree"
            upstream = root / "official-source.git"
            repository = root / "integration"
            upstream_worktree.mkdir()
            repository.mkdir()

            git(upstream_worktree, "init")
            git(upstream_worktree, "config", "user.name", "OmniPack Test")
            git(upstream_worktree, "config", "user.email", "omnipack-test@example.invalid")
            (upstream_worktree / "official.cps").write_bytes(b"official-save-object")
            git(upstream_worktree, "add", "official.cps")
            git(upstream_worktree, "commit", "-m", "official fixture")
            commit = git(upstream_worktree, "rev-parse", "HEAD").stdout.strip()
            git(upstream_worktree, "tag", "-a", "official-v1", "-m", "official-v1")
            subprocess.run(
                [str(git_executable), "clone", "--bare", str(upstream_worktree), str(upstream)],
                check=True, capture_output=True, text=True,
            )

            git(repository, "init")
            upstream_base = upstream.with_suffix("")
            with mock.patch.object(
                official_provenance, "OFFICIAL_REPOSITORY", upstream_base.as_uri()
            ):
                official_provenance.GitUpstream(
                    repository, str(git_executable), "official"
                ).fetch()

            local_tags = git(
                repository, "for-each-ref", "--format=%(refname)", "refs/tags"
            ).stdout.splitlines()
            self.assertEqual(local_tags, [])
            fetched_tag = git(
                repository, "rev-parse", "refs/remotes/official/tags/official-v1^{}"
            ).stdout.strip()
            self.assertEqual(fetched_tag, commit)

    def test_official_provenance_attack_matrix_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            corpus, base_manifest, path, upstream_bytes = self.make_provenance_case(root)
            attacks = []

            fake_revision = json.loads(json.dumps(base_manifest))
            fake_revision["source_revision"] = "0" * 40
            attacks.append(("fake revision", fake_revision, FakeUpstream(exists=False, blobs={path: upstream_bytes})))

            wrong_path = json.loads(json.dumps(base_manifest))
            wrong_path["files"][0]["path"] = "tests/saves/missing.cps"
            wrong_path["files"][0]["source_locator"] = f"{official_provenance.OFFICIAL_REPOSITORY}/blob/{'a' * 40}/tests/saves/missing.cps"
            attacks.append(("wrong upstream path", wrong_path, FakeUpstream(blobs={path: upstream_bytes})))

            modified_fixture = json.loads(json.dumps(base_manifest))
            attacks.append(("modified local fixture", modified_fixture, FakeUpstream(blobs={path: upstream_bytes})))

            wrong_hash = json.loads(json.dumps(base_manifest))
            wrong_hash["files"][0]["sha256"] = "0" * 64
            attacks.append(("manifest hash changed", wrong_hash, FakeUpstream(blobs={path: upstream_bytes})))

            wrong_locator = json.loads(json.dumps(base_manifest))
            wrong_locator["files"][0]["source_locator"] = f"{official_provenance.OFFICIAL_REPOSITORY}/blob/{'a' * 40}/different.cps"
            attacks.append(("locator path mismatch", wrong_locator, FakeUpstream(blobs={path: upstream_bytes})))

            wrong_repository = json.loads(json.dumps(base_manifest))
            wrong_repository["source_repository"] = "https://example.invalid/fake"
            attacks.append(("wrong repository", wrong_repository, FakeUpstream(blobs={path: upstream_bytes})))

            for label, manifest, upstream in attacks:
                with self.subTest(label=label):
                    fixture = corpus / path
                    fixture.write_bytes(b"modified" if label == "modified local fixture" else upstream_bytes)
                    result = official_provenance.validate_provenance(
                        corpus, manifest, upstream,
                        run_id="20260814T041500Z-8f31c1c7", commit="b" * 40,
                    )
                    self.assertFalse(result["passed"])
                    self.assertEqual(result["status"], "FAIL")

    def make_v2_provenance_case(
        self, root: Path
    ) -> tuple[Path, dict[str, object], str, bytes, str, bytes, dict[str, object]]:
        corpus = root / "corpus"
        git_path = "git/dust.cps"
        web_path = "web/maintainer.cps"
        git_bytes = b"official-benchmark-git-object"
        web_bytes = b"official-hosted-maintainer-save"
        (corpus / "git").mkdir(parents=True)
        (corpus / "web").mkdir(parents=True)
        (corpus / git_path).write_bytes(git_bytes)
        (corpus / web_path).write_bytes(web_bytes)
        revision = "a" * 40
        save_id = 1249335
        metadata = {
            "id": save_id,
            "username": "jacob1",
            "elevation": "Mod",
            "published": True,
            "date": 1738891791,
            "date_created": 1372986719,
            "is_banned": False,
        }
        manifest = {
            "schema": official_provenance.MANIFEST_SCHEMA_V2,
            "corpus_id": "v2-test-corpus",
            "retrieved_at": "2026-08-16",
            "redistribution": {
                "status": "local_only_not_for_redistribution",
                "basis": "official sources with no broad redistribution licence",
            },
            "files": [
                {
                    "path": git_path,
                    "source_kind": "github_git",
                    "source_repository": official_provenance.OFFICIAL_TPT_BENCH_REPOSITORY,
                    "source_revision": revision,
                    "source_path": "suites/screenfuls/dust.cps",
                    "source_locator": (
                        "https://raw.githubusercontent.com/The-Powder-Toy/tpt-bench/"
                        f"{revision}/suites/screenfuls/dust.cps"
                    ),
                    "sha256": hashlib.sha256(git_bytes).hexdigest(),
                },
                {
                    "path": web_path,
                    "source_kind": "official_web_save",
                    "source_repository": official_provenance.OFFICIAL_WEB_API_ORIGIN,
                    "source_date": metadata["date"],
                    "save_id": save_id,
                    "source_locator": f"{official_provenance.OFFICIAL_WEB_API_ORIGIN}/Browse/View.json?ID={save_id}",
                    "content_locator": f"{official_provenance.OFFICIAL_WEB_STATIC_ORIGIN}/{save_id}.cps",
                    "metadata": metadata,
                    "sha256": hashlib.sha256(web_bytes).hexdigest(),
                },
            ],
        }
        return corpus, manifest, git_path, git_bytes, web_path, web_bytes, metadata

    def test_official_v2_provenance_separates_git_and_maintainer_web_sources(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            corpus, manifest, _, git_bytes, _, web_bytes, metadata = self.make_v2_provenance_case(root)
            git_upstream = FakeUpstream(blobs={"suites/screenfuls/dust.cps": git_bytes})

            def fetch(url: str, *, accept: str) -> bytes:
                del accept
                if "View.json" in url:
                    server = {
                        "ID": metadata["id"], "Username": metadata["username"],
                        "Elevation": metadata["elevation"], "Published": metadata["published"],
                        "Date": metadata["date"], "DateCreated": metadata["date_created"],
                        "IsBanned": metadata["is_banned"],
                    }
                    return json.dumps(server).encode("utf-8")
                return web_bytes

            with mock.patch.object(official_provenance, "GitUpstream", return_value=git_upstream), \
                 mock.patch.object(official_provenance, "fetch_exact_https", side_effect=fetch):
                result = official_provenance.validate_provenance_v2(
                    corpus, manifest, root, "git", "official",
                    run_id="20260816T140000Z-a1b2c3d4", commit="b" * 40,
                )
            self.assertTrue(result["passed"])
            self.assertEqual(result["files_verified"], 2)
            self.assertEqual(
                {row["source_kind"] for row in result["files"]},
                {"github_git", "official_web_save"},
            )
            self.assertEqual(
                release_validation_audit.validate_raw_semantics(
                    "OfficialTPTCorpusProvenance", result, root=root,
                    candidate_sha256=None,
                ),
                [],
            )

    def test_official_v2_provenance_attack_matrix_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            corpus, base, _, git_bytes, _, web_bytes, metadata = self.make_v2_provenance_case(root)

            def fetch(url: str, *, accept: str) -> bytes:
                del accept
                if "View.json" in url:
                    return json.dumps({
                        "ID": metadata["id"], "Username": metadata["username"],
                        "Elevation": metadata["elevation"], "Published": metadata["published"],
                        "Date": metadata["date"], "DateCreated": metadata["date_created"],
                        "IsBanned": metadata["is_banned"],
                    }).encode("utf-8")
                return web_bytes

            attacks: list[tuple[str, dict[str, object], FakeUpstream, object]] = []
            fake_revision = json.loads(json.dumps(base))
            fake_revision["files"][0]["source_revision"] = "0" * 40
            fake_revision["files"][0]["source_locator"] = (
                "https://raw.githubusercontent.com/The-Powder-Toy/tpt-bench/"
                + "0" * 40 + "/suites/screenfuls/dust.cps"
            )
            attacks.append(("fake Git revision", fake_revision, FakeUpstream(exists=False), fetch))

            wrong_repository = json.loads(json.dumps(base))
            wrong_repository["files"][0]["source_repository"] = "https://example.invalid/fake"
            attacks.append(("wrong Git repository", wrong_repository, FakeUpstream(), fetch))

            wrong_web_locator = json.loads(json.dumps(base))
            wrong_web_locator["files"][1]["content_locator"] = "https://example.invalid/save.cps"
            attacks.append(("wrong web locator", wrong_web_locator, FakeUpstream(blobs={"suites/screenfuls/dust.cps": git_bytes}), fetch))

            wrong_web_author = json.loads(json.dumps(base))
            wrong_web_author["files"][1]["metadata"]["username"] = "random-user"
            attacks.append(("non-maintainer author", wrong_web_author, FakeUpstream(blobs={"suites/screenfuls/dust.cps": git_bytes}), fetch))

            def modified_content(url: str, *, accept: str) -> bytes:
                if "View.json" in url:
                    return fetch(url, accept=accept)
                return b"modified-web-save"

            attacks.append(("modified web bytes", json.loads(json.dumps(base)), FakeUpstream(blobs={"suites/screenfuls/dust.cps": git_bytes}), modified_content))

            website_only = json.loads(json.dumps(base))
            website_only["files"] = website_only["files"][1:]
            (corpus / "git" / "dust.cps").unlink()
            attacks.append(("website only", website_only, FakeUpstream(), fetch))

            for label, manifest, upstream, web_fetch in attacks:
                with self.subTest(label=label):
                    if label != "website only" and not (corpus / "git" / "dust.cps").exists():
                        (corpus / "git" / "dust.cps").write_bytes(git_bytes)
                    if label == "website only" and (corpus / "git" / "dust.cps").exists():
                        (corpus / "git" / "dust.cps").unlink()
                    with mock.patch.object(official_provenance, "GitUpstream", return_value=upstream), \
                         mock.patch.object(official_provenance, "fetch_exact_https", side_effect=web_fetch):
                        result = official_provenance.validate_provenance_v2(
                            corpus, manifest, root, "git", "official",
                            run_id="20260816T140000Z-a1b2c3d4", commit="b" * 40,
                        )
                    self.assertFalse(result["passed"], label)
                    self.assertEqual(result["status"], "FAIL", label)

    def test_official_v2_semantic_audit_rejects_forged_web_metadata(self) -> None:
        raw = {
            "schema": "omnipack-release-evidence",
            "schema_version": 1,
            "test": "official_tpt_provenance",
            "status": "PASS",
            "passed": True,
            "provenance_schema": official_provenance.MANIFEST_SCHEMA_V2,
            "source_repositories": [
                official_provenance.OFFICIAL_TPT_BENCH_REPOSITORY,
                official_provenance.OFFICIAL_WEB_API_ORIGIN,
            ],
            "repositories": [{
                "source_kind": "github_git",
                "repository": official_provenance.OFFICIAL_TPT_BENCH_REPOSITORY,
                "revision": "a" * 40,
                "revision_exists": True,
                "revision_reachable_from_official_remote": True,
            }, {
                "source_kind": "official_web_save",
                "repository": official_provenance.OFFICIAL_WEB_API_ORIGIN,
                "save_id": 1249335,
                "source_date": 1738891791,
                "metadata_verified": True,
                "content_hash_verified": True,
            }],
            "revision_exists": True,
            "revision_reachable_from_official_remote": True,
            "files_total": 2,
            "files_verified": 2,
            "files_failed": 0,
            "files": [{
                "path": "git/dust.cps", "source_kind": "github_git",
                "source_repository": official_provenance.OFFICIAL_TPT_BENCH_REPOSITORY,
                "source_revision": "a" * 40,
                "source_path": "suites/screenfuls/dust.cps",
                "source_locator": "https://raw.githubusercontent.com/The-Powder-Toy/tpt-bench/" + "a" * 40 + "/suites/screenfuls/dust.cps",
                "match": True, "upstream_sha256": "A" * 64,
                "manifest_sha256": "A" * 64, "fixture_sha256": "A" * 64,
            }, {
                "path": "web/maintainer.cps", "source_kind": "official_web_save",
                "source_repository": official_provenance.OFFICIAL_WEB_API_ORIGIN,
                "source_date": 1738891791, "save_id": 1249335,
                "source_locator": "https://powdertoy.co.uk/Browse/View.json?ID=1249335",
                "content_locator": "https://static.powdertoy.co.uk/1249335.cps",
                "metadata": {
                    "id": 1249335, "username": "not-a-maintainer", "elevation": "Mod",
                    "published": True, "date": 1738891791,
                    "date_created": 1372986719, "is_banned": False,
                },
                "match": True, "upstream_sha256": "B" * 64,
                "manifest_sha256": "B" * 64, "fixture_sha256": "B" * 64,
            }],
        }
        errors = release_validation_audit.validate_raw_semantics(
            "OfficialTPTCorpusProvenance", raw, root=Path.cwd(), candidate_sha256=None,
        )
        self.assertTrue(any("maintainer" in error for error in errors))

    def test_release_script_is_fail_closed_and_evidence_bound(self) -> None:
        script = (ROOT / "tools/release_1_1_0.ps1").read_text(encoding="utf-8")
        self.assertIn("Start-Process", script)
        self.assertIn("ConvertFrom-Json", script)
        self.assertIn('Status -ne "PASS"', script)
        self.assertIn('"OfficialTPTSaveCompatibility"', script)
        self.assertIn('"WindowsCleanMachine"', script)
        self.assertIn('"Soak2Hours"', script)
        self.assertNotIn("continue-on-error", script)
        self.assertNotIn("--force-pass", script)
        self.assertIn("[string] $OfficialSaveCorpus", script)
        self.assertIn("official_tpt_provenance.py", script)
        self.assertIn('"--provenance-evidence"', script)
        self.assertIn('"OfficialTPTCorpusProvenance"', script)
        self.assertIn('"Channel: $Channel"', script)
        self.assertIn('Invoke-GateProcess "NegativeGateSuite"', script)
        self.assertIn('"DocumentationConsistency","NegativeGateSuite"', script)
        self.assertIn('gate_name="CandidateSHA256"', script)
        self.assertIn('gate_name="ArtifactImmutability"', script)
        self.assertIn('gate_name="WindowsCleanMachine"', script)
        self.assertIn('symbols_member_sha256=$currentSymbolsMemberSha256', script)

        negative = (ROOT / "tools/release_negative_gate_suite.py").read_text(
            encoding="utf-8"
        )
        self.assertIn('expected_candidate_name = f"{args.artifact_stem}.zip"', negative)
        self.assertIn('expected_symbols_name = f"{args.symbol_artifact_stem}.zip"', negative)
        self.assertIn('"--package-version",$version,"--package-kind",$kind', script)
        negative_main = negative.split("def main() -> int:", 1)[1]
        self.assertNotIn(
            'TPT-ZH-OmniPack-1.1.0-staging-{RUN_ID}-Windows-x64-SDL3.zip',
            negative_main,
        )

    def test_gpu_validation_unsupported_cannot_exit_zero(self) -> None:
        source = (ROOT / "src/common/platform/SDLGPU.cpp").read_text(encoding="utf-8")
        self.assertIn("return kShaderUnavailable", source)
        self.assertIn("return kGPUDeviceCreationFailure", source)
        self.assertIn("WriteValidationJson", source)
        self.assertIn("gpu_validation_passed", source)

    def test_cpu_fallback_gate_executes_and_validates_production_path(self) -> None:
        source = (ROOT / "src/common/platform/SDLGPU.cpp").read_text(encoding="utf-8")
        self.assertIn("InjectedFailingThermalExecutor", source)
        self.assertIn("auto runtime = MakeFallbackAtmosphere();", source)
        self.assertIn("runtime.Step();", source)
        self.assertIn("SameFallbackAtmosphere(controlSnapshot, runtimeSnapshot)", source)
        self.assertNotIn("GPU_init_failed_CPU_reference_continues", source)

        script = (ROOT / "tools/release_1_1_0.ps1").read_text(encoding="utf-8")
        for marker in (
            "runtime_executor_invocations -eq 1",
            "initialization_backend_prearmed -eq $true",
            "initialization_backend_reset_to_cpu -eq $true",
            "same_step_cpu_fallback -eq $true",
            "backend_reset_to_cpu -eq $true",
            "cpu_control_state_match -eq $true",
            "runtime_nonfinite_cells -eq 0",
        ):
            self.assertIn(marker, script)

        baseline = release_negative_suite.cpu_fallback_payload()
        baseline.update({
            "schema": release_validation_audit.EVIDENCE_SCHEMA,
            "schema_version": release_validation_audit.EVIDENCE_SCHEMA_VERSION,
            "test": "cpu_fallback",
        })
        self.assertEqual(
            release_validation_audit.validate_raw_semantics(
                "CPUFallbackValidation", baseline, root=Path.cwd(),
                candidate_sha256=None,
            ),
            [],
        )
        for field, value in (
            ("initialization_backend_prearmed", False),
            ("runtime_executor_invocations", 0),
            ("same_step_cpu_fallback", False),
            ("backend_reset_to_cpu", False),
            ("cpu_control_state_match", False),
            ("runtime_nonfinite_cells", 1),
        ):
            with self.subTest(field=field):
                attacked = dict(baseline)
                attacked[field] = value
                self.assertTrue(release_validation_audit.validate_raw_semantics(
                    "CPUFallbackValidation", attacked, root=Path.cwd(),
                    candidate_sha256=None,
                ))

    def test_sdl3_runtime_gate_requires_real_vulkan_compute_fields(self) -> None:
        script = (ROOT / "tools/release_1_1_0.ps1").read_text(encoding="utf-8")
        self.assertIn('Read-KeyValueEvidence "$probe.stdout.txt"', script)
        for marker in (
            'production_thermal_diffusion_built -eq "true"',
            'gpu_supported -eq "true"', 'spirv_supported -eq "true"',
            'compute_poc_executed -eq "true"', 'deterministic_compare -eq "true"',
            'fallback_cpu -eq "false"',
            'cuda_backend_status -eq "not_implemented_optional_future_backend"',
        ):
            self.assertIn(marker, script)

    def test_portable_runtime_initializes_preferences_before_simulation_data(self) -> None:
        source = (ROOT / "src/PowderToy.cpp").read_text(encoding="utf-8")
        body = source.split("int RunPortableRuntimeValidation", 1)[1].split(
            "void PrintStartupDiagnostics", 1
        )[0]
        self.assertIn("GlobalPrefs globalPrefs;", body)
        self.assertLess(
            body.index("GlobalPrefs globalPrefs;"),
            body.index("SimulationData simulationData;"),
        )

    def test_release_gate_records_source_and_evidence_integrity(self) -> None:
        script = (ROOT / "tools/release_1_1_0.ps1").read_text(encoding="utf-8")
        self.assertIn('"SourceTreeClean"', script)
        self.assertIn("EvidenceSha256", script)
        self.assertIn("EvidenceSemanticIntegrity", script)
        self.assertIn("EvidenceHashIntegrity", script)
        self.assertIn("DocumentationConsistency", script)
        self.assertIn("WindowsPortableExtraction", script)
        clean = (ROOT / "tools/test_clean_release.ps1").read_text(encoding="utf-8")
        workflow = (ROOT / ".github/workflows/release-validation-windows.yml").read_text(encoding="utf-8")
        self.assertNotIn("OMNI_CLEAN_MACHINE", clean)
        self.assertNotIn("OMNI_CLEAN_MACHINE", script)
        self.assertNotIn("OMNI_CLEAN_MACHINE", workflow)
        self.assertIn("source_checkout_used", clean)
        self.assertIn("clean-runtime-validation", workflow)
        self.assertIn(
            "path: ${{ runner.temp }}/omnipack-official-corpus-input",
            workflow,
        )
        self.assertIn(
            '$corpusRoot = Join-Path $env:RUNNER_TEMP "omnipack-official-corpus-input"',
            workflow,
        )
        self.assertNotIn("path: official-corpus-input", workflow)
        self.assertIn("Prepare a current-run runtime-only transfer set", workflow)
        self.assertIn(
            "path: ${{ runner.temp }}/omnipack-candidate-transfer/",
            workflow,
        )
        self.assertNotIn("path: |\n            dist/1.1.0/", workflow)
        runtime_job = workflow.split("clean-runtime-validation:", 1)[1].split("finalize-release:", 1)[0]
        self.assertNotIn("actions/checkout", runtime_job)
        self.assertNotIn("setup-msys2", runtime_job)
        self.assertNotIn("Get-FileHash", runtime_job)
        self.assertNotIn("Select-Object -First 1", runtime_job)
        self.assertIn('$validations.Count -ne 1', runtime_job)
        self.assertIn('$candidates.Count -ne 1', runtime_job)
        self.assertIn('$runners.Count -ne 1', runtime_job)
        self.assertIn("runtime-validator-binding.json", runtime_job)
        self.assertIn("-ExpectedValidatorSha256 $validatorSha", runtime_job)
        self.assertIn("finalize-release", workflow)
        self.assertNotIn("  push:", workflow)
        self.assertNotIn("tags: [\"omnipack-v1.1.0\"]", workflow)
        self.assertIn("actions: read", workflow)
        self.assertIn("CORPUS_RUN_ID_INPUT: ${{ inputs.official_corpus_run_id }}", workflow)
        self.assertNotIn('$runId = "${{ inputs.official_corpus_run_id }}"', workflow)
        self.assertIn("OMNIPACK_OFFICIAL_CORPUS_WORKFLOW_ID", workflow)

    def test_release_workflow_finalizer_requires_exact_validator_and_binding_artifacts(self) -> None:
        workflow = (ROOT / ".github/workflows/release-validation-windows.yml").read_text(encoding="utf-8")
        finalizer_job = workflow.split("finalize-release:", 1)[1]
        self.assertIn('$binding = Get-ChildItem $cleanInput -Filter runtime-validator-binding.json -Recurse -File', finalizer_job)
        self.assertIn('$validator = Get-ChildItem $cleanInput -Filter test_clean_release.ps1 -Recurse -File', finalizer_job)
        self.assertIn('@($binding).Count -ne 1', finalizer_job)
        self.assertIn('@($validator).Count -ne 1', finalizer_job)
        self.assertIn("--runtime-validator-binding $binding.FullName", finalizer_job)
        self.assertIn("--runtime-validator $validator.FullName", finalizer_job)

    def test_release_workflow_cannot_create_a_stable_tag_before_final_gate(self) -> None:
        workflow = (ROOT / ".github/workflows/release-validation-windows.yml").read_text(encoding="utf-8")
        trigger = workflow.split("permissions:", 1)[0]
        self.assertIn("workflow_dispatch:", trigger)
        self.assertNotIn("push:", trigger)
        self.assertNotIn("tags:", trigger)

    def test_release_driver_uses_distinct_raw_evidence_for_each_meta_gate(self) -> None:
        script = (ROOT / "tools/release_1_1_0.ps1").read_text(encoding="utf-8")
        self.assertIn('File="evidence-hash-integrity.json"', script)
        self.assertIn('File="evidence-semantic-integrity.json"', script)
        self.assertIn('File="documentation-consistency.json"', script)
        self.assertEqual(script.count('$auditEvidence $'), 0)

    def test_meta_gate_evidence_records_are_not_skipped(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            validation = {
                "run_id": "20260814T041500Z-8f31c1c7",
                "commit": "a" * 40,
                "source_snapshot_sha256": "A" * 64,
                "build_inputs_sha256": "D" * 64,
                "gates": {
                    name: {
                        "Status": "PASS", "ExitCode": 0,
                        "Evidence": f"missing-{name}.json", "EvidenceSha256": "B" * 64,
                    }
                    for name in release_validation_audit.META_GATES
                },
            }
            hash_ok, semantic_ok, rows = release_validation_audit.audit_evidence(
                validation, Path(temporary)
            )
        self.assertFalse(hash_ok)
        self.assertFalse(semantic_ok)
        self.assertEqual({row["gate"] for row in rows}, release_validation_audit.META_GATES)

    def test_clean_machine_branch_is_marker_gated_and_executes_runner(self) -> None:
        script = (ROOT / "tools/release_1_1_0.ps1").read_text(encoding="utf-8")
        self.assertNotIn('if ($env:OMNI_CLEAN_MACHINE -eq "true")', script)
        self.assertNotIn('Invoke-GateProcess "WindowsCleanMachine" powershell.exe', script)
        self.assertIn("independent runtime-only Windows job evidence", script)
        self.assertIn('"-ExpectedArtifactSha256",$candidateSha256', script)
        self.assertIn('"ArtifactImmutability"', script)
        self.assertIn("Post-package gates must only read", script)
        self.assertIn("soak-candidate-extracted", script)
        self.assertIn('-NotePropertyName executable_source -NotePropertyValue "candidate_zip"', script)
        self.assertIn('foreach ($name in @("SourceSnapshotImmutability","SDL3GUI","Soak2Hours"', script)
        self.assertNotIn('Invoke-GateProcess "FinalPackage"', script)
        self.assertNotIn('Invoke-GateProcess "FinalPackageVerification"', script)

    def test_stable_package_is_staging_named_until_finalizer(self) -> None:
        script = (ROOT / "tools/release_1_1_0.ps1").read_text(encoding="utf-8")
        self.assertIn("staging-$runId-Windows-x64-SDL3", script)
        self.assertIn("[switch] $CandidateOnly", script)
        self.assertIn("STAGING_CANDIDATE_READY", script)
        self.assertNotIn('release-1.1.0: PASS', script)
        self.assertNotIn('Invoke-GateProcess "SDL3GUI" $exe', script)
        self.assertIn('"-GateName","SDL3GUI"', script)
        finalizer = (ROOT / "tools/finalize_release_1_1_0.py").read_text(encoding="utf-8")
        self.assertIn("transaction.rename(output)", finalizer)
        self.assertIn("promotion transaction changed artifact bytes", finalizer)
        self.assertNotIn("candidate.replace(stable)", finalizer)
        self.assertLess(finalizer.rindex("run_audit(validation", 0, finalizer.index("transaction.rename(output)")), finalizer.index("transaction.rename(output)"))
        self.assertLess(finalizer.index('"PROMOTION-PREPARED.json"'), finalizer.index("transaction.rename(output)"))
        self.assertLess(finalizer.index("transaction.rename(output)"), finalizer.index('"PROMOTION-COMPLETE.json"'))
        self.assertLess(finalizer.index("transaction.rename(output)"), finalizer.index("published = True"))
        self.assertIn("validate_published_document_set", finalizer)
        self.assertIn("stable_names_absent_before_final_audit", finalizer)
        self.assertIn("refresh_artifact_immutability", finalizer)
        self.assertIn("exclusive-lock-atomic-directory-publish", finalizer)

    def test_release_script_missing_raw_evidence_cannot_create_pass(self) -> None:
        script = (ROOT / "tools/release_1_1_0.ps1").read_text(encoding="utf-8")
        self.assertIn("Missing evidence can never be upgraded to PASS", script)
        self.assertIn("requested PASS was rejected", script)
        self.assertIn("raw evidence identity or result fields are invalid", script)

    def test_source_snapshot_hashes_real_worktree_bytes(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            subprocess.run(["git", "init", "-q"], cwd=root, check=True)
            subprocess.run(["git", "config", "user.email", "snapshot@example.invalid"], cwd=root, check=True)
            subprocess.run(["git", "config", "user.name", "Snapshot Test"], cwd=root, check=True)
            tracked = root / "tracked.txt"
            tracked.write_bytes(b"one\n")
            subprocess.run(["git", "add", "tracked.txt"], cwd=root, check=True)
            subprocess.run(["git", "commit", "-q", "-m", "snapshot"], cwd=root, check=True)
            before = source_snapshot.snapshot(root)
            tracked.write_bytes(b"two\n")
            after = source_snapshot.snapshot(root)
            self.assertNotEqual(before["source_worktree_sha256"], after["source_worktree_sha256"])
            self.assertEqual(before["tracked_files"], after["tracked_files"])

    def test_generic_command_pass_requires_zero_exit_and_hashed_streams(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            errors = release_validation_audit.validate_raw_semantics(
                "Build", {"exit_code": 1}, root=root, candidate_sha256=None,
            )
            self.assertTrue(any("exit_code=0" in error for error in errors))
            self.assertTrue(any("stdout" in error for error in errors))
            self.assertTrue(any("stderr" in error for error in errors))

            stdout = root / "stdout.txt"
            stderr = root / "stderr.txt"
            stdout.write_text("build output\n", encoding="utf-8")
            stderr.write_text("", encoding="utf-8")
            raw = {
                "exit_code": 0,
                "stdout": stdout.name,
                "stdout_sha256": hashlib.sha256(stdout.read_bytes()).hexdigest().upper(),
                "stderr": stderr.name,
                "stderr_sha256": hashlib.sha256(stderr.read_bytes()).hexdigest().upper(),
            }
            self.assertEqual(
                release_validation_audit.validate_raw_semantics(
                    "Build", raw, root=root, candidate_sha256=None,
                ),
                [],
            )

    def test_portable_extraction_pass_requires_real_runtime_results(self) -> None:
        errors = release_validation_audit.validate_raw_semantics(
            "WindowsPortableExtraction", {}, root=Path.cwd(), candidate_sha256="A" * 64,
        )
        self.assertTrue(any("runtime result" in error for error in errors))
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            run_id = "20260814T041500Z-8f31c1c7"
            candidate_sha = "A" * 64
            stdout = root / "portable.stdout.txt"
            stderr = root / "portable.stderr.txt"
            inner_path = root / "portable.inner.json"
            stdout.write_text("portable PASS\n", encoding="utf-8")
            stderr.write_text("", encoding="utf-8")
            inner = {
                "schema": "omnipack-release-evidence", "schema_version": 1,
                "payload_schema_version": 2, "test": "portable_runtime",
                "run_id": run_id, "candidate_sha256": candidate_sha,
                "status": "PASS", "passed": True,
                "initial_particle_count": 2, "post_step_particle_count": 2,
                "final_particle_count": 2, "atmosphere_non_finite_cells": 0,
            }
            inner.update({field: True for field in release_validation_audit.PORTABLE_RUNTIME_TRUE_FIELDS})
            inner_path.write_text(json.dumps(inner), encoding="utf-8")
            valid = {
                "run_id": run_id, "candidate_filename": "candidate.zip",
                "candidate_extracted_to_fresh_directory": True,
                "candidate_source_markers_checked": True,
                "candidate_source_tree_indicators": [],
                "runtime_target_executable_count": 1, "sanitized_path_used": True,
                "runtime_payload_schema_version": 2, "clean_shutdown": True,
                "project_build_tool_dependency_used": True,
                "initial_particle_count": 2, "post_step_particle_count": 2,
                "final_particle_count": 2, "atmosphere_non_finite_cells": 0,
                "runtime_stdout": stdout.name,
                "runtime_stdout_sha256": hashlib.sha256(stdout.read_bytes()).hexdigest().upper(),
                "runtime_stderr": stderr.name,
                "runtime_stderr_sha256": hashlib.sha256(stderr.read_bytes()).hexdigest().upper(),
                "runtime_inner_evidence": inner_path.name,
                "runtime_inner_evidence_sha256": hashlib.sha256(inner_path.read_bytes()).hexdigest().upper(),
            }
            valid.update({field: True for field in release_validation_audit.PORTABLE_RUNTIME_TRUE_FIELDS})
            self.assertEqual(
                release_validation_audit.validate_raw_semantics(
                    "WindowsPortableExtraction", valid, root=root,
                    candidate_sha256=candidate_sha,
                ),
                [],
            )
            inner["post_step_finite_passed"] = False
            inner_path.write_text(json.dumps(inner), encoding="utf-8")
            valid["runtime_inner_evidence_sha256"] = hashlib.sha256(
                inner_path.read_bytes()
            ).hexdigest().upper()
            mismatch_errors = release_validation_audit.validate_raw_semantics(
                "WindowsPortableExtraction", valid, root=root,
                candidate_sha256=candidate_sha,
            )
            self.assertIn(
                "portable runtime inner evidence has invalid post_step_finite_passed",
                mismatch_errors,
            )
            self.assertIn(
                "portable runtime outer/inner post_step_finite_passed mismatch",
                mismatch_errors,
            )

    def test_finalizer_rejects_replaced_symbols_before_stable_name_exists(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            run_id = "20260814T041500Z-8f31c1c7"
            candidate = root / f"TPT-ZH-OmniPack-1.1.0-staging-{run_id}-Windows-x64-SDL3.zip"
            symbols = root / f"TPT-ZH-OmniPack-1.1.0-staging-{run_id}-Windows-x64-Symbols.zip"
            candidate.write_bytes(b"candidate")
            symbols.write_bytes(b"replacement-symbols")
            validation = root / "RELEASE-VALIDATION.json"
            validation.write_text(json.dumps({
                "schema": "omnipack-release-validation", "schema_version": 1,
                "version": "1.1.0", "channel": "stable", "run_id": run_id,
                "commit": "a" * 40,
                "source_snapshot_sha256": "F" * 64,
                "build_inputs_sha256": "E" * 64,
                "candidate_sha256": hashlib.sha256(candidate.read_bytes()).hexdigest().upper(),
                "symbols_sha256": "0" * 64,
                "symbols_member_sha256": "1" * 64,
                "gates": {},
            }), encoding="utf-8")
            output = root / "out"
            stderr = io.StringIO()
            argv = [
                "finalize_release_1_1_0.py", "--validation-json", str(validation),
                "--candidate", str(candidate), "--symbols", str(symbols),
                "--clean-machine-evidence", str(root / "missing-clean.json"),
                "--runtime-validator-binding", str(root / "missing-binding.json"),
                "--runtime-validator", str(root / "missing-validator.ps1"),
                "--output-directory", str(output),
            ]
            with mock.patch.object(sys, "argv", argv), redirect_stderr(stderr):
                self.assertEqual(release_finalizer.main(), 1)
            self.assertIn("symbols bytes do not match aggregate", stderr.getvalue())
            self.assertFalse((output / "TPT-ZH-OmniPack-1.1.0-Windows-x64-Symbols.zip").exists())

    def test_finalizer_rejects_rc_without_rewriting_foreign_evidence(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            validation = root / "RELEASE-VALIDATION.json"
            validation.write_text(json.dumps({
                "schema": "omnipack-release-validation", "schema_version": 1,
                "version": "1.1.0-rc1", "channel": "rc",
                "run_id": "20260814T041500Z-8f31c1c7", "commit": "a" * 40,
                "status": "RC VALIDATION COMPLETE - STABLE BLOCKED",
                "final_status": "RC VALIDATION COMPLETE - STABLE BLOCKED",
                "blocking_items": ["Soak2Hours"], "gates": {},
            }, sort_keys=True), encoding="utf-8")
            original = validation.read_bytes()
            output = root / "stable-output"
            argv = [
                "finalize_release_1_1_0.py", "--validation-json", str(validation),
                "--candidate", str(root / "rc.zip"),
                "--symbols", str(root / "rc-symbols.zip"),
                "--clean-machine-evidence", str(root / "clean.json"),
                "--runtime-validator-binding", str(root / "binding.json"),
                "--runtime-validator", str(root / "validator.ps1"),
                "--output-directory", str(output),
            ]
            stderr = io.StringIO()
            with mock.patch.object(sys, "argv", argv), redirect_stderr(stderr):
                self.assertEqual(release_finalizer.main(), 1)
            self.assertIn("accepts only a 1.1.0 stable staging run", stderr.getvalue())
            self.assertEqual(validation.read_bytes(), original)
            self.assertFalse(output.exists())

    def test_finalizer_rejects_development_checkout_clean_claim(self) -> None:
        evidence = {
            "schema": "omnipack-release-evidence", "schema_version": 1,
            "test": "windows_clean_machine", "run_id": "20260814T041500Z-8f31c1c7",
            "status": "PASS", "passed": True, "candidate_sha256": "A" * 64,
            "source_checkout_used": True, "project_build_tool_dependency_used": False,
            "candidate_extracted_to_fresh_directory": True, "sanitized_path_used": True,
            "launch_passed": True, "save_reload_passed": True,
            "state_validate_passed": True, "clean_shutdown": True,
            "gate_started_at": "2026-08-14T00:00:00Z", "gate_finished_at": "2026-08-14T00:01:00Z",
        }
        with self.assertRaises(ValueError):
            release_finalizer.validate_clean_evidence(
                evidence, root=Path.cwd(), run_id="20260814T041500Z-8f31c1c7",
                candidate_sha256="A" * 64, runtime_validator_sha256="B" * 64,
            )

    def test_finalizer_rejects_runtime_validator_hash_or_identity_mismatch(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            artifact_root = root / "runtime-artifact"
            validator_root = artifact_root / "runtime-validator"
            validator_root.mkdir(parents=True)
            trusted_root = root / "trusted-checkout" / "tools"
            trusted_root.mkdir(parents=True)
            trusted = trusted_root / "test_clean_release.ps1"
            downloaded = validator_root / "test_clean_release.ps1"
            trusted.write_bytes(b"trusted validator\n")
            downloaded.write_bytes(trusted.read_bytes())
            trusted_sha = release_finalizer.sha256(trusted)
            run_id = "20260814T041500Z-8f31c1c7"
            commit = "a" * 40
            candidate = f"TPT-ZH-OmniPack-1.1.0-staging-{run_id}-Windows-x64-SDL3.zip"
            clean_path = artifact_root / "windows-clean-machine.json"
            clean = {
                "runtime_validator": "test_clean_release.ps1",
                "runtime_validator_sha256": trusted_sha,
                "validator_binding_checked": True,
            }
            clean_path.write_text(json.dumps(clean) + "\n", encoding="utf-8")
            binding_path = artifact_root / "runtime-validator-binding.json"
            binding = {
                "schema": "omnipack-release-evidence", "schema_version": 1,
                "test": "runtime_validator_binding", "run_id": run_id,
                "commit": commit, "status": "PASS", "passed": True,
                "candidate_filename": candidate, "candidate_sha256": "A" * 64,
                "runtime_validator_filename": "test_clean_release.ps1",
                "runtime_validator_sha256": trusted_sha,
                "clean_machine_evidence_filename": "windows-clean-machine.json",
                "clean_machine_evidence_sha256": release_finalizer.sha256(clean_path),
                "created_at": "2026-08-15T00:00:00Z",
            }
            binding_path.write_text(json.dumps(binding) + "\n", encoding="utf-8")
            self.assertEqual(
                release_finalizer.validate_runtime_validator_binding(
                    binding, binding_path=binding_path,
                    downloaded_validator=downloaded, trusted_validator=trusted,
                    clean_evidence_path=clean_path, clean=clean, run_id=run_id,
                    commit=commit, candidate_filename=candidate,
                    candidate_sha256="A" * 64,
                ),
                trusted_sha,
            )

            downloaded.write_bytes(b"replaced validator\n")
            with self.assertRaisesRegex(ValueError, "downloaded validator SHA256"):
                release_finalizer.validate_runtime_validator_binding(
                    binding, binding_path=binding_path,
                    downloaded_validator=downloaded, trusted_validator=trusted,
                    clean_evidence_path=clean_path, clean=clean, run_id=run_id,
                    commit=commit, candidate_filename=candidate,
                    candidate_sha256="A" * 64,
                )
            downloaded.write_bytes(trusted.read_bytes())

            wrong_binding = dict(binding)
            wrong_binding["runtime_validator_sha256"] = "0" * 64
            with self.assertRaisesRegex(ValueError, "runtime validator binding is invalid"):
                release_finalizer.validate_runtime_validator_binding(
                    wrong_binding, binding_path=binding_path,
                    downloaded_validator=downloaded, trusted_validator=trusted,
                    clean_evidence_path=clean_path, clean=clean, run_id=run_id,
                    commit=commit, candidate_filename=candidate,
                    candidate_sha256="A" * 64,
                )

            wrong_clean = dict(clean)
            wrong_clean["runtime_validator_sha256"] = "0" * 64
            with self.assertRaisesRegex(ValueError, "clean evidence runtime_validator_sha256"):
                release_finalizer.validate_runtime_validator_binding(
                    binding, binding_path=binding_path,
                    downloaded_validator=downloaded, trusted_validator=trusted,
                    clean_evidence_path=clean_path, clean=wrong_clean, run_id=run_id,
                    commit=commit, candidate_filename=candidate,
                    candidate_sha256="A" * 64,
                )

    def test_finalizer_rejects_wrong_or_dirty_trusted_checkout(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            repository = Path(temporary)
            subprocess.run(["git", "init", "-q"], cwd=repository, check=True)
            subprocess.run(["git", "config", "user.email", "finalizer@example.invalid"], cwd=repository, check=True)
            subprocess.run(["git", "config", "user.name", "Finalizer Test"], cwd=repository, check=True)
            tracked = repository / "trusted.txt"
            tracked.write_text("trusted\n", encoding="utf-8")
            subprocess.run(["git", "add", "trusted.txt"], cwd=repository, check=True)
            subprocess.run(["git", "commit", "-q", "-m", "trusted checkout"], cwd=repository, check=True)
            commit = subprocess.check_output(
                ["git", "rev-parse", "HEAD"], cwd=repository, text=True,
            ).strip()
            release_finalizer.validate_checkout_identity(commit, repository)
            with self.assertRaisesRegex(ValueError, "does not match aggregate commit"):
                release_finalizer.validate_checkout_identity("0" * 40, repository)
            tracked.write_text("dirty\n", encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "checkout is not clean"):
                release_finalizer.validate_checkout_identity(commit, repository)

    def test_finalizer_refuses_to_rewrite_stale_raw_artifact_identity(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            raw = root / "artifact-immutability.json"
            raw.write_text(json.dumps({
                "schema": "omnipack-release-evidence", "schema_version": 1,
                "test": "artifact_immutability", "status": "PASS", "passed": True,
                "run_id": "20260814T041500Z-8f31c1c7", "commit": "a" * 40,
                "candidate_sha256": "Z" * 64, "symbols_sha256": "Y" * 64,
                "symbols_member_sha256": "X" * 64,
            }), encoding="utf-8")
            validation = {
                "run_id": "20260814T041500Z-8f31c1c7", "commit": "a" * 40,
                "symbols_sha256": "B" * 64, "symbols_member_sha256": "C" * 64,
                "gates": {},
            }
            with self.assertRaisesRegex(ValueError, "stale candidate_sha256"):
                release_finalizer.set_gate(
                    validation, root, "ArtifactImmutability", "artifact_immutability",
                    "PASS", raw, "identity test", candidate_sha256="A" * 64,
                )
            unchanged = json.loads(raw.read_text(encoding="utf-8"))
            self.assertEqual(unchanged["candidate_sha256"], "Z" * 64)
            self.assertEqual(unchanged["symbols_sha256"], "Y" * 64)

    def test_finalizer_promotion_lock_is_exclusive_and_owner_checked(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            lock = Path(temporary) / ".stable.promotion.lock"
            run_id = "20260814T041500Z-8f31c1c7"
            release_finalizer.acquire_output_lock(lock, run_id=run_id, commit="a" * 40)
            with self.assertRaises(FileExistsError):
                release_finalizer.acquire_output_lock(
                    lock, run_id="20260814T041501Z-deadbeef", commit="b" * 40
                )
            release_finalizer.remove_owned_lock(lock, "20260814T041501Z-deadbeef")
            self.assertTrue(lock.is_file())
            release_finalizer.remove_owned_lock(lock, run_id)
            self.assertFalse(lock.exists())

    def test_finalizer_resets_stale_promotion_before_stable_names_exist(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            run_id = "20260814T041500Z-8f31c1c7"
            validation = {
                "run_id": run_id,
                "commit": "a" * 40,
                "candidate_sha256": "A" * 64,
                "symbols_sha256": "B" * 64,
                "symbols_member_sha256": "C" * 64,
                "gates": {
                    "CandidatePromotion": {"Status": "PASS"},
                },
            }
            for name in (
                "candidate-promotion.json",
                "bound-candidatepromotion.json",
                "gate-candidatepromotion.json",
            ):
                (root / name).write_text(
                    '{"status":"PASS","passed":true}\n', encoding="utf-8"
                )

            release_finalizer.reset_candidate_promotion_gate(
                validation, root, candidate_sha256="A" * 64,
            )

            raw = json.loads(
                (root / "candidate-promotion.json").read_text(encoding="utf-8")
            )
            self.assertEqual(raw["status"], "NOT_TESTED")
            self.assertFalse(raw["passed"])
            self.assertEqual(
                validation["gates"]["CandidatePromotion"]["Status"],
                "NOT_TESTED",
            )

        source = (ROOT / "tools/finalize_release_1_1_0.py").read_text(
            encoding="utf-8"
        ).split("def main() -> int:", 1)[1]
        reset = source.index("reset_candidate_promotion_gate(")
        pre_audit = source.index("run_audit(validation", reset)
        pre_blockers = source.index("pre_blockers = blockers", pre_audit)
        stable_copy = source.index("copy_verified(candidate, stable", pre_blockers)
        self.assertLess(reset, pre_audit)
        self.assertLess(pre_audit, pre_blockers)
        self.assertLess(pre_blockers, stable_copy)

    def test_finalizer_never_removes_a_transaction_it_did_not_create(self) -> None:
        source = (ROOT / "tools/finalize_release_1_1_0.py").read_text(encoding="utf-8")
        mkdir = source.index("transaction.mkdir()")
        owned = source.index("transaction_owned = True", mkdir)
        self.assertLess(mkdir, owned)
        self.assertIn("if transaction is not None and transaction_owned:", source)

    def test_finalizer_publishes_complete_directory_transaction(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            run_id = "20260814T041500Z-8f31c1c7"
            commit = "a" * 40
            source_snapshot = "F" * 64
            input_root = root / "input"
            input_root.mkdir()
            candidate_stem = f"TPT-ZH-OmniPack-1.1.0-staging-{run_id}-Windows-x64-SDL3"
            symbols_stem = f"TPT-ZH-OmniPack-1.1.0-staging-{run_id}-Windows-x64-Symbols"
            candidate = input_root / f"{candidate_stem}.zip"
            symbols = input_root / f"{symbols_stem}.zip"
            with zipfile.ZipFile(candidate, "w") as archive:
                archive.writestr("payload.txt", b"candidate")
            debug_bytes = b"debug-symbols"
            with zipfile.ZipFile(symbols, "w") as archive:
                archive.writestr(f"{symbols_stem}/tpt-zh-omnipack.debug", debug_bytes)
            candidate_sha = hashlib.sha256(candidate.read_bytes()).hexdigest().upper()
            symbols_sha = hashlib.sha256(symbols.read_bytes()).hexdigest().upper()
            member_sha = hashlib.sha256(debug_bytes).hexdigest().upper()
            release_finalizer.write_sidecar(candidate)
            release_finalizer.write_sidecar(symbols)

            validation_root = root / "validation"
            validation_root.mkdir()
            validation_json = validation_root / "RELEASE-VALIDATION.json"
            validation_text = validation_root / "RELEASE-VALIDATION.txt"
            build_info = validation_root / "BUILD-INFO.txt"
            gates = {
                name: {"Name": name, "Status": "PASS"}
                for name in release_finalizer.MANDATORY_GATES + release_finalizer.SUPPLEMENTAL_GATES
            }
            validation = {
                "schema": "omnipack-release-validation", "schema_version": 1,
                "version": "1.1.0", "channel": "stable", "run_id": run_id,
                "commit": commit, "source_snapshot_sha256": source_snapshot,
                "build_inputs_sha256": "E" * 64,
                "candidate_sha256": candidate_sha, "symbols_sha256": symbols_sha,
                "symbols_member_sha256": member_sha, "status": "FINAL AUDIT BEFORE PROMOTION",
                "final_status": "FINAL AUDIT BEFORE PROMOTION", "blocking_items": [],
                "gates": gates,
            }
            validation_json.write_text(json.dumps(validation), encoding="utf-8")
            build_info.write_text("TPT-ZH OmniPack 1.1.0\nGit commit: " + commit + "\nChannel: stable\n", encoding="utf-8")
            runtime_artifact = root / "runtime-artifact"
            runtime_validator_directory = runtime_artifact / "runtime-validator"
            runtime_validator_directory.mkdir(parents=True)
            clean = runtime_artifact / "windows-clean-machine.json"
            binding = runtime_artifact / "runtime-validator-binding.json"
            runtime_validator = runtime_validator_directory / "test_clean_release.ps1"
            runtime_validator.write_bytes((ROOT / "tools/test_clean_release.ps1").read_bytes())
            runtime_validator_sha = release_finalizer.sha256(runtime_validator)
            runtime_stdout = runtime_artifact / "windows_clean_machine.stdout.txt"
            runtime_stderr = runtime_artifact / "windows_clean_machine.stderr.txt"
            runtime_inner = runtime_artifact / "windows_clean_machine.inner.json"
            runtime_stdout.write_text("portable runtime PASS\n", encoding="utf-8")
            runtime_stderr.write_text("", encoding="utf-8")
            inner_value = {
                "schema": "omnipack-release-evidence", "schema_version": 1,
                "payload_schema_version": 2,
                "test": "portable_runtime", "status": "PASS", "passed": True,
                "run_id": run_id, "candidate_sha256": candidate_sha,
                "initial_particle_count": 2, "post_step_particle_count": 2,
                "final_particle_count": 2, "atmosphere_non_finite_cells": 0,
            }
            inner_value.update({field: True for field in release_validation_audit.PORTABLE_RUNTIME_TRUE_FIELDS})
            runtime_inner.write_text(json.dumps(inner_value), encoding="utf-8")
            clean_value = {
                "schema": "omnipack-release-evidence", "schema_version": 1,
                "test": "windows_clean_machine", "run_id": run_id,
                "status": "PASS", "passed": True, "candidate_sha256": candidate_sha,
                "candidate_filename": f"TPT-ZH-OmniPack-1.1.0-staging-{run_id}-Windows-x64-SDL3.zip",
                "source_checkout_used": False, "project_build_tool_dependency_used": False,
                "source_tree_indicators": [], "source_markers_checked": True,
                "development_tool_probe_completed": True,
                "development_tools_detected": {
                    name: None for name in ("gcc.exe", "g++.exe", "meson.exe", "ninja.exe", "glslc.exe", "bash.exe")
                },
                "runtime_validator": "test_clean_release.ps1",
                "runtime_validator_sha256": runtime_validator_sha,
                "validator_binding_checked": True,
                "runtime_job_kind": "runtime-only",
                "candidate_extracted_to_fresh_directory": True,
                "candidate_source_markers_checked": True,
                "candidate_source_tree_indicators": [],
                "runtime_target_executable_count": 1,
                "sanitized_path_used": True, "runtime_payload_schema_version": 2,
                "clean_shutdown": True,
                "initial_particle_count": 2, "post_step_particle_count": 2,
                "final_particle_count": 2, "atmosphere_non_finite_cells": 0,
                "runtime_stdout": runtime_stdout.name,
                "runtime_stdout_sha256": hashlib.sha256(runtime_stdout.read_bytes()).hexdigest().upper(),
                "runtime_stderr": runtime_stderr.name,
                "runtime_stderr_sha256": hashlib.sha256(runtime_stderr.read_bytes()).hexdigest().upper(),
                "runtime_inner_evidence": runtime_inner.name,
                "runtime_inner_evidence_sha256": hashlib.sha256(runtime_inner.read_bytes()).hexdigest().upper(),
                "gate_started_at": "2026-08-14T00:00:00Z", "gate_finished_at": "2026-08-14T00:01:00Z",
            }
            clean_value.update({field: True for field in release_validation_audit.PORTABLE_RUNTIME_TRUE_FIELDS})
            clean.write_text(json.dumps(clean_value), encoding="utf-8")
            binding.write_text(json.dumps({
                "schema": "omnipack-release-evidence", "schema_version": 1,
                "test": "runtime_validator_binding", "run_id": run_id,
                "commit": commit, "status": "PASS", "passed": True,
                "candidate_filename": candidate.name, "candidate_sha256": candidate_sha,
                "runtime_validator_filename": "test_clean_release.ps1",
                "runtime_validator_sha256": runtime_validator_sha,
                "clean_machine_evidence_filename": clean.name,
                "clean_machine_evidence_sha256": release_finalizer.sha256(clean),
                "created_at": "2026-08-14T00:01:01Z",
            }), encoding="utf-8")

            def fake_audit(value, audit_root, json_path, text_path, info_path, package_path,
                           symbols_path=None, **_artifact_paths):
                release_finalizer.write_json(json_path, value)
                release_finalizer.write_validation_text(value, text_path)

            output = root / "final-release"
            argv = [
                "finalize_release_1_1_0.py", "--validation-json", str(validation_json),
                "--candidate", str(candidate), "--symbols", str(symbols),
                "--clean-machine-evidence", str(clean),
                "--runtime-validator-binding", str(binding),
                "--runtime-validator", str(runtime_validator),
                "--output-directory", str(output),
            ]
            with mock.patch.object(release_finalizer.package_audit, "audit_package", return_value=[]), \
                 mock.patch.object(release_finalizer, "validate_source_manifest"), \
                 mock.patch.object(release_finalizer, "validate_checkout_identity"), \
                 mock.patch.object(release_finalizer, "run_audit", side_effect=fake_audit), \
                 mock.patch.object(release_finalizer, "run_negative_suite"), \
                 mock.patch.object(release_finalizer, "refresh_artifact_immutability"), \
                 mock.patch.object(
                     release_finalizer, "audit_frozen_evidence_bundle",
                     side_effect=lambda bundle, **_arguments: {
                         "passed": True,
                         "bundle_sha256": release_finalizer.sha256(bundle),
                         "members_total": 4,
                     },
                 ) as frozen_audit, \
                 mock.patch.object(sys, "argv", argv), redirect_stdout(io.StringIO()):
                self.assertEqual(release_finalizer.main(), 0)

            self.assertTrue(output.is_dir())
            self.assertEqual(
                hashlib.sha256((output / f"TPT-ZH-OmniPack-1.1.0-Windows-x64-SDL3.zip").read_bytes()).hexdigest().upper(),
                candidate_sha,
            )
            marker = json.loads((output / "PROMOTION-COMPLETE.json").read_text(encoding="utf-8"))
            self.assertTrue(marker["transaction_complete"])
            self.assertEqual(marker["run_id"], run_id)
            self.assertTrue(marker["evidence_bundle_audit_passed"])
            self.assertEqual(frozen_audit.call_count, 1)
            self.assertFalse((output.parent / f".{output.name}.promotion.lock").exists())
            self.assertTrue(candidate.exists())
            self.assertTrue(symbols.exists())
            source_validation = json.loads(validation_json.read_text(encoding="utf-8"))
            self.assertEqual(source_validation["final_status"], "READY FOR 1.1.0 STABLE")
            self.assertEqual(
                source_validation["gates"]["CandidatePromotion"]["Status"],
                "PASS",
            )
            promotion = json.loads(
                (validation_json.parent / "candidate-promotion.json").read_text(
                    encoding="utf-8"
                )
            )
            self.assertEqual(promotion["promotion_phase"], "published_and_reaudited")
            self.assertTrue(promotion["transaction_complete"])

            attacked_output = root / "attacked-release"
            attacked_argv = [
                "finalize_release_1_1_0.py", "--validation-json", str(validation_json),
                "--candidate", str(candidate), "--symbols", str(symbols),
                "--clean-machine-evidence", str(clean),
                "--runtime-validator-binding", str(binding),
                "--runtime-validator", str(runtime_validator),
                "--output-directory", str(attacked_output),
            ]
            attack_calls = {"count": 0, "member": None}

            def tampering_frozen_audit(bundle, **_arguments):
                attack_calls["count"] += 1
                audited_sha256 = release_finalizer.sha256(bundle)
                if attack_calls["count"] == 1:
                    with zipfile.ZipFile(bundle) as archive:
                        members = [(info, archive.read(info.filename)) for info in archive.infolist()]
                    target = next(info.filename for info, _ in members if info.filename == "candidate-promotion.json")
                    temporary_bundle = bundle.with_suffix(".attack.tmp")
                    with zipfile.ZipFile(temporary_bundle, "w", zipfile.ZIP_DEFLATED) as archive:
                        for info, data in members:
                            archive.writestr(
                                info,
                                b'{"status":"FAIL","passed":false}\n' if info.filename == target else data,
                            )
                    os.replace(temporary_bundle, bundle)
                    release_finalizer.write_sidecar(bundle)
                    prepared = bundle.parent / "PROMOTION-PREPARED.json"
                    marker = json.loads(prepared.read_text(encoding="utf-8"))
                    marker["evidence_sha256"] = release_finalizer.sha256(bundle)
                    release_finalizer.write_json(prepared, marker)
                    release_finalizer.write_sidecar(prepared)
                    attack_calls["member"] = target
                return {
                    "passed": True,
                    # The callback reports the exact bytes it audited. The
                    # attack above replaces the ZIP only after that audit.
                    "bundle_sha256": audited_sha256,
                    "members_total": 4,
                }

            attack_stderr = io.StringIO()
            with mock.patch.object(release_finalizer.package_audit, "audit_package", return_value=[]), \
                 mock.patch.object(release_finalizer, "validate_source_manifest"), \
                 mock.patch.object(release_finalizer, "validate_checkout_identity"), \
                 mock.patch.object(release_finalizer, "run_audit", side_effect=fake_audit), \
                 mock.patch.object(release_finalizer, "run_negative_suite"), \
                 mock.patch.object(release_finalizer, "refresh_artifact_immutability"), \
                 mock.patch.object(
                     release_finalizer, "audit_frozen_evidence_bundle",
                     side_effect=tampering_frozen_audit,
                 ), \
                 mock.patch.object(sys, "argv", attacked_argv), \
                 redirect_stdout(io.StringIO()), redirect_stderr(attack_stderr):
                self.assertEqual(release_finalizer.main(), 1)

            self.assertEqual(attack_calls["count"], 1)
            self.assertEqual(attack_calls["member"], "candidate-promotion.json")
            self.assertIn("changed after the completion semantic audit", attack_stderr.getvalue())
            self.assertFalse(attacked_output.exists())
            self.assertFalse((attacked_output / "PROMOTION-COMPLETE.json").exists())

    def test_published_documents_must_match_frozen_bundle_even_if_claims_are_reforged(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            documents = (
                root / "RELEASE-VALIDATION.json",
                root / "RELEASE-VALIDATION.txt",
                root / "BUILD-INFO.txt",
            )
            for index, document in enumerate(documents):
                document.write_text(f"original-{index}\n", encoding="utf-8")
                release_finalizer.write_sidecar(document)
            bundle = root / "evidence.zip"
            with zipfile.ZipFile(bundle, "w", zipfile.ZIP_DEFLATED) as archive:
                for document in documents:
                    archive.write(document, document.name)
            marker = {
                "validation_filename": documents[0].name,
                "validation_sha256": release_finalizer.sha256(documents[0]),
                "validation_text_filename": documents[1].name,
                "validation_text_sha256": release_finalizer.sha256(documents[1]),
                "build_info_filename": documents[2].name,
                "build_info_sha256": release_finalizer.sha256(documents[2]),
            }
            release_finalizer.validate_published_document_set(
                bundle=bundle, documents=documents, marker=marker
            )

            documents[0].write_text("forged\n", encoding="utf-8")
            release_finalizer.write_sidecar(documents[0])
            marker["validation_sha256"] = release_finalizer.sha256(documents[0])
            with self.assertRaisesRegex(ValueError, "frozen evidence bundle"):
                release_finalizer.validate_published_document_set(
                    bundle=bundle, documents=documents, marker=marker
                )

    def test_completion_marker_rejects_bundle_replaced_after_semantic_audit(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            bundle = root / "evidence.zip"
            with zipfile.ZipFile(bundle, "w", zipfile.ZIP_DEFLATED) as archive:
                archive.writestr("evidence.json", b'{"status":"PASS"}\n')
            audited = release_finalizer.sha256(bundle)
            replacement = root / "replacement.zip"
            with zipfile.ZipFile(replacement, "w", zipfile.ZIP_DEFLATED) as archive:
                archive.writestr("evidence.json", b'{"status":"FAIL"}\n')
            os.replace(replacement, bundle)
            with self.assertRaisesRegex(ValueError, "completion semantic audit"):
                release_finalizer.validate_bundle_audit_identity(
                    {"passed": True, "bundle_sha256": audited},
                    bundle,
                    phase="completion",
                )

    def test_frozen_evidence_bundle_is_semantically_reaudited(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            run_id = "20260814T041500Z-8f31c1c7"
            commit = "a" * 40
            candidate_sha = "A" * 64
            symbols_sha = "B" * 64
            member_sha = "C" * 64
            raw = {
                "schema": "omnipack-release-evidence", "schema_version": 1,
                "test": "official_tpt_save_compatibility",
                "gate_name": "OfficialTPTSaveCompatibility",
                "run_id": run_id, "commit": commit,
                "status": "NOT_TESTED", "passed": False,
                "gate_started_at": "2026-08-14T00:00:00Z",
                "gate_finished_at": "2026-08-14T00:00:01Z",
            }
            raw_path = root / "official-raw.json"
            raw_path.write_text(json.dumps(raw), encoding="utf-8")
            envelope = {
                "schema": "omnipack-release-evidence", "schema_version": 1,
                "test": "official_tpt_save_compatibility",
                "gate_name": "OfficialTPTSaveCompatibility",
                "run_id": run_id, "commit": commit,
                "status": "PASS", "passed": True, "exit_code": 0,
                "gate_started_at": "2026-08-14T00:00:00Z",
                "gate_finished_at": "2026-08-14T00:00:01Z",
                "source_evidence": raw_path.name,
                "source_evidence_sha256": hashlib.sha256(raw_path.read_bytes()).hexdigest().upper(),
            }
            envelope_path = root / "official-envelope.json"
            envelope_path.write_text(json.dumps(envelope), encoding="utf-8")
            validation = {
                "schema": "omnipack-release-validation", "schema_version": 1,
                "version": "1.1.0", "channel": "stable", "run_id": run_id,
                "commit": commit, "source_snapshot_sha256": "D" * 64,
                "candidate_sha256": candidate_sha, "symbols_sha256": symbols_sha,
                "symbols_member_sha256": member_sha,
                "status": "READY FOR 1.1.0 STABLE",
                "final_status": "READY FOR 1.1.0 STABLE", "blocking_items": [],
                "gates": {
                    "OfficialTPTSaveCompatibility": {
                        "Name": "OfficialTPTSaveCompatibility", "Status": "PASS", "ExitCode": 0,
                        "Evidence": envelope_path.name,
                        "EvidenceSha256": hashlib.sha256(envelope_path.read_bytes()).hexdigest().upper(),
                    }
                },
            }
            (root / "RELEASE-VALIDATION.json").write_text(json.dumps(validation), encoding="utf-8")
            (root / "RELEASE-VALIDATION.txt").write_text("synthetic semantic attack\n", encoding="utf-8")
            (root / "BUILD-INFO.txt").write_text("synthetic semantic attack\n", encoding="utf-8")
            (root / "run-metadata.json").write_text(json.dumps({
                "run_id": run_id, "commit": commit, "candidate_sha256": candidate_sha,
                "symbols_sha256": symbols_sha, "symbols_member_sha256": member_sha,
                "final_status": "READY FOR 1.1.0 STABLE",
            }), encoding="utf-8")
            bundle = root / "evidence.zip"
            with zipfile.ZipFile(bundle, "w", zipfile.ZIP_DEFLATED) as archive:
                for path in root.glob("*.json"):
                    archive.write(path, path.name)
                archive.write(root / "RELEASE-VALIDATION.txt", "RELEASE-VALIDATION.txt")
                archive.write(root / "BUILD-INFO.txt", "BUILD-INFO.txt")
            package = root / "stable.zip"
            package.write_bytes(b"synthetic package")
            symbols_package = root / "symbols.zip"
            symbols_package.write_bytes(b"synthetic symbols")
            candidate_sha = hashlib.sha256(package.read_bytes()).hexdigest().upper()
            symbols_sha = hashlib.sha256(symbols_package.read_bytes()).hexdigest().upper()
            validation["candidate_sha256"] = candidate_sha
            validation["symbols_sha256"] = symbols_sha
            (root / "RELEASE-VALIDATION.json").write_text(json.dumps(validation), encoding="utf-8")
            metadata = json.loads((root / "run-metadata.json").read_text(encoding="utf-8"))
            metadata["candidate_sha256"] = candidate_sha
            metadata["symbols_sha256"] = symbols_sha
            (root / "run-metadata.json").write_text(json.dumps(metadata), encoding="utf-8")
            with zipfile.ZipFile(bundle, "w", zipfile.ZIP_DEFLATED) as archive:
                for path in root.glob("*.json"):
                    archive.write(path, path.name)
                archive.write(root / "RELEASE-VALIDATION.txt", "RELEASE-VALIDATION.txt")
                archive.write(root / "BUILD-INFO.txt", "BUILD-INFO.txt")
            with mock.patch.object(release_finalizer.auditor, "audit_docs", return_value=(True, [])):
                with self.assertRaisesRegex(ValueError, "frozen evidence bundle audit failed"):
                    release_finalizer.audit_frozen_evidence_bundle(
                        bundle,
                        package=package,
                        symbols_package=symbols_package,
                        candidate_artifact=package,
                        symbols_artifact=symbols_package,
                        run_id=run_id,
                        commit=commit,
                        candidate_sha256=candidate_sha,
                        symbols_sha256=symbols_sha,
                        symbols_member_sha256=member_sha,
                    )

    def test_finalizer_binds_package_manifest_to_aggregate_source_snapshot(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            package = root / "candidate.zip"
            stem = "candidate"
            manifest = (
                "format=2\nkind=release\nversion=1.1.0\n"
                f"revision={'a' * 40}\nsource_state=clean\n"
                f"source_worktree_sha256={'A' * 64}\nsource_untracked_files=0\n"
                f"build_inputs_sha256={'C' * 64}\nbuild_inputs_ready=true\n"
            )
            with zipfile.ZipFile(package, "w") as archive:
                archive.writestr(f"{stem}/MANIFEST.txt", manifest)
            release_finalizer.validate_source_manifest(
                package, stem, commit="a" * 40, source_snapshot_sha256="A" * 64,
                build_inputs_sha256="C" * 64,
            )
            with self.assertRaisesRegex(ValueError, "source manifest is not bound"):
                release_finalizer.validate_source_manifest(
                    package, stem, commit="b" * 40, source_snapshot_sha256="A" * 64,
                    build_inputs_sha256="C" * 64,
                )
            with self.assertRaisesRegex(ValueError, "source manifest is not bound"):
                release_finalizer.validate_source_manifest(
                    package, stem, commit="a" * 40, source_snapshot_sha256="B" * 64,
                    build_inputs_sha256="C" * 64,
                )

    def test_gui_pass_with_missing_screenshot_is_semantically_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            errors = release_validation_audit.validate_raw_semantics(
                "SDL3GUI",
                {
                    "artifact": "missing.bmp", "window_created": True,
                    "frame_rendered": True, "resize": True, "fullscreen_toggle": True,
                    "keyboard": True, "mouse": True, "text_input": True,
                    "clipboard": True, "screenshot_created": True,
                    "clean_shutdown": True, "restart": True,
                },
                root=root, candidate_sha256="A" * 64,
            )
            self.assertTrue(any("screenshot" in error.lower() for error in errors))

    def test_gui_sdl_bitfields_bmp_is_semantically_supported(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            screenshot = root / "gui-smoke.bmp"
            width, height = 2, 1
            pixel_offset = 14 + 124
            pixels = bytes((0xA9, 0x72, 0x11, 0xFF, 0x3A, 0xD4, 0xE8, 0xFF))
            file_header = b"BM" + struct.pack(
                "<IHHI", pixel_offset + len(pixels), 0, 0, pixel_offset
            )
            dib = struct.pack(
                "<IiiHHIIiiII",
                124, width, height, 1, 32, 3, len(pixels), 0, 0, 0, 0,
            )
            masks = struct.pack(
                "<IIII", 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000
            )
            screenshot.write_bytes(file_header + dib + masks + bytes(124 - 56) + pixels)
            evidence = {
                "screenshot_width": width,
                "screenshot_height": height,
                "screenshot_bytes": screenshot.stat().st_size,
                "screenshot_pixel_count": width * height,
                "screenshot_nonzero_pixels": width * height,
                "screenshot_distinct_colors": 2,
            }

            self.assertEqual(
                release_validation_audit.validate_bmp(screenshot, evidence), []
            )

    def test_negative_suite_requires_valid_baseline_packages(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            run_id = release_negative_suite.RUN_ID
            candidate = root / f"TPT-ZH-OmniPack-1.1.0-staging-{run_id}-Windows-x64-SDL3.zip"
            symbols = root / f"TPT-ZH-OmniPack-1.1.0-staging-{run_id}-Windows-x64-Symbols.zip"
            with zipfile.ZipFile(candidate, "w") as archive:
                archive.writestr("candidate/PACKAGE-MANIFEST.sha256", "0" * 64)
                archive.writestr("candidate/tpt-zh-omnipack.exe", b"test executable")
            with zipfile.ZipFile(symbols, "w"):
                pass
            with mock.patch.object(release_negative_suite, "CANDIDATE", release_negative_suite.digest(candidate)), \
                 mock.patch.object(
                     release_negative_suite.package_audit,
                     "audit_package",
                     side_effect=[["baseline candidate is invalid"], []],
                 ):
                attacks, baselines, details = release_negative_suite.run(candidate, symbols, "candidate", "symbols")
            self.assertFalse(attacks["package_manifest_corruption"])
            self.assertFalse(attacks["symbols_missing"])
            self.assertFalse(baselines["candidate_package"])
            self.assertTrue(any("baseline candidate is invalid" in item for item in details))

    def test_negative_suite_promotion_attacks_start_from_valid_identity(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            run_id = release_negative_suite.RUN_ID
            candidate = root / (
                f"TPT-ZH-OmniPack-1.1.0-staging-{run_id}-Windows-x64-SDL3.zip"
            )
            symbols = root / (
                f"TPT-ZH-OmniPack-1.1.0-staging-{run_id}-Windows-x64-Symbols.zip"
            )
            candidate.write_bytes(b"candidate")
            symbols.write_bytes(b"symbols")

            attacks, baselines, details = release_negative_suite.promotion_attacks(
                root, candidate, symbols
            )

            self.assertTrue(all(baselines.values()), (baselines, details))
            self.assertTrue(baselines["promotion_current_input_identity"])
            self.assertTrue(baselines["promotion_current_staging_input_identity"])
            self.assertTrue(baselines["promotion_staging_input_identity_schema"])
            self.assertTrue(baselines["promotion_prepared_fixture_identity"])
            self.assertTrue(baselines["promotion_completed_stable_fixture_identity"])
            self.assertTrue(attacks)
            self.assertTrue(all(attacks.values()), attacks)

    def test_negative_suite_promotion_separates_rc_prepared_and_stable_identities(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            candidate = root / release_negative_suite.RC_CANDIDATE_FILENAME
            symbols = root / release_negative_suite.RC_SYMBOLS_FILENAME
            candidate.write_bytes(b"rc candidate")
            symbols.write_bytes(b"rc symbols")

            attacks, baselines, details = release_negative_suite.promotion_attacks(
                root, candidate, symbols
            )

            self.assertEqual(details, [])
            self.assertTrue(all(baselines.values()), baselines)
            self.assertTrue(baselines["promotion_rc_input_identity_schema"])
            self.assertTrue(baselines["promotion_current_input_identity"])
            self.assertTrue(baselines["promotion_current_rc_input_identity"])
            self.assertTrue(baselines["promotion_prepared_fixture_identity"])
            self.assertTrue(baselines["promotion_completed_stable_fixture_identity"])
            self.assertTrue(all(attacks.values()), attacks)

            # A stable-named output is not a valid pre-promotion input.  This
            # prevents a completed promotion identity from being reused as a
            # current RC/staging candidate baseline.
            stable = root / release_negative_suite.STABLE_FILENAME
            stable_symbols = root / release_negative_suite.STABLE_SYMBOLS_FILENAME
            stable.write_bytes(b"stable")
            stable_symbols.write_bytes(b"stable symbols")
            _, stable_baselines, stable_details = release_negative_suite.promotion_attacks(
                root / "stable-attempt", stable, stable_symbols
            )
            self.assertFalse(stable_baselines["promotion_current_input_identity"])
            self.assertTrue(any("stable names are post-promotion output only" in item for item in stable_details))

    def test_negative_suite_manifest_attack_starts_from_valid_packages(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            run_id = release_negative_suite.RUN_ID
            candidate = root / f"TPT-ZH-OmniPack-1.1.0-staging-{run_id}-Windows-x64-SDL3.zip"
            symbols = root / f"TPT-ZH-OmniPack-1.1.0-staging-{run_id}-Windows-x64-Symbols.zip"
            with zipfile.ZipFile(candidate, "w") as archive:
                archive.writestr("candidate/PACKAGE-MANIFEST.sha256", "0" * 64)
                archive.writestr("candidate/tpt-zh-omnipack.exe", b"test executable")
            with zipfile.ZipFile(symbols, "w"):
                pass
            with mock.patch.object(release_negative_suite, "CANDIDATE", release_negative_suite.digest(candidate)), \
                 mock.patch.object(
                     release_negative_suite.package_audit,
                     "audit_package",
                     side_effect=[[], [], ["manifest hash mismatch"], ["package is absent"]],
                 ) as audit_package:
                attacks, baselines, details = release_negative_suite.run(candidate, symbols, "candidate", "symbols")
            self.assertTrue(attacks["package_manifest_corruption"])
            self.assertTrue(attacks["symbols_missing"])
            self.assertTrue(baselines["candidate_package"])
            self.assertTrue(baselines["symbols_package"])
            self.assertEqual(details, [])
            self.assertEqual(audit_package.call_count, 4)

    def test_negative_suite_provenance_attacks_require_positive_baseline(self) -> None:
        with tempfile.TemporaryDirectory() as temporary, mock.patch.object(
            release_negative_suite.provenance,
            "validate_provenance",
            return_value={"status": "FAIL", "passed": False, "errors": ["reject all"]},
        ):
            attacks, baseline_passed, details = release_negative_suite.provenance_attacks(
                Path(temporary)
            )
        self.assertFalse(baseline_passed)
        self.assertTrue(attacks)
        self.assertTrue(all(value is False for value in attacks.values()))
        self.assertTrue(any("positive baseline" in item for item in details))

    def test_negative_suite_cli_rejects_wrong_candidate_sha(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            run_id = "20260815T070000Z-deadbeef"
            candidate = root / f"TPT-ZH-OmniPack-1.1.0-staging-{run_id}-Windows-x64-SDL3.zip"
            symbols = root / f"TPT-ZH-OmniPack-1.1.0-staging-{run_id}-Windows-x64-Symbols.zip"
            candidate.write_bytes(b"candidate")
            symbols.write_bytes(b"symbols")
            output = root / "negative.json"
            argv = [
                "release_negative_gate_suite.py",
                "--candidate", str(candidate), "--symbols", str(symbols),
                "--artifact-stem", candidate.stem,
                "--symbol-artifact-stem", symbols.stem,
                "--run-id", run_id, "--commit", "a" * 40,
                "--candidate-sha256", "A" * 64,
                "--package-version", "1.1.0", "--package-kind", "release",
                "--output", str(output),
            ]
            with mock.patch.object(sys, "argv", argv), mock.patch.multiple(
                release_negative_suite,
                RUN_ID="20260814T041500Z-8f31c1c7",
                COMMIT="a" * 40,
                CANDIDATE="B" * 64,
            ):
                self.assertEqual(release_negative_suite.main(), 1)
            result = json.loads(output.read_text(encoding="utf-8"))
            self.assertEqual(result["status"], "FAIL")
            self.assertIn("does not match candidate bytes", result["reason"])

    def test_negative_suite_semantic_case_uses_runtime_run_id(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            stdout = root / "stdout.txt"
            stderr = root / "stderr.txt"
            stdout.write_text("ok\n", encoding="utf-8")
            stderr.write_text("", encoding="utf-8")
            runtime_run_id = "20260815T071500Z-cafebabe"
            with mock.patch.object(release_negative_suite, "RUN_ID", runtime_run_id):
                hash_ok, semantic_ok = release_negative_suite.semantic_case(
                    root,
                    "Build",
                    "build",
                    {
                        "status": "PASS", "passed": True, "exit_code": 0,
                        "stdout": stdout.name,
                        "stdout_sha256": release_negative_suite.digest(stdout),
                        "stderr": stderr.name,
                        "stderr_sha256": release_negative_suite.digest(stderr),
                    },
                )
            self.assertTrue(hash_ok)
            self.assertTrue(semantic_ok)

    def test_negative_suite_is_bound_to_current_candidate_and_complete_matrix(self) -> None:
        attacks = {
            name: True for name in release_validation_audit.REQUIRED_NEGATIVE_ATTACKS
        }
        raw = {
            "status": "PASS", "passed": True,
            "candidate_sha256": "B" * 64,
            "candidate_sha256_observed": "B" * 64,
            "baselines_total": 1, "baselines_passed": 1,
            "baselines_failed": [], "baselines": {"valid": True},
            "attacks_total": len(attacks), "attacks_rejected": len(attacks),
            "attacks_failed": [], "attacks": attacks,
        }
        self.assertEqual(
            release_validation_audit.validate_raw_semantics(
                "NegativeGateSuite", raw, root=Path.cwd(),
                candidate_sha256="B" * 64,
            ),
            [],
        )
        raw["attacks"]["aggregate_pass_with_failed_evidence"] = False
        errors = release_validation_audit.validate_raw_semantics(
            "NegativeGateSuite", raw, root=Path.cwd(),
            candidate_sha256="B" * 64,
        )
        self.assertTrue(any("attack matrix" in error for error in errors))

    def test_candidate_promotion_requires_all_identity_invariants(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            run_id = "20260814T041500Z-8f31c1c7"
            candidate = root / f"TPT-ZH-OmniPack-1.1.0-staging-{run_id}-Windows-x64-SDL3.zip"
            symbols = root / f"TPT-ZH-OmniPack-1.1.0-staging-{run_id}-Windows-x64-Symbols.zip"
            candidate.write_bytes(b"candidate")
            symbols.write_bytes(b"symbols")
            candidate_sha = hashlib.sha256(candidate.read_bytes()).hexdigest().upper()
            symbols_sha = hashlib.sha256(symbols.read_bytes()).hexdigest().upper()
            raw = {
                "run_id": run_id,
                "candidate_filename": candidate.name,
                "candidate_sha256": candidate_sha,
                "stable_filename": "TPT-ZH-OmniPack-1.1.0-Windows-x64-SDL3.zip",
                "stable_sha256_expected": candidate_sha,
                "symbols_candidate_filename": symbols.name,
                "symbols_sha256": symbols_sha,
                "stable_symbols_filename": "TPT-ZH-OmniPack-1.1.0-Windows-x64-Symbols.zip",
                "stable_symbols_sha256_expected": symbols_sha,
                "symbols_member_sha256": "C" * 64,
                "promotion_phase": "published_and_reaudited",
                "publication_state": "published_and_reaudited",
                "transaction_complete": True,
                "post_publish_audit_passed": True,
                "stable_copy_published": True,
                "stable_sha256_observed": candidate_sha,
                "stable_symbols_copy_published": True,
                "stable_symbols_sha256_observed": symbols_sha,
                "byte_for_byte_identity": True,
                "atomic_rename_only": True,
                "atomic_directory_publish": True,
                "exclusive_output_lock_acquired": True,
                "promotion_complete_marker_required": True,
                "stable_names_absent_before_final_audit": True,
                "pre_promotion_gate_finished_at": "2000-01-01T00:00:00+00:00",
                "stable_name_creation_started_at": "2000-01-01T00:00:01+00:00",
                "pre_promotion_gate_passed": True,
                "stable_names_absent_before_pre_promotion_gate": True,
            }
            self.assertEqual(
                release_validation_audit.validate_raw_semantics(
                    "CandidatePromotion", raw, root=root, candidate_sha256=candidate_sha,
                    candidate_path=candidate, symbols_path=symbols,
                ),
                [],
            )
            mutations = {
                "candidate_filename": "old-candidate.zip",
                "symbols_candidate_filename": "old-symbols.zip",
                "stable_filename": "wrong-stable.zip",
                "stable_sha256_expected": "D" * 64,
                "stable_symbols_filename": "wrong-symbols.zip",
                "stable_symbols_sha256_expected": "E" * 64,
                "byte_for_byte_identity": False,
                "atomic_rename_only": False,
                "atomic_directory_publish": False,
                "exclusive_output_lock_acquired": False,
                "promotion_complete_marker_required": False,
                "stable_names_absent_before_final_audit": False,
                "promotion_phase": "prepared_for_atomic_directory_publish",
                "transaction_complete": False,
                "stable_name_creation_started_at": "1999-12-31T23:59:59+00:00",
            }
            for field, value in mutations.items():
                with self.subTest(field=field):
                    attacked = dict(raw)
                    attacked[field] = value
                    errors = release_validation_audit.validate_raw_semantics(
                        "CandidatePromotion", attacked, root=root,
                        candidate_sha256=candidate_sha,
                        candidate_path=candidate, symbols_path=symbols,
                    )
                    self.assertTrue(errors, field)

    def test_ready_aggregate_requires_candidate_promotion_gate(self) -> None:
        gates = {
            name: {"Status": "PASS"}
            for name in release_validation_audit.STABLE_MANDATORY_GATES
            if name != "CandidatePromotion"
        }
        passed, errors = release_validation_audit.audit_docs(
            {
                "schema": "omnipack-release-validation",
                "schema_version": 1,
                "version": "1.1.0",
                "channel": "stable",
                "status": "READY FOR 1.1.0 STABLE",
                "final_status": "READY FOR 1.1.0 STABLE",
                "commit": "a" * 40,
                "gates": gates,
            },
            None, None, None, "stable",
        )
        self.assertFalse(passed)
        self.assertTrue(any("CandidatePromotion" in error for error in errors))

    def test_ready_documentation_audit_requires_inputs_and_no_blockers(self) -> None:
        gates = {
            name: {"Status": "PASS"}
            for name in release_validation_audit.STABLE_MANDATORY_GATES
        }
        passed, errors = release_validation_audit.audit_docs(
            {
                "schema": "omnipack-release-validation", "schema_version": 1,
                "version": "1.1.0", "channel": "stable",
                "status": "READY FOR 1.1.0 STABLE",
                "final_status": "READY FOR 1.1.0 STABLE",
                "commit": "a" * 40, "blocking_items": ["hidden blocker"],
                "gates": gates,
            },
            None, None, None, "stable",
        )
        self.assertFalse(passed)
        self.assertTrue(any("blocking_items" in error for error in errors))
        self.assertTrue(any("validation text is required" in error for error in errors))
        self.assertTrue(any("BUILD-INFO is required" in error for error in errors))
        self.assertTrue(any("release package is required" in error for error in errors))

    def test_ready_aggregate_requires_package_verification_gates(self) -> None:
        self.assertIn("PackageVerification", release_validation_audit.STABLE_MANDATORY_GATES)
        self.assertIn("SymbolPackageVerification", release_validation_audit.STABLE_MANDATORY_GATES)
        gates = {
            name: {"Status": "PASS"}
            for name in release_validation_audit.STABLE_MANDATORY_GATES
        }
        gates["PackageVerification"]["Status"] = "FAIL"
        gates["SymbolPackageVerification"]["Status"] = "FAIL"
        _, errors = release_validation_audit.audit_docs(
            {
                "schema": "omnipack-release-validation", "schema_version": 1,
                "version": "1.1.0", "channel": "stable",
                "status": "READY FOR 1.1.0 STABLE",
                "final_status": "READY FOR 1.1.0 STABLE",
                "commit": "a" * 40, "blocking_items": [], "gates": gates,
            },
            None, None, None, "stable",
        )
        self.assertTrue(any("PackageVerification" in error for error in errors))
        self.assertTrue(any("SymbolPackageVerification" in error for error in errors))

    def test_finalizer_keeps_candidate_promotion_post_package_only(self) -> None:
        self.assertIn("CandidatePromotion", release_finalizer.MANDATORY_GATES)
        self.assertNotIn("CandidatePromotion", release_finalizer.PRE_PROMOTION_GATES)

    def test_provenance_summary_cannot_hide_a_bad_file_row(self) -> None:
        raw = {
            "repository": official_provenance.OFFICIAL_REPOSITORY,
            "revision": "a" * 40,
            "revision_exists": True,
            "revision_reachable_from_official_remote": True,
            "files_total": 1, "files_verified": 1, "files_failed": 0,
            "files": [{
                "path": "tests/save.cps", "match": True,
                "upstream_sha256": "A" * 64,
                "manifest_sha256": "A" * 64,
                "fixture_sha256": "B" * 64,
            }],
        }
        errors = release_validation_audit.validate_raw_semantics(
            "OfficialTPTCorpusProvenance", raw, root=Path.cwd(),
            candidate_sha256=None,
        )
        self.assertTrue(any("hash-inconsistent" in error for error in errors))

    def test_compatibility_summary_cannot_hide_a_failed_runtime_phase(self) -> None:
        row = {
            "path": "tests/save.cps", "fixture_sha256": "A" * 64,
            "probe_sha256": "C" * 64,
            "provenance_hash_binding_passed": True,
            "load": True, "missing_elements_zero": True, "initial_load_state_validate": True,
            "simulate": True, "save": True,
            "reload": False, "state_validate": True,
            "input_particles": 1, "initial_loaded_particles": 1,
            "output_particles": 1, "initial_particle_inventory": True,
            "negative_block_map": True, "negative_legacy_field": True,
            "negative_sign": True, "negative_validity_mask": True,
            "negative_deterministic_frame": True,
            "negative_simulation_option": True,
            "negative_codec_roundtrip": True,
            "passed": True, "exit_code": 0,
        }
        raw = {
            "source_repository": official_provenance.OFFICIAL_REPOSITORY,
            "source_revision": "a" * 40,
            "probe_sha256": "C" * 64,
            "files_total": 1, "files_passed": 1, "files_failed": 0,
            "files": [row],
        }
        errors = release_validation_audit.validate_raw_semantics(
            "OfficialTPTSaveCompatibility", raw, root=Path.cwd(),
            candidate_sha256=None,
        )
        self.assertTrue(any("runtime phase" in error for error in errors))

    def test_compatibility_summary_requires_probe_negative_attacks(self) -> None:
        row = {
            "path": "tests/save.cps", "fixture_sha256": "A" * 64,
            "probe_sha256": "C" * 64,
            "provenance_hash_binding_passed": True,
            "load": True, "missing_elements_zero": True, "initial_load_state_validate": True,
            "simulate": True, "save": True,
            "reload": True, "state_validate": True,
            "input_particles": 1, "initial_loaded_particles": 1,
            "output_particles": 1, "initial_particle_inventory": True,
            "negative_block_map": True, "negative_legacy_field": True,
            "negative_sign": True, "negative_validity_mask": True,
            "negative_deterministic_frame": True,
            "negative_simulation_option": True,
            # Deliberately omit negative_codec_roundtrip.
            "passed": True, "exit_code": 0,
        }
        raw = {
            "source_repository": official_provenance.OFFICIAL_REPOSITORY,
            "source_revision": "a" * 40,
            "probe_sha256": "C" * 64,
            "files_total": 1, "files_passed": 1, "files_failed": 0,
            "files": [row],
        }
        errors = release_validation_audit.validate_raw_semantics(
            "OfficialTPTSaveCompatibility", raw, root=Path.cwd(),
            candidate_sha256=None,
        )
        self.assertTrue(any("runtime phase" in error for error in errors))

    def test_official_compatibility_rehashes_fixture_after_provenance(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            corpus = root / "corpus"
            corpus.mkdir()
            fixture = corpus / "official.cps"
            fixture.write_bytes(b"modified-after-provenance")
            provenance_path = root / "provenance.json"
            original_sha = hashlib.sha256(b"official-upstream-bytes").hexdigest().upper()
            provenance_path.write_text(json.dumps({
                "schema": "omnipack-release-evidence", "schema_version": 1,
                "test": "official_tpt_provenance",
                "run_id": "20260814T041500Z-8f31c1c7", "commit": "a" * 40,
                "status": "PASS", "passed": True,
                "repository": official_provenance.OFFICIAL_REPOSITORY,
                "revision": "b" * 40, "revision_exists": True,
                "revision_reachable_from_official_remote": True,
                "files_total": 1, "files_verified": 1, "files_failed": 0,
                "files": [{
                    "path": "official.cps", "match": True,
                    "upstream_sha256": original_sha,
                    "manifest_sha256": original_sha,
                    "fixture_sha256": original_sha,
                }],
            }), encoding="utf-8")
            probe = root / "probe.exe"
            probe.write_bytes(b"not executed because hash binding fails")
            output = root / "compatibility.json"
            exit_code = official_compatibility.main([
                "--corpus", str(corpus), "--provenance-evidence", str(provenance_path),
                "--probe", str(probe), "--run-id", "20260814T041500Z-8f31c1c7",
                "--commit", "a" * 40, "--output", str(output),
            ])
            self.assertEqual(exit_code, 1)
            result = json.loads(output.read_text(encoding="utf-8"))
            self.assertFalse(result["passed"])
            self.assertFalse(result["files"][0]["provenance_hash_binding_passed"])

    def test_official_compatibility_requires_probe_attack_markers(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            corpus = root / "corpus"
            corpus.mkdir()
            fixture = corpus / "official.cps"
            fixture.write_bytes(b"official-upstream-bytes")
            digest = hashlib.sha256(fixture.read_bytes()).hexdigest().upper()
            provenance_path = root / "provenance.json"
            provenance_path.write_text(json.dumps({
                "schema": "omnipack-release-evidence", "schema_version": 1,
                "test": "official_tpt_provenance",
                "run_id": "20260814T041500Z-8f31c1c7", "commit": "a" * 40,
                "status": "PASS", "passed": True,
                "repository": official_provenance.OFFICIAL_REPOSITORY,
                "revision": "b" * 40, "revision_exists": True,
                "revision_reachable_from_official_remote": True,
                "files_total": 1, "files_verified": 1, "files_failed": 0,
                "files": [{
                    "path": "official.cps", "match": True,
                    "upstream_sha256": digest,
                    "manifest_sha256": digest,
                    "fixture_sha256": digest,
                }],
            }), encoding="utf-8")
            probe = root / "probe.exe"
            probe.write_bytes(b"synthetic probe identity")
            output = root / "compatibility.json"
            primary_markers = "\n".join((
                "official_save_load_pass=true",
                "official_save_missing_elements_zero=true",
                "official_save_initial_load_state_validate_pass=true",
                "official_save_simulate_pass=true",
                "official_save_save_pass=true",
                "official_save_reload_pass=true",
                "official_save_state_validate_pass=true",
                "input_particles=1",
                "initial_loaded_particles=1",
                "output_particles=1",
            ))
            completed = subprocess.CompletedProcess(
                [str(probe), str(fixture)], 0, primary_markers, ""
            )
            with mock.patch.object(official_compatibility.subprocess, "run", return_value=completed):
                exit_code = official_compatibility.main([
                    "--corpus", str(corpus), "--provenance-evidence", str(provenance_path),
                    "--probe", str(probe), "--run-id", "20260814T041500Z-8f31c1c7",
                    "--commit", "a" * 40, "--output", str(output),
                ])
            self.assertEqual(exit_code, 1)
            result = json.loads(output.read_text(encoding="utf-8"))
            self.assertFalse(result["passed"])
            self.assertFalse(result["files"][0]["negative_codec_roundtrip"])

    def test_official_compatibility_fails_closed_on_incomplete_coverage(self) -> None:
        """A file-complete run must still fail when the coverage contract is incomplete."""
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            corpus = root / "corpus"
            corpus.mkdir()
            fixture = corpus / "official.cps"
            fixture.write_bytes(b"official-upstream-bytes")
            digest = hashlib.sha256(fixture.read_bytes()).hexdigest().upper()
            provenance_path = root / "provenance.json"
            provenance_path.write_text(json.dumps({
                "schema": "omnipack-release-evidence", "schema_version": 1,
                "test": "official_tpt_provenance",
                "run_id": "20260814T041500Z-8f31c1c7", "commit": "a" * 40,
                "status": "PASS", "passed": True,
                "repository": official_provenance.OFFICIAL_REPOSITORY,
                "revision": "b" * 40, "revision_exists": True,
                "revision_reachable_from_official_remote": True,
                "files_total": 1, "files_verified": 1, "files_failed": 0,
                "files": [{
                    "path": "official.cps", "match": True,
                    "upstream_sha256": digest,
                    "manifest_sha256": digest,
                    "fixture_sha256": digest,
                }],
            }), encoding="utf-8")
            probe = root / "probe.exe"
            probe.write_bytes(b"synthetic probe identity")
            output = root / "compatibility.json"
            phase_markers = "\n".join((
                "official_save_load_pass=true",
                "official_save_missing_elements_zero=true",
                "official_save_initial_load_state_validate_pass=true",
                "official_save_simulate_pass=true",
                "official_save_save_pass=true",
                "official_save_reload_pass=true",
                "official_save_state_validate_pass=true",
                "official_save_negative_block_map_rejected=true",
                "official_save_negative_legacy_field_rejected=true",
                "official_save_negative_sign_rejected=true",
                "official_save_negative_validity_mask_rejected=true",
                "official_save_negative_deterministic_frame_rejected=true",
                "official_save_negative_simulation_option_rejected=true",
                "official_save_negative_codec_roundtrip_rejected=true",
                "input_particles=1",
                "initial_loaded_particles=1",
                "output_particles=1",
                "coverage_input_bytes=10000",
                "coverage_particles=1",
                "coverage_powders=1",
                "coverage_solids=1",
                "coverage_liquids=0",
                "coverage_gases=1",
                "coverage_temperature_signals=1",
                "coverage_pressure_cells=1",
                "coverage_velocity_signals=1",
                "coverage_wall_cells=1",
                "coverage_fan_cells=1",
                "coverage_electronics_particles=1",
                "coverage_life_particles=1",
                "coverage_signs=1",
                "coverage_decorated_particles=1",
                "coverage_legacy_state=1",
                "coverage_larger_save=1",
            ))
            completed = subprocess.CompletedProcess(
                [str(probe), str(fixture)], 0, phase_markers, ""
            )
            with mock.patch.object(official_compatibility.subprocess, "run", return_value=completed):
                exit_code = official_compatibility.main([
                    "--corpus", str(corpus), "--provenance-evidence", str(provenance_path),
                    "--probe", str(probe), "--run-id", "20260814T041500Z-8f31c1c7",
                    "--commit", "a" * 40, "--output", str(output),
                ])
            self.assertEqual(exit_code, 1)
            result = json.loads(output.read_text(encoding="utf-8"))
            self.assertEqual(result["status"], "FAIL")
            self.assertFalse(result["passed"])
            self.assertEqual(result["files_failed"], 0)
            self.assertFalse(result["coverage_passed"])
            self.assertIn("liquids", result["coverage_missing"])

    def test_soak_summary_cannot_hide_nonfinite_or_stall_counts(self) -> None:
        raw = {
            "wall_clock_seconds": 7200.0,
            "long_run_gate_pass": True,
            "performance_gate_pass": True,
            "omni_atmosphere_active": True,
            "nan_count": 1, "inf_count": 0, "stalls": 0,
            "simulation_steps": 1000, "heartbeat_count": 121,
            "heartbeat_progress_pass": True,
            "heartbeat_timing_pass": True,
            "finite_state_pass": True,
            "atmosphere_range_pass": True,
            "atmosphere_mass_closure_pass": True,
            "heartbeat_summary_match": True,
            "long_run_diagnostics_pass": True,
        }
        errors = release_validation_audit.validate_raw_semantics(
            "Soak2Hours", raw, root=Path.cwd(), candidate_sha256="A" * 64
        )
        self.assertTrue(any("non-finite" in error for error in errors))

    def test_soak_pass_is_bound_to_candidate_and_source_commit(self) -> None:
        raw = {
            "wall_clock_seconds": 7200.0,
            "long_run_gate_pass": True, "performance_gate_pass": True,
            "omni_atmosphere_active": True,
            "nan_count": 0, "inf_count": 0, "stalls": 0,
            "simulation_steps": 1000, "heartbeat_count": 121,
            "heartbeat_progress_pass": True, "heartbeat_timing_pass": True,
            "finite_state_pass": True, "atmosphere_range_pass": True,
            "atmosphere_mass_closure_pass": True,
            "heartbeat_summary_match": True, "long_run_diagnostics_pass": True,
            "public_zip_sha256": "B" * 64, "source_commit": "b" * 40,
        }
        errors = release_validation_audit.validate_raw_semantics(
            "Soak2Hours", raw, root=Path.cwd(), candidate_sha256="A" * 64,
            commit="a" * 40,
        )
        self.assertTrue(any("public ZIP hash" in error for error in errors))
        self.assertTrue(any("source commit" in error for error in errors))

    def test_clean_machine_runner_rejects_dev_checkout_even_with_marker(self) -> None:
        powershell = shutil.which("powershell.exe") or shutil.which("pwsh")
        if not powershell:
            self.skipTest("PowerShell is unavailable")
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            package = root / "empty.zip"
            with zipfile.ZipFile(package, "w"):
                pass
            digest = hashlib.sha256(package.read_bytes()).hexdigest()
            script = ROOT / "tools/test_clean_release.ps1"
            validator_digest = hashlib.sha256(script.read_bytes()).hexdigest()
            output_without = root / "without-marker.json"
            env_without = os.environ.copy()
            env_without.pop("OMNI_CLEAN_MACHINE", None)
            without = subprocess.run([
                powershell, "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", str(script),
                "-PackageZip", str(package), "-OutputJson", str(output_without),
                "-RunId", "20260814T041500Z-8f31c1c7", "-GateName", "WindowsCleanMachine",
                "-ExpectedArtifactSha256", digest,
                "-ExpectedValidatorSha256", validator_digest,
            ], check=False, env=env_without)
            self.assertNotEqual(without.returncode, 0)
            without_result = json.loads(output_without.read_text(encoding="utf-8-sig"))
            self.assertIn("detected a source checkout", without_result["reason"])
            self.assertTrue(without_result["source_checkout_used"])

            output_with = root / "with-marker.json"
            env_with = os.environ.copy()
            env_with["OMNI_CLEAN_MACHINE"] = "true"
            with_marker = subprocess.run([
                powershell, "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", str(script),
                "-PackageZip", str(package), "-OutputJson", str(output_with),
                "-RunId", "20260814T041500Z-8f31c1c7", "-GateName", "WindowsCleanMachine",
                "-ExpectedArtifactSha256", digest,
                "-ExpectedValidatorSha256", validator_digest,
            ], check=False, env=env_with)
            self.assertNotEqual(with_marker.returncode, 0)
            with_result = json.loads(output_with.read_text(encoding="utf-8-sig"))
            self.assertIn("detected a source checkout", with_result["reason"])
            self.assertTrue(with_result["source_checkout_used"])
            self.assertFalse(with_result["passed"])

    def test_clean_machine_runner_rejects_wrong_validator_sha(self) -> None:
        powershell = shutil.which("powershell.exe") or shutil.which("pwsh")
        if not powershell:
            self.skipTest("PowerShell is unavailable")
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            package = root / "empty.zip"
            with zipfile.ZipFile(package, "w"):
                pass
            output = root / "wrong-validator.json"
            script = ROOT / "tools/test_clean_release.ps1"
            completed = subprocess.run([
                powershell, "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", str(script),
                "-PackageZip", str(package), "-OutputJson", str(output),
                "-RunId", "20260814T041500Z-8f31c1c7", "-GateName", "WindowsCleanMachine",
                "-ExpectedArtifactSha256", hashlib.sha256(package.read_bytes()).hexdigest(),
                "-ExpectedValidatorSha256", "0" * 64,
            ], check=False)
            self.assertNotEqual(completed.returncode, 0)
            result = json.loads(output.read_text(encoding="utf-8-sig"))
            self.assertFalse(result["passed"])
            self.assertFalse(result["validator_binding_checked"])
            self.assertEqual(result["runtime_validator_sha256"], hashlib.sha256(script.read_bytes()).hexdigest().upper())
            self.assertIn("does not match expected validator", result["reason"])

    def test_release_scripts_do_not_depend_on_optional_get_file_hash_cmdlet(self) -> None:
        for name in ("release_1_1_0.ps1", "test_clean_release.ps1", "runtime_stress_test.ps1"):
            source = (ROOT / "tools" / name).read_text(encoding="utf-8")
            self.assertIn("function Get-Sha256Hex", source)
            self.assertNotIn("Get-FileHash", source)

    def test_release_processes_strip_credential_shaped_environment_variables(self) -> None:
        source = (ROOT / "tools" / "release_1_1_0.ps1").read_text(encoding="utf-8")
        self.assertIn("@($psi.Environment.Keys)", source)
        self.assertIn("TOKEN|SECRET|PASSWORD|PASSWD|API[_-]?KEY|PAT", source)
        self.assertIn("$psi.Environment.Remove($environmentName)", source)

    def test_evidence_tampering_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            source = root / "source.json"
            source.write_text('{"passed":true,"status":"PASS"}\n', encoding="utf-8")
            evidence = root / "gate.json"
            evidence.write_text(json.dumps({
                "schema": "omnipack-release-evidence",
                "schema_version": 1,
                "test": "build",
                "gate_name": "Build",
                "run_id": "20260814T041500Z-8f31c1c7",
                "commit": "a" * 40,
                "status": "PASS",
                "passed": True,
                "gate_started_at": "2000-01-01T00:00:00+00:00",
                "gate_finished_at": "2000-01-01T00:00:01+00:00",
                "source_evidence": "source.json",
                "source_evidence_sha256": hashlib.sha256(source.read_bytes()).hexdigest().upper(),
            }) + "\n", encoding="utf-8")
            digest = hashlib.sha256(evidence.read_bytes()).hexdigest().upper()
            validation = root / "RELEASE-VALIDATION.json"
            validation.write_text(json.dumps({
                "run_id": "20260814T041500Z-8f31c1c7",
                "commit": "a" * 40,
                "gates": {"Build": {"Status": "PASS", "Evidence": "gate.json", "EvidenceSha256": digest}}
            }), encoding="utf-8")
            evidence.write_text('{"passed":false}\n', encoding="utf-8")
            output = root / "audit.json"
            completed = subprocess.run([
                sys.executable, str(ROOT / "tools/release_validation_audit.py"),
                "--validation-json", str(validation), "--channel", "rc",
                "--output", str(output),
            ], check=False)
            self.assertNotEqual(completed.returncode, 0)
            result = json.loads(output.read_text(encoding="utf-8"))
            self.assertFalse(result["evidence_hash_integrity"])

    def test_semantic_attack_pass_gate_with_not_tested_evidence_is_rejected(self) -> None:
        for evidence_status, evidence_passed in (("NOT_TESTED", False), ("FAIL", False)):
            with self.subTest(evidence_status=evidence_status), tempfile.TemporaryDirectory() as temporary:
                root = Path(temporary)
                run_id = "20260814T041500Z-8f31c1c7"
                commit = "b" * 40
                source = root / "official-source.json"
                source.write_text(json.dumps({
                    "test": "official_tpt_save_compatibility",
                    "status": evidence_status,
                    "passed": evidence_passed,
                    "run_id": run_id,
                }) + "\n", encoding="utf-8")
                evidence = root / "gate-official.json"
                evidence.write_text(json.dumps({
                    "schema": "omnipack-release-evidence",
                    "schema_version": 1,
                    "test": "official_tpt_save_compatibility",
                    "gate_name": "OfficialTPTSaveCompatibility",
                    "run_id": run_id,
                    "commit": commit,
                    "status": evidence_status,
                    "passed": evidence_passed,
                    "gate_started_at": "2000-01-01T00:00:00+00:00",
                    "gate_finished_at": "2000-01-01T00:00:01+00:00",
                    "source_evidence": source.name,
                    "source_evidence_sha256": hashlib.sha256(source.read_bytes()).hexdigest().upper(),
                }) + "\n", encoding="utf-8")
                validation = root / "RELEASE-VALIDATION.json"
                validation.write_text(json.dumps({
                    "run_id": run_id,
                    "commit": commit,
                    "gates": {
                        "OfficialTPTSaveCompatibility": {
                            "Status": "PASS",
                            "Evidence": evidence.name,
                            "EvidenceSha256": hashlib.sha256(evidence.read_bytes()).hexdigest().upper(),
                        }
                    },
                }), encoding="utf-8")
                output = root / "audit.json"
                completed = subprocess.run([
                    sys.executable, str(ROOT / "tools/release_validation_audit.py"),
                    "--validation-json", str(validation), "--channel", "stable", "--output", str(output),
                ], check=False)
                self.assertNotEqual(completed.returncode, 0)
                result = json.loads(output.read_text(encoding="utf-8"))
                self.assertTrue(result["evidence_hash_integrity"])
                self.assertFalse(result["evidence_semantic_integrity"])

    def test_semantic_attack_raw_json_without_identity_is_rejected_after_rehash(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            run_id = "20260814T041500Z-8f31c1c7"
            commit = "b" * 40
            source = root / "raw.json"
            source.write_text(json.dumps({"status": "PASS", "passed": True}) + "\n", encoding="utf-8")
            envelope = root / "gate.json"
            envelope.write_text(json.dumps({
                "schema": "omnipack-release-evidence", "schema_version": 1,
                "test": "build", "gate_name": "Build", "run_id": run_id,
                "commit": commit, "status": "PASS", "passed": True,
                "gate_started_at": "2000-01-01T00:00:00+00:00",
                "gate_finished_at": "2000-01-01T00:00:01+00:00",
                "source_evidence": source.name,
                "source_evidence_sha256": hashlib.sha256(source.read_bytes()).hexdigest().upper(),
            }) + "\n", encoding="utf-8")
            validation = root / "RELEASE-VALIDATION.json"
            validation.write_text(json.dumps({
                "run_id": run_id, "commit": commit,
                "gates": {"Build": {"Status": "PASS", "Evidence": envelope.name,
                                     "EvidenceSha256": hashlib.sha256(envelope.read_bytes()).hexdigest().upper()}},
            }), encoding="utf-8")
            output = root / "audit.json"
            completed = subprocess.run([
                sys.executable, str(ROOT / "tools/release_validation_audit.py"),
                "--validation-json", str(validation), "--channel", "rc", "--output", str(output),
            ], check=False)
            self.assertNotEqual(completed.returncode, 0)
            result = json.loads(output.read_text(encoding="utf-8"))
            self.assertTrue(result["evidence_hash_integrity"])
            self.assertFalse(result["evidence_semantic_integrity"])

    def test_source_snapshot_semantic_change_is_rejected(self) -> None:
        raw = {
            "schema": "omnipack-release-evidence", "schema_version": 1,
            "test": "source_snapshot_immutability", "gate_name": "SourceSnapshotImmutability",
            "run_id": "20260814T041500Z-8f31c1c7", "commit": "a" * 40,
            "status": "PASS", "passed": True,
            "content_hash_start": "A" * 64, "content_hash_after_configure": "A" * 64,
            "content_hash_after_build": "B" * 64,
            "content_hash_before_package": "A" * 64, "content_hash_after_package": "A" * 64,
            "content_hash_end": "A" * 64,
            "tracked_files_start": 10, "tracked_files_after_configure": 10,
            "tracked_files_after_build": 10,
            "tracked_files_before_package": 10, "tracked_files_after_package": 10,
            "tracked_files_end": 10,
        }
        errors = release_validation_audit.validate_raw_semantics(
            "SourceSnapshotImmutability", raw, root=Path.cwd(), candidate_sha256=None
        )
        self.assertTrue(any("content snapshot changed" in error for error in errors))

    def test_source_snapshot_must_equal_aggregate_identity(self) -> None:
        raw = {
            "commit_start": "b" * 40, "commit_end": "b" * 40,
            "branch_start": "main", "branch_end": "main",
            "git_status_start": "", "git_status_end": "",
            "content_hash_start": "C" * 64,
            "content_hash_after_build": "C" * 64,
            "content_hash_before_package": "C" * 64,
            "content_hash_after_package": "C" * 64,
            "content_hash_end": "C" * 64,
            "tracked_files_start": 10, "tracked_files_after_build": 10,
            "tracked_files_before_package": 10, "tracked_files_after_package": 10,
            "tracked_files_end": 10,
        }
        errors = release_validation_audit.validate_raw_semantics(
            "SourceSnapshotImmutability", raw, root=Path.cwd(), candidate_sha256=None,
            commit="a" * 40, source_snapshot_sha256="F" * 64,
        )
        self.assertTrue(any("aggregate commit" in error for error in errors))
        self.assertTrue(any("aggregate source_snapshot_sha256" in error for error in errors))

    def test_source_tree_clean_requires_explicit_empty_porcelain(self) -> None:
        self.assertTrue(release_validation_audit.validate_raw_semantics(
            "SourceTreeClean", {}, root=Path.cwd(), candidate_sha256=None,
            source_snapshot_sha256="A" * 64,
        ))
        self.assertEqual(release_validation_audit.validate_raw_semantics(
            "SourceTreeClean", {
                "porcelain_output": "", "content_hash": "A" * 64,
                "tracked_files": 1,
            }, root=Path.cwd(), candidate_sha256=None,
            source_snapshot_sha256="A" * 64,
        ), [])

    def test_source_tree_clean_rejects_wrong_snapshot_binding(self) -> None:
        errors = release_validation_audit.validate_raw_semantics(
            "SourceTreeClean", {
                "porcelain_output": "", "content_hash": "B" * 64,
                "tracked_files": 1,
            }, root=Path.cwd(), candidate_sha256=None,
            source_snapshot_sha256="A" * 64,
        )
        self.assertTrue(any("aggregate source snapshot" in error for error in errors))

    def test_source_snapshot_immutability_rejects_equal_dirty_checkpoints(self) -> None:
        raw = {
            "commit_start": "a" * 40, "commit_end": "a" * 40,
            "branch_start": "main", "branch_end": "main",
            "git_status_start": " M tracked.cpp", "git_status_end": " M tracked.cpp",
        }
        for phase in (
            "start", "after_configure", "after_build", "before_package",
            "after_package", "end",
        ):
            raw[f"content_hash_{phase}"] = "A" * 64
            raw[f"tracked_files_{phase}"] = 1
        for phase in ("after_configure", "after_build", "before_package", "after_package", "end"):
            raw[f"build_inputs_hash_{phase}"] = "B" * 64
            raw[f"build_inputs_ready_{phase}"] = True
        errors = release_validation_audit.validate_raw_semantics(
            "SourceSnapshotImmutability", raw, root=Path.cwd(),
            candidate_sha256=None, commit="a" * 40,
            source_snapshot_sha256="A" * 64,
            build_inputs_sha256="B" * 64,
        )
        self.assertTrue(any("must both be clean" in error for error in errors))


if __name__ == "__main__":
    unittest.main()
