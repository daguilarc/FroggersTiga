## Context

```
Today:

  ┌─ desktop (JUCE standalone) ─────────────────────────────────────┐
  │ 6× SubModulePanel │ ModRackPanel │ PatchCableOverlay │ strip   │
  │ DesktopHostIO + DelayState + FroggersEngine (MIT)               │
  └─────────────────────────────────────────────────────────────────┘

  ┌─ web (WASM) ────────────────────────────────────────────────────┐
  │ PagedHostIO + WasmSimHost + DelayState + FroggersEngine         │
  └─────────────────────────────────────────────────────────────────┘

  ┌─ vcv/ (GPL, local-only) ────────────────────────────────────────┐
  │ PagedHostIO + DelayState + block ProcessBlock on primary module   │
  │ 72 HP primary: mod rack LEDs + I/O; 72 HP expander: six submodule columns │
  │ Expander links via Rack left/right expander API                   │
  └─────────────────────────────────────────────────────────────────┘

  ┌─ VST ───────────────────────────────────────────────────────────┐
  │ FroggersTigaPlugin (VST3/AU) — MainComponent + DesktopHostIO      │
  │ Preset snapshot + bypass; full mod rack with VCO Envelope scope   │
  └─────────────────────────────────────────────────────────────────┘
```

Field hardware shows **one page at a time**; desktop sim shows **all six submodule regions at once** for patching. This change targets **desktop layout parity**, not single-page Field LCD behavior.

**OMNI constraints:** one engine (`FroggersEngine`), one label table (`ParamDisplayNames`), accumulate UI mutations then apply to host—no per-knob duplicate routing tables.

**Related change:** `sim-pm3-knob-parity` — Audio row 7 = **Phase mod 3** on sim; **VCO Envelope** is mod source 4 only (mod rack scope/LED, not row 7). VCV labels must use `ParamDisplayNames::forHostPageRow(0, 6)`.

## Goals / Non-Goals

**Goals:**

- VCV module stack: prefer **two Rack rows** (row 1 mod rack + I/O, row 2 six columns) when mockup fits; fall back to **three rows** (3+3 column split on expanders A/B).
- Every submodule row exposes: label, knob, **mod input jack** (48 row jacks + 4 mod outputs + audio/CV/MIDI). **VCV:** mod rack LED-only (no scopes). **VST:** full desktop mod rack including VCO Envelope CV scope — resizable plugin window.
- `PagedHostIO` gains page-indexed APIs so VCV expander columns match `DesktopHostIO` routing semantics.
- VCV shares one engine + `DelayState` across expander modules.
- Shared layout constants and mod index mapping—single source, three hosts read it.

**Non-Goals:**

- Web sim widget changes (web keeps existing scope canvas for VCO Envelope; label dedupe is `sim-pm3-knob-parity`).
- Firmware / Daisy Field hardware UI changes.
- Oscilloscope / CV trace widgets on VCV field-parity panels (LED only there).
- Replacing `PatchCableOverlay` drag UX in VCV with nanovg cables in v1 (VCV uses physical jacks; desktop keeps overlay).
- Publishing `vcv/` to GitHub (stays local per repo policy).
- CLAP/LV2 in v1 (VST3 + AU only on JUCE path).

## Decisions

### D1 — VST reuses desktop UI verbatim

**Choice:** Add `juce_add_plugin(FroggersTigaPlugin …)` linking existing `desktop/Source/*` + `MainComponent`.

**Why:** Zero duplicate JUCE widgets; OMNI reuse. Standalone app remains separate target sharing sources.

**Alternative rejected:** New slim plugin UI — duplicates `SubModulePanel`, violates repetition rule.

### D2 — VCV widget is new nanovg layout, shared semantics only

**Choice:** Implement `vcv/src/widgets/FieldParityWidget` driven by extracted `HostPanelLayout` tables (HP positions, row count, mod indices)—not port JUCE components.

**Why:** Rack SDK widget API is incompatible with JUCE; sharing `IPanelBackend`-style adapters and constants is the boundary.

### D3 — Module layout: ~72 HP × 2–3 Rack rows (expander stack)

**Choice:** Target **72 HP wide**. Rack 2 enforces **exactly `RACK_GRID_HEIGHT` (380 px) per module** — a single widget cannot span multiple Eurorack rows.

**Preferred (2 rows):**

| Rack row | Module | Contents |
|----------|--------|----------|
| Row 1 (top) | **FroggersTiga** (primary) | Mod rack: MIDI, VCO Envelope, Random 1/2 **output jacks** + **LED indicators**; Random button; master audio/CV/MIDI/gate I/O |
| Row 2 | **FroggersTiga Expander** | All six submodule columns — 8 knobs + 8 mod input jacks each |

**Fallback (3 rows)** if six columns do not fit at readable density — **implemented** in `vcv-rack-panel-layout-fix` (v2.3.0):

| Rack row | Module | Contents |
|----------|--------|----------|
| Row 1 | Primary | Mod rack + I/O (same as above) |
| Row 2 | Expander A | Audio, Random, Reverb |
| Row 3 | Expander B | Filter, Drive, Delay |

All modules share one `PagedHostIO` + `DelayState` via Rack **expander** left/right linking.

**Alternative rejected:** `box.size.y = 3 * RACK_GRID_HEIGHT` on one module — Rack 2 `addModule()` throws.

Current stub widget uses invalid `box.size.y = 16 * RACK_GRID_HEIGHT` — must fix to `RACK_GRID_HEIGHT` per module.

### D3b — Mod rack indicators: VCV LED-only; VST keeps scopes

**Choice:** **VCV** mod rack uses **LED threshold indicators only** for mod sources 4–6 (no `CvScopeDisplay`). **VST/AU** reuses desktop `MainComponent` **verbatim**, including the VCO Envelope CV scope — resizable editor (~1440×720 minimum).

