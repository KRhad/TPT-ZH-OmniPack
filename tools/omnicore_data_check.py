#!/usr/bin/env python3
"""Fail-closed validation for the offline OmniCore data contract v1.

This tool has no production runtime consumer. It validates the 1.0.4 schema,
canonical SI unit vocabulary, identity-only Legacy mapping and any future catalog
records before later phases are allowed to generate runtime data.
"""

from __future__ import annotations

import argparse
import csv
from datetime import date
from decimal import Decimal, InvalidOperation
from fractions import Fraction
import hashlib
import io
import json
import os
from pathlib import Path
import re
import tempfile
from typing import Any, Iterable, Sequence


SCHEMA_VERSION = 1
DATASET_VERSION = "1.0.4-foundation.1"
UNIT_REGISTRY_ID = "omnicore.units.canonical-si.v1"
DATA_SCHEMA_SEMANTIC_SHA256 = "F41112F283DC4491527F27D7FB54AC650F2ED85D62FCBF2FCD11057097BE6F7A"
UNIT_SCHEMA_SEMANTIC_SHA256 = "7D2EA6179C59AFF675D72EB4DC5992D7A9FAA2DB0028D5E9DBCAE654CD68C504"
DATA_ROOT = Path("resources/omnicore/v1")
DIMENSIONS = ("mass", "length", "time", "temperature", "amount")

ID_RE = re.compile(r"^[a-z][a-z0-9_.-]*$")
MATERIAL_ID_RE = re.compile(r"^material\.[a-z][a-z0-9_.-]*$")
SPECIES_ID_RE = re.compile(r"^species\.[a-z][a-z0-9_.-]*$")
REACTION_ID_RE = re.compile(r"^reaction\.[a-z][a-z0-9_.-]*$")
LEGACY_ID_RE = re.compile(r"^[A-Z][A-Z0-9_]*$")
DECIMAL_RE = re.compile(
    r"^-?(0|[1-9][0-9]*)(\.[0-9]+)?([eE][+-]?(0|[1-9][0-9]*))?$"
)
RATIONAL_RE = re.compile(r"^[1-9][0-9]*(/[1-9][0-9]*)?$")
SHA256_RE = re.compile(r"^[A-F0-9]{64}$")

RECORD_STATUSES = {
    "legacy_only", "identity_only", "runtime_candidate", "runtime_approved"
}
PHASES = {"solid", "liquid", "gas", "plasma", "solution", "aerosol", "unknown"}
BEHAVIOR_CLASSES = {
    "GENERIC", "RULE_DRIVEN", "SPECIAL_CPU", "SPECIAL_GPU_CANDIDATE",
    "LEGACY_ONLY", "UNKNOWN",
}
PROVENANCE_METHODS = {
    "measured", "correlation", "derived", "interpolated", "estimated",
    "game_tuned", "exact_definition", "test_fixture",
}
CONFIDENCE_LEVELS = {"high", "medium", "low", "test_only"}
REDISTRIBUTION_STATUSES = {
    "redistributable", "redistributable_with_attribution", "public_domain",
    "project_generated", "reference_only", "permission_required",
}
TUNING_STATUSES = {
    "measured", "estimated", "interpolated", "game_tuned",
    "exact_definition", "test_fixture",
}
SOURCE_KINDS = {
    "primary_measurement", "peer_reviewed", "standard", "official_database",
    "vendor_datasheet", "software_generated", "project_generated", "test_fixture",
}
FORBIDDEN_VALUE_REDISTRIBUTION = {"reference_only", "permission_required"}
PROVENANCE_PLACEHOLDERS = {
    "unknown", "n/a", "na", "none", "not_applicable", "not applicable",
    "tbd", "todo", "unspecified", "unverified",
}
AI_PROVENANCE_RE = re.compile(
    r"(?i)(?:\bai[ -]?generated\b|\bchatgpt\b|\bgenerative[ -]?ai\b|"
    r"\blarge language model\b|\bllm\b)"
)
NONNEGATIVE_QUANTITIES = {
    "mass", "length", "time", "temperature", "amount", "pressure",
    "mass_density", "molar_mass", "heat_capacity", "specific_heat_capacity",
    "thermal_conductivity", "dynamic_viscosity", "diffusivity",
    "surface_tension", "first_order_rate_coefficient",
    "second_order_rate_coefficient", "amount_rate_density",
}


def _dimension(*values: int) -> dict[str, int]:
    return dict(zip(DIMENSIONS, values, strict=True))


QUANTITY_SPECS: dict[str, tuple[dict[str, int], str]] = {
    "dimensionless": (_dimension(0, 0, 0, 0, 0), "dimensionless"),
    "mass": (_dimension(1, 0, 0, 0, 0), "kilogram"),
    "length": (_dimension(0, 1, 0, 0, 0), "metre"),
    "time": (_dimension(0, 0, 1, 0, 0), "second"),
    "temperature": (_dimension(0, 0, 0, 1, 0), "kelvin"),
    "amount": (_dimension(0, 0, 0, 0, 1), "mole"),
    "pressure": (_dimension(1, -1, -2, 0, 0), "pascal"),
    "energy": (_dimension(1, 2, -2, 0, 0), "joule"),
    "mass_density": (_dimension(1, -3, 0, 0, 0), "kilogram_per_cubic_metre"),
    "molar_mass": (_dimension(1, 0, 0, 0, -1), "kilogram_per_mole"),
    "heat_capacity": (_dimension(1, 2, -2, -1, 0), "joule_per_kelvin"),
    "specific_heat_capacity": (
        _dimension(0, 2, -2, -1, 0), "joule_per_kilogram_kelvin"
    ),
    "thermal_conductivity": (
        _dimension(1, 1, -3, -1, 0), "watt_per_metre_kelvin"
    ),
    "dynamic_viscosity": (_dimension(1, -1, -1, 0, 0), "pascal_second"),
    "diffusivity": (_dimension(0, 2, -1, 0, 0), "square_metre_per_second"),
    "specific_energy": (_dimension(0, 2, -2, 0, 0), "joule_per_kilogram"),
    "molar_energy": (_dimension(1, 2, -2, 0, -1), "joule_per_mole"),
    "surface_tension": (_dimension(1, 0, -2, 0, 0), "newton_per_metre"),
    "first_order_rate_coefficient": (_dimension(0, 0, -1, 0, 0), "per_second"),
    "second_order_rate_coefficient": (
        _dimension(0, 3, -1, 0, -1), "cubic_metre_per_mole_second"
    ),
    "energy_density": (_dimension(1, -1, -2, 0, 0), "joule_per_cubic_metre"),
    "velocity": (_dimension(0, 1, -1, 0, 0), "metre_per_second"),
    "amount_rate_density": (
        _dimension(0, -3, -1, 0, 1), "mole_per_cubic_metre_second"
    ),
}

