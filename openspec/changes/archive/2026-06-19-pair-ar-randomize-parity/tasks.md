> **Reconciled (omni 1.2):** Code-backed; §5 manual host smokes remain open.

## 1. State layer

- [x] 1.1 Add `AudioPairArState::randomizeKnobs()` — loop `kCount`, `RGen::UniGenRange(0,1)`, `setKnob(i, v)` (mirror `DelayState::randomizeKnobs`)

## 2. Shared orchestration (OMNI)

- [x] 2.1 Create `sim/HostRandomize.hpp` with `RandomizePageWithExtras`, `RandomizePageModWithExtras`, `RandomizeAllPagesWithPairAr` gated on `AudioPairArLayout::kAudioHostPage`
- [x] 2.2 Wire `PagedHostIO::RandomizePage`, `RandomizePageMod`, `RandomizeAllPages` to shared helpers
- [x] 2.3 Wire `DesktopHostIO::applyMutation` for `RandomizePage`, `RandomizePageMod`, `RandomizeAllPages` to same helpers (keep existing `m_delay->randomizeKnobs()` on global all)

## 3. Verification

- [x] 3.1 Add sim unit test: Audio page randomize changes pair-AR knobs via `PagedHostIO`
- [x] 3.2 Add sim unit test: global `RandomizeAllPages` changes pair-AR knobs
- [x] 3.3 Add sim unit test: Audio page randmod changes pair-AR mod source/depth
- [x] 3.4 Run sim unit test target locally; all pass

## 4. Docs and polish

- [x] 4.1 Update `GlobalStrip` Rand All tooltip to mention pair-AR band (not only Delay)
- [x] 4.2 Optional: one line in `SIM_MANUAL.md` — Audio page Randomize includes pair-sum A/R knobs

## 5. Host smoke (manual)

- [ ] 5.1 Desktop: Audio Randomize / Randmod / global Rand All move pair-AR sliders
- [ ] 5.2 Web: same three actions on Audio page after WASM rebuild
- [ ] 5.3 Confirm global Rand Mods still randomizes pair-AR mod (regression — already wired)
