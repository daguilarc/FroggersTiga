## Context

**Current audio path** (`AudioEngine::audioDeviceIOCallbackWithContext`):

```text
tickControls → ProcessBlock (mono) → applyStereoBus → outL / outR
```

Stereo bus matches what the user hears (delay width on L/R). Sample rate is fixed at **44100 Hz** (`kSimSampleRate`). No recording tap exists today. Top bar order: Play, Stop, External, envelope, Audio, MIDI.

**JUCE stack:** `juce_audio_devices` + `juce_audio_utils` today. Export needs `juce_audio_formats` (`WavAudioFormat`, `OggVorbisAudioFormat`, `FlacAudioFormat`, `LameEncoderAudioFormat`).

## Goals / Non-Goals

**Goals:**

- User selects format (WAV default), clicks Record, plays sim, clicks Record again, names file, gets exported stereo file.
- Capture is the same stereo signal sent to the device (post-`applyStereoBus`).
- UI layout: transport row `Play | Stop | … | MIDI | Audio | [● RECORD + vertical formats]` with the record cluster **right-aligned** in the header (formats stacked below Record).

**Non-Goals:**

- Web browser download/export.
- Multitrack stems, input+output mix, or post-export editing.
- Recording when Play is stopped (no silent file generation).
- Sample-rate conversion beyond the existing 44100 sim rate.

## Decisions

### 1. Toggle record on transport bar (two-click flow)

```
Idle ──(click Record)──▶ Capturing ──(click Record)──▶ Save dialog ──▶ Idle
```

- First click: `startRecording()` if `isAudioRunning()`; otherwise show brief status (tooltip or alert: “Press Play first”).
- Second click: `stopRecording()` → async `FileChooser` → encode buffer → write file.
- Record button shows active state (filled red / “REC”) while capturing.

### 2. Exclusive format toggles (checkbox appearance)

Four `juce::ToggleButton`s in a `juce::ButtonGroup` (radio behavior): WAV, MP3, FLAC, OGG. Default **WAV** on launch. Only one selected; changing format while idle updates the export encoder used on the next stop.

### 3. Stereo capture in audio callback

Add `AudioRecorder` (or inline state on `AudioEngine`):

```text
audio thread (callback):
  if recording:
    append outL[i], outR[i] interleaved or planar into growable buffer
```

- Pre-size buffer capacity; grow in chunks if user records long takes (double capacity when full).
- Audio thread only **appends** — no file I/O on realtime thread.
- Max duration guard (e.g. 30 min) to avoid unbounded RAM; stop with user message if exceeded.

**Rationale:** OMNI accumulate-then-apply — collect samples in callback, encode once on stop on message thread.

### 4. Encode on stop via JUCE `AudioFormatWriter`

On message thread after `FileChooser`:

| Format | JUCE writer | Notes |
|--------|-------------|-------|
| WAV | `WavAudioFormat` | Always available |
| FLAC | `FlacAudioFormat` | Requires FLAC compile flag |
| OGG | `OggVorbisAudioFormat` | JUCE-bundled Vorbis |
| MP3 | `LameEncoderAudioFormat` | Requires LAME; document build dep |

`AudioFormatWriter::createWriterFor(file, sampleRate, numChannels, bitsPerSample, metadata, quality)` writes interleaved `AudioBuffer<float>` built from captured L/R.

If selected encoder is unavailable at runtime, show error dialog naming the format and keep captured buffer so user can pick WAV and retry.

### 5. `RecordExportCluster` component

`RecordButton` row: red circle + **RECORD** label (~28px tall). Below it, four exclusive format `ToggleButton`s stacked vertically (WAV, MP3, FLAC, OGG). Cluster width ~108px, placed in the header’s **right** column via `removeFromRight`.

### 6. Top-bar layout (`MainComponent::resized`)

**Target (v2 — fixes dead zone + clip):**

```text
row 1 (32px): Play | Stop | External | In env | … | MIDI | Audio | ● RECORD
row 2 (72px): mod rack (left, full remaining width)     | ☐ WAV
                                                         | ☐ MP3
                                                         | ☐ FLAC
                                                         | ☐ OGG
```

- **RECORD** circle + label sit in row 1, immediately right of **Audio**.
- Format toggles stack in the **right column** of row 2, beside the mod rack — not in a 100px header with 68px empty space on the left.
- Cluster width **120px**; format rows divide remaining height evenly (no 104px-in-100px clip).

**v1 bug (current code):** 100px header reserves right column for the full cluster while transport uses only the top 32px on the left → 68px dead band; format stack overflows cluster by ~4px; `ToggleButton` reads as radio pills, not checkboxes.

### 7. Format toggle chrome

Exclusive selection stays radio-group semantics. Visual: checkbox tick boxes (JUCE `ToggleButton` tick style or equivalent), one column, labels `WAV` / `MP3` / `FLAC` / `OGG`.

## Risks / Trade-offs

- **[Risk] MP3/LAME not linked in all builds** → Mitigation: runtime format availability check; WAV always works; README build note for LAME.
- **[Risk] Long recordings exhaust RAM** → Mitigation: max-duration cap with auto-stop.
- **[Risk] File write blocks UI** → Mitigation: show modal progress or disable Record until write completes; typical takes are short.
- **[Risk] Recording without Play** → Mitigation: refuse start with clear message.

## Migration Plan

1. Add `juce_audio_formats` to CMake.
2. Implement `AudioRecorder` + `AudioEngine` recording API.
3. Add `RecordButton` + format toggles + layout.
4. Manual test: WAV export, stereo content, MP3/FLAC/OGG on supported builds.

## Open Questions

- None blocking. MP3 requires LAME at build time — accept optional dependency with WAV fallback.
