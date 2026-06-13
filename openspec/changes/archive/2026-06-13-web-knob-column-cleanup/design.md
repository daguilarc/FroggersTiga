## Context

Current layout (user screenshot, Audio page while playing):

```
┌─────────────────────────────────────────────────────────┐
│  #mod-route-summary  "VCO1 ← Marbles 1 · 10%" …       │  ← pops in on Play + patch
├─────────────────────────────────────────────────────────┤
│  [VCO1 knob] [VCO2] … [Crunch]   ◀  field  ▶            │  ← primary controls
├─────────────────────────────────────────────────────────┤
│  #oled  eight rows: name | wave? | bar | badge          │  ← duplicate of knobs
└─────────────────────────────────────────────────────────┘
```

Reference ([thenoriegas.info](https://thenoriegas.info)):

```
┌─────────────────────────────────────────────────────────┐
│  [VCO1 knob + wave] [VCO2 + wave] …                     │  ← one surface
│  mod selects under each knob                            │
└─────────────────────────────────────────────────────────┘
```

**Existing code:**

| Piece | File | Notes |
|-------|------|-------|
| Route summary | `main.ts` `renderModRouteSummary` | Shows when `modSource !== 255` |
| OLED panel | `main.ts` `renderOled` | Gated on `audioRunning`; wave buttons for idx 0–2 on wasm page 0 |
| Live knob values | `syncKnobUi` | Already uses `row.value` — correct primary surface |
| Morph data | `screen.morphs[0..2]` | From worklet `postScreen` |

**Supersedes:** `web-sim-layout-ux` specs `web-mod-route-summary` (hide-when-empty) and `web-oled-collapse` (Play-gated OLED). User direction: remove panels, not tune them.

## Goals / Non-Goals

**Goals:**

- Zero layout shift when clicking Play — no new panels appear above or below knobs
- VCO morph visible and clickable on Audio page without Play gate (morph state from WASM; show last `screen` morph or neutral placeholder before first post)
- Reuse `waveSvg`, `evalWaveMorph`, `cycleVcoMorph` — no new WASM API
- OMNI: one update path — `onScreenUpdate` calls `syncKnobUi` + `renderVcoMorphButtons`; no parallel OLED render

**Non-Goals:**

- Moving mod bay scopes into knob columns (scopes stay in `#mod-bay`)
- Desktop sim layout
- `web-ext-in-meter` (follow-on apply)
- Group meta-panels (`web-field-module-groups` — already done in layout-ux)

## Decisions

### D1: Delete route summary, do not collapse it

**Choice:** Remove `#mod-route-summary` from DOM and delete `renderModRouteSummary` + click handler.

**Why:** User: "totally unnecessary." Mod source `<select>` + knob wiggle already convey routing. Hiding-when-empty (layout-ux) still pops the strip on Play when patched — wrong default.

**Alternative rejected:** Keep summary collapsed until hover — still duplicate UX.

### D2: Delete OLED panel; migrate morph buttons only

**Choice:** Remove `#oled`. Add per-column morph slot in knob factory for indices 0–2 only when `hostPage === 0` (Audio host page).

**Layout:** Each VCO column gets a horizontal `.knob-row`: rotary knob left, `.vco-morph-btn` right (same `.wave-btn` styling / blue stroke).

**Why:** User wants waveforms "on the right hand side of the knobs, same color as they are now." OLED value bars and name repetition add no information knobs lack.

**Alternative rejected:** Keep OLED collapsed to morph-only strip — still a second panel; user asked for inline.

### D3: Morph buttons not gated on `audioRunning`

**Choice:** Module-level `lastMorphs: number[] = [0, 0, 0]` assigned in `onScreenUpdate` from `data.morphs`. `renderVcoMorphButtons` reads `lastMorphs` and shows controls when `hostPage === 0 && wasmPage === 0`. Before first screen post, buttons show sine morph (0).

**Why:** noriegas shows morph state whenever engine has reported it; gating on Play caused the "why does stuff pop up when I play" complaint.

### D4: Single render pass; listeners wired once

**Choice:**

```
onScreenUpdate(data):
  renderModBay(...)
  syncKnobUi(rows)
  lastMorphs = data.morphs
  renderVcoMorphButtons(data.wasmPage)   // SVG innerHTML only; no listener rebind

setHostPage / changeHostPage:
  renderVcoMorphButtons(lastWasmPage)    // hide morph slots immediately on page leave
```

Knob factory (cols 0–2): `.knob-row` with knob + `.vco-morph-btn`; `click` listener wired once → `cycleVcoMorph`.

**Why:** OMNI accumulate-then-apply; one handler owns knob-column visuals. Rebinding listeners every screen tick (old `renderOled` pattern) is forbidden.

### D5: Page navigation hides morph slots

**Choice:** Non-Audio pages: morph button containers `hidden` or empty. No morph UI on Delay/Reverb/etc.

## Risks / Trade-offs

| Risk | Mitigation |
|------|------------|
| Users relied on route summary click-to-highlight | Remove highlight feature or move to mod-select focus only; acceptable loss per user |
| Mobile narrow columns with knob + wave | Flex row wraps; wave btn 28×28 matches current SVG |
| Stale morph before Play | First `postScreen` from page change or randomize updates; `setHostPage` already posts |
| layout-ux tasks conflict | Archive note: this change supersedes mod-route-summary + oled-collapse requirements |

## Migration Plan

1. Add morph button DOM in knob factory (columns 0–2)
2. Implement `renderVcoMorphButtons`; wire screen + Rand waves
3. Remove OLED + route summary HTML, JS, CSS
4. Update manual one line
5. Manual: Play — no panels pop in; Audio page shows morph beside VCO knobs; Rand waves updates SVG

## Open Questions

None — screenshot + noriegas reference define target layout.
