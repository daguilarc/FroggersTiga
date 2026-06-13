## Why

The desktop simulator builds to a local `.app` (macOS) or folder with `.exe` (Windows) under `desktop/build/`, but there is no repeatable way to produce **single-file download artifacts** for GitHub Releases or direct distribution. Users cannot install the app without cloning the repo and running CMake.

## What Changes

- Add **packaging scripts** under `desktop/scripts/` that wrap the Release build into distributable artifacts:
  - **macOS:** `FroggersTiga-<version>-macOS.dmg` (compressed DMG containing `FroggersTiga.app` + Applications symlink)
  - **Windows:** `FroggersTiga-<version>-Windows-Setup.exe` (Inno Setup installer bundling the Release artefact folder)
- Add **Inno Setup script** (`desktop/installer/FroggersTiga.iss`) for Windows single-file installer.
- Add **GitHub Actions workflow** (`.github/workflows/desktop-release.yml`) with three jobs (macOS build, Windows build, release aggregation) on `desktop-v*` tags.
- Document local packaging commands in `desktop/PACKAGING.md` and a short section in `README.md`.

## Capabilities

### New Capabilities

- `desktop-release-packages`: Reproducible single-file macOS DMG and Windows installer from Release CMake build; CI release workflow.

### Modified Capabilities

- (none)

## Impact

- `desktop/scripts/package-macos.sh` — DMG creation
- `desktop/scripts/package-windows.ps1` — invokes ISCC when present; documents manual path
- `desktop/scripts/read-version.sh` / `read-version.ps1` — parse CMake `VERSION` (sole source of truth)
- `desktop/installer/FroggersTiga.iss` — Inno Setup definition (version via ISCC `/D`)
- `.github/workflows/desktop-release.yml` — tag-triggered three-job build + GitHub Release upload
- `desktop/CMakeLists.txt` — `BUNDLE_ID` for packaging metadata
- `README.md`, `desktop/PACKAGING.md` — operator docs
