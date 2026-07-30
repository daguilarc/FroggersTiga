## Prerequisite (blocking)

- [ ] P0 Confirm `desktop-v2-operator-truth-repair` Packet 15.7 has landed (module rows carry no `ModLanePicker` mod column; parameter-detail is the sole modulation editor). Do NOT start Packet 2+ until this holds. **Note:** 15.7 is itself downstream of the upstream **15.0 parent engine-scope spike** (does the engine sum N lanes per row, or is that new DSP? — `operator-truth-repair` design.md D11a). If 15.0 resolves to new DSP, Packet 15's scope/timeline slips and so does this change's start.

---

## Packet 1 — Unified-layout fit spike (gates everything)

- [ ] 1.1 Measure: at 1280×920 with the mod column removed (post-15.7) and top chrome relocated (U3 target), can all seven module sections (Audio, Envelope, Filter, Distortion, Random S&H, Reverb, Delay) show every manifest-visible parameter without vertical scroll? Produce a concrete arrangement (multi-column module sections) or report exact overflow.
- [ ] 1.2 Decide section grouping / density from 1.1 (column count, per-section header + Randomize button footprint). Record in `design.md` U1.
- [ ] 1.3 Gate: no layout packet proceeds until 1.1 confirms fit (or the density decision in 1.2 makes it fit).

---

## Packet 2 — Retire carousel; unified parameter surface

- [ ] 2.1 Spec delta `desktop-v2-page-carousel`: retire prev/next paging + single-active-page; center area renders all module sections at once (U1)
- [ ] 2.2 `DesktopV2ChromeLayout`: unified module-section grid geometry from the Packet 1 decision; remove carousel arrow / header band
- [ ] 2.3 Replace `PageCarouselComponent` single-active-page model with all-sections rendering; each section titled, all its manifest rows visible; no per-section scroll
- [ ] 2.4 `LayoutBounds_test`: all module-section rows visible at 1280×920; no vertical scrollbar; no overlap

---

## Packet 3 — Per-module Randomize buttons

- [ ] 3.1 Each module-section header renders a **Randomize** button dispatching `MessageIn::Type::RandPage(page)` on the control-core bus (U2 — existing authority; no new message, no parallel mutator)
- [ ] 3.2 Confirm no revival of removed `EnqueueRandomizePanelMod` / panel `onRandomize` chain (rg = 0)
- [ ] 3.3 `ControlCoreBridge_test` / `GlobalControlParity_test`: per-module Randomize randomizes only that page; global Rand All/Mods scope unchanged

---

## Packet 4 — Relocate transport + global-command band

- [ ] 4.1 `DesktopV2ChromeLayout` + shells: move transport + global-command band to the right of the oscilloscope (U3); fill width to scope's right, no dead margin
- [ ] 4.2 Preserve corrected chrome from operator-truth-repair 16–18 (auto-scale scope, honest grid, Random S&H labels, no Shift) — relocate, do not re-fix
- [ ] 4.3 `LayoutBounds_test` + manual QA: chrome readable beside scope at 1280×920; reclaimed vertical space visible in parameter surface

---

## Packet 5 — Module content: VCO 1/3 cross-coupler

- [ ] 5.1 Spec delta `froggers-v2-product-contract`: Audio page exposes cross-couplers for VCO 1/2, 2/3, **and 1/3**
- [ ] 5.2 `froggers-v2-app-manifest`: add VCO 1/3 cross-coupler row (stable ID, eligibility, host-param registration) — single authority, pattern of 1/2 & 2/3
- [ ] 5.3 Re-baseline manifest-count assertions + `FroggersV2ProjectionValidators_test` (rows / host params) in this packet
- [ ] 5.4 Tests: 1/3 coupler present, routable, projects to desktop + hosted

---

## Packet 6 — Module content: ASR Envelope page

- [ ] 6.1 Spike: does the sim/DSP layer support a per-VCO sustain stage, or is ASR a new engine capability? (design.md open question)
- [ ] 6.2 Spec delta `froggers-v2-product-contract`: Envelope page exposes per-VCO **Attack / Sustain / Release** (ASR); full-word labels; titled **Envelope**; fixed right of Audio
- [ ] 6.3 `froggers-v2-app-manifest`: add 3 Sustain rows (per VCO); retire "Pair-AR" internal/UI naming → Envelope
- [ ] 6.4 Sustain = held level while gated; Release = post-gate fall; no decay knee (ASR, not ADSR)
- [ ] 6.5 Re-baseline manifest-count assertions + projection validators
- [ ] 6.6 Tests: attack/sustain/release per VCO; sustain holds while gated; label + page-title correctness

---

## Packet 7 — Docs + manual QA

- [ ] 7.1 `SIM_MANUAL.md` / `QUICK_DICT.md` (+ mirrors): unified all-parameters surface (no carousel), per-module Randomize, relocated chrome, three cross-couplers, ASR Envelope; `bash sim/check_operator_docs_sync.sh`
- [ ] 7.2 Manual operator QA at 1280×920: all params visible without scroll; per-module Randomize works; chrome beside scope; 1/3 coupler present; Envelope shows A/S/R per VCO
- [ ] 7.3 `openspec validate desktop-v2-unified-parameter-layout --strict`

---

## Archive gate (shared across both desktop-v2 changes)

Identical to `desktop-v2-operator-truth-repair` (execution-plan addendum). Both changes archive under one gate:

- **All three spikes resolved favorably:** **15.0** engine (N-lane summation shape, `operator-truth-repair`), **Packet 1** fit (all sections + params at 1280×920, this change), **Packet 6** sustain (per-VCO ASR: DSP vs wiring, this change). A spike that forces a scope change **pauses archive** — a spec delta (Packet 5/6 product-contract, Packet 2 carousel) must not baseline an unbuildable requirement.
- **All automated gates green:** full `ctest`, `check_subagent_packet_gates.sh` exit 0, `openspec validate --strict` on **both** changes.
- **Decisions belong to the parent (omni-rule §4):** the 15.0, Packet 1, and Packet 6 spikes are parent adjudications; subagents gather evidence, they do not decide.
- **Manual/visual items are marked `UNVALIDATED-AT-ARCHIVE — record on test`, not `[x]`** (contract-honest, not a false "done"). In this change that is **task 7.2** (1280×920 operator pass); its results are appended to the archived change after the manual test.

---

### Subagent execution contract

**Authority:** `scripts/SUBAGENT_OMNI_CONTRACT.md` — verbatim in every dispatch. Capped builds (`nice -n 10 … -j2`). Model: haiku for mechanical transcription with parent-authored diffs; sonnet only for the Packet 1/6 spikes and layout judgment.

**Order:** P0 → 1 → (2,3,4) → (5,6) → 7. Packet 1 spike gates 2–4.
