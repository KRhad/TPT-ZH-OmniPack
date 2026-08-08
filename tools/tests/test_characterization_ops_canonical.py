from __future__ import annotations

import bz2
import hashlib
import importlib.util
from pathlib import Path
import struct
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / "tools" / "characterization_ops_canonical.py"
SPEC = importlib.util.spec_from_file_location(
    "characterization_ops_canonical", SCRIPT
)
assert SPEC and SPEC.loader
canonicalizer = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(canonicalizer)
inspect_ops = canonicalizer.inspect_ops


def make_ops(stamp: str, date: int) -> bytes:
    payload = (
        b"prefix"
        + b"\x12date\x00"
        + struct.pack("<q", date)
        + b"\x02name\x00"
        + b"stamps\\"
        + stamp.encode("ascii")
        + b".stm\x00suffix"
    )
    return b"OPS1" + b"\x00\x00\x00\x00" + struct.pack("<I", len(payload)) + bz2.compress(payload)


class CharacterizationOpsCanonicalTest(unittest.TestCase):
    def test_volatile_stamp_metadata_has_stable_canonical_hash(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            first_path = Path(temporary) / "first.stm"
            second_path = Path(temporary) / "second.stm"
            first_path.write_bytes(make_ops("0123456789", 100))
            second_path.write_bytes(make_ops("ABCDEF0123", 200))

            first = inspect_ops(first_path)
            second = inspect_ops(second_path)

        self.assertNotEqual(first["raw_sha256"], second["raw_sha256"])
        self.assertNotEqual(first["payload_sha256"], second["payload_sha256"])
        self.assertEqual(
            first["canonical_payload_sha256"],
            second["canonical_payload_sha256"],
        )
        self.assertEqual(first["volatile_stamp_name_count"], 1)
        self.assertEqual(first["volatile_author_date_count"], 1)

    def test_declared_payload_length_is_checked(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "bad.stm"
            raw = bytearray(make_ops("0123456789", 100))
            struct.pack_into("<I", raw, 8, 1)
            path.write_bytes(raw)
            with self.assertRaisesRegex(ValueError, "payload length mismatch"):
                inspect_ops(path)

    def test_raw_hash_is_independently_reported(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "one.stm"
            raw = make_ops("0123456789", 100)
            path.write_bytes(raw)
            result = inspect_ops(path)
        self.assertEqual(result["raw_sha256"], hashlib.sha256(raw).hexdigest().upper())


if __name__ == "__main__":
    unittest.main()
