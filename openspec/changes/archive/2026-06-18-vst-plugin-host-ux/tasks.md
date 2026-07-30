## 1. Mutation drain (hosted functional blocker)

- [ ] 1.1 `MainComponent::timerCallback` — call `DrainPendingMutations()` when `isPluginHosted()` regardless of `isAudioRunning()`
- [ ] 1.2 Manual: Randmod with DAW transport stopped — mod routes change within ~1s

## 2. Patch cable overlay resync

- [ ] 2.1 Add `PatchCableOverlay::syncRoutesFromHost()` — rebuild hues from host `modIndex` per input port; clear hue when `modIndex == 255`
- [ ] 2.2 Wire `DesktopHostIO` post-mutation callback for mod-affecting types → `syncRoutesFromHost()`
- [ ] 2.3 `PluginProcessor::setStateInformation` → notify editor → `MainComponent` → `syncRoutesFromHost()`
- [ ] 2.4 Manual: Randmod reroutes visible cables; preset reload matches routes

## 3. Hosted chrome policy

- [ ] 3.1 `MainComponent::initFromEngine` — hide `m_recordCluster` when `isPluginHosted()`
- [ ] 3.2 Manual: no Record/Export cluster in DAW

## 4. QWERTY MIDI policy

- [ ] 4.1 `shouldCaptureQwertyMidi()` returns false when `isPluginHosted()`
- [ ] 4.2 Manual: DAW typing does not modulate; standalone QWERTY unchanged

## 5. Preset snapshot v3

- [ ] 5.1 Extend `SimPresetSnapshot`: `vcoMorph[3]`, `ccEnabled[2]`, `ccChannel[2]`, `ccNumber[2]`; bump `kVersion` to 3
- [ ] 5.2 `write()` / `read()` — v1/v2/v3 branches; v1/v2 get defaults for new fields
- [ ] 5.3 Manual: save/reload morph + CC config in DAW

## 6. Editor sizing

- [ ] 6.1 `PluginEditor`: `setResizeLimits(1440, 720, 8192, 4320)`
- [ ] 6.2 Manual: minimum size — scopes and labels readable

## 7. Documentation

- [ ] 7.1 `SIM_MANUAL.md` + synced copies: VST plugin-hosted § (chrome, QWERTY, snapshot v3)
- [ ] 7.2 Add cross-host mod-routing matrix (desktop/VST overlay, web dropdown, VCV Rack cables)
- [ ] 7.3 `desktop/PACKAGING.md` — local VST build unchanged

## 8. Manual DAW verification gate

- [ ] 8.1 VST3 load smoke test (Logic or Reaper macOS)
- [ ] 8.2 Randmod cable reroute + transport-stopped mutation test
- [ ] 8.3 VST vs standalone A/B with same snapshot bytes
- [ ] 8.4 Sign-off — section complete only when 8.1–8.3 pass

## 9. Local build verify

- [ ] 9.1 `cmake -B build -DBUILD_VST=ON && cmake --build build --config Release` — exit 0
- [ ] 9.2 Standalone still builds with `BUILD_VST=OFF`

## 10. Cross-change hygiene

- [ ] 10.1 Update `vcv-rack-field-parity` design: VST reference for Randmod + cable UX intent
- [ ] 10.2 VCV Randmod cable spawn scoped to indices 4–6 only (no CC mod rack outs) — align with VST/desktop engine pool
