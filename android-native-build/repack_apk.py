#!/usr/bin/env python3
"""Replace the native Powder engine in an APK while preserving its Android wrapper."""

from __future__ import annotations

import argparse
import copy
from pathlib import Path
import zipfile


ABIS = ("armeabi-v7a", "arm64-v8a", "x86", "x86_64")
SIGNATURE_SUFFIXES = (".SF", ".RSA", ".DSA", ".EC")


def is_old_signature(name: str) -> bool:
    upper = name.upper()
    if upper == "META-INF/MANIFEST.MF":
        return True
    return upper.startswith("META-INF/") and upper.endswith(SIGNATURE_SUFFIXES)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("input_apk", type=Path)
    parser.add_argument("native_libs", type=Path)
    parser.add_argument("output_apk", type=Path)
    args = parser.parse_args()

    replacements = {
        f"lib/{abi}/libapplication.so": args.native_libs / abi / "libapplication.so"
        for abi in ABIS
    }
    missing = [str(path) for path in replacements.values() if not path.is_file()]
    if missing:
        raise SystemExit("Missing native libraries:\n" + "\n".join(missing))

    args.output_apk.parent.mkdir(parents=True, exist_ok=True)
    replaced: set[str] = set()
    with zipfile.ZipFile(args.input_apk, "r") as source:
        with zipfile.ZipFile(
            args.output_apk,
            "w",
            allowZip64=True,
            strict_timestamps=False,
        ) as target:
            target.comment = source.comment
            for info in source.infolist():
                if is_old_signature(info.filename):
                    continue

                cloned = copy.copy(info)
                # zipalign will add fresh alignment metadata after this archive is written.
                cloned.extra = b""
                if info.filename in replacements:
                    data = replacements[info.filename].read_bytes()
                    replaced.add(info.filename)
                else:
                    data = source.read(info)

                target.writestr(
                    cloned,
                    data,
                    compress_type=info.compress_type,
                    compresslevel=9 if info.compress_type == zipfile.ZIP_DEFLATED else None,
                )

    expected = set(replacements)
    if replaced != expected:
        missing_entries = sorted(expected - replaced)
        args.output_apk.unlink(missing_ok=True)
        raise SystemExit("APK lacks expected entries:\n" + "\n".join(missing_entries))

    print(f"Replaced {len(replaced)} native libraries: {args.output_apk}")


if __name__ == "__main__":
    main()
