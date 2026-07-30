## Why

The VCV Rack wrapper is a minimal audio-through stub (page knob + I/O only). The desktop sim already implements full Field parity—six submodule panels, mod rack, patchbay, and global strip on `DesktopHostIO`. Users need the same all-at-once, patchable layout in Rack and in a DAW as a VST/AU, not a paged mini-module.

**OMNI audit finding:** `PagedHostIO` (used by VCV today) lacks the page-indexed knob/mod APIs and `DelayState` sidecar that desktop and web already use for all-six-columns-at-once behavior. VCV widget work on top of the current stub will look correct but route incorrectly until the host layer is extended first.

## What Changes

- Replace the 12 HP stub with a **wide chonker panel** (~60–84 HP) showing **all six submodules** (Audio, Random, Reverb, Filter, Drive, Delay) simultaneously with knobs, labels, and **per-row mod input jacks** patchable from the mod rack. Use up to **three stacked Rack rows** (primary module + expanders) if one 3U row is too cramped; prefer **two rows** (mod rack + I/O row, one expander with all six columns) when a paper mockup confirms fit.
- **Extend `PagedHostIO`** with page-indexed knob/mod APIs mirroring `DesktopHostIO` (`SetPageKnob`, `SetPageModSource`, `GetPageParam`, sim display names via `ParamDisplayNames`).
- **Wire `DelayState` + `SetSimFxInsert`** on the VCV shared engine instance (same pattern as `WasmSimHost` / desktop `AudioEngine`).
- Add mod rack row (MIDI, VCO Envelope, Random 1/2 outputs) with output jacks matching desktop routing indices. **No oscilloscopes on VCV** (LED indicators only). **VST keeps scopes** — resizable editor has room for full desktop mod rack UX.
- Extract **shared panel semantics** (row names, mod indices, layout constants) from desktop into a host-neutral layer reused by VCV widget code and JUCE UI.
- Add **JUCE VST3/AU plugin target** that reuses the existing desktop `MainComponent` stack (same visual design as standalone app; plugin wrapper only).
- Keep **license boundary**: MIT `src/core/` + sim headers; GPL `vcv/`; plugin target MIT (JUCE + core, no Rack SDK).
- **Prerequisite:** `sim-pm3-knob-parity` label/doc alignment (Audio row 7 = Phase mod 3; VCO Envelope is mod source 4 only) before VCV Audio column ships.
- **Non-breaking** for firmware, web sim, and existing standalone desktop build.

## Capabilities

### New Capabilities

- `shared-host-panel-model`: Host-neutral panel/mod layout tables, page-indexed host APIs, and backend adapters shared by desktop, VCV, and VST.
- `vcv-field-parity-module`: Full-width VCV module stack (up to 3 Rack rows via expanders) with six visible submodules, mod rack jacks, LED-only mod indicators, DelayState FX, and Field-parity I/O.
- `juce-vst-plugin`: VST3/AU plugin build sharing desktop UI and `DesktopHostIO` audio path (full mod rack including VCO Envelope scope; resizable editor).

### Modified Capabilities

- (none — `openspec/specs/` baseline was removed; all requirements are new)

## Impact

- `src/core/PagedHostIO.hpp` — **extend** with page-indexed APIs and sim display names (not unchanged)
- `sim/DelayState.hpp` — VCV expander stack wires same sidecar as desktop/web
- `vcv/src/plugin.cpp` — block `ProcessBlock`, shared engine + DelayState across expanders; remove invalid multi-row `box.size`
- `vcv/src/widgets/` — new nanovg layout driven by `HostPanelLayout` + `ParamDisplayNames`
- `vcv/plugin.json` (module dimensions metadata)
- `desktop/CMakeLists.txt` — add `juce_add_plugin` target; shared sources from `desktop/Source/`
- New `sim/HostPanelLayout.hpp` — layout constants extracted from `DesktopChromeLayout.hpp`
- CI: optional VST build matrix; VCV build remains local (folder gitignored on remote)
- Manual: `SIM_MANUAL.md` cross-reference for Rack/VST install paths; PM3/VCO Envelope glossary (overlaps `sim-pm3-knob-parity`)
