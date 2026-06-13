## Context

```
Today:
  cmake --build desktop/build --config Release
    → macOS: .../Release/FroggersTiga.app
    → Windows: .../Release/FroggersTiga.exe (+ sibling DLLs in artefact dir)

Desired download UX:
  macOS user   → one .dmg, drag app to Applications
  Windows user → one Setup.exe, next-next-finish

Stack: JUCE 8 CMake (`juce_add_gui_app`), no existing CPack/installer config.
Web sim already ships via GitHub Pages CI; desktop has no CI.
```

## Data Flow

| Stage | Input | Transform | Output |
|-------|-------|-----------|--------|
| Version | `desktop/CMakeLists.txt` `project(... VERSION x.y.z)` | `read-version.sh` / `.ps1` regex parse | stdout `x.y.z`; exit 1 on miss |
| Build | source tree | `cmake -B build && cmake --build build --config Release` | `FroggersTigaDesktop_artefacts/Release/` app or exe + DLLs |
| Resolve artefact | build dir | prefer `Release/` subdir; glob `FroggersTiga.app` / `FroggersTiga.exe`; fail with clear error | single artefact path |
| macOS package | `.app` + VERSION | stage temp dir + Applications symlink → one `hdiutil create -format UDZO` | `desktop/dist/FroggersTiga-${VERSION}-macOS.dmg` |
| Windows package | Release dir + VERSION | `ISCC /DMyAppVersion=${VERSION} /DReleaseDir=...` → one compile | `desktop/dist/FroggersTiga-${VERSION}-Windows-Setup.exe` |
| CI tag gate | git tag `desktop-vX.Y.Z` | strip prefix; compare to CMake VERSION; fail on mismatch | validated release version |
| CI release | both platform artifacts | `build-macos` + `build-windows` upload → `release` job downloads → `softprops/action-gh-release` | GitHub Release assets |

One VERSION read per packaging run. Inno `.iss` never hardcodes version; ISCC receives `/DMyAppVersion=`.

## Goals / Non-Goals

**Goals:**

- One command (per platform) from a clean Release build → single distributable file.
- Version string in filename matches `project(FroggersTigaDesktop VERSION x.y.z)` in `desktop/CMakeLists.txt`.
- CI builds macOS + Windows on git tag `desktop-v*` and attaches both files to a GitHub Release.
- CI fails when tag semver ≠ CMake `VERSION`.
- Unsigned builds work for development and beta distribution (with documented Gatekeeper/SmartScreen caveats).

**Non-Goals:**

- Apple notarization / Developer ID signing (follow-up change).
- Windows Authenticode signing (follow-up change).
- Linux AppImage or Flatpak.
- Mac App Store / Microsoft Store distribution.
- Auto-update inside the app.
- CMake `install()` targets — Inno copies the Release artefact dir directly.

## Decisions

### D1: macOS artifact = DMG (not ZIP, not PKG)

**Choice:** Compressed read-only DMG (`hdiutil create -format UDZO`) with staged layout:

```
FroggersTiga.dmg
└── (volume)
    ├── FroggersTiga.app
    └── Applications → /Applications   (symlink)
```

**Why:** Standard macOS download format; one file; drag-to-install UX without extra tools.

**Alternative rejected:** ZIP of `.app` — works but no Applications shortcut; feels less polished.

**Implementation:** `desktop/scripts/package-macos.sh` — require existing Release build, stage to temp dir, `hdiutil create`, output to `desktop/dist/`.

### D2: Windows artifact = Inno Setup installer (not raw ZIP)

**Choice:** Inno Setup 6 script compiles artefact folder into one `FroggersTiga-<version>-Windows-Setup.exe`.

**Why:** JUCE Release output on Windows is a directory (exe + runtime DLLs). Inno Setup is the standard single-file installer for indie JUCE apps; ISCC runs headless in CI via `choco install innosetup`.

**Alternative rejected:** 7-Zip SFX — works but worse UX (extract-only, no Start Menu entry).

