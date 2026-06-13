# Desktop MIDI input clarity — tasks

## 1. CvMidiBridge note → mod

- [x] 1.1 Add held-note state + `PushMidiNote(channel, note, velocity, isNoteOn)` on configured in channel
- [x] 1.2 `drainMidiIn`: `mods[0]` = max held velocity / 127; CC path unchanged for in CC
- [x] 1.3 Verify OMNI: single drain site, state updated in push, applied once in drain

## 2. QWERTY → MIDI mod jack

- [x] 2.1 Replace `feedComputerKeyboardMod` CC collapse with `feedMidiInNote` from `handleNoteOn`/`handleNoteOff`
- [x] 2.2 Use `kQwertyPianoKeys` + `getNoteVelocity`; chord partial-release behavior
- [ ] 2.3 Patch MIDI jack → knob; verify scope moves on A/W/P keys during Play (pitch CV semantics — see `desktop-qwerty-midi-pitch-cv`)

## 3. Hardware MIDI In notes

- [x] 3.1 `handleIncomingMidiMessage`: Note On/Off → `PushMidiNote`; keep CC → `PushMidiCc`
- [ ] 3.2 Hardware keyboard on configured channel drives `m_mods[0]`

## 4. MIDI Out (VCO Env) only

- [x] 4.1 Rename settings labels to **MIDI Out (VCO Env)**; help text for envelope-only physical out
- [x] 4.2 Send envelope via `tickMidiOut` only when `m_midiOut` is open; add **None** out device option
- [x] 4.3 Stop auto-opening first out device at launch if **None** is desired default

## 5. MIDI Settings UX

- [x] 5.1 **Refresh devices**; MIDI In mod-rack copy; piano legend
- [x] 5.2 `isHardwareMidiInputOpenFailed()` + status label
- [x] 5.3 Dialog layout height / `resized()` for new copy

## 6. Verification

- [ ] 6.1 QWERTY → patched MIDI mod jack (not physical MIDI out port)
- [ ] 6.2 VCO Env → physical MIDI monitor only when out device selected
- [ ] 6.3 Hardware note keyboard → MIDI mod jack
- [x] 6.4 Desktop Release build
