## Why

The Audio pair-AR bottom band on desktop truncates labels to “Atta” / “Rele” for two stacked reasons: (1) horizontal text in a 14 px-tall row inside ~70 px-wide columns, and (2) `kBandHeight` (68 px) is shorter than the jack + knob + label stack (~76 px). Operators cannot read **Attack 1+2**, **Release 1+2**, etc.

## What Changes

- **Desktop only:** pair-AR band labels render **rotated 90° clockwise**, full strings from `ParamDisplayNames::forAudioPairAr`
- **`AudioPairArLayout.hpp`:** add `kStackGap`, `kPairArLabelZoneH`, recompute `kBandHeight` as a single constexpr sum (no duplicated magic numbers in `SubModulePanel`)
- **`PairArRotatedLabel.h`:** header-only component (paint + `setText` + `repaint`); four instances in existing `layoutPairArBand()` loop — no `.cpp`, no CMake edit
- **Delta spec:** amend `audio-pair-ar-desktop-ui` — labels below knob are rotated, not horizontal

**Non-goals:** Web UI, VCV silkscreen, engine/snapshot, label renames, `MainComponent` / window chrome height changes

## Capabilities

### New Capabilities

- `pair-ar-rotated-desktop-labels`: Rotated pair-AR label rendering on desktop

### Modified Capabilities

- `audio-pair-ar-desktop-ui`: Label orientation and legibility under band column width (delta in this change)

## Impact

- `sim/AudioPairArLayout.hpp` — band height formula, label zone height, stack gap
- `desktop/Source/PairArRotatedLabel.h` — new header-only component
- `desktop/Source/SubModulePanel.{h,cpp}` — swap label type, band layout, `setSize` uses `kBandHeight` only
