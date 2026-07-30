## Context

```
Current pair-AR band (Audio page, desktop)
Column ~70 px wide × kBandHeight 68 px
┌────────┐
│  jack  │  20 px
│ (knob) │  38 px
│ Atta…  │  14 px  ← juce::Label horizontal, clipped
└────────┘

Stack: 20 + 2 + 38 + 2 + 14 = 76 px > kBandHeight 68 → clip
"Release 1+2" ~75 px wide → needs ~75 px height when rotated
```

Labels remain from `ParamDisplayNames::forAudioPairAr(i)` — rendering only.

`MainComponent` assigns panel height from window chrome; `SubModulePanel::layoutPanel()` uses `getLocalBounds()`. Fixing **`kBandHeight`** fixes the band; no `DesktopChromeLayout` change required.

## OMNI audit (this proposal)

| Rule | Finding | Resolution |
|------|---------|------------|
| Single authority | `kBandHeight` must not be duplicated vs `setSize` magic | One formula in `AudioPairArLayout.hpp`; `setSize` adds `kBandTopPad + kBandHeight` only |
| Repetition | Magic `2` px gaps twice per column in `layoutPairArBand` | Add `kStackGap`; use in loop and in `kBandHeight` sum |
| Repetition | Four label paint blocks if drawn in `SubModulePanel::paint` | One `PairArRotatedLabel` class, four instances in one loop |
| One-time helper | Separate `.cpp` for ~20 lines of paint | **Header-only** `PairArRotatedLabel.h` — no CMake churn (WaveMorphButton needs `.cpp`; this does not) |
| Nesting | Keep paint in component | `SubModulePanel` stays flat |
| Defensive code | Runtime font measurement each layout | Fixed `kPairArLabelZoneH` constant sized for longest string **Release 1+2** at 11 pt (~80 px safe margin) |
| Modified spec | Original desktop-ui spec implied horizontal label | Delta spec in this change amends requirement |
| Data flow | `refresh()` → `setText()` → `repaint()` | Document in component contract |

**Helper extraction review (`PairArRotatedLabel`):**

| Criterion | Met |
|-----------|-----|
| Trigger count ≥2 | Yes — transform paint + explicit bounds contract |
| Domain boundary | Yes — pair-AR label rendering |
| Complexity | Yes — rotation + centred draw |
| Local scope | Yes — desktop Audio band only |

## Goals / Non-Goals

**Goals:**

- Full label strings visible at 300 px module width
- Text rotated **90° clockwise** (reads upward; first character at bottom of column)
- Table-driven constants and single layout loop

**Non-goals:**

- Web, VCV, engine, snapshot, Release→Decay rename, window default height bump

## Decisions

### D1 — Increase band height via constexpr sum

**Choice:** In `AudioPairArLayout.hpp`:

```cpp
constexpr int kStackGap = 2;
constexpr int kPairArLabelZoneH = 80;  // covers "Release 1+2" at 11 pt rotated
constexpr int kBandHeight =
    kJackSize + kStackGap + kKnobSize + kStackGap + kPairArLabelZoneH;
```

Remove use of `kLabelRowH` for pair-AR (that constant stays for vertical rows only).

**Why:** Rotation maps string width → required height; 80 px safe constant avoids JUCE font measurement at runtime.

### D2 — Header-only `PairArRotatedLabel`

**Choice:** `desktop/Source/PairArRotatedLabel.h` — inline class:

- `setText(const juce::String&)` stores text and calls `repaint()`
- `paint()` — `AffineTransform::rotation(+π/2, cx, cy)`, `drawText` centred, 11 pt font

**Why:** Minimal diff; no CMakeLists edit; one implementation reused by four components.

### D3 — Layout: explicit zones, not “whatever remains”

**Choice:** `layoutPairArBand()` loop per column:

1. `removeFromTop(kJackSize)` — jack (centred)
2. `removeFromTop(kStackGap)`
3. `removeFromTop(kKnobSize)` — knob (centred)
4. `removeFromTop(kStackGap)`
5. `removeFromTop(kPairArLabelZoneH)` — `PairArRotatedLabel` bounds

**Why:** Explicit zones match `kBandHeight` sum; “remaining height” drifts if gaps change elsewhere.

### D4 — Panel height

**Choice:** `setSize(300, 480 + kBandTopPad + kBandHeight)` for Audio page — replace hard-coded `68` with `kBandHeight`.

**Why:** `480` base unchanged; band delta tracks constant authority.

## Risks / Trade-offs

| Risk | Mitigation |
|------|------------|
| Taller Audio panel body | ~12 px net increase over broken 68 px band; window 720 px unchanged |
| Longest label grows later | Bump `kPairArLabelZoneH` in one header; comment ties to `forAudioPairAr(1)` |
| Rotation reads awkwardly | User requested 90° CW; matches Eurorack silkscreen convention |

## Migration Plan

1. Land `AudioPairArLayout` constant updates
2. Add `PairArRotatedLabel.h`, wire in `SubModulePanel`
3. Desktop Release build + visual check four columns

## Open Questions

None.
