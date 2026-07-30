## ADDED Requirements

### Requirement: Repository surfaces are classified

The repository SHALL classify maintenance-relevant paths as public repo source, generated publication output, local cache/build output, external/local-only product surface, local-only planning state, or excluded firmware surface. Hygiene checks SHALL use the classification instead of maintaining unrelated path allow/deny lists.

#### Scenario: Host artifact hygiene uses shared classifications

- **WHEN** host artifact hygiene runs
- **THEN** it rejects prohibited tracked host build/cache outputs using the shared classification
- **THEN** it allows only named generated publication exceptions and excluded firmware paths

#### Scenario: New path class is added once

- **WHEN** a new generated or local-only surface is introduced
- **THEN** its path class is declared in the shared classification before hygiene scripts permit it

### Requirement: OpenSpec remains local-only and git-free

OpenSpec artifacts SHALL be treated as local-only planning state in this workspace. OpenSpec helpers, OpenSpec hygiene checks, and subagents SHALL NOT run git commands, require git worktrees, or use git as the transport for planning artifacts.

#### Scenario: OpenSpec planning is local

- **WHEN** an OpenSpec change is created or applied
- **THEN** it uses local filesystem artifacts under `openspec/`
- **THEN** it does not require git tracking, git worktrees, or git commands

#### Scenario: Subagents avoid git

- **WHEN** a subagent implements or reviews a task
- **THEN** it does not run git commands
- **THEN** any git inspection or mutation remains the responsibility of the primary agent when explicitly requested by the user

### Requirement: Host/version UI differences are explicit projections

Desktop, web/WASM, VST/AU, and VCV Rack MAY expose different UI features, layouts, and host integrations. Such differences SHALL be represented as explicit host projections from a named authority, not as independently edited duplicated tables or undocumented forks.

#### Scenario: Different MIDI UI is allowed

- **WHEN** desktop exposes two MIDI CC cells, web exposes one MIDI CC cell, and VST/VCV expose no MIDI CC cells
- **THEN** this is compliant if those differences derive from the host mod-rack projection authority

#### Scenario: Duplicated host table is rejected

- **WHEN** a host adds a local page, label, scope, or mod-rack table that duplicates an existing shared authority
- **THEN** hygiene or review rejects the table unless it is generated from that authority or documented as a deliberate host projection

### Requirement: Large dependencies and tooling are cache-aware

Large dependency/tooling surfaces SHALL be documented as public repo source, external install, or local cache. Local caches and downloaded runtimes SHALL NOT be treated as canonical project source. New cleanup work SHALL NOT add dependencies unless explicitly justified.

#### Scenario: Downloaded Node or Emscripten is local-only

- **WHEN** a downloaded Node runtime, Emscripten SDK, Rack SDK, or CMake dependency cache exists in the working tree
- **THEN** it is ignored as local cache or documented as an external install/cache path
- **THEN** no generated runtime directory is treated as canonical project source

#### Scenario: Desktop configure can use a local dependency cache

- **WHEN** desktop/VST configuration needs JUCE
- **THEN** the build documentation describes the pinned version and local cache/override path, with network fetch treated as an explicit fallback

### Requirement: Publication mirrors have one authority

Published documentation and web assets that mirror root sources SHALL be generated from, copied from, or freshness-checked against their root authority. Mirrors SHALL NOT become independent edit surfaces.

#### Scenario: Help docs mirror root manuals

- **WHEN** `SIM_MANUAL.md`, `QUICK_DICT.md`, or `LICENSE` changes
- **THEN** `docs/` and `web/public/` mirrors are regenerated or freshness checks fail until they match their authority

#### Scenario: Intentional Pages artifacts are named

- **WHEN** a generated Pages artifact is committed under `docs/`
- **THEN** the repository classification names it as an intentional publication artifact
- **THEN** host artifact hygiene does not treat that exception as permission to track unrelated build outputs

### Requirement: Firmware compatibility wrappers remain thin

Mirrored `src/common/<name>.hpp` wrappers for `src/core/<name>.hpp` SHALL remain include/path compatibility shims unless a file is explicitly classified as firmware-only adapter code. Shared DSP/control logic SHALL live in `src/core` first.

#### Scenario: Core wrapper stays thin

- **WHEN** a `src/common` header has the same basename as a `src/core` header
- **THEN** it contains only a compatibility include or is listed as an approved firmware-only adapter exception

#### Scenario: Shared DSP edit lands in core

- **WHEN** a feature changes shared DSP or sim control behavior used by host surfaces
- **THEN** the primary implementation lands in `src/core` or `sim`, not as a divergent copy under `src/common`
