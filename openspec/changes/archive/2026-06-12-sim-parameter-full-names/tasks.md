# Sim parameter full names — tasks

## 1. Shared dictionary

- [x] 1.1 Add `sim/ParamDisplayNames.hpp` with full table (six pages × eight rows; row 7 = **Crunch**)
- [x] 1.2 Wire into `desktop/CMakeLists.txt` / include path if needed

## 2. Desktop

- [x] 2.1 `DesktopPanelBackend::getRowName` → `ParamDisplayNames::forHostPageRow`
- [x] 2.2 `DelayHostBackend::getRowName` → page 5 lookup
- [x] 2.3 Bump `kMinLabelWidth` to 72 if **Cross-coupler** / **Comb feedback** clip at 1680×720
- [x] 2.4 Tooltips on **Crunch** row: mention Field `FUEG` in Manual (optional one line)

## 3. WASM + web

- [x] 3.1 `wasm/bindings.cpp`: screen rows use display names
- [x] 3.2 `npm run build:wasm`
- [x] 3.3 `web/src/main.ts`: page blurbs say Crunch not fuegoizer; trim redundant `DELAY_HINTS` where label suffices
- [x] 3.4 `web/src/froggers-processor.ts`: align `DELAY_ROW_NAMES` comments or remove if unused in UI

## 4. Quick Dict

- [x] 4.1 Rewrite `QUICK_DICT.md` with full left tokens + short glosses (**Crunch** not Fuegoizer)
- [x] 4.2 Run `npm run sync:docs` (or build) so `web/public/quick-dict.md` matches

## 5. Verification

- [ ] 5.1 Desktop: all six panels show full titles; knob 8 = **Crunch** on each
- [ ] 5.2 Web: pages 1–6 knob columns + OLED mock match desktop strings (completed with `web-sim-core-fix` apply — includes **Filter delay**)
- [ ] 5.3 About → Quick Dict left tokens match on-screen labels
- [x] 5.4 Desktop Release build + `npm run build`
