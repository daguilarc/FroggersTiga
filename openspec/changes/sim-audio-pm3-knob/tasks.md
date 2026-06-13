## 1. Engine (sim-only PM3 routing)

- [x] 1.1 Add `RuntimeParam m_pm3`, `bool m_simDedicatedPm3Knob`, and `SetSimDedicatedPm3Knob(bool)` to `FroggersEngine.hpp` (include `&m_pm3` in `ApplySmoothingRates`)
- [x] 1.2 In `ReadParamsBlock`: when flag true, `m_pm3.SetTarget(ZeroedExp(GetParam(6)))` and `m_oscLvl.SetTarget(ExpMap(0.01f, 1.0f, 0.4f))`; else keep existing OLVL from GetParam(6)
- [x] 1.3 In `StepOscillators`: when flag true, `pm3d = m_pm3.Process()`; else keep `pm3d = ZeroedExp(fuegKnob)`
- [x] 1.4 Call `SetSimDedicatedPm3Knob(true)` in `DesktopHostIO::Init()` and `PagedHostIO::Init()` next to `SetSimWaveMorph(true)`

## 2. Display names and static labels

- [x] 2.1 `sim/ParamDisplayNames.hpp` Audio row 6: **Phase mod 3**; row 7 all pages: **Crispy** (replace **VCO Envelope** / **Crunch**)
- [x] 2.2 `web/src/main.ts` `HOST_PAGE_LABELS` Audio row 6: **Phase mod 3**; row 7: **Crispy**
- [x] 2.3 Desktop `SubModulePanel` row 7 via `getRowName(7)` (no ad hoc **Crunch** init string)

## 3. Documentation

- [x] 3.1 Update `QUICK_DICT.md` Audio section: row 6 **Phase mod 3**; **Crispy** sim gloss drops PM3 (Field note only in `MANUAL.md`)
- [x] 3.2 Update `SIM_MANUAL.md` Audio table row 7 → **Phase mod 3** with one-line behavior (VCO2 → VCO3 when 2→3 coupling)
- [x] 3.3 Mirror to `web/public/quick-dict.md` and `web/public/sim-manual.md`; run `npm run sync:docs`

## 4. Verification

- [x] 4.1 Desktop: Audio page, coupling CW, row 6 affects timbre; Crispy min + row 6 up still has PM3
- [x] 4.2 Web: label reads **Phase mod 3** / **Crispy**; mod source **VCO Envelope** unchanged in mod bay
- [x] 4.3 Firmware build path does not call `SetSimDedicatedPm3Knob` (Field parity on OLVL/FUEG unchanged)
