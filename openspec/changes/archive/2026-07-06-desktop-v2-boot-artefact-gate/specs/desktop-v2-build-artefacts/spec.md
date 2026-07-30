## ADDED Requirements

### Requirement: macOS Release bundle is the canonical standalone artefact

Desktop v2 macOS standalone builds SHALL place the launchable `.app` bundle under `desktop-v2/build/FroggersTigaDesktopV2_artefacts/Release/FroggersTigaV2.app`. Documentation and launch scripts SHALL NOT direct operators to a sibling bundle at `FroggersTigaDesktopV2_artefacts/FroggersTigaV2.app` unless that path is verified to be the same build output as Release.

#### Scenario: PACKAGING documents Release path

- **WHEN** an operator reads `desktop-v2/PACKAGING.md` for macOS Release output
- **THEN** the documented path includes the `Release/` subdirectory

#### Scenario: Launch script opens Release bundle

- **WHEN** an operator runs `scripts/open-desktop-v2.sh` from a repo with a successful Release build
- **THEN** macOS opens `Release/FroggersTigaV2.app`

### Requirement: Stale root-level macOS bundle is removed after build

The desktop-v2 build SHALL remove or replace a pre-existing `FroggersTigaDesktopV2_artefacts/FroggersTigaV2.app` that is not the current Release output, so an older pre-fix binary cannot be launched after rebuild.

#### Scenario: Post-build stale bundle cleanup

- **WHEN** `cmake --build desktop-v2/build --config Release` completes successfully
- **THEN** no launchable stale `FroggersTigaDesktopV2_artefacts/FroggersTigaV2.app` remains from a prior build generation
