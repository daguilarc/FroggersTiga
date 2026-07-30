# Convergence Remainder Crosswalk

Maps unfinished work from archived `2026-07-07-froggers-v2-sheaf-runtime-convergence` into this unified change. Operator-truth repair items are **not** separate fixes — they are symptoms of incomplete convergence (layout authority, mod-grid projection, randomization authority, host sync).

## OMNI merge principle

| Symptom (operator QA) | Root cause (convergence gap) | Unified packet |
|----------------------|------------------------------|----------------|
| Comically wide mod dropdowns | `modW` expansion + no manifest compact route cell | **1** layout authority |
| Must scroll Audio page | Vertical budget not closed on real chrome | **1** layout authority |
| Unlabeled scene sliders | Performance band chrome incomplete | **1** layout authority |
| Global Rand Mods broken | 9.2/9.9 not implemented; wrong message wiring | **4** after mod authority **2** |
| Must visit Drive/Filter page | Host sync not full-engine | **3** host bridge |
| Triplicated randomize UI | 3.11 parity incomplete + module headers remain (plus dead hidden `CenterGlobalClusterV2` duplicate — packet 4.9) | **6** UI dedup |
| Write Seq / All Steps incoherent | 9.7, 9.9, 9.11 incomplete (partial arm/capture code in tree — verify/complete) | **5** sequencer |
| No global oscilloscope | 4A undone at archive; `GlobalOscilloscopeDisplay` now wired in tree — verify why operator saw none, complete gaps | **8** |
| Runtime pages thin | 5, 7 undone at archive; `Source/runtime/` components now in tree — verify/complete | **9** |
| Controller/MIDI page | 6 undone (partial code may exist) | **10** |
| VST/AU gaps | 8.2–8.6 undone | **11** |

## Archived convergence task disposition

| Section | Done at archive | Carried here as |
|---------|-----------------|-----------------|
| 0–2 Manifest + validators | Yes | Baseline — no reimplementation |
| 3 Layout shell | Ledger yes; operator QA failed scroll/mod width | **Packet 1** re-verify + fix |
| 4 Facade | No at archive; facade + test now in tree, `MainComponent` uses it | **Packet 7** verify/complete |
| 4A Global oscilloscope | No at archive; component now wired in tree | **Packet 8** verify/complete |
| 5 Runtime pages | No at archive; `Source/runtime/` now in tree | **Packet 9** verify/complete |
| 6 Controller | No at archive; data-driven `MidiCvSettingsComponent` via `buildTargetMappingRows()` in tree | **Packet 10** verify/complete |
| 7 Runtime audio | No at archive; projections in tree | **Packet 9** verify/complete |
| 8 Hosted params | 8.1 only | **Packet 11** |
| 9 Modulation + sequencer | No at archive; manifest eligibility + `ModLanePicker` + partial Write Seq in tree; 4×4 grid absent | **Packets 2, 4, 5, 6** |
| 10 OMNI closure | Partial | **Packet 13** |
| 11 Boot | Yes | Baseline |
| 12 Operator docs | 12.5 only | **Packet 12** |
| Repair-only (prior change) | N/A | Woven into packets above |

## Dedup rules

- Do not implement mod-width cap **and** leave `modW = rowWidth - modX` elsewhere — one `DesktopV2ChromeLayout` mutation.
- Do not wire Rand Mods until manifest mod routes are the sole depth authority (packet 2 before packet 4).
- Do not mark layout tasks from convergence §3 `[x]` without packet 1 gate + manual QA 8.2/8.14 pass.

## Operator critique overlay (2026-07-09)

Live test: Packet 2 drill-down works but **single-route + dropdown** model is rejected.

| Symptom | Root cause | Target packet |
|---------|------------|---------------|
| Dropdown redundant with 15-lane grid | `ModLanePicker` + `setSingleModSource` | **15** |
| Must assign before drill-in | `onParamPress` `hasAssignment` gate | **15** |
| Rand Mod moves one detail knob | Only `modSource[0]` lane active in UI/audio | **15** |
| Whole-encoder drill-in would break drag-to-turn | `mouseDown` → `onPress` on full ring | **15** (15.8, D17 — MOD LED only) |
| MOD label missing | Packet 2 skipped encoder affordance | **15.8** |

Packet 2 `[x]` = transitional plumbing only. Baseline `desktop-v2-mod-source-grid` depth-zero-per-lane contract closes in Packet 15.

**Future:** mod column removal (15.7) enables unified all-parameters page — out of scope for this change.

## Operator critique overlay (2026-07-09, top chrome)

| Symptom | Root cause | Target packet |
|---------|------------|---------------|
| Oscilloscope trace flat at top | Shared linear Y; one EF pegged high crushes others | **16** (per-trace auto-scale) |
| Scope radios truncated/overlapping | `GlobalStripV2` second row shares x with buttons above | **17** |
| Dead space right of Shift | Layout stops at Shift; no fill | **17** |
| Performance band grid broken | Fixed constants; 60px marbles columns | **17** |
| UI says Marbles | Manifest `Random/Marbles N` + `labelTailAfterSlash` | **18** |
| Shift appears to do nothing | `ShiftHeld` no-op; no held-gesture model | **18** |
| External-audio mod lanes not greyed/OFF | Core availability never wired; detail grid no disabled styling | **15** (15.3/15.3a); re-opens 2.6 |
| Detail cell 16 says "Target" not "Target (Back)" | Hardcoded in `SubmodulePagePanel` / `AdsrPagePanel` | **15** (15.8a) |
| MIDI encoder turn/press not mappable per param | Only 10 global `kControllerTargetDeclarations`; no `ParamTurn`/`ModDrillIn` dispatch | **19** (after **15.2**) |

Packet 1.3 `[x]` and Packet 8 `[x]` are transitional until Packets 17 and 16 close respectively. Packet 10 `[x]` is transitional for hardware encoder parity until Packet 19 closes.

## Device-neutral mod entry (Sheaf-aligned)

| Backend gesture | Message | Packet |
|-----------------|---------|--------|
| Ring drag / encoder rotate | `ParamTurn` | 15.2, 15.8; MIDI wiring **19** |
| Center MOD click / encoder button press | `ModDrillIn` | 15.2, 15.8; MIDI wiring **19** |
| Target (Back) press | `ParamPress` (exit detail) | 15.8a |

Per-row MIDI encoder manifest targets + `MidiCvAssignmentTable` dispatch: **Packet 19** (blocked on 15.2 `ModDrillIn`).
