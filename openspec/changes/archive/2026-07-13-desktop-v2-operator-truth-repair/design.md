## Context

Two changes were split incorrectly:

1. **Archived convergence** — 72/118 tasks undone; automated gates green; operator UX red. The working tree contains substantial uncommitted implementations from that change (oscilloscope, runtime pages, facade, manifest mod eligibility, partial Write Seq) — packets must verify/complete against the tree, not reimplement from the archive ledger.
2. **Operator-truth repair** — diagnosed wiring bugs but scoped too narrow; treated symptoms as isolated fixes.

Operator findings map to convergence gaps (see `convergence-remainder-crosswalk.md`). **Packet 1–2 (layout + mod grid) address mod dropdown width, scroll, and mod authority before packet 4 rewires randomization.**

## Architecture (target data flow)

```
Manifest (15-lane rack, rows, scope controls)
        │
        ▼
DesktopV2ChromeLayout ──► compact mod cells (fixed kModCellW)
        │                  4×4 detail grid
        ▼
UI projections (global band, carousel, sequencer)
        │
        ▼
FroggersV2ControlCore::executeRandomization (sole rand mutator)
        │
        ▼
FroggersV2HostBridge::syncToHost() (extended in packet 3 to loop all kNumHostPages, not just activePage())
        │
        ▼
DesktopHostIO / PageManager (always-active signal path)
```

Note: `syncToHost()` is the existing function (currently single-page, `FroggersV2HostBridge.cpp:144`). Packet 3 extends it in place — this is not a new `syncAllPagesToHost()` method. A separate existing function, `syncAllModRoutesToHost()`, already loops all pages for mod *routes* (source/depth) only; packet 3 brings knob *values* to the same all-pages coverage without introducing a third differently-named sync entry point.

## Packet order (OMNI-enforced, sequential)

| Packet | Content | Gate |
|--------|---------|------|
| 0 | Baseline gates, operator-truth grep script | `check_desktop_v2_operator_truth.sh` scaffold |
| 1 | Layout + mod-cell + performance band | `LayoutBounds_test` + no scroll + modW cap |
| 2 | Mod source grid + 4×4 detail | Mod eligibility tests, compact pickers |
| 3 | Full-page host sync | Non-visible page affects audio test |
| 4 | Randomization authority | Rand Mods live + scope wired |
| 5 | Sequencer operator loop | Write seq, all-steps rand, 9.10–9.12 tests |
| 6 | UI dedup | No module-header rand; no duplicate scope |
| 7 | Facade | `FroggersV2AppCoreFacade_test` |
| 8 | Global oscilloscope | `GlobalOscilloscopeDisplay_test` |
| 9 | Runtime pages + audio | `RuntimePages_test` |
| 10 | Controller | `MidiCvAssignment_test` |
| 11 | VST/hosted | Hosted editor tests |
| 12 | Operator docs | `sim/check_operator_docs_sync.sh` |
| 13 | OMNI closure | hedge grep, host-master audit, validate --strict |
| 14 | Manual QA (Packet 14) | All 14.1–14.18 pass |

**Mandatory after every packet:**

```bash
bash scripts/check_subagent_packet_gates.sh
bash scripts/check_desktop_v2_operator_truth.sh
```

## Decisions (retained + extended)

### D0 — Grid before rand wiring

Complete layout authority and manifest mod projection (packets 1–2) before randomization authority (packet 4). Wide dropdowns and broken global Rand Mods share the same root: UI not projecting manifest compact cells and control core not using manifest scope.

### D1–D8

Retain decisions D1–D8 from the narrow repair design (randomization table, full sync, UI dedup, mod width, performance band, Delay, verification gates, manual QA — Packet 14).

### D9 — Convergence remainder in-band

Facade, oscilloscope, runtime pages, controller, VST, and full doc sync are **in scope** of this change, not a follow-up. Archive only when packets 1–14 complete.

### D10 — Re-open convergence §3 ledger honesty

Tasks 3.6–3.9 marked `[x]` in archive are **re-verified** in packet 1; they do not count as done until manual QA 14.2/14.14 pass.

## Migration plan

