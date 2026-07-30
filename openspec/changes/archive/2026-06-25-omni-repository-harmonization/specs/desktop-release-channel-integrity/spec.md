## ADDED Requirements

### Requirement: Exactly one desktop release channel is admitted
Desktop release automation SHALL admit only the tag `froggerstiga-v1`. The workflow trigger pattern SHALL remain `froggerstiga-v*`, but every build and publish job MUST be gated so any other matching tag performs no release work.

#### Scenario: Canonical channel tag
- **WHEN** a push moves `froggerstiga-v1`
- **THEN** the macOS, Windows, and release jobs are eligible to run

#### Scenario: Other matching tag
- **WHEN** a pushed tag matches the workflow wildcard but is not `froggerstiga-v1`
- **THEN** no desktop artifact is built or published

#### Scenario: Manual dispatch is unavailable
- **WHEN** a maintainer inspects or invokes the desktop release workflow
- **THEN** release work is reachable only from a `froggerstiga-v*` tag push and not from `workflow_dispatch` or a branch ref

### Requirement: Publication updates the canonical release
Release automation SHALL attach the current DMG and Windows installer to the release for `froggerstiga-v1` and SHALL NOT create an additional desktop release channel.

#### Scenario: Existing channel release
- **WHEN** the release job runs after the canonical tag moves
- **THEN** the existing canonical release receives replacement assets and notes rendered from `SIM_MANUAL.md` Version history

### Requirement: Release documentation names only the canonical channel
Operator and packaging documentation SHALL provide only the exact canonical tag commands and SHALL NOT describe wildcard or versioned desktop tag alternatives.

#### Scenario: Packaging instructions
- **WHEN** a maintainer reads `README.md` or `desktop/PACKAGING.md`
- **THEN** the documented publish flow moves and force-pushes only `froggerstiga-v1`

### Requirement: Package and application versions have one authority
`desktop/CMakeLists.txt` project version SHALL be the package-version authority for desktop, VST/AU, shared sim release metadata, and private web package metadata. Host-visible application version code SHALL consume `JUCE_APPLICATION_VERSION_STRING` rather than a literal. This harmonization SHALL NOT increment the current version.

#### Scenario: Desktop About/version query
- **WHEN** the desktop application reports its version
- **THEN** it equals the CMake project version

#### Scenario: Metadata drift
- **WHEN** web package metadata or current-release documentation disagrees with the CMake project version
- **THEN** the release metadata check fails before publication

#### Scenario: Historical versions remain historical
- **WHEN** the metadata check scans changelogs or prior Version history entries
- **THEN** it ignores historical release numbers and compares only current release metadata
