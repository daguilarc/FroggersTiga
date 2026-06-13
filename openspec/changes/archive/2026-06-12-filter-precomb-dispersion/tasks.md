## 1. PureDelay + engine remap

- [x] 1.1 `Comb.hpp`: replace `SetDelaySamples(freq)` with `SetDelaySeconds(seconds)` using `m_sampleRate`
- [x] 1.2 `FroggersEngine.hpp`: replace `m_pureDelayFreq` with seconds target `ExpParam(0.001f, 0.1f, knob0)` in `ReadParamsBlock`
- [x] 1.3 `ApplyOutputFx`: call `m_pureDelay.SetDelaySeconds(...)` from processed runtime param
- [x] 1.4 Firmware build still passes (`make` in `src/FroggersTiga`)

## 2. Display names + docs

- [x] 2.1 `ParamDisplayNames.hpp` Filter row 0 → **Comb offset**
- [x] 2.2 `QUICK_DICT.md` + `npm run sync:docs` → **Comb offset** gloss
- [x] 2.3 `web/src/main.ts` Filter page blurb mentions comb offset (not comb line)

## 3. WASM + verify

- [x] 3.1 `npm run build:wasm`
- [ ] 3.2 Desktop Filter row 0 label **Comb offset**; knob 1 audibly longer smear than knob 0
- [ ] 3.3 Web Filter page shows **Comb offset** after `web-sim-bootstrap-repair` lands
