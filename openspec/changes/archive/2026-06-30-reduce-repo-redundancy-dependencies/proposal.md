## Why

The current omni harmonization removed many host-behavior contradictions, but the repository still has maintenance drag from duplicated mirrors, large dependency/tooling surfaces, and unclear local-only planning boundaries. This change narrows the update surface so future host, web, desktop, and spec edits flow through fewer authorities without treating intentional per-version UI differences as violations.

## What Changes

- Add an explicit repository maintenance contract for dependency footprint, generated artifacts, local-only tooling, and duplicated mirrors.
- Document OpenSpec as local-only planning state for this workspace; OpenSpec helpers and subagents must not perform git operations.
- Audit and classify large dependency/tooling surfaces: vendored Daisy firmware dependencies, Rack SDK, Emscripten SDK, downloaded Node runtime, JUCE FetchContent cache/build trees, and generated host build outputs.
- Reduce copied publication/doc assets where possible by deriving mirrors from root sources during build or by adding freshness checks for every intentionally committed mirror.
- Launch-gate public SIM manual content so `SIM_MANUAL.md` and its mirrors describe only launched desktop/web surfaces until VST/AU and VCV testing is complete.
- Treat `src/common` firmware wrappers around `src/core` as compatibility shims, not duplicated DSP ownership, while adding guardrails so new shared DSP work lands in `src/core` first.
- Consolidate overlapping hygiene and verification scripts into a small set of documented gates with shared path classifications instead of repeated allow/deny lists.
- Preserve valid host/version differences: desktop may keep two MIDI CC cells, web may keep one, VCV/VST may keep host-native routing, and UI layouts may diverge when the divergence is named as a host projection.

## Capabilities

### New Capabilities

- `repository-maintenance-surface`: Governs dependency footprint, generated artifact boundaries, local-only tooling, source mirrors, local-only OpenSpec planning, and the rule that host/version UI differences are allowed only when expressed as explicit projections.
- `manual-launch-gated-hosts`: Public operator manuals only document launched host surfaces; unreleased/local-only hosts remain internal until launch.

### Modified Capabilities

- `froggers-host-master`: Adds a host-level maintenance requirement that shared host contracts state whether they are public repo source or local-only planning state, and that generated/public mirrors have a declared authority and freshness gate.
- `field-operator-doc-parity`: Clarifies that firmware-facing docs may remain separate, but root/published help mirrors must be either generated or freshness-checked from an authority.
- `sim-operator-doc-parity`: Clarifies that web/public and docs help mirrors are publication outputs derived from root manuals, not independent editing surfaces, and removes VST/AU plus VCV from public SIM manual mirrors until launch.

## Impact

- Local workspace policy: `.gitignore`, `.git/info/exclude` guidance, local-only OpenSpec planning, local-only VCV/VST policies, generated and published assets.
- Dependency/tooling surfaces: `External/`, `Rack-SDK/`, `.emsdk/`, downloaded `node-v*`, JUCE FetchContent build caches, package metadata, and build output directories.
- Shared code boundaries: `src/core/`, `src/common/`, firmware wrappers, host-only `sim/`, desktop, web/WASM, VCV, and VST/AU sources.
- Verification scripts and docs: `SIM_MANUAL.md`, `docs/sim-manual.md`, `web/public/sim-manual.md`, `scripts/check_host_artifact_hygiene.sh`, `scripts/check_openspec_hygiene.sh`, `sim/check_operator_docs_sync.sh`, `scripts/sync-help-docs.sh`, `docs/CI.md`, `README.md`, and OpenSpec specs/tasks.
