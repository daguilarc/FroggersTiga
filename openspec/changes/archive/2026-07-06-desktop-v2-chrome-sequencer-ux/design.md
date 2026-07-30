## Context

Desktop v2 on `froggerstiga-desktop-v2` boots and syncs after `desktop-v2-boot-sync-fix`, but operator QA shows horizontal layout collapse and spec drift from archived `desktop-v2-ux-and-sequencer`:

- `MainComponent.cpp` L317 gives the VCO EF scope all flex after Play/Stop/MIDI/Audio.
- `PerformanceBandV2` crams scene buttons, marbles, blend, BPM, Steps, and sequencer transport into one row — labels truncate.
- `SubmodulePagePanel::layoutRows` L193 centers encoders with `withSizeKeepingCentre` in leftover width.
- `ModSourceCell` shrinks when source is None (14 px label strip vs taller assigned).
- `MainComponent.h` L53 labels audio transport **Engine**; v1 and operator expectation is **Play**.
- `FroggersV2ControlCore::onRandSequencerStep` All-steps scope skips `hasData` steps (L741–744) — contradicts operator intent.
- `SequencerPanelComponent` uses checkbox-styled toggles labeled **Step** / **Pattern**; dice is unlabeled.
- `FroggersV2HostBridge::onSequencerStepAdvance` L41 captures into landed `m_playhead` — step 0 skipped on first beat.
- `DesktopV2ChromeLayout.hpp` **exists** but lacks transport scope cap, mod-cell height, and module-row column offsets; `kPerfSceneButtonSize` (30 px) and `kPerfBlendEndpointLabelW` (10 px) are undersized for 1280 px QA.

**Edit-step invariant (verified in code):** `SequencerState::m_editStep` defaults to 0, wraps via prev/next, clamps on `setPatternLength`, no deselect API. Third Rand-seq scope is unnecessary.

## Goals / Non-Goals

**Goals:**

- Restore **Play** / **Stop** for audio transport; keep **Start Sequence** / **Stop Sequence** for sequencer.
- Fix Rand-seq scope: **Step** = edit step; **All steps** = full pattern overwrite (rename UI from **Pattern**).
- Move BPM, Steps, sequencer transport, **Write Seq.** to sequencer toolbar; restore v1 **Record audio** in transport row; export format in **Audio** menu.
- Cap scope width; left-align module rows; fixed mod-cell footprint.
- Extend layout authority (`DesktopV2ChromeLayout.hpp`) shared by `MainComponent` and `HostedMainComponentV2`.
- Update tests and `QUICK_DICT` mirrors.

**Non-Goals:**

- Per-step L/R/blend storage in step snapshots (future).
- A/R knobs on Audio page (remain on Pair-AR page).
- Checkbox → radio restyling outside sequencer scope toggles.
- Extracting RecordButton to `common/ui/` in this change (copy v1 sources into v2 target first).

## Decisions

### D1 — Audio transport label: Play, not Engine

**Choice:** `m_play{"Play"}` in `MainComponent.h`; docs say Play.

**Rationale:** Operator rejected Engine. Sequencer duplicate-Play problem is solved by **Start Sequence** naming, not renaming audio transport.

**Alternative rejected:** Keep Engine to distinguish from sequencer — operator explicitly rejected.

### D2 — Rand-seq scope semantics

**Choice:**

| UI label | Constant | Behavior |
|----------|----------|----------|
| Step | `kRandSeqScopeStep` | Randomize `m_editStep` only |
| All steps | `kRandSeqScopePattern` | Loop `0..patternLength-1`, overwrite all |

**Rationale:** Operator never approved blank-only Pattern. Edit step is always defined, so no third scope.

**Code change:** Remove `if (hasData) continue;` in All-steps loop (`FroggersV2ControlCore.cpp` L741–744). Update `ControlCoreBridge_test` to assert step 3 is overwritten.

**Alternative rejected:** Keep internal name Pattern with blank-only behavior — rejected by operator.

