## Context

Desktop v2 splits UI state (`FroggersV2ControlCore`) from audio host state (`DesktopHostIO` inside `AudioEngine`). `FroggersV2HostBridge::syncToHost()` pushes visible-row knob values through `HostParameterRoutingV2::applyPageKnob`, then writes global Crunchy:

```92:108:desktop-v2/Source/control/FroggersV2HostBridge.cpp
void FroggersV2HostBridge::syncToHost()
{
    DelayState fallbackDelay;
    DelayState& delay = m_host.m_delay != nullptr ? *m_host.m_delay : fallbackDelay;
    const uint8_t page = m_core.activePage();
    for (uint8_t slot = 0; slot < m_core.visibleCount(); ++slot)
    {
        // ...
        HostParameterRoutingV2::applyPageKnob(page, row, effective.effective, m_host, delay);
    }

    m_host.SetGlobalCrunchy(m_core.globalCrunchy());
```

Carousel page changes call the same sync path via `desktop_v2::pushSelectPage`.

## Root cause (verified)

### 1. Dangling `HostCallbackContext` reference

```25:27:desktop-v2/Source/DesktopV2HostCallbacks.cpp
void wireCallbacks(const HostCallbackContext& ctx)
{
    ctx.carousel.onPageChanged = [&ctx](uint8_t page) { pushSelectPage(ctx, page); };
```

```85:88:desktop-v2/Source/MainComponent.cpp
void MainComponent::wireCallbacks()
{
    desktop_v2::wireCallbacks({m_core, m_bridge, m_audio->getHost(), m_carousel, m_lastModRoutesVersion});
}
```

The braced `HostCallbackContext` is a **temporary** destroyed when `wireCallbacks` returns. Lambdas stored on `PageCarouselComponent` hold `&ctx` to freed stack memory.

### 2. Boot sequence triggers the dangling callback immediately

```74:77:desktop-v2/Source/MainComponent.cpp
    wireCallbacks();
    wireMidiCvCallbacks();
    pushSelectPage(0);
    m_carousel.setActivePage(0);
```

- First `pushSelectPage(0)` uses a **valid** temporary context (stack frame of `pushSelectPage`).
- `setActivePage(0)` calls `onPageChanged` even when page is already 0:

```73:76:desktop-v2/Source/ui/PageCarouselComponent.cpp
    if (onPageChanged)
    {
        onPageChanged(m_page);
    }
```

That second `pushSelectPage` reads **garbage** `DesktopHostIO&` from the dangling `ctx`, producing an invalid `this` in `SetGlobalCrunchy` (lldb observed `x0` values like `0xb0d800128` while valid `AudioEngine::m_host` addresses look like `0x…00128` on the same run).

### 3. What this is NOT

- **Not** a wrong `m_globalCrunchy` clamp implementation — `SetGlobalCrunchy` is a three-line assign (`DesktopHostIO.hpp:360-363`).
- **Not** the pre-fix v1 `SetPageKnob` mod-index path alone — knob routing fix is already on the fork; crash persists after that fix.
- **Not** proven: ASAN heap corruption inside `KnobUpdate` — no ASAN run yet; lower priority than dangling ref given boot ordering.

## Design

### A. Stable callback binding (required)

Store `HostCallbackContext` on `MainComponent` and `HostedMainComponentV2` with lifetime matching the window. `wireCallbacks` lambdas SHALL capture a **pointer by value** to the context object (`[ctxPtr = &ctx]` with `pushSelectPage(*ctxPtr, …)`), not `[&ctx]` on the `wireCallbacks` reference parameter — in C++, `[&ctx]` binds to the parameter's lifetime, which ends when `wireCallbacks` returns, even when `ctx` aliases a member.

```cpp
// MainComponent.h — add member
desktop_v2::HostCallbackContext m_hostCallbacks;

// DesktopV2HostCallbacks.hpp — add (2 callers → OMNI repetition trigger met)
void refreshAndWireHostCallbacks(HostCallbackContext& ctx,
                                 froggers_v2::FroggersV2ControlCore& core,
                                 froggers_v2::FroggersV2HostBridge& bridge,
                                 DesktopHostIO& host,
                                 PageCarouselComponent& carousel,
                                 uint32_t& lastModRoutesVersion);

// MainComponent.cpp — wireCallbacks
desktop_v2::refreshAndWireHostCallbacks(m_hostCallbacks,
                                        m_core, m_bridge, m_audio->getHost(),
                                        m_carousel, m_lastModRoutesVersion);
```

`refreshAndWireHostCallbacks` assigns all refs into `ctx`, then calls `wireCallbacks(ctx)`. Both host components use this entry point — no duplicated braced-init blocks.

Update `wireCallbacks` lambdas to `[ctxPtr = &ctx]` (pointer by value). Callers pass `m_hostCallbacks` by reference so `ctxPtr` addresses the member. Apply to `onRandomize` / `onRandomizeMod`.

`MainComponent::pushSelectPage` and `pushRandomizeMod` (and hosted equivalents) SHALL pass `m_hostCallbacks` — not a fresh braced temporary — so every host-sync path reads the same context object.

Apply the same pattern in `HostedMainComponentV2` (verified duplicate at L36–38, L27–29).

### B. Single boot page-select (required)

Boot sequence on both hosts:

1. `pushSelectPage(0)` — core `SelectPage` + `syncToHost` + `carousel.refresh`.
2. `m_carousel.selectPage(0, false)` — UI visibility only; no `onPageChanged`.

