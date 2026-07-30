## Context

Mod indices 5 (Random 1) and 6 (Random 2) use `ModCellPresentation::Led` in `HostPanelLayout::kModRackCatalog` on desktop, web, VST/AU, and VCV. Today all four hosts gate display at a hard **0.55** CV threshold:

- Desktop/VST: `ModModuleBox.cpp` — binary green fill
- Web: `ModLedIndicator.ts` — `data-on="true|false"`
- VCV: `plugin.cpp` — `setBrightness(cv > 0.55 ? 1 : 0)` on `LIGHT_MOD_RANDOM1/2`

Froggers holds smoothed bag S&H values on `m_mods[5]` and `m_mods[6]`; proportional LEDs communicate those held CV magnitudes better than the current threshold without changing DSP. This change defines Froggers' display behavior directly rather than depending on an unverified hardware-emulation claim.

Omni rule: one curve authority in `sim/`, host UIs consume it — no per-host threshold literals.

## Goals / Non-Goals

**Goals:**

- One shared brightness function for Random LEDs on **desktop, web, VST/AU, and VCV**.
- Weaker CV → visibly dimmer green; CV ≥ 0.55 → full brightness when transport/audio is active.
- Sim unit tests for golden curve points; Playwright e2e on web mod bay.
- Extend the existing generated host-display projection for web parity (`--check` in existing verification).

**Non-Goals:**

- VCO Envelope LED on VCV (index 4) — remains binary threshold until a separate change; desktop/web use scopes for index 4.
- Seven per-output Marbles jacks, mode/routing LEDs, or hardware panel parity.
- DSP, mod routing, `B5` / Rand Resample semantics, or preset format changes.
- Animated LED slew (instant brightness per UI frame is acceptable).

## Decisions

### D1: Shared curve in `sim/ModLedBrightness.hpp`

**Choice:** Add `ModLedDisplayBrightness(float cv01, bool active)` with one named reference constant:

```cpp
constexpr float kModLedFullBrightnessCv = 0.55f;
```

For `active == false`, return `0`. Clamp `cv01` to `[0, 1]`; values at or above `kModLedFullBrightnessCv` return `1`. Below that point, normalize by the full-brightness CV and return `normalized * normalized`.

**Rationale:** The fixed quadratic curve deliberately suppresses the low end more than a linear mapping, making weak and strong held values easier to distinguish while preserving the existing 0.55 reference as “full green.” Multiplication is also safe and cheap in VCV's audio-rate `process()`.

**Alternative rejected:** Keep linear `cv / 0.55` — works but looks nearly binary for mid-high held values; weaker differentiation at low CV.

**Alternative rejected:** Per-host curves — violates omni generated-display authority pattern.

### D2: Web parity via the existing host-display generator

**Choice:** Extend `scripts/generate-host-display.mjs` to parse `kModLedFullBrightnessCv` from `sim/ModLedBrightness.hpp` and emit `modLedDisplayBrightness(cv, active)` into the existing `web/src/hostDisplay.generated.ts`. The existing `--check` and `verify:host-display` paths remain authoritative.

**Rationale:** This keeps one generated display projection and one existing freshness check instead of creating a generator, generated file, npm script, and CI path for a two-line curve.

### D3: Web DOM contract

**Choice:** Replace `data-on` with `data-brightness="0.000"` (3 decimal string) on `.mod-led`, and set a numeric inline custom property such as `--mod-led-brightness` to the same value. Initialize both to zero in the constructor. Render a persistent dark LED base and an accent-colored `::before` overlay whose opacity is `var(--mod-led-brightness)`. Remove the `[data-on="true"]` rule.

**Rationale:** Playwright can inspect numeric brightness, while CSS receives an actual numeric value. CSS cannot use a `data-*` attribute as numeric opacity on its own. Keeping the dark base avoids making a zero-level LED disappear into the panel.

### D4: Desktop/VST paint

**Choice:** `ModModuleBox::paint` always draws the existing dark LED base, then overlays `juce::Colour(0xff3fb950).withAlpha(brightness)` using `ModLedDisplayBrightness(m_lastLevel, m_audioRunning)`.

**Rationale:** Reuses the existing ellipse and preserves a visible LED housing at zero while the green overlay alpha maps directly from the shared function.

### D5: VCV lights

**Choice:** `lights[LIGHT_MOD_RANDOM1/2].setBrightness(ModLedDisplayBrightness(host.GetCvOut(5/6), true))` each `process()` — VCV has no separate “audio running” UI gate; engine always processes when clocked.

**Rationale:** Rack modules show live CV at output jacks; brightness tracks held mod value whenever the block runs.

### D6: Playwright coverage

**Choice:** New `web/e2e/marbles-mod-led-level.spec.ts`:

1. **Idle:** before Play, Random LED `data-brightness` is `0`.
2. **Live:** after `startSimAudio` + Rand Resample, assert the Random LEDs expose numeric brightness, at least one is lit, the old `data-on` contract is absent, and the CSS custom property equals `data-brightness`.
3. **Curve import:** test file imports `modLedDisplayBrightness` from generated TS and asserts clamp, inactive, and golden points (negative → 0, 0 → 0, 0.275 → 0.25, 0.55 → 1, above-range → 1, inactive → 0).

**Rationale:** The DOM test covers browser wiring without relying on repeated random draws; the deterministic C++ and generated-TS golden tests cover the actual curve.

## Risks / Trade-offs

- **[Risk] Low CV can be nearly invisible on bright displays** → Preserve the dark LED housing and manually smoke-test representative low/mid values; changing the curve is a later design decision if needed.
- **[Risk] TS/C++ drift** → The existing host-display generator `--check` remains in `npm run test:e2e` and repository verification.
- **[Risk] The VCV widget is green-only** → `setBrightness` accepts continuous brightness; no polarity/red channel is required for the unipolar Random CV.
- **[Trade-off] No smooth LED attack** → Acceptable; matches scope refresh cadence.

## Migration Plan

No state migration. Ship across all sim artifacts. Docs/tooltips and the current `SIM_MANUAL.md` version-history entry update in the same PR. This desktop-only presentation fix does not change the CMake/package version and does not create a new release tag or GitHub Release; publishing continues through the single `froggerstiga-v1` channel.

## Open Questions

None.
