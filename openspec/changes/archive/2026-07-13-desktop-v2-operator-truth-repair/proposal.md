## Why

Archiving `froggers-v2-sheaf-runtime-convergence` at 46/118 tasks left 72 items undone while automated gates passed. Operator QA (2026-07-07) then exposed that many reported "bugs" are not separate defects — they are **incomplete convergence**: layout/mod-grid authority was marked done but mod cells still expand to full-row width, Rand Mods was never wired through manifest scope, host bridge still syncs only the visible page, and the 4×4 mod detail grid / 15-lane rack UI was never implemented (the manifest eligibility catalog `isModSourceEligibleForRow` exists; no UI consumes it yet).

Splitting "operator truth repair" from "convergence remainder" hid the dependency: **the grid and manifest mod projection fix most of what the operator saw wrong.** This unified change completes both under one OMNI packet order.

## What Changes

### Unified scope (convergence remainder + operator truth)

**Packet 1 — Layout & mod-cell authority (fixes visible UI first)**
- Cap `moduleRowColumns().modW` at `kModCellW`; rebalance vertical budget so Audio fits at 1280×920 without scroll.
- Performance band labels (scene blend endpoints, marbles, macros — no anonymous G1/G2-only controls).
- Re-verify `LayoutBounds_test` against operator truth, not ledger checkbox alone.

**Packet 2 — Mod source grid convergence (archived §9.1–9.6)**
- Manifest-owned 15-lane source rack as sole mod eligibility authority.
- Sheaf-style 4×4 parameter-detail grid; compact mod lane pickers on module rows (route summaries, not full-row dropdowns).
- VCO pair-bus eligibility, external-audio unavailable/off, LFO lanes, None-as-cleared route state.

**Packet 3 — Full-engine host sync**
- `syncToHost()` pushes all pages every frame; carousel selection affects UI only.

**Packet 4 — Randomization authority**
- Single `executeRandomization` mutator; wire scope radios; fix global Rand Mods (live depths, not sequencer-only).
- Include Delay overlay when active.
- Delete dead `CenterGlobalClusterV2` — a permanently hidden duplicate of the `GlobalStripV2` rand cluster with the same broken `pushRandMods → RandSequencerMods` wiring (`CenterGlobalClusterV2.cpp:112`); otherwise gate 0.2 cannot pass within packet 4's write scope.

**Packet 5 — Sequencer operator loop (archived §9.7, 9.10–9.12)**
- Write Seq click-to-write + play-advance capture; All Steps + Rand Mods / Rand-seq scope; long-press clear; empty-all-unwritten no-op.

**Packet 6 — UI deduplication**
- Remove module-header Randomize/Randmod; remove duplicate sequencer scope radios.

**Packet 7 — Facade boundary (archived §4)**
- `FroggersV2AppCoreFacade` as migration shell; equivalence tests.

**Packet 8 — Global oscilloscope (archived §4A)**
- Verify/complete existing `GlobalOscilloscopeDisplay` (already wired visible in standalone + hosted shells with fixed-capacity `std::array` bindings and a test) against §4A: three VCO default traces, manifest-declared taps. Complete gaps only — do not reimplement.

**Packet 9 — Runtime pages (archived §5, §7)**
- Verify/complete existing `desktop-v2/Source/runtime/` components (File/Patch, Audio, Controllers pages, projections, hosted status panel, `RuntimePages_test`); labeled audio state; no duplicate Audio-page oscilloscope. Complete gaps only — do not reimplement.

**Packet 10 — Controller configuration (archived §6)**
- Manifest target IDs; multi-target fan-out; verify/complete MidiCv data-driven UI.

**Packet 11 — VST/AU hosted projection (archived §8.2–8.6)**
- Stable IDs, round-trip, hosted editor hiding standalone-only controls.

**Packet 12 — Operator docs (archived §12 + repair §7)**

**Packet 13 — OMNI closure (archived §10.2, 10.8–10.17)**

**Packet 14 — Manual QA at 1280×920 (archived 10.7 + operator-truth checklist)**