UNIT_SPECS: dict[str, tuple[str, str]] = {
    "dimensionless": ("1", "dimensionless"),
    "kilogram": ("kg", "mass"),
    "metre": ("m", "length"),
    "second": ("s", "time"),
    "kelvin": ("K", "temperature"),
    "mole": ("mol", "amount"),
    "pascal": ("Pa", "pressure"),
    "joule": ("J", "energy"),
    "kilogram_per_cubic_metre": ("kg/m^3", "mass_density"),
    "kilogram_per_mole": ("kg/mol", "molar_mass"),
    "joule_per_kelvin": ("J/K", "heat_capacity"),
    "joule_per_kilogram_kelvin": ("J/(kg*K)", "specific_heat_capacity"),
    "watt_per_metre_kelvin": ("W/(m*K)", "thermal_conductivity"),
    "pascal_second": ("Pa*s", "dynamic_viscosity"),
    "square_metre_per_second": ("m^2/s", "diffusivity"),
    "joule_per_kilogram": ("J/kg", "specific_energy"),
    "joule_per_mole": ("J/mol", "molar_energy"),
    "newton_per_metre": ("N/m", "surface_tension"),
    "per_second": ("1/s", "first_order_rate_coefficient"),
    "cubic_metre_per_mole_second": ("m^3/(mol*s)", "second_order_rate_coefficient"),
    "joule_per_cubic_metre": ("J/m^3", "energy_density"),
    "metre_per_second": ("m/s", "velocity"),
    "mole_per_cubic_metre_second": ("mol/(m^3*s)", "amount_rate_density"),
}


def expected_unit_registry() -> dict[str, Any]:
    quantity_kinds = [
        {"id": key, "dimension": dimension, "canonical_unit": unit}
        for key, (dimension, unit) in QUANTITY_SPECS.items()
    ]
    units = []
    for unit_id, (symbol, quantity_kind) in UNIT_SPECS.items():
        units.append({
            "id": unit_id,
            "symbol": symbol,
            "quantity_kind": quantity_kind,
            "dimension": QUANTITY_SPECS[quantity_kind][0],
            "scale_to_si": "1",
            "offset_to_si": "0",
        })
    return {
        "schema_version": SCHEMA_VERSION,
        "document_type": "omnicore_unit_registry",
        "registry_id": UNIT_REGISTRY_ID,
        "definition_basis": "canonical_si_units_only_no_physical_property_data",
        "dimensions": list(DIMENSIONS),
        "quantity_kinds": quantity_kinds,
        "units": units,
    }


CONTRACT_FIELDS: dict[str, tuple[set[str], set[str]]] = {
    "LocalizedNames": ({"en"}, {"en", "zh_CN"}),
    "Provenance": ({
        "source_title", "source_version", "source_date", "accessed_on",
        "source_locator", "source_kind", "method", "confidence", "redistribution_status",
        "license_or_terms", "license_evidence", "license_sha256", "tuning_status",
    }, {
        "source_title", "source_version", "source_date", "accessed_on",
        "source_locator", "source_kind", "method", "confidence", "redistribution_status",
        "license_or_terms", "license_evidence", "license_sha256", "tuning_status",
    }),
    "Range": ({"minimum", "maximum", "unit"}, {"minimum", "maximum", "unit"}),
    "Validity": ({"temperature", "pressure"}, {"temperature", "pressure"}),
    "PropertyDefinition": ({
        "property_id", "quantity_kind", "value", "unit", "validity", "provenance"
    }, {
        "property_id", "quantity_kind", "value", "unit", "validity", "provenance"
    }),
    "LegacyMapping": (
        {"element_identifier", "stable_id", "relationship"},
        {"element_identifier", "stable_id", "relationship"},
    ),
    "CompositionTerm": (
        {"component_kind", "component_id", "basis", "fraction"},
        {"component_kind", "component_id", "basis", "fraction"},
    ),
    "MaterialDefinition": ({
        "id", "names", "status", "behavior_class", "phases", "legacy_mapping",
        "composition", "properties", "provenance",
    }, {
        "id", "names", "status", "behavior_class", "phases", "legacy_mapping",
        "composition", "properties", "provenance",
    }),
    "SpeciesDefinition": ({
        "id", "names", "status", "chemistry_enabled", "elemental_composition",
        "charge_number", "phases", "legacy_mapping", "properties", "provenance",
    }, {
        "id", "names", "status", "chemistry_enabled", "elemental_composition",
        "charge_number", "phases", "legacy_mapping", "properties", "provenance",
    }),
    "StoichiometricTerm": (
        {"species_id", "coefficient", "phase"},
        {"species_id", "coefficient", "phase"},
    ),
    "ReactionDefinition": ({
        "id", "status", "reactants", "products", "reversible", "rate_model",
        "energy_change", "provenance",
    }, {
        "id", "status", "reactants", "products", "reversible", "rate_model",
        "energy_change", "provenance",
    }),
    "LegacyMapEntry": ({
        "identifier", "stable_id", "record_status", "canonical_identifier",
        "mapping_status",
    }, {
        "identifier", "stable_id", "record_status", "canonical_identifier",
        "mapping_status",
    }),
    "LegacyMaterialMap": ({
        "schema_version", "document_type", "mapping_id", "source_registry",
        "source_hash_contract", "source_sha256", "source_semantics", "physical_properties_imported",
        "generation_tool", "mappings",
    }, {
        "schema_version", "document_type", "mapping_id", "source_registry",
        "source_hash_contract", "source_sha256", "source_semantics", "physical_properties_imported",
        "generation_tool", "mappings",
    }),
    "Catalog": ({
        "schema_version", "document_type", "catalog_id", "dataset_version",
        "unit_registry_id", "catalog_status", "runtime_consumption", "materials",
        "species", "reactions",
    }, {
        "schema_version", "document_type", "catalog_id", "dataset_version",
        "unit_registry_id", "catalog_status", "runtime_consumption", "materials",
        "species", "reactions",
    }),
}


class ValidationFailure(ValueError):
    pass


def _reject_json_constant(value: str) -> None:
    raise ValidationFailure(f"non-standard/non-finite JSON constant: {value}")


def _reject_duplicate_keys(pairs: list[tuple[str, Any]]) -> dict[str, Any]:
    result: dict[str, Any] = {}
    for key, value in pairs:
        if key in result:
            raise ValidationFailure(f"duplicate JSON key: {key}")
        result[key] = value
    return result


def load_json_strict(path: Path) -> Any:
    try:
        payload = path.read_bytes()
    except OSError as exc:
        raise ValidationFailure(f"cannot read {path}: {exc}") from exc
    if payload.startswith(b"\xef\xbb\xbf"):
        raise ValidationFailure(f"{path}: UTF-8 BOM is forbidden")
    try:
        text = payload.decode("utf-8", errors="strict")
    except UnicodeDecodeError as exc:
        raise ValidationFailure(f"{path}: invalid UTF-8: {exc}") from exc
    try:
        return json.loads(
            text,
            object_pairs_hook=_reject_duplicate_keys,
            parse_constant=_reject_json_constant,
        )
    except (json.JSONDecodeError, ValidationFailure) as exc:
        raise ValidationFailure(f"{path}: invalid JSON: {exc}") from exc


def _has_control(value: str) -> bool:
    return any(ord(character) < 0x20 or ord(character) == 0x7F for character in value)


def _string(value: Any, label: str, errors: list[str], *, pattern: re.Pattern[str] | None = None) -> str | None:
    if not isinstance(value, str) or not value or value != value.strip() or _has_control(value):
        errors.append(f"{label}: expected nonempty trimmed string without control characters")
        return None
    if pattern is not None and not pattern.fullmatch(value):
        errors.append(f"{label}: invalid format: {value!r}")
        return None
    return value


def _check_object(
    value: Any,
    label: str,
    contract: str,
    errors: list[str],
) -> dict[str, Any] | None:
    if not isinstance(value, dict):
        errors.append(f"{label}: expected object")
        return None
    required, allowed = CONTRACT_FIELDS[contract]
    missing = sorted(required - set(value))
    unknown = sorted(set(value) - allowed)
    if missing:
        errors.append(f"{label}: missing fields: {', '.join(missing)}")
    if unknown:
        errors.append(f"{label}: unknown fields: {', '.join(unknown)}")
    return value


