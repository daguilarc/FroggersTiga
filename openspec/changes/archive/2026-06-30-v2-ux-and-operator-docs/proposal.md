## Why

`froggerstiga-desktop-v2` shipped engine and chrome scaffolding, but verified gaps leave operators without discoverable UX for scenes, gestures, and the sequencer, mislabeled pair-AR controls on web, and fuego behavior that contradicts operator docs. `QUICK_DICT.md` has zero scene/gesture/sequencer entries (grep over root `QUICK_DICT.md` returns no matches) while `docs/sim-manual.md` documents those features only in the full manual (§Module vs Scene, §Gestures, §Sequencer). Desktop v2 UI also violates its own OpenSpec in several places verified in source.

## What Changes

### Operator documentation (Quick Dict + manual parity)

- Add **Scenes**, **Gestures (G1/G2)**, and **Sequencer** sections to `QUICK_DICT.md` with step-by-step “how to use” glosses (store endpoint → blend → perform; select gesture lane → turn ring → adjust weight; arm record → step advance → recall).
- Sync mirrors via `scripts/sync-help-docs.sh` (`docs/quick-dict.md`, `web/public/quick-dict.md`, desktop v2 embedded assets).
- Reconcile **Crunchy vs pair-AR** copy: `docs/sim-manual.md` line 34 claims global Crunchy affects web pair-AR; `AudioPairArState.hpp` applies mod only (no `m_globalCrunchy` / `V2FuegoStack`). `sim-operator-doc-parity` currently says pair-AR is not fuegoized at all. This change wires **global Crunchy only** on `UsesV2Fuego` hosts (Web, DesktopV2, VstV2 per `SimModSource.hpp`) and updates docs to state: Crispy excludes pair-AR; Crunchy includes pair-AR on v2 fuego hosts.

### Web pair-AR labels

- Replace abbreviated `Att.` / `Rel.` in `ParamDisplayNames::forAudioPairAr` with full **Attack 1+2**, **Release 1+2**, **Attack 2+3**, **Release 2+3** (already required by `openspec/specs/audio-pair-ar-engine/spec.md` table; current code at `sim/ParamDisplayNames.hpp` lines 63–65 still abbreviates).
- Regenerate `web/src/hostDisplay.generated.ts` via `scripts/generate-host-display.mjs`.

### Web pair-AR fuego engine path

- Apply `Page::ApplyV2MusicalFuego` to pair-AR effective values on `UsesV2Fuego` hosts inside `AudioPairArState`, reading Audio page Crispy + global Crunchy via `PagedHostIO` audio page pointer.
- Crispy (Audio row 7) applies to pair-AR on web per operator requirement (same `ApplyMusicalRow` stack as musical rows).

### Desktop v2 layout and control discoverability

Verified spec violations to fix:

| Gap | Evidence | Spec |
|-----|----------|------|
| Only **4** encoder rows visible; bank paging at default 1280×820 | `DesktopV2ChromeLayout.hpp` `kVisibleEncoderSlots = 4`; `SubmodulePagePanel` loops 4 slots | `desktop-v2-page-carousel` requires **10 rows** on modules 1–5 and ADSR |
| Carousel prev/next at far header edges | `PageCarouselComponent::resized()` places `m_prev` left 28px, `m_next` right 28px, title fills center | UX feedback: arrows should flank title |
| Gesture lanes are **toggle-only**; no value sliders | `GlobalStripV2.hpp` lines 41–42; no `GestureWeight` UI; core has `gestureWeight()` + `MessageIn::GestureWeight` in `FroggersV2ControlCore.cpp` | `desktop-v2-global-controls` requires select **and value control per lane** |
| LFO/VCO buttons not wired | `GlobalStripV2.cpp` sets bounds only; no `onClick` handlers | `desktop-v2-global-controls` requires bus routing |
| Scene/gesture/sequencer cramped in 44px global strip | `kGlobalStripH = 44`; all controls in one row | Add **performance band** between scope grid and module carousel: scene S1–S3 + blend, G1/G2 weight sliders, sequencer transport summary |

### Web transport, Rand waveforms, and external input meter (regressions)

Verified in `web/src/main.ts` and `web/src/froggers-processor.ts`:

