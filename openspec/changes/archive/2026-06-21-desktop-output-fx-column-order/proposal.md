## Why

The archived `desktop-filter-reverb-column-order` change swapped Filter and Reverb columns (`{0, 1, 3, 2, 4}`) but left **Drive** rightmost. The engine runs output FX as **Drive → Filter → Reverb** (`FrogBlock` polynomial/SRR/destruction before `ApplyOutputFx` filter stages, then reverb wet/dry). Desktop columns still read Random → Filter → Reverb → **Drive**, which inverts the first stage of the output chain. Permuting Drive left of Filter completes the left-to-right signal mirror for pages 4→3→2 without touching DSP or host page indices.

## What Changes

- Update `kDesktopCoreColumnPageOrder` from `{0, 1, 3, 2, 4}` to `{0, 1, 4, 3, 2}` (Audio, Random S&H, **Drive**, **Filter**, **Reverb**).
- Keep **host page indices**, parameter IDs, patch-cable routing, presets, WASM/web page order, and hardware Field page order unchanged.
- Update `SIM_MANUAL.md` desktop host guide: output FX columns now read Drive → Filter → Reverb left-to-right; Field hardware page order unchanged.
- Extend verification to page 4 (Drive) patch jacks and GAIN knob binding after the column move.

## Capabilities

### New Capabilities

_(none — extends existing desktop column-order capability)_

### Modified Capabilities

- `desktop-host-panel-column-order`: Require full output FX segment order Drive → Filter → Reverb (pages 4 → 3 → 2), not filter↔reverb swap only.
- `froggers-host-master`: Clarify desktop MAY permute horizontal positions of pages 2, 3, and 4 to match output FX signal flow.

## Impact

- `desktop/Source/DesktopChromeLayout.hpp` (`kDesktopCoreColumnPageOrder` constant and comment)
- `SIM_MANUAL.md` and synced `web/public/sim-manual.md` (desktop column order note)
- No changes to `MainComponent::resized()` loop structure (already map-driven), `FroggersEngine.hpp`, `ParamDisplayNames.hpp`, web `HOST_PAGE_NAMES`, VST/AU, VCV, or preset/state compatibility
