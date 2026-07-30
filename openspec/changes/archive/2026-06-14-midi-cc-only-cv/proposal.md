## Why

MIDI In currently mixes notes, QWERTY piano, and a single CC→CV path into `mods[0]`. The product need is simpler: **two hardware MIDI CC inputs**, each `(channel, CC number) → 0–1 CV on the mod rack**. The mod rack **MIDI CC 1 / MIDI CC 2 columns** (patch jacks on desktop) are what we mean by "OUT"—not hardware MIDI Out. Notes and QWERTY add complexity without value.

The **website** today has no MIDI and no mod-rack entries for `mods[0]`/`mods[1]`. It should gain the same two CC→CV mod sources via **Web MIDI**, gated behind user permission like External Audio.

## What Changes

### Desktop

- **Remove** note handling, QWERTY piano, computer-keyboard MIDI input.
- **Add** two MIDI input pairs in MIDI Settings (one row): **MIDI CC 1** and **MIDI CC 2** (each Channel + CC).
- **Rename** mod rack **MIDI** → **MIDI CC 1**; **add** **MIDI CC 2** column to its right (five-column row).
- **Keep** hardware MIDI Out unchanged (envelope to physical port—separate from mod rack).
- **Fix** CC slider text-box width.

### Web

- **Add** **External MIDI** button directly under **External** (External Audio); request Web MIDI access only when user turns it on (same permission pattern as mic).
- **Add** **MIDI CC 1** and **MIDI CC 2** scopes to mod bay (left of VCO Envelope), driven by `mods[0]` and `mods[1]`.
- **Add** mod-source dropdown entries for indices 0 and 1 (same modulation workflow as VCO Envelope / Random).
- **Wire** Web MIDI CC messages → wasm `PushMidiCc` → shared `CvMidiBridge` on `PagedHostIO`.
- **Extend** wasm scope ring for mod indices 0 and 1.
- Update docs (four-copy sync).

## Capabilities

### New Capabilities

- `midi-cc-to-mod-cv`: Two CC→CV inputs on shared `CvMidiBridge`; notes ignored; drives `mods[0]` and `mods[1]`.
- `mod-rack-dual-midi-jacks`: Desktop mod rack five-column row with MIDI CC 1 + MIDI CC 2.
- `web-midi-mod-rack`: Web MIDI ingest with permission-gated External MIDI control and mod-bay parity.

### Modified Capabilities

- (none)

## Impact

- `src/core/CvMidiBridge.hpp`, `PagedHostIO.hpp`, `DesktopHostIO.hpp`
- `sim/ParamDisplayNames.hpp`, `src/core/SimModSource.hpp`
- `wasm/bindings.cpp`, `sim/WasmSimHost.hpp`
- Desktop: `MidiSettingsComponent`, `ModRackPanel`, `AudioEngine`, `MainComponent`
- Web: `index.html`, `main.ts`, `froggers-processor.ts`, `style.css`
- Docs; supersedes `fix-midi-cc-display-width`