def _list(value: Any, label: str, errors: list[str]) -> list[Any] | None:
    if not isinstance(value, list):
        errors.append(f"{label}: expected array")
        return None
    return value


def _decimal(value: Any, label: str, errors: list[str]) -> Decimal | None:
    if not isinstance(value, str) or not DECIMAL_RE.fullmatch(value):
        errors.append(f"{label}: expected canonical finite decimal string")
        return None
    try:
        parsed = Decimal(value)
    except InvalidOperation:
        errors.append(f"{label}: invalid decimal")
        return None
    if not parsed.is_finite():
        errors.append(f"{label}: non-finite decimal")
        return None
    if parsed == 0 and value.startswith("-"):
        errors.append(f"{label}: negative zero is not canonical")
        return None
    return parsed


def _rational(value: Any, label: str, errors: list[str]) -> Fraction | None:
    if not isinstance(value, str) or not RATIONAL_RE.fullmatch(value):
        errors.append(f"{label}: expected positive canonical rational string")
        return None
    try:
        parsed = Fraction(value)
    except (ValueError, ZeroDivisionError):
        errors.append(f"{label}: invalid rational")
        return None
    if parsed <= 0:
        errors.append(f"{label}: rational must be positive")
        return None
    canonical = (
        str(parsed.numerator)
        if parsed.denominator == 1
        else f"{parsed.numerator}/{parsed.denominator}"
    )
    if value != canonical:
        errors.append(f"{label}: rational must be reduced canonical form {canonical}")
        return None
    return parsed


def _iso_date(value: Any, label: str, errors: list[str]) -> date | None:
    checked = _string(value, label, errors)
    if checked is None:
        return None
    try:
        return date.fromisoformat(checked)
    except ValueError:
        errors.append(f"{label}: expected ISO calendar date YYYY-MM-DD")
        return None


def _sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest().upper()


def _sha256_bytes(payload: bytes) -> str:
    return hashlib.sha256(payload).hexdigest().upper()


def _canonical_nonnegative_integer(value: str, label: str) -> int:
    if not re.fullmatch(r"0|[1-9][0-9]*", value):
        raise ValidationFailure(f"{label}: expected canonical nonnegative integer")
    return int(value, 10)


def read_csv_strict(path: Path) -> tuple[list[str], list[dict[str, str]]]:
    try:
        stream = path.open("r", encoding="utf-8-sig", newline="")
    except OSError as exc:
        raise ValidationFailure(f"cannot read {path}: {exc}") from exc
    with stream:
        reader = csv.reader(stream)
        try:
            header = next(reader)
        except StopIteration as exc:
            raise ValidationFailure(f"{path}: empty CSV") from exc
        if not header or any(not field for field in header):
            raise ValidationFailure(f"{path}: blank CSV header field")
        if len(header) != len(set(header)):
            raise ValidationFailure(f"{path}: duplicate CSV header field")
        rows: list[dict[str, str]] = []
        for line_number, fields in enumerate(reader, start=2):
            if len(fields) != len(header):
                raise ValidationFailure(
                    f"{path}:{line_number}: expected {len(header)} fields, got {len(fields)}"
                )
            rows.append(dict(zip(header, fields, strict=True)))
    return header, rows


def canonical_csv_sha256(header: list[str], rows: list[dict[str, str]]) -> str:
    stream = io.StringIO(newline="")
    writer = csv.writer(stream, lineterminator="\n")
    writer.writerow([
        field.replace("\r\n", "\n").replace("\r", "\n")
        for field in header
    ])
    for row in rows:
        writer.writerow([
            row[field].replace("\r\n", "\n").replace("\r", "\n")
            for field in header
        ])
    return _sha256_bytes(stream.getvalue().encode("utf-8"))


def build_legacy_map(root: Path) -> dict[str, Any]:
    registry_path = root / "docs" / "ELEMENT_REGISTRY.csv"
    header, rows = read_csv_strict(registry_path)
    required = {
        "identifier", "stable_id", "implementation_status", "is_duplicate", "duplicate_of"
    }
    missing = sorted(required - set(header))
    if missing:
        raise ValidationFailure(
            f"{registry_path}: missing mapping fields: {', '.join(missing)}"
        )
    implemented = [row for row in rows if row["implementation_status"] == "implemented"]
    mappings: list[dict[str, Any]] = []
    identifiers: set[str] = set()
    stable_ids: set[int] = set()
    for row in implemented:
        identifier = row["identifier"]
        if not LEGACY_ID_RE.fullmatch(identifier):
            raise ValidationFailure(f"{registry_path}: invalid identifier {identifier!r}")
        if identifier.casefold() in identifiers:
            raise ValidationFailure(f"{registry_path}: duplicate identifier {identifier}")
        identifiers.add(identifier.casefold())
        stable_id = _canonical_nonnegative_integer(
            row["stable_id"], f"{registry_path}: {identifier} stable_id"
        )
        if stable_id < 0 or stable_id in stable_ids:
            raise ValidationFailure(
                f"{registry_path}: {identifier} has duplicate/negative stable_id {stable_id}"
            )
        stable_ids.add(stable_id)
        if row["is_duplicate"] not in {"true", "false"}:
            raise ValidationFailure(
                f"{registry_path}: {identifier} has invalid is_duplicate flag"
            )
        is_alias = row["is_duplicate"] == "true"
        canonical = row["duplicate_of"] if is_alias else identifier
        if not canonical or not LEGACY_ID_RE.fullmatch(canonical):
            raise ValidationFailure(
                f"{registry_path}: {identifier} has invalid canonical identifier"
            )
        mappings.append({
            "identifier": identifier,
            "stable_id": stable_id,
            "record_status": "compatibility_alias" if is_alias else "canonical",
            "canonical_identifier": canonical,
            "mapping_status": "identity_only",
        })
    mappings.sort(key=lambda item: (item["stable_id"], item["identifier"]))
    known = {item["identifier"] for item in mappings}
    for item in mappings:
        if item["canonical_identifier"] not in known:
            raise ValidationFailure(
                f"{registry_path}: {item['identifier']} targets unknown canonical identifier "
                f"{item['canonical_identifier']}"
            )
    return {
        "schema_version": SCHEMA_VERSION,
        "document_type": "legacy_material_map",
        "mapping_id": "omnicore.legacy-material-map.v1",
        "source_registry": "docs/ELEMENT_REGISTRY.csv",
        "source_hash_contract": "canonical_csv_utf8_lf_v1",
        "source_sha256": canonical_csv_sha256(header, rows),
        "source_semantics": "legacy_gameplay_metadata_not_si",
        "physical_properties_imported": False,
        "generation_tool": "tools/omnicore_data_check.py",
        "mappings": mappings,
    }


def _canonical_json(document: Any) -> str:
    return json.dumps(document, ensure_ascii=False, indent=2) + "\n"


def semantic_json_sha256(document: Any) -> str:
    payload = json.dumps(
        document,
        ensure_ascii=False,
        sort_keys=True,
        separators=(",", ":"),
    ).encode("utf-8")
    return _sha256_bytes(payload)


def refresh_legacy_map(root: Path, destination: Path) -> None:
    document = build_legacy_map(root)
    destination.parent.mkdir(parents=True, exist_ok=True)
    payload = _canonical_json(document)
    temporary_name: str | None = None
    try:
        with tempfile.NamedTemporaryFile(
            "w", encoding="utf-8", newline="\n", dir=destination.parent,
            prefix=destination.name + ".", suffix=".tmp", delete=False,
        ) as stream:
            stream.write(payload)
            stream.flush()
            os.fsync(stream.fileno())
            temporary_name = stream.name
        os.replace(temporary_name, destination)
        temporary_name = None
    finally:
        if temporary_name is not None:
            Path(temporary_name).unlink(missing_ok=True)


