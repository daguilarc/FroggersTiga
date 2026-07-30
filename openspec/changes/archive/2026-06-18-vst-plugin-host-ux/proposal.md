> **Superseded by `omni-repository-harmonization`.** Valid hosted-UX findings are absorbed into omni §5.6–5.7; do not implement this plan as written. Archive with `--skip-specs`.

## Why

The VST/AU plugin shares `MainComponent` with standalone desktop — including `PatchCableOverlay`, Randmod, and the five-cell mod rack. **Field-parity visuals already exist.** What blocks a *functional* plugin is hosted-policy gaps (chrome, keyboard, presets), a **mutation-drain bug when the DAW transport is stopped**, and **no overlay resync after preset load / Randmod** so cables can disagree with `modIndex` state.

VCV Rack will implement the same *user-facing* mod-randomize behavior with native `Cable` objects (see `vcv-rack-field-parity`). **VST is the reference host** for that behavior: closed UI, mod rack → knob mod inputs, Randmod shuffles routes and cables redraw. This change closes VST hosting gaps so VCV can A/B against a working plugin.

**OMNI audit (2026-06-15 — code review):**

| Issue | Evidence | Impact |
|-------|----------|--------|
| **Mutations stall when DAW stopped** | `MainComponent::timerCallback` calls `DrainPendingMutations` only when `!isAudioRunning()`; hosted sets `m_audioRunning = true` permanently | Randmod / Randomize / morph buttons no-op when DAW transport is stopped |
| **Preset reload desyncs cables** | `SimPresetSnapshot::read` writes `modIndex` directly; `PatchCableOverlay::m_cableHues` untouched | DAW reload: DSP routes correct, cable colors/positions stale until user repatches |
| **Record/Export visible in DAW** | `initFromEngine` hides Play/Stop/Audio; `m_recordCluster` not hidden | Misleading standalone transport UI |
| **QWERTY MIDI in DAW** | No `isPluginHosted()` guard in `shouldCaptureQwertyMidi()` | DAW typing can drive CC1 when Computer Keyboard enabled |
| **Snapshot v3 incomplete** | v2 has pair-AR rows; morph + CC config still not serialized | DAW reload loses morph indices and CC routing |
| **Editor min size drift** | `setResizeLimits(1024, 600, …)` vs 1440×720 | Mod rack scopes clip at minimum |

**Confirmed already passing:**

| Check | VST |
|-------|-----|
| `PatchCableOverlay` + drag patch | ✓ shared `MainComponent` |
| Randmod → `EnqueueRandomizePanelMod` | ✓ same backend as standalone |
| Mod routes in snapshot body | ✓ `RowState.modIndex` per page row |
| DAW MIDI ingest | ✓ `processHostedBlock` → `ingestMidiMessage` |
| `tickControls` → `drainMutationQueue` on audio thread | ✓ when `processBlock` runs |

## What Changes

- **Always drain UI mutations when hosted** — `DrainPendingMutations` on editor timer regardless of `m_audioRunning`, or equivalent single path in `tickControls` + timer (no duplicate apply)
- **Resync `PatchCableOverlay` after mod-affecting state changes** — preset `setStateInformation`, Randmod / Rand Mods / manual patch: repaint from `modIndex`; assign stable hue per connection when route appears
- **Hide Record/Export cluster** when `isPluginHosted()`
- **Disable QWERTY MIDI when hosted** in `shouldCaptureQwertyMidi()`
- **Extend `SimPresetSnapshot` to v3** — `vcoMorph[3]`, `ccEnabled[2]`, `ccChannel[2]`, `ccNumber[2]`; `read()` accepts v1/v2 with defaults
- **Align plugin editor minimum size** to 1440×720
- **Document cross-host mod-routing model** — VST = desktop closed patch graph; VCV = Rack cables (separate change); web = mod bay dropdowns
- **Manual DAW verification gate** — load, Randmod cable reroute, preset recall, A/B audio

## Capabilities

### New Capabilities

- `vst-plugin-host-ux`: Hosted chrome, keyboard policy, snapshot v3, editor sizing, DAW verification
- `vst-mod-routing-parity`: Mutation drain when transport stopped, overlay resync after preset/Randmod, parity matrix vs desktop/VCV

### Modified Capabilities

- (none on public `openspec/specs/` — VST sources local-only)

## Impact

- `desktop/Source/MainComponent.cpp` — hosted chrome, mutation drain policy, overlay resync hook
- `desktop/Source/PatchCableOverlay.cpp` — optional `syncFromHost()` to rebuild hues from `modIndex`
- `desktop/Source/PluginProcessor.cpp` — `setStateInformation` notifies editor to resync overlay
- `sim/SimPresetSnapshot.hpp` — v3 fields; v1/v2 read paths
- `desktop/Source/PluginEditor.cpp` — resize limits; preset-load callback
- `SIM_MANUAL.md`, synced copies — VST plugin-hosted § + mod-routing parity matrix
- **Reference for:** `vcv-rack-field-parity` Randmod + Rack `Cable` spawn (VST proves DSP + UX intent)

## Relationship to other changes

- **Spun off from:** `vcv-rack-field-parity` § VST audit
- **VCV reciprocal:** VCV Randmod spawns Rack cables for mod indices 4–6; VST uses `PatchCableOverlay` redraw; same `RandomizePageMod` engine
- **Does not implement:** VCV `addCable` plumbing (separate change)
