## Context

```
Web knob column (intended — all pages identical):
┌─────────────┐
│ Label       │
│ [hint slot] │  ← fixed height, empty on non-Delay pages
│   (knob)    │
│ Mod source  │
│  [select]   │
└─────────────┘

Delay page today (broken):
  page-chrome.delay-page     → orange border (only Delay)
  applyDelayKnobHints()      → hint display:none when mod patched
  Randomize mod              → random mod sources → mixed heights

Mod bay today:
  VCO level  → continuous scope  ✓ keep → rename VCO Envelope
  Marbles 1  → stepHold scope    ✗ replace with LED, label → Marbles 1 S&H
  Marbles 2  → stepHold scope    ✗ replace with LED, label → Marbles 2 S&H

Desktop mod rack already labels "VCO Envelope" (ModRackPanel.cpp).
Web still says "VCO level" in three places (main.ts).
```

## Root cause: Delay Randomize mod layout shift

1. User clicks **Randomize mod** on Delay page → `delayRandomizeMod` → worklet `postScreen`.
2. `syncKnobUi` sees new `modSource` values → `modSourceChanged = true`.
3. `applyDelayKnobHints()` runs: for each row with mod ≠ None, sets `knobHintLabels[i].style.display = "none"`.
4. Only row 0 has hint text (`~0–2 s`); patched rows collapse hint slot → column height shrinks.
5. Unpatched rows keep hint slot → **uneven grid** (screenshot).

Orange border is separate: `.page-chrome.delay-page { border-color: var(--delay-accent) }` — intentional delay theming from `stereo-delay-page`, not governed by the shared column spec.

## Data Flow

| Event | Input | Transform | Output |
|-------|-------|-----------|--------|
| Screen tick | `modLevels[5,6]` | one loop: scope push/draw for VCO Envelope; LED setLevel for Marbles | mod bay repaint |
| Morph click | user click | `CycleVcoMorph` thresholds (`v<0.25→0.5`, `v<0.75→1`, else `0`) + `cycleVcoMorph` | SVG update + WASM morph |
| Page change | `hostPage` | single `renderPageChrome` | no delay-only CSS classes |
| Delay hint | row 0 only | static text in reserved slot | no display toggle on mod |

## Goals / Non-Goals

**Goals:**

- Six host pages use one knob-column structure and chrome styling.
- Randomize mod on any page does not change column dimensions.
- VCO morph click and Rand waveforms work on Audio page while playing.
- Marbles 1/2 show green LED on/off on web and desktop, labeled **Marbles 1 S&H** / **Marbles 2 S&H**.
- **VCO Envelope** naming consistent across sim hosts.

**Non-Goals:**

- Removing Delay as a distinct host page (page 6/6 label stays).
- Changing Marbles DSP or step timing.
- Desktop SubModulePanel row grid (separate change, already landed).

## Decisions

### D1: Remove delay-accent chrome

**Choice:** Delete `.page-chrome.delay-page`, `.page-pill.delay-pill.active`, unused `--delay-accent`, `renderPageChrome` toggle of `delay-page`, and `delay-pill` class on page pill buttons. Delay page uses same chrome as Audio–Drive.

**Why:** User expectation is one governing layout; orange border signals ad-hoc fork.

### D2: Fixed hint slot; hints never mod-gated

**Choice:** `.knob-hint` always `display: block` with `min-height: 1.2em` on all six pages. Only Delay row 0 shows `~0–2 s`; other rows empty string. Delete `applyDelayKnobHints` and stop hiding hint slots in `applyStaticKnobLabels` on non-Delay pages.

**Why:** Eliminates layout shift on Randomize mod without special-casing Delay in `syncKnobUi`.

### D3: VCO morph visibility and click path

**Choice:**

- Show morph buttons when `hostPage === 0` only (drop `wasmPage === 0` gate).
- Morph click: `requireEngineForAction()` then `send({ type: "cycleVcoMorph" })`.
- Optimistic SVG: cycle `lastMorphs[i]` locally using same thresholds as `CycleVcoMorph` in `FroggersEngine.hpp` (`v < 0.25 → 0.5`, `v < 0.75 → 1`, else `0`), then reconcile on `screen`.
- Rename `#rand-morphs` label to **Rand waveforms**.

**Why:** `wasmPage` desyncs on Delay round-trip; clicks before Play were no-ops without guard; optimistic update gives immediate feedback.

### D4: Marbles LED indicator

**Choice:** Threshold `level > 0.55` → LED on (green `#3fb950`), else off (`#21262d` dim). Web: new `ModLedIndicator` div in mod bay for indices 1–2 (Marbles). Desktop: `ModModuleBox` for mod indices 5–6 renders filled circle instead of `CvScopeDisplay`.

**Why:** Marbles CV is sample-and-hold stepped random — flat between gate/Marbles-button steps. Scope trace looks like broken oscilloscope. VCO Envelope stays continuous scope. **S&H** in the label makes the behavior obvious without reading the manual.

### D6: Marbles S&H labels everywhere mod sources are named

**Choice:** User-visible Marbles mod source strings:

| Location | Before | After |
|----------|--------|-------|
| Web mod bay | Marbles 1, Marbles 2 | Marbles 1 S&H, Marbles 2 S&H |
| Web mod `<select>` options | Marbles 1, Marbles 2 | Marbles 1 S&H, Marbles 2 S&H |
| Desktop mod rack (`ModRackPanel`) | Marbles 1, Marbles 2 | Marbles 1 S&H, Marbles 2 S&H |
| Quick Dict / manual mod entries | Marbles 1, Marbles 2 | Marbles 1 S&H, Marbles 2 S&H |

**Out of scope:** Host page name **Marbles** (page 2/6), global **Marbles** action button, keyboard `m` — those trigger a step; they are not mod source labels.

**Why:** User confirmed Marbles is S&H; label should say so on every mod output surface.

### D5: VCO Envelope rename

**Choice:** Update `ParamDisplayNames` Audio row 6, `INTERNAL_MOD_LABELS`, mod dropdown option text, mod bay label, and synced docs. WASM/engine internal names unchanged.

## Risks / Trade-offs

| Risk | Mitigation |
|------|------------|
| LED threshold feels wrong | Use 0.55; tune in manual verify |
| Removing delay accent reduces wayfinding | Page title still reads **Delay (6/6)** |
| Optimistic morph drift | Reconcile on every `screen` message |

## Migration Plan

1. CSS + main.ts delay hint/chrome cleanup.
2. Morph fixes + button rename.
3. ParamDisplayNames + web labels.
4. ModLedIndicator web + desktop ModModuleBox branch.
5. `npm run build` (web); desktop build; manual Delay Randomize mod + morph click checks.

## Open Questions

None.
