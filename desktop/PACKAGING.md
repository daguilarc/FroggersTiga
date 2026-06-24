# Desktop packaging

Single-file distributables for macOS (DMG) and Windows (Inno Setup installer) from a Release CMake build.

## Version source of truth

`desktop/CMakeLists.txt` declares:

```cmake
project(FroggersTigaDesktop VERSION x.y.z)
```

All packaging scripts read this version. GitHub Release tag:

```text
froggerstiga-v1     ← only movable release channel (web sim download links)
```

Package metadata uses CMake `VERSION`. Move the tag on `main` and force-push to rebuild DMG + EXE on CI.

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

Output: `desktop/dist/FroggersTiga.dmg`

Mount the DMG, drag **FroggersTiga.app** to **Applications**, then open the app.

### Windows → Setup.exe

```powershell
./desktop/scripts/package-windows.ps1
```

Output: `desktop/dist/FroggersTiga-Setup.exe`

## Unsigned builds

These packages are **not** code-signed.

| Platform | User experience |
|----------|-----------------|
| macOS | Gatekeeper may block first launch. Right-click → **Open**, or run `xattr -cr /Applications/FroggersTiga.app` for local dev. |
| Windows | SmartScreen may warn on first run. User chooses **Run anyway**. |

Signing and notarization are planned as a follow-up change.

## GitHub Releases (CI)

Move the release tag to `main` and push (rebuilds DMG + EXE on CI):

```sh
git tag -f froggerstiga-v1
git push origin froggerstiga-v1 --force
```

Workflow `.github/workflows/desktop-release.yml` builds on `macos-14` and `windows-latest`, then attaches:

- `FroggersTiga.dmg`
- `FroggersTiga-Setup.exe`

to the GitHub Release for that tag. Release notes are rendered from `SIM_MANUAL.md` **Version history** (`desktop/scripts/render-release-notes.sh`).

## Scripts

| Script | Purpose |
|--------|---------|
| `read-version.sh` / `read-version.ps1` | Print CMake `VERSION` |
| `verify-tag-version.sh` | CI gate: exact release channel tag `froggerstiga-v1` |
| `verify-release-metadata.sh` | CI gate: CMake, app macro, web package roots, README/SIM_MANUAL current-release headings |
| `package-macos.sh` | Stage `.app` + Applications link → DMG |
| `package-windows.ps1` | Run ISCC with `/DMyAppVersion` and `/DReleaseDir` |

## Local-only targets (not on public `main`)

Public GitHub releases ship **desktop standalone** and **web sim** only.

| Target | Policy |
|--------|--------|
| **VCV Rack plugin** (`vcv/`) | Local-only; directory in `.gitignore`. CV-only — no MIDI widgets or CC mod sources; per-parameter CV jacks combine with internal routes. |
| **VST3 / AU** (`PluginEditor`, `PluginProcessor`) | Local-only; sources in `.gitignore`; `BUILD_VST=OFF` by default. **107** DAW-automatable host parameters; no hosted CC ingest or MIDI Settings — map MIDI in the DAW. Mod rack: VCO Envelope + Random 1/2 only. |

To build VST locally (after restoring plugin sources on your machine):

```sh
cd desktop
cmake -B build -DBUILD_VST=ON
cmake --build build --config Release
```

Hosted parameter inventory is validated by `HostParameterProcessor_test` (expects exactly 107 parameters). CI (`host-preflight.yml`, `desktop-release.yml`, `pages.yml`) never sets `BUILD_VST=ON` and never builds `vcv/`. See `docs/CI.md`.

**Worktree hygiene:** `scripts/verify_clean_rebuild.sh` rebuilds sim, web, and desktop/VST from clean output trees and fails if tracked source drifts or generated host-display files are stale.

