## 1. Layout map

- [x] 1.1 Add `constexpr std::array<uint8_t, 5> kDesktopCoreColumnPageOrder{0, 1, 3, 2, 4}` in `DesktopChromeLayout.hpp` (or `MainComponent` anonymous namespace) with a one-line comment citing `ApplyOutputFx` filter-before-reverb order
- [x] 1.2 Change `MainComponent::resized()` to assign column bounds using the map (iterate columns, `setBounds` on `m_panels[pageIndex]`) instead of `m_panels[i]` at column `i`

## 2. Verification

- [x] 2.1 Manual smoke: Filter column left of Reverb; Wet/dry and Comb feedback knobs affect correct DSP
- [x] 2.2 Manual smoke: patch-cable jacks on pages 2 and 3 align with swapped panels
- [x] 2.3 Confirm web page pill order and `ParamDisplayNames` host page indices unchanged (`web/e2e/host-page-pill-order.spec.ts`)

## 3. Documentation

- [x] 3.1 Update `SIM_MANUAL.md` desktop host guide: note Filter column precedes Reverb column to match signal flow; Field hardware page order unchanged
- [x] 3.2 Run `scripts/sync-help-docs.sh` so `web/public/sim-manual.md` matches
