## Context

```
Current (no gating)
───────────────────
MIDI CC 1/2 always "live" in mod rack
  → mods[0/1] latch from CC / QWERTY
  → always patchable, always in random-mod pool
  → always in web assignableModOptions

Desired (full data flow)
────────────────────────
Enable flag per CC pair (CvMidiBridge authority)
  → PushMidiCc / QWERTY feed gated; latch zeroed on disable
  → drainMidiIn writes 0.0 to mods[0/1] when disabled
  → IsSimModSourceAvailable(modIndex, bridge)
       → assignment guards (SetPageModSource, delay setModSource, patch overlay)
       → PickSimRandomModIndex pool (50% None preserved, filter CC indices)
       → froggers_assignable_mod_count/index(host) filtered list
       → UI grey state (desktop mod rack, web mod bay)
  → SetMidiCcPairEnabled(false) → ClearModRoutesForIndex in one pass
```

**Authority today:** `CvMidiBridge` holds channel/CC config; `SimModSource.hpp` defines assignable indices `{0,1,4,5,6}`; wasm `froggers_assignable_mod_*` feeds web dropdowns; desktop uses `PatchCableOverlay` + `ModModuleBox` jacks.

**Hosts:** Desktop standalone + JUCE VST/AU (via `DesktopHostIO` + shared UI), Web (External MIDI gate + mod dropdown). **VCV Rack is out of scope** — separate ingest path in `vcv/src/plugin.cpp` bypasses `CvMidiBridge` enable flags.

## Goals / Non-Goals

**Goals:**

- Single source of truth for "is mod index 0/1 available?" in core/wasm.
- Desktop + VST: Enable toggle per CC row in MIDI Settings; grey mod rack column; non-interactive output jack.
- Web: CC1/CC2 enable toggles (External MIDI off ⇒ both off); grey mod bay scopes; dropdown rebuilt from host-scoped wasm availability.
- Random mod and route clearing enforced in core (not UI-only).
- QWERTY → CC1 respects CC1 enable.
- Preserve existing 50%-None random-mod semantics; filter only CC indices from the non-None pool.

**Non-Goals:**

- Disabling VCO Env or Random S&H mod sources.
- Persisting enable flags to disk (session-only is fine for v1).
- Web full MIDI Settings dialog (channel/CC sliders stay desktop-only; web keeps defaults ch1/CC1, ch1/CC2).
- VCV Rack CC ingest parity (separate change `vcv-cc-mod-gating`).
- VST External MIDI master gate (VST uses DAW MIDI; inherits desktop defaults + core gating).

## Decisions

### D1 — Enable flags live on `CvMidiBridge`

Add pair-indexed enable storage:

```cpp
static constexpr uint8_t kCcModIndices[] = {0, 1};

bool m_inCcEnabled[2] = {true, true};

bool isCcPairEnabled(uint8_t pairIndex) const
{
    return pairIndex < 2 && m_inCcEnabled[pairIndex];
}

bool isCcModIndexEnabled(uint8_t modIndex) const
{
    for (uint8_t i = 0; i < 2; ++i)
        if (kCcModIndices[i] == modIndex) return m_inCcEnabled[i];
    return false;
}
```

**Rationale:** Bridge owns ingest + latch; flags belong with pair config. Shared by desktop, wasm, JUCE VST/AU via `DesktopHostIO` / `PagedHostIO`.

**Ingest:** `PushMidiCc` returns early if the matched pair is disabled. On disable transition, zero the latch (`m_inCcLevel1/2`) in the setter.

**Drain:** `drainMidiIn` writes 0.0 to `mods[0/1]` when the pair is disabled.

### D2 — Availability helper in `SimModSource.hpp`

```cpp
inline bool IsSimModSourceAvailable(uint8_t modIndex, const CvMidiBridge& bridge)
{
    if (modIndex == 0 || modIndex == 1)
        return bridge.isCcModIndexEnabled(modIndex);
    return IsSimAssignableModIndex(modIndex);
}
```

Build random-mod pool preserving 50%-None semantics:

```cpp
inline uint8_t PickSimRandomModIndex(RGen& rgen, const CvMidiBridge& bridge)
{
    if (rgen.UniGen() < 0.5f)
        return 255;
    uint8_t pool[5];
    uint8_t count = 0;
    for (uint8_t idx : {0, 1, 4, 5, 6})
        if (IsSimModSourceAvailable(idx, bridge)) pool[count++] = idx;
    if (count == 0)
        return 255;
    return pool[rgen.RangeGen(count)];
}
```

**Alternative rejected:** Filter only in UI — random mod and stale routes would still leak disabled sources.

**Plumbing:** Thread `CvMidiBridge&` through `Parameter::RandomizeModSim`, `PageManager::RandomizePageModSim` / `RandomizeAllPagesModSim`, `DelayState::randomizeMod`, and `DesktopHostIO::applyMutation` random-mod paths.

