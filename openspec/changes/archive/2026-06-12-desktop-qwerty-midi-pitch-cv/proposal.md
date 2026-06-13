## Why

`desktop-midi-input-clarity` routed QWERTY piano keys through real note on/off, but the MIDI mod jack still behaves as a **0/1 gate**: every QWERTY key sends velocity 127, `CvMidiBridge` exports only `max(velocity)` to `m_mods[0]`, and note pitch is discarded. The scope and patched knobs cannot distinguish **A** from **W** from **P**. Note events are also applied on the UI thread while `drainMidiIn` reads held state on the audio thread without a queue.

## What Changes

- **Pitch + velocity → `m_mods[0]`** — Highest held note on the configured in channel maps to pitch CV across the QWERTY range (MIDI 60–75). `pitchStep = clamp((highestNote − 60 + 1) / 16, 0, 1)` so every piano key including **A** produces a distinct non-zero step; mod level = `pitchStep × (maxHeldVelocity / 127)`; 0 when no notes held. Hardware MIDI keeps velocity scaling.
- **Audio-thread note drain** — Note on/off events enqueue via an atomic SPSC ring (same pattern as `HostMutation`); `drainMidiIn` applies held-note state once per block on the audio thread. `PushMidiNote` does not mutate held state on the message thread.
- **Scope reflects pitch steps** — MIDI mod scope shows stepped CV as keys change (no code change if scope already reads `GetCvOut(0)` after drain).
- **Docs** — Update `QUICK_DICT.md` / MIDI Settings legend + In CC tooltip: MIDI mod is **pitch CV × velocity**, not a gate.
- **Supersedes** partial semantics in `desktop-midi-input-clarity` spec (“max velocity only”).
- **Unchanged** — Physical MIDI Out (VCO Env only); patchbay; web (no MIDI); QWERTY velocity stays 127 for v1.

## Capabilities

### New Capabilities

- `desktop-midi-pitch-cv`: Pitch-normalized MIDI mod from held notes, audio-thread note queue, QWERTY per-key differentiation.

### Modified Capabilities

- `desktop-qwerty-midi-input`: Replace “max velocity only” with pitch × velocity semantics (delta in this change).

## Impact

- `src/core/CvMidiBridge.hpp` — `MidiNoteEvent` atomic SPSC queue, drain-time held-note update, single-pass pitch+velocity recompute
- `desktop/Source/AudioEngine.cpp` — `feedMidiInNote` enqueues only (no bridge mutation on message thread)
- `desktop/Source/MidiSettingsComponent.cpp` — legend + In CC tooltip copy
- `QUICK_DICT.md` — MIDI mod description
- `openspec/changes/desktop-midi-input-clarity/` — superseded velocity-only requirement noted on archive
