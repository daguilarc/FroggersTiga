## Why

`desktop-chrome-cohesion` set `RecordExportCluster` bounds to the **entire header** (`m_recordCluster.setBounds(header)`) while Play, Stop, External, Audio, and MIDI remain **sibling** components underneath it in z-order. JUCE delivers mouse events to the topmost component whose bounds contain the click. The cluster’s empty background intercepts transport clicks — **Play does nothing at startup**. Randomize still works because it lives below the header. This is a hit-test/layout bug, not an audio engine failure.

## What Changes

- **Tight cluster bounds** — `RecordExportCluster` bounds SHALL be the union of the RECORD row rect and the format-column rect only, never the full header.
- **Pass-through policy** — cluster SHALL use `setInterceptsMouseClicks(false, true)` so only RECORD and format children receive clicks.
- **Explicit transport z-order** — after layout, transport controls SHALL be `toFront` below `PatchCableOverlay` (overlay `hitTest` already passes through off-jack clicks).
- **Regression task** — verify Play → audio runs, Audio/MIDI dialogs open, RECORD and format toggles still work.

## Capabilities

### New Capabilities

- `desktop-header-hit-test`: Header layout does not block transport or settings buttons.

### Modified Capabilities

- `desktop-chrome-layout` (delta over `desktop-chrome-cohesion`): Record cluster bounds policy — union of chrome children, not parent header.

## Impact

- `desktop/Source/MainComponent.cpp` — `resized()` cluster bounds + transport `toFront`
- `desktop/Source/RecordExportCluster.cpp` — intercept policy in constructor
- `openspec/changes/desktop-chrome-cohesion/design.md` — footnote hit-test correction
