#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
from pathlib import Path


def main() -> int:
    parser = argparse.ArgumentParser(description="Audit the OmniPack Android direct port")
    parser.add_argument("--source-root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--json", action="store_true")
    args = parser.parse_args()
    root = args.source_root.resolve()

    manifest = (root / "android/AndroidManifest.template.xml").read_text(encoding="utf-8")
    activity = (root / "android/PowderActivity.template.java").read_text(encoding="utf-8")
    packer = (root / "android/build-apk.py").read_text(encoding="utf-8")
    aligner = (root / "android/align-apk.py").read_text(encoding="utf-8")
    root_meson = (root / "meson.build").read_text(encoding="utf-8")
    src_meson = (root / "src/meson.build").read_text(encoding="utf-8")
    resources_meson = (root / "android/res/meson.build").read_text(encoding="utf-8")
    zh_label = (root / "android/res/values-zh-rCN/strings.xml").read_text(encoding="utf-8")

    forbidden_storage_permissions = (
        "android.permission.WRITE_EXTERNAL_STORAGE",
        "android.permission.READ_EXTERNAL_STORAGE",
        "android.permission.MANAGE_EXTERNAL_STORAGE",
    )
    checks = {
        "stable_app_id": 'package="@APPID@"' in manifest and "org.tptzh.omnipack" in (root / "meson_options.txt").read_text(encoding="utf-8"),
        "version_code_is_positive_mapping": "@ANDROID_VERSION_CODE@" in manifest and "android_version_code" in src_meson,
        "minimum_sdk_21": 'android:minSdkVersion="21"' in manifest,
        "target_sdk_33": 'android:targetSdkVersion="33"' in manifest,
        "landscape_direct_port": 'android:screenOrientation="landscape"' in manifest,
        "app_private_storage": "getExternalFilesDir(null)" in activity and "getFilesDir()" in activity,
        "no_legacy_storage_permission": not any(value in manifest for value in forbidden_storage_permissions),
        "cleartext_disabled": 'android:usesCleartextTraffic="false"' in manifest,
        "api21_base64_compatible": "android.util.Base64" in activity and "java.util.Base64" not in activity,
        "touch_ui_default": "default_touch_ui = true" in src_meson,
        "windows_resource_paths_normalized": "resource_paths" in packer and "os.path.abspath" in packer,
        "portable_apk_entry_separator": "sha_apk_path" in packer and "'/'.join" in packer,
        "zipalign_16k": "'-P', '16'" in aligner,
        "elf_page_alignment_16k": "max-page-size=16384" in root_meson,
        "android_platform_identifier": "ANDROIDARM64" in root_meson,
        "zh_cn_launcher_label": "values-zh-rCN" in resources_meson and "万象沙盘" in zh_label,
        "build_script_present": (root / "tools/build_android.ps1").is_file(),
    }
    passed = all(checks.values())
    report = {"android_port_audit_pass": passed, "checks": checks}
    if args.json:
        print(json.dumps(report, ensure_ascii=False, indent=2))
    else:
        for key, value in checks.items():
            print(f"{key}={str(value).lower()}")
        print(f"android_port_audit_pass={str(passed).lower()}")
    return 0 if passed else 1


if __name__ == "__main__":
    raise SystemExit(main())