def _validate_schema(schema: Any, unit_schema: Any, errors: list[str]) -> None:
    if semantic_json_sha256(schema) != DATA_SCHEMA_SEMANTIC_SHA256:
        errors.append("schema.json: complete semantic contract hash drifted")
    if semantic_json_sha256(unit_schema) != UNIT_SCHEMA_SEMANTIC_SHA256:
        errors.append("unit-registry.schema.json: complete semantic contract hash drifted")
    if not isinstance(schema, dict) or schema.get("$schema") != "https://json-schema.org/draft/2020-12/schema":
        errors.append("schema.json: expected JSON Schema draft 2020-12")
        return
    definitions = schema.get("$defs")
    if not isinstance(definitions, dict):
        errors.append("schema.json: missing $defs object")
        return
    for name, (required, allowed) in CONTRACT_FIELDS.items():
        definition = definitions.get(name)
        if not isinstance(definition, dict):
            errors.append(f"schema.json: missing $defs/{name}")
            continue
        if name not in {"LegacyMapping"} and definition.get("additionalProperties") is not False:
            errors.append(f"schema.json: $defs/{name} must reject additional properties")
        if set(definition.get("required", [])) != required:
            errors.append(f"schema.json: $defs/{name} required fields drifted")
        properties = definition.get("properties")
        if not isinstance(properties, dict) or set(properties) != allowed:
            errors.append(f"schema.json: $defs/{name} property fields drifted")
    status = definitions.get("RecordStatus", {}).get("enum")
    if set(status or []) != RECORD_STATUSES:
        errors.append("schema.json: RecordStatus enum drifted")
    phases = definitions.get("Phase", {}).get("enum")
    if set(phases or []) != PHASES:
        errors.append("schema.json: Phase enum drifted")
    if not isinstance(unit_schema, dict) or unit_schema.get("$schema") != "https://json-schema.org/draft/2020-12/schema":
        errors.append("unit-registry.schema.json: expected JSON Schema draft 2020-12")
        return
    unit_defs = unit_schema.get("$defs")
    if not isinstance(unit_defs, dict):
        errors.append("unit-registry.schema.json: missing $defs object")
        return
    expected_unit_defs = {
        "DimensionVector": {"mass", "length", "time", "temperature", "amount"},
        "QuantityKind": {"id", "dimension", "canonical_unit"},
        "UnitDefinition": {
            "id", "symbol", "quantity_kind", "dimension", "scale_to_si", "offset_to_si"
        },
        "UnitRegistry": {
            "schema_version", "document_type", "registry_id", "definition_basis",
            "dimensions", "quantity_kinds", "units",
        },
    }
    for name, fields in expected_unit_defs.items():
        definition = unit_defs.get(name)
        if not isinstance(definition, dict):
            errors.append(f"unit-registry.schema.json: missing $defs/{name}")
            continue
        if definition.get("additionalProperties") is not False:
            errors.append(
                f"unit-registry.schema.json: $defs/{name} must reject additional properties"
            )
        if set(definition.get("required", [])) != fields:
            errors.append(f"unit-registry.schema.json: $defs/{name} required fields drifted")
        properties = definition.get("properties")
        if not isinstance(properties, dict) or set(properties) != fields:
            errors.append(f"unit-registry.schema.json: $defs/{name} property fields drifted")


def _validate_units(document: Any, errors: list[str]) -> dict[str, str]:
    expected = expected_unit_registry()
    if document != expected:
        errors.append(
            "units.json: canonical registry differs from the validator-owned SI v1 contract"
        )
    return {
        unit_id: quantity_kind
        for unit_id, (_, quantity_kind) in UNIT_SPECS.items()
    }


def _validate_names(value: Any, label: str, errors: list[str]) -> None:
    obj = _check_object(value, label, "LocalizedNames", errors)
    if obj is None:
        return
    _string(obj.get("en"), f"{label}.en", errors)
    if "zh_CN" in obj:
        _string(obj.get("zh_CN"), f"{label}.zh_CN", errors)


def _validate_provenance(
    value: Any,
    label: str,
    root: Path,
    catalog_id: str,
    errors: list[str],
    *,
    numeric_value: bool,
    runtime_record: bool = False,
) -> None:
    obj = _check_object(value, label, "Provenance", errors)
    if obj is None:
        return
    for field in (
        "source_title", "source_version", "source_locator", "license_or_terms",
        "license_evidence",
    ):
        checked = _string(obj.get(field), f"{label}.{field}", errors)
        if checked is not None:
            if checked.casefold() in PROVENANCE_PLACEHOLDERS:
                errors.append(f"{label}.{field}: placeholder provenance is forbidden")
            if AI_PROVENANCE_RE.search(checked):
                errors.append(f"{label}.{field}: AI-generated data is forbidden provenance")
    source_date = _iso_date(obj.get("source_date"), f"{label}.source_date", errors)
    accessed_on = _iso_date(obj.get("accessed_on"), f"{label}.accessed_on", errors)
    if source_date is not None and accessed_on is not None and source_date > accessed_on:
        errors.append(f"{label}: source_date is after accessed_on")
    method = obj.get("method")
    source_kind = obj.get("source_kind")
    confidence = obj.get("confidence")
    redistribution = obj.get("redistribution_status")
    tuning = obj.get("tuning_status")
    if source_kind not in SOURCE_KINDS:
        errors.append(f"{label}.source_kind: unsupported value {source_kind!r}")
    if method not in PROVENANCE_METHODS:
        errors.append(f"{label}.method: unsupported value {method!r}")
    if confidence not in CONFIDENCE_LEVELS:
        errors.append(f"{label}.confidence: unsupported value {confidence!r}")
    if redistribution not in REDISTRIBUTION_STATUSES:
        errors.append(
            f"{label}.redistribution_status: unsupported value {redistribution!r}"
        )
    if tuning not in TUNING_STATUSES:
        errors.append(f"{label}.tuning_status: unsupported value {tuning!r}")
    fixture_fields = (
        source_kind == "test_fixture"
        or method == "test_fixture"
        or tuning == "test_fixture"
    )
    if fixture_fields:
        if not catalog_id.startswith("test."):
            errors.append(f"{label}: test_fixture provenance is forbidden outside test.* catalogs")
        if not (
            method == "test_fixture"
            and source_kind == "test_fixture"
            and tuning == "test_fixture"
            and confidence == "test_only"
            and redistribution == "project_generated"
        ):
            errors.append(f"{label}: test_fixture provenance fields are inconsistent")
    if (numeric_value or runtime_record) and redistribution in FORBIDDEN_VALUE_REDISTRIBUTION:
        errors.append(
            f"{label}: bundled/runtime data cannot use redistribution_status={redistribution}"
        )
    evidence = obj.get("license_evidence")
    evidence_hash = obj.get("license_sha256")
    if evidence_hash is not None and (
        not isinstance(evidence_hash, str) or not SHA256_RE.fullmatch(evidence_hash)
    ):
        errors.append(f"{label}.license_sha256: expected uppercase SHA-256 or null")
    if isinstance(evidence, str) and evidence:
        if evidence.startswith(("https://", "http://", "project://", "test://")):
            if evidence_hash is not None:
                errors.append(
                    f"{label}: remote/generated license evidence cannot claim an unverified local hash"
                )
        else:
            evidence_path = Path(evidence)
            if evidence_path.is_absolute() or ".." in evidence_path.parts:
                errors.append(f"{label}.license_evidence: unsafe repository path")
            else:
                resolved = (root / evidence_path).resolve()
                try:
                    resolved.relative_to(root.resolve())
                except ValueError:
                    errors.append(f"{label}.license_evidence: path escapes source root")
                else:
                    if not resolved.is_file():
                        errors.append(f"{label}.license_evidence: file does not exist: {evidence}")
                    elif evidence_hash is None:
                        errors.append(f"{label}.license_sha256: required for repository evidence")
                    elif _sha256(resolved) != evidence_hash:
                        errors.append(f"{label}.license_sha256: does not match {evidence}")


