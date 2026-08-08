# GitHub update channel

Release builds request `Startup.json` from the public `public-source` branch. The manifest selects one signed or unsigned release asset by the compiled `IDENT_PLATFORM`, records its exact byte size and SHA-256, and never contains credentials.

- `WIN64` uses the existing `BuTT` + bzip2 executable update format. The game verifies the downloaded wrapper before replacing the portable EXE.
- `ANDROIDARM64` uses a signed APK. The game verifies it, stages it through Android `PackageInstaller`, and leaves the final installation decision to Android and the user.

Generate publishable channel files with `tools/package_github_update.py`. The public 1.0.0 snapshot uses an internal update build number in addition to its player-visible version: build 0 can move once to build 1, while build 1 does not repeatedly offer the same assets. Future channel changes must update the internal build/version and both platform records together.

This channel does not make the project release-ready. Windows Authenticode, a production Android signing key, tag/Release creation, long-run gates, and human GUI/DPI acceptance remain separate requirements.