## Capabilities

### New (from prior repair change)

- `desktop-v2-randomization-authority`
- `desktop-v2-sequencer-operator-loop`

### Modified (repair + convergence, spec delta in this change's `specs/`)

- `desktop-v2-control-core`, `desktop-v2-grid-layout`, `desktop-v2-page-carousel`, `desktop-v2-performance-band-chrome`, `desktop-v2-sequencing`, `froggers-v2-product-contract`

### Implemented against existing baseline (no spec delta — requirements already merged, packets 7–11 close the implementation gap only)

- `desktop-v2-mod-source-grid`, `desktop-v2-scope-visualization`, `froggers-v2-controller-configuration`, `froggers-v2-runtime-audio-configuration`, `froggers-v2-sheaf-runtime`, `vst-v2-midi-modulation`

These six capabilities were already synced to `openspec/specs/**` by the archived convergence change even though its implementation tasks were left undone. Packets 2, 7–11 build code to conform to those existing baseline requirements — they do not change the requirement text. Do not add MODIFIED/ADDED spec delta files for these six unless a packet needs to change what the requirement actually says; a delta with no requirement change will not pass `openspec validate --strict` (packet 13.7) and was previously listed here in error. Packet acceptance criteria for 7–11 cite the baseline `openspec/specs/<capability>/spec.md` scenarios directly.

## Impact

- **Supersedes:** narrow `desktop-v2-operator-truth-repair` scope; absorbs archived convergence §4–9, §10 (partial), §12.
- **Baseline (done, do not redo):** manifest §1–2, layout shell §3 (re-verify only), boot §11, validator infrastructure.
- **Non-goals:** v1 desktop/web/VST, Daisy firmware, release tags, CMake version bumps.

## OMNI audit

| Rule | Unified fix |
|------|-------------|
| Single authority | Mod routes, layout geometry, randomization, host sync — one source each; Packet 15.2a collapses the parallel `ParamState::modSource[]` array to lane identity; Packet 19.0 retires the hardcoded `initTargetRows`/`rowKindForIndex` target table for a manifest projection |
| No validator/grep gaming | `check_desktop_v2_operator_truth.sh` (0.2) fails on `RandSequencerMods` in `pushRandMods`, single-page `syncToHost`, and uncapped `modW` — packets 1/3/4 must clear it by fixing the code, not the grep pattern |
| Repetition | Retire parallel Rand paths (`EnqueueRandomizePanelMod`, packet 4.6), dead hidden `CenterGlobalClusterV2` duplicate of `GlobalStripV2` (packet 4.9), duplicate scope UI (packet 6.2), expanded mod columns (packet 1.1) |
| Accumulate then apply | Packet 3 full-page host sync accumulates effective values per page/row before pushing to `DesktopHostIO`; no repeated per-row host mutation inside nested loops beyond the existing single pass |
| Nesting ≤ 3 | Packet 3's `syncToHost` page/row loop and packet 2's manifest-eligibility lookups stay at ≤3 levels; flagged for postflight audit in packet 13.6 if a packet's diff exceeds this |
| Defensive code only where real | Packet 4.7's Delay-overlay branch only guards the case where `m_host.m_delay` is null (a real, already-existing nullable per `FroggersV2HostBridge::syncModRoutes`), not speculative states |
| Imports global, no dead code | Packet 6 removal of module-header Randomize/Randmod and duplicate scope radios must also remove now-unused member fields/includes in `SubmodulePagePanel`, `AdsrPagePanel`, `SequencerPanelComponent` — not just detach the callback |
| Contract honesty | Manual QA blocks archive; convergence `[x]` on layout re-opened until packet 1 + 14 pass; six convergence-remainder capabilities cite existing baseline specs rather than claiming a spec delta that doesn't exist (see Capabilities above) |

See `convergence-remainder-crosswalk.md` for task-level mapping.

## Operator feedback — mod routing (2026-07-09)

Live test after Packet 2 launch: operator confirms drill-down is **per-parameter row**, but rejects the **dropdown-then-drill-in** flow.

