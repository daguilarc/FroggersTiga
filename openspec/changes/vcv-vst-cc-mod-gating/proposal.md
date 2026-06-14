## Why

Desktop and web now gate MIDI CC mod sources via `CvMidiBridge` enable flags (`midi-cc-mod-gating`). VCV Rack bypasses that authority in `vcv/src/plugin.cpp` (`drainVcvMidiIn` writes CC 1–4 directly to `mods[]`), and the VST plugin hides MIDI Settings despite sharing `DesktopHostIO` core gating. Host parity requires VCV ingest through the bridge and explicit VST documentation/UI alignment.

## What Changes

- **VCV Rack:** Route CC ingest through `host.m_midiBridge.PushMidiCc` + `drainMidiIn`; respect `isCcPairEnabled`; add CC1/CC2 enable toggles on the field-parity panel.
- **VCV Rack:** Grey disabled CC mod columns; block patch assignment to disabled CC indices; clear routes on disable (reuse core `ClearModRoutesForIndex` pattern).
- **JUCE VST/AU:** Expose MIDI Settings (or equivalent CC enable controls) when plugin-hosted; document that DAW CC respects enable flags with both CC default On.
- **Shared:** Reuse `IsSimModSourceAvailable`, `SetMidiCcPairEnabled`, and pair-indexed APIs from core — no duplicate CC1/CC2 branches.

## Capabilities

### New Capabilities

- `vcv-cc-mod-gating`: VCV Rack CC ingest through `CvMidiBridge`; panel enable toggles; grey mod UI; assignment gating aligned with desktop/web.

### Modified Capabilities

- `juce-vst-plugin`: MIDI Settings visibility / CC enable UX when plugin-hosted; inherits core gating from `midi-cc-mod-gating`.
- `vcv-field-parity-module`: CC1/CC2 enable toggles on 2-row panel; disabled-column grey state.

## Impact

- **VCV:** `vcv/src/plugin.cpp`, field-parity panel widgets, mod-rack column paint
- **VST:** `AudioEngine::showMidiSettings` plugin-hosted guard, `MainComponent` MIDI button visibility, `SIM_MANUAL.md`
- **Core (read-only reuse):** `CvMidiBridge.hpp`, `SimModSource.hpp`, `DesktopHostIO.hpp` — already implemented in `midi-cc-mod-gating`
- **Reference change:** `openspec/changes/midi-cc-mod-gating/`
