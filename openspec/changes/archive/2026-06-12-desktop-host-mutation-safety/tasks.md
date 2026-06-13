## 1. Mutation queue core

- [x] 1.1 Replace morph-only queue in `DesktopHostIO.hpp` with typed `HostMutation` enum + union payload (extend existing ring buffer)
- [x] 1.2 Implement `enqueueMutation()` with coalesce for `RandomizeAllMod` / `RandomizeAllPages`
- [x] 1.3 Implement `drainMutationQueue()` at start of `tickControls()`; remove separate `drainMorphQueue`
- [x] 1.4 Route `RandomizeVcoMorphs`, `CycleVcoMorph`, `NudgeVco3Morph` through mutation enum — `CycleMorph` added in `desktop-vco-morph-fix`

## 2. Sim randomize picker

- [x] 2.1 Add `PickSimRandomModIndex(RGen&)` to `SimModSource.hpp` — P(none)=0.5, else uniform `{0,4,5,6}`
- [x] 2.2 Add `Parameter::RandomizeModSim(float knobPos)` in `Parameter.hpp`
- [x] 2.3 Add `PageManager::RandomizePageModSim` / `RandomizeAllPagesModSim`; firmware keeps `RandomizeMod`
- [x] 2.4 Update `DelayState::randomizeMod()` — randomize `modSource[]` + `modDepth[]` per row using same picker
- [x] 2.5 Add `DelayState::sanitizeModSources()` + core page sanitize helper; call from `DesktopHostIO::Init()`

## 3. Wire desktop UI to queue

- [x] 3.1 `GlobalStrip`: enqueue `RandomizeAllPages`, `RandomizeAllMod` (+ Delay randomize in same mutation)
- [x] 3.2 `PanelBackend`: enqueue per-panel `RandomizePage` / `RandomizePageMod` / Delay equivalents
- [x] 3.3 `PatchCableOverlay::setModSource` → enqueue `SetPageModSource` / `DelaySetModSource` instead of direct host calls
- [x] 3.4 Grep desktop for direct `RandomizePanelMod`, `SetPageModSource`, `randomizeMod` bypassing queue; fix stragglers

## 4. DSP recovery

- [x] 4.1 In `AudioEngine::audioDeviceIOCallbackWithContext`, after `ProcessBlock`, if block non-finite → `SoftResetFxState()` + `delay.softResetFx()` on audio thread (guard against per-sample repeat)
- [x] 4.2 `audioDeviceStopped` / `audioDeviceError`: soft-reset FX if `m_lastBlockNonFinite`
- [ ] 4.3 Keep `stopAudio()` soft-reset path; verify Play → Randomize mod (all) → audio continues without app restart

## 5. WASM parity

- [x] 5.1 Wire `froggers_randomize_all_mod` / per-page mod randomize to `RandomizeModSim` paths in bindings
- [x] 5.2 Confirm web dropdown + randomize never assigns `{1,2,3}` (grep + manual)

## 6. Patch overlay verification

- [ ] 6.1 Manual: gray **SRR1** input → drag → **VCO Envelope** output → cable persists
- [ ] 6.2 Manual: empty Delay **DTIM** input → drag → **Marbles 1** output (`stereo-delay-page` §E.7)
- [ ] 6.3 Manual: connected plug → void disconnect; move to different row; reassign to different output; same-output cancel
- [ ] 6.4 Manual: fan-out one output → two inputs on different panels

## 7. Randomize + ghost routes

- [ ] 7.1 Manual: global + per-panel **Randomize mod** — no lit jacks without visible cable; no `modIndex` in `{1,2,3}`
- [ ] 7.2 Manual: Delay **Randomize mod** assigns sources — cables appear on overlay
- [x] 7.3 Build desktop: `cd desktop/build && cmake --build .`

## 8. Spec / archive alignment

- [x] 8.1 On archive: supersede `desktop-patchbay-vcv` and update `stereo-delay-page` tasks §E.4/E.6 (sources + depths)
- [x] 8.2 On archive: supersede `sim-hosts-multi-ui` empty-input no-op scenario + output-only patch wording
- [x] 8.3 Close `desktop-host-corrections` morph queue task 2.2 after `desktop-vco-morph-fix` lands (`GetMorph` linear + `CycleMorph` + idle drain)
