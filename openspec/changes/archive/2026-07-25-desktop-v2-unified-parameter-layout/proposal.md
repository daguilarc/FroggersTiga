# Unified parameter layout + module-content convergence

> **ABSORBED / CANCELLED for standalone execution (2026-07-20).**  
> Live owner: [`desktop-v2-sheaf-runtime-harmonization`](../desktop-v2-sheaf-runtime-harmonization/).  
> Absorbed: carousel retirement, unified surface, chrome relocate, VCO 1/3 coupler, ASR Envelope.  
> **Not** absorbed: Random S&H as a module section — that page and bag params are **deleted** in the harmonization change (mod lanes + ganged visualizer only).  
> Do not implement tasks from this change; archive when harmonization is accepted.

## Why

Once `desktop-v2-operator-truth-repair` Packet 15 removes the per-row `ModLanePicker` dropdown column (modulation moves entirely into the parameter-detail 4×4 grid reached by MOD-LED drill-in), the horizontal space each module row spent on a mod dropdown is reclaimed. At that point the **page carousel is no longer necessary**: every module's parameters can be shown at once on a single 1280×920 surface, with the transport/global-command controls relocated beside the oscilloscope to reclaim vertical space.

Live operator review (2026-07-09, `../desktop-v2-operator-truth-repair/operator-qa-2026-07-09.md` and chat) also surfaced **module-content gaps against the existing product contract** that are not layout bugs — they are unbuilt requirements:

- `froggers-v2-product-contract` names **three cross-coupled VCOs**, but the Audio page exposes only VCO 1/2 and VCO 2/3 cross-couplers — the **1/3** pair is missing.
- The contract names an **"Envelope page after Audio/VCO"**, but the implementation ships a mis-named **"Pair-AR"** page with **attack/release only** and abbreviated labels, positioned via the carousel rather than fixed after Audio.
- Operator intent (2026-07-09): the envelope is **ASR** — add a per-VCO **Sustain** level (held while gated; release after gate-off), full-word labels **Attack / Sustain / Release**, page titled **Envelope**, immediately right of Audio.

This change is the operator's stated "future layout" that `operator-qa-2026-07-09.md` §Operator-product-critique item 5 explicitly deferred out of the operator-truth-repair change.

## What changes

1. **Retire the page carousel.** Replace one-module-at-a-time paging with a single unified parameter surface: every module section (Audio, Envelope, Filter, Distortion, Random S&H, Reverb, Delay) and all its parameters visible at 1280×920, no carousel arrows, no per-page module scroll.
2. **Per-module Randomize.** A Randomize button under each module-section header, wired to the existing control-core `RandPage(page)` authority message (single authority — no parallel randomize path).
3. **Relocate top chrome.** Move transport (Play/Stop/Record) and the global-command band (Rand All / Rand Mods / scope pairs / Crunchy) to the right of the oscilloscope, reclaiming the vertical band the carousel header used, so all parameters fit without scrolling.
4. **Third cross-coupler.** Add the VCO **1/3** cross-coupler to the Audio module (Audio now exposes 1/2, 2/3, 1/3), manifest-owned.
5. **ASR Envelope page.** Envelope module exposes per-VCO **Attack / Sustain / Release** (ASR) with full-word labels; page titled **Envelope** (retire "Pair-AR"); fixed immediately right of Audio in the unified surface.

## Dependencies

- **BLOCKED ON** `desktop-v2-operator-truth-repair` **Packet 15** (`ModLanePicker` removal / mod column reclaim, design D12/D17, task 15.7). The unified surface assumes module rows carry **no** mod dropdown column and that parameter-detail is the sole modulation editor. Do not start this change's layout packets until 15.7 lands.
- Builds on the corrected chrome from operator-truth-repair Packets 16–18 (oscilloscope auto-scale, honest grid, Random S&H naming, Shift removal) — this change relocates that corrected chrome, it does not re-fix it.

## Non-goals

- Multi-depth mod routing, `ModDrillIn`, MOD/CV LED behavior — **Packet 15** of operator-truth-repair.
- Oscilloscope auto-scale, top-chrome grid re-layout, Random S&H naming, Shift removal, MIDI encoder targets — **Packets 16–19** of operator-truth-repair.
- Full ADSR (decay knee) — this change ships **ASR** per operator decision 2026-07-09.
- Web parameter subset (`web-v2-parameter-subset`) and VST/AU hosted editor layout — unchanged.
