## Why

OMNI audit of `sim-pm3-knob-parity`, `vcv-vst-field-parity-panel`, and current implementation found **label authority drift**, **CI gaps**, **VST host-compliance holes**, and **stale OpenSpec context** despite Phase A landing. These violate OMNI single-source rules and leave the original PM3 confusion unresolved at the UX layer (web ignores wasm row names; users conflate VCO Envelope mod source with Audio row 7).

## What Changes

- Web sim UI SHALL read knob/page labels from wasm (`froggers_row_name` / screen `rows[].name`) instead of duplicate `HOST_PAGE_LABELS` / hardcoded mod-bay strings.
- CI SHALL run `sim/check_param_display_names.sh` (and extended mod-source checks) on every Pages build; fail on drift.
- VST plugin SHALL persist/restore `DesktopHostIO` page + knob state via `getStateInformation` / `setStateInformation`; honor host bypass where applicable.
- VCV stub SHALL reuse member audio buffers (no per-block `std::vector` alloc in `process`).
- OpenSpec artifacts for `vcv-vst-field-parity-panel` and `sim-pm3-knob-parity` SHALL reflect Phase A reality (PagedHostIO APIs, DelayState, VST target exist).
- Document consolidation path for four-copy manual sync (`SIM_MANUAL.md` → `docs/` → `web/public/`) via single copy step in build scripts.

## Capabilities

### New Capabilities

- `host-label-single-authority`: Web UI and mod bay use wasm/C++ label exports only; remove or generate duplicate TS tables.
- `compliance-ci-gates`: Automated label/doc drift checks in GitHub Actions.
- `vst-plugin-host-compliance`: Preset serialization and DAW bypass semantics for `FroggersTigaPlugin`.
- `vcv-runtime-efficiency`: Reusable process buffers and artifact accuracy for VCV stub until Phase B widget.

### Modified Capabilities

- (none — no archived `openspec/specs/` baseline in repo)

## Impact

- `web/src/main.ts`, `web/src/froggers-processor.ts` — label source of truth
- `.github/workflows/pages.yml` — CI gate
- `sim/check_param_display_names.sh` — extended checks
- `desktop/Source/PluginProcessor.cpp`, `AudioEngine` — preset + bypass
- `vcv/src/plugin.cpp` — buffer reuse
- `openspec/changes/vcv-vst-field-parity-panel/design.md` — context diagram update
- `openspec/changes/sim-pm3-knob-parity/tasks.md` — close manual verification with scripted smoke where possible
