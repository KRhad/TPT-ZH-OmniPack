from __future__ import annotations

import copy
import importlib.util
import json
from pathlib import Path
import sys
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[2]
TOOL_PATH = ROOT / "tools" / "omnicore_data_check.py"
SPEC = importlib.util.spec_from_file_location("omnicore_data_check", TOOL_PATH)
assert SPEC is not None and SPEC.loader is not None
tool = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(tool)

PACKAGE_TOOL_PATH = ROOT / "tools" / "package_source_release.py"
PACKAGE_SPEC = importlib.util.spec_from_file_location(
    "package_source_release_for_omnicore_test", PACKAGE_TOOL_PATH
)
assert PACKAGE_SPEC is not None and PACKAGE_SPEC.loader is not None
package_tool = importlib.util.module_from_spec(PACKAGE_SPEC)
sys.modules[PACKAGE_SPEC.name] = package_tool
PACKAGE_SPEC.loader.exec_module(package_tool)

DATA_ROOT = ROOT / "resources" / "omnicore" / "v1"


def provenance() -> dict[str, object]:
    return {
        "source_title": "Synthetic contract fixture",
        "source_version": "fixture-v1",
        "source_date": "2026-08-10",
        "accessed_on": "2026-08-10",
        "source_locator": "test://omnicore-data-contract",
        "source_kind": "test_fixture",
        "method": "test_fixture",
        "confidence": "test_only",
        "redistribution_status": "project_generated",
        "license_or_terms": "project test fixture",
        "license_evidence": "test://project-generated",
        "license_sha256": None,
        "tuning_status": "test_fixture",
    }


def base_catalog() -> dict[str, object]:
    return {
        "schema_version": 1,
        "document_type": "omnicore_catalog",
        "catalog_id": "test.fixture",
        "dataset_version": "1.0.4-foundation.1",
        "unit_registry_id": "omnicore.units.canonical-si.v1",
        "catalog_status": "schema_only",
        "runtime_consumption": False,
        "materials": [],
        "species": [],
        "reactions": [],
    }


def species(
    record_id: str,
    composition: dict[str, int],
    charge: int = 0,
) -> dict[str, object]:
    return {
        "id": record_id,
        "names": {"en": record_id},
        "status": "identity_only",
        "chemistry_enabled": True,
        "elemental_composition": composition,
        "charge_number": charge,
        "phases": ["gas"],
        "legacy_mapping": None,
        "properties": [],
        "provenance": provenance(),
    }


def water_material() -> dict[str, object]:
    return {
        "id": "material.legacy_water",
        "names": {"en": "Legacy water mapping"},
        "status": "identity_only",
        "behavior_class": "LEGACY_ONLY",
        "phases": ["liquid"],
        "legacy_mapping": {
            "element_identifier": "DEFAULT_PT_WATR",
            "stable_id": 2,
            "relationship": "identity",
        },
        "composition": [],
        "properties": [],
        "provenance": provenance(),
    }


def physical_property() -> dict[str, object]:
    return {
        "property_id": "density",
        "quantity_kind": "mass_density",
        "value": "1",
        "unit": "kilogram_per_cubic_metre",
        "validity": {
            "temperature": {"minimum": "1", "maximum": "1000", "unit": "kelvin"},
            "pressure": {"minimum": "0", "maximum": "1000000", "unit": "pascal"},
        },
        "provenance": provenance(),
    }


def balanced_reaction() -> dict[str, object]:
    return {
        "id": "reaction.synthetic_water",
        "status": "identity_only",
        "reactants": [
            {"species_id": "species.h2", "coefficient": "2", "phase": "gas"},
            {"species_id": "species.o2", "coefficient": "1", "phase": "gas"},
        ],
        "products": [
            {"species_id": "species.h2o", "coefficient": "2", "phase": "gas"}
        ],
        "reversible": False,
        "rate_model": {
            "kind": "unselected",
            "rationale": "Synthetic balance fixture; no runtime kinetics",
        },
        "energy_change": None,
        "provenance": provenance(),
    }


