## Why

The desktop **MIDI Settings** dialog truncates CC number fields to an ellipsis (`…`) because each CC `juce::Slider` is allocated only **50 px** width while displaying values in the range **0–127** (up to three digits). Users cannot read or confidently edit CC numbers—breaking a basic MIDI configuration workflow. The underlying `CvMidiBridge` state and callbacks are correct; this is a layout/display defect only.

## What Changes

- Define shared layout constants for MIDI settings numeric controls (channel vs CC widths) instead of repeated magic `50` literals.
- Configure CC sliders with an explicit `TextBoxRight` style and a text-box width that fits **127**.
- Widen CC slider bounds in `resized()` so the value text is fully visible at all CC values (0–127).
- Apply the same CC slider setup to **In CC** and **Out CC** via one initialization path (OMNI repetition rule).
- Add a manual verification step: open MIDI Settings, set In CC and Out CC to **10**, **74**, and **127**; confirm no truncation.

## Capabilities

### New Capabilities

- `desktop-midi-cc-display`: MIDI Settings CC controls SHALL display the full numeric CC value (0–127) without ellipsis truncation at default dialog size (480×420).

### Modified Capabilities

- (none — no archived baseline specs in repo)

## Impact

- `desktop/Source/MidiSettingsComponent.cpp` — slider text-box configuration and `resized()` layout constants
- `desktop/Source/MidiSettingsComponent.h` — no API changes expected
- No changes to `CvMidiBridge`, audio routing, or web sim (browser build has no MIDI Settings UI)
