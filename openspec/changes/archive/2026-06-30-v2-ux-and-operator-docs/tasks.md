## 1. Pair-AR labels and host display

- [x] 1.1 Update `sim/ParamDisplayNames.hpp` `forAudioPairAr` to Attack 1+2 / Release 1+2 / Attack 2+3 / Release 2+3 (remove Att./Rel.)
- [x] 1.2 Run `scripts/generate-host-display.mjs`; verify `web/src/hostDisplay.generated.ts` `PAIR_AR_KNOB_LABELS` matches authority
- [x] 1.3 Update `web/e2e/v2-expanded-pages.spec.ts` and `web-mobile-knob-labels` selectors for full Attack labels

## 2. Pair-AR global Crunchy engine path

- [x] 2.1 Route pair-AR through `Page::ApplyV2MusicalFuego` (Crunchy + Audio Crispy) when `UsesV2Fuego`
- [x] 2.2 Wire Audio page pointer from `PagedHostIO` and `DesktopHostIO` init
- [x] 2.3 Extend `sim/AudioPairArEffective_test.cpp`: Crunchy max, Crispy max, Crunchy zero equals raw; Desktop v1 unchanged
- [x] 2.4 Rebuild WASM; verify web `froggers_get_audio_pair_ar_effective` reflects Crunchy and Crispy when mod inactive

## 3. Operator documentation

- [x] 3.1 Add Quick Dict sections: Scenes, Gestures, Sequencer, Crunchy/Crispy/pair-AR matrix (`QUICK_DICT.md`)
- [x] 3.2 Update `SIM_MANUAL.md` Global Crunchy / pair-AR bullets for Crunchy-on-pair-AR (v2 hosts) and Crispy exclusion
- [x] 3.3 Run `scripts/sync-help-docs.sh`; pass `sim/check_operator_docs_sync.sh`
- [x] 3.4 Rebuild desktop v2 assets so embedded Quick Dict picks up changes

## 4. Desktop v2 performance band

- [x] 4.1 Add `PerformanceBandV2` component: Scene S1–S3 + blend, G1/G2 toggles + weight sliders, sequencer transport
- [x] 4.2 Wire gesture weight sliders to `MessageIn::GestureWeight`; scene controls to existing bus types
- [x] 4.3 Insert band in `MainComponent` between scope grid and carousel; adjust `DesktopV2ChromeLayout` heights
- [x] 4.4 Remove duplicate scene/gesture/sequencer controls from `GlobalStripV2` after band is wired
- [x] 4.5 Wire or hide LFO/VCO buttons per `MessageIn::Type` audit (no dead controls)

## 5. Desktop v2 module layout

- [x] 5.1 Raise `kVisibleEncoderSlots` to 10; show all rows for Audio (8) and expanded/ADSR (10) at default window size
- [x] 5.2 Hide bank prev/next when all rows fit; retain bank paging only on height-constrained resize
- [x] 5.3 Update `SubmodulePagePanel` and `AdsrPagePanel` to allocate up to 10 ring rows
- [x] 5.4 Reflow `PageCarouselComponent::resized()` so prev/next flank module title

## 6. Web transport, morph sync, and external meter

- [x] 6.1 Align `requireEngineForAction`: on failure when `audioRunning`, call `applyPlayingStatus()` before return; gate `#rand-morphs` on `engineActionReady()` same as `#rand-all`
- [x] 6.2 `onScreen`: always assign `lastMorphs` from payload; `renderVcoMorphButtons` updates SVG from `lastMorphs` on every morph-bearing screen update (including off Audio page)
- [x] 6.3 Add `#external-meter-label` with states `Off` / `Waiting for Play` / active bar; stop using `#status` as implicit meter readout
- [x] 6.4 Ensure `startAudio` calls `connectWorkletOutput()` before expecting `inputPeak`; post immediate screen refresh after external enable when `audioRunning`
- [x] 6.5 Add Playwright: Rand waveforms updates VCO morph SVG on Audio page after Play; external meter shows active state after Play + External on

## 7. Verification (this change)

- [x] 7.1 `ctest --test-dir sim/build` including new pair-AR Crunchy tests
- [ ] 7.2 Desktop v2 manual QA: performance band, 10 visible rows on Filter, gesture weight affects ring edit, carousel arrows
- [ ] 7.3 Web manual QA: Attack/Release labels; Crunchy audibly/on-screen affects pair-AR knobs; Rand waveforms + transport after Play (per §6)
- [x] 7.4 Playwright v2 + mobile label + transport/meter specs green (`PLAYWRIGHT_BROWSERS_PATH` local cache)

## 8. v2 release gates (from `froggerstiga-desktop-v2` §10)

- [x] 8.1 `make -C sim test` passes including `VcoAdsrState_test`, `V2ModSource_test`, and `SequencerState_test`
- [x] 8.2 Build `FroggersTigaV2.app`
- [ ] 8.3 Build VST3/AU v2 artefacts
- [ ] 8.4 Manual desktop: module carousel, ADSR gated envelopes, Crunchy global, scene rings, 2 gesture lanes, sequencer record/playback
- [ ] 8.5 DAW: MIDI map to `Global/Crunchy`, ADSR param, and `Sequencer/BPM`
- [ ] 8.6 Manual web: expanded pages 1–5 + global Crunchy (pair-AR labels/fuego per §2–§3; transport per §6)
- [ ] 8.7 `cd web && npm run test:e2e` full suite passes (not only v2 specs)
- [ ] 8.8 Confirm v1 default build on branch (`BUILD_DESKTOP_V2=OFF`, `BUILD_VST_V2=OFF`) before merge to `main`
