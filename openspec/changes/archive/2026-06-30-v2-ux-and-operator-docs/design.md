## Context

`froggerstiga-desktop-v2` implemented `FroggersV2ControlCore` scene/gesture/sequencer semantics and desktop chrome, but operator-facing docs and several UI surfaces lag the engine.

**Verified current state:**

```
┌─────────────────────────────────────────────────────────────┐
│ Transport row                                               │
├─────────────────────────────────────────────────────────────┤
│ GlobalStripV2 (44px) — Rand*, Crunchy, Shift, G1/G2 toggles,│
│   LFO/VCO (unwired), S1–S3, blend, seq transport            │
├─────────────────────────────────────────────────────────────┤
│ ScopeGrid (88px)                                            │
├─────────────────────────────────────────────────────────────┤
│ PageCarousel — prev [far left] … title … [far right] next   │
│   SubmodulePagePanel — 4 visible encoder rows + bank paging │
├─────────────────────────────────────────────────────────────┤
│ SequencerPanel (toggle, 132px)                              │
└─────────────────────────────────────────────────────────────┘
```

- `QUICK_DICT.md`: parameter glossary only; no Scene/Gesture/Sequencer (111 lines, ends at Field-only section).
- `docs/sim-manual.md`: Scene/Gesture/Sequencer documented starting ~line 216; not mirrored in Quick Dict.
- Pair-AR path: `AudioPairArState::blendKnob` → mod only; engine reads `getEffectiveSmoothed` in `FroggersEngine.hpp` ~693–701.
- Page fuego path: `Page::GetParamV2` → `V2FuegoStack` with `m_globalCrunchy` pointer from host IO.
- `UsesV2Fuego(Web | DesktopV2 | VstV2)` in `SimModSource.hpp` line 34–37.

## Goals / Non-Goals

**Goals:**

1. Operators can learn Scene, Gesture, and Sequencer workflow from **Quick Dict** without opening the full manual.
2. Web Audio pair-AR knobs show **Attack** / **Release** full labels and respond to **global Crunchy** on v2 fuego hosts.
3. Desktop v2 default window shows **all module rows** (8 Audio, 10 expanded/ADSR) per carousel spec.
4. Desktop v2 exposes **gesture weight sliders** and relocates scene/gesture/sequencer controls to a dedicated **performance band** below scopes.
5. Carousel prev/next flank the module title.
6. LFO/VCO global buttons are hidden until bus types and handlers exist (Decision 6).

**Non-Goals:**

- Web scene/gesture/sequencer UI (explicitly excluded in `web-v2-parameter-subset`).
- Crispy fuego on pair-AR (manual §Crispy: pair-AR moddable, not Crispy-scrambled).
- v1 desktop pair-AR band layout changes beyond label authority sync.
- Hardware Field encoder bank paging (Daisy) — desktop window has room; hardware keeps `BankSlot` pattern.

## Decisions

### 1. Pair-AR fuego: full musical row stack on `UsesV2Fuego` hosts

**Choice:** `AudioPairArState` calls `Page::ApplyV2MusicalFuego` (Crunchy + Audio-page Crispy via `ApplyMusicalRow`) post-mod when host uses v2 fuego.

**Rationale:** Operator requirement: Crispy affects web pair-AR Attack/Release. Reuses page row fuego path — no second crispy implementation.

**Data flow after change:**

```
pair-AR knob → mod blend → Page::ApplyV2MusicalFuego → RuntimeParam smoother → PairArEnvelope
```

Host IO sets audio page pointer in `init` alongside page `ConfigureV2Fuego`.

### 2. Label authority: fix `ParamDisplayNames`, not UI strings

**Choice:** Update `forAudioPairAr` to match `audio-pair-ar-engine` spec table; regenerate host display.

**Rationale:** Spec already requires Attack 1+2 etc.; abbreviation is implementation drift.

### 3. Desktop v2 visible rows: derive from window height, not fixed bank of 4

**Choice:** Replace fixed `kVisibleEncoderSlots = 4` with `visibleRowCount(page, contentHeight)`:

- Audio (page 0): 8 rows
- Modules 1–5: 10 rows (`V2ParamDisplayNames::kV2ExpandedNumRows`)
- ADSR (page 6): 10 rows

Remove bank prev/next when `visibleRowCount == rowCountForPage(page)`. Retain bank controls only when content height cannot fit all rows (hosted VST min height 720, resize path).

**Rationale:** `desktop-v2-page-carousel` Scenario “Expanded FX pages show ten rows” requires exactly ten visible, not four with paging at 1280×820.

**Implementation:** Add `kMaxVisibleEncoderSlots = 10` to layout header; `SubmodulePagePanel` and `AdsrPagePanel` allocate up to 10 ring rows; control core `visibleCount` set from layout on resize.

### 4. Performance band component

**Choice:** New `PerformanceBandV2` between `ScopeGridComponent` and `PageCarouselComponent` in `MainComponent` layout:

| Zone | Controls |
|------|----------|
| Scene | Label “Scene”, S1/S2/S3, horizontal blend slider |
| Gestures | G1/G2 toggles + horizontal weight sliders (0–1) posting `MessageIn::GestureWeight` |
| Sequencer | Play, Rec arm, BPM, length (duplicate from strip or move entirely) |

`GlobalStripV2` keeps Rand*, Crunchy, Shift, Rand Resample; drops scene/gesture/sequencer duplicates after band is wired.

**Rationale:** Spec requires gesture value controls; 44px strip cannot host sliders. Matches user feedback: controls above module panel, below scopes.

### 5. Carousel header layout

**Choice:** `resized()` order: `[← 28][4][Title flex][4][→ 28]` with title centred in remaining space (arrows adjacent to title block, not panel edges).

### 6. LFO/VCO buttons

**Choice:** Hide LFO and VCO buttons in `GlobalStripV2` until `MessageIn::Type` gains `LfoSelect` and `VcoSelect` entries **and** `FroggersV2ControlCore` handles them. Verified 2026-06-30: current enum ends at `Clock` — no LFO/VCO select types.

**Follow-up (separate change or tail of this change after enum extension):** Wire `onClick` to post the new message types and update scope/mod grid focus per Sheaf semantics.

**Implementation gate:** grep `MessageIn::Type` before showing buttons; visible buttons without handlers is a spec violation (`desktop-v2-global-controls`).

### 7. Quick Dict structure

Add sections in learning order after Transport:

1. **Scenes** — S1/S2/S3 endpoints, blend slider, global storage, encoder ring L/R arcs
2. **Gestures** — G1/G2 select, weight slider, turn ring while selected, badges, Rand All clears selection
3. **Sequencer** — BPM, length, play, record arm, step gates in panel, per-step scene/gesture snapshot on record
4. **Crunchy & Crispy & pair-AR** — explicit matrix (table)

Keep entries short; link “Full guide → Manual” at top.

### 8. Web transport, morph sync, and external meter

**Choice:** Unify global-strip actions on `engineActionReady()` (`engineReady || workletNode && (audioRunning || transportIntentPlaying)`). Rand waveforms (`#rand-morphs`) uses the same readiness contract as Rand All (`#rand-all`) — no stricter Play-only gate.

**Status line contract:** `requireEngineForAction()` on failure sets `"Click Play first"` **only** when `!audioRunning`. When `audioRunning` is true, call `applyPlayingStatus()` before returning false so transport copy is not clobbered.

**Morph SVG contract:** `onScreen` always updates `lastMorphs` from processor payload. `renderVcoMorphButtons` refreshes SVG from `lastMorphs` whenever morph data changes, even when `hostPage !== 0` (buttons stay hidden off Audio; SVG is current when the user returns).

**External meter contract:** Add `#external-meter-label` with explicit states: `Off` (external disabled), `Waiting for Play` (external on, worklet not pulling), active level bar (external on and processor running). Do not route meter state through `#status`.

**Data flow after change:**

```
User click (#rand-morphs | #rand-all)
  → engineActionReady()?
      no + audioRunning → applyPlayingStatus(); abort
      no + !audioRunning → status "Click Play first"; abort
      yes → send(worklet message)
Worklet randomizeMorphs → postScreen(morphs, inputPeak, …)
Main onScreen → lastMorphs := morphs; renderVcoMorphButtons; renderInputMeter

External enable → initWorklet + mic → worklet input
Play → connectWorkletOutput → setRunning(true) → process() posts inputPeak
onScreen → renderInputMeter(peak, externalEnabled && processorRunning)
```

## Risks / Trade-offs

| Risk | Mitigation |
|------|------------|
| Pair-AR Crunchy changes timbre on web Audio | Unit test + manual note; matches documented intent |
| Taller chrome reduces module panel height | Raise `kDefaultHeight` from 820 to 880 in `DesktopV2ChromeLayout` during performance-band integration; bump `kMinHeight` in the same edit |
| Duplicate seq controls during migration | Remove from global strip in same PR as performance band |
| LFO/VCO bus types absent | Hide buttons until `LfoSelect` / `VcoSelect` exist in `MessageIn::Type` and control core (decision 6) |
| Quick Dict length | Cap v2 performance sections ~40 lines; parameter tables unchanged |

## Migration Plan

1. Engine + labels + tests (sim) — no UI dependency.
2. Regenerate host display; rebuild WASM for web.
3. Web transport / morph sync / external meter (`tasks.md` §6).
4. Desktop v2 layout refactor (performance band, row count, carousel).
5. Docs sync script + `check_operator_docs_sync.sh`.
6. Playwright + manual QA checklist from tasks.md (§7).
7. v2 release gates from `froggerstiga-desktop-v2` §10 (tasks.md §8): sim suite, desktop/VST builds, DAW MIDI, full e2e, v1 regression before merge.

Rollback: revert `AudioPairArState` Crunchy pointer (pair-AR returns to pre-fuego); UI layout is additive component swap.

## Open Questions

None blocking — Crunchy-on-pair-AR decision is locked to global-only on v2 fuego hosts per proposal.
