## Context

Pair-sum A/R parameters live in `AudioPairArState` (four knobs + mod routes), owned by both `PagedHostIO` (WASM/web) and `DesktopHostIO` (JUCE). Randomize today:

```
Action                    Page rows    Pair-AR knobs    Pair-AR mod    Delay knobs
─────────────────────────────────────────────────────────────────────────────────
Page Randomize (audio)    ✓            ✗                ✗              N/A
Page Randmod (audio)      ✓            N/A              ✗              N/A
Global Rand All           ✓            ✗                N/A            ✓
Global Rand Mods          ✓            N/A              ✓ (global)     ✓
```

Delay FX uses the same pattern we need: `DelayState::randomizeKnobs()` / `randomizeMod()`, with page-level mutations on desktop and `WasmSimHost::randomizeAllIncludingDelay()` for global knobs. Pair-AR was added without `randomizeKnobs()` and without host wiring — an incomplete port of the Delay pattern.

Desktop Audio panel **Randomize** → `EnqueueRandomizePanel(0)` → `PageManager::RandomizePage(0)` only. Web Audio page Rand → `froggers_randomize_page(host, 0)` → `PagedHostIO::RandomizePage(0)` only. UI refresh paths already read pair-AR state (`SubModulePanel::refresh`, web `postScreen()`); no frontend work once DSP state updates.

## Goals / Non-Goals

**Goals:**

- One randomize pipeline for pair-AR across desktop and WASM
- Match Delay semantics: page actions affect that panel’s extras; global Rand All includes all non-page-manager knob sets on the Audio/Delay surfaces
- Unit-test coverage for each entry point
- Use `AudioPairArLayout::kAudioHostPage` (0) as the page gate — not magic literals scattered in hosts

**Non-Goals:**

- New randomize buttons or UI layout
- VCV Rack / firmware
- Changing what “Rand Mods” randomizes globally (pair-AR mod already included)
- Separate WASM bindings like `froggers_pair_ar_randomize_knobs` (host orchestration suffices, same as page rows)

## Decisions

### 1. Add `randomizeKnobs()` on `AudioPairArState`

Mirror `DelayState::randomizeKnobs()`: loop `kCount`, `RGen::UniGenRange(0,1)`, call `setKnob(i, value)` so smoothers update.

**Alternative:** Inline loop at each call site — rejected (repetition / OMNI).

### 2. Shared orchestration in `sim/HostRandomize.hpp`

Free functions (inline) called from both hosts:

| Function | Behavior |
|----------|----------|
| `RandomizePageWithExtras(pm, page, pairAr, bridge)` | `pm.RandomizePage(page)`; if `page == kAudioHostPage`, `pairAr.randomizeKnobs()` |
| `RandomizePageModWithExtras(pm, page, pairAr, bridge)` | `pm.RandomizePageModSim(page, bridge)`; if audio page, `pairAr.randomizeMod(bridge)` |
| `RandomizeAllPagesWithPairAr(pm, pairAr)` | `pm.RandomizeAllPages()` + `pairAr.randomizeKnobs()` |

`PagedHostIO::RandomizePage/Mod/AllPages` call these instead of raw `PageManager` only.

`DesktopHostIO::applyMutation` for `RandomizePage`, `RandomizePageMod`, `RandomizeAllPages` call the same helpers. `RandomizeAllPages` keeps existing `m_delay->randomizeKnobs()` after page + pair-AR.

**Alternative:** Duplicate `if (page == 0)` in two files — rejected (OMNI repetition).

**Alternative:** Make `DesktopHostIO` inherit/delegate to `PagedHostIO` — out of scope; too large a refactor.

### 3. WASM global Rand All

`froggers_randomize_all_pages` → `WasmSimHost::randomizeAllIncludingDelay()` → `io.RandomizeAllPages()` + `delay.randomizeKnobs()`.

After decision 2, `RandomizeAllPages()` includes pair-AR; `randomizeAllIncludingDelay` needs no new line for pair-AR (Delay stays explicit, same as today).

### 4. Web / desktop UI

No `main.ts` branch for pair-AR (unlike Delay’s `delayRandomizeKnobs`). Audio page Rand already sends `randomizePage` with `hostPage === 0`.

Desktop `SubModulePanel::refresh()` already updates pair-AR sliders from backend after mutations drain.

### 5. Testing

Add sim unit test (e.g. extend existing host test or new `HostRandomizeTest.cpp`):

- Seed or snapshot knob values before/after each action
- Assert at least one pair-AR knob changes (or all four differ from prior with high probability — prefer deterministic mock `RGen` if available; otherwise assert not all equal to init 0.5)

Cover: page randomize audio, page randmod audio, global all pages (via `PagedHostIO`).

## Risks / Trade-offs

- **[Risk] Double randomize if caller and helper both touch pair-AR** → Mitigation: only helper mutates pair-AR; remove any ad-hoc calls elsewhere
- **[Risk] Non-audio page Randomize accidentally hits pair-AR** → Mitigation: gate on `kAudioHostPage` only
- **[Risk] Desktop vs WASM `RandomizeAllPages` naming** — desktop uses `RandomizeAllPagesIndependent()` in mutation, WASM uses `RandomizeAllPages()` — helper wraps the correct manager method per call site; document in tasks

## Migration Plan

1. Implement state + helper + host wiring
2. Run sim unit tests
3. Manual smoke: desktop Audio Randomize / Randmod / global strip; web same on Audio page
4. Rebuild WASM for web deploy

Rollback: revert single commit; no preset/schema migration.

## Open Questions

- None blocking — page index authority is `AudioPairArLayout::kAudioHostPage`.
