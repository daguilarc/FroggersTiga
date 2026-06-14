## 1. Layout constants

- [ ] 1.1 Add anonymous-namespace constants in `MidiSettingsComponent.cpp`: `kChannelControlWidth` (50), `kCcControlWidth` (80), `kCcTextBoxWidth` (44), `kRowControlHeight` (24)

## 2. CC slider configuration

- [ ] 2.1 In constructor, loop over `{&m_inCc, &m_outCc}` and apply: `LinearHorizontal`, `TextBoxRight` with `kCcTextBoxWidth`, `setNumDecimalPlacesToDisplay(0)`
- [ ] 2.2 Replace magic `50` in `resized()` channel bounds with `kChannelControlWidth`
- [ ] 2.3 Replace magic `50` in `resized()` CC bounds (`m_inCc`, `m_outCc`) with `kCcControlWidth`

## 3. Verification

- [ ] 3.1 Rebuild desktop standalone target
- [ ] 3.2 Manual QA: open MIDI Settings → set In CC to 10, 74, 127 — confirm no ellipsis
- [ ] 3.3 Manual QA: set Out CC to 10, 74, 127 — confirm no ellipsis
- [ ] 3.4 Confirm CC value still writes to `CvMidiBridge.m_inCc` / `m_outCc` after edit
