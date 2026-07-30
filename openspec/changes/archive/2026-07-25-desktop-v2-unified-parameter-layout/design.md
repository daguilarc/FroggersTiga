# Design — Unified parameter layout + module-content convergence

## Guiding principle

The carousel existed to fit one module at a time when each row carried a mod dropdown. Packet 15 removes that column, so the constraint is gone. This change spends the reclaimed space on **showing everything at once**, not on a prettier carousel.

## U1 — Single unified parameter surface replaces the carousel

The center content area SHALL render **all module sections at once** (Audio, Envelope, Filter, Distortion, Random S&H, Reverb, Delay) rather than one carousel page. `PageCarouselComponent`'s prev/next paging and single-active-page model are retired. Module sections are laid out as titled column groups on the shared 10px grid; each section shows all its manifest-visible parameter rows.

**Open layout question (Packet 1 spike):** the widest module has 10 parameter rows; seven sections must coexist at 1280×920 with the chrome relocated (U3). The spike SHALL confirm a concrete arrangement (e.g. multi-column module sections) that fits every parameter without vertical scroll, or report the exact overflow so section grouping / density is adjusted before build. No layout packet lands until the spike confirms fit.

## U2 — Per-module Randomize via existing control-core authority

Each module section header carries a **Randomize** button dispatching `MessageIn::Type::RandPage` with that section's page index onto the control-core bus — the same single-authority path retained through operator-truth-repair Packet 13. No new randomize message type, no parallel mutator, no revival of the removed `EnqueueRandomizePanelMod`/panel `onRandomize` chain (deleted in operator-truth-repair Packets 4/6/13). Rand-All/Rand-Mods scope semantics are unchanged; per-module Randomize is a page-scoped convenience over the existing authority.

## U3 — Relocate transport + global-command band beside the oscilloscope

The transport cluster (Play/Stop/Record) and the global-command band (Rand All, Rand Mods, scope radio pairs, Crunchy) move to the **right of the oscilloscope** in the top band, reclaiming the vertical space the carousel header + arrows occupied. The oscilloscope keeps its position/size; controls fill the width to its right (eliminating the dead margin operator-truth-repair OQ-09-9 flagged). This change relocates the **already-corrected** chrome from operator-truth-repair Packets 16–18; it does not re-derive label/grid fixes.

## U4 — Third cross-coupler (VCO 1/3)

`froggers-v2-app-manifest` SHALL add a VCO **1/3** cross-coupler parameter row to the Audio page, alongside the existing 1/2 and 2/3 couplers, so the Audio section exposes all three pairings of the three cross-coupled VCOs. Row eligibility, stable ID, and host-parameter registration follow the same manifest-owned pattern as 1/2 and 2/3 (single authority — no parallel enum). Host-parameter count and Audio-page row count increase accordingly; projection validators re-baseline.

## U5 — ASR Envelope page

The Envelope module SHALL expose, per VCO (1, 2, 3): **Attack**, **Sustain**, **Release** — a held-sustain envelope (ASR), not attack/release only. Operator-visible labels use full words (no "Attk"/"Rel"). The page is titled **Envelope** (the internal/UI "Pair-AR" name is retired) and is fixed immediately right of Audio in the unified surface (the baseline product contract already places "Envelope after Audio/VCO").

**Sustain semantics:** Sustain is a **level** (0..1) held while the gate is high; Release is the fall time after gate-off. No decay-to-sustain knee (that would be full ADSR — explicitly out of scope per 2026-07-09 decision). New Sustain rows are manifest-owned; host-parameter/row counts increase; projection validators re-baseline.

## Sequencing

Packet 1 (spike) gates everything. Layout packets (2–4) precede or parallel module-content packets (5–6) but all assume operator-truth-repair Packet 15 has removed the mod column. Tests + docs + manual QA close (Packet 7).

## Open questions

- **U1 fit at 1280×920** — resolved by Packet 1 spike; may force multi-column section density or a scroll-fallback decision if seven full sections genuinely do not fit.
- **Manifest count churn** — U4 + U5 add rows/host-params; every `FroggersV2ProjectionValidators_test` and manifest-count assertion (currently 65 rows / 142 host params / 15 sources) must be re-baselined in the same packet that adds the rows, not left failing.
- **Envelope engine** — does the sim/DSP layer already support a sustain stage per VCO, or is ASR a new engine capability? Packet 6 spike confirms whether Sustain is a wiring change or a DSP addition before committing the manifest rows.

**Shared archive gate:** the Packet 1 (fit) and Packet 6 (sustain) spikes above, plus `operator-truth-repair` **15.0** (engine N-lane summation), form one three-spike gate that blocks archiving **both** changes — a spike forcing a scope change pauses archive so no spec delta baselines an unbuildable requirement. See `tasks.md` §"Archive gate (shared across both desktop-v2 changes)".
