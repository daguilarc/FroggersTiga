## 1. Core — dual CC→CV inputs (shared)

- [x] 1.1 Remove note queue and QWERTY constants from `CvMidiBridge.hpp`
- [x] 1.2 Dual in pairs + latches; `drainMidiIn` → `mods[0]`, `mods[1]`
- [x] 1.3 `PushMidiCc` tags pair 1 or 2; discard non-matching
- [x] 1.4 Add `m_midiBridge` to `PagedHostIO`; call `drainMidiIn` in `tickControls`
- [x] 1.5 `ParamDisplayNames::forModSource` — cases 0 `"MIDI CC 1"`, 1 `"MIDI CC 2"`
- [x] 1.6 `SimModSource` / assignable index list includes mod **1**

## 2. Desktop — settings, mod rack, cleanup

- [x] 2.1 CC-only ingest; remove QWERTY / `feedMidiInNote`
- [x] 2.2 `MidiSettingsComponent`: MIDI CC 1 | MIDI CC 2 input row (Ch+CC each)
- [x] 2.3 Mod rack five columns: MIDI CC 1, MIDI CC 2, VCO Env, Random ×2
- [x] 2.4 `HostPanelLayout` + `ModRackPanel` five-box loop
- [x] 2.5 CC slider width fix; dialog 520 px
- [x] 2.6 Hardware MIDI Out envelope section unchanged

## 3. Wasm

- [x] 3.1 `froggers_push_midi_cc(host, channel, cc, value)` in `wasm/bindings.cpp`
- [x] 3.2 `WasmSimHost::kScopeModIndices` → `{0, 1, 4, 5, 6}`; scope ring size 5

## 4. Web — External MIDI + mod bay

- [x] 4.1 `index.html` + CSS: **External MIDI** button under **External** audio button
- [x] 4.2 `main.ts`: `setExternalMidiEnabled` — `requestMIDIAccess`, permission errors, listener attach/detach
- [x] 4.3 `froggers-processor.ts`: handle `{ type: "midiCc" }` → `froggers_push_midi_cc`
- [x] 4.4 Prepend mod-bay scopes mod 0 + 1; extend `SCOPE_MOD_INDICES`
- [x] 4.5 Build mod-source `<select>` from wasm assignable indices + labels at `ready`; remove hardcoded options; replace `modSelectIndex` with findIndex

## 5. Docs + verification

- [x] 5.1 Update docs — dual CC mod inputs; web External MIDI; no QWERTY
- [x] 5.2 Sync mirrored copies
- [x] 5.3 Extend `check_mod_source_labels.sh` for mods 0, 1 and mod-select hardcoding
- [x] 5.4 Desktop: two CC controllers → two patch routes
- [x] 5.5 Web: External MIDI on → CC1/CC2 scopes + dropdown modulation
