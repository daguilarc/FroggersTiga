# Desktop packaging

Single-file distributables for macOS (DMG) and Windows (Inno Setup installer) from a Release CMake build.

## Version source of truth

`desktop/CMakeLists.txt` declares:

```cmake
project(FroggersTigaDesktop VERSION x.y.z)
```

All packaging scripts read this version. GitHub Release tags **must** match:

```text
desktop-vX.Y.Z   ← tag prefix
     └── same X.Y.Z as CMake VERSION
```

CI fails if tag semver and CMake `VERSION` diverge.

## Prerequisites

### macOS

- Xcode command-line tools (`cmake`, `hdiutil`)
- Release build (see below)

### Windows

- Visual Studio 2022 build tools (CMake generator)
- [Inno Setup 6](https://jrsoftware.org/isinfo.php) (`ISCC.exe` on PATH or default install location)
- Release build (see below)

## Build Release

From repository root:

```sh
cd desktop
cmake -B build
cmake --build build --config Release
```

Artefact locations:

| Platform | Release output |
|----------|----------------|
| macOS | `desktop/build/FroggersTigaDesktop_artefacts/Release/FroggersTiga.app` |
| Windows | `desktop/build/FroggersTigaDesktop_artefacts/Release/FroggersTiga.exe` (+ DLLs in same folder) |

## Package locally

### macOS → DMG

```sh
./desktop/scripts/package-macos.sh
```

Output: `desktop/dist/FroggersTiga-<version>-macOS.dmg`

Mount the DMG, drag **FroggersTiga.app** to **Applications**, then open the app.

### Windows → Setup.exe

```powershell
./desktop/scripts/package-windows.ps1
```

Output: `desktop/dist/FroggersTiga-<version>-Windows-Setup.exe`

## Unsigned builds

These packages are **not** code-signed.

| Platform | User experience |
|----------|-----------------|
| macOS | Gatekeeper may block first launch. Right-click → **Open**, or run `xattr -cr /Applications/FroggersTiga.app` for local dev. |
| Windows | SmartScreen may warn on first run. User chooses **Run anyway**. |

Signing and notarization are planned as a follow-up change.

## GitHub Releases (CI)

Push a matching tag:

```sh
git tag desktop-v1.0.0
git push origin desktop-v1.0.0
```

Workflow `.github/workflows/desktop-release.yml` builds on `macos-14` and `windows-latest`, then attaches:

- `FroggersTiga-<version>-macOS.dmg`
- `FroggersTiga-<version>-Windows-Setup.exe`

to the GitHub Release for that tag.

## Scripts

| Script | Purpose |
|--------|---------|
| `read-version.sh` / `read-version.ps1` | Print CMake `VERSION` |
| `verify-tag-version.sh` | CI gate: tag semver = CMake VERSION |
| `package-macos.sh` | Stage `.app` + Applications link → DMG |
| `package-windows.ps1` | Run ISCC with `/DMyAppVersion` and `/DReleaseDir` |
