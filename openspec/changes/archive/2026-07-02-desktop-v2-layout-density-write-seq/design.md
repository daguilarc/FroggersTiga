## Context

Desktop v2 uses a **10px grid unit (u)**. Default window is **128u × 92u (1280×920px)**. Module rows currently place label (9u) + encoder (5u) on the left and mod cell (7u) on the **right edge** via `moduleRowModX(rowWidth) = rowWidth - kModCellW`, leaving **~107u of dead space** per row. Global randomize/Crunchy/Shift occupy a **4u bottom strip** (`kGlobalStripH`) while the sequencer is capped at **13u** (`kSequencerH`) with **2u step cells** that truncate two-digit labels.

Write Seq. capture paths (from `FroggersV2HostBridge` + `SequencerPanelComponent`):
1. **Start Sequence** (armed): immediate `capture(playhead)` → step 0.
2. **Each beat** (armed): `onSequencerStepAdvance` after host increments playhead; `stepLeft = (playhead + len - 1) % len`.
3. **Stopped navigate** (armed): `setEditStep` captures `oldEdit` before changing.

**Bug:** Start Sequence captures step 0, then the **first beat advance** also captures `stepLeft` when playhead=1 → **step 0 again**. Step 1 is first captured when playhead leaves step 1 (second beat). While playing, `m_editStep` does not follow `m_playhead`, so the operator cannot see which step is being written. Steps 10–15 show `...` at 2u cell size.

## Data flow

All pipelines reuse existing structures; no throwaway intermediates.

### Layout constants

`DesktopV2ChromeLayout.hpp` (single source) → row/carousel/sequencer layout code → component bounds. Define constants once in task 1; all layout code reads from the header.

### Write Seq. capture

`FroggersV2ControlCore` live state → `captureSequencerStepSnapshot(out)` → `SequencerState::captureStep(N, out)` → on advance `applySequencerStepSnapshot(currentStep())`. Mod fields follow the same path once `SequencerStepSnapshot` is extended.

### Blank-step factory seed (playback)

`onSequencerStepAdvance`: read `currentStep()` → if `!hasData`, `captureFactoryStepSnapshot(m_steps[playhead])` into the step slot (accumulate) → `applySequencerStepSnapshot(currentStep())` (apply once).

### Rand-seq

UI scope (`Step` / `All steps` / full-step context) + playback state → resolve `targetStep` (D5c) → mutate `m_steps[target]` or full pattern loop → `randomizeSceneEndpointsAndBlend()` once per trigger → apply landed step if playing.

### Rand Mods (per-step snapshots)

`CenterGlobalClusterV2` Rand Mods click → resolve target step(s) from sequencer scope (D5d) → randomize mod source/depth into each target step's snapshot fields → on step recall, `applySequencerStepSnapshot` restores mod routing from the snapshot (not live-global-only while sequencer is active).

## Goals / Non-Goals

**Goals:**
- Pack global controls into the module **center column** on the 10px grid; eliminate per-row horizontal void.
- Remove standalone bottom global strip; give **+5u** vertical budget to sequencer.
- Widen mod cells and step cells so labels are readable at 1280×920 without ellipsis.
- Fix Write Seq. playing semantics and edit-step visibility while armed.
- Paint radio affordance for mutually exclusive toggles (Step/All steps, export format).

**Non-Goals:**
- AdsrPagePanel center-float encoder refactor (separate change).
- Re-introducing bank paging or changing default window size.
- Moving scene band (S1/S2/S3) into center cluster — scene stays in performance band.
- VST hosted editor layout beyond shared center cluster if carousel is shared.

## Decisions

### D1 — Center global cluster column (grid)

**Choice:** Add `CenterGlobalClusterV2` as a **fixed-width center column** in the carousel/module area.

| Constant | Value | Rationale |
|----------|-------|-----------|
| `kModuleRowCenterClusterX` | **15u** (150px) | After label 9u + encoder 5u + gap 1u |
| `kCenterGlobalClusterW` | **15u** (150px) | Fits widest button "Rand waveforms" (13u text + 2u chrome) |
| `kModuleRowModX` | `15u + 15u + 1u` = **31u** | Mod column immediately right of cluster |
| `kModCellW` | **18u** (180px) | Longest mod source label without ellipsis at 1280px |

**Layout (one row, u = 10px):**

```
|0────9u────|9─5u─|1u|15u CENTER CLUSTER|1u|18u MOD|flex pad|
  label      enc  gap  Rand All…Shift      gap  dropdown
```

Center cluster is **one component** spanning the submodule viewport height (not duplicated per row). Submodule row layout only positions label, encoder, mod; cluster is a sibling in `PageCarouselComponent`.

**Rationale:** Operator asked to fill the white void; a single center column matches Eurorack “utility column” mental model and avoids repeating globals on every row.

### D2 — Remove bottom global strip (standalone)

**Choice:** `MainComponent::resized` stops allocating `m_globalStrip`. Controls migrate to `CenterGlobalClusterV2`.

**Vertical budget transfer:**

| Region | Before | After |
|--------|--------|-------|
| Global strip + gap | 4u + 1u = **5u** | **0u** |
| Sequencer | 13u | **18u** (+5u) |
| Carousel | unchanged flex | +0u (strip removed from bottom) |

**Rationale:** Sequencer step grid is the beneficiary; operator explicitly wants more sequencer space.

### D3 — Submodule randomize buttons

**Choice:** Keep **Randomize** / **Randmod** on the **left** above the encoder viewport (page-local actions). Only **global** actions move to center cluster.

**Alternative rejected:** Move all randomize into center — mixes page vs global scope.

### D4 — Step grid and performance band label widths

