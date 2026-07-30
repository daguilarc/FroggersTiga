## 1. Engine

- [x] 1.1 In `MixExternalAndOsc`, remove product term, `ZeroedExp(fueg)`, and `fueg` parameter; return parallel average when `hasExternal`
- [x] 1.2 Update `ProcessSample` call site to match new `MixExternalAndOsc` signature

## 2. Field manual

- [x] 2.1 Update `MANUAL.md` signal-flow ascii: external present → parallel ring mod only (no FUEG continuum)
- [x] 2.2 Remove mix-topology table (lines 27–34); external active = parallel ring mod only
- [x] 2.3 Update pages table Audio row 8: drop “mix topology”; keep fuegoizer + PM3 pointer
- [x] 2.4 Rename “Audio page exception: PM3 and mix topology” → “Audio page exception: PM3”; drop mix-topology bullet and “entanglement continuum” phrasing
- [x] 2.5 Update Audio knob 8 table row: fuegoizer + PM3 only
- [x] 2.6 Note pair-AR applies to VCO-only mix, not external ring-mod path
- [x] 2.7 Audio page: plain-language cross-coupler, PM1–3, parallel external ring mod (match sim manual tone)
- [x] 2.8 Marbles page: Mutable Instruments Marbles inspiration (one line)
- [x] 2.9 Update `QUICK_DICT.md` Field + Global glosses (FUEG, Ext. In.) — no topology blend; sync `docs/quick-dict.md` and `web/public/quick-dict.md`

## 3. Sim manual rewrite (all mirrors)

- [x] 3.1 Rewrite `SIM_MANUAL.md` in learner order (see `proposal.md` Sim manual rewrite): Getting sound → External → Randomize strip → Crispy → signal path → pages → mod bay → host boxes → appendix
- [x] 3.2 Fix desktop layout copy: six equal columns **Audio → Random → Drive → Filter → Reverb → Delay**; delete “Delay overlay” / “five panels” wording
- [x] 3.3 Audio section: three VCOs, cross-coupler (CCW 1→2 / CW 2→3), PM1–3 gating, parallel external ring mod when Ext. In. gate open, VCO-only when silent
- [x] 3.4 Random section: Mutable Instruments Marbles inspiration; bags, Rand Resample, mod-rack Random 1/2 S&H outputs
- [x] 3.5 Drive → Filter → Reverb → Delay page sections in learning order; each notes Crispy on row 8
- [x] 3.6 Crispy gloss: scramble knobs 1–7, mod-then-scramble, moddable; no external-mix / topology language
- [x] 3.7 Compress mod bay + per-host MIDI matrices into mod-bay section and appendix (not before Play/External)
- [x] 3.8 Copy rewritten manual to `docs/sim-manual.md` and `web/public/sim-manual.md` (byte-identical body)
- [x] 3.9 Update `docs/quick-dict.md`, `web/public/quick-dict.md`, root `QUICK_DICT.md` — Crispy and Ext. In. glosses match new manual tone; no topology blend

## 4. Verification

- [x] 4.1 Build firmware (`src/FroggersTiga`) and sim hosts; confirm compile clean
- [x] 4.2 Run web e2e if any test asserts external-mix topology copy
- [x] 4.3 Grep repo for stale topology language (`product ring`, `mix topology`, `parallel ring mod` morph, `FUEG continuum`, `entanglement continuum`, `blends external ring-mod`, `topology into the mix`, `Delay overlay`); remove hits outside changelog/history
- [x] 4.5 Read-through: manual opens with Play/Stop; Audio precedes Random in page sections; desktop Delay described as sixth column
- [x] 4.4 Behavioral check: external gate open, sweep Audio knob 8 — pre-drive timbre unchanged; fuegoizer/scramble on knobs 1–7 still responds

## 5. Main spec archive

- [x] 5.1 Sync all delta specs to `openspec/specs/` (`external-ring-mod-mix`, `field-operator-doc-parity`, `sim-operator-doc-parity`, `sim-pm3-knob-parity`)