**Local requirement:** Install [Inno Setup](https://jrsoftware.org/isinfo.php); CI installs via Chocolatey.

### D3: Version and output naming

**Choice:** `FroggersTiga-${VERSION}-macOS.dmg` and `FroggersTiga-${VERSION}-Windows-Setup.exe` where `VERSION` is read from `desktop/CMakeLists.txt` `project(... VERSION ...)`.

**Why:** Matches user expectation; no manual version bump in packaging scripts.

### D4: CI trigger and job graph

**Choice:** Workflow `.github/workflows/desktop-release.yml`:

| Job | Runner | Steps |
|-----|--------|-------|
| `build-macos` | `macos-14` | verify tag semver = CMake VERSION → cmake Release → `package-macos.sh` → upload artifact |
| `build-windows` | `windows-latest` | verify tag semver = CMake VERSION → choco innosetup → cmake Release → `package-windows.ps1` → upload artifact |
| `release` | `ubuntu-latest` | `needs: [build-macos, build-windows]` → download both → `softprops/action-gh-release` |

Trigger: `push: tags: ['desktop-v*']` (e.g. `desktop-v1.0.0`).

**Why:** Windows build requires Windows runner; macOS DMG requires macOS. Tag prefix avoids accidental release on firmware tags. Three jobs required — a matrix alone cannot aggregate release assets.

### D5: Bundle identifier for macOS

**Choice:** Set `BUNDLE_ID com.froggers.tiga.desktop` on `juce_add_gui_app` for consistent identity in DMG and future signing.

**Why:** Required for notarization later; harmless for unsigned builds.

### D6: Version propagation (single source)

**Choice:** `desktop/CMakeLists.txt` `project(... VERSION ...)` is the sole version source.

- `read-version.sh` / `read-version.ps1`: shared contract — regex `project(FroggersTigaDesktop VERSION x.y.z)`, stdout version, exit 1 on miss.
- macOS / Windows package scripts call read-version once per run.
- Inno: `#ifndef MyAppVersion` guard; `package-windows.ps1` passes `/DMyAppVersion=${VERSION}` to ISCC.
- CI: on tag push, strip `desktop-v` prefix and fail if ≠ read-version output.

**Why:** OMNI single authority; prevents drift between tag, filename, and installer metadata.

### D7: Artefact resolution

**Choice:** Resolve under `desktop/build/FroggersTigaDesktop_artefacts/`:

1. Prefer `Release/FroggersTiga.app` (macOS) or `Release/` dir containing `FroggersTiga.exe` (Windows).
2. Fall back to top-level `FroggersTiga.app` only when `Release/` absent (Debug-only local builds).
3. Fail with readable error listing expected paths when neither exists.

**Why:** JUCE may leave stale top-level `.app` from non-Release builds; Release packaging must not pick the wrong binary.

### D8: CI permissions

**Choice:** Workflow-level `permissions: contents: write` for release job asset upload. Build jobs use default read.

## Risks / Trade-offs

| Risk | Mitigation |
|------|------------|
| macOS Gatekeeper blocks unsigned app | Document: right-click → Open, or `xattr -cr` for dev; signing is follow-up |
| Windows SmartScreen warns on unsigned exe | Document; Authenticode is follow-up |
| Inno Setup not installed locally | Script prints install URL; CI always has it |
| JUCE artefact path changes between versions | D7 resolution order + clear fail message |
| Tag bumped without CMake VERSION | D6 CI gate fails before packaging |

## Migration Plan

1. Land packaging scripts + Inno `.iss` + docs.
2. Test locally on macOS (DMG mount + launch).
3. Test Windows path in CI or local VM.
4. Bump CMake VERSION if needed, tag `desktop-v1.0.0` to produce first GitHub Release assets.
5. Link download URLs from README / project site.

## Resolved Questions

- **Release tag convention:** `desktop-v*` only (avoids collision with firmware semver tags).