1. Packets 0–2 — fix what the operator sees (grid, labels, mod cells).
2. Packets 3–6 — fix what the operator does (rand, sync, sequencer, dedup).
3. Packets 7–11 — convergence features (facade, scope, runtime, controller, VST).
4. Packets 12–14 — docs, audits, manual QA.

## Operator critique — mod routing model (2026-07-09 live test)

Live test on FroggersTigaV2 confirmed Packet 2 shipped a **transitional single-route model** that contradicts the baseline `desktop-v2-mod-source-grid` contract and operator intent.

```
CURRENT (Packet 2)                         TARGET (Packet 15)
─────────────────                         ────────────────────
Module row: [dropdown pill]               Module row: (no mod column)
     │ assigns ONE modSource[0]                 │
     ▼                                         ▼
Press encoder (only if assigned)          Click MOD/CV LED center only
     │                                    (not whole ring — drag-to-turn)
     ▼                                         ▼
4×4 grid: 15 lanes visible                4×4 grid: 15 independent depths
     but ONE lane editable/active              depth 0 = off; many lanes active
```

| Behavior | Current implementation | Target contract |
|----------|------------------------|-----------------|
| Entry to detail | Dropdown assign → encoder press | **MOD/CV LED click** only; no dropdown; no whole-encoder press |
| Active routes per row | `setSingleModSource` → one `modSource[0]` | Per-lane `modDepth[i]`; 0 = off |
| Module-row mod UI | `ModLanePicker` popup (15-lane menu) | Retired |
| Rand Mod in detail | One lane shows movement (assigned route) | All eligible lane depths randomized |
| Audio summation | One source depth via slot 0 | Sum all non-zero eligible lane depths |

**Future enabler (not Packet 15):** removing the mod dropdown column frees horizontal space for a unified all-parameters page. Captured as follow-on layout work only.

See `operator-qa-2026-07-09.md` for session evidence.

### D11 — Multi-depth replaces single-route (Packet 15)

Per-parameter modulation uses fifteen independent depth encoders. Depth zero means the lane is off. Multiple lanes MAY be non-zero on the same row. `setSingleModSource` and the `hasAssignment` gate on `onParamPress` are retired. **Mod detail opens from a dedicated MOD/CV LED hit target**, not from `ParamPress` on the whole encoder ring (preserves mouse drag-to-turn on the ring).

### D11a — Multi-lane summation is engine scope, gated by the 15.0 spike (Packet 15)

`ParamState` (control core) holds per-lane `modDepth[15]`, but the **`src/core/` audio engine stores and applies exactly one (source, depth) per modulated target**. The fundamental knob type `Parameter` (`src/core/Parameter.hpp:54-55`) has scalar `m_modIndex` + `m_modAmount`; the same single-source shape recurs in `sim/DelayState.hpp:267` (`std::array<uint8_t, kNumRows> modSource{}`), `sim/SequencerState.hpp:18-19` (`SequencerSlotPayload::modSource[page][row]`), and `src/core/AudioPairArState.hpp`. `sim/VcvSectionAdapter.hpp:116-117` only *forwards* one `SetPageModSource/Depth` to the host IO (`src/core/Page.hpp:387/409`), which stores it on the single-source `Parameter`. The `desktop-v2/Source/` bridge (`FroggersV2HostBridge.cpp:146`) merely projects that one value.

**15.0 spike VERDICT (2026-07-11, corrected): it is real DSP work, but FULLY CONTAINED to the desktop-v2 + VST V2-fuego path — it does NOT touch v1/Daisy.**

*Containment (hard requirement — v2/VST only):* the V2 modulation apply is already forked behind `m_useV2Fuego` (`Page.hpp:146`), which is enabled **only** by `configureV2FuegoPages()` in `DesktopHostIO`/`PagedHostIO` (`DesktopHostIO.hpp:718-726`) — the desktop-v2 + VST host IO. v1 desktop uses the legacy `ln` host and never calls `ConfigureV2Fuego`; Daisy never references it. Both take the `m_useV2Fuego == false` legacy branch. **All Packet-15 engine work stays inside the V2 branch + the V2 host IO + a V2-only per-lane store; the legacy path is not edited.** (An earlier draft wrongly called this a "v1/Daisy regression surface" — that conflated a shared *file* with shared *behavior*; corrected.)

