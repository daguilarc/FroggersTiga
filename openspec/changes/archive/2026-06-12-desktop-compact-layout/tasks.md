# Desktop compact layout + Quick Dict format — tasks

## 1. Default window size

- [x] 1.1 `Main.cpp` + `MainComponent.cpp`: default **1680×720** (replace 2016×720)
- [x] 1.2 Footnote superseded 2016 default in `stereo-delay-page`, `desktop-sim-ux-polish`, `desktop-wave-controls`, `desktop-panel-knobs`, `app-header-help-menu/quick-dict-doc`

## 2. Compact randomize buttons

- [x] 2.1 `SubModulePanel`: `m_randomizeMod` text → **Randmod**
- [x] 2.2 `SubModulePanel::layoutPanel`: intrinsic-width button row (`Font::getStringWidth` + ≤6 px padding)
- [x] 2.3 `GlobalStrip`: **Rand all**, **Randmod all**, **Rand waves**, **Marbles**; intrinsic or proportional layout without dead space

## 3. Quick Dict rewrite

- [x] 3.1 Rewrite `QUICK_DICT.md` as `PRMT : Parameter Name` per page; **PRMT** = sim panel label (`VCO1` not `V1VO` on Audio); short names from `MANUAL.md`
- [x] 3.2 Copy/sync to `web/public/quick-dict.md`
- [x] 3.3 Opening line: abbreviations only; detail → **Manual**

## 4. Verification

- [x] 4.1 Launch at default size on ≤1920-wide display — no horizontal overflow
- [x] 4.2 Every panel shows **Randmod** (not “Randomize mod”)
- [x] 4.3 Randomize button is visibly tighter than before (not half-column wide)
- [x] 4.4 Help → Quick Dict shows `PRMT : Name` format, no parameter tables
- [x] 4.5 Desktop debug + Release build
- [x] 4.6 At 1680 default: VCO1/2/3 labels + 28 px wave buttons fully visible on Audio panel
