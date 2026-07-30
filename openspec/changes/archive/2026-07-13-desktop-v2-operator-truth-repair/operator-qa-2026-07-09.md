# Operator QA — 2026-07-09 (live desktop v2)

Session: FroggersTigaV2 Release build launched via `scripts/open-desktop-v2.sh`. Operator exercised Packet 2 mod drill-down flow.

## Findings

| ID | Observation | Code truth | Verdict |
|----|-------------|------------|---------|
| OQ-09-1 | Drill-down is per-parameter row | `m_modView.targetRow` pins detail to one row; labels show manifest lane names for that row | **Confirmed** |
| OQ-09-2 | Dropdown pick then encoder press opens detail | `onParamPress` gates `m_modView.open` on `hasAssignment`; `ModLanePicker` popup assigns via `setSingleModSource` | **Confirmed** |
| OQ-09-3 | Dropdown feels redundant vs 15-lane grid | Detail renders 15 lanes but control core allows one `modSource[0]` route per row | **Confirmed — product mismatch** |
| OQ-09-4 | Rand Mod only moves one knob in detail | `randomizeLiveModDepths` writes all `modDepth[i]` slots, but `computeEffective` / `slotViewEffective` only apply lanes with `modSource[i] != kNoSelection`; single-source model leaves one active lane | **Confirmed** |
| OQ-09-5 | MOD label absent; assign-then-press required | `EncoderRingComponent` has no MOD affordance; `mouseDown` fires `onPress` on **whole ring** — would collide with drag-to-turn if drill-in moved to full encoder; manual promises MOD click on CV LED | **Confirmed — doc/impl gap; drill-in MUST NOT be whole-encoder click** |

## Operator product critique (authoritative intent)

1. **Retire module-row dropdown (`ModLanePicker`)** — obsolete once multi-depth model lands. The 15-lane parameter-detail grid is the sole modulation surface per row.
2. **Multi-lane depth model** — each manifest lane has independent bipolar depth; **depth 0 = off**. Multiple lanes MAY be non-zero simultaneously on the same parameter. No separate "pick one source" step.
3. **MOD-only drill-in (refined 2026-07-09)** — retire dropdown; open parameter detail by clicking the **center MOD/CV LED** only — **not** the ring (ring = drag-to-turn). LED is **always at geometric center** (fixed position). **Color and intensity** — not position — reflect modulation relative to the attenuated-range centerpoint. Idle: **greyed-out green MOD**; active: red/green bias per CV LED rules.
4. **Rand Mod semantics** — randomize depths across **all eligible lanes** on targeted rows (respecting manifest eligibility and depth-zero-off policy), not a single pre-selected route.
5. **Future layout (out of scope here)** — unified page showing all parameters at once becomes feasible once per-row mod routing drops the compact dropdown column. Note only; no packet in this change.

## Relationship to baseline spec

`openspec/specs/desktop-v2-mod-source-grid/spec.md` already states depth-zero/off per lane and fifteen simultaneous lane representations. Packet 2 implementation diverged: `setSingleModSource` enforces one route per row and `ModLanePicker` preserves v1-style pre-selection. This session records that divergence as **operator-invalid transitional behavior**, not the target contract.

## Disposition (mod routing)

Deferred to **Packet 15** (see `tasks.md`). Do not mark Packet 2 `[x]` entries as satisfying final operator contract until Packet 15 closes or tasks are explicitly re-opened.

---

## Top chrome + oscilloscope feedback (same session)