### D3 — Edit-step invariant (no third scope)

**Choice:** Document and test invariant; no UI for “no selection.”

**Evidence:** `SequencerState.hpp` `m_editStep = 0`; `setPatternLength` clamps; arrows wrap.

**Rationale:** Sequencer can be idle while audio Play runs; edit step remains valid for Step-scope dice.

### D4 — Layout authority (`DesktopV2ChromeLayout.hpp`)

**Choice:** **Extend** existing header (file already in tree) with:

```cpp
namespace DesktopV2ChromeLayout {
constexpr int kTransportScopeMaxWidth = 320;
constexpr int kSceneButtonMinWidth = 44;   // bump from kPerfSceneButtonSize (30)
constexpr int kBlendLabelMinWidth = 16;    // bump from kPerfBlendEndpointLabelW (10)
constexpr int kModCellHeight = 48;
// module row column offsets (labelW, encoderX, modX)
}
```

Deprecate or alias undersized perf constants to the new names in the same edit.

**Rationale:** OMNI data-flow rule — one source for geometry; Main + Hosted + SubmodulePagePanel consume it.

**Alternative rejected:** Per-component magic numbers — caused current drift.

### D5 — Transport row layout pipeline

**Choice:** Extract `layoutStandaloneTransportRow(juce::Rectangle<int>& transport)` in `desktop-v2/Source/ui/` (header + inline or `.cpp`). Fixed-width left cluster, then scope `setBounds` with `jmin(remaining.width(), kTransportScopeMaxWidth)`:

```
Play | Stop | MIDI | Audio | RecordButton | Scope[max 320] | (remainder unused)

Export format (WAV/MP3/FLAC/OGG): **Audio** menu only.
```

Sequencer toolbar (below): … | Write Seq. | …

**Rationale:** Audio **Record audio** stays in transport; Write Seq. lives with the grid it writes to.

### D6 — Sequencer toolbar ownership

**Choice:** `SequencerPanelComponent` owns BPM (`Label` + `Slider` — same widgets as today in `PerformanceBandV2`), Steps, Start/Stop Sequence, prev/next, Rand-seq dice+label, Step/All steps radios. `PerformanceBandV2` sheds those controls.

**Rationale:** Operator: BPM/Steps far from grid — colocate with grid.

**Data flow:** Toolbar widgets → existing bridge messages (`setPatternLength`, clock BPM, sequencer play/stop) — reuse message types, relocate widgets only.

### D7 — Write Seq. (not Record)

**Choice:** Sequencer toolbar toggle **Write Seq.**; rename `SequencerState::m_recordArm` → `m_writeSeqArm` and update all references (`PerformanceBandV2`, `FroggersV2HostBridge`, host params, tests). v1 round red **Record audio** is audio-only.

**Rationale:** Operator rejected sequencer hijack of Record. Two capture domains: audio file vs step buffer.

### D7b — Write Seq. when stopped

**Choice:** On edit-step change while armed: `capture(live → m_steps[oldEdit])`, then `apply(m_steps[newEdit])`. On disarm while stopped: `capture(live → m_steps[editStep])`.

**Rationale:** Step-by-step programming without running the clock. Matches “click through steps and save” intent.

### D7c — Write Seq. when playing + step 0 fix

**Problem:** `onSequencerStepAdvance` runs **after** `m_playhead` increments. Current code captures into the **landed** step, so first advance 0→1 writes step 1 and **skips step 0**.

**Choice (two-part):**

1. **On Start Sequence** with Write Seq. armed: `capture(live → m_steps[m_playhead])` immediately (step 0 at pattern start).
2. **On each beat advance** with Write Seq. armed: capture into **step left**:
   `stepLeft = (m_playhead + patternLength - 1) % patternLength` (valid because playhead already advanced).

Then `apply(m_steps[m_playhead])` as today.

**Data flow:**

