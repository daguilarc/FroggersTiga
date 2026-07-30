## Why

The VCV Rack surface currently flattens a paged Field/web engine model into Rack columns, leaving page-oriented APIs (`SetPageKnob`, `m_currentPage`, shared `m_knobPositions`) in the Rack-facing control path. That leaks the wrong abstraction into a host that has no pages, risks current-page replay corrupting section values, and obscures how main, left, and right Rack modules should cooperate.

This change makes VCV a section/expander architecture: the main module owns the engine, adjacent modules contribute named section controls, and Rack CV/modulation is evaluated as per-block effective state without mutating page/hardware latch state.

## What Changes

- **BREAKING:** VCV implementation SHALL stop treating Rack columns as UI pages. Rack-facing code SHALL address named sections such as Audio, Random, Filter, Drive, Reverb, Delay, and VCO AR rather than `page` indices.
- Add a VCV section-control model that maps Rack knobs, internal mod routes, and per-parameter CV jacks into section-local base/effective values without using `m_currentPage` or shared `m_knobPositions`.
- Redefine the VCV module topology:
  - Main module owns the shared engine, audio/CV/gate I/O, global mod outputs, Random trigger, global Crunchy, and global Crunchy CV input.
  - Left extension contributes VCO AR controls when present; if absent, main uses defaults.
  - Right extension(s) contribute visible section controls and FX/stereo I/O when present; if absent, main uses defaults.
- Add global Crunchy to VCV as a main-module control with a CV input. The effective global Crunchy value participates in VCV section processing without replacing per-section Crispy controls.
- Preserve omni rules: VCV remains CV-only with no Froggers-owned MIDI/CC boundary, uses only internal mod sources `4/5/6`, keeps the GPL Rack boundary inside `vcv/`, and remains local-only/pre-launch.
- Update VCV docs/spec artifacts to remove stale page, MIDI/CC-enable, and obsolete expander wording.

## Capabilities

### New Capabilities
- `vcv-section-expander-architecture`: VCV section model, main/left/right expander responsibilities, page-free Rack-facing control API, global Crunchy with CV input, and per-block CV/effective-value semantics.

### Modified Capabilities
- `froggers-host-master`: VCV host contract changes from page-indexed Rack controls to named section/expander controls; adds global Crunchy CV and makes `m_currentPage`/`m_knobPositions` unavailable to VCV.
- `vcv-panel-silkscreen`: VCV faceplates must label the page-free topology, global Crunchy/CV, left VCO AR extension, and right section/FX extension controls without MIDI/CC-enable wording.

## Impact

- `vcv/src/plugin.cpp` and `vcv/src/widgets/FieldParityWidget.hpp`: replace page-indexed control writes with section-addressed state collection and main-owned effective-value application.
- `sim/` and `src/core/`: add or expose a VCV-safe section adapter over existing engine parameter banks while avoiding current-page/hardware-latch side effects.
- `sim/VcvPanelLayout.hpp`, `sim/HostPanelLayout.hpp`, and display-name authorities: add section/extension labels and global Crunchy/CV layout constants.
- `vcv/res/*.svg` and `vcv/scripts/generate_panels.py`: regenerate path-based panels with page-free labels.
- `vcv/README.md`, `vcv/DEVELOPMENT.md`, `desktop/PACKAGING.md`, and `openspec/specs/froggers-host-master/spec.md`: update docs to match the omni-compliant VCV contract.
- Tests/checks: add focused coverage for VCV section writes, Random/Randmod behavior, global Crunchy CV, no current-page replay, no double internal-route modulation, no MIDI leakage, GPL boundary, panel bounds, and path-only SVG silkscreen.
