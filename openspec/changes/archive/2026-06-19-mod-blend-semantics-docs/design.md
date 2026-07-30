## Context

**Page-row pipeline** (`Parameter::Get`):

```
base ──► ModMgr::Modulate (crossfade) ──► Fuegoize via Crispy->Get(modMgr) ──► DSP
```

**Delay sidecar today** (broken):

```
rows 0–6: inline crossfade (no M1–M4 gate) ──► Fuegoize(..., raw knobs[7], ...)
row 7 (Crispy): mod assigned in UI ──► ignored
```

**Root cause:** `DelayState` was extracted with duplicated inline blend + raw Crispy pointer. Never ported Crispy mod or `ModMgr::Modulate`.

## Goals / Non-Goals

**Goals:**
- Delay Crispy mod works audibly and in UI display on **desktop and web**
- Fuego on Delay rows 1–7 uses **effective** (modulated) Crispy — match `Parameter::Get`
- Single blend function: `ModMgr::Modulate` everywhere in `DelayState`
- Web `postScreen` reads `getEffectiveKnob` when mod assigned (WASM binding)
- Document mod→fuego order and crossfade semantics for operators
- Regression test

**Non-goals:**
- `AudioPairArState` / pair-AR `ModMgr` refactor (still inline crossfade)
- VCV patch-depth seeding from knob position
- Hardware Field OLED workflow changes

## Decisions

### 1. Fix DelayState in-place — no Parameter wrapper

**Choice:** Add private `blendKnob(row, base)` calling `m_modMgr->Modulate(base, modSource[row], modDepth[row])`; add `effectiveCrispy()` = `blendKnob(kFuegRow, knobs[kFuegRow])`; replace all `knobs[kFuegRow]` in `Fuegoize` calls with `effectiveCrispy()`.

**Why:** Minimal diff; DelayState keeps its sidecar role. OMNI: one blend path via `ModMgr::Modulate`.

**Sketch:**

```cpp
void beginBlock(const ModMgr* modMgr) { m_modMgr = modMgr; }

float blendKnob(uint8_t row, float base) const {
    if (!m_modMgr || row >= kNumRows || modSource[row] == 255 || modDepth[row] <= 0.0f)
        return base;
    return m_modMgr->Modulate(base, modSource[row], modDepth[row]);
}

float effectiveCrispy() const {
    return blendKnob(kFuegRow, knobs[kFuegRow]);
}

float blendRow(uint8_t row, float knobSmoothed) const {
    return Fuegoize(blendKnob(row, knobSmoothed), effectiveCrispy(), row);
}

float getEffectiveKnob(uint8_t row) const {
    if (row == kFuegRow)
        return blendKnob(kFuegRow, knobs[kFuegRow]);
    if (modSource[row] == 255)
        return Fuegoize(knobs[row], effectiveCrispy(), row);
    return blendRow(row, knobs[row]);
}
```

### 2. `beginBlock(const ModMgr*)` not `float*`

**Choice:** Replace `beginBlock(const float* mods)` with `beginBlock(const ModMgr* modMgr)`.

**Why:** `Modulate` needs `m_mods[]` and `m_externalCvActive[]`. Three call sites: `AudioEngine.cpp`, `WasmSimHost.hpp`, `vcv/src/plugin.cpp` — all have `m_pageManager.m_modMgr` in scope.

### 3. Extend mod lifecycle to row 7

**Choice:** `randomizeMod` and `clearModRoutesForIndex` loop `0 .. kNumRows-1` (include Crispy).

**Why:** Page `RandomizePageMod` includes row 8 (FUEG/Crispy). Delay randmod should match.

### 4. Docs cover mod→fuego pipeline

**Choice:** Manual subsection order: crossfade → then Crispy scrambles low bits → Crispy itself can be modded (affects scramble intensity) → pair-AR excluded from fuego.

**Why:** Operators hit confusion at both layers; one pipeline diagram prevents false "CV attenuator" mental model.

### 5. Web Delay knob display — mirror pair-AR pattern

**Choice:** Add `froggers_delay_get_effective_knob(host, row)` → `host->delay.getEffectiveKnob(row)`. In `postScreen`, when `onDelayPage`, set `row.value` to effective when `modSource !== 255`, else base knob — same branch as pair-AR rows in the same function.

**Why:** `DelayState` fix restores DSP and desktop `SubModulePanel` display, but web `postScreen` currently hardcodes `froggers_delay_get_knob` for all Delay rows. Without this binding, browser rotaries stay frozen at base while mod is active.

**Sketch (TypeScript):**

```typescript
const modSource = this.wasm.froggers_delay_get_row_mod_source(this.host, row);
rows.push({
  value: modSource === 255
    ? this.wasm.froggers_delay_get_knob(this.host, row)
    : this.wasm.froggers_delay_get_effective_knob(this.host, row),
  modSource,
  modDepth: this.wasm.froggers_delay_get_row_mod_depth(this.host, row),
  // ...
});
```

Existing `main.ts` drag handlers already send `delayModDepth` vs `delayKnob`; no change needed there.

## Risks / Trade-offs

- **[Risk] `effectiveCrispy()` read before `beginBlock`** → returns unmodulated base (same class of bug as pair-AR pre-fix). Mitigation: `getEffectiveKnob` uses `m_modMgr` when set; WASM `ProcessBlock` calls `delay.beginBlock(&modMgr)` each block; UI timer runs during audio; test covers sweep.
- **[Risk] Web display stale before first audio block** → `getEffectiveKnob` needs live `m_modMgr`; `WasmSimHost::process` already calls `beginBlock` before engine tick — same mitigation as pair-AR effective read path.
- **[Risk] Behavior change for presets with dead Delay-Crispy mod routes** → routes become live. Intended fix, not regression.
- **[Risk] M1–M4 gating newly affects Delay rows** → correct parity with page rows; document in manual.

## Open Questions

None — explore resolved scope.
