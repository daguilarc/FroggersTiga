## Context

Host page indices are stable (`ParamDisplayNames::forHostPage`): 2=Reverb, 3=Filter, 4=Drive. Output audio path in `FroggersEngine::ProcessSample`:

```text
osc mix → FrogBlock (Drive page: polynomial, SRR, digr, hash, fuzz)
       → ApplyOutputFx: pure delay → comb → resonant bump → (Delay insert) → reverb wet/dry
```

Desktop already uses a layout map (`kDesktopCoreColumnPageOrder`) from the archived filter↔reverb change. Current value `{0, 1, 3, 2, 4}` yields Audio | Random | Filter | Reverb | **Drive** — Drive still last among core FX columns despite being first in the chain.

Hardware Field page order (SW1/SW2) lists Reverb (3) before Filter (4) before Drive (5 in manual ordering); this change continues to target **desktop column layout only** for signal-flow literacy.

## Goals / Non-Goals

**Goals:**

- Set desktop core column order to `{0, 1, 4, 3, 2}` so Drive → Filter → Reverb reads left-to-right after Audio and Random S&H.
- Preserve `SubModulePanel` `pageIndex` bindings (4=Drive params, 3=Filter, 2=Reverb).
- Keep patch overlay jack positions aligned with permuted panel bounds.

**Non-Goals:**

- Renumbering host pages globally (web pills, WASM, VST parameter inventory, presets).
- Changing DSP order in `FroggersEngine`.
- Moving Delay (page 5) — remains the sixth column; its sidecar insert point in `ApplyOutputFx` is after filter stages, before reverb, and Delay has its own overlay column.
- VCV panel field order.

## Decisions

### D1: Extend existing layout map constant

**Choice:** Change `kDesktopCoreColumnPageOrder` from `{0, 1, 3, 2, 4}` to `{0, 1, 4, 3, 2}` in `DesktopChromeLayout.hpp`.

**Rationale:** One-line constant update; `MainComponent::resized()` already iterates the map. No structural code change.

**Alternative rejected:** Separate Drive-only swap logic in `resized()` — duplicates the map pattern and violates single-source column order.

### D2: Panel array stays indexed by page

**Choice:** Keep `m_panels[page]` ownership; only column rectangles permute.

**Rationale:** Unchanged from prior change; `refresh()`, patch overlay, and randomize handlers iterate by page index.

### D3: Delay column unchanged

**Choice:** Column 6 (index 5) still receives `m_panels[5]` with remaining width after five equal core columns.

**Rationale:** Delay is a distinct sidecar page, not part of the Drive/Filter/Reverb page trio; moving it would widen scope without improving the core FX literacy goal.

## Risks / Trade-offs

- **[Risk] Patch cable overlay misaligned on page 4** → `syncRoutesFromHost()` already runs after `resized()`; verify Drive panel jacks after column move.
- **[Risk] User expects hardware page order** → SIM_MANUAL note: desktop columns optimize for signal flow; Field SW1/SW2 order unchanged.
- **[Trade-off] Delay column not between Filter and Reverb** → Matches sidecar semantics (insert after filter DSP, before reverb send) without splitting the three primary FX pages across Delay.

## Migration Plan

No state migration. One-line constant change plus doc update; ship in next desktop release.

## Open Questions

None — full output FX order verified in `FroggersEngine::ProcessSample` and `ApplyOutputFx`.
