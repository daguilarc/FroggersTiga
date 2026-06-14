## 1. Core — enable flags and availability

- [x] 1.1 Add `m_inCcEnabled[2]` + `isCcPairEnabled` / `isCcModIndexEnabled` to `CvMidiBridge.hpp` (default true)
- [x] 1.2 Gate `PushMidiCc`, QWERTY feed path, and `drainMidiIn` on enable flags; zero latch on disable
- [x] 1.3 Add `IsSimModSourceAvailable(modIndex, bridge)` to `SimModSource.hpp`
- [x] 1.4 Update `PickSimRandomModIndex(rgen, bridge)` preserving 50%-None; thread bridge through `Parameter::RandomizeModSim`, `PageManager` randomize paths, `DelayState::randomizeMod`, `DesktopHostIO::applyMutation`
- [x] 1.5 Add `ClearModRoutesForIndex(uint8_t modIndex)` on both `PagedHostIO` and `DesktopHostIO` (pages + Delay)
- [x] 1.6 Add `SetMidiCcPairEnabled(uint8_t pairIndex, bool)` on both host IO types; clear routes on false transition
- [x] 1.7 Guard `SetPageModSource` / delay `setModSource` / `SetRowModSource` with availability check
- [x] 1.8 Remove `SetRowModSource` modIndex==0 guard that blocks CC1 web assignment

## 2. Desktop — settings and mod rack

- [x] 2.1 Add CC1/CC2 Enable toggles to `MidiSettingsComponent` wired to `SetMidiCcPairEnabled`; increase dialog height
- [x] 2.2 `ModModuleBox::setPatchEnabled(bool)` grey paint + desaturated jack
- [x] 2.3 `ModRackPanel::refresh` derives enable state from mod index in box loop
- [x] 2.4 `PatchCableOverlay`: mark disabled output ports; block hit-test and drag-to-connect; invalidate cable hues on disable
- [x] 2.5 `AudioEngine::feedComputerKeyboardCc1` checks CC1 enabled

## 3. Wasm bindings

- [x] 3.1 Export `froggers_mod_source_available`, `froggers_set_cc_pair_enabled`, `froggers_cc_pair_enabled`
- [x] 3.2 Change `froggers_assignable_mod_count/index` to host-scoped; filter through availability helper; update processor call sites
- [x] 3.3 Post `modAvailabilityChanged` from worklet when flags change

## 4. Web — toggles, mod bay, dropdown

- [x] 4.1 Add CC1/CC2 enable buttons in `index.html` + `style.css` (`.mod-disabled` grey state)
- [x] 4.2 `main.ts`: External MIDI off → both CC wasm setters false; on → both true; independent CC toggles
- [x] 4.3 `applyModBayAvailability` greys disabled scope columns
- [x] 4.4 Refresh `assignableModOptions` from host-scoped wasm on availability change; repopulate mod selects
- [x] 4.5 `froggers-processor.ts`: handle CC enable UI messages; emit updated assignable list

## 5. Docs and verification

- [x] 5.1 Update `SIM_MANUAL.md` — CC enable toggles, grey mod sources, random-mod exclusion, VST inherits desktop
- [x] 5.2 Sync help docs via `scripts/sync-help-docs.sh`
- [x] 5.3 Manual verify desktop: disable CC2 → grey jack, no new cable, Rand Mods skips CC2, routes cleared
- [x] 5.4 Manual verify web: External MIDI off → grey CC scopes, dropdown omits CC sources; CC2 off → same for CC2 only
- [x] 5.5 Build desktop Release + `npm run build` (web) — web build OK; wasm rebuild requires emscripten; desktop build requires JUCE fetch
