## Why

`desktop-chrome-cohesion` fixed desktop label drift, mod CV visibility, and touch-friendly chrome. The web sim already has strong mobile paging (`web-sim-page-ux`), but **UI strings still disagree with Quick Dict and desktop**: global strip says “Randomize all” while docs say **Rand All**; mod UI still says **VCO Envelope** while Quick Dict says **VCO level**; mod meters snap to zero when audio stops so Marbles held CV looks dead; transport and global-strip buttons lack the 44px touch targets used elsewhere. Web must gain **intent parity**, not desktop layout (no two-row header, no trace rack, no RECORD).

## What Changes

- **Label policy** — global strip: **Rand All**, **Rand Mods**, **Rand waves**, **Marbles**; mod sources everywhere: **VCO level** (not VCO Envelope); route summary and dropdowns use the same `MOD_SOURCE_LABELS` map.
- **Mod meter visibility** — persist last CV level when Play stops (dim fill); brief step emphasis on Marbles 1/2 level change while playing (adapt desktop `m_lastLevel` intent to 6px bars).
- **Mobile touch targets** — `min-height: 44px` on `.controls-top .transport`, `.controls-top #external-btn`, and `.global-strip button`.
- **Reading order** — move `#mod-route-summary` below `#page-chrome` so users see page context before routes (matches `web-sim-page-ux` design).
- **Mod bay hint** — subtitle “CV level while playing” on collapsible mod bay toggle area.

**Non-goals:** window resize policy, desktop header reflow, RECORD/export, CV trace scopes, patch cables, MIDI rack.

## Capabilities

### New Capabilities

- `web-chrome-labels`: Global strip and mod-source string parity with Quick Dict and desktop.
- `web-mod-meter-visibility`: Mod bay meters show held CV and step changes on mobile-friendly bars.
- `web-mobile-touch-targets`: Transport and global strip meet 44px minimum touch height.

### Modified Capabilities

- `web-page-chrome` (delta over `web-sim-page-ux`): DOM reading order — route summary after page chrome header.
- `sim-mod-patchbay` (delta): Web mod source display name **VCO level** aligned with Quick Dict.

## Impact

- `web/index.html` — button text, DOM order for route summary
- `web/src/main.ts` — `MOD_SOURCE_LABELS`, `INTERNAL_MOD_LABELS`, `renderModBay` hold/step logic
- `web/src/style.css` — touch targets, dim idle meter fill, optional step flash
- `openspec/changes/web-sim-page-ux/proposal.md` — footnote global strip label supersession
