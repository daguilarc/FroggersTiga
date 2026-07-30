## 1. Layout map

- [x] 1.1 Update `kDesktopCoreColumnPageOrder` in `DesktopChromeLayout.hpp` from `{0, 1, 3, 2, 4}` to `{0, 1, 4, 3, 2}`; update comment to cite full Drive→Filter→Reverb output FX order
- [x] 1.2 Confirm `MainComponent::resized()` already assigns bounds via the map (no structural change expected)

## 2. Verification

- [x] 2.1 Manual smoke: Drive column left of Filter left of Reverb; GAIN, Comb feedback, and Wet/dry knobs affect correct DSP
- [x] 2.2 Manual smoke: patch-cable jacks on pages 2, 3, and 4 align with swapped panels
- [x] 2.3 Confirm web page pill order and `ParamDisplayNames` host page indices unchanged

## 3. Documentation

- [x] 3.1 Update `SIM_MANUAL.md` desktop host guide: output FX columns read Drive → Filter → Reverb; Field hardware page order unchanged
- [x] 3.2 Run `scripts/sync-help-docs.sh` so `web/public/sim-manual.md` matches
