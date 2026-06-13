## Context

Web sim architecture (unchanged):

```text
main.ts (UI thread)
  ↔ postMessage ↔
froggers-processor.ts (AudioWorklet = audio thread)
  ↔ WASM ↔ PagedHostIO + DelayState
```

**Mobile constraint:** One host page visible at a time. Eight knob columns + OLED + mod `<select>` per column. Prev/next flanking knobs (`stereo-delay-page` §F.4). This works; the problem is discoverability and parity with desktop **semantics** without patch cables.

Current gaps in `web/src/main.ts`:

| Gap | Today |
|-----|--------|
| Per-page randomize | Global strip only |
| Mod visibility | Dropdown per knob; no summary |
| Wave UX | `SIN`/`SAW`/`SQR` text buttons |
| Delay help | Names only (DTIM…) |
| Page nav | ◀ ▶ only |
| Mod bay | Always visible 3 meters |

Desktop has six simultaneous panels; web cannot. **Design principle:** compress desktop **per-panel affordances** into **page chrome** without merging pages.

## Goals / Non-Goals

**Goals:**

- User on phone understands which page they are on, what it does, and which mods are active **on this page**.
- Per-page **Randomize** / **Randomize mod** match desktop panel buttons.
- Delay page reads as a first-class FX page, not "page 6 with cryptic labels."
- Touch targets ≥44 px; no horizontal knob scroll at ≤720 px.
- Sim-valid mod indices only in UI and after randomize (inherits core fix).

**Non-Goals:**

- Patch cables or drag-drop mod routing on web — **mod assignment SHALL remain `<select>` per knob**; route summary MUST NOT write mod indices or accept drag input.
- MIDI in browser.
- Merging six pages into one scroll view.
- Reskin to match desktop rack layout.

## Decisions

### 1. Layout — page chrome + bottom nav

```text
┌──────────────────────────────────────┐
│ Play Stop External    status         │
├──────────────────────────────────────┤
│ [mod bay — collapsible on mobile]    │
├──────────────────────────────────────┤
│ ┌─ Page chrome ───────────────────┐  │
│ │ Delay (6/6)                     │  │
│ │ Stereo delay after reverb bump  │  │
│ │ [Randomize] [Randomize mod]     │  │
│ └─────────────────────────────────┘  │
│ Active routes: DTIM←Marbles1 50% …    │
├──────────────────────────────────────┤
│  ◀   [8 knob columns + mod selects] ▶│
│      [OLED rows + wave SVGs]         │
├──────────────────────────────────────┤
│ Audio Marbles Reverb Filter Drive Delay │  ← pill nav
├──────────────────────────────────────┤
│ Global: Rand all | Rand mod all | …  │
└──────────────────────────────────────┘
```

**Page chrome** replaces generic `Page: Delay (6/6)` span with structured block. **Bottom pill nav** duplicates page index for thumbs; arrows remain for Field parity.

**Alternative rejected:** Hamburger menu for pages — hides discoverability.

### 2. Mod route summary (cable substitute)

Render from `screen` message rows on each update:

```text
For each row where modSource !== 255:
  "{name} ← {sourceLabel} · {depth%}"
```

Tap row scrolls/highlights that knob column (optional v1: highlight only). No editing in summary — dropdown remains source of truth.

Source labels match desktop: `None | VCO level | Marbles 1 | Marbles 2`.

### 3. Mod column labeling

When `modSource === 255`: slider sends knob value; label = param name.

When mod active: slider sends mod depth; label under slider = **Mod depth**; OLED badge shows abbreviated source (`M1`, `VCO`, etc.).

Select label always **Mod source** above `<select>`.

### 4. Delay param hints

Static map in `main.ts` (or small JSON):

| Row | Hint (one line, below name on Delay page only) |
|-----|------------------------------------------------|
| DTIM | Delay time (exp, ~0–3 s) |
| DSND | Send to delay |
| DFBK | Feedback |
| DWID | Stereo width |
| DTON | Tone / filter |
| DMOD | Modulation rate |
| DMIX | Wet mix |
| FUEG | Fuegoizer (scramble) |

Shown in page chrome blurb area or under knob label when `hostPage === 5`.

### 5. Wave SVG buttons

Replace `waveLabel()` text with `<svg class="wave-icon">` paths for three bands (`morph < 0.33`, `< 0.66`, else). Size 28×28 CSS px in OLED row. Tooltip unchanged.

Match desktop `WaveMorphButton` intent without JUCE.

### 6. Per-page randomize wiring

| Button | hostPage 0–4 | hostPage 5 |
|--------|--------------|------------|
| Page Randomize | `randomizePage(page)` | `delayRandomizeKnobs` |
| Page Randomize mod | `randomizePageMod(page)` | `delayRandomizeMod` |

Global strip unchanged for all-pages variants.

Add WASM exports if `froggers_randomize_page` / `froggers_randomize_page_mod` missing.

### 7. Mobile mod bay collapse

CSS `@media (max-width: 720px)`: mod bay header "Mod sources ▾" toggles visibility. Default collapsed after first visit optional — **default expanded** on first Play to teach mod concept, remember preference in `sessionStorage`.

### 8. Swipe paging

`pointerdown` / `pointerup` on `.field-layout` with horizontal delta > 60 px triggers `hostPageDelta ±1`. Ignore when `knobDragging[i]`. Passive listeners where possible.

## Risks / Trade-offs

| Risk | Mitigation |
|------|------------|
| Vertical space on small phones | Collapsible mod bay; compact page chrome |
| Route summary stale | Driven from same `screen` payload as knobs |
| Per-page randomize export gap | Add bindings in same PR as UI |
| Duplicated nav (arrows + pills + swipe) | Consistent `setHostPage` single path |

## Migration Plan

1. Land WASM per-page randomize exports + sim randomize (from `desktop-host-mutation-safety`).
2. HTML/CSS shell (chrome, pills, collapsible bay).
3. `main.ts` logic (summary, hints, SVG waves, swipe).
4. Manual §F + new web § mod summary checks.

## Open Questions

- Persist mod bay collapsed state in `localStorage` vs `sessionStorage` — use `sessionStorage` v1.
