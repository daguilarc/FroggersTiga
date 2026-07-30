## Why

Standalone desktop pins audio and DSP to **44100 Hz** (`kSimSampleRate`) with no UI to switch rates. macOS and many interfaces default to **48000 Hz**, causing device/DSP mismatch warnings. The VST path already follows the host rate via `prepareToPlay`; standalone should offer the same two rates the sim supports.

## What Changes

- Add **44.1 kHz / 48 kHz** selector in Audio Settings.
- Single rate authority in `sim/HostAudioConfig.hpp` (supported rates + default).
- `AudioEngine` drives `setHostSampleRate` from the **opened device rate**, not a hardcoded constant.
- Export/recording metadata uses the active engine rate.
- Recorder capacity sized for max supported rate (48 kHz × 30 min).
- Bump desktop version **1.0.1 → 1.0.2**; rebuild DMG; push release assets.

## Capabilities

### New Capabilities

- `desktop-sample-rate-select`: Standalone sample-rate UI, engine sync, export parity.

### Modified Capabilities

- (none at repo spec level)

## Impact

- `sim/HostAudioConfig.hpp` (new)
- `desktop/Source/AudioEngine.{h,cpp}`
- `desktop/Source/AudioSettingsComponent.{h,cpp}`
- `desktop/Source/AudioRecorder.h`
- `desktop/CMakeLists.txt` (version bump)
