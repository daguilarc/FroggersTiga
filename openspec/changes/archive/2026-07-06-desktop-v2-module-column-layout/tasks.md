## OMNI verification gates (run before merge)

- [ ] OMNI.1 Build desktop-v2 standalone (`FroggersTigaDesktopV2` links)
- [ ] OMNI.2 `LayoutBounds_test` passes and proves zero center-cluster/mod-cell intersection at 1280×920
- [ ] OMNI.3 Audio page at 1280×920 has no vertical scrollbar when eight rows fit
- [ ] OMNI.4 Helper authority audit: `PageCarouselComponent`, `SubmodulePagePanel`, and `AdsrPagePanel` consume `moduleRowColumns`; panel code has no independent `gridPx(31)` mod X placement
- [ ] OMNI.5 Plan/spec language grep returns zero hedge terms in this change folder
- [ ] OMNI.6 New layout branches keep nesting depth ≤3 in touched layout functions
- [ ] OMNI.7 Manual operator QA at 1280×920: no overlap, no Shift/encoder collision, no label ellipsis

## 1. Layout authority

- [ ] 1.1 Add `ModuleRowColumnLayout` struct + `moduleRowColumns(int rowWidth)` to `DesktopV2ChromeLayout.hpp`
- [ ] 1.2 Derive mod X from column sum; deprecate direct `gridPx(31)` usage in panel code
- [ ] 1.3 Update `kPerfMarblesLabelH` to `gridPx(2)`; center marbles labels vertically in performance band
- [ ] 1.4 Add center-cluster compact-gap and internal-scroll constants to `DesktopV2ChromeLayout.hpp`

## 2. Carousel column split (remove overlay)

- [ ] 2.1 Refactor `PageCarouselComponent::resized` to split body into label+encoder | center | mod regions via `moduleRowColumns`
- [ ] 2.2 Bind `m_centerCluster` to center column bounds only — remove full-height overlay `setBounds`
- [ ] 2.3 Pass column geometry into `SubmodulePagePanel` / `AdsrPagePanel` (width + mod column origin)

## 3. Submodule + ADSR panel restructure

- [ ] 3.1 Narrow encoder viewport to label+encoder column width
- [ ] 3.2 Move mod cells into a sibling `m_modColumnViewport` at x=0 within mod column content
- [ ] 3.3 Sync vertical scroll between encoder and mod columns when `docH > availH`
- [ ] 3.4 Apply identical column layout to `AdsrPagePanel`
- [ ] 3.5 Scroll policy: hide scrollbars when document fits; reset view position to top
- [ ] 3.6 Keep center cluster overflow contained by compact spacing plus internal vertical scroll

## 4. Sequencer + performance band polish

- [ ] 4.1 Top-align sequencer step grid (remove vertical centering dead space)
- [ ] 4.2 Fix performance band label truncation (scene ordinals, marbles S&H labels)

## 5. Layout regression gate

- [ ] 5.1 Add `desktop-v2/tests/LayoutBounds_test.cpp` — assert no center-cluster ∩ mod-cell intersection at 1280×920 on Audio page
- [ ] 5.2 Add test assertion: Audio 8-row page has no vertical scrollbar when viewport height ≥ document height
- [ ] 5.3 Register test in `desktop-v2/CMakeLists.txt`; wire into ctest
- [ ] 5.4 Add focused grep check for no independent `gridPx(31)` mod X placement in panel code

## 6. Hosted parity + verification

- [ ] 6.1 Verify `HostedMainComponentV2` carousel uses same column split
- [ ] 6.2 Build Release; run `./scripts/open-desktop-v2.sh`
- [ ] 6.3 Manual QA at 1280×920: no overlap, no scroll on Audio, no `...` on mod/scene labels
- [ ] 6.4 Run `ctest --test-dir desktop-v2/build -R LayoutBounds --output-on-failure`
- [ ] 6.5 OMNI compliance check: complete OMNI.1–OMNI.7 gates above

## Manual QA checklist (1280×920)

1. Module Audio: all 8 rows visible without scrolling.
2. Rand All column does not cover mod dropdowns.
3. Shift does not cover encoder dials.
4. Performance band: no `...` on scene or S&H labels.
5. Sequencer step grid uses upper portion of panel — no large empty band above steps.