def chemistry_catalog() -> dict[str, object]:
    document = base_catalog()
    document["species"] = [
        species("species.h2", {"H": 2}),
        species("species.o2", {"O": 2}),
        species("species.h2o", {"H": 2, "O": 1}),
    ]
    document["reactions"] = [balanced_reaction()]
    return document


def audit_document(document: dict[str, object]) -> list[str]:
    with tempfile.TemporaryDirectory() as temporary:
        path = Path(temporary) / "catalog.json"
        path.write_text(
            json.dumps(document, ensure_ascii=False, indent=2) + "\n",
            encoding="utf-8",
            newline="\n",
        )
        errors, _ = tool.audit(ROOT, catalog_path=path)
    return errors


def audit_raw(payload: bytes) -> list[str]:
    with tempfile.TemporaryDirectory() as temporary:
        path = Path(temporary) / "catalog.json"
        path.write_bytes(payload)
        errors, _ = tool.audit(ROOT, catalog_path=path)
    return errors


class OmniCoreDataCheckTests(unittest.TestCase):
    def test_repository_foundation_passes_and_maps_every_implemented_slot(self) -> None:
        errors, stats = tool.audit(ROOT)
        self.assertEqual(errors, [])
        self.assertEqual(stats, {
            "materials": 0,
            "species": 0,
            "reactions": 0,
            "legacy_mappings": 488,
        })

    def test_schema_declares_required_definition_families(self) -> None:
        schema = tool.load_json_strict(DATA_ROOT / "schema.json")
        self.assertTrue({
            "MaterialDefinition",
            "SpeciesDefinition",
            "ReactionDefinition",
            "PropertyDefinition",
            "Provenance",
            "LegacyMaterialMap",
        }.issubset(schema["$defs"]))

    def test_nested_schema_constraint_drift_is_rejected_by_semantic_hash(self) -> None:
        original = tool.load_json_strict(DATA_ROOT / "schema.json")
        unit_schema = tool.load_json_strict(DATA_ROOT / "unit-registry.schema.json")
        mutations: list[tuple[dict[str, object], dict[str, object]]] = []
        relaxed_id = copy.deepcopy(original)
        relaxed_id["$defs"]["MaterialDefinition"]["properties"]["id"]["pattern"] = ".*"
        mutations.append((relaxed_id, unit_schema))
        no_rate_model = copy.deepcopy(original)
        del no_rate_model["$defs"]["ReactionRateModel"]
        mutations.append((no_rate_model, unit_schema))
        relaxed_dimension = copy.deepcopy(unit_schema)
        relaxed_dimension["$defs"]["DimensionVector"]["properties"]["mass"]["type"] = "string"
        mutations.append((original, relaxed_dimension))
        for mutated_schema, mutated_unit_schema in mutations:
            with self.subTest():
                errors: list[str] = []
                tool._validate_schema(mutated_schema, mutated_unit_schema, errors)
                self.assertTrue(any("complete semantic contract hash drifted" in error for error in errors))

    def test_legacy_map_is_deterministic_sorted_and_identity_only(self) -> None:
        first = tool.build_legacy_map(ROOT)
        second = tool.build_legacy_map(ROOT)
        self.assertEqual(first, second)
        mappings = first["mappings"]
        self.assertEqual(
            [item["stable_id"] for item in mappings],
            sorted(item["stable_id"] for item in mappings),
        )
        self.assertTrue(all(item["mapping_status"] == "identity_only" for item in mappings))
        self.assertFalse(first["physical_properties_imported"])

    def test_legacy_source_hash_is_lf_crlf_independent(self) -> None:
        source = (ROOT / "docs" / "ELEMENT_REGISTRY.csv").read_text(
            encoding="utf-8-sig"
        )
        normalized = source.replace("\r\n", "\n").replace("\r", "\n")
        with tempfile.TemporaryDirectory() as temporary:
            base = Path(temporary)
            left = base / "lf"
            right = base / "crlf"
            (left / "docs").mkdir(parents=True)
            (right / "docs").mkdir(parents=True)
            (left / "docs" / "ELEMENT_REGISTRY.csv").write_bytes(
                normalized.encode("utf-8")
            )
            (right / "docs" / "ELEMENT_REGISTRY.csv").write_bytes(
                normalized.replace("\n", "\r\n").encode("utf-8")
            )
            left_map = tool.build_legacy_map(left)
            right_map = tool.build_legacy_map(right)
        self.assertEqual(left_map, right_map)
        self.assertEqual(left_map["source_hash_contract"], "canonical_csv_utf8_lf_v1")

    def test_legacy_source_hash_normalizes_quoted_field_newlines(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            base = Path(temporary)
            lf = base / "lf.csv"
            crlf = base / "crlf.csv"
            lf.write_bytes(b'id,note\n1,"line one\nline two"\n')
            crlf.write_bytes(b'id,note\r\n1,"line one\r\nline two"\r\n')
            left_header, left_rows = tool.read_csv_strict(lf)
            right_header, right_rows = tool.read_csv_strict(crlf)
        self.assertEqual(
            tool.canonical_csv_sha256(left_header, left_rows),
            tool.canonical_csv_sha256(right_header, right_rows),
        )

    def test_legacy_source_hash_normalizes_quoted_header_newlines(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            base = Path(temporary)
            lf = base / "lf.csv"
            crlf = base / "crlf.csv"
            lf.write_bytes(b'"id\nname",note\n1,value\n')
            crlf.write_bytes(b'"id\r\nname",note\r\n1,value\r\n')
            left_header, left_rows = tool.read_csv_strict(lf)
            right_header, right_rows = tool.read_csv_strict(crlf)
        self.assertEqual(
            tool.canonical_csv_sha256(left_header, left_rows),
            tool.canonical_csv_sha256(right_header, right_rows),
        )

    def test_compound_registry_has_exact_width_and_quoted_comma_formulas(self) -> None:
        header, rows = tool.read_csv_strict(ROOT / "docs" / "COMPOUND_REGISTRY.csv")
        self.assertEqual(len(header), 13)
        by_id = {row["compound_id"]: row for row in rows}
        self.assertEqual(by_id["compound.ferrite"]["formula"], "(Fe,Zn)3O4")
        self.assertEqual(
            by_id["compound.piezoelectric_ceramic"]["formula"], "Pb(Zr,Ti)O3"
        )

    def test_malformed_csv_width_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "bad.csv"
            path.write_text("a,b\n1,2,3\n", encoding="utf-8")
            with self.assertRaisesRegex(tool.ValidationFailure, "expected 2 fields, got 3"):
                tool.read_csv_strict(path)

    def test_duplicate_json_key_is_rejected(self) -> None:
        payload = (
            b'{"schema_version":1,"schema_version":1,"document_type":"omnicore_catalog"}'
        )
        errors = audit_raw(payload)
        self.assertTrue(any("duplicate JSON key" in error for error in errors))

    def test_nan_and_infinity_are_rejected(self) -> None:
        for constant in (b"NaN", b"Infinity", b"-Infinity"):
            with self.subTest(constant=constant):
                errors = audit_raw(b'{"schema_version":' + constant + b"}")
                self.assertTrue(any("non-standard/non-finite" in error for error in errors))

    def test_bom_and_invalid_utf8_are_rejected(self) -> None:
        for payload, expected in (
            (b"\xef\xbb\xbf{}", "BOM is forbidden"),
            (b'{"x":"\xff"}', "invalid UTF-8"),
        ):
            with self.subTest(expected=expected):
                self.assertTrue(any(expected in error for error in audit_raw(payload)))

    def test_unknown_and_missing_catalog_fields_are_rejected(self) -> None:
        document = base_catalog()
        document["surprise"] = True
        del document["dataset_version"]
        errors = audit_document(document)
        self.assertTrue(any("missing fields: dataset_version" in error for error in errors))
        self.assertTrue(any("unknown fields: surprise" in error for error in errors))

    def test_unsupported_version_and_runtime_consumer_are_rejected(self) -> None:
        document = base_catalog()
        document["schema_version"] = 2
        document["runtime_consumption"] = True
        errors = audit_document(document)
        self.assertTrue(any("only 1 is supported" in error for error in errors))
        self.assertTrue(any("runtime_consumption" in error for error in errors))

    def test_unit_registry_is_validator_owned(self) -> None:
        units = tool.load_json_strict(DATA_ROOT / "units.json")
        units["units"][0]["symbol"] = "anything"
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "units.json"
            path.write_text(json.dumps(units) + "\n", encoding="utf-8")
            errors, _ = tool.audit(ROOT, units_path=path)
        self.assertTrue(any("validator-owned SI v1 contract" in error for error in errors))

    def test_legacy_identifier_and_stable_id_must_match_together(self) -> None:
        document = base_catalog()
        material = water_material()
        material["legacy_mapping"]["stable_id"] = 3
        document["materials"] = [material]
        errors = audit_document(document)
        self.assertTrue(any("stable ID mismatch for DEFAULT_PT_WATR" in error for error in errors))

    def test_identity_only_legacy_mapping_cannot_be_proxy(self) -> None:
        document = base_catalog()
        material = water_material()
        material["legacy_mapping"]["relationship"] = "proxy"
        document["materials"] = [material]
        errors = audit_document(document)
        self.assertTrue(any("identity_only material requires relationship=identity" in error for error in errors))

    def test_identity_only_species_mapping_cannot_be_proxy(self) -> None:
        document = base_catalog()
        record = species("species.legacy_water_vapour", {"H": 2, "O": 1})
        record["legacy_mapping"] = {
            "element_identifier": "DEFAULT_PT_WTRV",
            "stable_id": 20,
            "relationship": "proxy",
        }
        document["species"] = [record]
        errors = audit_document(document)
        self.assertTrue(any("identity_only species requires relationship=identity" in error for error in errors))

    def test_duplicate_record_ids_are_rejected_case_insensitively(self) -> None:
        document = base_catalog()
        first = water_material()
        second = copy.deepcopy(first)
        second["legacy_mapping"] = None
        document["materials"] = [first, second]
        errors = audit_document(document)
        self.assertTrue(any("duplicate ID material.legacy_water" in error for error in errors))

    def test_zero_property_value_is_present_but_missing_value_is_rejected(self) -> None:
        zero = physical_property()
        zero["value"] = "0"
        errors: list[str] = []
        tool._validate_property(zero, "fixture", ROOT, "test.fixture", errors)
        self.assertFalse(any("missing fields: value" in error for error in errors))
        missing = physical_property()
        del missing["value"]
        errors = []
        tool._validate_property(missing, "fixture", ROOT, "test.fixture", errors)
        self.assertTrue(any("missing fields: value" in error for error in errors))

    def test_property_requires_canonical_unit_ranges_and_complete_provenance(self) -> None:
        value = physical_property()
        value["unit"] = "g/cm3"
        value["validity"]["temperature"]["minimum"] = "400"
        value["validity"]["temperature"]["maximum"] = "300"
        del value["provenance"]["source_version"]
        errors: list[str] = []
        tool._validate_property(value, "fixture", ROOT, "test.fixture", errors)
        self.assertTrue(any("not canonical" in error for error in errors))
        self.assertTrue(any("minimum exceeds maximum" in error for error in errors))
        self.assertTrue(any("missing fields: source_version" in error for error in errors))

    def test_negative_absolute_validity_range_is_rejected(self) -> None:
        value = physical_property()
        value["validity"]["pressure"]["minimum"] = "-1"
        errors: list[str] = []
        tool._validate_property(value, "fixture", ROOT, "test.fixture", errors)
        self.assertTrue(any("absolute range cannot be negative" in error for error in errors))

    def test_negative_zero_property_value_is_rejected(self) -> None:
        value = physical_property()
        value["value"] = "-0"
        errors: list[str] = []
        tool._validate_property(value, "fixture", ROOT, "test.fixture", errors)
        self.assertTrue(any("negative zero is not canonical" in error for error in errors))

    def test_reference_only_numeric_value_is_rejected(self) -> None:
        value = physical_property()
        value["provenance"]["redistribution_status"] = "reference_only"
        errors: list[str] = []
        tool._validate_property(value, "fixture", ROOT, "test.fixture", errors)
        self.assertTrue(any("cannot use redistribution_status=reference_only" in error for error in errors))

    def test_test_fixture_provenance_is_rejected_from_repository_catalog(self) -> None:
        errors: list[str] = []
        tool._validate_property(
            physical_property(), "fixture", ROOT, "omnicore.foundation.v1", errors
        )
        self.assertTrue(any("forbidden outside test.* catalogs" in error for error in errors))

    def test_ai_and_placeholder_provenance_are_rejected(self) -> None:
        value = physical_property()
        value["provenance"]["source_title"] = "AI-generated property"
        value["provenance"]["source_version"] = "unknown"
        errors: list[str] = []
        tool._validate_property(value, "fixture", ROOT, "test.fixture", errors)
        self.assertTrue(any("AI-generated data is forbidden" in error for error in errors))
        self.assertTrue(any("placeholder provenance is forbidden" in error for error in errors))

    def test_unsafe_license_evidence_path_is_rejected(self) -> None:
        value = physical_property()
        value["provenance"]["license_evidence"] = "../secret.txt"
        errors: list[str] = []
        tool._validate_property(value, "fixture", ROOT, "test.fixture", errors)
        self.assertTrue(any("unsafe repository path" in error for error in errors))

    def test_chemistry_enabled_species_requires_valid_elemental_composition(self) -> None:
        document = base_catalog()
        missing = species("species.missing", {})
        unknown = species("species.unknown", {"NotAnElement": 1})
        document["species"] = [missing, unknown]
        errors = audit_document(document)
        self.assertTrue(any("requires elemental composition" in error for error in errors))
        self.assertTrue(any("unknown atomic symbol 'NotAnElement'" in error for error in errors))

    def test_material_composition_requires_known_references_and_exact_sum(self) -> None:
        document = base_catalog()
        material = water_material()
        material["composition"] = [{
            "component_kind": "species",
            "component_id": "species.missing",
            "basis": "mass_fraction",
            "fraction": "1/2",
        }]
        document["materials"] = [material]
        errors = audit_document(document)
        self.assertTrue(any("fractions must sum exactly to 1" in error for error in errors))
        self.assertTrue(any("unknown species 'species.missing'" in error for error in errors))

    def test_exact_rational_atom_and_charge_balanced_reaction_passes(self) -> None:
        self.assertEqual(audit_document(chemistry_catalog()), [])

    def test_reaction_phase_must_be_declared_by_species(self) -> None:
        document = chemistry_catalog()
        document["reactions"][0]["products"][0]["phase"] = "liquid"
        errors = audit_document(document)
        self.assertTrue(any("is not declared by species species.h2o" in error for error in errors))

    def test_identity_reaction_cannot_carry_kinetics_or_energy(self) -> None:
        document = chemistry_catalog()
        reaction = document["reactions"][0]
        reaction["rate_model"] = {"kind": "arrhenius"}
        reaction["energy_change"] = physical_property()
        errors = audit_document(document)
        self.assertTrue(any("must keep rate_model.kind=unselected" in error for error in errors))
        self.assertTrue(any("identity_only reaction must use null" in error for error in errors))

    def test_reference_only_reaction_definition_is_rejected(self) -> None:
        document = chemistry_catalog()
        document["reactions"][0]["provenance"]["redistribution_status"] = "reference_only"
        errors = audit_document(document)
        self.assertTrue(any("cannot use redistribution_status=reference_only" in error for error in errors))

    def test_atom_imbalance_is_rejected(self) -> None:
        document = chemistry_catalog()
        document["reactions"][0]["products"][0]["coefficient"] = "1"
        errors = audit_document(document)
        self.assertTrue(any("atom imbalance" in error for error in errors))

    def test_charge_imbalance_is_rejected(self) -> None:
        document = chemistry_catalog()
        document["species"][2]["charge_number"] = 1
        errors = audit_document(document)
        self.assertTrue(any("charge imbalance" in error for error in errors))

    def test_unknown_duplicate_and_zero_reaction_participants_are_rejected(self) -> None:
        document = chemistry_catalog()
        reaction = document["reactions"][0]
        reaction["reactants"].append({
            "species_id": "species.h2",
            "coefficient": "1",
            "phase": "gas",
        })
        reaction["reactants"].append({
            "species_id": "species.o2",
            "coefficient": "0",
            "phase": "gas",
        })
        reaction["products"][0]["species_id"] = "species.missing"
        errors = audit_document(document)
        self.assertTrue(any("expected positive canonical rational" in error for error in errors))
        self.assertTrue(any("duplicate reaction participant" in error for error in errors))
        self.assertTrue(any("unknown species species.missing" in error for error in errors))

    def test_unreduced_rational_is_rejected(self) -> None:
        document = chemistry_catalog()
        document["reactions"][0]["reactants"][0]["coefficient"] = "4/2"
        errors = audit_document(document)
        self.assertTrue(any("reduced canonical form 2" in error for error in errors))

    def test_legacy_csv_integer_requires_canonical_spelling(self) -> None:
        for value in ("00", "+1", " 1", "01"):
            with self.subTest(value=value):
                with self.assertRaisesRegex(
                    tool.ValidationFailure, "canonical nonnegative integer"
                ):
                    tool._canonical_nonnegative_integer(value, "fixture")

    def test_failed_refresh_does_not_create_or_overwrite_output(self) -> None:
        invalid = base_catalog()
        invalid["runtime_consumption"] = True
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            catalog_path = directory / "bad-catalog.json"
            catalog_path.write_text(json.dumps(invalid) + "\n", encoding="utf-8")
            existing = directory / "existing.json"
            existing.write_bytes(b"sentinel")
            missing = directory / "missing.json"
            errors = tool.validated_refresh_legacy_map(
                ROOT, existing, catalog_path=catalog_path
            )
            self.assertTrue(errors)
            self.assertEqual(existing.read_bytes(), b"sentinel")
            errors = tool.validated_refresh_legacy_map(
                ROOT, missing, catalog_path=catalog_path
            )
            self.assertTrue(errors)
            self.assertFalse(missing.exists())

    def test_legacy_map_modification_is_rejected(self) -> None:
        document = tool.load_json_strict(DATA_ROOT / "legacy-material-map.json")
        document["mappings"][0]["stable_id"] = 999999
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "legacy-material-map.json"
            path.write_text(json.dumps(document) + "\n", encoding="utf-8")
            errors, _ = tool.audit(ROOT, legacy_map_path=path)
        self.assertTrue(any("identity map is stale or was modified" in error for error in errors))

    def test_production_sources_do_not_consume_foundation_files(self) -> None:
        needles = ("resources/omnicore", "omnicore_data_check", "catalog.json")
        hits: list[str] = []
        for path in (ROOT / "src").rglob("*"):
            if path.suffix not in {".cpp", ".h"}:
                continue
            text = path.read_text(encoding="utf-8")
            for needle in needles:
                if needle in text:
                    hits.append(f"{path.relative_to(ROOT)}:{needle}")
        self.assertEqual(hits, [])

    def test_public_source_package_keeps_validator_with_contract(self) -> None:
        member = "tools/omnicore_data_check.py"
        self.assertIn(member, package_tool.ALLOWED_TOOLS)
        self.assertIn(member, package_tool.REQUIRED_MEMBERS)
        self.assertTrue({
            "resources/omnicore/v1/README.md",
            "resources/omnicore/v1/catalog.json",
            "resources/omnicore/v1/legacy-material-map.json",
            "resources/omnicore/v1/schema.json",
            "resources/omnicore/v1/unit-registry.schema.json",
            "resources/omnicore/v1/units.json",
        }.issubset(package_tool.REQUIRED_MEMBERS))


if __name__ == "__main__":
    unittest.main()