def _validate_range(
    value: Any,
    label: str,
    expected_unit: str,
    errors: list[str],
) -> tuple[Decimal | None, Decimal | None]:
    obj = _check_object(value, label, "Range", errors)
    if obj is None:
        return None, None
    minimum = _decimal(obj.get("minimum"), f"{label}.minimum", errors)
    maximum = _decimal(obj.get("maximum"), f"{label}.maximum", errors)
    if obj.get("unit") != expected_unit:
        errors.append(f"{label}.unit: expected canonical unit {expected_unit}")
    if minimum is not None and minimum < 0:
        errors.append(f"{label}.minimum: absolute range cannot be negative")
    if maximum is not None and maximum < 0:
        errors.append(f"{label}.maximum: absolute range cannot be negative")
    if minimum is not None and maximum is not None and minimum > maximum:
        errors.append(f"{label}: minimum exceeds maximum")
    return minimum, maximum


def _validate_property(
    value: Any,
    label: str,
    root: Path,
    catalog_id: str,
    errors: list[str],
) -> tuple[str | None, str | None]:
    obj = _check_object(value, label, "PropertyDefinition", errors)
    if obj is None:
        return None, None
    property_id = _string(obj.get("property_id"), f"{label}.property_id", errors, pattern=ID_RE)
    quantity = _string(obj.get("quantity_kind"), f"{label}.quantity_kind", errors, pattern=ID_RE)
    unit = _string(obj.get("unit"), f"{label}.unit", errors, pattern=ID_RE)
    parsed_value = _decimal(obj.get("value"), f"{label}.value", errors)
    if quantity not in QUANTITY_SPECS:
        errors.append(f"{label}.quantity_kind: unknown canonical quantity {quantity!r}")
    elif unit != QUANTITY_SPECS[quantity][1]:
        errors.append(
            f"{label}.unit: {unit!r} is not canonical for quantity {quantity!r}"
        )
    if parsed_value is not None and quantity in NONNEGATIVE_QUANTITIES and parsed_value < 0:
        errors.append(f"{label}.value: {quantity} cannot be negative")
    validity = _check_object(obj.get("validity"), f"{label}.validity", "Validity", errors)
    if validity is not None:
        _validate_range(
            validity.get("temperature"), f"{label}.validity.temperature", "kelvin", errors
        )
        _validate_range(
            validity.get("pressure"), f"{label}.validity.pressure", "pascal", errors
        )
    _validate_provenance(
        obj.get("provenance"), f"{label}.provenance", root, catalog_id, errors,
        numeric_value=True,
    )
    return property_id, quantity


def _validate_properties(
    value: Any,
    label: str,
    root: Path,
    catalog_id: str,
    errors: list[str],
) -> list[dict[str, Any]]:
    items = _list(value, label, errors)
    if items is None:
        return []
    seen: set[str] = set()
    valid_items: list[dict[str, Any]] = []
    for index, item in enumerate(items):
        item_label = f"{label}[{index}]"
        property_id, _ = _validate_property(item, item_label, root, catalog_id, errors)
        if property_id is not None:
            folded = property_id.casefold()
            if folded in seen:
                errors.append(f"{item_label}.property_id: duplicate property {property_id}")
            seen.add(folded)
        if isinstance(item, dict):
            valid_items.append(item)
    return valid_items


def _legacy_registry(root: Path, errors: list[str]) -> dict[str, dict[str, str]]:
    try:
        _, rows = read_csv_strict(root / "docs" / "ELEMENT_REGISTRY.csv")
    except ValidationFailure as exc:
        errors.append(str(exc))
        return {}
    result: dict[str, dict[str, str]] = {}
    for row in rows:
        identifier = row.get("identifier", "")
        if identifier in result:
            errors.append(f"ELEMENT_REGISTRY.csv: duplicate identifier {identifier}")
        result[identifier] = row
    return result


def _atomic_symbols(root: Path, errors: list[str]) -> set[str]:
    try:
        _, rows = read_csv_strict(root / "docs" / "PERIODIC_ELEMENT_SOURCE_MAP.csv")
    except ValidationFailure as exc:
        errors.append(str(exc))
        return set()
    symbols = {row.get("symbol", "") for row in rows}
    if len(symbols) != 118 or "" in symbols:
        errors.append(
            "PERIODIC_ELEMENT_SOURCE_MAP.csv: expected 118 unique nonempty atomic symbols"
        )
    return symbols


def _validate_legacy_mapping(
    value: Any,
    label: str,
    registry: dict[str, dict[str, str]],
    errors: list[str],
) -> tuple[str, int] | None:
    if value is None:
        return None
    obj = _check_object(value, label, "LegacyMapping", errors)
    if obj is None:
        return None
    identifier = _string(
        obj.get("element_identifier"), f"{label}.element_identifier", errors,
        pattern=LEGACY_ID_RE,
    )
    stable_id = obj.get("stable_id")
    if isinstance(stable_id, bool) or not isinstance(stable_id, int) or stable_id < 0:
        errors.append(f"{label}.stable_id: expected nonnegative integer")
        stable_id = -1
    if obj.get("relationship") not in {"identity", "proxy", "tool", "visualization"}:
        errors.append(f"{label}.relationship: unsupported relationship")
    if identifier is None:
        return None
    row = registry.get(identifier)
    if row is None:
        errors.append(f"{label}: unknown Legacy identifier {identifier}")
        return identifier, stable_id
    if row.get("implementation_status") != "implemented":
        errors.append(f"{label}: Legacy identifier {identifier} is not implemented")
    if row.get("is_duplicate") == "true":
        errors.append(f"{label}: compatibility aliases cannot own physical definitions")
    try:
        expected_id = _canonical_nonnegative_integer(
            row.get("stable_id", ""),
            f"{label}: registry stable ID for {identifier}",
        )
    except ValidationFailure as exc:
        errors.append(str(exc))
    else:
        if stable_id != expected_id:
            errors.append(
                f"{label}: stable ID mismatch for {identifier}: {stable_id} != {expected_id}"
            )
    return identifier, stable_id


def _validate_phases(value: Any, label: str, errors: list[str]) -> None:
    phases = _list(value, label, errors)
    if phases is None:
        return
    if not phases:
        errors.append(f"{label}: at least one phase is required")
    if any(phase not in PHASES for phase in phases):
        errors.append(f"{label}: unknown phase")
    if len(phases) != len(set(phase for phase in phases if isinstance(phase, str))):
        errors.append(f"{label}: duplicate phase")


def _record_status(value: Any, label: str, errors: list[str]) -> str | None:
    if value not in RECORD_STATUSES:
        errors.append(f"{label}: unsupported record status {value!r}")
        return None
    return value


