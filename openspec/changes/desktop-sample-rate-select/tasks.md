## 1. Rate authority

- [x] 1.1 Add `sim/HostAudioConfig.hpp`: `kSupportedRates`, `kDefaultSampleRate`, `kNumSupportedRates`, `kMaxSampleRate`, `maxRecordingSamples()`

## 2. AudioEngine

- [x] 2.1 Replace `kSimSampleRate` with `m_hostSampleRate`; init from `kDefaultSampleRate`
- [x] 2.2 `audioDeviceAboutToStart`: sync DSP to device rate; store `m_hostSampleRate`
- [x] 2.3 `writeCaptureToFile`: use `m_hostSampleRate`
- [x] 2.4 Silent-input detect: 1 s threshold from `m_hostSampleRate`
- [x] 2.5 Add `getHostSampleRate()` / `getSupportedSampleRates()` accessors

## 3. Audio Settings UI

- [x] 3.1 Sample-rate combo populated from `HostAudioConfig`
- [x] 3.2 `applySampleRate()` on change; preserve in/out devices
- [x] 3.3 Status line shows active sample rate

## 4. AudioRecorder

- [x] 4.1 Size buffer for `HostAudioConfig::maxRecordingSamples()` (48k × 30 min)

## 5. Release

- [x] 5.1 Bump `desktop/CMakeLists.txt` to 1.0.2
- [x] 5.2 Release build + `package-macos.sh`
- [ ] 5.3 Commit, push, move tag `froggerstiga-v1`, verify release assets on GitHub
