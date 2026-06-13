## Context

### User-reported failure (post `b4f15dd`)

```
Expected:                      Actual (reported):

┌────── VCO1 ──────┐          ┌──────┐  label
│     VCO1         │          │ empty│  ∿∿  ← waveform behind
│      ∿∿∿         │          │ box  │  (knob)
│     (knob)       │          └──────┘
│   Mod source     │          border left, content shifted right
└──────────────────┘
```

1. **Right-shift:** Labels, knobs, waveforms sit to the right of the visible bordered cell background.
2. **Waveforms behind:** VCO morph SVGs obscured by knob or neighboring column content (stacking / overlap).

### What `b4f15dd` changed

| Area | Before (`e047049`) | After (`b4f15dd`) |
|------|-------------------|-------------------|
| `.knob-col` | `flex column; align-items: center` | `grid; justify-items: center` |
| Morph DOM | Inside `.knob-row` beside knob | After label, before hint |
| Morph z-index | `z-index: 1` | **removed** |
| Cols 3–7 | no morph row | `.knob-morph-slot` placeholder |
| `.knob-row` | `grid 1fr auto` | `flex center`, knob only |

## Root-cause hypotheses (ranked)

### H1 — Grid child overflow without containment (most likely for right-shift)

`.knob-col` uses `display: grid; justify-items: center` but direct children include:

- `.mod-select { max-width: 5.5rem }` (~88px) — **wider than ~84px mobile cell**
- `.knob-row { width: 100% }` — percentage width on a grid item centered in a shrinking track
- Grid items default `min-width: auto` — refuse to shrink below intrinsic content width

When child intrinsic width exceeds `.knob-col` track width, content **overflows visibly** (default `overflow: visible`). Border/background stay on the shrunken cell; painted content extends right (and left). User sees content **outside** the box to the right.

**Verify:** DevTools 375px → inspect `.knob-col` computed width vs `.mod-select` width; check overflow.

### H2 — `justify-items: center` + mixed width strategies

Flex (`align-items: center`) and grid (`justify-items: center`) behave differently when children mix `width: 100%`, fixed px, and intrinsic inline content (`<label>`, `<select>`). Grid centers each row item independently; a wide row-4 select can shift visual weight right relative to the border box.

**Verify:** Toggle `justify-items: stretch` + inner `display:flex; flex-direction:column; align-items:center` wrapper — see if shift disappears.

### H3 — Waveforms behind due to stacking order (z-index removal + DOM order)

`b4f15dd` removed `.vco-morph-btn { z-index: 1 }` (intended to stop cross-cell bleed). Side effects:

- `.rotary-knob { position: relative }` creates a stacking context; knob row comes **after** morph in DOM → knob paints **on top** if rows overlap even slightly (tight `gap: 0.25rem`, subpixel rounding).
- If morph still bleeds into next column (H1 overflow), **later** column knobs paint on top → waveform "behind" neighbor knob.

**Verify:** DevTools → overlap regions; test `isolation: isolate` on `.knob-col` or restore morph `z-index` **inside cell only** (not cross-cell).

### H4 — Morph row collapses when hidden (height mismatch)

`renderVcoMorphButtons` sets `btn.hidden = true` on non-Audio pages (`display: none`). Cols 0–2 lose morph row; cols 3–7 keep `.knob-morph-slot`. Audio page: morph visible but **hint row between morph and knob** — user asked morph **between label and knob**; implementation put hint in between (label → morph → **hint** → knob).

Does not explain right-shift directly; explains wrong vertical rhythm and possible overlap if hint empty but morph/knob spacing wrong.

### H5 — Process failure: no visual verification

Tasks 3.1–3.3 marked complete without DevTools screenshot or live mobile check. CSS layout regressions are invisible to `npm run build`.

## Goals / Non-Goals (for fix agent)

**Goals:**

- All column content (label, morph, hint, knob, mod label, select) visually **inside** `.knob-col` border on 375px and 960px
- VCO morph **visible and tappable**, not behind knob or neighbor
- One DOM template all viewports; no mobile-only JS layout branches
- OMNI: single column structure; fix width/overflow in CSS not per-column hacks

**Non-Goals:**

- Revert entire mobile 2-column grid or phone mic limiter work
- Desktop side-by-side morph (user wanted label–knob gap; fix vertical stack, don't revert to horizontal bleed)

## Recommended fix direction (for follow-up apply)

### D1 — Contain overflow on `.knob-col`

```css
.knob-col {
  overflow: hidden; /* or clip — test tap targets */
  justify-items: stretch; /* not center */
}
.knob-col > * {
  min-width: 0;
  max-width: 100%;
  justify-self: center;
}
.mod-select {
  width: 100%;
  max-width: 100%; /* override 5.5rem on narrow cells */
}
```

### D2 — Restore flex column OR single inner stack wrapper

Revert `.knob-col` to `display: flex; flex-direction: column; align-items: center` (known good for centering) **while keeping** vertical morph DOM order. Grid on outer cell was unnecessary complexity.

### D3 — Fix morph slot order

DOM: `label → morph|placeholder → hint → knob` is current. User wanted morph between label and knob — consider `label → morph → knob → hint` or `label → hint → morph → knob` after visual test. **Do not** put hint between morph and knob if user spec says label–knob gap.

### D4 — Stacking inside cell only

If overlap persists: `.knob-col { isolation: isolate; }` and ensure morph row and knob row do not share vertical space. Do **not** reintroduce global morph z-index that paints over neighbor columns.

### D5 — Mandatory visual gate before merge

- Screenshot 375px Audio page: all 8 cells, content inside borders
- Screenshot 960px Audio page: 4×2 grid alignment with mod bay
- Tap test: morph cycles SVG

## Risks / Trade-offs

| Risk | Mitigation |
|------|------------|
| `overflow: hidden` clips focus rings | Use `overflow: clip` or pad cell |
| Revert to flex loses grid uniformity | Inner wrapper for stack |
| Re-deploy needed | Pages CI from `main` |

## Migration Plan

1. Reviewer reads this handoff + inspects live site at 375px
2. Confirm H1/H3 with DevTools (30 min)
3. New change `web-knob-col-align-fix` implements D1+D2 (+ D3 if needed)
4. Visual gate D5 before push
5. Optional: revert `b4f15dd` CSS only as interim hotfix if fix takes >1 session

## Open Questions

- Does failure reproduce on desktop ≥721px or mobile-only? (User said "worse than before" — confirm both)
- Is `html { scrollbar-gutter: stable }` contributing to asymmetric right shift?