*The one real DSP item — RESOLVED (user 2026-07-11): additive attenuverter model.* The V2 apply becomes:

```
out = clamp( knob + Σ over eligible lanes ( tapᵢ · aᵢ ), 0, 1 )
```

where `aᵢ` is the lane's **signed** depth (−1…+1, `clampSigned`) and `tapᵢ` = `PermanentModTapRack::GetTap(i)`. The knob stays put; each active lane adds/subtracts its tap scaled by depth; the 0–1 clamp is the only limiter (no cap/normalize). This **replaces** the current single-lane crossfade `knob*(1-amount)+tap*amount` in the V2 branch (`Page.hpp:146-151`) — so single-lane V2 mod feel changes too (intended; existing V2 apply tests re-baseline). Depths come from the control core's existing `ParamState.modDepth[15]`, plumbed through the V2 host IO into a V2-only per-row store; the 15-tap `PermanentModTapRack` already supplies all source values.

**Consequence:** Packet 15 is **not** pure UI transcription — it carries one DSP design decision (the blend formula) + engine plumbing, so it is a parent-designed / sonnet-implemented + TDD packet, not haiku. But it is bounded and v2/VST-contained, **not** a core redesign. The scalar `Parameter.m_modIndex/m_modAmount` and the legacy `ModMgr` (7-source) stay as-is for v1/Daisy. The 15.2a `ParamState::modSource[15]` → lane-identity collapse holds regardless.

### D12 — Retire ModLanePicker on module rows (Packet 15)

`ModLanePicker` compact dropdown is transitional scaffolding from Packet 2, not the converged UX. Parameter detail is the sole per-row modulation editor. Module pages lose the right-hand mod column once Packet 15 lands (layout follow-on adjusts `DesktopV2ChromeLayout`). The sixteenth detail cell label SHALL read **Target (Back)** (closes detail / returns to module page).

### D17 — MOD/CV LED is the sole mod drill-in target (Packet 15)

`EncoderRingComponent` SHALL render a center **MOD** affordance (CV LED + label) at the **fixed geometric center** of the encoder — the LED does **not** move with the attenuated range. **Color and intensity** of that center LED encode signed modulation bias relative to the attenuated-range centerpoint (red below, green above, greyed-out green when idle/no signal). The LED + **MOD** label are the **only** click target that opens parameter-detail modulation. The **ring annulus** (outer/inner arcs, excluding the center MOD hit zone) remains click+vertical-drag to turn the parameter. Whole-ring `mouseDown` SHALL NOT open mod detail.

**Device-neutral enter-mod action (Sheaf-aligned):** `ModDrillIn(page, slot)` is the single semantic action for opening parameter-detail modulation. Backends dispatch it from distinct physical gestures without re-deriving UI hit geometry in the control core:

| Backend | Turn gesture → | Press / drill-in gesture → |
|---------|----------------|----------------------------|
| Mouse | Ring drag → `ParamTurn` | Center MOD LED click → `ModDrillIn` |
| MIDI pressable encoder | Rotation → `ParamTurn` | Button press → `ModDrillIn` |
| Touch | Ring drag → `ParamTurn` | Center MOD tap → `ModDrillIn` |

`ParamPress` on the module-row encoder ring is retired for mod entry. `ParamPress` on the detail-grid **Target (Back)** cell remains the exit action. Per-row MIDI encoder turn/press manifest targets are **Packet 19** (depends on 15.2); Packet 15 only requires the `ModDrillIn` message boundary.

### D18 — Per-row MIDI encoder controller targets (Packet 19)

After Packet 15.2 lands `ModDrillIn`, Packet 19 extends Packet 10's controller-configuration model so hardware pressable encoders map to the same device-neutral messages as mouse/touch:

