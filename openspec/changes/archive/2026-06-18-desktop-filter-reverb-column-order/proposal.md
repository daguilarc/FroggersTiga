## Why

Desktop standalone lays out submodule columns left-to-right as Audio → Random S&H → **Reverb → Filter** → Drive, but the engine applies **Filter (pure delay, comb, resonant bump) before Reverb** in `FroggersEngine::ApplyOutputFx`. That UI order inverts the filter↔reverb segment of the output FX chain and contradicts `MANUAL.md` (“Filter — in series before reverb”). Reordering the two desktop columns to **Filter → Reverb** aligns visual layout with actual audio flow without changing DSP.

## What Changes

- Swap horizontal positions of the Filter (host page 3) and Reverb (host page 2) `SubModulePanel` columns in desktop standalone layout only.
- Keep **host page indices**, parameter IDs, patch-cable routing, presets, WASM/web page order, and hardware Field page order unchanged.
- Document in `SIM_MANUAL.md` that desktop column order now matches filter→reverb signal order while web remains paged (hardware page order preserved on Field).
- Add a regression check that panel `pageIndex` bindings remain 2=Reverb params and 3=Filter params after the visual swap.

## Capabilities

### New Capabilities

- `desktop-host-panel-column-order`: Desktop standalone horizontal submodule column order reflects output FX signal flow for Filter and Reverb while preserving stable host page indices.

### Modified Capabilities

- `froggers-host-master`: Clarify that host page index 2/3 labels are stable across hosts; desktop column **layout** may permute visual order without renumbering pages.

## Impact

- `desktop/Source/MainComponent.cpp` (panel creation order and/or layout `resized()` column positions)
- `desktop/Source/PatchCableOverlay.cpp` (jack screen bounds follow panel positions — verify after swap)
- `SIM_MANUAL.md` desktop host guide (column order note)
- No changes to `FroggersEngine.hpp`, `ParamDisplayNames.hpp` page table order, web `HOST_PAGE_NAMES`, VST/AU, VCV, or preset/state compatibility
