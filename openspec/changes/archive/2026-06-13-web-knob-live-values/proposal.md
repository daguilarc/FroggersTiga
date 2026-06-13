## Why

The web sim knobs are supposed to be the primary control surface — same as [thenoriegas.info](https://thenoriegas.info) and the desktop sim. When WASM posts live parameter state, knobs move. When mod CV is active, knobs wiggle. When Randomize fires, knobs jump to new positions. That is the baseline contract.

`syncKnobUi` currently does the opposite when mod is patched: it pins the knob to static **mod depth** instead of `row.value` (the effective modulated parameter from `froggers_row_value` / `Parameter::Get`). The knob looks dead while CV moves. The redundant `#oled` strip was carrying the live values nobody asked for. Desktop `SubModulePanel` and archived `desktop-sim-ux-polish` already shipped the correct behavior years ago; web never caught up.

## What Changes

- **Idle knob display**: When the user is not dragging, every knob column SHALL show `row.value` from the WASM `screen` payload — effective modulated parameter value, patched or not. Same source as Randomize refresh and desktop `getEffectiveKnob`.
- **Drag-on-patched-row**: Pointer-down on a patched row snaps the knob to `row.modDepth` and edits depth until pointer-up — desktop `onDragStart` parity. Unpatched rows continue to edit base knob value.
- **Static labels**: Knob column labels stay the page parameter names from `HOST_PAGE_LABELS`. Mod routing is shown via mod-source dropdown and route summary, not by renaming the column to "Mod depth".
- **Randomize parity**: Page Randomize, Rand All, and post-randomize `screen` ticks SHALL move knob pointers the same way as noriegas — no special case for patched rows on idle display.
- **OLED demotion**: `#oled` is no longer the live-value surface. Remove or keep collapsed; knob columns own value display.

## Capabilities

### New Capabilities

- `web-knob-live-values`: Web rotary knobs reflect WASM effective parameter values on idle refresh; patched-row drag edits mod depth; labels stay static.

### Modified Capabilities

<!-- none at openspec/specs/ root; web-rotary-knobs lived in archived web-sim-core-fix with incorrect patched-idle semantics -->

## Impact

- `web/src/main.ts` — `syncKnobUi`, `updateKnobLabels`, knob drag-start snap, optional OLED removal
- `web/src/RotaryKnob.ts` — optional `onDragStart` hook before drag baseline is captured
- `web/src/style.css` — only if `#oled` is removed from layout
- `openspec/changes/web-sim-layout-ux` — OLED collapse spec becomes optional cleanup, not live-value carrier
- No WASM, processor, or desktop changes — `row.value` already carries effective values
