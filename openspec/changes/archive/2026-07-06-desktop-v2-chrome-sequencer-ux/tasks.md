## OMNI verification gates (run before merge)

- [x] OMNI.1 Build desktop-v2 standalone (`FroggersTigaDesktopV2` links); manual QA at 1280 px — **operator** verify Play, Record audio + formats, Write Seq., scope ≤ 320
- [x] OMNI.2 `ControlCoreBridge_test`: `test_sequencer_dice_step_and_pattern`, `test_sequencer_record_capture`, `test_write_seq_*` pass; suite exits on pre-existing `test_pair_ar_gate_policy` failure (unrelated)
- [x] OMNI.3 `rg 'm_play\{"Engine"\}' desktop-v2/` — zero matches
- [x] OMNI.4 Sequencer UI uses **Write Seq.** only (`m_writeSeq`); no `m_seqRecord` in panel
- [x] OMNI.5 Recording API in `AudioEngine.cpp`; `m_recorder.appendStereo` in `renderSimOutputChunk`
- [x] OMNI.6 Nesting audit on write-seq handlers and layout helpers — max 3 levels (subagent verified)
- [x] OMNI.7 Spec hedge grep — zero forbidden terms
- [x] OMNI.8 `SubmodulePagePanel.cpp` — zero `withSizeKeepingCentre` in encoder rows

## 1. Layout authority

- [x] 1.1 **Extend** `desktop-v2/Source/ui/DesktopV2ChromeLayout.hpp` (file exists): add `kTransportScopeMaxWidth`, `kSceneButtonMinWidth`, `kBlendLabelMinWidth`, `kModCellHeight`, module-row column offsets; alias or replace undersized `kPerfSceneButtonSize` / `kPerfBlendEndpointLabelW`
- [x] 1.2 Wire `MainComponent::resized` transport row to cap scope at `kTransportScopeMaxWidth`; leave flex remainder unused (replace L317 `m_vcoEfScope.setBounds(transport)`)
- [x] 1.3 VST: `HostedMainComponentV2::resized` keeps full-width scope strip; consume shared chrome constants where applicable
- [x] 1.4 Extract `layoutStandaloneTransportRow(juce::Rectangle<int>& transport)` — Play, Stop, MIDI, Audio, RecordButton, scope cap; call from `MainComponent::resized`

## 2. Audio transport Play label

- [x] 2.1 Change `MainComponent.h` `m_play{"Engine"}` → `m_play{"Play"}`
- [x] 2.2 Grep desktop-v2 for user-facing **Engine** on audio transport; remove or restrict to internal `AudioEngine` type names only

## 3. Audio export restore

- [x] 3.1 Port v1 `startRecording` / `stopRecording` / `writeCaptureToFile` into `desktop-v2/Source/AudioEngine`
- [x] 3.2 Feed `m_recorder` from v2 audio output callback while recording
- [x] 3.3 Add v1 `RecordButton` to v2 transport row (red circle + **Record audio**); WAV/MP3/FLAC/OGG format toggles in **Audio** menu (`AudioSettingsComponent`); `AudioEngine` holds export format
- [x] 3.4 Wire `handleRecordClick` parity with v1 (`desktop/Source/MainComponent.cpp`)
- [x] 3.5 Hide audio export cluster in `HostedMainComponentV2`

## 4. Performance band chrome

- [x] 4.1 Apply `kSceneButtonMinWidth` to S1/S2/S3 in `PerformanceBandV2`
- [x] 4.2 Raise marbles label strip to `kPerfMarblesLabelH`; verify S&H 1 / S&H 2 readable at 1280 px
- [x] 4.3 Apply `kBlendLabelMinWidth` to L/R blend labels
- [x] 4.4 Remove BPM, Steps, Start Sequence, Stop Sequence, and `m_seqRecord` from `PerformanceBandV2`

## 5. Sequencer toolbar

