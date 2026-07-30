## 1. VCV — bridge ingest and unification

- [x] 1.1 Replace `drainVcvMidiIn` direct `mods[]` writes with `host.m_midiBridge.PushMidiCc` per queued message; remove CC 3–4 branch
- [x] 1.2 Delete module-level `midiBridge`; route `tickMidiOut` through `host.m_midiBridge`
- [x] 1.3 Verify `ProcessBlock` → `tickControls` → `drainMidiIn` is the sole writer of `mods[0/1]` (no ingest-side `mods[]` mutation)

## 2. VCV — panel enable UI

- [x] 2.1 Add CC1/CC2 enable toggles on primary panel (pair-indexed loop → `host.SetMidiCcPairEnabled`)
- [x] 2.2 Dim enable controls when off; confirm route clear on disable via core `ClearModRoutesForIndex`

## 3. VST — MIDI Settings parity

- [x] 3.1 Remove `showMidiSettings` plugin-hosted early return; add plugin-hosted layout to `MidiSettingsComponent` (CC rows + enable toggles only)
- [x] 3.2 Confirm toolbar MIDI Settings button opens dialog in plugin mode
- [x] 3.3 Verify DAW CC ingest respects enable flags; mod rack greys disabled CC columns via existing `ModRackPanel::refresh`

## 4. Docs and verification

- [x] 4.1 Update `SIM_MANUAL.md` VST section: CC enable toggles available in plugin MIDI Settings; DAW routes CC to plugin input
- [x] 4.2 Sync help docs (`web/public/sim-manual.md`)
- [x] 4.3 Build VCV plugin + VST; manual verify disable CC2 on VCV (mods[1] zero, routes cleared) and VST (grey column, DAW CC ignored)
