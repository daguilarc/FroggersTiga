## 1. UX cleanup

- [x] 1.0 Remove mod-bay hint “CV trace while playing” from `web/index.html`; drop unused `.mod-bay-hint` CSS

## 2. CSS layout

- [x] 2.1 `@media (max-width: 720px)`: `.knobs { grid-template-columns: repeat(2, minmax(0, 1fr)); }`
- [x] 2.2 Replace `.knob-row` flex with `grid-template-columns: 1fr auto`; center knob in col 1, morph in col 2
- [x] 2.3 Add `.vco-morph-btn { position: relative; z-index: 1; }` and `.knob-row .rotary-knob { justify-self: center; }`

## 3. Phone mic limiter

- [x] 3.1 Add `softLimit`, `EXT_IN_PAD`, `EXT_IN_DRIVE` to `froggers-processor.ts`; limit in external loop before `heap.set`; peak from limited samples
- [x] 3.2 In `main.ts` `setExternalEnabled`, use `matchMedia("(max-width: 720px)")` to set mobile `echoCancellation` / `noiseSuppression` on `getUserMedia`
- [x] 3.3 Bump `x_extInputLimiterDrive` to `5.0f` in `src/core/FroggersEngine.hpp`
- [x] 3.4 Rebuild WASM and copy to `web/public/froggers.wasm`

## 4. Verify

- [x] 4.1 DevTools 375px: Audio page — morph waveforms visible and tappable
- [x] 4.2 DevTools 721px+: 4-column layout unchanged
- [x] 4.3 Mobile or simulated: External on + Play — no runaway feedback clip; meter moves without pegging full scale on room noise
- [x] 4.4 Desktop wide: External on — echoCancellation remains false

## 5. Archive

- [x] 5.1 Archive change; merge specs into `openspec/specs/web-vco-morph-mobile/` and `openspec/specs/web-phone-mic-limit/` (or combined spec)
