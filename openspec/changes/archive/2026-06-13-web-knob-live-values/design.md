## Context

```
WASM screen tick (every ~20 audio blocks)
    │
    ├─ rows[i].value     ← froggers_row_value → Parameter::Get(modMgr)  [effective]
    ├─ rows[i].modDepth  ← attenuator depth (user-set, changes only on drag)
    └─ rows[i].modSource ← 255 = unpatched

Desktop SubModulePanel (correct):
    idle + patched  → knob = effective value (wiggles)
    drag start      → snap to modDepth, edit depth until drag end
    idle + unpatched → knob = base/effective value

Web main.ts (wrong):
    idle + patched  → knob = modDepth  (frozen)
    idle + unpatched → knob = row.value
    label           → "Mod depth" when patched
```

`row.value` is already computed in the worklet. The bug is a one-line branch in `syncKnobUi` plus missing drag-start snap. No new WASM export.

Reference: archived `desktop-sim-ux-polish/specs/desktop-panel-knobs` — "Knobs show effective value when modded and idle". [thenoriegas.info](https://thenoriegas.info) — knobs track live state and randomize visibly.

## Goals / Non-Goals

**Goals:**

- Web knob idle display matches desktop and noriegas: `row.value` always when `!knobDragging[i]`
- Patched-row drag edits mod depth with snap-on-pointer-down (desktop parity)
- Randomize visibly moves all idle knobs including patched rows
- Static parameter labels unchanged during mod routing
- Single data path: one `screen` handler → one `syncKnobUi` loop over eight columns

**Non-Goals:**

- New mod UI (sliders, depth readout, separate attenuator control)
- WASM or processor changes
- Desktop behavior changes
- Rebuilding patch-cable overlay (web uses dropdown mod assign)
- Full `#oled` removal in this change (optional follow-up; demoted from live-value role)

## Decisions

### D1: Idle display always uses `row.value`

**Choice:** In `syncKnobUi`, replace `row.modSource === 255 ? row.value : row.modDepth` with `row.value` for all rows when `!knobDragging[i]`.

**Why:** `row.value` is `Parameter::Get(modMgr)` — the same effective value desktop shows. OMNI: one source, one assignment per screen tick, no per-row branch on display value.

**Alternative rejected:** Keep modDepth on idle — contradicts noriegas/desktop and makes knobs look broken under live CV.

### D2: Snap to modDepth on pointer-down when patched

**Choice:** Extend `RotaryKnob` constructor with optional `onDragStart?: () => void` called at pointer-down before `dragStartValue` is captured. In `main.ts` knob factory, when `modSelect !== 255`, `onDragStart` reads latest `row.modDepth` from last screen rows (store `lastScreenRows` ref updated in `onScreenUpdate`) and `setValue(modDepth)`.

**Why:** Matches desktop `onDragStart` → `setValue(getModDepth())`. User edits depth from known attenuator position, not from wiggling effective value.

**Alternative rejected:** Edit effective value while dragging patched row — would send wrong message type and fight live CV updates.

### D3: Labels stay static; mod state elsewhere

**Choice:** Remove "Mod depth" label overlay from `updateKnobLabels`. Always use `HOST_PAGE_LABELS[hostPage][i]` unless delay hints apply. Mod active state: mod-source `<select>` + `#mod-route-summary`.

**Why:** noriegas and desktop keep parameter names on columns. "Mod depth" rename implied the knob was a depth control at rest — it is not.

### D4: `lastScreenRows` accumulator for drag-start snap

**Choice:** Module-level `let lastScreenRows: ScreenRow[] = []` assigned once per `onScreenUpdate`. Drag-start reads `lastScreenRows[i].modDepth`.

**Why:** OMNI accumulate-then-apply; no per-knob WASM query from main thread. Depth at pointer-down is the last posted screen depth.

### D5: OLED no longer carries unique live-value duty

**Choice:** Do not expand OLED in this change. After D1, knobs own live display. Existing `web-sim-layout-ux` OLED collapse remains; future change can delete `#oled` entirely.

**Why:** Minimal scope. Fixing knobs removes the only functional justification for the black bar.

## Risks / Trade-offs

| Risk | Mitigation |
|------|------------|
| Knob wiggle at 20-frame cadence feels stepped | Same cadence as desktop timer + noriegas; acceptable |
| Drag snap uses stale modDepth if screen lags | Depth changes only on user drag or randomize; screen posts on same cadence as values |
| User expects knob at rest to show attenuator position | Desktop trade-off documented in `desktop-sim-ux-polish`; user explicitly wants noriegas parity |
| `lastScreenRows` empty before first screen tick | Drag before Play already gated by `requireEngineForAction`; no snap needed pre-engine |

## Migration Plan

1. Implement D1–D4 in `main.ts` + `RotaryKnob.ts`
2. `npm run build` in `web/`
3. Manual: Play → patch Marbles → confirm knob wiggles; drag → edits depth; Randomize → knobs jump
4. No data migration; no deploy ordering dependency

## Open Questions

None — behavior is fully specified by desktop parity and noriegas reference.
