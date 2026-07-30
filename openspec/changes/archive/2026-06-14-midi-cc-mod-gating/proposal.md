## Why

MIDI CC 1 and MIDI CC 2 can be configured but not explicitly disabled. When a user turns a CC input off, the mod rack column still looks live, patch cables still connect, and random-mod still assigns it — causing silent or confusing modulation. Gating must follow input enable state on desktop, web, and JUCE VST/AU.

## What Changes

- Per-row **Enable** toggle for **MIDI CC 1** and **MIDI CC 2** in desktop MIDI Settings (default On).
- Web **CC 1** / **CC 2** enable toggles beside External MIDI (default Off until External MIDI is on; each CC row independently toggleable).
- Core authority: disabled CC mod sources are unavailable for assignment, random mod, and CV output.
- Desktop mod rack: disabled columns greyed out; output jacks not patchable; existing cables to disabled sources cleared on disable.
- Web mod bay: disabled scopes greyed out; mod dropdown excludes disabled sources; existing assignments cleared on disable.
- Random mod (`Rand mod`, `Rand Mods`, delay random mod) excludes disabled MIDI CC sources from the pool (50%-None semantics preserved).
- QWERTY keyboard CC feed respects **MIDI CC 1** enable only.
- JUCE VST/AU inherits desktop core gating + MIDI Settings UI (both CC default On).

## Capabilities

### New Capabilities

- `midi-cc-mod-gating`: Enable/disable MIDI CC mod sources; grey unavailable UI; block patch/dropdown assignment and random mod selection; clear stale routes when disabled.

### Modified Capabilities

- `midi-cc-to-mod-cv` (delta in this change): extends dual CC ingest with per-pair enable flags and assignment gating. Reference prior change `openspec/changes/midi-cc-only-cv/specs/midi-cc-to-mod-cv/spec.md`.

## Impact

- **Core:** `CvMidiBridge.hpp`, `SimModSource.hpp`, `Page.hpp` / `Parameter.hpp`, `DelayState.hpp`, `PagedHostIO.hpp` / `DesktopHostIO.hpp`
- **Desktop + VST:** `MidiSettingsComponent`, `ModModuleBox`, `ModRackPanel`, `PatchCableOverlay`, `AudioEngine` (QWERTY feed)
- **Wasm:** `bindings.cpp` — host-scoped `froggers_mod_source_available`, `froggers_assignable_mod_*`, pair enable getters/setters
- **Web:** `main.ts`, `style.css`, `froggers-processor.ts` — mod bay grey state, dynamic assignable list refresh
- **Out of scope:** VCV Rack (`vcv/src/plugin.cpp` separate ingest path) — separate change
- **Docs/CI:** `SIM_MANUAL.md`, extend `sim/check_mod_source_labels.sh` for host-parameterized availability API
