## Why

Desktop v2 at 1280×920 wastes **~107u** of horizontal space in every module row (9u label + 5u encoder left-flush, 7u mod cell right-flush via `moduleRowModX(rowWidth)`), while Rand All / Crunchy / Shift sit in a dedicated bottom strip that steals vertical budget from the sequencer. Mod source and step labels truncate (`...`) at grid constants chosen before text measurement. **Write Seq.** is functionally opaque after the first step: duplicate step-0 capture, playhead vs edit-step divergence while playing, and 2u step cells that cannot show two-digit step numbers.

Operator QA on the current build confirms layout density and write-seq UX are blocking daily use. This change packs the module center void and fixes sequencer capture semantics.

## Data flow (OMNI)

| Pipeline | Enters | Transforms | Exits |
|----------|--------|------------|-------|
| Layout density | `DesktopV2ChromeLayout.hpp` constants (single source) | Row/carousel/sequencer layout reads constants once | Component bounds; mod at `kModuleRowModX`, not right-flush |
| Write Seq. | Live `FroggersV2ControlCore` state | `captureSequencerStepSnapshot` → `m_steps[N]` | `applySequencerStepSnapshot` on recall |
| Blank step seed | `hasData == false` on advance | `captureFactoryStepSnapshot` into playhead slot (accumulate) | `applySequencerStepSnapshot(currentStep())` once (apply) |
| Rand-seq | Scope + play/stop state | Resolve `targetStep`; mutate `m_steps[target]` or full-pattern loop; `randomizeSceneEndpointsAndBlend()` once | Endpoints + step snapshot(s) written |
| Rand Mods | Scope + play/stop state | Randomize mod fields into target snapshot(s) | Mod routing recalled per step on playback via `applySequencerStepSnapshot` |
| Hosted layout | Same constants + `CenterGlobalClusterV2` | `HostedMainComponentV2` removes bottom strip; carousel hosts center cluster | Hosted/editor parity with standalone at 128u min width |

## What Changes

- **Center global cluster (10px grid):** Move Rand All, Rand Mods, Rand waveforms, Rand Resample, Crunchy encoder, and Shift from `GlobalStripV2` into a new **center column** in the module/carousel area (the void between encoder rings and mod dropdowns). Remove the bottom global strip row from standalone and hosted layouts.
- **Module row repack:** Replace `moduleRowModX(rowWidth)` right-flush with **label | encoder | center cluster (shared) | mod** layout. Widen mod cells (7u → 18u) and performance-band marbles labels (3u → 6u); row label width stays 9u. No mod-source or step-number ellipsis at default 1280×920.
- **Sequencer height reclaim:** Transfer bottom strip height (`kGlobalStripH` + gap = **5u / 50px**) to `kSequencerH` so the step grid gains vertical space.
- **Step grid readability:** Increase `kSequencerStepCellSize` to **3u** minimum so steps 10–15 display two-digit labels at 1280×920.
- **Write Seq. fix:** Correct playing capture semantics (step-left after advance without double-capturing step 0 on Start Sequence + first beat); while Write Seq. armed and sequence playing, **edit step follows playhead** and beat capture targets the step being left; stopped mode keeps navigate-to-save. Add UI feedback when a step is captured.
- **Radio affordance (scope/export only):** Step / All steps and Audio export format toggles use radio L&F painting in `DesktopV2LookAndFeel` (Write Seq. and Shift remain true toggles).
- **Per-step mod routing (required):** Extend `SequencerStepSnapshot` with mod source + depth per page/row; capture/restore on Write Seq. and include in Rand-seq / Rand Mods scope behavior.
- **Story-driven specification:** Three operator stories plus decision-variant axes (clock order, randomization scope, write-target policy, per-step mod policy).

## User Stories (Operator Acceptance)

