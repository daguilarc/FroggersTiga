## Why

Desktop v2 boots and runs, but operator QA on `froggerstiga-desktop-v2` showed **horizontal layout failure**: the oscilloscope consumed the transport row, performance-band controls truncated (S1/S2, Record, slider labels), module encoder rows floated in empty space, and sequencer controls were split from the grid they operated on. Archived `desktop-v2-ux-and-sequencer` also introduced **wrong product choices** without operator sign-off: audio transport labeled **Engine** (should stay **Play**), Rand-seq **Pattern** scope that only filled blank steps (operator intent: **all steps** vs **edit step only**), and checkbox affordances for mutually exclusive scopes.

## What Changes

- **BREAKING (docs + UI copy):** Revert standalone audio transport label **Engine** → **Play**. Keep sequencer transport as **Start Sequence** / **Stop Sequence** (the actual duplicate-Play fix).
- **BREAKING (behavior):** Rand-seq **All steps** scope (UI rename from **Pattern**) randomizes **every step in pattern length** (overwrites steps with `hasData == true`). **Step** scope randomizes **edit step only**. Remove blank-only Pattern behavior from core, tests, and docs.
- **Edit-step invariant (confirmed):** Exactly one edit step is always selected (`m_editStep` default `0`; wraps via ←/→; clamped on pattern-length change). No deselect-all path; no third scope option required.
- **Transport row:** Cap VCO EF scope width; **Record audio** button only (v1 `RecordButton` — red circle + label) in transport. **WAV / MP3 / FLAC / OGG** export format lives in the **Audio menu** (`AudioSettingsComponent`, opened by **Audio** button) — not in the transport row. Play/Stop/MIDI/Audio + **Record audio** — not sequencer write.
- **Write Seq.:** Sequencer toolbar toggle **Write Seq.** (renamed from Record); captures step snapshots when stopped (on edit-step change) and when playing (on Start Sequence + each beat advance). Fixes step-0 capture bug.
- **Performance band:** Fix truncation (wider scene buttons, readable S&H labels, L/R blend labels); move **BPM**, **Steps**, **Start Sequence** to the **sequencer toolbar** adjacent to the step grid.
- **Module rows:** Left-align label | encoder | mod with grid-derived gaps (no `withSizeKeepingCentre` dead space in `SubmodulePagePanel`).
- **Mod cells:** Fixed cell height; None vs assigned source uses same outer dimensions.
- **Sequencer toolbar:** Label Rand-seq dice; **radio buttons** for Step vs All steps scope beside dice; prev/next use `SequencerState::prevEditStep` / `nextEditStep` (no duplicated wrap math).
- **Shared types:** `src/core/ExportFormat.hpp` — shared enum so v1 `RecordExportCluster` does not include v1 `AudioEngine.h` when built into v2 (redefinition fix).
- **Docs:** Update `QUICK_DICT.md` + mirrors — Play/Stop, Rand-seq scope, layout glossary. No release version bump.
- **VST:** Same chrome/sequencer fixes via shared components; still no Play row (DAW owns audio transport).

## Capabilities

### New Capabilities

- `desktop-v2-chrome-layout`: Transport row scope budget, module row horizontal geometry, mod cell fixed footprint, extend shared `DesktopV2ChromeLayout` constants (including transport control widths/gaps — not helper-local magic numbers).
- `desktop-v2-performance-band-chrome`: Scene/gesture/marbles band sizing and labeling without truncation.
- `desktop-v2-sequencer-toolbar`: Sequencer-adjacent transport (BPM, Steps, Start Sequence, **Write Seq.**), Rand-seq dice + scope radios, edit-step toolbar.
- `desktop-v2-audio-export`: **Record audio** in transport; export format (WAV/MP3/FLAC/OGG) in **Audio menu**; wire v2 `AudioEngine` recording API + persisted format preference.

### Modified Capabilities

- `desktop-v2-sequencing`: Edit-step invariant, toolbar placement, Rand-seq scope, **Write Seq.** stopped + playing capture semantics.
- `desktop-v2-scope-visualization`: Maximum scope width in standalone transport row; flex remainder to control clusters.
- `desktop-v2-control-core`: `onRandSequencerStep` All-steps scope overwrites all pattern steps.
- `sim-operator-doc-parity`: Play (not Engine), Rand-seq Step vs All steps, sequencer toolbar map.

## Impact

