# Web chrome cohesion — tasks

## 1. Label parity

- [x] 1.1 `main.ts`: `INTERNAL_MOD_LABELS` + `MOD_SOURCE_LABELS` → **VCO level**; dropdown HTML template option text
- [x] 1.2 `index.html`: global strip → **Rand All**, **Rand Mods**, **Rand waves**, **Marbles**
- [x] 1.3 Footnote `web-sim-page-ux/proposal.md` global strip label supersession

## 2. Mod meter visibility

- [x] 2.1 `main.ts`: module-level `modMeterDisplay` cache + `renderModBay(levels, audioRunning)`
- [x] 2.2 Idle: dim fill at last level when `!audioRunning` (class `mod-meter-fill--idle`)
- [x] 2.3 Step: detect Marbles Δ > 0.02 → `mod-cell-step` class, clear after 200ms
- [x] 2.4 `index.html` + CSS: mod bay hint **CV trace while playing** (deduped in `web-sim-bootstrap-repair`)

## 3. Mobile touch targets

- [x] 3.1 `style.css`: `min-height: 44px` on transport + external + global-strip buttons
- [x] 3.2 Verify wrap on 390px — no clipped strip labels

## 4. Layout reading order

- [x] 4.1 `index.html`: move `#mod-route-summary` below `#page-chrome`

## 5. Verification

- [ ] 5.1 390px: no horizontal scroll; strip + transport ≥44px
- [ ] 5.2 Play → Marbles → stop: meters dim but non-zero
- [ ] 5.3 Route summary + dropdown show **VCO level**
- [x] 5.4 `npm run build` (web) succeeds
- [ ] 5.5 §F mobile layout still passes with chrome + pills (`web-sim-page-ux` §7.1)
- [ ] 5.6 Delay page: hints visible; per-page Randomize mod changes dropdowns + summary (`web-sim-page-ux` §7.2)
- [ ] 5.7 Audio page: route summary lists modulated rows; wave SVG cycles on tap (`web-sim-page-ux` §7.3)
- [ ] 5.8 Global Randomize mod (all) from Delay page updates core + Delay summary when navigating (`web-sim-page-ux` §7.4)
- [ ] 5.9 Mod bay collapse on 390 px — knobs usable without horizontal scroll (`web-sim-page-ux` §7.5)
