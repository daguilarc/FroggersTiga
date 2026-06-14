## Why

Desktop and web gate MIDI CC mod sources via `CvMidiBridge` enable flags (`midi-cc-mod-gating`). VCV Rack bypasses that authority: `drainVcvMidiIn` pushes CC 1–4 directly into `mods[]` without `PushMidiCc`, and `ProcessBlock` → `tickControls` → `drainMidiIn` immediately overwrites `mods[0]` and `mods[1]` from an empty bridge latch — CC ingest on VCV is effectively broken today. VCV also keeps a **second** `CvMidiBridge` (`midiBridge`) for MIDI out only, diverging from `host.m_midiBridge`. The VST plugin inherits core gating for DAW CC ingest but `showMidiSettings` returns early when plugin-hosted, leaving the visible MIDI Settings toolbar button a no-op.

## What Changes

- **VCV Rack:** Replace `drainVcvMidiIn` direct `mods[]` writes with `host.m_midiBridge.PushMidiCc(ch, cc, value)` per queued message; rely on existing `host.ProcessBlock` → `tickControls` → `drainMidiIn` for latch → `mods[0/1]`.
- **VCV Rack:** Remove CC 3–4 direct `mods[]` writes (only CC pairs 1 and 2 exist in core; indices 2–3 are not assignable mod sources).
- **VCV Rack:** Unify MIDI out on `host.m_midiBridge`; delete the duplicate module-level `midiBridge` member.
- **VCV Rack:** Add CC1/CC2 enable toggles on the primary panel (pair-indexed loop → `host.SetMidiCcPairEnabled`); dim CC-related panel affordances when disabled.
- **VCV Rack:** Route clearing and assignment guards reuse core `SetMidiCcPairEnabled`, `ClearModRoutesForIndex`, and `IsModSourceAvailable` — no duplicate CC1/CC2 branches.
- **JUCE VST/AU:** Show CC enable toggles when plugin-hosted (CC-only `MidiSettingsComponent` section; hide device pickers and MIDI Out). Wire toolbar MIDI button or hide it when the dialog is unavailable.
- **Docs:** Align `SIM_MANUAL.md` VST section with CC enable UX (core gating already active for DAW CC; user needs enable controls).

## Capabilities

### New Capabilities

- `vcv-cc-mod-gating`: VCV CC ingest through `host.m_midiBridge`; panel enable toggles; disabled-state visuals; assignment gating aligned with desktop/web.

### Modified Capabilities

- `juce-vst-cc-mod-gating`: Plugin-hosted CC enable UX; inherits core gating from `midi-cc-mod-gating`.
- `vcv-field-parity-module`: CC1/CC2 enable toggles on primary row; single `CvMidiBridge` on `PagedHostIO`.

## Impact

- **VCV:** `vcv/src/plugin.cpp` (ingest, bridge unification, enable toggles)
- **VST:** `desktop/Source/AudioEngine.cpp` (`showMidiSettings` guard), `desktop/Source/MidiSettingsComponent.cpp` (plugin-hosted layout), `desktop/Source/MainComponent.cpp` (MIDI button visibility)
- **Core (read-only reuse):** `CvMidiBridge.hpp`, `SimModSource.hpp`, `PagedHostIO.hpp` — implemented in `midi-cc-mod-gating`
- **Reference change:** `openspec/changes/midi-cc-mod-gating/`
