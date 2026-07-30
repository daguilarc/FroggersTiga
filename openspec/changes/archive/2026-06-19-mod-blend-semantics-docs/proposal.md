## Why

Two problems surfaced from operator confusion about modulation:

1. **Undocumented semantics:** Desktop/web knobs **display the effective modulated value** while idle but edit **mod depth** while dragging. The blend is `base × (1 − depth) + mod × depth` (crossfade, not VCV attenuator). Fuego (Crispy) applies **after** mod crossfade on rows 1–7. Manuals never explain this pipeline.

2. **Delay Crispy bug:** `DelayState` lets operators patch mod to Delay Crispy (row 8) in UI, but the engine **ignores** it — `getEffectiveKnob(7)` returns raw base, `processInsert` never blends row 7, and fuego on rows 1–7 uses **raw** `knobs[7]` instead of modulated Crispy. Page rows use `Parameter::Get(modMgr)` for both; Delay sidecar does not. Patched mod cables to Delay Crispy are dead routes — an OMNI parity violation.

## What Changes

- **Fix `DelayState`:** route row 7 through the same mod crossfade as rows 0–6; feed **effective Crispy** (base + mod blend, no self-fuego) into `Fuegoize()` for rows 0–6; use `ModMgr::Modulate` (single blend path + M1–M4 presence gating) instead of inline crossfade math
- **`beginBlock` takes `const ModMgr*`** on `DelayState` (call sites pass `&m_pageManager.m_modMgr`) — enables `Modulate` gating parity with page rows
- **Include row 7 in** `randomizeMod` and `clearModRoutesForIndex` — match page-row mod lifecycle
- **Unit test:** Delay Crispy mod affects `getEffectiveKnob(7)` and fuego intensity on row 0 when mod bus sweeps
- **Web WASM display parity:** add `froggers_delay_get_effective_knob` binding; `froggers-processor.ts` `postScreen` uses effective value when Delay row has mod assigned (today uses raw `get_knob` only — same gap as pair-AR pre-fix)
- **New baseline spec** `mod-blend-semantics`: blend formula, mod→fuego order, base/depth/effective, M1–M4 gating, Delay Crispy parity, UI display-vs-drag on desktop **and web**
- **Sim operator docs:** Mod depth & blend + mod-then-fuego pipeline in Mod bay / Global controls sections

**Non-goals:** changing blend math to VCV attenuverter; VCV `applyPageModJack` depth-at-patch seeding parity; refactoring `AudioPairArState` to `ModMgr::Modulate` (follow-up)

## Capabilities

### New Capabilities

- `mod-blend-semantics`: Engine blend formula, mod→fuego pipeline, M1–M4 gating, Delay Crispy mod parity, sim UI semantics

### Modified Capabilities

- `sim-operator-doc-parity`: Manuals document mod blend + fuego interaction

## Impact

- `sim/DelayState.hpp` — fix Crispy mod + fuego feed; `beginBlock(const ModMgr*)`
- `desktop/Source/AudioEngine.cpp`, `sim/WasmSimHost.hpp`, `vcv/src/plugin.cpp` — updated `beginBlock` call
- `sim/DelayCrispyMod_test.cpp` (or adjacent) — regression test
- `wasm/bindings.cpp`, `wasm/CMakeLists.txt`, `wasm/build.sh` — export `froggers_delay_get_effective_knob`
- `web/src/froggers-processor.ts` — Delay page screen rows use effective knob when mod assigned
- `SIM_MANUAL.md`, `docs/sim-manual.md`, `web/public/sim-manual.md`, QUICK_DICT copies, `MANUAL.md` § Modulation cross-ref
- `openspec/specs/mod-blend-semantics/spec.md` (new via archive)
