## 1. Engine + authority (OMNI)

- [ ] 1.1 Add `VcoArState.hpp` — 3× `PairArEnvelope`, 6 knobs, mod source/depth arrays, `randomizeKnobs()` / `randomizeMod()`
- [ ] 1.2 Add `ParamDisplayNames::forVcvVcoAr(uint8_t index)` — Att./Rel. × VCO1–3 labels
- [ ] 1.3 Add `VcvPanelLayout` constants for VCO AR module (`kVcoArHp`, column grid)
- [ ] 1.4 Extend main host IO: `SetVcoArLinked`, knob getters/setters; wire into `FroggersEngine::MixOscVoices` with VCV-gated per-VCO path

## 2. DSP + tests

- [ ] 2.1 Implement per-VCO `Step` loop (3 envelopes, one formula) — bypass when `!vcoArLinked`
- [ ] 2.2 When linked on VCV, disable pair-sum `m_pair12`/`m_pair23` for that block
- [ ] 2.3 Add `VcoArState_test.cpp` or extend `PairArEnvelope_test.cpp`: min/max time + rise/fall ordering per VCO index
- [ ] 2.4 Confirm desktop `PairArEnvelope_test` and sim pair-AR path still pass unchanged

## 3. VCO Crispy + randomize

- [ ] 3.1 Wire left-expander Crispy to fuego mask Audio rows 0–2 only (independent of page row-7 Crispy)
- [ ] 3.2 Wire Randomize → `VcoArState::randomizeKnobs()`, Randmod → `randomizeMod()` via shared rising-edge table
- [ ] 3.3 Manual: VCO Crispy at max scrambles VCO1–3 freqs; PM3 row unaffected

## 4. VCV module + panel

- [ ] 4.1 Register `FroggersTigaVcoArExpander` in `plugin.cpp` / `plugin.json`
- [ ] 4.2 Implement left-expander link: expander `process()` writes into main `VcoArState`
- [ ] 4.3 Generate VCO AR SVG via `generate_panels.py`; run `check_vcv_panel_svg.sh` + bounds CI
- [ ] 4.4 Update `vcv/README.md` and `vcv/DEVELOPMENT.md` patch diagram (left → main → right → FX)

## 5. Verification

- [ ] 5.1 Rack 100% zoom: all labels readable; Randomize/Randmod/Crispy silkscreened
- [ ] 5.2 Patch test: max Att. VCO1 → multi-second swell on VCO1 only (not pair-sum behavior)
- [ ] 5.3 Desktop/web smoke: pair-AR band unchanged (Att. 1+2 still pair-sum)
