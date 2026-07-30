## 1. Minimal panel SVG (`vcv/res/`)

- [x] 1.1 `FroggersTiga.svg` + dark (24 HP): gray fill, tiny corner “FroggersTiga” (~6–8 pt Comic Sans), black silkscreen for mod rack / CC / I/O labels from `ParamDisplayNames`
- [x] 1.2 `FroggersTigaVoicing.svg` + dark (48 HP): tiny corner brand + 4 column titles + 8 row labels per column
- [x] 1.3 `FroggersTigaFx.svg` + dark (36 HP): tiny corner brand + Reverb/Delay columns + stereo L/R labels

## 2. Layout constants

- [x] 2.1 Update `sim/VcvPanelLayout.hpp`: `kVoicingHp=48`, `kFxHp=36`, `kVoicingColumns=4`, `kFxColumns=2`, CC row Y = 10.5 grid
- [x] 2.2 Update `sim/check_vcv_panel_bounds.sh`: Voicing/FX HP, CC vs gate ≥2 GRID separation
- [x] 2.3 Verify bounds script in CI passes

## 3. Module topology (`plugin.cpp`)

- [x] 3.1 Replace Expander A with `FroggersTigaVoicingModuleT` (pages 0,1,3,4)
- [x] 3.2 Replace Expander B with `FroggersTigaFxModuleT` (pages 2,5 + stereo I/O)
- [x] 3.3 FX audio: sum L/R in → mono; duplicate mono out → L/R
- [x] 3.4 Update `plugin.json` slugs to Voicing/FX, version 2.4.0

## 4. Widget rewrite

- [x] 4.1 All widgets: `setPanel(createPanel(asset::plugin(...)))` + positions matching SVG
- [x] 4.2 Primary: CC row at Y=10.5; gate at 11.5 grid X; no coordinate overlap
- [x] 4.3 Replace `GreenRedLight` → `GreenLight` on mod rack and CC enables
- [x] 4.4 Voicing: 4 columns, `RoundSmallBlackKnob`, mod jacks
- [x] 4.5 FX: 2 columns + stereo jack row
- [x] 4.6 Remove `addPageTitleLabel` from `FieldParityWidget.hpp`

## 5. Build and manual Rack tests

- [x] 5.1 `arch -x86_64 make` exit 0
- [x] 5.2 `arch -x86_64 ./build.sh --install`
- [ ] 5.3 Manual: all labels visible at 100% without hover; tiny corner brand present, not dominant
- [ ] 5.4 Manual: Voicing + FX + Primary chained; Random 1 → Audio patch works
- [ ] 5.5 Manual: green LEDs only on mod rack — no red state, no scopes
- [x] 5.6 Update `vcv/DEVELOPMENT.md` topology + migration from Expander A/B

## 6. Cross-change hygiene

- [x] 6.1 Note in `vcv-vst-field-parity-panel/design.md`: VCV minimal gray SVG + LED-only; VST unchanged