```
Start Sequence + armed → capture → m_steps[playhead]
Beat advance (host)    → playhead++
Callback + armed       → capture → m_steps[stepLeft]
Callback (always)      → apply m_steps[playhead]
Edit step change+armed → capture → m_steps[oldEdit]; apply m_steps[newEdit]
```

**Alternative rejected:** Capture into landed step — loses step 0 on first beat.

### D8 — Module row layout

**Choice:** Replace `withSizeKeepingCentre` with column layout:

```
| label (fixed %) | encoder (fixed px) | mod cell (fixed px) | gap |
```

Compute X from `rowArea.getX() + columnOffset[i]`.

**Rationale:** OMNI repetition — one loop over rows with shared column offsets.

### D9 — Mod cell fixed height

**Choice:** `ModSourceCell::resized()` always uses `kModCellHeight`; None state renders empty scope area inside same bounds.

**Rationale:** Stops row jump on assignment.

### D10 — Scope radio appearance

**Choice:** Keep `ToggleButton` + `radioGroupId` (already wired in `SequencerPanelComponent.cpp` L58–73); add `setClickingTogglesState(false)` on group members; rename label **All steps** not Pattern; set radio-button LAF or minimum size so controls read as radios not checkboxes.

**Rationale:** Functional radios exist today but look like checkboxes; operator wants radio affordance.

### D11 — Restore v2 audio export

**Choice:** **Record audio** (`RecordButton`) in the transport row; **WAV / MP3 / FLAC / OGG** export format in the **Audio** menu (`AudioSettingsComponent`). `AudioEngine` persists `ExportFormat`; export on stop uses engine preference. Label updated from v1 **RECORD** to **Record audio**. Red circle chrome unchanged (`RecordButton.cpp`).

**Rationale:** Operator wants explicit audio vs Write Seq. distinction; format belongs with other audio I/O settings when opening **Audio**, not crowding the transport row.

## OMNI compliance map (verified violations → fix location)

| Violation | Type | Location | Fix | Task |
|---|---|---|---|---|
| Scope consumes transport flex | Data flow | `MainComponent.cpp` L317 | `layoutStandaloneTransportRow` + `kTransportScopeMaxWidth` | 1.2, 1.4 |
| Encoder center-float | Repetition | `SubmodulePagePanel.cpp` L193 | Column offsets from `DesktopV2ChromeLayout` | 7.1–7.2 |
| Mod cell height jump | Data flow | `ModSourceCell` None vs assigned | `kModCellHeight` fixed bounds | 7.3 |
| All-steps blank-only skip | Data flow | `FroggersV2ControlCore.cpp` L741–744 | Remove `hasData` continue | 8.1 |
| Write-seq step-0 skip | Data flow | `FroggersV2HostBridge.cpp` L41 | Capture step-left + Start Sequence immediate | 6.3–6.5 |
| Engine label | Artifact drift | `MainComponent.h` L53 | **Play** | 2.1 |
| Sequencer Record naming | Artifact drift | `PerformanceBandV2` `m_seqRecord` | Move to toolbar as **Write Seq.**; rename `m_writeSeqArm` | 4.4, 5.3 |
| Dead `m_recorder` | Data flow | `AudioEngine.h` L120 | Port v1 recording API | 3.1–3.2 |
| Chrome constants incomplete | Artifact drift | `DesktopV2ChromeLayout.hpp` | Extend with cap + mod height + offsets | 1.1 |
| Pattern UI label | Artifact drift | `SequencerPanelComponent.hpp` L90 | **All steps** | 5.4 |
| Hedge in spec | Plan language | `desktop-v2-sequencing/spec.md` L66 | `MessageIn::Clock` only | (this audit) |
| Non-goals contradicted audio export | Artifact drift | `design.md` prior Non-Goals | Removed — export is in scope | (this audit) |
| Formats in transport row | Data flow | `MainComponent` + `RecordExportCluster` | Formats in **Audio** menu; `RecordButton` only in transport | 11.1 |
| Transport widths not in chrome header | Repetition | `DesktopV2TransportLayout.hpp` | `kTransportPlayStopW`, `kTransportSettingsW`, gaps in chrome header | 11.2 |
| Edit-step wrap duplicated in panel | Repetition | `SequencerPanelComponent` prev/next | `SequencerState::wrappedEditStep(±1)` → `setEditStep` | 11.3 |
| Orphan `m_title` | No dead code | `SequencerPanelComponent` ctor | Removed unused label | 11.4 |
| Submodule single-pass bounds | Accumulate then apply | `SubmodulePagePanel::layoutRows` | Two-pass `RowLayout` array | 11.5 |
| Local `kModLabelStripH` | Layout authority | `ModSourceCell.cpp` | `DesktopV2ChromeLayout::kModLabelStripH` | 11.6 |

