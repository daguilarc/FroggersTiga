# Desktop QWERTY MIDI pitch CV — tasks

## 1. CvMidiBridge queue + pitch formula

- [x] 1.1 Add `MidiNoteEvent` + atomic SPSC ring (`kMidiNoteQueueSize = 64`, mirror `HostMutation`); `PushMidiNote` enqueues only
- [x] 1.2 `drainMidiIn`: drain note queue into `m_heldVelocity`, then single-pass recompute `pitchStep × velNorm` with `pitchStep = clamp((highestNote − 60 + 1) / 16, 0, 1)`
- [x] 1.3 On queue overflow drop oldest event; `jassert` in debug builds
- [x] 1.4 Remove `recomputeModLevelFromNotes` from `PushMidiNote` path; held arrays mutate only in `drainMidiIn`
- [x] 1.5 CC path: SPSC queue drains after note level; last matching CC overwrites `mods[0]`

## 2. AudioEngine thread boundary

- [x] 2.1 `feedMidiInNote` → bridge enqueue only (verify no direct held-state mutation on message thread)
- [x] 2.2 Hardware `handleIncomingMidiMessage` note path uses same enqueue API

## 3. Docs and settings copy

- [x] 3.1 `QUICK_DICT.md`: MIDI mod = pitch CV × velocity, highest note wins; hardware notes outside 60–75 clamp
- [x] 3.2 `MidiSettingsComponent` piano legend: pitch steps on scope when patched (every key including **A**)
- [x] 3.3 `MidiSettingsComponent` In CC tooltip: notes drive pitch CV × velocity; CC overrides when sent
- [x] 3.4 Note in `desktop-midi-input-clarity` artifacts that velocity-only mod semantics are superseded

## 5. CC SPSC queue (thread safety)

- [x] 5.1 Add `MidiCcEvent` + atomic SPSC ring (`kMidiCcQueueSize = 64`); `PushMidiCc` enqueues only
- [x] 5.2 `drainCcQueue` in `drainMidiIn` after note level; remove `std::vector m_pendingIn`
- [x] 5.3 Overflow drops oldest with `jassert` in debug builds

## 4. Verification

- [ ] 4.1 Play + patch MIDI → knob; **A**, **W**, and **P** move parameter to three distinct non-zero values
- [ ] 4.2 Hold **A**+**P**, release **P** → level drops to **A** pitch (1/16 × velocity)
- [ ] 4.3 Hold **A**+**D**, release **A** → level stays at **D** pitch (highest remaining note)
- [ ] 4.4 **A** alone shows scope activity above idle (mods[0] ≈ 0.0625, not 0)
- [ ] 4.5 Hardware keyboard: same note, soft vs hard velocity changes level
- [ ] 4.6 Scope shows steps, not flat 0/1 gate, while changing keys
- [x] 4.7 Desktop debug + Release build
