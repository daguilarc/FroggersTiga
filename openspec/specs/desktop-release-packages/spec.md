# desktop-release-packages Specification

## Purpose

Reproducible single-file desktop simulator downloads: macOS DMG and Windows Inno Setup installer from a Release CMake build, with `desktop-v*` tag-triggered GitHub Releases.
## Requirements
### Requirement: macOS single-file DMG package

The desktop packaging flow SHALL produce a compressed DMG file containing `FroggersTiga.app` and an `Applications` symlink for drag-to-install.

#### Scenario: Package macOS after Release build

- **WHEN** an operator runs `desktop/scripts/package-macos.sh` after a successful Release CMake build
- **THEN** a file `desktop/dist/FroggersTiga.dmg` is created
- **AND** mounting the DMG shows `FroggersTiga.app` and an `Applications` folder link

#### Scenario: DMG app launches

- **WHEN** the user copies `FroggersTiga.app` from the mounted DMG to Applications and opens it
- **THEN** the FroggersTiga desktop simulator launches

#### Scenario: Packaging fails without Release build

- **WHEN** an operator runs `desktop/scripts/package-macos.sh` without a Release artefact under `FroggersTigaDesktop_artefacts/`
- **THEN** the script exits non-zero with an error naming the expected Release paths

### Requirement: Windows single-file installer

The desktop packaging flow SHALL produce a single Windows Setup executable that installs the Release artefact folder (exe and required DLLs).

#### Scenario: Package Windows with Inno Setup

- **WHEN** Inno Setup 6 is available and an operator runs the Windows packaging script after a Release build
- **THEN** a file `desktop/dist/FroggersTiga-Setup.exe` is created
- **AND** running the installer places FroggersTiga in the chosen directory with Start Menu shortcut

### Requirement: Fixed product download filenames

Packaging scripts SHALL use stable product filenames without version or platform suffixes in the download name.

#### Scenario: macOS download name

- **WHEN** packaging completes on macOS
- **THEN** the DMG is named `FroggersTiga.dmg`

#### Scenario: Windows download name

- **WHEN** packaging completes on Windows
- **THEN** the installer is named `FroggersTiga-Setup.exe`

#### Scenario: Tag semver matches CMake VERSION

- **WHEN** CI runs on tag `desktop-v1.0.1` and `desktop/CMakeLists.txt` declares `VERSION 1.0.0`
- **THEN** CI fails before producing release assets

### Requirement: CI release workflow

A GitHub Actions workflow SHALL build macOS and Windows packages on desktop release tags and upload them as GitHub Release assets.

#### Scenario: Tag triggers release build

- **WHEN** a maintainer pushes tag `desktop-v1.0.0` with matching CMake `VERSION 1.0.0`
- **THEN** CI builds Release on `macos-14` and `windows-latest`
- **AND** attaches `FroggersTiga.dmg` and `FroggersTiga-Setup.exe` to the GitHub Release

#### Scenario: Packaging documented for local use

- **WHEN** a developer reads `desktop/PACKAGING.md`
- **THEN** they find exact commands to build and package on macOS and Windows without CI