| ID | Observation | Code truth | Verdict |
|----|-------------|------------|---------|
| OQ-09-6 | One oscilloscope trace pinned flat at top | Default taps are VCO **EF** CVs (`kOscilloscopeTaps`); `CvScopeDisplay::sampleY` maps 0..1 **linearly on a shared axis**; a pegged/near-1 EF draws at the top edge and dominates | **Confirmed** — fix via per-trace auto-scale (Packet 16) |
| OQ-09-7 | Traces should show visible *activity* without one line dominating | Shared linear 0..1 Y; one high EF crushes others | **Confirmed — fix: per-trace auto-scale (operator-selected); exp/log rejected** |
| OQ-09-8 | Global command band: many controls, labels truncated `"..."` | `GlobalStripV2` packs two rows into `kGlobalCommandBandH` (60px); scope radios use fixed widths (`kGlobalScopeSceneAllW` 90px, `kGlobalScopeSceneCurrentW` 110px, etc.) on a second row that **reuses x positions under left buttons** — grid collision | **Confirmed** |
| OQ-09-9 | Dead space right of Shift | `GlobalStripV2::resized` stops laying out at Shift; **never fills** `commandRow` remainder to the right | **Confirmed** |
| OQ-09-10 | Performance band: empty grid cells + crushed/overlapping controls | Fixed constants laid left-to-right; gesture sliders + scene row compete for width; marbles columns only 60px (`kPerfMarblesColW`) for manifest tail `"Marbles 1"` | **Confirmed** |
| OQ-09-11 | UI says "Marbles" | Manifest `displayName` is `"Random/Marbles 1"`; `PerformanceBandV2::labelTailAfterSlash` → **"Marbles 1"** | **Confirmed — violates host-master / scope-viz UI naming** |
| OQ-09-12 | Shift button — does it do anything? | `MessageIn::ShiftHeld` is **no-op** in `FroggersV2ControlCore::applyMessage`; `PageCarouselComponent::setShiftHeld` ignores `held`; product contract says **no held-gesture model** | **Confirmed — dead chrome** |
| OQ-09-13 | External-audio mod lanes should be greyed out and OFF with no input | Baseline `desktop-v2-mod-source-grid` requires visible-but-unavailable/off; `ModLanePicker` disables menu items when `externalAudioAvailable` is false; **`FroggersV2ControlCore::setExternalAudioAvailable` is never called from `MainComponent`** (only oscilloscope gets runtime updates); parameter-detail grid has **no greyed/disabled lane styling**; depths not cleared on unavailability | **Confirmed — spec intent, implementation gap** |
| OQ-09-14 | 16th detail cell should say **Target (Back)** | `SubmodulePagePanel` / `AdsrPagePanel` hardcode label `"Target"` for `isTarget` cell | **Confirmed — operator label correction** |
| OQ-09-15 | Whole-encoder click must NOT open mod detail | `EncoderRingComponent::mouseDown` fires `onPress` on entire ring for drag-to-turn; full-encoder drill-in would trap users | **Confirmed — center MOD LED click only; ring drag separate** |
| OQ-09-16 | CV LED position vs attenuated-range semantics | Baseline spec wording can be read as LED *moving* to attenuated center; operator: LED **fixed at geometric center**; color/intensity encode attenuated-range bias | **Confirmed — clarify in Packet 15 spec delta** |
| OQ-09-17 | MIDI pressable encoder: turn vs press | Sheaf device-neutral model: rotation → `ParamTurn`, button → enter-mod action; control core already splits turn/press messages; manifest lacks per-row encoder targets today | **Confirmed — `ModDrillIn` is the device-neutral enter-mod action; MIDI wiring deferred to Packet 19** |

## Intent timeline (clarification)

| When | Operator said | Artifact status |
|------|---------------|-----------------|
| First live-test critique | Retire dropdown; multi-depth; open detail without pre-assign | **Packet 15** from that session — not new |
| Same session (refined) | Not whole encoder — click **MOD/CV LED** center only | **OQ-09-15**, task 15.8/15.2 updated below |

## Operator product critique (top chrome)

1. **Oscilloscope per-trace auto-scale (Packet 16, D13)** — each trace normalizes to its own recent min/max in the ring buffer before paint, so all three VCO traces show activity even when one EF sits pegged high on a shared axis. Display-only; engine CV unchanged. Exp/log Y rejected. Spike 16.1 still confirms EF taps vs spec wording.
2. **Global command band grid** — re-layout as an honest grid: no truncated scope radios under scene buttons; use remaining width right of Shift; increase band height or reflow if needed.
3. **Performance band grid** — audit `DesktopV2ChromeLayout` constants at 1280px; eliminate empty cells and overlapping scope/scene radios; ensure every slider/toggle has readable manifest-backed label text.
4. **Random source UI naming** — operator-facing labels **Random S&H 1** and **Random S&H 2** only (`froggers-host-master`, `desktop-v2-scope-visualization`, archived `desktop-v2-chrome-sequencer-ux`). **Never "Marbles" in UI.** Manual may say "inspired by Mutable Instruments Marbles" (`sim-operator-doc-parity`).
5. **Retire Shift** from global command band — no v2 behavior; MIDI mapping row may remain in Controllers page but on-screen Shift toggle is misleading dead chrome.

6. **External-audio mod lanes greyed and OFF** — when no external audio is active/available, **External Audio (audio rate)** and **External Audio (envelope follower)** lanes remain visible in parameter detail (and any interim mod UI) but render **greyed/disabled**, depth forced to **0/off**, non-editable, and excluded from Rand Mods. Wire `externalAudioAvailable` from the audio engine into `FroggersV2ControlCore` (today only the oscilloscope receives runtime updates).

7. **Detail grid cell 16 label** — the dedicated Crispy/target cell SHALL read **Target (Back)**, not **Target** alone. Pressing it closes parameter detail and returns to the module page (unchanged behavior; label communicates affordance).

8. **Device-neutral enter-mod (`ModDrillIn`)** — opening parameter-detail modulation is one semantic action across backends: center MOD LED click (mouse/touch), encoder button press (MIDI). Encoder rotation maps to `ParamTurn` only. Aligns with Sheaf portable-action model (`sheaf-adoption-inventory.md`). **Packet 15.2** delivers the message boundary; **Packet 19** wires per-row MIDI encoder manifest targets and dispatch.

## Disposition (top chrome)

Deferred to **Packets 16–19** (see `tasks.md`).
