## ADDED Requirements

### Requirement: Host build products are not tracked
Generated CMake trees, object files, test executables, packaged desktop/VCV artifacts, web/WASM distributions, and IDE metadata produced under the in-scope `sim/`, `desktop/`, `wasm/`, `web/`, and `vcv/` host trees SHALL be ignored and SHALL NOT be tracked as source.

#### Scenario: Native test build
- **WHEN** a developer configures and runs the sim test suite
- **THEN** `git status --short` shows no changes caused solely by generated test products

#### Scenario: Host package build
- **WHEN** a desktop, web/WASM, or VCV package is built
- **THEN** its generated build/package output remains untracked unless it is an explicit Pages publication artifact

### Requirement: Daisy Field firmware is outside artifact-hygiene scope
This capability SHALL NOT classify, remove, re-ignore, or otherwise change the original Daisy Field firmware application's sources, build products, support/demo applications, build system, or manual under `src/FroggersTiga/`, other hardware-facing `src/` paths, `src/common/`, `src/mk/`, `External/libDaisy/`, or `MANUAL.md`. Shared `src/core/` files MAY be touched only when required and verified by an in-scope desktop/web/VCV/VST behavior change.

#### Scenario: Host hygiene scan encounters firmware paths
- **WHEN** the host artifact check enumerates tracked files
- **THEN** it excludes the Daisy firmware paths without reporting compliance or noncompliance for them

#### Scenario: Shared core dependency
- **WHEN** an in-scope host change modifies `src/core/`
- **THEN** its task and verification evidence identify the consuming host behavior and make no firmware verification claim

### Requirement: Published Pages artifacts are explicit exceptions
The repository MAY track `docs/` output required by the configured GitHub Pages publication flow, but SHALL verify that it was produced by the canonical web build and help-doc sync.

#### Scenario: Pages build output
- **WHEN** the web publication bundle is refreshed
- **THEN** only the intentional `docs/` publication diff is eligible for commit

### Requirement: Published help documents remain deterministic mirrors
`SIM_MANUAL.md` and `QUICK_DICT.md` SHALL remain the editable authorities for the two tracked mirrors of each document under `docs/` and `web/public/`. Generated `web/dist/` copies SHALL remain untracked build output.

#### Scenario: Published manual drift
- **WHEN** a tracked help mirror differs from its root authority
- **THEN** preflight verification fails and names the drifting mirror

#### Scenario: Web distribution build
- **WHEN** Vite copies help files into `web/dist/`
- **THEN** those generated copies remain untracked and are not treated as an additional editable authority

### Requirement: Artifact hygiene is automatically checked
A host-scoped repository check SHALL fail when a prohibited generated host path is tracked or when canonical in-scope OpenSpec source is hidden by a blanket ignore, while excluding Daisy firmware paths, vendored dependencies, documented ephemeral state, and intentional publication artifacts.

#### Scenario: Accidental tracked object
- **WHEN** a new object file or build-tree file is added inside an in-scope host path and outside an allowed publication exception
- **THEN** the hygiene check exits nonzero and names the path

#### Scenario: Planning tree blanket ignore
- **WHEN** `.gitignore` contains a rule that excludes the complete `openspec/` planning home
- **THEN** the hygiene check exits nonzero and requires selective ephemeral-state rules instead
