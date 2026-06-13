## Context

**Reported failure:** Ext. In. on + input device selected (mic, line, interface) + Play running → peak meter flat, no ring mod.

**Current data flow:**

```
Audio Settings (input device name set)
        │
        ▼
audioDeviceIOCallbackWithContext
        │
        ├── numInputChannels == 0  ──► m_inBlock zeros (common: inputChannels BitSet empty)
        ├── inputChannelData[0] null ──► m_inBlock zeros
        ├── macOS TCC denies capture ──► samples all zero (no NSMicrophoneUsageDescription in Info.plist)
        └── else copy ch0 ──► m_inputPeak + ProcessBlock
```

`desktop-external-input-fix` implemented copy-when-enabled logic but never ensured channels are **active** or **permitted**. Verification 5.1–5.3 unchecked.

Built app Info.plist (verified): no `NSMicrophoneUsageDescription`. On macOS this blocks CoreAudio input for **every** source type Apple routes through the audio-input privacy gate — not microphone-specific despite the key name.

## Goals / Non-Goals

**Goals:**

- Any configured input device (mic, line-in, USB interface) delivers non-zero samples to the engine when Ext. In. + Play are on
- Input channel 0 active whenever an input device is selected
- macOS audio capture permission in app bundle
- User sees why input is silent instead of a dead meter
- OMNI: one setup function, one status enum, one callback write path

**Non-Goals:**

- Remove Ext. In. checkbox (web keeps separate External toggle; desktop keeps host gate per archived spec)
- Change Schmidt thresholds (0.01 / 0.005)
- Separate input/output device picker beyond JUCE Audio Settings
- Windows/Linux permission UX beyond documenting JUCE defaults (macOS is the confirmed failure)

## Decisions

### D1: `syncInputChannelSetup()` — single channel activation path

**Choice:** One function in `AudioEngine`:

```
getAudioDeviceSetup(setup)
if inputDeviceName non-empty AND device has ≥1 input:
    setup.inputChannels.setBit(0)
    setup.useDefaultInputChannels = false
setAudioDeviceSetup(setup, true)
```

Call from: constructor (after sample-rate set), `audioDeviceAboutToStart`, and `showAudioSettings` dialog dismiss (close callback — not per audio block).

**Why:** OMNI single path; fixes `numInputChannels == 0` for all device types. No per-device copy-paste.

**Alternative rejected:** Rely on user to check input channel boxes in Audio Settings — already failing silently.

### D2: Platform permission via JUCE CMake (all input types)

**Choice:**

```cmake
MICROPHONE_PERMISSION_ENABLED TRUE
MICROPHONE_PERMISSION_TEXT "FroggersTiga needs audio input access for external ring-mod (mic, line-in, or interface)."
```

**Why:** JUCE maps this to `NSMicrophoneUsageDescription`. Apple uses this key for **all** audio input privacy on macOS regardless of physical source. User-facing text says "audio input" not "microphone only".

**Alternative rejected:** Mic-only wording — misleads interface/line-in users.

### D3: `InputRouteStatus` enum — accumulate in callback, read in UI

**Choice:**

| Status | Condition |
|--------|-----------|
| `Idle` | Ext. In. off or !Play |
| `Ok` | Ext. In. + Play + numInputChannels > 0 + peak > threshold recently |
| `NoInputChannels` | Ext. In. + Play + numInputChannels == 0 |
| `SilentCapture` | Ext. In. + Play + channels > 0 + peak ≈ 0 for ≥ ~1 s |

Update status once at end of `audioDeviceIOCallbackWithContext`. Track sustained silence with `m_silentSampleCount` (increment by `numSamples` when peak < ~1e-4, reset on signal; `SilentCapture` after `kSilentCaptureSamples` ≈ 1 s at 44.1 kHz). `MainComponent` adds `juce::Label m_routeHint` beside transport (after input meter); `timerCallback` reads `getInputRouteMessage()` when status is not `Ok` and Ext. In. + Play.

**Why:** OMNI accumulate-then-apply; O(1) UI read; covers permission denial and silent interfaces.

### D4: Audio Settings min input channels = 1

**Choice:** `AudioDeviceSelectorComponent(..., 1, 2, 0, 2, ...)` — was `(0, 2, ...)`.

**Why:** Product requires at least one input when using Ext. In.; prevents empty `inputChannels` BitSet from UI.

### D5: Keep host Ext. In. gate + engine Schmidt gate

**Choice:** No change to double-gate model from `desktop-external-input-fix`. Host checkbox zeros input when off; Schmidt decides ring mod when on.

**Why:** Matches shipped UX and web parity (default off). `desktop-host-corrections` gate-only model deferred.

## Risks / Trade-offs

| Risk | Mitigation |
|------|------------|
| Forcing ch0 active conflicts with user's channel 2 selection | v1: document ch0; v2: follow first set bit in `inputChannels` |
| SilentCapture false positive on quiet line | Require ~1 s sustained silence; threshold ~1e-4 |
| Permission prompt annoys users who only use VCO | Default Ext. In. off; prompt only when OS requires on first capture |
| `setAudioDeviceSetup` in callback path causes glitch | Call only on init, aboutToStart, settings close — not per block |

## Migration Plan

1. CMake permission flags → rebuild app → macOS prompts on first input capture
2. `syncInputChannelSetup()` in AudioEngine
3. Route status + UI hint
4. Audio Settings min channels + settings-close hook
5. Docs + MANUAL_VERIFY sign-off (mic + line/interface)

## Open Questions

None blocking. Channel-2 selection deferred to v2 if users need it.