**VCO Envelope scope semantics (desktop/web/VST):** traces **mod CV** from `m_mods[4]` — slow envelope of |VCO mix|, not audio waveform. See `UpdateM5FromVco` in `FroggersEngine.hpp`.

**VCV LED policy:** green when CV > 55% while processing (same threshold as Random LEDs); continuous envelope means LED reflects level, not S&H steps.

### D4 — Host IO: extend PagedHostIO before VCV widget

| Host | IO struct | Delay FX | All-pages-at-once APIs | UI framework |
|------|-----------|----------|------------------------|--------------|
| Desktop standalone | `DesktopHostIO` | `DelayState` ✓ | `SetPageKnob(page,…)` ✓ | JUCE |
| VST/AU | `DesktopHostIO` (same) | `DelayState` ✓ | ✓ | JUCE |
| Web | `PagedHostIO` + `WasmSimHost` | `DelayState` ✓ | wasm per-page bindings | TS |
| VCV (target) | `PagedHostIO` + `DelayState` | **add** | **add page-indexed APIs** | Rack widget |

**OMNI audit gap (must fix in Phase A):** `PagedHostIO` today only has `SetRowModSource` / `SetKnob` for the **current page**. It lacks `SetPageKnob`, `SetPageModSource`, `GetPageParam`, and returns firmware names from `GetRowName` (`OLVL`) instead of `ParamDisplayNames`.

**Choice:** Extend `PagedHostIO` in MIT `src/core/` with the same page-indexed surface `DesktopHostIO` exposes for knobs and mod routing. VCV widget calls those APIs directly—no duplicate routing table in `vcv/`.

### D5 — VCV DelayState parity

**Choice:** VCV expander stack owns one `DelayState` + `SetSimFxInsert(simDelayInsertCallback, &delay)` on the shared `FroggersEngine`, matching `WasmSimHost` and desktop `AudioEngine`.

**Why:** Delay column on desktop uses `DelayHostBackend` → `DelayState`, not engine page params alone. VCV Delay column without this sidecar will not match desktop timbre.

### D6 — Extract layout tables before VCV widget work

**Choice:** Move numeric layout from `DesktopChromeLayout.hpp` + mod box widths into `sim/HostPanelLayout.hpp` (header-only, MIT). Include `ParamDisplayNames` references for labels. Desktop includes it; VCV maps grid units → HP via scale factor.

### D7 — VCV audio processing: block ProcessBlock

**Choice:** Replace per-sample `ProcessBlock(..., n=1)` in `vcv/src/plugin.cpp` with rack block accumulation (fill buffer per `ProcessArgs`, one `ProcessBlock` per block).

**Why:** OMNI accumulate-then-apply; matches desktop/web; reduces CPU overhead with 48+ params.

### D8 — Phased delivery (reordered after OMNI audit)

1. **Phase A:** `HostPanelLayout` extraction + **`PagedHostIO` page-indexed APIs + DelayState on VCV engine** + VST plugin boots with audio passthrough.
2. **Phase B:** VCV widget scaffold at target size; Audio column complete (labels from `ParamDisplayNames`, row 7 = Phase mod 3).
3. **Phase C:** Remaining five columns + mod rack jacks + routing regression vs desktop.

**Prerequisite:** Apply `sim-pm3-knob-parity` doc/label fixes before Phase B Audio column ships.

## Risks / Trade-offs

| Risk | Mitigation |
|------|------------|
| VCV widget built before host APIs | Phase A gate: page-indexed APIs + DelayState land before Phase B |
| 72 HP × six columns cramped | Paper mockup; fall back to 3-row 3+3 split |
| 48+ jacks clutter panel | Compact PJ301M grid; labels from `ParamDisplayNames` |
| GPL/MIT boundary breach | No `#include <rack.hpp>` outside `vcv/` |
| JUCE plugin + standalone symbol clash | Separate CMake targets, shared static lib `FroggersTigaUI` |
| VCV CPU with per-frame param polling | Params update on change only; block ProcessBlock |
| Nesting depth in widget layout | Pre-split `layoutSubmoduleColumn()` helpers (OMNI max 4) |
| PM3 vs VCO Envelope user confusion | `sim-pm3-knob-parity` glossary; row 7 ≠ mod source 4 |

## Migration Plan

1. Apply `sim-pm3-knob-parity` (docs + label authority).
2. Extend `PagedHostIO`; wire VCV `DelayState`; land `HostPanelLayout` header.
3. Add VST target; verify in Reaper/Logic.
4. VCV widget submodule-by-submodule; tag local release `vcv-field-parity-v1`.
5. Rollback: VCV keeps stub on branch; VST target optional in CMake `BUILD_VST=OFF`.

## Appendix: Mod index → jack mapping

| Mod index | `ParamDisplayNames` | Desktop indicator | VCV indicator | Output voltage |
|-----------|---------------------|-------------------|---------------|----------------|
| 0 | (MIDI) | MIDI box | MIDI jack | MIDI |
| 4 | VCO Envelope | CV scope | Green LED | 0–10 V |
| 5 | Random 1 S&H | Green LED | Green LED | 0–10 V |
| 6 | Random 2 S&H | Green LED | Green LED | 0–10 V |

Sim Audio row 7 (index 6): **Phase mod 3** — knob on submodule panel, not a mod rack output.

## Open Questions

1. Exact HP width after paper mockup — 60, 72, or 84?
2. Two-row (6 columns on one expander) vs three-row (3+3) — mockup decides.
3. VST: include desktop transport/recorder cluster or strip for plugin?
4. VCV external audio input jack — mirror desktop Ext. In. toggle or always-on?
