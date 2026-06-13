## Why

Desktop sim users can hear output through Play but cannot save performances.

**Implementation status:** Code landed (`RecordExportCluster`, `AudioRecorder`, `juce_audio_formats`). Manual verification tasks remain open.

## What Changes

- **Record control** — red circle + **RECORD** label in a vertical cluster on the **right** of the transport bar (right of **MIDI** and **Audio**); toggles record on first click, stops and opens save dialog on second click.
- **Format selectors** — exclusive format toggles (WAV, MP3, FLAC, OGG) stacked **vertically below** the Record row in the same cluster; **WAV** selected by default.
- **Stereo capture** — record post-`applyStereoBus` L/R at 44100 Hz while capture is active and audio is running.
- **Save dialog** — on stop, `FileChooser` prompts for file name and location; extension matches selected format.
- **Unchanged** — web sim (no file export), VCV, firmware, MIDI routing.

## Capabilities

### New Capabilities

- `desktop-audio-export`: Transport-bar record UI, format selection, stereo capture, and file export.

### Modified Capabilities

- (none)

## Impact

- `desktop/Source/AudioEngine.*` — recording state, stereo FIFO/buffer, encode-on-stop
- `desktop/Source/MainComponent.*` — format toggles, record button, top-bar layout
- `desktop/Source/RecordButton.*` — circle + **RECORD** label
- `desktop/Source/RecordExportCluster.*` — record row + vertical format toggles, right-aligned in header
- `desktop/CMakeLists.txt` — `juce::juce_audio_formats`; optional LAME/FLAC/OGG linkage per JUCE module config