- `src/core/ExportFormat.hpp` — shared `ExportFormat` enum (v1/v2 `AudioEngine`, `RecordExportCluster`).
- `desktop-v2/Source/ui/DesktopV2ChromeLayout.hpp` — extended constants; **must** own transport button widths/gaps; **`kRecordButtonMinWidth`** (Record audio only — smaller than full cluster).
- `desktop-v2/Source/ui/DesktopV2TransportLayout.hpp` — `layoutStandaloneTransportRow`; consume chrome constants only.
- `desktop-v2/Source/MainComponent.cpp` — transport row: Play, **Record audio** (`RecordButton` only), scope cap.
- `desktop-v2/Source/AudioEngine.*` — v1 recording API; `m_recorder` fed in output path; **`m_exportFormat`** (or equivalent) read on export.
- `desktop-v2/Source/AudioSettingsComponent.*` — **Export format** radio row (WAV/MP3/FLAC/OGG); writes format to `AudioEngine`.
- `desktop/Source/RecordButton.*` — transport record control (not full `RecordExportCluster` with format toggles in row).
- `desktop-v2/Source/control/FroggersV2HostBridge.*` — write-seq capture; `captureLiveToSequencerStep` / `recallSequencerStep`.
- `desktop-v2/Source/ui/SequencerPanelComponent.*` — toolbar migration, Write Seq., Rand-seq; remove or lay out orphan `m_title`.
- `desktop-v2/Source/ui/PerformanceBandV2.*` — slim band.
- `desktop-v2/Source/ui/SubmodulePagePanel.cpp` — left-anchored columns; optional second-pass `setBounds` per D8.
- `desktop-v2/Source/ui/ModSourceCell.cpp` — `kModCellHeight`; hoist label-strip height to chrome constants.
- `desktop-v2/Source/control/FroggersV2ControlCore.cpp` — All-steps overwrite (no `hasData` skip).
- `sim/SequencerState.hpp` — `m_writeSeqArm` (renamed from `m_recordArm`).
- `desktop-v2/tests/ControlCoreBridge_test.cpp` — All-steps + write-seq tests.
- `QUICK_DICT.md`, mirrors.
- `HostedMainComponentV2` — shared sequencer panel; no audio Record row.

**Out of scope (follow-up change):** `AdsrPagePanel.cpp` still uses `withSizeKeepingCentre` for encoder rows — same UX class as pre-fix SubmodulePagePanel.

## Evidence — verified against tree (2026-07-01 post-apply)

| Claim | Status | Verification |
|-------|--------|----------------|
| Default edit step 0 | **Done** | `SequencerState.hpp` `m_editStep = 0` |
| All-steps overwrites hasData | **Done** | `FroggersV2ControlCore.cpp` L739–744 — no skip |
| Step-0 write-seq | **Done** | `FroggersV2HostBridge.cpp` step-left + panel Start Sequence capture; tests pass |
| Engine → Play | **Done** | `MainComponent.h` `m_play{"Play"}` |
| Scope cap 320 | **Done** | `DesktopV2TransportLayout.hpp` + `kTransportScopeMaxWidth` |
| Sequencer toolbar owns clock | **Done** | BPM/Steps/Start Sequence/Write Seq. in `SequencerPanelComponent` |
| Recording API + recorder feed | **Done** | `AudioEngine.cpp` `startRecording`, `m_recorder.appendStereo` |
| Submodule left-anchor | **Done** | `SubmodulePagePanel.cpp` column offsets; no `withSizeKeepingCentre` |
| ExportFormat redefinition fix | **Done** | `src/core/ExportFormat.hpp`; `RecordExportCluster.h` includes it |
| Record formats in Audio menu | **Done** | `AudioSettingsComponent` export format row; `AudioEngine::exportFormat` |
| Transport record control | **Done** | `RecordButton` only in transport; `kRecordButtonMinWidth` |
| Transport widths in chrome header | **Done** | `kTransportPlayStopW`, `kTransportSettingsW`, `kTransportGapSm/Md` in `DesktopV2ChromeLayout.hpp` |
| prev/next wrap repetition | **Done** | `SequencerState::wrappedEditStep(±1)` → `setEditStep(target)` |
| Orphan `m_title` | **Done** | Removed from `SequencerPanelComponent` |
| Submodule accumulate-then-apply | **Done** | Two-pass row layout in `SubmodulePagePanel::layoutRows` |
| `kModLabelStripH` in chrome header | **Done** | `DesktopV2ChromeLayout::kModLabelStripH` |
| Manual QA 1280 px | **Open** | Task 10.2 — operator |
| Full `ControlCoreBridge_test` green | **Open** | Pre-existing `test_pair_ar_gate_policy` failure after change-scoped tests pass |

## OMNI rule audit — planning (2026-07-01)

Initial audit scope: planning artifacts + pre-implementation code. Gaps closed in `design.md`, `tasks.md`, and specs (non-goals contradiction, hedge terms, terminology, OMNI.1–8 gates).

## OMNI rule audit — post-implementation (2026-07-01)

Audit scope: implemented tree after `/opsx:apply`, subagent-driven development, and Section 11 OMNI fixups. **Code matches operator UX and OMNI guardrails.** Remaining gates: operator manual QA (10.2); pre-existing full-suite test failure.

### Compliant (implemented)