### D3 — Route clearing on disable (core mutation)

When `SetMidiCcPairEnabled(pairIndex, false)`:

1. Set flag + zero latch for that pair.
2. Call `ClearModRoutesForIndex(kCcModIndices[pairIndex])` — one pass over all pages + Delay: any matching `modIndex` → set 255, depth 0.

Implement on both `PagedHostIO` and `DesktopHostIO` (each owns its own `m_midiBridge`).

**OMNI:** One pass over pages accumulates clears; apply in place (reuse existing page/delay setters).

### D4 — Assignment guard at host boundary

`SetPageModSource`, `DelayState::setModSource`, `SetRowModSource`, and `PatchCableOverlay::setModSource` path:

- If `!IsSimModSourceAvailable(modIndex, bridge)` → no-op (existing route unchanged).

Remove `SetRowModSource` modIndex==0 guard that blocks CC1 web assignment.

Desktop patch overlay skips disabled outputs in `hitOutputPort` and rejects `finishDrag` to disabled `modIndex`.

### D5 — Desktop UI

**MidiSettingsComponent:** Add `ToggleButton` "On" per CC row (bound to `SetMidiCcPairEnabled` via AudioEngine/host). Increase dialog height to fit toggles. Toggling off triggers D3 clear + mod rack refresh.

**ModModuleBox:** `setPatchEnabled(bool)` — when false: label/scope/jack at ~40% opacity, desaturated jack ring.

**ModRackPanel::refresh:** Derive enable state from mod index in existing box loop (`modIndex 0/1 → bridge.isCcModIndexEnabled`).

**PatchCableOverlay:** `OutputPort` gets `bool patchEnabled` from box; filter in `hitOutputPort`, `paint` (grey ring). On disable transition, invalidate cable hues for that mod index.

### D6 — Wasm + web parity

New exports:

```cpp
int froggers_mod_source_available(WasmSimHost* host, int modIndex);
void froggers_set_cc_pair_enabled(WasmSimHost* host, int pairIndex, int enabled);
int froggers_cc_pair_enabled(WasmSimHost* host, int pairIndex);
```

Change assignable API to host-scoped:

```cpp
int froggers_assignable_mod_count(WasmSimHost* host);
int froggers_assignable_mod_index(WasmSimHost* host, int index);
```

Both filter through `IsSimModSourceAvailable`.

**Web transport row** (under External Audio / External MIDI):

```
[External MIDI: Off]
[CC 1: Off] [CC 2: Off]   ← disabled until External MIDI on; user can turn each off independently
```

Flow:

- External MIDI Off → `froggers_set_cc_pair_enabled(0,0)` + `froggers_set_cc_pair_enabled(1,0)`, refresh UI.
- External MIDI On → set both enables true (defaults), user may toggle CC1/CC2 off individually.

**main.ts:**

- `applyModBayAvailability(flags)` adds `.mod-disabled` CSS to scope/LED containers.
- On enable change: call wasm setters, request assignable list refresh from worklet (`postMessage` → re-post `assignableModOptions`), repopulate selects.

**froggers-processor.ts:** After `set_cc_pair_enabled`, post `{ type: "modAvailabilityChanged", ... }`.

### D7 — Web vs desktop default semantics

| Host | CC1/CC2 default | User control |
|------|-----------------|--------------|
| Desktop / VST | Both On | MIDI Settings Enable per row |
| Web | Both Off until External MIDI On | CC1/CC2 toggles + External MIDI master |

Computer keyboard on desktop drives CC1 only; CC1 Enable off blocks QWERTY in `feedComputerKeyboardCc1`.

## Risks / Trade-offs

- **[Risk] UI-only disable without core clear** → Mitigation: D3 + D4 enforce in host IO.
- **[Risk] Web/desktop default mismatch confuses users** → Mitigation: document in SIM_MANUAL; web CC toggles visible only when External MIDI on.
- **[Risk] Stale cables after disable** → Mitigation: core clears routes; overlay hue invalidation + repaint on refresh timer.
- **[Risk] Random-mod bridge plumbing missed** → Mitigation: explicit tasks for PageManager/DelayState call-chain refactor.

## Migration Plan

1. Land core flags + availability helpers + route clearing on both host IO types.
2. Thread `CvMidiBridge&` through random-mod call sites.
3. Wire desktop UI + patch overlay.
4. Extend wasm bindings (host-scoped assignable API) + web toggles/CSS.
5. Manual verify: disable CC2 → grey column, cable blocked, Rand Mods never picks CC2, existing CC2 routes cleared.
6. Update SIM_MANUAL + sync help docs.

## Open Questions

- None blocking v1. Persistence of enable flags is deferred.