| Manifest target kind | Physical input (examples) | Control-core message |
|----------------------|---------------------------|----------------------|
| `{param_stable_id}_encoder_turn` | Relative CC, encoder rotation | `ParamTurn(page, slot, delta)` |
| `{param_stable_id}_encoder_mod_drill_in` | Encoder button, note-on, CC threshold | `ModDrillIn(page, slot)` |

Targets SHALL be **generated from the parameter inventory** (`HostParameterInventoryV2` / product row stable IDs), not from carousel slot indices or UI labels. `MidiCvAssignmentTable` dispatches to the control-core bus; `MidiCvSettingsComponent` lists targets via `buildTargetMappingRows()`. Parameter-detail depth-cell encoder MIDI mapping is out of scope for Packet 19.

### D13 — Oscilloscope per-trace auto-scale (Packet 16) — operator-selected

Multi-trace global oscilloscope SHALL use **per-trace auto-scale**: each trace normalizes to its own recent min/max over the display ring buffer before Y mapping. A pegged-high trace on shared linear 0..1 axis no longer flattens the other traces; each line shows **activity within its own recent range**.

- Display-only transform; engine CV remains 0..1.
- Exp/log Y mapping is **rejected** for 0..1 CV — does not fix pegged traces and obscures level semantics.
- Packet 16.1 spike still confirms default `kOscilloscopeTaps` (VCO EF) are the correct product signals; auto-scale is the display fix regardless.

### D14 — Top chrome honest grid (Packet 17)

`GlobalStripV2` and `PerformanceBandV2` SHALL lay out on the shared 10px grid without truncated labels, overlapping scope radios, or dead space right of Shift. Scope radio pairs SHALL NOT share x-columns with unrelated buttons on the row above. Band height MAY increase if required for two readable rows.

### D15 — Random S&H UI naming (Packet 18)

Operator-visible strings for random mod sources SHALL read **Random S&H 1** and **Random S&H 2**. The substring **Marbles** SHALL NOT appear in desktop v2 UI chrome (performance band labels, mod lane pickers, global strip). Manual/tooltip copy MAY reference Mutable Instruments Marbles per `sim-operator-doc-parity`. Manifest `stableId` values (`random_marbles_1`) are internal; `displayName` projection for UI MUST be corrected.

### D16 — Retire Shift entirely (Packet 18) — updated 2026-07-09

v2 has no held-gesture semantics: every Shift input path (on-screen toggle, keyboard Shift key via `updateShiftFromKeyboard`, MIDI shift button via `MidiCvAssignmentTable`) dead-ends at `FroggersV2ControlCore::applyMessage` `case ShiftHeld: break;` — nothing reads a held state to gate behavior. Shift is a fossil of Sheaf's held-modifier randomization model, which Froggers deliberately rejected in favor of discrete scope buttons (Rand All / Rand Mods / per-module Randomize) — see `sheaf-adoption-inventory.md` §"Departure: randomization model" for the full rationale and the re-evaluation trigger.

**Decision (updated):** remove the **entire** Shift machinery, not just the on-screen toggle — the toggle, the keyboard driver, the MIDI shift *target*, and the inert `MessageIn::ShiftHeld` message/handler. A mappable-but-inert shift target is the same dead-chrome trap as the on-screen button (nothing consumes it). No shift affordance is retained on any surface. If Sheaf's modifier model later gains group/all-scope selection (see re-evaluation trigger), reopen.

### D19 — Row/page display-name authority location (Packet R remediation, 2026-07-11)

**Context / how this arose:** fixing the pre-existing `V2ParamDisplayNames::forHostPageRow` link failure, commit `264304a` *moved* the row/page label tables (`kLegacyHostPageLabels`, `kLegacyHostRowGrid`, `kPairArRowLabels`, `kExpansionTailRowLabels`) from `desktop-v2/Source/manifest/FroggersV2AppManifest.hpp` into `sim/V2ParamDisplayNames.hpp`. That was a **single-authority relocation done inside execution** — an OMNI §1 / process-rule C3 violation — and it turned `check_desktop_v2_duplicate_authority.sh` RED (that gate greps for `kExpansionTail`/`kPairArRows` and only approves the manifest path).