1. **Story 1 — Play-only performance (sequencer off):**
   - Operator presses `Play` to start continuous sound.
   - Operator never enables sequencer playback (`Start Sequence` stays off).
   - Operator uses everything else: scene S1/S2/S3, Rand All, Rand Mods, Rand waveforms, Rand Resample, Crunchy, Shift, and module mod menus.
   - Sequencer grid remains visible; sequencer `m_playing` stays false, so the playhead does not advance and no step snapshots are applied to the control core.

2. **Story 2 — Sequencer before audio, then full-sequence randomization:**
   - Operator enables sequencer clock (`Start Sequence` on) while audio is still off.
   - Operator does not arm Write Seq. during this story.
   - When audio starts, sequencer begins advancing steps on beats.
   - Each step starts from **factory default step payload** on first advance into a blank step: `captureFactoryStepSnapshot` seeds `m_steps[S]`, then `applySequencerStepSnapshot` applies it (factory knob defaults, crunchy/gesture defaults, gate defaults per `captureFactoryStepSnapshot` rules).
   - Operator selects **All steps** scope and triggers `Rand-seq` randomization.
   - Expected payload after `Rand-seq` (All steps):
     - Every step in `0..patternLength-1` has `hasData = true`.
     - Every step has randomized scene slot centers for all page/rows covered by `captureSequencerStepSnapshot`.
     - Gesture weights are cleared; gate is unchanged (Rand-seq Step/All steps scope policy).
   - Operator triggers `Rand Mods`; acceptance criteria:
     - Mod routing is randomized **per step** for the active sequencer scope (All steps → every step in `0..patternLength-1`; Step scope → targeted step per `randomization-target-policy`).
     - Each step snapshot stores mod source assignments and depths for all page/rows in the snapshot model; playhead recall restores step `N` mod routing from `m_steps[N]`.

3. **Story 3 — Audio first, then sequencer, then step-by-step randomization:**
   - Operator starts audio with `Play`.
   - Operator enables sequencer playback (`Start Sequence` on).
   - While sequencer is playing, operator triggers `Rand-seq` in **Step** scope.
   - Expected payload after each `Rand-seq` (Step scope):
     - Only the targeted step is overwritten (other steps keep their previous snapshots).
     - Targeting is deterministic: during playback it targets the step being executed by the clock; when stopped it targets `m_editStep`.
   - Operator triggers `Rand Mods`; acceptance criteria match Story 2 mod-randomization-policy (per-step snapshot writes, scope-driven target selection, no live-global-only routing while sequencer is active).

## Decision variants (explicit axes for iterative user-story expansion)

These axes support iterative story expansion; each variant maps to a deterministic expected outcome and test case:

- **audio-start-order**: sequencer on before audio vs audio on before sequencer (same axis as `sequencer-start-order`)
- **randomization-scope**: `Step` vs `All steps` vs `Full step` (context-menu vs dice)
- **randomization-target-policy**: when playing, randomize the playhead step; when stopped, randomize `m_editStep`
- **write-target-policy**: Write Seq. capture is step-left on beat advance; Start Sequence captures playhead once; first advance skips duplicate step-0 capture
- **mod-randomization-policy**: per-step mod routing stored in snapshots (required); Rand Mods + Write Seq. capture/restore mod assignments per step

### Verified current-state facts used in these stories

- VCO startup is **not** 30 Hz by default. `V1VO/V2VO/V3VO` defaults are `0.35/0.4/0.45` normalized in `FroggersEngine::Config`, then mapped exponentially over `20 Hz .. 20 kHz`.
- Parameter startup is **not mostly zero**. Reverb/filter defaults include mid values (`0.5` on several rows), while drive rows and PM rows default to `0.0`.
- ADSR release defaults are `R1/R2/R3 = 0.2` in `V2EngineSetup::configureAdsrPage`.
- Current `SequencerStepSnapshot` stores scene centers + crunchy + gestures + gate only — **this change extends it** so mod sources and depths are per-step.

## Capabilities

### New Capabilities

- `desktop-v2-center-global-cluster`: Center-column placement of global randomize + Crunchy + Shift; bottom strip removal on standalone and hosted; grid constants for center column width and mod column X.

### Modified Capabilities

