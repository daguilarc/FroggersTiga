## 1. Manual restructure

- [x] 1.1 Rewrite `SIM_MANUAL.md` intro and Quick start (play-first, no `ParamDisplayNames` in opener)
- [x] 1.2 Add **Layout** and **Global controls** sections (Crispy once, grouped transport table)
- [x] 1.3 Restructure **Mod bay** — fix broken bullet list; single subsections for CC enable, mod indicators, Random S&H
- [x] 1.4 Convert page sections to 3-column tables (Row | Parameter | What it does) with cross-refs to Global Crispy
- [x] 1.5 Rewrite **Host guide** (Desktop / Web / VST) after page reference; remove "no longer controls PM3" phrasing
- [x] 1.6 Verify Audio PM3 / Crispy / VCO Envelope distinction matches `ParamDisplayNames.hpp` and current DSP behavior

## 2. Quick Dict cadence

- [x] 2.1 Rewrite `QUICK_DICT.md` to em-dash format and section order from design
- [x] 2.2 Add **Global** Crispy entry; remove six per-page Crispy duplicates
- [x] 2.3 Fix **Rand mod** spelling; add CC disable one-liner on MIDI CC entries
- [x] 2.4 Align mod-source and transport glosses with rewritten manual (semantic parity, tighter wording)

## 3. Mirror sync and guardrails

- [x] 3.1 Copy canonical `SIM_MANUAL.md` → `docs/sim-manual.md` and `web/public/sim-manual.md`
- [x] 3.2 Copy canonical `QUICK_DICT.md` → `docs/quick-dict.md` and `web/public/quick-dict.md`
- [x] 3.3 Add `sim/check_operator_docs_sync.sh` and wire into existing sim check/CI path
- [x] 3.4 Run sync check locally; confirm all four mirror pairs are byte-identical

## 4. Optional UI copy alignment

- [x] 4.1 Update `web/src/main.ts` `PAGE_BLURBS[0]` if still saying "output level" (match Audio page semantics)
- [x] 4.2 Smoke-test web help modal loads Manual and Quick Dict without broken markdown rendering

## 5. Verification

- [x] 5.1 Diff review: no factual regression vs current `SIM_MANUAL.md` (CC gating, VST MIDI Settings, PM3 row 7)
- [x] 5.2 Confirm each manual dedup requirement from `specs/sim-manual-structure/spec.md` is satisfied
- [x] 5.3 Confirm Quick Dict format requirements from `specs/sim-quick-dict-cadence/spec.md` are satisfied