### Shared helper extraction (OMNI repetition — trigger ≥2 met)

| Helper | Trigger count | Boundary | Complexity | Contract | Callers |
|--------|---------------|----------|------------|----------|---------|
| `layoutStandaloneTransportRow` | 2 (standalone Main; shared constant with Hosted scope strip budget) | Transport row geometry | Column strip + scope cap | In: transport rect ref; out: bounds set on Play/Stop/MIDI/Audio/Record/Scope | `MainComponent::resized` |
| `layoutModuleRows` (inline refactor) | 2+ rows per panel | Submodule encoder row | 2-pass bounds | In: row area + column offsets; out: label/encoder/mod bounds | `SubmodulePagePanel::layoutRows` |

Review enforcement — `layoutStandaloneTransportRow`: Trigger ≥2 **Yes** (Main + chrome constant shared with Hosted) | Domain boundary **Yes** | Complexity **Yes** (multi-control strip) | Contract **Yes** | Side effects clear **Yes** (setBounds only) | Local scope **Yes** (`desktop-v2/Source/ui/`).

### Implementation nesting

Layout helpers and write-seq handlers stay ≤3 levels. All-steps Rand-seq loop stays flat (single `for` + body, no nested conditionals after `hasData` removal).

### Compliant by design

- **Edit-step invariant:** no new API surface; document + test existing behavior.
- **Radio group:** reuse existing `radioGroupId` wiring; styling change only.
- **Imports:** extend existing translation units; remove dead perf-band widgets after toolbar move.

## Risks / Trade-offs

| Risk | Mitigation |
|------|------------|
| Toolbar crowding at 1280 px | Measure toolbar min width in `SequencerPanelComponent::resized`; when `toolbarMinW > availableW`, split to two rows inside sequencer panel (task 5.7) |
| RecordButton v1→v2 coupling | Copy v1 `RecordButton` into v2 target; format UI in v2 `AudioSettingsComponent`; defer `common/ui/` extraction |
| All-steps dice destroys intentional step data | Operator-requested; context-menu Randomize still per-step; Reset restores factory |
| HostedMainComponentV2 divergence | Shared `SequencerPanelComponent` + `DesktopV2ChromeLayout` constants; VST hides audio export (task 3.5, 5.8) |
| Test `ControlCoreBridge_test` pattern assertion inverted | Update test in same PR as core fix (task 8.2) |
| Partial prior work | `DesktopV2ChromeLayout.hpp` and radio group exist — tasks say **extend** / **finish**, not greenfield |

## Migration Plan

1. Implement on `froggerstiga-desktop-v2`.
2. Run `desktop-v2` tests; fix `ControlCoreBridge_test` pattern + write-seq expectations.
3. Manual QA at 1280 px: Play visible, S1/S2 readable, Rand-seq labels, All steps overwrites, Record audio + formats.
4. Update `QUICK_DICT.md` + mirrors.
5. Archive change; sync delta specs to `openspec/specs/`.

Rollback: revert branch; no data migration (UI-only + Rand-seq behavior).

## Open Questions

None blocking implementation.
