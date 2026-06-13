## 1. Platform audio capture permission

- [x] 1.1 `desktop/CMakeLists.txt`: add `MICROPHONE_PERMISSION_ENABLED TRUE` and source-agnostic `MICROPHONE_PERMISSION_TEXT` to `juce_add_gui_app`
- [x] 1.2 Rebuild Release app; confirm `Info.plist` contains `NSMicrophoneUsageDescription`

## 2. Input channel setup

- [x] 2.1 `AudioEngine`: add `syncInputChannelSetup()` — enable input channel 0 when input device name is set
- [x] 2.2 Call from constructor (after sample-rate setup), `audioDeviceAboutToStart`, and Audio Settings dialog close
- [x] 2.3 `showAudioSettings`: min input channels 0 → 1; on dialog dismiss call `syncInputChannelSetup()` (store `DialogWindow*` or close lambda from `launchAsync` — not per audio block)
- [x] 2.4 Log `numInputChannels` in `audioDeviceAboutToStart` when Ext. In. debugging is needed (JUCE Logger)

## 3. Routing diagnostics

- [x] 3.1 `AudioEngine`: add `InputRouteStatus` enum; `m_silentSampleCount` accumulator for ~1 s sustained silence; update status once at end of callback (`Idle`, `Ok`, `NoInputChannels`, `SilentCapture`)
- [x] 3.2 `AudioEngine`: expose `getInputRouteStatus()` and human-readable `getInputRouteMessage()`
- [x] 3.3 `MainComponent`: add `juce::Label m_routeHint` beside transport (after input meter); `timerCallback` sets text from `getInputRouteMessage()` when status is not `Ok` and Ext. In. + Play
- [x] 3.4 Clear route message when status returns to `Ok`

## 4. Docs

- [x] 4.1 `SIM_MANUAL.md`: desktop Ext. In. requires **Ext. In. on + Play**; works with mic, line-in, or interface
- [x] 4.2 `MANUAL_VERIFY.md`: fix step to "Ext. In. on + Play + signal"; add line/interface check

## 5. Verification (required before archive)

- [ ] 5.1 Built-in mic: Ext. In. + Play + speak → meter moves → ring mod above Schmidt threshold
- [ ] 5.2 Line/interface input: same with external interface or line-in source
- [ ] 5.3 Ext. In. off → meter idle, VCO-only timbre
- [ ] 5.4 Audio Settings: cannot disable all input channels
- [ ] 5.5 Fresh install / reset privacy: permission prompt appears; grant → input works
- [ ] 5.6 `NoInputChannels` / `SilentCapture`: `m_routeHint` shows diagnostic; clears when routing returns to `Ok`
- [ ] 5.7 Mark archived `desktop-external-input-fix` tasks §5.1–5.3 complete if verified here
