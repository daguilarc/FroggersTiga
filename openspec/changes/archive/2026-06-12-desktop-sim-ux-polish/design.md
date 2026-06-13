## Context

Post-`stereo-delay-page`, the desktop app shows six adjacent panels (2016×720, ~336 px each), a four-box mod rack, and a full-window `PatchCableOverlay`. The original `sim-hosts-multi-ui` design chose vertical sliders for speed of implementation and assumed five panels (~403 px) where label truncation was "fixed" by removing mod dropdowns. Adding a sixth Delay panel and shipping without enforcing 44.1 kHz exposed gaps between spec intent and runtime behavior.

### Root causes (verified in code)

| Symptom | Cause |
|---------|--------|
| Wrong sample rate / delay time off | `audioDeviceAboutToStart` sets `SetSampleRate(device->getCurrentSampleRate())` — typically 48000 on macOS. Delay init uses 44100 in ctor but is overwritten on Play. |
| "Input bars never fill" | `ModModuleBox` meters bind to `GetCvOut(modIndex)` — **mod bus CV** (MIDI CC, VCO feat, Marbles). No UI shows **external ring-mod audio** envelope. Values stay 0 until Play + active mod sources. User drags from meter thinking it is a jack. |
| Patch cables "don't work" | 14 px hit radius on 14 px drawn jacks; ports synced only in `resized()`; no hover affordance; user may click painted jacks on `SubModulePanel` below overlay without hitting overlay `hitTest`; six narrow panels compress jack spacing. |
| V1VO / wave labels cut off | Row layout: 56 px label column minus 22 px wave button → ~34 px for text; font ellipsis at 336 px panel width. |
| Tall useless sliders | `LinearVertical` consumes remaining row width; knobs would use fixed ~40 px and free label space. |

## Goals / Non-Goals

**Goals:**

- Desktop sim DSP always at **44100 Hz** when audio runs.
- Clear **ring-mod input meter** vs **mod bus meters**.
- Patch cables work first-try: drag output jack → input jack on any panel.
- Submodule UI: compact **rotary knobs**, readable labels, lower row height.
- When a row has mod patched, knob displays **effective parameter value** while user is not dragging (visual modulation).

**Non-Goals:**

- Web UI changes (except documenting 44100 parity if needed later).
- Firmware / VCV / sixth WASM page.
- Replacing JUCE sliders with custom skeuomorphic graphics beyond `RotaryHorizontalVerticalDrag` or `Rotary` style.
- Audio input device picker UX overhaul (existing Audio Settings dialog stays).

## Decisions

### 1. Sample rate: force 44100 at device open

**Decision:** After `initialiseWithDefaultDevices`, call `AudioDeviceManager::setAudioDeviceSetup` with `sampleRate = 44100.0` and `bufferSize` unchanged. In `audioDeviceAboutToStart`, always `m_host.SetSampleRate(44100.f)` and `m_delay.setSampleRate(44100.f)` — ignore device-reported rate for DSP (JUCE still runs callback at device rate; if mismatch, add lightweight linear resampler in a follow-up — **v1 assumes device accepts 44100**, which is universal on macOS/Windows).

**Alternative rejected:** Resample I/O always — heavier; unnecessary if we request 44100 from device manager.

**Alternative rejected:** Keep device rate — violates `sim-hosts-multi-ui` and breaks delay time calibration vs spec.

### 2. Ring-mod meter: transport bar, not mod rack

**Decision:** Add `juce::Slider` horizontal meter next to **Ring mod in: On/Off** toggle. `refresh()` at 15 Hz sets value from `m_host.m_engine.GetEnvelopeLevel()` when ring mod is On and audio running; dim when Off.

**Alternative rejected:** Fifth mod-rack box — conflates CV patching with audio input again.

### 3. Mod rack labels

**Decision:** Subtitle under each box title: **"Mod out"**; tooltip: "CV level while audio is playing — drag from jack below to patch."

### 4. Patch cables

**Decision:**

- Increase `kPortHitRadius` to **22 px** (visual jack stays 14 px, hit area larger).
- Call `syncPatchPorts()` from `timerCallback()` every frame (cheap — 48 ports max).
- Draw jack hover ring in overlay when cursor within hit radius.
- On successful `setModSource`, call `panel->refresh()` for affected page so slider/knob mode updates immediately.

**Bug check:** `PatchCableOverlay::hitTest` returns false outside jacks — clicks pass to sliders. Jacks on `SubModulePanel` are decorative only; overlay owns interaction. Confirm `toFront(false)` after every resize.

### 5. Rotary knobs

**Decision:** Replace `LinearVertical` with `juce::Slider::RotaryHorizontalVerticalDrag`, `textBoxBelow = false`, diameter **38 px**, placed in fixed-width column right of jack. Row layout:

```
[ label 72px | wave 20px ] [ jack 18px ] [ knob 44px ] [ value readout optional 48px ]
```

Remove value readout if cramped — knob position is sufficient.

**Mod display:** Add `DesktopHostIO::GetEffectiveKnob(page, row)` reading post-mod value from `PageManager` / engine stored knob after last `ProcessBlock` (expose last computed target from `RuntimeParam` or page stored value + mod application — implement minimal read API in core).

When `mod != 255` and not dragging: knob shows effective value. When user starts drag: switch to mod-depth editing (current behavior).

### 6. Panel width / window size

**Decision (superseded by `desktop-compact-layout`):** Default was 2016×720 (336 px/panel). New default **1680×720** (~280 px/panel). Intrinsic-width Randmod/Randomize buttons reduce wasted chrome; wave rows still fit at 280 px.

**Original (obsolete):** Keep 2016 width — withdrawn.

## Risks / Trade-offs

- **[Risk] Device refuses 44100** → Show alert in Audio Settings path listing actual vs requested rate; log warning.
- **[Risk] Effective knob read API touches core** → Keep read-only accessor; no DSP behavior change.
- **[Risk] Timer syncPatchPorts every frame** → 48 bounds updates at 15 Hz is negligible vs repaint already happening.
- **[Trade-off] Knob shows effective value when patched** → Differs from Field hardware (attenuator position); matches user expectation for "knob moves with modulation."

## Migration Plan

1. Land sample-rate fix first — validates delay timing immediately.
2. Ring-mod meter + mod rack labels — clarifies UX before patch cable retest.
3. Patch cable hit targets + sync.
4. Knob + layout refactor per panel.
5. Manual test checklist (tasks §6).

No data migration. User rebuilds desktop app.

## Open Questions

- None blocking v1. Resampler fallback if 44100 open fails on exotic hardware → defer to task note.
