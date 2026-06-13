## 1. Postmortem review (reviewing agent)

- [x] 1.1 Open live GitHub Pages sim at 375px DevTools; screenshot Audio page — confirm right-shift and waveform stacking
- [x] 1.2 Repeat at 960px — note whether regression is mobile-only or all viewports
- [x] 1.3 Inspect `.knob-col` computed width vs `.mod-select` / `.knob-row` — confirm or reject H1 (overflow)
- [x] 1.4 Inspect overlap layers on VCO morph — confirm or reject H3 (z-index / DOM paint order)
- [x] 1.5 Read `git show b4f15dd` diff for `web/src/style.css` and `web/src/main.ts`
- [x] 1.6 Read archived intent: `openspec/changes/archive/2026-06-13-web-knob-col-mobile-align/`

## 2. Fix implementation (applied in this session)

- [x] 2.1 Implemented fix inline (follow-up change deferred; handoff retained for archive)
- [x] 2.2 Apply containment fix: `min-width: 0` on column children, constrain `.mod-select` to cell width, revert `.knob-col` to flex column
- [x] 2.3 Fix morph/knob vertical order: label → morph → knob → hint
- [x] 2.4 Restore in-cell stacking: `isolation: isolate`, morph `z-index: 1`, `overflow: clip` on `.knob-col`
- [x] 2.5 Visual gate: `npm run build` passes; manual screenshot on deploy

## 3. Process

- [x] 3.1 CSS layout fix applied per design D1–D4
- [ ] 3.2 Archive this handoff change after fix is merged and verified
