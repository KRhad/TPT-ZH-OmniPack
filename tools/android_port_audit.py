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
    android_meson = (root / "android/meson.build").read_text(encoding="utf-8")
    android_platform = (root / "src/common/platform/Android.cpp").read_text(encoding="utf-8")
    posix_process = (root / "src/common/platform/PosixProc.cpp").read_text(encoding="utf-8")
    root_meson = (root / "meson.build").read_text(encoding="utf-8")
    src_meson = (root / "src/meson.build").read_text(encoding="utf-8")
    game_view = (root / "src/gui/game/GameView.cpp").read_text(encoding="utf-8")
    resources_meson = (root / "android/res/meson.build").read_text(encoding="utf-8")
    zh_label = (root / "android/res/values-zh-rCN/strings.xml").read_text(encoding="utf-8")
    build_script = (root / "tools/build_android.ps1").read_text(encoding="utf-8")

    forbidden_storage_permissions = (
        "android.permission.WRITE_EXTERNAL_STORAGE",
        "android.permission.READ_EXTERNAL_STORAGE",
        "android.permission.MANAGE_EXTERNAL_STORAGE",
    )
    checks = {
        "stable_app_id": 'package="@APPID@"' in manifest and "org.tptzh.omnipack" in (root / "meson_options.txt").read_text(encoding="utf-8"),
        "version_code_is_positive_mapping": "@ANDROID_VERSION_CODE@" in manifest and "android_version_code" in src_meson,
        "marketing_version_independent_of_update_build": "@DISPLAY_VERSION_PATCH@" in manifest
        and "@BUILD_NUM@" not in manifest,
        "minimum_sdk_21": 'android:minSdkVersion="21"' in manifest,
        "target_sdk_33": 'android:targetSdkVersion="33"' in manifest,
        "landscape_direct_port": 'android:screenOrientation="landscape"' in manifest,
        "app_private_storage": "getExternalFilesDir(null)" in activity and "getFilesDir()" in activity,
        "no_legacy_storage_permission": not any(value in manifest for value in forbidden_storage_permissions),
        "cleartext_disabled": 'android:usesCleartextTraffic="false"' in manifest,
        "api21_base64_compatible": "android.util.Base64" in activity and "java.util.Base64" not in activity,
        "android_language_restart_bridge": all(
            token in activity
            for token in (
                "restartApplication",
                "Intent.makeRestartActivityTask",
                "PendingIntent.FLAG_IMMUTABLE",
                "finishAffinity()",
            )
        )
        and 'CallActivityVoidFunc("restartApplication")' in android_platform,
        "android_open_uri_bridge": "public void openUri" in activity
        and 'CallActivityVoidFunc("openUri", uri)' in android_platform,
        "android_verified_update_installer": all(
            token in activity
            for token in (
                "PackageInstaller.SessionParams",
                "STATUS_PENDING_USER_ACTION",
                "canRequestPackageInstalls",
                "ACTION_MANAGE_UNKNOWN_APP_SOURCES",
            )
        )
        and "android.permission.REQUEST_INSTALL_PACKAGES" in src_meson
        and 'CallActivityVoidFunc("installApkUpdate", filename)' in android_platform,
        "posix_self_exec_disabled_on_android": "#ifndef __ANDROID__\nvoid DoRestart()" in posix_process,
        "touch_ui_default": "default_touch_ui = true" in src_meson,
        "touch_local_save_primary_action": "CtrlBehaviour() || ui::Engine::Ref().TouchUI" in game_view,
        "touch_online_browser_long_press": "searchButton->SetLongPressCallback" in game_view
        and 'if (ui::Engine::Ref().TouchUI)\n\t\t\tc->OpenSearch("")' in game_view,
        "windows_resource_paths_normalized": "resource_paths" in packer and "os.path.abspath" in packer,
        "portable_apk_entry_separator": "sha_apk_path" in packer and "'/'.join" in packer,
        "zipalign_16k": "'-P', '16'" in aligner,
        "distribution_library_stripped": "strip-android-library" in android_meson and "--strip-unneeded" in android_meson,
        "elf_page_alignment_16k": "max-page-size=16384" in root_meson,
        "android_platform_identifier": "ANDROIDARM64" in root_meson,
        "zh_cn_launcher_label": "values-zh-rCN" in resources_meson and "万象沙盘" in zh_label,
        "public_build_excludes_tests": "-Dbuild_tests=false" in build_script,
        "public_build_enables_updates": "-Dignore_updates=false" in build_script
        and "raw.githubusercontent.com/KRhad/TPT-ZH-OmniPack/public-source" in build_script,
        "public_build_has_update_revision": "-Dupdate_build=$UpdateBuild" in build_script,
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
