## Why

Commit `b4f15dd` (`web-knob-col-mobile-align`) regressed the web sim knob field: on mobile (and possibly desktop), labels, knobs, and VCO waveforms render **to the right of** their bordered `.knob-col` boxes, and waveforms sit **behind** knobs/text. The prior change fixed horizontal morph bleed but introduced a worse alignment/stacking failure. This change is a **postmortem handoff** for a reviewing agent — not an immediate fix PR.

## What Changes

- Document observed failure modes with screenshots checklist for reviewer
- Rank root-cause hypotheses grounded in `b4f15dd` diff (`web/src/style.css`, `web/src/main.ts`)
- Specify acceptance criteria for a follow-up fix change
- Recommend fix strategy (OMNI-compliant) without implementing in this change

## Capabilities

### New Capabilities

- `web-knob-col-align-fix`: Correct knob-column containment — content centered inside bordered cells, morph visible and tappable, no cross-cell overlap.

### Modified Capabilities

- (none in this handoff — fix change will modify `web-knob-column-stack`, `web-vco-morph-inline`, `web-vco-morph-mobile`)

## Impact

- **Broken commit:** `b4f15dd` on `main` (live via GitHub Pages after deploy)
- **Files to inspect:** `web/src/style.css` (`.knob-col`, `.knob-row`, `.mod-select`, `.vco-morph-btn`), `web/src/main.ts` (column DOM order lines ~361–470)
- **Reference:** `openspec/changes/archive/2026-06-13-web-knob-col-mobile-align/` (intent vs outcome)
- **Good baseline:** `e047049` (side-by-side morph — mobile bleed but content inside boxes)