**Critique (authoritative for follow-on work):**

- Module-row `ModLanePicker` dropdown is **obsolete** — captured in **Packet 15 from first live-test critique**, not a new realization.
- Target: **all fifteen lanes** on a parameter share one detail view; each lane depth defaults to **0 (off)**; **multiple lanes MAY be active** simultaneously.
- **MOD/CV LED click** opens parameter detail — **not** ring click (ring = drag-to-turn). Center LED **fixed in place**; color/intensity reflect attenuated-range bias.
- **Rand Mod** must randomize **eligible lane depths**, not a single pre-selected route.
- **Future:** dropping the mod column enables an all-parameters-on-one-page layout (out of scope for this change; note only).

Packet 2 delivered transitional plumbing (4×4 layout, manifest lane labels, tests) under a **single-source** control-core model (`setSingleModSource`, `modSource[0]`). That implementation **does not satisfy** the baseline `desktop-v2-mod-source-grid` depth-zero-per-lane contract. **Packet 15** records the correction; baseline spec delta for `desktop-v2-mod-source-grid` is added in Packet 15 when requirement text is updated to explicitly retire `ModLanePicker` and document multi-depth summation.

**Engine-scope caveat (omni-rule assumption-break):** the audio engine currently stores **one** (source, depth) per row (`FroggersV2HostBridge` / `HostParameterRoutingV2` single `SetPageModSource` / `SetPageModDepth`, single-source `DelayState::Modulate`, single-field sequencer snapshot). Summing N independent audio-rate lanes may be **engine/DSP work, not control-core wiring** — Packet **15.0** is a parent engine-scope spike that gates all 15.x code (`design.md` D11a). If summation is new DSP, Packet 15 is re-scoped at the OpenSpec layer before dispatch. The parallel `ParamState::modSource[]` array is collapsed to lane identity (15.2a) so a route has one authority, not two.

Evidence: `operator-qa-2026-07-09.md`, `design.md` D11–D12, D11a.

## Operator feedback — top chrome + oscilloscope (2026-07-09, continued)

Live test after mod drill-down exercise.

**Oscilloscope:** Works, but one VCO trace sits flat at the top. Default taps are VCO envelope followers on a **shared linear 0..1 Y axis** in `CvScopeDisplay`. **Fix (operator-selected): per-trace auto-scale** — each trace normalizes to its own recent min/max so all three show activity. Exp/log scaling rejected.

**Global command band:** Too many controls, truncated `"..."` labels, dead space right of Shift, scope radios crushed/overlapping under scene-related buttons — layout does not read as a coherent grid.

**Performance band:** Same grid failure — empty cells, overlapping controls, unreadable labels.

**Random source naming:** UI shows **Marbles** via manifest tail projection; operator cites specs requiring **Random S&H 1/2** in UI (Marbles name manual-only for MI inspiration).

**Shift:** On-screen toggle is dead chrome — `ShiftHeld` is a no-op; v2 has no held-gesture model.

**External-audio mod lanes:** Operator confirms lanes must be **greyed out and OFF** when no external audio is connected. Baseline spec already requires visible-but-unavailable/off; implementation gap: control core never receives `setExternalAudioAvailable` from the shell, and parameter-detail cells do not render disabled/greyed state. Packet 15.3/15.3a; Packet 2.6 re-opened.

**Device-neutral mod entry:** `ModDrillIn(page, slot)` is the single enter-mod action (Sheaf-aligned). Mouse: center MOD LED click. MIDI pressable encoder: button press (rotation → `ParamTurn`). **Packet 19** adds per-row encoder manifest targets and MIDI dispatch; **Packet 15.2** delivers the message boundary.

Packets **16–19** record corrections and follow-ons. Packet 1 `[x]` performance-band labeling and Packet 8 `[x]` oscilloscope do **not** satisfy final operator contract until 16–18 close. Packet 10 `[x]` does **not** satisfy hardware encoder parity until **19** closes (after **15.2**).

Evidence: `operator-qa-2026-07-09.md`, `design.md` D13–D16.