- `desktop-v2-global-controls`: Global randomize/Crunchy/Shift live in center cluster, not bottom strip; `kGlobalStripH` removed from standalone and hosted chrome budgets.
- `desktop-v2-mod-source-grid`: Mod column X derived from center cluster right edge; minimum mod cell width from longest source label at 1280px.
- `desktop-v2-page-carousel`: Carousel/module area hosts center cluster component; vertical budget shifts from global strip to sequencer.
- `desktop-v2-sequencing`: Write Seq. playing capture semantics, edit-step/playhead coupling while armed, step cell minimum size, capture feedback.
- `desktop-v2-grid-layout`: New chrome constants (`kCenterGlobalClusterW`, `kModuleRowCenterClusterX`, updated `kSequencerH`, `kSequencerStepCellSize`, `kModCellW`).
- `sim-operator-doc-parity`: Document center cluster and updated Write Seq. workflow.
- `desktop-v2-control-core`: Story-2/3 randomization semantics, per-step mod fields in snapshots, Rand-seq/Rand Mods target selection while playing, factory seed on blank-step advance.

## Helper extraction (OMNI)

| Extraction | Verdict | Rationale |
|------------|---------|-----------|
| `CenterGlobalClusterV2` from `GlobalStripV2` | **Extract** | Domain boundary + explicit callback contract (≥2 triggers); see `design.md` Helper extraction review |
| `m_writeSeqJustStarted` flag | **Inline** | Single flag, two branches; no helper |
| Factory seed guard in `onSequencerStepAdvance` | **Inline** | Named pipeline step; accumulate-then-apply in place |

## Verification (OMNI)

After each implementation slice, verify:

1. Layout constants defined only in `DesktopV2ChromeLayout.hpp`; no duplicate magic numbers in layout code.
2. Snapshot mutations accumulate into `m_steps[N]`, then apply once per advance/recall.
3. `ControlCoreBridge_test` covers write-seq dedup, blank-step factory seed, Rand-seq/Rand Mods target selection.
4. Manual QA at 1280×920: center cluster visible, no bottom strip (standalone + hosted), steps 10–15 show two-digit labels, Write Seq. steps 0–2 each `hasData` after three beats.

## Impact

- `desktop-v2/Source/ui/DesktopV2ChromeLayout.hpp` — center column + widened mod/step constants; `kGlobalStripH` deprecated for standalone and hosted.
- `desktop-v2/Source/ui/CenterGlobalClusterV2.*` (new) — hosts controls moved from `GlobalStripV2`.
- `desktop-v2/Source/ui/GlobalStripV2.*` — removed from standalone and hosted layouts; widgets move to `CenterGlobalClusterV2`.
- `desktop-v2/Source/ui/SubmodulePagePanel.*` — label/encoder/mod row layout; mod at `kModuleRowModX` (18u wide).
- `desktop-v2/Source/ui/PageCarouselComponent.*` — lay out center cluster beside submodule viewport.
- `desktop-v2/Source/MainComponent.cpp` — remove bottom global strip; taller sequencer bounds.
- `desktop-v2/Source/HostedMainComponentV2.cpp` — remove bottom global strip; add center cluster to carousel.
- `desktop-v2/Source/ui/SequencerPanelComponent.*` — write-seq UX fixes, larger step grid.
- `desktop-v2/Source/control/FroggersV2HostBridge.*` — playing capture dedup / semantics.
- `desktop-v2/Source/ui/PerformanceBandV2.*` — wider marbles labels.
- `desktop-v2/Source/ui/DesktopV2LookAndFeel.*` — radio painting for radio-group toggles.
- `desktop-v2/Source/control/FroggersV2ControlCore.*` — snapshot mod fields, factory seed on advance, Rand-seq/Rand Mods target selection.
- `desktop-v2/tests/ControlCoreBridge_test.cpp` — write-seq playing capture tests for steps 1+; blank-step factory seed; Rand-seq/Rand Mods target selection.
- `QUICK_DICT.md` + mirrors.