def _validate_material(
    value: Any,
    index: int,
    root: Path,
    catalog_id: str,
    registry: dict[str, dict[str, str]],
    errors: list[str],
) -> tuple[str | None, dict[str, Any] | None, tuple[str, int] | None]:
    label = f"catalog.materials[{index}]"
    obj = _check_object(value, label, "MaterialDefinition", errors)
    if obj is None:
        return None, None, None
    record_id = _string(obj.get("id"), f"{label}.id", errors, pattern=MATERIAL_ID_RE)
    _validate_names(obj.get("names"), f"{label}.names", errors)
    status = _record_status(obj.get("status"), f"{label}.status", errors)
    if obj.get("behavior_class") not in BEHAVIOR_CLASSES:
        errors.append(f"{label}.behavior_class: unsupported behavior class")
    _validate_phases(obj.get("phases"), f"{label}.phases", errors)
    mapping = _validate_legacy_mapping(
        obj.get("legacy_mapping"), f"{label}.legacy_mapping", registry, errors
    )
    composition = _list(obj.get("composition"), f"{label}.composition", errors)
    if composition is not None:
        seen_components: set[tuple[str, str]] = set()
        bases: set[str] = set()
        total = Fraction(0)
        for term_index, term in enumerate(composition):
            term_label = f"{label}.composition[{term_index}]"
            term_obj = _check_object(term, term_label, "CompositionTerm", errors)
            if term_obj is None:
                continue
            kind = term_obj.get("component_kind")
            component_id = _string(
                term_obj.get("component_id"), f"{term_label}.component_id", errors,
                pattern=ID_RE,
            )
            basis = term_obj.get("basis")
            if kind not in {"material", "species"}:
                errors.append(f"{term_label}.component_kind: unsupported kind")
            if basis not in {"mass_fraction", "mole_fraction", "volume_fraction"}:
                errors.append(f"{term_label}.basis: unsupported basis")
            else:
                bases.add(basis)
            fraction = _rational(term_obj.get("fraction"), f"{term_label}.fraction", errors)
            if fraction is not None:
                total += fraction
            if isinstance(kind, str) and component_id is not None:
                key = (kind, component_id.casefold())
                if key in seen_components:
                    errors.append(f"{term_label}: duplicate composition component")
                seen_components.add(key)
        if composition and len(bases) != 1:
            errors.append(f"{label}.composition: all fractions must use one basis")
        if composition and total != 1:
            errors.append(f"{label}.composition: fractions must sum exactly to 1")
    properties = _validate_properties(
        obj.get("properties"), f"{label}.properties", root, catalog_id, errors
    )
    if status in {"identity_only", "legacy_only"} and properties:
        errors.append(f"{label}: {status} records cannot carry physical properties")
    if status in {"identity_only", "legacy_only"} and mapping is None:
        errors.append(f"{label}: {status} material requires a Legacy mapping")
    if (
        status == "identity_only"
        and isinstance(obj.get("legacy_mapping"), dict)
        and obj["legacy_mapping"].get("relationship") != "identity"
    ):
        errors.append(f"{label}: identity_only material requires relationship=identity")
    _validate_provenance(
        obj.get("provenance"), f"{label}.provenance", root, catalog_id, errors,
        numeric_value=bool(composition),
        runtime_record=status in {"runtime_candidate", "runtime_approved"},
    )
    return record_id, obj, mapping


def _validate_species(
    value: Any,
    index: int,
    root: Path,
    catalog_id: str,
    registry: dict[str, dict[str, str]],
    atomic_symbols: set[str],
    errors: list[str],
) -> tuple[str | None, dict[str, Any] | None, tuple[str, int] | None]:
    label = f"catalog.species[{index}]"
    obj = _check_object(value, label, "SpeciesDefinition", errors)
    if obj is None:
        return None, None, None
    record_id = _string(obj.get("id"), f"{label}.id", errors, pattern=SPECIES_ID_RE)
    _validate_names(obj.get("names"), f"{label}.names", errors)
    status = _record_status(obj.get("status"), f"{label}.status", errors)
    chemistry_enabled = obj.get("chemistry_enabled")
    if not isinstance(chemistry_enabled, bool):
        errors.append(f"{label}.chemistry_enabled: expected boolean")
        chemistry_enabled = False
    composition = obj.get("elemental_composition")
    if not isinstance(composition, dict):
        errors.append(f"{label}.elemental_composition: expected object")
        composition = {}
    else:
        for symbol, count in composition.items():
            if symbol not in atomic_symbols:
                errors.append(f"{label}.elemental_composition: unknown atomic symbol {symbol!r}")
            if isinstance(count, bool) or not isinstance(count, int) or count <= 0:
                errors.append(
                    f"{label}.elemental_composition.{symbol}: expected positive integer"
                )
    if chemistry_enabled and not composition:
        errors.append(f"{label}: chemistry-enabled species requires elemental composition")
    charge = obj.get("charge_number")
    if isinstance(charge, bool) or not isinstance(charge, int):
        errors.append(f"{label}.charge_number: expected integer")
    _validate_phases(obj.get("phases"), f"{label}.phases", errors)
    mapping = _validate_legacy_mapping(
        obj.get("legacy_mapping"), f"{label}.legacy_mapping", registry, errors
    )
    properties = _validate_properties(
        obj.get("properties"), f"{label}.properties", root, catalog_id, errors
    )
    if status in {"identity_only", "legacy_only"} and properties:
        errors.append(f"{label}: {status} records cannot carry physical properties")
    if (
        status == "identity_only"
        and isinstance(obj.get("legacy_mapping"), dict)
        and obj["legacy_mapping"].get("relationship") != "identity"
    ):
        errors.append(f"{label}: identity_only species requires relationship=identity")
    _validate_provenance(
        obj.get("provenance"), f"{label}.provenance", root, catalog_id, errors,
        numeric_value=bool(composition),
        runtime_record=status in {"runtime_candidate", "runtime_approved"},
    )
    return record_id, obj, mapping


def _validate_stoichiometric_side(
    value: Any,
    label: str,
    species: dict[str, dict[str, Any]],
    errors: list[str],
) -> list[tuple[dict[str, Any], Fraction]]:
    items = _list(value, label, errors)
    if items is None:
        return []
    if not items:
        errors.append(f"{label}: at least one participant is required")
    result: list[tuple[dict[str, Any], Fraction]] = []
    seen: set[str] = set()
    for index, item in enumerate(items):
        item_label = f"{label}[{index}]"
        obj = _check_object(item, item_label, "StoichiometricTerm", errors)
        if obj is None:
            continue
        species_id = _string(
            obj.get("species_id"), f"{item_label}.species_id", errors,
            pattern=SPECIES_ID_RE,
        )
        coefficient = _rational(
            obj.get("coefficient"), f"{item_label}.coefficient", errors
        )
        phase = obj.get("phase")
        if phase not in PHASES:
            errors.append(f"{item_label}.phase: unknown phase")
        if species_id is None or coefficient is None:
            continue
        folded = species_id.casefold()
        if folded in seen:
            errors.append(f"{item_label}: duplicate reaction participant {species_id}")
        seen.add(folded)
        species_record = species.get(species_id)
        if species_record is None:
            errors.append(f"{item_label}: unknown species {species_id}")
            continue
        if species_record.get("chemistry_enabled") is not True:
            errors.append(f"{item_label}: species is not chemistry-enabled: {species_id}")
        species_phases = species_record.get("phases")
        if isinstance(species_phases, list) and phase not in species_phases:
            errors.append(
                f"{item_label}.phase: {phase!r} is not declared by species {species_id}"
            )
        result.append((species_record, coefficient))
    return result