| Rule | Finding |
|------|---------|
| Data flow — write-seq | Step-left capture, Start Sequence immediate capture, stopped navigate/disarm per design D7b/D7c |
| Data flow — Rand-seq | Single `randomizeSceneEndpointsAndBlend()` then full-pattern loop |
| Data flow — scope cap | Transport scope ≤ `kTransportScopeMaxWidth`; remainder unused |
| Nesting | New handlers ≤3 levels (`onSequencerStepAdvance`, `setEditStep`, layout helpers) |
| Defensive code | Bounds/null guards only on proven paths |
| Helper extraction | `captureLiveToSequencerStep` / `recallSequencerStep` — 2+ callers, explicit contract |
| Plan language | Specs: zero forbidden hedge terms (OMNI.7) |
| Repetition — layout authority | `DesktopV2ChromeLayout.hpp` consumed by Main, transport layout, SubmodulePagePanel, ModSourceCell, perf band |
| Repetition — edit-step wrap | `SequencerState::wrappedEditStep` single source; panel calls `setEditStep(target)` for write-seq capture |
| Accumulate then apply | Submodule row layout: compute `RowLayout` array, then `setBounds` |
| No dead code | Orphan `m_title` removed |
| Verification (scoped) | Change-scoped tests pass before pre-existing suite failure |

### Violations — Section 11 (all fixed in tree)

| Rule | Was | Fix (task) |
|------|-----|------------|
| **Data flow — record export** | Formats on `RecordExportCluster` in transport | **11.1:** formats in **Audio** menu; `RecordButton` only in transport |
| **Repetition — edit wrap** | Panel duplicated `% patternLength` | **11.3:** `wrappedEditStep(±1)` → `setEditStep(target)` — never mutate then `setEditStep(m_editStep)` (early-return skips write-seq capture) |
| **No dead code** | `m_title` never laid out | **11.4:** removed |
| **Data flow — layout authority** | Transport widths local to transport helper | **11.2:** `kTransportPlayStopW`, `kTransportSettingsW`, gaps in chrome header |
| **Accumulate then apply** | Submodule `setBounds` in same loop as row iteration | **11.5:** two-pass `RowLayout` then apply |
| **Data flow — mod cell** | `kModLabelStripH` anonymous in `ModSourceCell.cpp` | **11.6:** hoisted to `DesktopV2ChromeLayout.hpp` |

### Risks — tracked, not blocking merge after Section 11

| Risk | Mitigation |
|------|------------|
| `AdsrPagePanel` center-float encoders | Out of scope; separate change |
| `test_pair_ar_gate_policy` suite failure | Pre-existing; not introduced by this change |
| Manual QA 10.2 | Operator gate before archive |

### Implementation guardrails (unchanged + post-audit)

| Rule | Directive |
|------|-----------|
| Data flow | `layoutStandaloneTransportRow`: Play → Stop → MIDI → Audio → RecordButton → scope cap; remainder unused |
| Data flow — write-seq | Playing: capture on Start Sequence to `m_playhead`; on advance capture to step left **after** host increment |
| Data flow — record export | Format in **Audio menu** → stored on `AudioEngine`; **Record audio** in transport reads it on stop/export |
| Repetition | prev/next → `SequencerState::wrappedEditStep(±1)` then `setEditStep`; transport widths in `DesktopV2ChromeLayout.hpp` |
| Accumulate then apply | Submodule row layout: compute rects, then apply bounds |
| Defensive code | No edit-step “none” path; clamp only in `setPatternLength` |
| Plan language | Grep specs for hedge terms before merge |

### Section 11 — OMNI compliance fixups (complete except operator QA)

1. **11.1** ✅ Split audio export UI: **Record audio** in transport; formats in **Audio** menu; `AudioEngine` export preference.
2. **11.2** ✅ Transport control width/gap constants in `DesktopV2ChromeLayout.hpp`.
3. **11.3** ✅ Sequencer prev/next → `setEditStep(wrappedEditStep(±1))` (write-seq capture preserved).
4. **11.4** ✅ Removed orphan `m_title` from `SequencerPanelComponent`.
5. **11.5** ✅ Submodule two-pass row bounds (`RowLayout` accumulate, then `setBounds`).
6. **11.6** ✅ `kModLabelStripH` in chrome layout header.
7. **10.2** ☐ Operator manual QA at 1280 px — Play, **Record audio**, format in Audio menu, Write Seq., scope ≤ 320.

## Supersedes (artifact drift)

Archived `desktop-v2-ux-and-sequencer` tasks marked [x] for UI polish while code remained broken. This change **supersedes** these operator-rejected decisions:

- Task 3.1 “Top transport → Engine” → **reverted to Play**
- Design §Rand-seq Pattern “blank steps only” → **all steps in pattern**
- Design “Pattern mode dice fills blank steps only (confirmed)” → **rejected**
