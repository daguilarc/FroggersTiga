## Why

Desktop **Ext. In.** appears dead for every input source — built-in mic, line-in, audio interface — even when the checkbox is on, Play is running, and Audio Settings shows an input device selected. The peak meter stays flat and ring mod never opens. `desktop-external-input-fix` landed routing code but verification tasks 5.1–5.3 were never completed; root causes were never fixed: JUCE **active input channels** are never enabled in code, macOS **audio capture permission** is missing from the app bundle (Apple's `NSMicrophoneUsageDescription` gate applies to all audio input, not mic-only), and the UI gives **no diagnostic** when the callback receives silence.

## What Changes

### Input device setup (any source)

- Add `syncInputChannelSetup()` in `AudioEngine`: when an input device is configured, force **input channel 0 active** in `AudioDeviceSetup.inputChannels`; call on init, `audioDeviceAboutToStart`, and after Audio Settings closes.
- Raise `AudioDeviceSelectorComponent` minimum input channels from **0 → 1** so users cannot accidentally disable all inputs while Ext. In. is a product feature.

### Platform audio capture permission (not mic-only)

- Add JUCE `MICROPHONE_PERMISSION_ENABLED` + permission text to `desktop/CMakeLists.txt`. On macOS this plist key gates **all** CoreAudio input capture (mic, line-in, USB interface) — misnamed by Apple, required for any external input.
- Permission string user-facing: "external audio input" / ring-mod routing, not "microphone" alone.

### Routing diagnostics

- Track `InputRouteStatus` per callback: `ok`, `noInputChannels`, `silentCapture` (Ext. In. + Play + channels active but peak ≈ 0 for N blocks).
- Surface status in transport area when routing is broken (e.g. "No input channels — enable input in Audio Settings" or "Input silent — check macOS Privacy → Microphone / line level").
- Meter stays driven by `getInputPeakLevel()`; idle chrome unchanged when Ext. In. off or stopped.

### Docs + verification

- Fix `SIM_MANUAL.md` desktop Ext. In. line (requires **both** Ext. In. and Play; any input device).
- Complete `MANUAL_VERIFY.md` checks for mic **and** line/interface input.

## Capabilities

### New Capabilities

- `desktop-ext-in-device-setup`: Guarantee JUCE active input channels when an input device is configured; re-sync on device change.
- `desktop-audio-capture-permission`: Platform permission plist/CMake for audio input capture on macOS (all source types).
- `desktop-ext-in-diagnostics`: User-visible routing status when Ext. In. + Play but no samples arrive.

### Modified Capabilities

- (none — archived `desktop-external-input-routing` / `desktop-input-level-meter` never promoted to `openspec/specs/`; this change introduces them with the fixes above)

## Impact

- `desktop/CMakeLists.txt` — JUCE audio capture permission flags
- `desktop/Source/AudioEngine.{h,cpp}` — `syncInputChannelSetup()`, route status, device-change hook
- `desktop/Source/MainComponent.cpp` — display routing status; Audio Settings close callback
- `SIM_MANUAL.md`, `MANUAL_VERIFY.md` — accurate Ext. In. steps
- No WASM, web, or firmware changes
