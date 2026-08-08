#!/usr/bin/env python3
"""Audit the GitHub-backed desktop/Android update channel without publishing."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path, PurePosixPath
import re
from typing import Sequence


PROJECT_URL = "https://github.com/KRhad/TPT-ZH-OmniPack"
UPDATE_SERVER = "https://raw.githubusercontent.com/KRhad/TPT-ZH-OmniPack/public-source"
SHA256_RE = re.compile(r"^[0-9A-F]{64}$")


def file_sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest().upper()


def validate_manifest(path: Path, asset_root: Path | None = None) -> dict[str, bool]:
    document = json.loads(path.read_text(encoding="utf-8"))
    stable = document["Updates"]["Stable"]
    platforms = stable["Platforms"]
    checks: dict[str, bool] = {
        "manifest_schema": document.get("SchemaVersion") == 1,
        "manifest_product": document.get("Product") == "TPT-ZH-OmniPack",
        "manifest_project_url": document.get("ProjectURL") == PROJECT_URL,
        "manifest_release_conservative": document.get("ReleaseReady") is False,
        "manifest_no_public_tests": document.get("PublicTestsIncluded") is False,
        "manifest_platforms": set(platforms) == {"WIN64", "ANDROIDARM64"},
    }
    expected_formats = {"WIN64": "butt-executable", "ANDROIDARM64": "android-apk"}
    for platform, expected_format in expected_formats.items():
        asset = platforms[platform]
        prefix = platform.lower()
        relative = asset.get("File", "")
        checks[f"{prefix}_https_relative_asset"] = (
            isinstance(relative, str)
            and relative.startswith("/updates/")
            and ".." not in PurePosixPath(relative).parts
        )
        checks[f"{prefix}_format"] = asset.get("Format") == expected_format
        checks[f"{prefix}_size"] = isinstance(asset.get("Size"), int) and asset["Size"] > 0
        checks[f"{prefix}_sha256"] = bool(SHA256_RE.fullmatch(asset.get("Sha256", "")))
        if asset_root is not None and checks[f"{prefix}_https_relative_asset"]:
            asset_path = asset_root / relative.lstrip("/")
            checks[f"{prefix}_asset_present"] = asset_path.is_file()
            checks[f"{prefix}_asset_size_matches"] = (
                asset_path.is_file() and asset_path.stat().st_size == asset["Size"]
            )
            checks[f"{prefix}_asset_hash_matches"] = (
                asset_path.is_file() and file_sha256(asset_path) == asset["Sha256"]
            )
    return checks


def audit_source(root: Path) -> dict[str, bool]:
    meson_options = (root / "meson_options.txt").read_text(encoding="utf-8")
    config = (root / "src/Config.template.h").read_text(encoding="utf-8")
    request_header = (root / "src/client/http/Request.h").read_text(encoding="utf-8")
    request_impl = (root / "src/client/http/Request.cpp").read_text(encoding="utf-8")
    request_manager = (root / "src/client/http/requestmanager/RequestManager.h").read_text(encoding="utf-8")
    libcurl = (root / "src/client/http/requestmanager/Libcurl.cpp").read_text(encoding="utf-8")
    startup = (root / "src/client/http/StartupRequest.cpp").read_text(encoding="utf-8")
    updater = (root / "src/gui/update/UpdateActivity.cpp").read_text(encoding="utf-8")
    platform = (root / "src/common/platform/Platform.h").read_text(encoding="utf-8")
    android = (root / "src/common/platform/Android.cpp").read_text(encoding="utf-8")
    activity = (root / "android/PowderActivity.template.java").read_text(encoding="utf-8")
    manifest = (root / "android/AndroidManifest.template.xml").read_text(encoding="utf-8")
    windows = (root / "src/common/platform/Windows.cpp").read_text(encoding="utf-8")
    root_meson = (root / "meson.build").read_text(encoding="utf-8")
    intro = (root / "src/gui/game/IntroText.h").read_text(encoding="utf-8")
    en = json.loads((root / "src/lang/en-US.json").read_text(encoding="utf-8"))
    zh = json.loads((root / "src/lang/zh-CN.json").read_text(encoding="utf-8"))
    package_tool = root / "tools/package_github_update.py"
    return {
        "github_update_server_default": UPDATE_SERVER in meson_options,
        "automatic_checks_enabled_default": "'ignore_updates'" in meson_options
        and "value: false" in meson_options[meson_options.index("'ignore_updates'") :][:180],
        "project_url_configured": PROJECT_URL in meson_options and "PROJECT_URL" in config,
        "platform_manifest_routing": 'info.isMember("Platforms")' in startup
        and "IDENT_PLATFORM" in startup,
        "manifest_sha256_required": "invalid SHA-256" in startup and 'asset->isMember("Sha256")' in startup,
        "manifest_size_required": "invalid package size" in startup and 'asset->isMember("Size")' in startup,
        "request_http1_1_flag": "void ForceHttp1_1();" in request_header
        and "handle->forceHttp1_1 = true;" in request_impl
        and "bool forceHttp1_1 = false;" in request_manager,
        "libcurl_http1_1_enforced": "CURLOPT_HTTP_VERSION" in libcurl
        and "CURL_HTTP_VERSION_1_1" in libcurl,
        "github_startup_http1_1": "if (alternate)" in startup and "ForceHttp1_1();" in startup,
        "update_download_http1_1": "request->ForceHttp1_1();" in updater,
        "update_download_bounded_retry": "constexpr int updateDownloadAttempts = 2;" in updater
        and "attempt < updateDownloadAttempts" in updater
        and "data = ByteString{};" in updater,
        "download_sha256_verified": "Sha256Hex" in updater and "SHA-256 mismatch" in updater,
        "download_size_verified": "Package size mismatch" in updater,
        "windows_existing_updater_used": "Platform::UpdateStart(res)" in updater,
        "windows_single_exe_runtime": "host_platform == 'windows' and is_static" in root_meson
        and "project_link_args += [ '-static' ]" in root_meson,
        "windows_rollback_on_failure": windows.count("RenameFile(updName, exeName, true);") >= 2,
        "android_platform_install_api": "CanInstallUpdatePackage" in platform
        and 'CallActivityVoidFunc("installApkUpdate", filename)' in android,
        "android_uri_open_bridge": 'CallActivityVoidFunc("openUri", uri)' in android
        and "public void openUri" in activity,
        "android_package_installer": "PackageInstaller.SessionParams" in activity
        and "session.commit" in activity,
        "android_system_confirmation": "STATUS_PENDING_USER_ACTION" in activity
        and "Intent.EXTRA_INTENT" in activity,
        "android_unknown_sources_gate": "canRequestPackageInstalls" in activity
        and "ACTION_MANAGE_UNKNOWN_APP_SOURCES" in activity,
        "android_install_permission": "android.permission.REQUEST_INSTALL_PACKAGES" in manifest
        or "android.permission.REQUEST_INSTALL_PACKAGES" in (root / "src/meson.build").read_text(encoding="utf-8"),
        "startup_github_visible": "intro.project.github" in intro
        and PROJECT_URL in meson_options,
        "startup_github_localized": "intro.project.github" in en and "intro.project.github" in zh,
        "package_generator_present": package_tool.is_file(),
    }


def main(argv: Sequence[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--manifest", type=Path)
    parser.add_argument("--asset-root", type=Path)
    parser.add_argument("--json", action="store_true")
    args = parser.parse_args(argv)
    try:
        checks = audit_source(args.source_root.resolve())
        if args.manifest:
            checks.update(
                validate_manifest(
                    args.manifest.resolve(),
                    args.asset_root.resolve() if args.asset_root else None,
                )
            )
    except (OSError, KeyError, TypeError, ValueError, json.JSONDecodeError) as error:
        print(f"github-update-audit: ERROR {error}")
        return 1
    passed = all(checks.values())
    if args.json:
        print(json.dumps({"github_update_audit_pass": passed, "checks": checks}, indent=2))
    else:
        for name, value in checks.items():
            print(f"{name}={str(value).lower()}")
        print(f"github_update_audit_pass={str(passed).lower()}")
    return 0 if passed else 1


if __name__ == "__main__":
    raise SystemExit(main())
