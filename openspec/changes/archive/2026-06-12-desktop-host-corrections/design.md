## Context

FroggersTiga engine external path:

```
input sample → limiter → envelope → Schmidt gate → hasExternal?
  false → OLVL × osc mix (3 VCOs)
  true  → ring-mod (FUEG blends product / parallel)
```

Desktop sim added `ExternalInputMode::Off` defaulting to zeroing `m_inBlock` — redundant with the gate and blocking line input until a second toggle. Web Mic/External Off remains valid (permission + no capture until opt-in).

Mod bus (`m_mods[0,4,5,6]` in sim UI) has no sine LFO sources. Reverb LFO (`RMOD`/`RRAT`) and delay LFO (`DMOD`) are effect-internal. Marbles are manual-step random CV (`B5` / **Marbles** strip button).

Wave/morph UI writes `m_vcoMorph[i].m_knobValue` on the message thread while audio reads every sample → torn float / NaN → NaN in reverb state → silence until app restart. `audioDeviceStopped()` is empty → `m_audioRunning` stuck true → Play disabled.

## Goals / Non-Goals

**Goals:**

- Desktop external audio: route input when device configured; gate decides mix.
- Morph changes never read/write shared floats across threads unsafely.
- Play/Stop always recoverable after CoreAudio stop or NaN event.
- Mod rack and wave UI self-explanatory.
- Retain `desktop-sim-ux-polish` cable/knob/44100 fixes.

**Non-Goals:**

- New patchable LFO mod sources (v2; document gap in v1).
- Firmware OLED rename (`V1VO` stays on hardware).
- Web External/Mic toggle removal.
- Resampler if device refuses 44100 (log warning only, same as polish).

## Decisions

### 1. Desktop external audio — gate only

**Decision:** Delete `ExternalInputMode`, ring-mod toggle, and meter-driven gating. `AudioEngine` always copies `inputChannelData[0]` when `numInputChannels > 0`. Optional read-only **In** meter in transport (dim when envelope below gate threshold).

**Rationale:** User selects input in Audio Settings; unplugging = silence = gate low = VCO path. Matches MANUAL OLVL behavior.

**Web unchanged:** `External: Off` still zeros WASM input until user enables.

### 2. Morph command queue

**Decision:** `DesktopHostIO` holds a small lock-free queue (or atomic pending morph commands) drained at start of `tickControls()`:

| Command | Action |
|---------|--------|
| `NudgeMorph(i, delta)` | from wave button |
| `SetMorph(i, value)` | if needed |
| `RandomizeAllMorphs()` | from Randomize waves |

UI buttons enqueue; audio thread applies before `ProcessBlock`.

**Alternative rejected:** `std::mutex` around engine — risks audio thread blocking.

### 3. NaN hardening

**Decision:** In `ModulatedMorph` / `EvalWaveMorph` entry, if `!std::isfinite(morph)` use `0.f`. On Stop, if last block peak was non-finite, call `engine` soft reset (re-init delay/reverb lines) — minimal scope.

### 4. Transport

**Decision:**

```cpp
void audioDeviceStopped() {
    m_audioRunning = false;
    notifyTransportChanged();  // MainComponent updates Play/Stop
}
void audioDeviceError(const String& msg) {
    log; m_audioRunning = false; notifyTransportChanged();
}
```

`startAudio()` remains idempotent: if `!m_audioRunning`, add callback.

### 5. Labels

| Old | New (desktop UI) |
|-----|------------------|
| VCO feat | VCO level |
| V1VO / V2VO / V3VO | VCO1 / VCO2 / VCO3 |
| `...` wave button | Painted mini waveform icon (see §8) |

`DesktopPanelBackend::getRowName(page 0, row 0..2)` returns display aliases; core param names unchanged.

### 8. Wave button shows `...` — root causes and fix

**Observed:** Pill button beside VCO label displays JUCE default `...` instead of a waveform icon.

**Root causes (verified in `SubModulePanel.cpp`):**

| # | Cause | Evidence |
|---|--------|----------|
| 1 | **No initial label** | `TextButton` never gets `setButtonText` in ctor; only in `refresh()` at 15 Hz |
| 2 | **Button too narrow** | `kWaveButtonWidth = 18` px — JUCE truncates any text to `...` |
| 3 | **Wrong control type** | `TextButton` + ASCII `~` `^` `n` instead of drawn icon per `sim-hosts-multi-ui` §6b "wave icon" |
| 4 | **Weak glyphs** | `~`/`^`/`n` are not recognizable as sine/saw/square even when they render |

**Decision:** Replace `TextButton` with a small custom `WaveMorphButton` (`juce::Component`):

- Fixed size **28×28 px** beside label (widen label column if needed).
- `paint()` draws one of three mini paths (sine curve, saw tooth, square) from morph band: `<0.33` sine, `<0.66` saw, else square.
- `setMorph(float)` called in ctor and `refresh()` — no timer wait for first paint.
- `onClick` → enqueue `NudgeMorph` (§2 morph queue).
- Optional tooltip: "Cycle waveform: sine → saw → square".

**Web parity (same change, lower priority):** Replace `waveGlyph()` `~^n` in `main.ts` with inline SVG or Unicode (∿) at readable size; match three-band logic.

```
Row layout (audio VCO rows):
[ VCO1  28px ] [ ~icon~ ] [ knob ] [ mod-in ○ ]
                  ↑
            painted Path, not TextButton
```

### 6. Marbles / LFO expectations

**Decision:** Mod rack tooltips: "Marbles — random CV; press **Marbles** to step." No new mod outputs in v1. Add design note for v2: tap reverb LFO or dedicated sine LFO on `m_mods[7+]`.

### 7. Relationship to `desktop-sim-ux-polish`

Ring-mod toggle/meter tasks in polish are **superseded**. Polish tasks 1.x, 3.x, 5.x remain authoritative. Implement corrections in one apply pass after or merged with remaining polish tasks.

## Risks / Trade-offs

- **[Risk] Desktop input bleed** — line noise may flutter gate → use existing Schmidt hysteresis; no change.
- **[Risk] Morph queue one-block latency** — acceptable; 1–2 ms at 512 buffer.
- **[Trade-off] No LFO mod bus v1** — user expectation gap; document loudly in UI.

## Migration Plan

1. Audio transport + morph queue (fixes silence bug).
2. Remove ring-mod toggle.
3. Labels + wave buttons.
4. Update `sim-hosts-multi-ui` delta specs in this change folder for archive merge.

## Open Questions

- None blocking v1. v2 LFO mod bus is a separate proposal if requested.
