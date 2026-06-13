## Why

Desktop sim host UX diverged from engine truth and from `sim-hosts-multi-ui` intent: external audio is gated twice (host Off toggle + engine Schmidt gate), wave/morph controls poison DSP from the UI thread, Play/Stop does not recover after device stop, mod labels are opaque (`VCO feat`, `V1VO`, `...` buttons), and users expect patchable LFOs that never existed on the mod bus. `desktop-sim-ux-polish` partially fixed layout/cables/sample rate but encoded wrong ring-mod UX and left audio-safety gaps.

## What Changes

- **Remove desktop ring-mod On/Off toggle and meter-as-control** — always pass configured audio input to `ProcessBlock`; `m_extGate` + `OLVL` handle VCO-only vs ring-mod (firmware parity). Web keeps External/Mic toggle (browser permission).
- **Audio thread safety** — queue VCO morph writes (`cycleVcoMorph`, `RandomizeVcoMorphs`, `Randomize waves`) for application in `tickControls()` before `ProcessBlock`; clamp morph with `isfinite` guard in `EvalWaveMorph` path.
- **Transport recovery** — `audioDeviceStopped()` / `audioDeviceError()` clear `m_audioRunning` and refresh Play/Stop; optional DSP soft-reset on Stop if output was non-finite.
- **Mod rack labels** — rename **VCO feat** → **VCO level**; subtitle **Mod out**; document that Marbles are manual-clock random CV (not LFOs); reverb/delay LFOs remain effect-internal (non-goal: new LFO mod bus in v1).
- **Audio panel labels** — desktop shows **VCO1** / **VCO2** / **VCO3** (display override; firmware OLED names unchanged).
- **Wave icon buttons** — replace 18 px `TextButton` (`...`) with **28 px painted** sine/saw/square icon from morph band; set morph in ctor; tooltip "Cycle waveform".
- **Supersedes** `desktop-sim-ux-polish` ring-mod meter/toggle requirements — replace with passive input indicator only (optional).
- **Retains** completed `desktop-sim-ux-polish` work: 44100 Hz, thick random-hue cables, per-knob mod-in jacks, rotary knobs, patch hit targets.

## Capabilities

### New Capabilities

- `desktop-external-audio-routing`: Gate-based external input; no host Off switch on desktop.
- `desktop-audio-thread-safety`: Morph changes and transport recovery; no permanent silence from UI actions during Play.
- `desktop-mod-rack-labels`: Human mod source names and Marbles/LFO expectation documentation.
- `desktop-wave-controls`: Wave cycle buttons with readable labels and safe interaction during audio.

### Modified Capabilities

- `desktop-simulator` (delta over `sim-hosts-multi-ui`): Remove **External: Off | L | R** host gating; input device selection only.
- `sim-mod-patchbay` (delta over `sim-hosts-multi-ui`): Rename VCO feat → VCO level in all sim host UI strings.

## Impact

- `desktop/Source/AudioEngine.*` — remove `ExternalInputMode` gating; device stop/error handlers; morph command queue
- `desktop/Source/MainComponent.*` — remove ring-mod toggle/meter control strip
- `desktop/Source/GlobalStrip.cpp`, `SubModulePanel.*` — morph via queue; wave button UX
- `desktop/Source/ModRackPanel.cpp`, `ModModuleBox.*` — labels
- `desktop/Source/PanelBackend.hpp` — VCO1/2/3 display names on audio page
- `src/core/DesktopHostIO.hpp` or `FroggersEngine.hpp` — morph command drain in `tickControls`; `isfinite` on morph
- `openspec/changes/desktop-sim-ux-polish/*` — annotate superseded ring-mod tasks
- Web, firmware, WASM page count unchanged
