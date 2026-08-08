from __future__ import annotations

import argparse
import bz2
import hashlib
import json
from pathlib import Path
import re
import struct


STAMP_NAME = re.compile(rb"stamps[\\/][0-9A-Fa-f]{10}\.stm")
AUTHOR_DATE = re.compile(rb"\x12date\x00.{8}(?=\x02name\x00)", re.DOTALL)
CANONICAL_STAMP_NAME = b"stamps\\0000000000.stm"
CANONICAL_AUTHOR_DATE = b"\x12date\x00" + (b"\x00" * 8)


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest().upper()


def inspect_ops(path: Path) -> dict[str, object]:
    raw = path.read_bytes()
    if len(raw) <= 15 or raw[:4] != b"OPS1":
        raise ValueError(f"not an OPS1 container: {path}")
    declared_payload_length = struct.unpack_from("<I", raw, 8)[0]
    if raw[12:15] != b"BZh":
        raise ValueError(f"OPS payload is not bzip2: {path}")
    payload = bz2.decompress(raw[12:])
    if len(payload) != declared_payload_length:
        raise ValueError(
            "OPS payload length mismatch: "
            f"declared={declared_payload_length}, actual={len(payload)}"
        )

    stamp_names = list(STAMP_NAME.finditer(payload))
    author_dates = list(AUTHOR_DATE.finditer(payload))
    if len(stamp_names) != 1:
        raise ValueError(
            f"expected one volatile stamp author name, found {len(stamp_names)}"
        )
    if len(author_dates) != 1:
        raise ValueError(
            f"expected one volatile stamp author date, found {len(author_dates)}"
        )

    canonical = STAMP_NAME.sub(lambda _match: CANONICAL_STAMP_NAME, payload)
    canonical = AUTHOR_DATE.sub(lambda _match: CANONICAL_AUTHOR_DATE, canonical)
    if len(canonical) != len(payload):
        raise ValueError("canonicalization changed OPS payload length")

    return {
        "schema_version": 1,
        "path": str(path.resolve()),
        "raw_length_bytes": len(raw),
        "raw_sha256": sha256(raw),
        "payload_length_bytes": len(payload),
        "payload_sha256": sha256(payload),
        "canonical_payload_sha256": sha256(canonical),
        "volatile_stamp_name_count": len(stamp_names),
        "volatile_author_date_count": len(author_dates),
        "canonicalization": "zero authors.date and normalize authors.name stamp ID",
    }


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Inspect and canonicalize volatile metadata in an OPS stamp"
    )
    parser.add_argument("ops", type=Path)
    parser.add_argument("--json", action="store_true")
    args = parser.parse_args()
    result = inspect_ops(args.ops)
    if args.json:
        print(json.dumps(result, sort_keys=True, separators=(",", ":")))
    else:
        for key, value in result.items():
            print(f"{key}={value}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
