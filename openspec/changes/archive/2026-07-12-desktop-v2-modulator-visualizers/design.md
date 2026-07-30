## Context

`CvScopeDisplay` is a generic multi-trace CV widget (`kMaxTraces = 4`). `GlobalOscilloscopeDisplay` projects ≤4 lanes from the 15-lane permanent catalog and currently samples with `m_host->GetCvOut(modIndex)`. Detail underlays need histories for all 15 lanes.

## Goals / Non-Goals

**Goals:**

- One UI-thread CV history authority for lanes 0..14.
- Single `GetCvOut` push loop per tick; scope and underlays consume the store.
- Display-only underlay under detail-grid lane encoders; Target (Back) null; hit geometry unchanged.

**Non-Goals:**

- Module-row MOD LED / signed-bias polish.
- Raising global strip to 15 simultaneous traces.
- Sheaf portable `Visualizer` / DrawCommand / runtime-shell port.
- Patch persistence of visualizer pointers.

## Decisions

### D1 — Accessor (Packet 0 — pinned)

Pinned: `DesktopHostIO::GetCvOut(uint8_t modIndex)` (same as `GlobalOscilloscopeDisplay::refreshTraces` today). Shells already bind that host into the oscilloscope (`MainComponent` / `HostedMainComponentV2` → `m_globalOscilloscope.bindHost(...)`).

### D2 — `CvLaneHistoryStore`

Header-only store: 15 rings of `CvScopeDisplay::kBufferSize`. Owns samples. Encoders hold non-owning views.

### D3 — Scope consumes store

After the push loop, `refreshTraces` uses `store.latest(binding.modIndex)` (or equivalent) and must not call `GetCvOut`.

### D4 — Underlay stacking

Paint underlay first inside `EncoderRingComponent`, then existing arcs/dot/MOD. Greyed lanes apply the same availability alpha. Degenerate min≈max → midline at 0.5.

### D5 — Shared bind helper

`bindDetailUnderlays` used by Submodule + Adsr (2 call sites). Clear all when detail closed; clear Target; bind lane cells from store + manifest color.

## Data flow

```
Host.GetCvOut(lane) → CvLaneHistoryStore (15 rings)
  → GlobalOscilloscopeDisplay (≤4 traces via pushSample(latest))
  → bindDetailUnderlays → EncoderRing underlay paint
```

## Risks

- Stale underlay pointers if store moves: store is shell-owned, address-stable for app lifetime.
- Scope visual regression if store not pushed before refresh: order is push-all then refreshTraces then panel refresh.
