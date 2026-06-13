## 1. WASM / core prerequisites

- [x] 1.1 Confirm `desktop-host-mutation-safety` landed `RandomizeModSim` + Delay source randomize in WASM bindings
- [x] 1.2 Add `froggers_randomize_page(host, page)` and `froggers_randomize_page_mod(host, page)` exports if missing
- [x] 1.3 Wire worklet handlers `randomizePage` / `randomizePageMod` in `froggers-processor.ts`

## 2. HTML shell

- [x] 2.1 Add `#page-chrome` block (title, blurb, per-page Randomize buttons) above `.field-layout`
- [x] 2.2 Add `#mod-route-summary` element below mod bay
- [x] 2.3 Add `#page-pills` nav row with six buttons (Audio … Delay)
- [x] 2.4 Add collapsible header for `#mod-bay` on mobile

## 3. Page chrome logic (`main.ts`)

- [x] 3.1 Static `PAGE_BLURBS` map for host pages 0–5; render in chrome on `screen` update
- [x] 3.2 Wire page **Randomize** / **Randomize mod** to scoped worklet messages
- [x] 3.3 Delay hints map for rows DTIM–FUEG; show under knob labels when `hostPage === 5`

## 4. Mod route summary

- [x] 4.1 Implement `renderModRouteSummary(rows)` from `screen` payload
- [x] 4.2 Source label map `{255: None, 4: VCO level, 5: Marbles 1, 6: Marbles 2}`
- [x] 4.3 Optional: tap summary row highlights matching knob column

## 5. Mod column + wave UX

- [x] 5.1 Add **Mod source** label above each `<select>`; toggle slider label Knob vs **Mod depth**
- [x] 5.2 Replace `waveLabel()` text buttons with inline SVG `wave-icon` component (three bands)
- [x] 5.3 OLED badge shows abbreviated mod source when active

## 6. Mobile navigation

- [x] 6.1 Pill nav: highlight active page; click → `hostPage` message
- [x] 6.2 Swipe handler on `.field-layout` with 60 px threshold; respect `knobDragging`
- [x] 6.3 CSS: pills 44×44 min; Delay page accent (`--delay-accent`); collapsible mod bay @720px

## 7. Manual verification (web)

- [ ] 7.1 §F mobile layout still passes with chrome + pills
- [ ] 7.2 Delay page: hints visible; per-page Randomize mod changes dropdowns + summary
- [ ] 7.3 Audio page: route summary lists modulated rows (dropdown-assigned only); wave SVG cycles on tap
- [ ] 7.4 Global Randomize mod (all) from Delay page updates core + Delay summary when navigating
- [ ] 7.5 Mod bay collapse on 390 px viewport — knobs usable without horizontal scroll
