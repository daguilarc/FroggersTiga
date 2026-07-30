## Context

**Current DOM order** (`web/index.html`):

```
controls-top     Play · Stop · External Audio · External MIDI · meter · status
mod-bay-toggle
mod-bay
page-chrome      page Randomize / Randomize mod
field-layout     knobs + page nav
page-pills
global-strip     Rand All · Rand Mods · Rand Resample · Rand waveforms  ← bottom
hint
```

Global randomize is separated from transport and I/O on every viewport. Desktop centers `.global-strip` below pills via `@media (min-width: 721px) { justify-content: center }`.

**Handlers:** `main.ts` binds `#rand-all`, `#rand-mod`, `#marbles-btn`, `#rand-morphs` — IDs must not change.

## Goals / Non-Goals

**Goals:**

- Global strip directly under External MIDI, above mod bay, on mobile and desktop
- One `.global-strip`, one button set — OMNI: no duplicate markup, no resize JS, no per-breakpoint placement divergence
- Playwright mobile + desktop regression on layout order

**Non-goals:**

- Reposition page-chrome Randomize buttons
- VCV / native desktop app layout

## Decisions

### 1. DOM: nest strip in `.transport-io` under `.external-controls`

```html
<div class="controls-top">
  <div class="transport-play">
    <button id="play-btn" class="transport play">Play</button>
    <button id="stop-btn" class="transport stop" disabled>Stop</button>
  </div>
  <div class="transport-io">
    <div class="external-controls">
      <button id="external-btn">External Audio: Off</button>
      <button id="external-midi-btn">External MIDI: Off</button>
    </div>
    <nav class="global-strip" aria-label="Global randomize">…</nav>
  </div>
  <div id="external-meter" class="external-meter">…</div>
  <span id="status" class="status">…</span>
</div>
```

**Why:** Document order matches visual order on all viewports. One structure, one layout rule.

**Rejected:** Mobile-only CSS grid + `display: contents` with desktop bottom placement — repetition of placement logic across breakpoints.

**Rejected:** Duplicate strip for mobile/desktop — OMNI repetition violation.

**Rejected:** `appendChild` on resize — imperative DOM mutation.

### 2. CSS: column flex on `.transport-io` (all viewports)

```css
.transport-play {
  display: flex;
  gap: 1rem;
  align-items: center;
}

.transport-io {
  display: flex;
  flex-direction: column;
  gap: 0.35rem;
}

.global-strip {
  margin-top: 0;
}
```

`.controls-top` keeps existing `display: flex; flex-wrap: wrap; align-items: center`.

Remove desktop-only `.global-strip { justify-content: center }` — strip aligns under the external I/O column.

### 3. Playwright: layout order on mobile and desktop

Assert `getBoundingClientRect()`:

- `.global-strip` top &lt; `#mod-bay` top
- `.global-strip` top &gt; `#external-midi-btn` bottom

Use `MOBILE_USE` and `DESKTOP_USE` from `e2e/helpers.ts`. No audio start required.

### 4. No TS changes

Button wiring unchanged.

## Risks / Trade-offs

- **[Risk] Narrow desktop row wraps transport-io below play** → Mitigation: `flex-wrap` on `.controls-top` already handles; strip stays under MIDI within column
- **[Risk] Touch target overlap on narrow phones** → Mitigation: keep `min-height: 44px` on strip buttons (existing rule)

## Migration Plan

1. HTML restructure + CSS
2. Playwright mobile + desktop layout tests
3. Deploy via Pages on merge

Rollback: revert single commit.

## Open Questions

None.
