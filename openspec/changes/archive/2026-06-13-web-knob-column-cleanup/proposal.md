## Why

The web sim duplicates control state in three places: a mod-route text strip above the knobs, an eight-row OLED panel below the knobs, and the knob columns themselves. The route strip only appears after Play when patches exist — it repeats what mod-source dropdowns already show. The OLED repeats parameter names, value bars, and mod badges that knobs already express through position and patching. [thenoriegas.info](https://thenoriegas.info) never needed these panels: knobs are the live surface, randomize moves knobs, and VCO waveform morph is a small control beside each VCO — not a separate mock display. This change removes the redundant panels and inlines the one OLED artifact worth keeping (VCO morph wave buttons) into the VCO1–VCO3 knob columns.

## What Changes

- **Remove `#mod-route-summary` entirely** — no text box above knobs on any page, stopped or playing. Mod routing stays visible only via each column's mod-source `<select>` and live knob motion.
- **Remove `#oled` eight-row panel** — no black box below knobs; no Play-gated pop-in of duplicate names/bars/badges.
- **Inline VCO morph controls** — on Audio page (WASM page 0), VCO1–VCO3 columns show the existing blue waveform button to the right of the rotary knob (same `waveSvg` / `cycleVcoMorph` behavior). Updates on every `screen` post including Rand waves / Marbles.
- **Knob columns remain primary** — live values from `syncKnobUi`, static labels from `applyStaticKnobLabels`; no secondary value readout.
- **Docs** — `SIM_MANUAL.md` / quick-dict: click waveform beside VCO knob, not OLED row.

## Capabilities

### New Capabilities

- `web-knob-primary-surface`: Knob columns are the sole parameter control surface; mod-route summary and OLED duplicate panels removed.
- `web-vco-morph-inline`: VCO1–VCO3 waveform morph buttons render inside knob columns on Audio page, driven by `screen.morphs`.

### Modified Capabilities

- (none at repo `openspec/specs/` — supersedes in-flight change specs `web-mod-route-summary` and `web-oled-collapse` from `web-sim-layout-ux`)

## Impact

- `web/index.html` — remove `#mod-route-summary` and `#oled` nodes
- `web/src/main.ts` — delete `renderModRouteSummary`, `renderOled`, `syncOledVisibility`; add `renderVcoMorphButtons(morphs, wasmPage)` on screen update; wire click → `cycleVcoMorph`
- `web/src/style.css` — remove `.mod-route-summary`, `.oled*` rules; add `.knob-row` + `.vco-morph-btn` layout (knob + morph slot)
- `web/public/sim-manual.md` — update VCO morph instruction
- **Prerequisite:** `web-sim-agent-handoff` applied (worklet `postScreen` alive) so morphs and knob values refresh
- **Does not change:** mod bay scopes, transport, `web-ext-in-meter` (separate apply)