def _balance(
    side: Iterable[tuple[dict[str, Any], Fraction]],
) -> tuple[dict[str, Fraction], Fraction]:
    atoms: dict[str, Fraction] = {}
    charge = Fraction(0)
    for species, coefficient in side:
        for symbol, count in species.get("elemental_composition", {}).items():
            if isinstance(count, int) and not isinstance(count, bool):
                atoms[symbol] = atoms.get(symbol, Fraction(0)) + coefficient * count
        species_charge = species.get("charge_number")
        if isinstance(species_charge, int) and not isinstance(species_charge, bool):
            charge += coefficient * species_charge
    return atoms, charge


def _validate_rate_model(
    value: Any,
    label: str,
    status: str | None,
    root: Path,
    catalog_id: str,
    errors: list[str],
) -> str | None:
    if not isinstance(value, dict):
        errors.append(f"{label}: expected object")
        return None
    kind = value.get("kind")
    if kind == "unselected":
        expected = {"kind", "rationale"}
        if set(value) != expected:
            errors.append(f"{label}: unselected model fields must be exactly {sorted(expected)}")
        _string(value.get("rationale"), f"{label}.rationale", errors)
        if status in {"runtime_candidate", "runtime_approved"}:
            errors.append(f"{label}: runtime record requires a selected rate model")
        return kind
    if kind != "arrhenius":
        errors.append(f"{label}.kind: unsupported rate model {kind!r}")
        return None
    if status in {"identity_only", "legacy_only"}:
        errors.append(f"{label}: {status} reaction must keep rate_model.kind=unselected")
    expected = {
        "kind", "overall_order", "pre_exponential_factor",
        "temperature_exponent", "activation_energy",
    }
    if set(value) != expected:
        errors.append(f"{label}: Arrhenius model fields must be exactly {sorted(expected)}")
    order = value.get("overall_order")
    if isinstance(order, bool) or order not in {1, 2}:
        errors.append(f"{label}.overall_order: only exact order 1 or 2 is supported in v1")
    expected_prefactor = {
        1: "first_order_rate_coefficient",
        2: "second_order_rate_coefficient",
    }.get(order)
    _, prefactor_kind = _validate_property(
        value.get("pre_exponential_factor"), f"{label}.pre_exponential_factor",
        root, catalog_id, errors,
    )
    _, exponent_kind = _validate_property(
        value.get("temperature_exponent"), f"{label}.temperature_exponent",
        root, catalog_id, errors,
    )
    _, activation_kind = _validate_property(
        value.get("activation_energy"), f"{label}.activation_energy",
        root, catalog_id, errors,
    )
    if expected_prefactor is not None and prefactor_kind != expected_prefactor:
        errors.append(
            f"{label}.pre_exponential_factor: expected {expected_prefactor} for order {order}"
        )
    if exponent_kind != "dimensionless":
        errors.append(f"{label}.temperature_exponent: expected dimensionless quantity")
    if activation_kind != "molar_energy":
        errors.append(f"{label}.activation_energy: expected molar_energy quantity")
    return kind


def _validate_reaction(
    value: Any,
    index: int,
    root: Path,
    catalog_id: str,
    species: dict[str, dict[str, Any]],
    errors: list[str],
) -> str | None:
    label = f"catalog.reactions[{index}]"
    obj = _check_object(value, label, "ReactionDefinition", errors)
    if obj is None:
        return None
    record_id = _string(obj.get("id"), f"{label}.id", errors, pattern=REACTION_ID_RE)
    status = _record_status(obj.get("status"), f"{label}.status", errors)
    reactants = _validate_stoichiometric_side(
        obj.get("reactants"), f"{label}.reactants", species, errors
    )
    products = _validate_stoichiometric_side(
        obj.get("products"), f"{label}.products", species, errors
    )
    if not isinstance(obj.get("reversible"), bool):
        errors.append(f"{label}.reversible: expected boolean")
    rate_kind = _validate_rate_model(
        obj.get("rate_model"), f"{label}.rate_model", status,
        root, catalog_id, errors,
    )
    energy = obj.get("energy_change")
    if energy is None:
        if status in {"runtime_candidate", "runtime_approved"}:
            errors.append(f"{label}.energy_change: runtime reaction requires energy data")
    else:
        if status in {"identity_only", "legacy_only"}:
            errors.append(f"{label}.energy_change: {status} reaction must use null")
        _, energy_kind = _validate_property(
            energy, f"{label}.energy_change", root, catalog_id, errors
        )
        if energy_kind != "molar_energy":
            errors.append(f"{label}.energy_change: expected molar_energy quantity")
    _validate_provenance(
        obj.get("provenance"), f"{label}.provenance", root, catalog_id, errors,
        numeric_value=True,
        runtime_record=status in {"runtime_candidate", "runtime_approved"},
    )
    if reactants and products:
        reactant_atoms, reactant_charge = _balance(reactants)
        product_atoms, product_charge = _balance(products)
        if reactant_atoms != product_atoms:
            errors.append(
                f"{label}: atom imbalance: reactants={reactant_atoms}, products={product_atoms}"
            )
        if reactant_charge != product_charge:
            errors.append(
                f"{label}: charge imbalance: reactants={reactant_charge}, "
                f"products={product_charge}"
            )
    if status in {"runtime_candidate", "runtime_approved"} and rate_kind != "arrhenius":
        errors.append(f"{label}: runtime reaction does not have a validated Arrhenius model")
    return record_id


def _material_cycles(
    materials: dict[str, dict[str, Any]],
    errors: list[str],
) -> None:
    edges: dict[str, list[str]] = {}
    for material_id, material in materials.items():
        targets = []
        for term in material.get("composition", []):
            if isinstance(term, dict) and term.get("component_kind") == "material":
                component = term.get("component_id")
                if isinstance(component, str):
                    targets.append(component)
        edges[material_id] = targets
    visiting: set[str] = set()
    visited: set[str] = set()

    def visit(node: str, path: list[str]) -> None:
        if node in visiting:
            cycle = path[path.index(node):]
            errors.append("catalog.materials: composition cycle: " + " -> ".join(cycle))
            return
        if node in visited:
            return
        visiting.add(node)
        for target in edges.get(node, []):
            visit(target, path + [target])
        visiting.remove(node)
        visited.add(node)

    for material_id in sorted(materials):
        visit(material_id, [material_id])


