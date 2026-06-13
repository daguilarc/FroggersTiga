## Why

After the knob-column UX cleanup, the simulator reads slightly off-center on desktop browsers: the mod bay and page chrome span the full `#app` width, but the knob grid sits in a narrower column between flanking page arrows, and action strips use mixed left/center alignment. Mobile centering must stay intact — page pills, swipe, and 44px nav targets are already correct there.

## What Changes

- **Unify the horizontal layout axis** so mod bay, knob field, page pills, and global strip share one centered content column on desktop.
- **Desktop (>720px):** hide flanking page-nav buttons (keyboard `[`/`]` and page pills remain); knob grid uses full `#app` width like the mod bay above it.
- **Mobile (≤720px):** keep the existing three-column `field-layout` with prev/next arrows beside the knob grid — no regression to touch targets or swipe.
- **Fix mod bay CSS conflict:** remove `#mod-bay { display: flex }` override so `.mod-bay` grid (`repeat(3, 1fr)`) governs all three mod cells equally.
- **Center global strip** on desktop to match page pills (left-aligned wrap remains on mobile if needed).
- **Stable scrollbar gutter** on `html` so `margin: 0 auto` centering does not shift when a vertical scrollbar appears.

## Capabilities

### New Capabilities

- `web-field-horizontal-align`: Shared horizontal alignment for mod bay, knob field, and bottom action strips; responsive desktop vs mobile field layout.

### Modified Capabilities

- `host-ui-delay-page`: Clarify that flanking page-nav arrows are required only at viewport ≤720px; desktop may omit visible arrows.

## Impact

- `web/src/style.css` — layout rules only; no JS changes required
- `openspec/specs/host-ui-delay-page/spec.md` — delta at archive time
