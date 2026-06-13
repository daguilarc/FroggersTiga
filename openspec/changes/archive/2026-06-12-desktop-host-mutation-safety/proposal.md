## Why

Desktop sim host UI actions poison DSP when they mutate shared engine state from the message thread while audio reads the same floats every sample. Morph randomize was partially queued (`desktop-host-corrections`), but **Randomize mod**, **Randomize all**, and **per-panel randomize** still write `PageManager` / `DelayState` directly — producing torn floats, NaN in reverb/delay lines, and permanent silence until Stop (and sometimes not even then). In parallel, patch-bay specs contradicted VCV UX (empty-input no-op), `RandomizeMod` assigns hardware-only indices 1–3 (ghost routes), and Delay **Randomize mod** randomizes depths only. These are one failure class: **host mutations must be serialized on the audio thread** and **sim randomize must use one valid source set**.

**Supersedes:** `desktop-patchbay-vcv` (patch + randomize scope absorbed here). Remaining gaps from `desktop-host-corrections` (NaN in-play recovery, non-morph randomize safety) close here.

## What Changes

- **Unified host mutation queue** (`DesktopHostIO`): morph, randomize params, randomize mod, patch `SetPageModSource`, Delay `setModSource`/`setModDepth` — UI enqueues; `tickControls()` drains once before `ProcessBlock`.
- **Sim-valid randomize** — `RandomizeModSim()`: P(none)=0.5, else uniform among `{0, 4, 5, 6}`. Same picker for `DelayState::randomizeMod()` (sources + depths). Firmware keeps full `RandomizeMod`.
- **Bidirectional VCV patch cables** (already in source): confirm manual scenarios; remove erroneous empty-input no-op from archived `sim-mod-patchbay` delta.
- **Sanitize legacy ghost routes** on desktop `Init()`: clear stored mod indices `{1, 2, 3}` on core pages and Delay rows.
- **NaN / FX recovery** — detect non-finite output in audio callback → `SoftResetFxState()` + `DelayState::softResetFx()` on the audio thread; also on `stopAudio`, `audioDeviceStopped`, and `audioDeviceError`.
- **WASM parity** — `PagedHostIO` / bindings use `RandomizeModSim` path (worklet is already audio-thread; no desktop-style queue needed for web, but same sim-valid set).
- **Manual test alignment** — update `stereo-delay-page` §E.4/E.6 for sources+depths; add empty-input→output patch scenarios.

## Capabilities

### New Capabilities

- `desktop-host-mutation-queue`: Single lock-free queue draining all UI-initiated engine mutations before DSP.
- `desktop-dsp-recovery`: Non-finite block detection and FX soft-reset without requiring app restart.

### Modified Capabilities

- `sim-mod-patchbay`: Bidirectional VCV drag; sim-valid randomize; ghost-route sanitize/reject; output highlight when dragging from input; remove empty-input no-op scenario.
- `host-ui-delay-page`: Delay randomize mod assigns sources + depths; patch parity with core panels.
- `desktop-audio-thread-safety`: Extend morph-only queue requirement to all randomize and mod-assignment mutations from desktop UI.

## Impact

- `src/core/DesktopHostIO.hpp` — mutation queue, drain in `tickControls`, `RandomizeModSim` wiring
- `src/core/Parameter.hpp`, `src/core/SimModSource.hpp` — sim picker helper
- `src/core/Page.hpp` — optional `RandomizePageModSim` entry points
- `sim/DelayState.hpp` — `randomizeMod()` sources + depths; `sanitizeModSources()`
- `desktop/Source/PatchCableOverlay.{h,cpp}` — verify bidirectional drag (mostly done)
- `desktop/Source/AudioEngine.cpp` — in-play NaN recovery; device stop/error soft-reset
- `desktop/Source/GlobalStrip.cpp`, `PanelBackend.hpp` — enqueue randomize instead of direct calls
- `wasm/bindings.cpp`, `src/core/PagedHostIO.hpp` — sim randomize path
- `openspec/changes/desktop-patchbay-vcv/` — superseded (do not implement separately)
- `openspec/changes/stereo-delay-page/tasks.md` — §E.4/E.6 expectation updates at archive
