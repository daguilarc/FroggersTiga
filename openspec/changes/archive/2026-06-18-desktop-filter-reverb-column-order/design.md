## Context

Host page indices are stable across sim surfaces (`ParamDisplayNames::forHostPage`): 2=Reverb, 3=Filter, 4=Drive. `FroggersEngine::ApplyOutputFx` processes output as:

```text
pure delay → comb → resonant bump → (Delay sidecar insert) → reverb wet/dry
```

Filter-page knobs drive the first three stages; Reverb-page knobs drive the send. Desktop `MainComponent` currently places panels in index order (`m_panels[i]` at column `i`), so Reverb appears left of Filter despite filter stages preceding reverb in audio.

Hardware Field page order (SW1/SW2) also lists Reverb before Filter; this change targets **desktop column layout only** to prioritize signal-flow literacy over hardware page-number parity.

## Goals / Non-Goals

**Goals:**

- Swap visual column positions of host pages 2 and 3 in desktop standalone.
- Preserve `SubModulePanel` `pageIndex` arguments (2 still binds Reverb params, 3 still binds Filter params).
- Keep patch overlay jack positions consistent with swapped panel bounds.

**Non-Goals:**

- Renumbering host pages globally (web pills, WASM, VST parameter inventory, presets).
- Changing DSP order in `FroggersEngine`.
- Reordering Drive relative to Filter/Reverb (Drive remains rightmost of the five core columns; full FX signal is Drive → Filter → Reverb, so a complete left-to-right signal mirror would also move Drive — out of scope).
- VCV panel field order.

## Decisions

### D1: Visual swap via layout map, not `ParamDisplayNames` reorder

**Choice:** Introduce a `constexpr` column→`pageIndex` map in `MainComponent` (or `DesktopChromeLayout.hpp`) used only in `resized()` when assigning `setBounds`.

**Rationale:** `ParamDisplayNames`, web `HOST_PAGE_NAMES`, host parameter stable IDs, and saved state all key off page index 2=Reverb / 3=Filter. Permuting display names in shared tables would break cross-host parity and preset semantics.

**Alternative rejected:** Swap entries in `ParamDisplayNames::forHostPage` — would mislabel web/VCV and invert documentation without moving DSP.

### D2: Panel array stays indexed by page

**Choice:** Keep `m_panels[page]` ownership; only change which column rectangle each panel receives in `resized()`.

**Rationale:** `refresh()`, patch overlay, and randomize handlers iterate by page index. Swapping array slots would scatter page-index lookups.

### D3: Column order constant

**Choice:** Document target left-to-right page order as `{0, 1, 3, 2, 4}` for the five visible core columns (Audio, Random, **Filter**, **Reverb**, Drive); Delay overlay unchanged at index 5.

## Risks / Trade-offs

- **[Risk] Patch cable overlay misaligned** → Re-run overlay bounds collection after layout; verify pair-AR and page-row jacks on pages 2 and 3.
- **[Risk] User expects hardware page order** → Note in SIM_MANUAL that desktop columns optimize for signal flow; Field page order unchanged.
- **[Trade-off] Drive still right of Reverb** → Partial signal-order mirror only; acceptable per scoped goal (filter↔reverb inversion fix).

## Migration Plan

No state migration. Ship in next desktop release; web sim unaffected.

## Open Questions

None — DSP order verified in `FroggersEngine::ApplyOutputFx` (filter stages before `ProcessReverb`).
