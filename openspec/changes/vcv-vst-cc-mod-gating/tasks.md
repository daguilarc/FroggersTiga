## 1. VCV — bridge ingest

- [ ] 1.1 Replace `drainVcvMidiIn` direct `mods[]` writes with `PushMidiCc` + `drainMidiIn`
- [ ] 1.2 Wire `SetMidiCcPairEnabled` on VCV host IO; clear routes on disable

## 2. VCV — panel UI

- [ ] 2.1 Add CC1/CC2 enable toggles to field-parity panel (pair-indexed loop)
- [ ] 2.2 Grey disabled CC mod columns; block assignment to unavailable indices

## 3. VST — MIDI Settings parity

- [ ] 3.1 Unblock CC enable controls for plugin-hosted mode (MIDI Settings or inline toggles)
- [ ] 3.2 Verify DAW CC ingest respects enable flags with both CC default On

## 4. Docs and verification

- [ ] 4.1 Update `SIM_MANUAL.md` VST + VCV CC gating sections
- [ ] 4.2 Sync help docs
- [ ] 4.3 Build VCV plugin + VST; manual verify disable CC2 on each host
