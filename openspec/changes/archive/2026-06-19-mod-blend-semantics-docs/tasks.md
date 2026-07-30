> **Reconciled (omni 1.2):** Code-backed; task 5.5 manual web smoke remains open.

## 1. DelayState fix — OMNI blend + Crispy mod

- [x] 1.1 Change `DelayState::beginBlock` to accept `const ModMgr*`; store `m_modMgr`; update `AudioEngine.cpp`, `WasmSimHost.hpp`, `vcv/src/plugin.cpp`
- [x] 1.2 Add private `blendKnob(row, base)` delegating to `m_modMgr->Modulate`; add `effectiveCrispy()` for row 7 blend without self-fuego
- [x] 1.3 Refactor `blendRow`, `getEffectiveKnob`, `processInsert` to use `blendKnob` + `effectiveCrispy()` in all `Fuegoize` calls — remove inline crossfade math
- [x] 1.4 Extend `randomizeMod`, `clearModRoutesForIndex`, and `sanitizeModSources` to include row 7 (Crispy)

## 2. Web WASM display parity

- [x] 2.1 Add `froggers_delay_get_effective_knob` in `wasm/bindings.cpp` → `host->delay.getEffectiveKnob(row)`
- [x] 2.2 Export symbol in `wasm/CMakeLists.txt` and `wasm/build.sh`
- [x] 2.3 Declare in `web/src/froggers-processor.ts` WasmBindings; update `postScreen` Delay branch to use effective when `modSource !== 255` (mirror pair-AR pattern)

## 3. Regression test

- [x] 3.1 Add `sim/DelayCrispyMod_test.cpp`: Crispy mod depth 1.0, sweep mod bus, assert `getEffectiveKnob(7)` follows blend
- [x] 3.2 Same test: modulated Crispy changes `getEffectiveKnob(0)` vs static Crispy at same base
- [x] 3.3 Register test in `sim/CMakeLists.txt`

## 4. Operator documentation

- [x] 4.1 Add **Mod depth & blend** + **mod then fuego** subsections to `SIM_MANUAL.md` (Mod bay + Delay/Crispy gloss)
- [x] 4.2 Copy to `docs/sim-manual.md` and `web/public/sim-manual.md`
- [x] 4.3 Add QUICK_DICT entries (mod depth, Crispy mod) to all three QUICK_DICT copies
- [x] 4.4 Add cross-ref under `MANUAL.md` § Modulation and § Fuegoizer (mod-first pipeline)
- [x] 4.5 Add v1.0.3 changelog bullet (Delay Crispy mod fix + mod blend docs) to all three sim-manual copies

## 5. Verification

- [x] 5.1 Run `DelayCrispyMod_test` and existing sim tests
- [x] 5.2 Rebuild WASM (`wasm/build.sh`); confirm new export resolves in processor
- [x] 5.3 Grep manual copies for "Mod depth & blend" — confirm parity
- [x] 5.4 Manual smoke desktop: patch VCO Envelope → Delay Crispy, knob moves, scramble breathes
- [ ] 5.5 Manual smoke web: Delay page, assign VCO Envelope to Crispy, Play on — rotary tracks scope while idle