Add `bool fireCallback = true` parameter to `PageCarouselComponent::selectPage`; guard `onPageChanged` behind it. `setActivePage(page)` delegates to `selectPage(page)` with default `true` (user clicks unchanged).

### C. Boot smoke gate + callback lifetime test (required)

1. **`BootSmoke_test.cpp`**: build standalone binary path from env or skip if not built; launch `FroggersTigaV2` subprocess, sleep ≤3s, assert process alive, `kill`.
2. **`CallbackLifetime_test.cpp`**: `HostCallbackContext` stored as a test fixture member (outlives `refreshAndWireHostCallbacks`); wire carousel callbacks; invoke `onPageChanged` after wiring returns; assert host sync ran without fault. Test fails on pre-fix `[&ctx]` capture (dangling ref); passes after stable-pointer capture.

Document commands in `tasks.md` for manual QA.

### D. Quick Dict (required, docs delta only)

`QUICK_DICT.md` already documents Engine/Stop (§Transport), Shift (§Shift), gates (§Gates), and Rand actions (§Transport). **Add only:**

| Topic | Content |
|-------|---------|
| Boot outcome | Standalone v2 window opening and staying open is expected; instant exit indicates a build/runtime fault |
| Page carousel nav | Left/right arrow buttons change module page; Rand / Rand Mod on carousel header (cross-ref §Transport Rand lines) |

Run `scripts/sync-help-docs.sh` after edits. Do **not** change `**Release v1.0.4**`.

## Data flow (fixed boot)

```
MainComponent ctor
  → refreshAndWireHostCallbacks(m_hostCallbacks, core, bridge, host, carousel, version)
  → pushSelectPage(0)                         // uses m_hostCallbacks; one syncToHost
  → m_carousel.selectPage(0, fireCallback=false)  // UI only
  → window visible, process alive
```

## Risks

| Risk | Mitigation |
|------|------------|
| `HostedMainComponentV2` duplicates bug | Same `m_hostCallbacks` + `refreshAndWireHostCallbacks` + boot sequence (tasks 1.4, 2.3; spec requirement) |
| Plugin editor path differs | `HostedMainComponentV2.cpp` L27–29 mirrors `MainComponent.cpp` L74–77 — fix both in same PR |
| `m_audio` optional re-seat | `m_hostCallbacks.host` must be refreshed if `AudioEngine` ever re-created (not today; no defensive refresh in this change) |

## OMNI compliance map (verified violations → fix location)

| Violation | Type | Location | Fix | Task |
|---|---|---|---|---|
| `[&ctx]` on `wireCallbacks` parameter | Data flow / lifetime | `DesktopV2HostCallbacks.cpp` L27–37 | Capture `HostCallbackContext*` to member | 1.2 |
| Temporary context at call site | Data flow | `MainComponent.cpp` L87, `HostedMainComponentV2.cpp` L38 | `m_hostCallbacks` + `refreshAndWireHostCallbacks` | 1.1, 1.5 |
| Duplicate host wiring blocks | Repetition | `MainComponent.cpp` L85–87, `HostedMainComponentV2.cpp` L36–38 | `refreshAndWireHostCallbacks` (2 callers) | 1.5 |
| Braced-init in `pushSelectPage` / `pushRandomizeMod` | Data flow | Both host `.cpp` push methods | Route through `m_hostCallbacks` | 1.6 |
| Boot double `syncToHost` | Data flow | `MainComponent.cpp` L76–77, `HostedMainComponentV2.cpp` L28–29 | `selectPage(0, false)` after `pushSelectPage(0)` | 2.1–2.2 |
| `onPageChanged` fires on unchanged page | Data flow | `PageCarouselComponent.cpp` L73–76 | `fireCallback` param; `false` at boot | 2.1 |
| GUI launch not gated | Verification | `desktop-v2/tests/` | `BootSmoke_test.cpp` | 4.1–4.2 |
| Dangling capture not unit-tested | Verification | — | `CallbackLifetime_test.cpp` | 4.3 |
| Doc scope overstated | Artifact drift | `proposal.md` original §What Changes | Delta: boot outcome + carousel nav only | 5.1 |

### Shared helper extraction (OMNI repetition — trigger ≥2 met)

| Helper | Trigger count | Boundary | Complexity | Contract | Callers |
|--------|---------------|----------|------------|----------|---------|
| `refreshAndWireHostCallbacks` | 2 (`MainComponent`, `HostedMainComponentV2`) | Host callback binding | Assign 5 refs + wire | In: ctx + live refs; out: ctx populated, carousel callables set | Both host `wireCallbacks()` |

Review enforcement: Trigger ≥2 **Yes** | Domain boundary **Yes** | Complexity **Yes** (assign + delegate) | Contract **Yes** | Side effects clear **Yes** (mutates ctx + carousel) | Local scope **Yes** (`desktop_v2` namespace).

### Implementation nesting

`wireCallbacks`, `pushSelectPage`, `refreshAndWireHostCallbacks` stay flat (≤2 levels). No new UI handler nesting.

### Compliant by design

- **`selectPage(..., fireCallback)`**: parameter on existing method, not a new abstraction; default `true` preserves user-click behavior.
- **No ASAN defensive branches**: dangling ref is the proven root cause; ASAN run is optional follow-up, not blocking.
- **Imports**: changes stay in existing translation units; no new dead code.
