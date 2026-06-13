## Why

The desktop MIDI Settings page and implementation confuse **internal mod routing** with **physical MIDI ports**. Users expect:

- **MIDI In** (default: Computer keyboard) — QWERTY piano notes or an external MIDI source feed the **MIDI** mod jack on the rack (→ patch cables → knob CV). That is the sim’s “MIDI mod out,” not a cable leaving the computer.
- **MIDI Out (VCO Env)** — optional mirror of **VCO Envelope** mod to a **physical MIDI output device**, only when one is selected/open.

Today QWERTY is squashed into a fake single **CC** (always 127), hardware **Note On/Off is ignored**, and labels do not explain VCO Env vs mod jack.

## What Changes

- **MIDI In → mod rack** — QWERTY and hardware **note** messages on the configured in channel drive `m_mods[0]` (max held velocity → 0–1). Hardware **CC** on configured in channel + in CC still works for knob controllers. **Superseded:** velocity-only note semantics replaced by pitch CV x velocity in `desktop-qwerty-midi-pitch-cv`.
- **Computer keyboard stays default MIDI In** — correct placement; fix behavior and copy, not location.
- **MIDI Out (VCO Env)** — rename section; envelope CC to physical `m_midiOut` **only when** a MIDI out device is open. No keyboard traffic on this port.
- **Settings clarity** — piano legend, refresh devices, mod-controller status, open-failure feedback.
- **Unchanged** — one MIDI mod lane (`m_mods[0]`); patchbay; focus/modal guards; web (no MIDI).

## Capabilities

### New Capabilities

- `desktop-midi-settings-ux`: Labels **MIDI Out (VCO Env)**, legend, refresh, status, open-failure feedback.

### Modified Capabilities

- `desktop-qwerty-midi-input`: Notes (not CC collapse) from QWERTY → `m_mods[0]`; hardware notes on MIDI In accepted the same way.

## Impact

- `src/core/CvMidiBridge.hpp` — note-aware mod input (held-note velocity → `m_mods[0]`)
- `desktop/Source/MainComponent.cpp` — QWERTY feeds note mod state; remove CC-only collapse
- `desktop/Source/AudioEngine.cpp` — hardware `handleIncomingMidiMessage` handles notes; `tickMidiOut` only when `m_midiOut` open
- `desktop/Source/MidiSettingsComponent.*` — **MIDI Out (VCO Env)** labels, legend, refresh, layout

---

**Superseded:** QWERTY mod level = max velocity only → **`desktop-qwerty-midi-pitch-cv`** pitch step × velocity on `m_mods[0]`.
