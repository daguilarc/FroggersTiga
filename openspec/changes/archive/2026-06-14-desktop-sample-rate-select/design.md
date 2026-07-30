## Context

```
Today (standalone)                    Target
─────────────────                     ──────
kSimSampleRate = 44100 everywhere     HostAudioConfig.hpp authority
No rate UI                            Combo: 44100 | 48000
audioDeviceAboutToStart → 44100       → device.getCurrentSampleRate()
writeCaptureToFile → 44100            → m_hostSampleRate
VST prepareToPlay → host rate ✓       unchanged
Web → audioContext.sampleRate ✓       unchanged
```

## OMNI audit

| Finding | Severity | Fix |
|---------|----------|-----|
| `kSimSampleRate` in AudioEngine, AudioSettings, export, silent-detect | **Violation** | `HostAudioConfig.hpp` single table |
| `audioDeviceAboutToStart` ignores device rate | **Bug** | Sync DSP to opened device rate |
| `AudioRecorder` max sized for 44100 only | **Risk at 48k** | Size for 48000×30min |
| No sample-rate UI | **Missing feature** | Combo in Audio Settings |
| VST already uses `setHostSampleRate` | **OK** | Plugin path unchanged |
| Web follows `audioContext.sampleRate` | **OK** | Out of scope |

**Repetition:** rate list appears once in `HostAudioConfig`; UI and engine read it.

## Decisions

### D1 — Supported rates: 44100 and 48000 only

**Choice:** `constexpr double kSupportedRates[] = {44100.0, 48000.0}`; default 44100.

**Why:** Matches user request and common macOS default; engine already supports arbitrary `SetSampleRate`.

### D2 — DSP follows opened device rate

**Choice:** In `audioDeviceAboutToStart`, call `setHostSampleRate(device->getCurrentSampleRate())`. On rate change via settings, restart device with new `setup.sampleRate` then sync.

**Why:** Same contract as VST `prepareToPlay`; eliminates mismatch log spam.

### D3 — UI in Audio Settings only

**Choice:** Sample-rate `ComboBox` above output device; changing rate re-applies device setup preserving in/out selection.

**Why:** Minimal surface; no new dialog.

### D4 — Export uses active rate

**Choice:** `writeCaptureToFile` writes at `m_hostSampleRate`.

**Why:** Recorded buffer sample count matches real-time capture rate.

## Non-Goals

- Arbitrary rates (88.2, 96 kHz)
- Web sim rate selector (browser sets `audioContext.sampleRate`)
- VCV/VST changes
