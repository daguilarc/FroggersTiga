> **Reconciled (omni 1.2):** Code-backed; tasks 4.2–4.3 visual manual checks remain open.

## 1. Layout constants (single authority)

- [x] 1.1 `AudioPairArLayout.hpp`: add `kStackGap`, `kPairArLabelZoneH` (80), recompute `kBandHeight` as jack + gaps + knob + gaps + label zone
- [x] 1.2 Stop using `kLabelRowH` for pair-AR; pair-AR band uses `kBandHeight` only
- [x] 1.3 `SubModulePanel` Audio `setSize`: `480 + kBandTopPad + kBandHeight` (no literal 68)

## 2. Rotated label component (header-only)

- [x] 2.1 Add `desktop/Source/PairArRotatedLabel.h`: `setText` + `repaint`, `paint` with `AffineTransform::rotation(+π/2, …)`, 11 pt centred draw
- [x] 2.2 Replace `std::array<juce::Label, 4>` with `std::array<PairArRotatedLabel, 4>` in `SubModulePanel.h`; include header

## 3. Layout + refresh (one loop)

- [x] 3.1 `layoutPairArBand()`: explicit stack using `kStackGap` and `kPairArLabelZoneH` (not magic `2` or `kLabelRowH`)
- [x] 3.2 `refresh()`: `m_pairArLabels[i].setText(getPairArName(i))`
- [x] 3.3 Constructor: remove horizontal-label setup (`setJustificationType`, etc.) for pair-AR labels

## 4. Verification

- [x] 4.1 Build desktop Release
- [ ] 4.2 Visual: four full rotated labels at 300 px width — no **Atta** / **Rele** truncation
- [ ] 4.3 Visual: jack → knob → label order unchanged; patch overlay jacks still align
