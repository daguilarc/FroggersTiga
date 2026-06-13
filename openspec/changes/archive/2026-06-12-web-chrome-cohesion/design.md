## Context

**Web layout (unchanged paging model):**

```text
app-header → transport → mod-bay (collapsible ≤720px) → route summary → page chrome → knobs → pills → global strip
```

**Drift vs desktop + Quick Dict:**

| Surface | Web now | Target |
|---------|---------|--------|
| Global strip | Randomize all / Randomize mod (all) / Randomize waves | Rand All / Rand Mods / Rand waves |
| Mod bay + dropdown | VCO Envelope | VCO level |
| Mod meters | width = live level only; 0 when stopped | hold last level dimmed; step flash on change |
| Touch | page chrome + pills = 44px; strip + transport not | all primary actions ≥44px |

`web-sim-page-ux` proposal kept full global-strip words; `desktop-chrome-cohesion` + Quick Dict supersede to abbreviated strip labels. Page chrome keeps **Randomize** / **Randomize mod** (per-page scope).

## Goals / Non-Goals

**Goals:**

- One label map in `main.ts` (`MOD_SOURCE_LABELS`, `INTERNAL_MOD_LABELS`) drives bay, dropdown, summary, badges.
- Mod meter renderer reuses accumulated state — no per-frame throwaway DOM rebuild without reading hold cache.
- CSS-only touch target bump; no desktop layout port.

**Non-Goals:**

- Desktop `DesktopChromeLayout.hpp`, two-row header, RECORD cluster.
- 96px CV trace scopes or centered mod rack.
- Patch cables, browser MIDI, waveform export.

## Decisions

### 1. Central label constants

```ts
const INTERNAL_MOD_LABELS = ["VCO level", "Marbles 1", "Marbles 2"];
const MOD_SOURCE_LABELS: Record<number, string> = {
  255: "None",
  4: "VCO level",
  5: "Marbles 1",
  6: "Marbles 2",
};
```

Update `index.html` global strip button text to match. Page chrome buttons unchanged (**Randomize**, **Randomize mod**).

### 2. Mod meter hold + step (OMNI: accumulate then apply)

Module-level state in `main.ts`:

```ts
const modMeterDisplay = new Float32Array(7); // indices 4–6 used
let modMetersIdle = false;
```

`renderModBay(levels, audioRunning)`:

- While `audioRunning`: display `levels[i]`; cache into `modMeterDisplay`; detect Marbles step (`|Δ| > 0.02` on 5/6) → add `mod-cell-step` class for one animation frame (200ms).
- When stopped: `modMetersIdle = true`; display `modMeterDisplay[i]` at reduced opacity (CSS `.mod-meter-fill--idle`).

Do not zero meters on stop.

### 3. Touch targets

```css
.controls-top .transport,
.controls-top #external-btn,
.global-strip button {
  min-height: 44px;
}
```

Preserve `flex-wrap` on global strip for narrow viewports.

### 4. DOM reading order

Move `#mod-route-summary` in `index.html` to immediately **after** `#page-chrome` (before `#field-layout`). No JS change to summary logic.

### 5. Mod bay hint

Add `.mod-bay-hint` under toggle: “CV level while playing”.

## Risks / Trade-offs

| Risk | Mitigation |
|------|------------|
| Abbreviated strip labels confuse new users | Quick Dict + tooltips unchanged; help menu documents full names |
| Stale meter after long stop | Acceptable — shows last musical state like desktop idle scope |
| Step flash missed at 15Hz screen tick | Threshold 0.02; 200ms CSS transition visible on mobile |

## Migration Plan

1. Labels + HTML button text.
2. `renderModBay` state machine + CSS idle/step classes.
3. Touch CSS + DOM reorder.
4. Manual verify at 390px and 720px widths.

## Open Questions

- None blocking.