| Symptom | Root cause (verified) |
|---------|----------------------|
| **Rand waveforms** appears dead; status shows **Click Play first** after Play | `requireEngineForAction()` (`main.ts` 436–441) gates `#rand-morphs` but **not** `#rand-all` / `#marbles-btn` (699–707). On failure it sets `statusEl` to `"Click Play first"` and does **not** restore `applyPlayingStatus()` when `audioRunning` is already true. `ready` handler (1054–1079) restores playing status when `audioRunning`; the stuck-status bug is the failure path in `requireEngineForAction`, not the ready handler. |
| **VCO waveform icons** do not update after Rand waveforms | `randomizeMorphs` in processor calls `postScreen()` (`froggers-processor.ts` 293–295) — engine path is fine when `wasmReady`. Main-thread gate blocks the message before Play/engine-ready race settles. `renderVcoMorphButtons` (`main.ts` 272–281) only runs on Audio page (`hostPage === 0`); must refresh morph SVG from `lastMorphs` on every screen update including off-Audio. |
| **Ext. In monitor** appears dead; status stuck on **Click Play first** | External peak meter (`#external-meter-fill`) is driven only when `externalEnabled && audioRunning` (`main.ts` 505–507). Meter has **no label** — operators read adjacent `#status` (`index.html` 99–103), which `requireEngineForAction` overwrites. `inputPeak` updates only inside worklet `process()` when graph is pulling (`froggers-processor.ts` 465–520, posts screen every 20 frames). External-before-Play can connect mic without `connectWorkletOutput()` until Play (`setExternalEnabled` 916–973 vs `startAudio` 1173–1174). |

Fixes:

- Unify global-strip engine readiness: Rand waveforms uses same readiness contract as Rand All (`engineActionReady()`), not a stricter Play-only gate.
- `requireEngineForAction`: on failure when `audioRunning`, restore `applyPlayingStatus()` before return.
- After `randomizeMorphs` screen payload: always update `lastMorphs` and refresh VCO morph SVG (even when Audio page not visible).
- External meter: add explicit idle/active labels (`Off` / `Waiting for Play` / level bar); ensure Play + External connects worklet to destination before expecting peaks; trigger immediate `postScreen` after external enable when audio is running.

### Tests and CI

- Extend `sim/AudioPairArEffective_test.cpp` for Crunchy-on-pair-AR on v2 fuego path.
- Update Playwright selectors for full Attack/Release labels.
- Add Playwright: Rand waveforms changes VCO morph SVG on Audio page after Play; external meter active after Play + External on.
- Add desktop v2 UI smoke checks where automatable.

### v2 release gates (carried from `froggerstiga-desktop-v2` §10)

Verification deferred from the desktop-v2 change ships here so UX/doc fixes and merge readiness are tracked in one place:

- Full sim test suite (`make -C sim test`) including `VcoAdsrState_test`, `V2ModSource_test`, `SequencerState_test`
- Build `FroggersTigaV2.app` and VST3/AU v2 artefacts
- Manual desktop: ADSR gated envelopes, global Crunchy, scene rings, gesture lanes, sequencer record/playback (in addition to performance-band UX fixes in this change)
- DAW: MIDI map to `Global/Crunchy`, ADSR param, `Sequencer/BPM`
- Manual web: expanded pages 1–5 + global Crunchy + pair-AR labels/fuego
- Full `cd web && npm run test:e2e` green
- v1 default build regression (`BUILD_DESKTOP_V2=OFF`, `BUILD_VST_V2=OFF`) before merge to `main`

## Capabilities

### New Capabilities

- `operator-quick-dict-performance`: Quick Dict sections for Scenes, Gestures, Sequencer, Crunchy/Crispy/pair-AR rules, with learner-first “how to use” steps.
- `web-transport-morph-meter`: Web engine-readiness contract, Rand waveforms + VCO morph UI sync, external input meter labeling and peak refresh.

### Modified Capabilities

