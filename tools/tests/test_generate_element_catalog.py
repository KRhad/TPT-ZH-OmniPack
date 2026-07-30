from __future__ import annotations

import contextlib
import csv
import importlib.util
import io
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest
from unittest import mock


SCRIPT = Path(__file__).resolve().parents[1] / "generate_element_catalog.py"
SPEC = importlib.util.spec_from_file_location("generate_element_catalog", SCRIPT)
assert SPEC is not None and SPEC.loader is not None
generate_element_catalog = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = generate_element_catalog
SPEC.loader.exec_module(generate_element_catalog)


CATALOG_HEADER = """\
#pragma once

#include <span>
#include <string_view>

struct ElementCatalogRecord
{
    std::string_view identifier;
    std::string_view displayCode;
    std::string_view englishName;
    std::string_view chineseName;
    std::string_view sourceMod;
    std::string_view sourceId;
    int stableId;
    std::string_view menuCategory;
    std::string_view elementState;
    std::string_view defaultEnabled;
    std::string_view duplicateOf;
    std::string_view saveCompatibility;
    std::string_view implementationStatus;
    std::string_view testStatus;
    std::string_view sourceCommit;
    std::string_view englishDescription;
    std::string_view chineseDescription;
    std::string_view license;
    std::string_view notes;
    std::string_view recipeEnglish;
    std::string_view recipeChinese;
    std::string_view productionEnglish;
    std::string_view productionChinese;
    std::string_view useEnglish;
    std::string_view useChinese;
    std::string_view hazardEnglish;
    std::string_view hazardChinese;
};

std::span<ElementCatalogRecord const> GetElementCatalog();
ElementCatalogRecord const *FindElementCatalogByIdentifier(
    std::string_view identifier
);
ElementCatalogRecord const *FindElementCatalogByStableId(int stableId);
"""

EXPECTED_REQUIRED_FIELDS = (
    "identifier",
    "display_code",
    "english_name",
    "chinese_name",
    "source_mod",
    "source_id",
    "menu_category",
    "element_state",
    "default_enabled",
    "save_compatibility",
    "implementation_status",
    "test_status",
    "source_commit",
    "english_description",
    "chinese_description",
    "license",
)


