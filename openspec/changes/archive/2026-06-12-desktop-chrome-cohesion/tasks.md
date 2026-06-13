# Desktop chrome cohesion — tasks

## 1. Window resize

- [x] 1.1 `MainWindow`: `setResizable(true, true)` + `setResizeLimits(1024, 600, 8192, 4320)`
- [x] 1.2 Default **1440×720** in `Main.cpp` + `MainComponent.cpp` (supersedes 1680)
- [x] 1.3 Footnote `desktop-compact-layout` design §1 → 1440 + strip label supersession

## 2. Shared layout constants

- [x] 2.1 Add `DesktopChromeLayout.hpp`: `kModBoxWidth=96`, `kModBoxGap=16`, `kModBoxMinWidth=80`, `kRecordClusterW=120`, `kFormatRowH=20`, `kTransportRowH=32`, `kModRackRowH=72`
- [x] 2.2 `MainComponent::resized`: two-row header (`kTransportRowH` + `kModRackRowH`); no full-height right column dead band

## 3. Header reflow (`desktop-audio-export` §5.1)

- [x] 3.1 Row 1: Play | Stop | External | In env | … | MIDI | Audio | **RECORD**
- [x] 3.2 Row 2: mod rack (left, full remaining width) | format toggles (right `kRecordClusterW`)

## 4. Mod rack centered fixed width

- [x] 4.1 `ModRackPanel::resized`: center `4×kModBoxWidth + 3×kModBoxGap` group; use `kModBoxGap`
- [x] 4.2 Allow box shrink to `kModBoxMinWidth` only when window narrower than rack group

## 5. Global strip full labels

- [x] 5.1 `GlobalStrip`: button text → **Rand All**, **Rand Mods**, **Rand waves**, **Marbles**
- [x] 5.2 `GlobalStrip::resized`: `getBestWidthForHeight` per button
- [x] 5.3 Center button group in strip bounds

## 6. CV scope visibility

- [x] 6.1 `CvScopeDisplay`: level fill for StepHold; cache `m_lastLevel`
- [x] 6.2 `paintIdle`: draw at `m_lastLevel` when known (dimmed)
- [x] 6.3 `ModModuleBox::refresh`: oversample step edges (min/max → multiple pushSample)
- [x] 6.4 `InputEnvelopeIndicator`: tooltip "Input level (Play + External on)"

## 7. Record cluster polish (`desktop-audio-export` §5.2–5.3)

- [x] 7.1 `RecordExportCluster::resized`: `kRecordClusterW=120`; distribute height remainder across 4 rows
- [x] 7.2 Set uniform `Font` (11pt bold) on all format toggles — JUCE `ToggleButton` has no `setFont`; equal row bounds enforce visual parity
- [x] 7.3 Format toggles: checkbox tick chrome with radio-group exclusivity
- [x] 7.4 Mark `desktop-audio-export` tasks 5.1–5.3 absorbed here

## 8. Quick Dict alignment

- [x] 8.1 Update `QUICK_DICT.md` transport lines: **Rand All**, **Rand Mods** (not Rand all / Randmod all)

## 9. Verification

- [x] 9.1 Resize window narrower than 1680 — succeeds
- [x] 9.2 **Marbles** + **OGG** labels full size at default; no 68px header dead band
- [x] 9.3 Mod scopes ~96px wide, centered, not 400px
- [x] 9.4 Play → Marbles → held CV visible on scope
- [x] 9.5 External + Play → input level bar fills
- [x] 9.6 Strip shows **Rand All**, **Rand Mods**, **Rand waves**, **Marbles** without truncation
- [x] 9.7 Audio panel wave rows fit at 1440×720 (no clip)
- [x] 9.8 Desktop Release build
