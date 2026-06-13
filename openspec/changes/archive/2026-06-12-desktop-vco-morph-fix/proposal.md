## Why

VCO wave controls appear dead: wave icons do not change on click, **Randomize VCO Waveform** has no audible or visible effect, and audio stays sine-shaped. Root cause is **`VcoWaveMorph::GetMorph` uses `ExpParam::Compute(0.0f, 1.0f, knob)`** — division-by-zero / `0 × ∞` → **NaN**, clamped to **0** in `ModulatedMorph`, so morph is always sine regardless of knob. A second bug: **`CycleVcoMorph` bypasses the mutation queue** (`desktop-host-corrections` task 2.2 marked done but not implemented), causing message-thread writes and requiring **Play** for queued **Randomize** to drain.

## What Changes

- **Linear morph read** — `GetMorph` returns modulated knob clamped 0–1 (no `ExpParam` with `min=0`).
- **Queue all morph UI** — Add `CycleMorph` mutation; route `CycleVcoMorph` through `enqueueMutation`; wave button click enqueues cycle (not direct engine write).
- **Drain morph queue when idle** — `DrainPendingMutations()` from UI timer when audio stopped so **Rand waves** updates morph + icon without Play (audio still needs Play to hear).
- **Cycle semantics** — Keep discrete knob targets **0 → 0.5 → 1 → 0** mapping to sine / saw / square morph bands for click; randomize stays continuous **0–1**.
- **Icon refresh** — After morph mutation drains, `SubModulePanel::refresh` reads `GetVcoDisplayMorph` (now non-stuck-at-zero).

## Capabilities

### New Capabilities

- `desktop-vco-morph-fix`: Correct morph evaluation, queued cycle/randomize, idle drain.

### Spec deltas in this change

- `specs/froggers-core/spec.md` — MODIFIED morph requirement (linear, no `ExpParam(0,1)`).

### Modified Capabilities

- `desktop-wave-controls` (delta over `desktop-host-corrections`): Morph changes affect audio and icon; cycle uses queue.
- `froggers-core` (delta over `sim-hosts-multi-ui`): Remove broken `ExpParam(0,1)` morph mapping; linear 0–1 read.

## Impact

- `src/core/VcoWaveMorph.hpp` — fix `GetMorph`
- `src/core/DesktopHostIO.hpp` — `CycleMorph` mutation type; queue `CycleVcoMorph`; public `DrainPendingMutations()`
- `desktop/Source/MainComponent.cpp` — `DrainPendingMutations()` in `timerCallback` when !audioRunning
- `openspec/changes/desktop-host-corrections/tasks.md` — correct falsely checked task 2.2
- `openspec/changes/desktop-host-mutation-safety/tasks.md` — correct falsely checked task 1.4
- `openspec/changes/sim-hosts-multi-ui/specs/froggers-core/spec.md` — remove exponential morph requirement (superseded)

**Follow-up (out of scope):** `PagedHostIO::CycleVcoMorph` and web wave tap use the same core `GetMorph`; WASM host still calls engine directly on cycle — fix in a later web parity pass.