**The real tension (why the link failure existed at all):** `sim/V2ParamDisplayNames.hpp` *declares* `forHostPageRow`, and `sim/DelayState.hpp` (a layer *below* the manifest) *calls* it — so the code's existing layering already expects `sim/` to own this interface, while the duplicate-authority gate declares the *manifest* the owner. Two truths. This is a genuine authority-location decision, not a mechanical fix.

**Decision (Packet R):** the single authority for row/page display-name **data** is `sim/V2ParamDisplayNames.hpp` (the lowest layer that needs it — `DelayState`, `PagedHostIO`, wasm bindings all sit at or below it and none include the manifest). The manifest's `productPageDisplayName`/`productRowDisplayName` **project from** it (delegate), preserving the manifest as the authority for *host-parameter inventory* while the display-name strings live where every layer can reach them. Because this moves a *declared* authority, Packet R MUST **update `check_desktop_v2_duplicate_authority.sh` APPROVED_PATHS to add `sim/V2ParamDisplayNames.hpp`** in the same change, so single-authority is re-established (one location, everything projects from it) and the gate goes green again. Rejected alternative: revert the move and stop `sim/DelayState` calling `forHostPageRow` — larger, and fights the existing layering.

### D20 — Assignability vs. rand-eligibility: a real bug traced to Packet 15-B, fixed in the 15–19 closeout (2026-07-12)

**The bug:** the manifest has two similarly-named predicates that are *not* interchangeable: `isModSourceEligibleForRow` (gates on `ModulationSource::randomizable` — used by Rand All/Mods, which deliberately excludes external-audio lanes) and `isModLaneAssignable` (the manual-assignment gate, added in Packet 15-C2, correctly special-cases external-audio lanes as assignable whenever audio is actually available, independent of `randomizable`). Packet 15-B (`75e11e5`) wired `FroggersV2HostBridge::syncModRoutes`'s ToHost lane push using `isModSourceEligibleForRow` — meaning **external-audio modulation lanes never reached the audio engine at all, even with external audio connected and a depth dialed in**, because that predicate returns `false` for them unconditionally. This shipped silently: no test in this session's own suite (`test_v2_lane_depth_additive_sum_reaches_engine`, `test_multiple_lanes_active_simultaneously_on_one_row`, etc.) specifically exercised an external-audio lane index, so the discrepancy was never exercised.

**The fix (15–19 closeout commit `b8db8bd`):** both consumers — the engine-sync path (`FroggersV2HostBridge::syncModRoutes`) and the UI-facing `computeEffective` (which also needed the page/row-aware overload for 15.4) — now consistently use `isModLaneAssignable`. Verified by inspection: both sites read the same predicate, so what the UI displays now matches what actually reaches the engine. Full desktop-v2 (14/14) and sim/ (22/22) suites independently re-run and green after this fix.

**Gap:** no test specifically locks in "external-audio lane becomes available → depth reaches both the UI effective value and the engine sum" — the fix is correct by inspection and consistent across both call sites, but this exact scenario (the one that was silently broken) isn't regression-proofed by a dedicated test yet. Worth a follow-up test, not a blocker.

**Process note:** this was caught by an independent second implementation (a separate agent closing packets 15's remainder through 19 in one pass), not by this session's own packet-by-packet review — a useful reminder that gate-green and test-green are necessary, not sufficient, when the bug is a wrong-but-plausible predicate choice rather than a missing check.

## Open questions

- **Packet 15 host/sequencer/engine summation:** ~~open~~ **resolved to a scope decision — see D11a and task 15.0 (parent engine-scope spike).** The single-(source, depth)-per-row engine IO must expand to N-lane audio-rate summation; whether that is wiring or new DSP is the 15.0 gate that blocks all 15.x code. This is engine work, not projection widening.
- **Packet 15 eligibility in detail:** unavailable lanes (VCO self-feedback, external audio absent) remain visible but non-editable — unchanged from baseline spec.
- Unified all-parameters page: deferred past Packet 15; depends on mod column removal.
- Oscilloscope default tap set: Packet 16.1 spike confirms EF taps vs spec "VCO 1/2/3" wording. Display fix is per-trace auto-scale (D13), not exp/log.