| Constant | Before | After |
|----------|--------|-------|
| `kSequencerStepCellSize` | 2u (20px) | **3u** (30px) |
| `kPerfMarblesColW` | 3u (30px) | **6u** (60px) for "S&H 1" / "S&H 2" |

16 steps × 3u = 48u grid width — fits in 18u-tall sequencer panel with toolbar 3u.

### D5 — Write Seq. playing semantics

**Choice:**

1. **Remove duplicate step-0 capture:** On Start Sequence, capture step 0 once. On first `onSequencerStepAdvance`, skip capture if `m_writeSeqJustStarted` flag set (clear after first advance).
2. **Edit step follows playhead while armed + playing:** When Write Seq. armed and `m_playing`, set `m_editStep = m_playhead` on each advance and on Start Sequence so orange highlight tracks the writable step.
3. **Stopped mode unchanged:** Navigate with prev/next or click step → capture on leave via `setEditStep`.
4. **Capture flash:** Brief highlight pulse on step cell when `captureLiveToSequencerStep` succeeds (UI-only).

**Alternative rejected:** Capture to `m_editStep` instead of step-left while playing — breaks beat-synced multitrack workflow.

### D5b — Factory-seed blank steps during playback

**Choice:** During `FroggersV2HostBridge::onSequencerStepAdvance`, before applying the current step snapshot, seed blank steps in the step array, then apply once.

**Implementation rule:**
- If `m_host.m_sequencer.currentStep().hasData == false`, call
  `m_core.captureFactoryStepSnapshot(m_host.m_sequencer.m_steps[m_host.m_sequencer.m_playhead])`
  to write factory defaults into the playhead step slot.
- Then call `m_core.applySequencerStepSnapshot(m_host.m_sequencer.currentStep())`.

**Rationale:** Story 2 requires that enabling the step sequencer before audio still produces factory-derived presets when audio starts, instead of applying zero-initialized step snapshot payloads.

### D5c — `Rand-seq` Step scope targeting while playing

**Choice:** `FroggersV2ControlCore::onRandSequencerStep` SHALL interpret `kRandSeqScopeStep` as:
- targetStep = `m_sequencer->m_playhead` when `m_sequencer->m_playing == true`
- targetStep = `m_sequencer->m_editStep` when `m_sequencer->m_playing == false`

Then the system SHALL apply the existing scope payload rules (randomize scene slots, clear gestures, set `hasData = true`, leave gate unchanged for Step/All steps scopes).

**Rationale:** Story 3 requires deterministic step-by-step randomization while the clock is running. This must target the step being executed by the sequencer clock.

### D5d — Rand Mods per-step snapshot writes

**Choice:** `Rand Mods` from the center cluster SHALL write mod source and depth into step snapshot fields (extended in `SequencerStepSnapshot`), not only live `ParamState` mod routes.

Target step resolution mirrors D5c:
- **Step** sequencer scope + playing → target `m_playhead`
- **Step** sequencer scope + stopped → target `m_editStep`
- **All steps** sequencer scope → every step in `0..patternLength-1`

Each target step receives independent randomized mod assignments. Step recall via `applySequencerStepSnapshot` restores stored mod routing when the playhead enters that step.

**Rationale:** Stories 2 and 3 require per-step mod routing in snapshots. Live-only `EnqueueRandomizeAllMod` does not satisfy sequencer recall semantics.

### D6 — Radio L&F

**Choice:** Override `DesktopV2LookAndFeel::drawToggleButton` for components with `radioGroupId != 0` to paint circular radio buttons. Applies to Step/All steps and Audio export format. Write Seq. and Shift keep checkbox drawing.

### D7 — Hosted editor layout

**Choice:** `HostedMainComponentV2` SHALL use the same `CenterGlobalClusterV2` in the carousel area and SHALL NOT allocate a bottom `GlobalStripV2` row. Remove `m_globalStrip` bounds from hosted layout the same way as standalone.

**Rationale:** Hosted and standalone share carousel chrome; a thin hosted strip duplicates globals and breaks density parity at 128u min width.

## Helper extraction review

| Extraction | Trigger count (>=2) | Domain boundary | Complexity | Contract | Side effects | Local scope | Verdict |
|------------|---------------------|-----------------|------------|----------|--------------|-------------|---------|
| `CenterGlobalClusterV2` (from `GlobalStripV2`) | Yes (boundary + contract) | Yes — center column host | No | Yes — same `DesktopHostIO` callbacks | Yes — bus mutations | Yes — `desktop-v2/Source/ui/` | **Extract** |
| `m_writeSeqJustStarted` on `SequencerState` | No (single flag, 2 branches) | No | No | N/A | N/A | N/A | **Inline** in bridge advance path |
| Factory seed in `onSequencerStepAdvance` | No | Yes — advance pipeline | No | Yes | Yes | Yes | **Inline** guard before apply |

## Risks / Trade-offs

| Risk | Mitigation |
|------|------------|
| Center cluster crowds narrow hosts | Constants in `DesktopV2ChromeLayout`; VST uses same column math at 128u min width |
| Mod column still truncates on very long names | 18u width + tooltip; measure longest catalog label |
| Write-seq flag edge cases | Tests: start + 3 beats → steps 0,1,2 each `hasData` once |
| HostedMainComponentV2 still references GlobalStripV2 | Task 7.1: migrate hosted to `CenterGlobalClusterV2`; remove bottom strip |

## Migration Plan

1. Land chrome constants + `CenterGlobalClusterV2` (move widgets, same callbacks).
2. Repack submodule rows + remove global strip from `MainComponent`.
3. Bump sequencer/mod/step constants; fix Write Seq. bridge + panel.
4. L&F radio paint; docs sync.
5. Manual QA at 1280×920.