class GenerateElementCatalogTests(unittest.TestCase):
    @staticmethod
    def make_row(
        identifier: str = "DEFAULT_PT_TEST",
        stable_id: str = "200",
        **overrides: str,
    ) -> dict[str, str]:
        row = {field: "" for field in generate_element_catalog.FIELDS}
        row.update(
            {
                "identifier": identifier,
                "display_code": "TEST",
                "english_name": "Test",
                "chinese_name": "测试",
                "source_mod": "example/source",
                "source_id": "200",
                "stable_id": stable_id,
                "menu_category": "SC_SPECIAL",
                "element_state": "solid",
                "default_enabled": "true",
                "save_compatibility": "compatible",
                "implementation_status": "implemented",
                "test_status": "unit-tested",
                "source_commit": "0123456789abcdef0123456789abcdef01234567",
                "english_description": "Test element.",
                "chinese_description": "测试元素。",
                "license": "GPL-3.0-only",
            }
        )
        row.update(overrides)
        return row

    @staticmethod
    def write_registry(
        path: Path,
        rows: list[dict[str, str]],
        *,
        fieldnames: tuple[str, ...] | None = None,
    ) -> None:
        path.parent.mkdir(parents=True, exist_ok=True)
        columns = fieldnames or generate_element_catalog.FIELDS
        with path.open("w", encoding="utf-8", newline="") as registry_file:
            writer = csv.DictWriter(
                registry_file,
                fieldnames=columns,
                extrasaction="ignore",
            )
            writer.writeheader()
            writer.writerows(rows)

    @staticmethod
    def run_main(
        output: Path,
        registry: Path,
        source_root: Path | None = None,
        content: Path | None = None,
    ) -> tuple[int, str, str]:
        stdout = io.StringIO()
        stderr = io.StringIO()
        argv = [str(SCRIPT), str(output), str(registry)]
        if source_root is not None:
            argv.append(str(source_root))
        if content is not None:
            assert source_root is not None
            argv.append(str(content))
        with mock.patch.object(sys, "argv", argv):
            with contextlib.redirect_stdout(stdout):
                with contextlib.redirect_stderr(stderr):
                    result = generate_element_catalog.main()
        return result, stdout.getvalue(), stderr.getvalue()

    def assert_row_rejected_without_output_damage(
        self,
        row: dict[str, str],
        expected_error: str,
    ) -> None:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            registry = root / "registry.csv"
            output = root / "catalog.cpp"
            original_output = b"// known-good generated output\n"
            output.write_bytes(original_output)
            self.write_registry(registry, [row])

            result, _, error = self.run_main(output, registry)
            self.assertEqual(result, 1)
            self.assertIn(expected_error, error)
            self.assertEqual(output.read_bytes(), original_output)

    def assert_compiles(self, source: Path) -> None:
        compiler = (
            shutil.which("c++")
            or shutil.which("g++")
            or shutil.which("clang++")
        )
        if compiler is None:
            self.skipTest("no C++ compiler is available for syntax validation")

        include_root = source.parent / "include"
        header = include_root / "gui" / "elementsearch" / "ElementCatalog.h"
        header.parent.mkdir(parents=True)
        with header.open("w", encoding="utf-8", newline="") as header_file:
            header_file.write(CATALOG_HEADER)
        completed = subprocess.run(
            [
                compiler,
                "-std=c++20",
                "-fsyntax-only",
                "-I",
                str(include_root),
                str(source),
            ],
            check=False,
            capture_output=True,
            text=True,
            encoding="utf-8",
            errors="replace",
        )
        self.assertEqual(
            completed.returncode,
            0,
            msg=completed.stdout + completed.stderr,
        )

    def test_valid_minimal_registry_is_sorted_deterministically_and_compiles(
        self,
    ) -> None:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            high = self.make_row("DEFAULT_PT_HIGH", "201", display_code="HIGH")
            low = self.make_row("DEFAULT_PT_LOW", "17", display_code="LOW")
            first_registry = root / "first.csv"
            second_registry = root / "second.csv"
            first_output = root / "first.cpp"
            second_output = root / "second.cpp"
            self.write_registry(first_registry, [high, low])
            self.write_registry(second_registry, [low, high])

            first_result, _, first_error = self.run_main(
                first_output, first_registry
            )
            second_result, _, second_error = self.run_main(
                second_output, second_registry
            )
            self.assertEqual(first_result, 0, first_error)
            self.assertEqual(second_result, 0, second_error)

            first_text = first_output.read_text(encoding="utf-8")
            self.assertEqual(first_text, second_output.read_text(encoding="utf-8"))
            self.assertIn(
                "constexpr std::array<ElementCatalogRecord, 2> catalog",
                first_text,
            )
            self.assertIn(
                "std::span<ElementCatalogRecord const> GetElementCatalog()",
                first_text,
            )
            self.assertLess(
                first_text.index('"DEFAULT_PT_LOW"'),
                first_text.index('"DEFAULT_PT_HIGH"'),
            )
            self.assert_compiles(first_output)

    def test_content_registry_is_compiled_into_matching_omnipack_record(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            registry = root / "registry.csv"
            content = root / "content.csv"
            output = root / "catalog.cpp"
            self.write_registry(
                registry,
                [self.make_row(identifier="OMNI_PT_TEST")],
            )
            content.write_text(
                "identifier,recipe_en,recipe_zh,production_en,production_zh,use_en,use_zh,hazard_en,hazard_zh\n"
                "OMNI_PT_TEST,Recipe,配方,Production,生产,Use,用途,Hazard,危险\n",
                encoding="utf-8",
                newline="",
            )
            with mock.patch.object(
                generate_element_catalog, "validate_repository", return_value=True
            ):
                result, _, error = self.run_main(output, registry, root, content)
            self.assertEqual(result, 0, error)
            generated = output.read_text(encoding="utf-8")
            self.assertIn('"Recipe", "配方", "Production", "生产", "Use", "用途", "Hazard", "危险"', generated)

    def test_missing_column_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            registry = root / "registry.csv"
            output = root / "catalog.cpp"
            columns = tuple(
                field
                for field in generate_element_catalog.FIELDS
                if field != "notes"
            )
            self.write_registry(
                registry,
                [self.make_row()],
                fieldnames=columns,
            )

            result, _, error = self.run_main(output, registry)
            self.assertEqual(result, 1)
            self.assertIn("missing columns: notes", error)
            self.assertFalse(output.exists())

    def test_duplicate_header_and_bad_row_width_are_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            output = root / "catalog.cpp"
            original_output = b"// known-good generated output\n"
            output.write_bytes(original_output)

            duplicate_header = root / "duplicate-header.csv"
            with duplicate_header.open(
                "w", encoding="utf-8", newline=""
            ) as registry_file:
                registry_file.write(
                    ",".join(generate_element_catalog.FIELDS)
                    + ",identifier\n"
                )
            result, _, error = self.run_main(output, duplicate_header)
            self.assertEqual(result, 1)
            self.assertIn("duplicate columns: identifier", error)
            self.assertEqual(output.read_bytes(), original_output)

            too_wide = root / "too-wide.csv"
            with too_wide.open("w", encoding="utf-8", newline="") as registry_file:
                writer = csv.writer(registry_file)
                writer.writerow(generate_element_catalog.FIELDS)
                row = self.make_row()
                writer.writerow(
                    [row[field] for field in generate_element_catalog.FIELDS]
                    + ["unexpected"]
                )
            result, _, error = self.run_main(output, too_wide)
            self.assertEqual(result, 1)
            self.assertIn("CSV row width does not match", error)
            self.assertEqual(output.read_bytes(), original_output)

            too_narrow = root / "too-narrow.csv"
            with too_narrow.open(
                "w", encoding="utf-8", newline=""
            ) as registry_file:
                writer = csv.writer(registry_file)
                writer.writerow(generate_element_catalog.FIELDS)
                row = self.make_row()
                writer.writerow(
                    [row[field] for field in generate_element_catalog.FIELDS[:-1]]
                )
            result, _, error = self.run_main(output, too_narrow)
            self.assertEqual(result, 1)
            self.assertIn("CSV row width does not match", error)
            self.assertEqual(output.read_bytes(), original_output)

    def test_optional_repository_validation_is_fail_closed(self) -> None:
        source_root = Path("source root")
        registry = Path("registry.csv")
        completed = subprocess.CompletedProcess(
            args=[],
            returncode=1,
            stdout="",
            stderr="semantic registry failure\n",
        )
        with mock.patch.object(
            generate_element_catalog.subprocess,
            "run",
            return_value=completed,
        ) as run:
            stderr = io.StringIO()
            with contextlib.redirect_stderr(stderr):
                valid = generate_element_catalog.validate_repository(
                    source_root,
                    registry,
                )
        self.assertFalse(valid)
        self.assertIn("repository registry validation failed", stderr.getvalue())
        self.assertIn("semantic registry failure", stderr.getvalue())
        command = run.call_args.args[0]
        self.assertIn("--root", command)
        self.assertIn(str(source_root), command)
        self.assertIn("--registry", command)
        self.assertIn(str(registry), command)

    def test_noncanonical_stable_ids_are_rejected_without_overwrite(self) -> None:
        for stable_id in (
            "not-an-integer",
            "+1",
            "01",
            " 1",
            "1 ",
            "   ",
            "",
        ):
            with self.subTest(stable_id=stable_id):
                self.assert_row_rejected_without_output_damage(
                    self.make_row(stable_id=stable_id),
                    "element catalog:2: invalid stable_id",
                )

    def test_out_of_range_stable_ids_are_rejected_without_overwrite(self) -> None:
        for stable_id, expected_error in (
            ("-1", "invalid stable_id"),
            ("512", "outside the supported range 0..511"),
            ("999", "outside the supported range 0..511"),
        ):
            with self.subTest(stable_id=stable_id):
                self.assert_row_rejected_without_output_damage(
                    self.make_row(stable_id=stable_id),
                    expected_error,
                )

    def test_empty_required_fields_are_rejected_without_overwrite(self) -> None:
        for field in EXPECTED_REQUIRED_FIELDS:
            with self.subTest(field=field):
                self.assert_row_rejected_without_output_damage(
                    self.make_row(**{field: "   "}),
                    f"empty required field '{field}'",
                )

    def test_forbidden_control_characters_are_rejected_without_overwrite(
        self,
    ) -> None:
        for field in generate_element_catalog.FIELDS:
            with self.subTest(field=field, codepoint=0):
                self.assert_row_rejected_without_output_damage(
                    self.make_row(**{field: "before\x00after"}),
                    f"field '{field}' contains forbidden control character U+0000",
                )
        for codepoint in (0x00, 0x01, 0x08, 0x0B, 0x0C, 0x0E, 0x1F, 0x7F):
            with self.subTest(codepoint=codepoint):
                self.assert_row_rejected_without_output_damage(
                    self.make_row(notes=f"before{chr(codepoint)}after"),
                    f"forbidden control character U+{codepoint:04X}",
                )

    def test_duplicate_stable_id_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            registry = root / "registry.csv"
            output = root / "catalog.cpp"
            self.write_registry(
                registry,
                [
                    self.make_row("DEFAULT_PT_FIRST", "200"),
                    self.make_row("DEFAULT_PT_SECOND", "200"),
                ],
            )

            result, _, error = self.run_main(output, registry)
            self.assertEqual(result, 1)
            self.assertIn("element catalog:3: duplicate stable_id 200", error)
            self.assertFalse(output.exists())

    def test_identifier_duplicate_is_case_insensitive(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            registry = root / "registry.csv"
            output = root / "catalog.cpp"
            self.write_registry(
                registry,
                [
                    self.make_row("DEFAULT_PT_CASE", "200"),
                    self.make_row("default_pt_case", "201"),
                ],
            )

            result, _, error = self.run_main(output, registry)
            self.assertEqual(result, 1)
            self.assertIn(
                "element catalog:3: duplicate identifier default_pt_case",
                error,
            )
            self.assertFalse(output.exists())

    def test_cpp_strings_escape_quotes_backslashes_and_newlines(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            registry = root / "registry.csv"
            output = root / "catalog.cpp"
            description = 'He said "go" at C:\\sandbox\nnext line'
            self.write_registry(
                registry,
                [self.make_row(english_description=description)],
            )

            result, _, error = self.run_main(output, registry)
            self.assertEqual(result, 0, error)
            generated = output.read_text(encoding="utf-8")
            self.assertIn(
                r'"He said \"go\" at C:\\sandbox\nnext line"',
                generated,
            )
            self.assertNotIn('He said "go" at C:\\sandbox\nnext line', generated)
            self.assert_compiles(output)

    def test_success_writes_only_the_requested_output(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            registry = root / "registry.csv"
            output = root / "chosen-name.cpp"
            self.write_registry(registry, [self.make_row()])
            files_before = {
                path.relative_to(root)
                for path in root.rglob("*")
                if path.is_file()
            }

            with mock.patch.object(
                Path,
                "write_text",
                side_effect=AssertionError("generator must use Path.open"),
            ):
                result, _, error = self.run_main(output, registry)
            self.assertEqual(result, 0, error)
            files_after = {
                path.relative_to(root)
                for path in root.rglob("*")
                if path.is_file()
            }
            self.assertEqual(
                files_after - files_before,
                {output.relative_to(root)},
            )

    def test_validation_error_neither_creates_nor_overwrites_output(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            registry = root / "registry.csv"
            absent_output = root / "absent.cpp"
            self.write_registry(
                registry,
                [self.make_row(stable_id="invalid")],
            )

            result, _, error = self.run_main(absent_output, registry)
            self.assertEqual(result, 1)
            self.assertIn("invalid stable_id", error)
            self.assertFalse(absent_output.exists())

            successful_output = root / "successful.cpp"
            self.write_registry(registry, [self.make_row(stable_id="200")])
            result, _, error = self.run_main(successful_output, registry)
            self.assertEqual(result, 0, error)
            successful_bytes = successful_output.read_bytes()

            self.write_registry(
                registry,
                [self.make_row(stable_id="invalid")],
            )
            result, _, error = self.run_main(successful_output, registry)
            self.assertEqual(result, 1)
            self.assertIn("invalid stable_id", error)
            self.assertEqual(successful_output.read_bytes(), successful_bytes)


if __name__ == "__main__":
    unittest.main()