- `sim-operator-doc-parity`: Pair-AR fuego rules (Crunchy yes on v2 hosts, Crispy no); Quick Dict must cover v2 performance controls.
- `audio-pair-ar-engine`: Global Crunchy fuego on pair-AR for `UsesV2Fuego` hosts; enforce full Attack/Release labels at authority.
- `audio-pair-ar-web-ui`: Label sync after `ParamDisplayNames` fix.
- `pair-ar-rotated-desktop-labels`: Desktop v1 band uses full words (already spec'd; enforce via authority).
- `web-mobile-knob-labels`: Update mobile label assertions from `Att.` to `Attack`.
- `web-mobile-external-audio-routing`: Meter state labels; Play+External graph connection before peak display.
- `web-playwright-e2e`: E2E for Rand waveforms and external meter after Play.
- `desktop-v2-page-carousel`: Default window shows all module rows without bank paging when height permits.
- `desktop-v2-global-controls`: Performance band layout; gesture weight sliders; LFO/VCO bus wiring.

## Impact

- **Engine:** `src/core/AudioPairArState.hpp`, `PagedHostIO.hpp`, `DesktopHostIO.hpp`, `sim/V2FuegoStack.hpp`
- **Labels:** `sim/ParamDisplayNames.hpp`, `scripts/generate-host-display.mjs`, `web/src/hostDisplay.generated.ts`
- **Web UI:** `web/src/main.ts`, `web/src/froggers-processor.ts`, `web/index.html`, `web/src/style.css`
- **Desktop v2 UI:** `desktop-v2/Source/ui/DesktopV2ChromeLayout.hpp`, `GlobalStripV2.*`, `PageCarouselComponent.cpp`, `MainComponent.cpp`, new performance-band component
- **Docs:** `QUICK_DICT.md`, `SIM_MANUAL.md`, mirrors, `sim/check_operator_docs_sync.sh`
- **Tests:** `sim/AudioPairArEffective_test.cpp`, `web/e2e/*.spec.ts`, desktop layout tests when automatable

## OMNI rule audit (2026-06-30)

Audit scope: `v2-ux-and-operator-docs` planning artifacts + verified sources (`web/src/main.ts`, `desktop-v2/`, `sim/ParamDisplayNames.hpp`, `AudioPairArState.hpp`, `FroggersV2ControlCore.hpp`). Implementation not started (all tasks open).

### Compliant

| Rule | Finding |
|------|---------|
| Data flow — pair-AR Crunchy | `design.md` decision 1 documents knob → mod → `ApplyGlobal(crunchy)` → smoother → envelope; host IO sets pointer at init alongside page fuego. |
| Label authority | Pair-AR labels flow `ParamDisplayNames::forAudioPairAr` → `generate-host-display.mjs` → host UI; no duplicate label tables in specs. |
| Repetition — docs | One Quick Dict source (`QUICK_DICT.md`) + `sync-help-docs.sh` mirrors; tasks require sync script pass. |
| Repetition — pair-AR fuego | Single `ApplyGlobal` path in `AudioPairArState`; no per-host copy-paste in proposal. |
| Defensive code | Crunchy pointer gated by `UsesV2Fuego(hostKind)`; v1 desktop explicitly excluded in spec scenarios. |
| Spec ownership — Quick Dict | `operator-quick-dict-performance` owns section content; `sim-operator-doc-parity` owns mirror/search parity gates — cross-referenced, not duplicate tables. |

### Gaps closed in this audit (artifact updates)

| Rule | Finding | Resolution |
|------|---------|------------|
| Missing delta specs | Proposal lists `web-transport-morph-meter` (new) and modifies `web-mobile-external-audio-routing` / `web-playwright-e2e` with no delta specs on disk. | Add `specs/web-transport-morph-meter/spec.md`, `specs/web-mobile-external-audio-routing/spec.md`, `specs/web-playwright-e2e/spec.md`. |
| Tasks vs proposal | Web transport / Rand morphs / external meter fixes appear in proposal §Web transport but had no implementation tasks. | Add `tasks.md` §8 Web transport, morph sync, and external meter. |
| Plan language | `design.md` used hedge phrases (`if needed`, `or equivalent`, conditional LFO/VCO wiring). | Rewrite decisions 6 and 3 mitigation; add decision 8 with deterministic web data flow. |
| Data flow — web transport | Engine readiness, morph SVG refresh, and meter peaks were described only in proposal tables. | Capture in `design.md` decision 8 and `web-transport-morph-meter` spec. |
| Verification | Rand morphs and external meter had proposal test bullets but no spec scenarios or task IDs. | Add Playwright scenarios in `web-playwright-e2e` delta; wire tasks 8.x and 6.4. |
| LFO/VCO bus | `MessageIn::Type` has no `LfoSelect` / `VcoSelect` (verified in `FroggersV2ControlCore.hpp`). | Decision 6: hide buttons until types and handlers land; no visible dead controls. |

### Implementation guardrails (from audit)

| Rule | Directive |
|------|-----------|
| Nesting | Web `requireEngineForAction` fix: restore playing status on failure via early return after `applyPlayingStatus()` when `audioRunning` — do not nest status branches deeper than 3 levels. |
| Accumulate then apply | Desktop `visibleRowCount(page, contentHeight)` computes row count once on resize; panels read layout result — do not mutate slot visibility per-row in a loop over shared strip state. |
| One-time helpers | Extract `visibleRowCount` only if trigger count ≥2 (complexity + domain boundary); default inline in layout header per one-time-helper rule. |
| Repetition | Gesture weight sliders for G1/G2: one loop over `kNumGestures` posting `MessageIn::GestureWeight` — no copy-paste lane handlers. |

### Risk surfaced by audit

External peak meter depends on worklet `process()` pulling audio (`inputPeak` in screen payload). Enabling External before Play connects mic to worklet but not worklet to destination until `connectWorkletOutput()` in `startAudio`. Operators must see meter state labels (`Off` / `Waiting for Play` / active bar), not infer state from `#status` alone.