- [x] 5.1 Move BPM and Steps editors into `SequencerPanelComponent` toolbar above step grid (Label + Slider — same widgets as perf band today)
- [x] 5.2 Move Start Sequence / Stop Sequence buttons into sequencer toolbar
- [x] 5.3 Add **Write Seq.** toggle in sequencer toolbar; rename `SequencerState::m_recordArm` → `m_writeSeqArm` and update host param display name **Write Seq.**
- [x] 5.4 Rename scope toggle **Pattern** → **All steps**; keep `kRandSeqScopePattern` constant
- [x] 5.5 Add **Rand-seq** text label beside dice; set tooltip **Randomize sequencer steps (scene slots)**
- [x] 5.6 Style Step / All steps as radio buttons (`radioGroupId`, `setClickingTogglesState(false)`, default Step selected)
- [x] 5.7 In `SequencerPanelComponent::resized`, measure toolbar min width; when `toolbarMinW > availableW` at 1280 px window, split toolbar to two rows inside sequencer panel
- [x] 5.8 Verify `HostedMainComponentV2` uses shared `SequencerPanelComponent` after toolbar move (BPM/Steps/Write Seq. visible in VST editor; no audio Record row)

## 6. Write Seq. capture behavior

- [x] 6.1 Stopped + armed: on edit-step change, capture live → `m_steps[oldEdit]` then apply `m_steps[newEdit]`
- [x] 6.2 Stopped + disarm: capture live → `m_steps[editStep]` once
- [x] 6.3 Playing + armed on Start Sequence: immediate capture → `m_steps[m_playhead]`
- [x] 6.4 Playing + armed on beat advance: capture → `m_steps[stepLeft]` where `stepLeft = (playhead + len - 1) % len` **after** host advance
- [x] 6.5 Update `FroggersV2HostBridge::onSequencerStepAdvance` (replace capture-to-playhead bug); wire Start Sequence immediate capture in bridge or panel
- [x] 6.6 Tests: step 0 on first beat; step 0 on Start Sequence; stopped navigate save (`ControlCoreBridge_test.cpp`)

## 7. Module row and mod cell layout

- [x] 7.1 Refactor `SubmodulePagePanel::layoutRows` to left-anchored column offsets from `DesktopV2ChromeLayout`
- [x] 7.2 Remove `withSizeKeepingCentre` encoder placement in leftover width
- [x] 7.3 Fix `ModSourceCell` to `kModCellHeight` for None and assigned states

## 8. Rand-seq behavior fix

- [x] 8.1 Remove `hasData` skip in `FroggersV2ControlCore::onRandSequencerStep` All-steps loop (L741–744)
- [x] 8.2 Update `ControlCoreBridge_test` All-steps dice test: assert step 3 **is** overwritten when `hasData == true`

## 9. Documentation

- [x] 9.1 Update `QUICK_DICT.md`: Play/Stop, **Record audio** vs Write Seq., Rand-seq, write-seq workflows
- [x] 9.2 Sync `docs/quick-dict.md` and `web/public/quick-dict.md`

## 10. Verification (summary — see OMNI gates §top)

- [x] 10.1 Automated OMNI.3–OMNI.8 gates pass; OMNI.1 build pass; OMNI.2 change-scoped tests pass
- [ ] 10.2 Manual: S1/S2/S&H labels readable; All steps overwrites non-blank steps; audio Record requires Play first (v1 parity) — **operator QA at 1280 px**

## 11. OMNI compliance fixups (post-implementation audit — required before archive)

- [x] 11.1 Split audio export UI: **Record audio** (`RecordButton`) in transport only; WAV/MP3/FLAC/OGG in **Audio** menu; `AudioEngine::exportFormat` / `setExportFormat`; `kRecordButtonMinWidth`
- [x] 11.2 Move transport control width/gap constants from `DesktopV2TransportLayout.hpp` into `DesktopV2ChromeLayout.hpp` (`kTransportPlayStopW`, `kTransportSettingsW`, `kTransportGapSm`, `kTransportGapMd`)
- [x] 11.3 Sequencer prev/next: `setEditStep(m_sequencer->wrappedEditStep(±1))` — single wrap source on `SequencerState`; **not** `prevEditStep()` then `setEditStep(m_editStep)` (early-return skips write-seq capture)
- [x] 11.4 Removed orphan `m_title` from `SequencerPanelComponent`
- [x] 11.5 `SubmodulePagePanel`: two-pass row bounds — accumulate `RowLayout`, then `setBounds`
- [x] 11.6 Hoist `kModLabelStripH` from `ModSourceCell.cpp` into `DesktopV2ChromeLayout.hpp`
