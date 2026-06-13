# Desktop audio export — tasks

## 1. Build dependencies

- [x] 1.1 Add `juce::juce_audio_formats` to `desktop/CMakeLists.txt`
- [x] 1.2 Enable FLAC / OGG / LAME in JUCE config where available; document optional MP3 build dep in README

## 2. Audio capture

- [x] 2.1 Add `AudioRecorder` (stereo float buffer, grow-on-full, max-duration guard)
- [x] 2.2 `AudioEngine`: `startRecording()`, `stopRecording()`, `isRecording()`, `wasRecordingSuccessful()` API
- [x] 2.3 Tap `outL`/`outR` post-`applyStereoBus` in `audioDeviceIOCallbackWithContext` when recording
- [x] 2.4 Refuse `startRecording()` when `!isAudioRunning()` with user-visible feedback

## 3. Encode and save

- [x] 3.1 Map format enum → `AudioFormat` (`Wav`, `Flac`, `Ogg`, `Lame`)
- [x] 3.2 On stop: build `AudioBuffer<float>` from capture, `AudioFormatWriter::createWriterFor`, write file
- [x] 3.3 Async `FileChooser` with correct extension filter per format
- [x] 3.4 Runtime encoder-missing error dialog; preserve buffer for WAV retry

## 4. Transport UI

- [x] 4.1 Add `RecordButton` — round red idle/active states
- [x] 4.2 Add exclusive format toggles (WAV, MP3, FLAC, OGG); WAV default; `ButtonGroup` radio behavior
- [x] 4.3 `RecordExportCluster`: vertical formats below Record; cluster right of MIDI/Audio in `MainComponent::resized`
- [x] 4.4 Wire Record click → start/stop + save flow

## 5. Transport layout polish (v1 gaps)

- [x] 5.1 Reflow `MainComponent::resized`: row 1 = transport + RECORD; row 2 = mod rack + vertical format column (eliminate 68px left dead zone) — absorbed by `desktop-chrome-cohesion`
- [x] 5.2 `RecordExportCluster`: fit format rows in allocated height (no bottom clip); cluster width 120px — absorbed by `desktop-chrome-cohesion`
- [x] 5.3 Format toggles: checkbox tick chrome with radio-group exclusivity — absorbed by `desktop-chrome-cohesion`

## 6. Verification

- [ ] 6.1 Play → Record → Play sim → Record → save WAV → file plays back in external player
- [ ] 6.2 Verify stereo width present in exported file when delay width active
- [ ] 6.3 Record without Play shows feedback, no file
- [ ] 6.4 FLAC/OGG/MP3 on builds with encoders linked
- [ ] 6.5 Visual: RECORD label not clipped; MIDI | Audio | RECORD order; checkboxes vertical under RECORD
- [x] 6.6 Desktop Release build
