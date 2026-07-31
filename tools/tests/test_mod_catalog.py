from __future__ import annotations

import importlib
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[2]
CATALOG_DIR = ROOT / "tools" / "mod_catalog"
sys.path.insert(0, str(CATALOG_DIR))

common = importlib.import_module("catalog_common")
cpp = importlib.import_module("extract_cpp_elements")
lua = importlib.import_module("extract_lua_elements")
duplicates = importlib.import_module("detect_duplicates")
reactions = importlib.import_module("extract_reactions")
scanner = importlib.import_module("scan_repositories")
validator = importlib.import_module("validate_mod_catalog")


class ModCatalogTests(unittest.TestCase):
    def test_balanced_block_ignores_braces_in_strings_and_comments(self) -> None:
        text = '{ value = "}"; /* { */ if (x) { y(); } } tail'
        self.assertEqual(common.balanced_block(text, 0), text[:-5])

    def test_cpp_constructor_extraction(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            repo = Path(temporary)
            path = repo / "src" / "simulation" / "elements" / "TEST.cpp"
            path.parent.mkdir(parents=True)
            path.write_text(
                '''
                Element::Element_TEST() {
                    Identifier = "MOD_PT_TEST";
                    Name = "TEST";
                    MenuSection = SC_SOLIDS;
                    Properties = TYPE_SOLID | PROP_CONDUCTS;
                    HighTemperature = 900.0f;
                    HighTemperatureTransition = PT_LAVA;
                }
                int Element_TEST::update(UPDATE_FUNC_ARGS) {
                    if (parts[i].temp > 500) sim->create_part(-1, x, y, PT_FIRE);
                    return 0;
                }
                ''',
                encoding="utf-8",
            )
            rows = cpp.extract_file("fixture", repo, path, "a" * 40, "verified")
        self.assertEqual(len(rows), 1)
        self.assertEqual(rows[0]["source_identifier"], "MOD_PT_TEST")
        self.assertEqual(rows[0]["state"], "solid")
        self.assertIn("PT_FIRE", rows[0]["dependencies"])
        self.assertEqual(rows[0]["particle_creation"], 1)

    def test_cpp_destructor_is_not_an_element_constructor(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            repo = Path(temporary)
            path = repo / "src" / "simulation" / "elements" / "TEST.cpp"
            path.parent.mkdir(parents=True)
            path.write_text(
                '''
                Element_TEST::~Element_TEST() {
                    Identifier = "MOD_PT_FAKE";
                    Name = "FAKE";
                }
                ''',
                encoding="utf-8",
            )
            rows = cpp.extract_file("fixture", repo, path, "a" * 40, "verified")
        self.assertEqual(rows, [])

    def test_lua_allocation_and_global_scan_risk(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            repo = Path(temporary)
            path = repo / "pack.lua"
            path.write_text(
                '''
                local test = elements.allocate("fixture", "TEST")
                elements.property(test, "Name", "Testium")
                elements.property(test, "Properties", elements.TYPE_SOLID)
                elements.property(test, "Update", update_test)
                tpt.register_step(step)
                for i = 0, NPART do sim.partProperty(i, "type") end
                ''',
                encoding="utf-8",
            )
            rows = lua.extract_file("fixture", repo, path, "b" * 40, "verified")
        self.assertEqual(len(rows), 1)
        self.assertEqual(rows[0]["source_code"], "TEST")
        self.assertTrue(rows[0]["global_scan"])
        self.assertEqual(rows[0]["performance_risk"], "high")

    def test_lua_elem_alias_is_extracted(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            repo = Path(temporary)
            path = repo / "fan.lua"
            path.write_text(
                '''
                local trit = elem.allocate("FANMOD", "TRIT")
                elem.property(trit, "Name", "TRIT")
                elem.property(trit, "Properties", elem.TYPE_GAS)
                elem.element(trit, { Description = "Tritium" })
                ''',
                encoding="utf-8",
            )
            rows = lua.extract_file("fanmod", repo, path, "b" * 40, "unknown")
        self.assertEqual(len(rows), 1)
        self.assertEqual(rows[0]["source_identifier"], "FANMOD_PT_TRIT")
        self.assertEqual(rows[0]["source_name"], "TRIT")

    def test_repository_scan_covers_elem_alias_and_license_scope(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            repo = Path(temporary)
            (repo / "scripts").mkdir()
            (repo / "assets").mkdir()
            (repo / "README.md").write_text("Licensed under GPL-3.0.\n", encoding="utf-8")
            (repo / "scripts" / "fan.lua").write_text(
                'local trit = elem.allocate("FANMOD", "TRIT")\n', encoding="utf-8"
            )
            (repo / "assets" / "icon.png").write_bytes(b"fixture")
            subprocess.run(["git", "init", "-q", str(repo)], check=True)
            subprocess.run(["git", "-C", str(repo), "add", "."], check=True)
            self.assertEqual(scanner.detected_elements(repo), 1)
            scope = scanner.license_scope(repo)
        self.assertEqual(scope["element_source_files_scanned"], 1)
        self.assertEqual(scope["asset_files_scanned"], 1)
        self.assertTrue(scope["readme_license_mentions"])

    def test_raw_lua_source_is_bound_to_sha256(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            script = root / "fixture" / "pack.lua"
            script.parent.mkdir()
            script.write_text('local x = elem.allocate("FIXTURE", "TEST")\n', encoding="utf-8")
            digest = common.sha256(script)
            row = scanner.scan_raw_lua(
                {"mod_id": "fixture", "forum_url": "", "repository_url": "", "port_priority": "1"},
                root,
                {
                    "relative_path": "fixture/pack.lua",
                    "sha256": digest,
                    "download_url": "https://example.invalid/pack.lua",
                    "author": "Fixture",
                    "license": "unknown",
                },
            )
        self.assertIsNotNone(row)
        assert row is not None
        self.assertEqual(row["audit_status"], "downloaded_source")
        self.assertEqual(row["source_commit"], f"sha256:{digest}")
        self.assertEqual(row["element_count_detected"], 1)

    def test_duplicate_classifier_detects_same_code(self) -> None:
        candidate = {
            "source_mod": "fixture",
            "source_identifier": "FIXTURE_PT_WATR",
            "source_name": "Water",
            "source_code": "WATR",
            "state": "liquid",
            "properties": {"Properties": "TYPE_LIQUID"},
        }
        current = [{
            "identifier": "DEFAULT_PT_WATR", "code": "WATR", "name": "Water",
            "state": "liquid", "properties": "TYPE_LIQUID", "module": "official", "stable_id": "2",
        }]
        result = duplicates.classify(candidate, current)
        self.assertIn(result["classification"], {"complete_duplicate", "same_name_different_behavior"})

    def test_probability_markers_cover_cpp_and_lua_forms(self) -> None:
        markers = reactions.probability_markers(
            "rng.chance(1, 250); rand() % 17; math.random(2, 9)"
        )
        self.assertEqual(markers, ["1, 250", "17", "2, 9"])

    def test_repository_generated_catalogs_pass(self) -> None:
        self.assertEqual(validator.validate(ROOT), [])


if __name__ == "__main__":
    unittest.main()
