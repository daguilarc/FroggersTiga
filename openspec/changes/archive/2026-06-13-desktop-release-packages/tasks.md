## 1. macOS packaging

- [x] 1.1 Add `desktop/scripts/package-macos.sh` — resolve Release `.app` path (D7), stage with Applications symlink, `hdiutil create -format UDZO`, write to `desktop/dist/`
- [x] 1.2 Add `BUNDLE_ID com.froggers.tiga.desktop` to `juce_add_gui_app` in `desktop/CMakeLists.txt`
- [x] 1.3 Add `desktop/dist/` to `.gitignore`

## 2. Windows packaging

- [x] 2.1 Add `desktop/installer/FroggersTiga.iss` — install Release artefact dir; version from ISCC `/DMyAppVersion=` (no hardcoded default in committed path)
- [x] 2.2 Add `desktop/scripts/package-windows.ps1` — locate ISCC, pass `/DMyAppVersion` and `/DReleaseDir`, fail with install URL if missing

## 3. Version helper

- [x] 3.1 Add `desktop/scripts/read-version.sh` and `read-version.ps1` — shared contract: parse `VERSION` from `CMakeLists.txt`, stdout only, exit 1 on miss
- [x] 3.2 Add `desktop/scripts/verify-tag-version.sh` — strip `desktop-v` from tag; fail if ≠ read-version (CI gate)

## 4. CI release workflow

- [x] 4.1 Add `.github/workflows/desktop-release.yml` — three jobs (`build-macos`, `build-windows`, `release`); not a single matrix-only workflow
- [x] 4.2 Windows job: `choco install innosetup`; both build jobs run verify-tag-version + package scripts; release job uses `needs:` + `permissions: contents: write`

## 5. Documentation

- [x] 5.1 Add `desktop/PACKAGING.md` — prerequisites, build + package commands, unsigned-app notes, tag = CMake VERSION rule
- [x] 5.2 Update `README.md` Desktop section with link to PACKAGING.md and `desktop-v*` release tag convention

## 6. Verify

- [x] 6.1 macOS: `./desktop/scripts/package-macos.sh` → DMG mounts, app runs
- [x] 6.2 Windows: package script produces Setup.exe (local VM or CI log) — script implemented; verify on first `desktop-v*` CI run
- [x] 6.3 Dry-run CI workflow syntax (`actionlint` or manual review)

## 7. Archive

- [x] 7.1 Archive change; merge spec into `openspec/specs/desktop-release-packages/`
