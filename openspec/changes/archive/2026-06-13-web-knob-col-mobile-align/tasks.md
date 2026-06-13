## 1. DOM structure

- [x] 1.1 In `main.ts` knob column build: append `vco-morph-btn` after `knob-label-main`, before `knob-hint`; keep morph only for indices 0–2
- [x] 1.2 Append `.knob-morph-slot` placeholder (28×28, `aria-hidden`) after label for indices 3–7
- [x] 1.3 Remove morph append from `.knob-row`; row holds rotary knob only

## 2. CSS column stack

- [x] 2.1 `.knob-col`: `display: grid; grid-auto-rows: auto; justify-items: center; gap: 0.25rem`
- [x] 2.2 `.knob-morph-slot`: `width/height: 28px` invisible spacer
- [x] 2.3 `.knob-row`: flex center, knob-only — remove `grid-template-columns: 1fr auto`
- [x] 2.4 `.vco-morph-btn`: remove `position: relative; z-index: 1`; keep 28×28 tap target, centered in column
- [x] 2.5 Remove `.knob-row .rotary-knob { justify-self: center }` (obsolete)

## 3. Verify

- [x] 3.1 DevTools 375px Audio: morph between label and knob; no overlap with neighbor columns; knobs centered in cells
- [x] 3.2 DevTools 960px Audio: 4-column grid unchanged; morph between label and knob; mod bay alignment intact
- [x] 3.3 Morph click + Rand waveforms still update SVGs; engine guard unchanged
- [x] 3.4 `cd web && npm run build`

## 4. Archive

- [x] 4.1 Archive change; merge specs into `openspec/specs/web-knob-column-stack/`, update `web-vco-morph-inline` and `web-vco-morph-mobile`