def validate_catalog(document: Any, root: Path, errors: list[str]) -> dict[str, int]:
    obj = _check_object(document, "catalog", "Catalog", errors)
    if obj is None:
        return {"materials": 0, "species": 0, "reactions": 0}
    if obj.get("schema_version") != SCHEMA_VERSION:
        errors.append(f"catalog.schema_version: only {SCHEMA_VERSION} is supported")
    if obj.get("document_type") != "omnicore_catalog":
        errors.append("catalog.document_type: expected omnicore_catalog")
    catalog_id = _string(obj.get("catalog_id"), "catalog.catalog_id", errors, pattern=ID_RE)
    if catalog_id is None:
        catalog_id = "invalid"
    if obj.get("dataset_version") != DATASET_VERSION:
        errors.append(f"catalog.dataset_version: expected {DATASET_VERSION}")
    if obj.get("unit_registry_id") != UNIT_REGISTRY_ID:
        errors.append(f"catalog.unit_registry_id: expected {UNIT_REGISTRY_ID}")
    if obj.get("catalog_status") != "schema_only":
        errors.append("catalog.catalog_status: 1.0.4 permits schema_only only")
    if obj.get("runtime_consumption") is not False:
        errors.append("catalog.runtime_consumption: 1.0.4 requires false")

    materials_input = _list(obj.get("materials"), "catalog.materials", errors) or []
    species_input = _list(obj.get("species"), "catalog.species", errors) or []
    reactions_input = _list(obj.get("reactions"), "catalog.reactions", errors) or []
    registry = _legacy_registry(root, errors)
    atomic_symbols = _atomic_symbols(root, errors)

    materials: dict[str, dict[str, Any]] = {}
    species: dict[str, dict[str, Any]] = {}
    all_ids: set[str] = set()
    legacy_owners: set[tuple[str, int]] = set()
    for index, item in enumerate(materials_input):
        record_id, record, mapping = _validate_material(
            item, index, root, catalog_id, registry, errors
        )
        if record_id is not None and record is not None:
            folded = record_id.casefold()
            if folded in all_ids:
                errors.append(f"catalog.materials[{index}].id: duplicate ID {record_id}")
            all_ids.add(folded)
            materials[record_id] = record
        if mapping is not None:
            if mapping in legacy_owners:
                errors.append(f"catalog.materials[{index}]: duplicate Legacy mapping {mapping}")
            legacy_owners.add(mapping)
    for index, item in enumerate(species_input):
        record_id, record, mapping = _validate_species(
            item, index, root, catalog_id, registry, atomic_symbols, errors
        )
        if record_id is not None and record is not None:
            folded = record_id.casefold()
            if folded in all_ids:
                errors.append(f"catalog.species[{index}].id: duplicate ID {record_id}")
            all_ids.add(folded)
            species[record_id] = record
        if mapping is not None:
            if mapping in legacy_owners:
                errors.append(f"catalog.species[{index}]: duplicate Legacy mapping {mapping}")
            legacy_owners.add(mapping)

    for material_id, material in materials.items():
        for term_index, term in enumerate(material.get("composition", [])):
            if not isinstance(term, dict):
                continue
            kind = term.get("component_kind")
            component_id = term.get("component_id")
            if kind == "material" and component_id not in materials:
                errors.append(
                    f"catalog material {material_id} composition[{term_index}]: "
                    f"unknown material {component_id!r}"
                )
            if kind == "species" and component_id not in species:
                errors.append(
                    f"catalog material {material_id} composition[{term_index}]: "
                    f"unknown species {component_id!r}"
                )
    _material_cycles(materials, errors)

    reaction_ids: set[str] = set()
    for index, item in enumerate(reactions_input):
        record_id = _validate_reaction(item, index, root, catalog_id, species, errors)
        if record_id is not None:
            folded = record_id.casefold()
            if folded in all_ids or folded in reaction_ids:
                errors.append(f"catalog.reactions[{index}].id: duplicate ID {record_id}")
            reaction_ids.add(folded)

    for collection_name, records in (
        ("materials", materials_input), ("species", species_input),
        ("reactions", reactions_input),
    ):
        for index, record in enumerate(records):
            if isinstance(record, dict) and record.get("status") not in {
                "identity_only", "legacy_only"
            }:
                errors.append(
                    f"catalog.{collection_name}[{index}]: schema_only catalog cannot contain "
                    f"{record.get('status')!r} record"
                )
    return {
        "materials": len(materials_input),
        "species": len(species_input),
        "reactions": len(reactions_input),
    }


def audit(
    root: Path,
    *,
    catalog_path: Path | None = None,
    units_path: Path | None = None,
    schema_path: Path | None = None,
    unit_schema_path: Path | None = None,
    legacy_map_path: Path | None = None,
    check_legacy_map: bool = True,
) -> tuple[list[str], dict[str, int]]:
    root = root.resolve()
    data_root = root / DATA_ROOT
    catalog_path = catalog_path or data_root / "catalog.json"
    units_path = units_path or data_root / "units.json"
    schema_path = schema_path or data_root / "schema.json"
    unit_schema_path = unit_schema_path or data_root / "unit-registry.schema.json"
    legacy_map_path = legacy_map_path or data_root / "legacy-material-map.json"
    errors: list[str] = []
    loaded: dict[str, Any] = {}
    input_documents = [
        ("catalog", catalog_path), ("units", units_path), ("schema", schema_path),
        ("unit_schema", unit_schema_path),
    ]
    if check_legacy_map:
        input_documents.append(("legacy_map", legacy_map_path))
    for key, path in input_documents:
        try:
            loaded[key] = load_json_strict(path)
        except ValidationFailure as exc:
            errors.append(str(exc))
    if "schema" in loaded and "unit_schema" in loaded:
        _validate_schema(loaded["schema"], loaded["unit_schema"], errors)
    if "units" in loaded:
        _validate_units(loaded["units"], errors)

    for registry_name in ("COMPOUND_REGISTRY.csv", "REACTION_REGISTRY.csv"):
        try:
            read_csv_strict(root / "docs" / registry_name)
        except ValidationFailure as exc:
            errors.append(str(exc))

    legacy_count = 0
    try:
        expected_legacy = build_legacy_map(root)
    except ValidationFailure as exc:
        errors.append(str(exc))
    else:
        legacy_count = len(expected_legacy["mappings"])
        if check_legacy_map and loaded.get("legacy_map") != expected_legacy:
            errors.append(
                "legacy-material-map.json: generated identity map is stale or was modified"
            )

    stats = {"materials": 0, "species": 0, "reactions": 0, "legacy_mappings": legacy_count}
    if "catalog" in loaded:
        stats.update(validate_catalog(loaded["catalog"], root, errors))
    return sorted(set(errors)), stats


def validated_refresh_legacy_map(
    root: Path,
    destination: Path,
    *,
    catalog_path: Path | None = None,
    units_path: Path | None = None,
    schema_path: Path | None = None,
    unit_schema_path: Path | None = None,
) -> list[str]:
    preflight_errors, _ = audit(
        root,
        catalog_path=catalog_path,
        units_path=units_path,
        schema_path=schema_path,
        unit_schema_path=unit_schema_path,
        legacy_map_path=destination,
        check_legacy_map=False,
    )
    if preflight_errors:
        return preflight_errors
    refresh_legacy_map(root.resolve(), destination)
    final_errors, _ = audit(
        root,
        catalog_path=catalog_path,
        units_path=units_path,
        schema_path=schema_path,
        unit_schema_path=unit_schema_path,
        legacy_map_path=destination,
    )
    return final_errors


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--source-root", type=Path, default=Path(__file__).resolve().parents[1]
    )
    parser.add_argument(
        "--refresh-legacy-map", action="store_true",
        help="atomically regenerate the identity-only Legacy map after validation",
    )
    parser.add_argument("--quiet", action="store_true")
    return parser


def main(argv: Sequence[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    root = args.source_root.resolve()
    legacy_path = root / DATA_ROOT / "legacy-material-map.json"
    if args.refresh_legacy_map:
        try:
            refresh_errors = validated_refresh_legacy_map(root, legacy_path)
        except (OSError, ValidationFailure) as exc:
            refresh_errors = [str(exc)]
        if refresh_errors:
            for error in refresh_errors:
                print(f"ERROR: {error}")
            print(
                "OmniCore data refresh failed before output replacement with "
                f"{len(refresh_errors)} error(s)."
            )
            return 1
    errors, stats = audit(root)
    if errors:
        for error in errors:
            print(f"ERROR: {error}")
        print(f"OmniCore data validation failed with {len(errors)} error(s).")
        return 1
    if not args.quiet:
        print(
            "OmniCore data validation passed: "
            f"materials={stats['materials']}, species={stats['species']}, "
            f"reactions={stats['reactions']}, "
            f"legacy_mappings={stats['legacy_mappings']}, runtime_consumption=false"
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
