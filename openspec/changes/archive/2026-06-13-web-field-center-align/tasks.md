## 1. Mod bay grid cleanup

- [x] 1.1 Delete `#mod-bay { display: flex; … }` override block in `style.css`
- [x] 1.2 Ensure `.mod-bay` uses `display: grid; grid-template-columns: repeat(3, minmax(0, 1fr))` only

## 2. Responsive field layout

- [x] 2.1 Add `@media (min-width: 721px)`: hide `.page-nav`; `.field-layout { grid-template-columns: 1fr }`; `.knobs { grid-column: 1 }`
- [x] 2.2 Confirm `@media (max-width: 720px)` retains `auto 1fr auto` and visible `.page-nav` (no change to touch targets)

## 3. Horizontal axis polish

- [x] 3.1 `@media (min-width: 721px)`: `.global-strip { justify-content: center }`
- [x] 3.2 `html { scrollbar-gutter: stable; }`
- [x] 3.3 `.knobs { align-items: stretch }` (replace `end`)

## 4. Verify

- [x] 4.1 Desktop ~1280px: mod bay edges align with knob grid; global strip centered with page pills
- [x] 4.2 Mobile ~390px: flanking arrows visible; knobs do not horizontal-scroll; layout remains centered
- [x] 4.3 `cd web && npm run build`
