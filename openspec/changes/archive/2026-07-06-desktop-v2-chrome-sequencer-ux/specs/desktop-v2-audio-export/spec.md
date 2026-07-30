## ADDED Requirements

### Requirement: v2-audio-export-cluster-restored

Standalone desktop v2 SHALL place v1 `RecordButton` (round red circle + label **Record audio**) in the top transport row for **audio output to file** capture. **WAV / MP3 / FLAC / OGG** export format toggles SHALL appear in the **Audio** menu (`AudioSettingsComponent`, opened by the **Audio** button) — not in the transport row. This is distinct from sequencer **Write Seq.**

#### Scenario: Record audio chrome

- **WHEN** standalone desktop v2 renders the transport row
- **THEN** a round red record circle appears with the text **Record audio** immediately to its right (v1 `RecordButton` affordance; label updated from v1 **RECORD**)
- **THEN** clicking the control starts/stops audio file capture, not step-buffer write
- **THEN** no WAV / MP3 / FLAC / OGG toggles appear in the transport row

#### Scenario: Audio record starts after Play

- **WHEN** the operator presses **Record audio** while audio is not running
- **THEN** recording does not start and the operator is prompted to press **Play** first (same as v1)
- **WHEN** audio **Play** is active and the operator presses **Record audio**
- **THEN** `AudioEngine` captures output samples via `AudioRecorder`

#### Scenario: Audio record stops and exports

- **WHEN** the operator presses **Record audio** again while capturing
- **THEN** capture stops and `writeCaptureToFile` writes using the export format selected in the **Audio** menu
- **THEN** the file dialog or save path behavior matches v1 desktop

#### Scenario: Format row in Audio menu

- **WHEN** the operator opens the **Audio** menu
- **THEN** WAV / MP3 / FLAC / OGG toggles appear under an **Export format** label
- **THEN** selecting a format updates `AudioEngine` export preference used on the next export
- **THEN** no format toggle is labeled **Write Seq**

#### Scenario: VST omits audio export cluster

- **WHEN** FroggersTigaPluginV2 is hosted
- **THEN** audio export cluster is hidden (DAW owns bounce/export)

### Requirement: v2-audio-engine-recording-api

`desktop-v2/Source/AudioEngine` SHALL expose the v1 recording API (`startRecording`, `stopRecording`, `isRecording`, `writeCaptureToFile`, `hasCapturedAudio`), persist `ExportFormat` preference (`exportFormat` / `setExportFormat`), and feed `m_recorder` from the audio output path. Dead `m_recorder` member without wiring is not permitted after this change.

#### Scenario: Recorder fed during playback

- **WHEN** audio is running and recording is active
- **THEN** stereo output samples append to `AudioRecorder` each callback block

#### Scenario: No dead recorder member

- **WHEN** implementation is complete
- **THEN** `m_recorder` is started/stopped only through the public recording API

#### Scenario: Export format persisted on engine

- **WHEN** the operator selects FLAC in the **Audio** menu and later exports a capture
- **THEN** `writeCaptureToFile` receives `ExportFormat::Flac` from `AudioEngine::exportFormat()`
